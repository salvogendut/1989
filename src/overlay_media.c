/* Native picker handoff and disk-image operations, independent of row layout. */
#include "main.h"
#include "overlay_media.h"
#include "file.h"
#include "notify.h"
#include "sdlscreen.h"
#include <SDL3/SDL.h>
#include <string.h>

const char *OverlayMedia_ScsiRole(int id) {
    static const char *const roles[ESP_MAX_DEVS] = {
        "Alternate boot", "System disk", "Data disk", "CD-ROM",
        "External floppy", "Spare device", "Extra / swap disk"
    };
    return id >= 0 && id < ESP_MAX_DEVS ? roles[id] : "Host controller";
}

SCSI_DEVTYPE OverlayMedia_ScsiType(const CNF_PARAMS *p, int id) {
    if (id < 0 || id >= ESP_MAX_DEVS) return SD_NONE;
    SCSI_DEVTYPE type = p->SCSI.target[id].nDeviceType;
    if (type != SD_NONE) return type;
    return id == 3 ? SD_CD : id == 4 ? SD_FLOPPY : SD_HARDDISK;
}

static bool scsi_attached(const SCSIDISK *disk) {
    return disk->nDeviceType != SD_NONE &&
           (disk->nDeviceType != SD_HARDDISK || disk->bDiskInserted);
}

int OverlayMedia_ScsiDiskNumber(const CNF_PARAMS *p, int id) {
    if (id < 0 || id >= ESP_MAX_DEVS || !scsi_attached(&p->SCSI.target[id]))
        return -1;
    int number = 0;
    for (int i = 0; i < id; i++)
        if (scsi_attached(&p->SCSI.target[i])) number++;
    return number;
}

bool OverlayMedia_Eject(CNF_PARAMS *p, OvDialogKind kind) {
    if (kind >= OV_DIALOG_SCSI0 && kind <= OV_DIALOG_SCSI6) {
        SCSI_DEVTYPE type = p->SCSI.target[kind - OV_DIALOG_SCSI0].nDeviceType;
        if (type != SD_CD && type != SD_FLOPPY) {
            notify_post("EJECT IS FOR REMOVABLE MEDIA; DEL DISCONNECTS A FIXED DISK");
            return false;
        }
    } else if (kind < OV_DIALOG_FLOPPY0 || kind > OV_DIALOG_MO1) {
        return false;
    }
    return OverlayMedia_Set(p, kind, NULL);
}

bool OverlayMedia_Disconnect(CNF_PARAMS *p, OvDialogKind kind) {
    if (kind < OV_DIALOG_SCSI0 || kind > OV_DIALOG_MO1) return false;
    if (!OverlayMedia_Set(p, kind, NULL)) return false;
    if (kind <= OV_DIALOG_SCSI6)
        p->SCSI.target[kind - OV_DIALOG_SCSI0].nDeviceType = SD_NONE;
    else if (kind <= OV_DIALOG_FLOPPY1)
        p->Floppy.drive[kind - OV_DIALOG_FLOPPY0].bDriveConnected = false;
    else
        p->MO.drive[kind - OV_DIALOG_MO0].bDriveConnected = false;
    return true;
}

static SDL_SpinLock request_lock;
static struct {
    bool busy, ready, cancelled;
    OvDialogKind kind;
    long long size;
    char path[FILENAME_MAX];
} request;

static void selected(void *unused, const char *const *files, int filter) {
    (void)unused; (void)filter;
    SDL_LockSpinlock(&request_lock);
    snprintf(request.path, sizeof(request.path), "%s", files && files[0] ? files[0] : "");
    request.ready = true;
    SDL_UnlockSpinlock(&request_lock);
}

bool OverlayMedia_Busy(void) {
    SDL_LockSpinlock(&request_lock);
    bool busy = request.busy;
    SDL_UnlockSpinlock(&request_lock);
    return busy;
}

bool OverlayMedia_Request(OvDialogKind kind, const UI89Config *ui, long long size) {
    static const SDL_DialogFileFilter filters[] = {
        { "NeXT images", "sd;SD;fd;FD;dsk;DSK;img;IMG;bin;BIN;iso;ISO;od;OD" },
        { "All files", "*" }
    };
    if (kind <= OV_DIALOG_NONE || kind >= OV_DIALOG_COUNT) return false;
    SDL_LockSpinlock(&request_lock);
    if (request.busy) { SDL_UnlockSpinlock(&request_lock); return false; }
    memset(&request, 0, sizeof(request));
    request.busy = true;
    request.kind = kind;
    request.size = size;
    SDL_UnlockSpinlock(&request_lock);
    const char *dir = ui->szLastDir[kind];
    if (!dir[0] || !File_DirExists(dir)) dir = NULL;
    if (size > 0)
        SDL_ShowSaveFileDialog(selected, NULL, sdlWindow, filters, 2, dir);
    else
        SDL_ShowOpenFileDialog(selected, NULL, sdlWindow, filters, 2, dir, false);
    return true;
}

void OverlayMedia_Cancel(void) {
    SDL_LockSpinlock(&request_lock);
    request.cancelled = true;
    SDL_UnlockSpinlock(&request_lock);
}

bool OverlayMedia_Poll(OvDialogKind *kind, char *path, long long *size) {
    SDL_LockSpinlock(&request_lock);
    bool ready = request.busy && request.ready;
    if (ready) {
        *kind = request.cancelled ? OV_DIALOG_NONE : request.kind;
        *size = request.size;
        snprintf(path, FILENAME_MAX, "%s", request.path);
        request.busy = false;
    }
    SDL_UnlockSpinlock(&request_lock);
    return ready;
}

void OverlayMedia_Remember(UI89Config *ui, OvDialogKind kind, const char *path) {
    if (kind <= OV_DIALOG_NONE || kind >= OV_DIALOG_COUNT) return;
    char dir[FILENAME_MAX];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
#ifdef _WIN32
    char *backslash = strrchr(dir, '\\');
    if (backslash && (!slash || backslash > slash)) slash = backslash;
#endif
    if (!slash) return;
    if (slash == dir) slash[1] = '\0'; else *slash = '\0';
    if (File_DirExists(dir)) snprintf(ui->szLastDir[kind], FILENAME_MAX, "%s", dir);
}

bool OverlayMedia_Set(CNF_PARAMS *p, OvDialogKind kind, const char *path) {
    bool inserted = path && path[0];
    if (inserted && (!File_Exists(path) || File_DirExists(path))) {
        notify_post("IMAGE NOT FOUND");
        return false;
    }
    if (kind >= OV_DIALOG_SCSI0 && kind <= OV_DIALOG_SCSI6) {
        SCSIDISK *d = &p->SCSI.target[kind - OV_DIALOG_SCSI0];
        if (!inserted && d->nDeviceType == SD_HARDDISK) d->nDeviceType = SD_NONE;
        if (inserted && d->nDeviceType == SD_NONE)
            d->nDeviceType = OverlayMedia_ScsiType(p, kind - OV_DIALOG_SCSI0);
        if (d->nDeviceType == SD_CD) d->bWriteProtected = true;
        d->bDiskInserted = inserted;
        snprintf(d->szImageName, FILENAME_MAX, "%s", inserted ? path : "");
    } else if (kind == OV_DIALOG_FLOPPY0 || kind == OV_DIALOG_FLOPPY1) {
        FLPDISK *d = &p->Floppy.drive[kind - OV_DIALOG_FLOPPY0];
        if (inserted) {
            off_t size = File_Length(path);
            if (size != 737280 && size != 1474560 && size != 2949120) {
                notify_post("FLOPPY MUST BE 720 KB, 1.44 MB OR 2.88 MB");
                return false;
            }
            d->bDriveConnected = true;
        }
        /* Ejecting a disk must not disconnect its drive. */
        d->bDiskInserted = inserted;
        snprintf(d->szImageName, FILENAME_MAX, "%s", inserted ? path : "");
    } else if (kind == OV_DIALOG_MO0 || kind == OV_DIALOG_MO1) {
        MODISK *d = &p->MO.drive[kind - OV_DIALOG_MO0];
        if (inserted) d->bDriveConnected = true;
        d->bDiskInserted = inserted;
        snprintf(d->szImageName, FILENAME_MAX, "%s", inserted ? path : "");
    } else if (kind >= OV_DIALOG_ROM030 && kind <= OV_DIALOG_ROMTURBO) {
        if (!inserted) return false;
        char *rom = kind == OV_DIALOG_ROM030 ? p->Rom.szRom030FileName :
                    kind == OV_DIALOG_ROM040 ? p->Rom.szRom040FileName : p->Rom.szRomTurboFileName;
        snprintf(rom, FILENAME_MAX, "%s", path);
    } else return false;
    return true;
}

bool OverlayMedia_Create(const char *path, long long size, char *result) {
    if (size <= 0 || strlen(path) + 5 > FILENAME_MAX) return false;
    snprintf(result, FILENAME_MAX, "%s", path);
    const char *base = strrchr(result, '/');
    base = base ? base + 1 : result;
    if (!strchr(base, '.')) strcat(result, ".img");
    FILE *f = fopen(result, "wbx");
    if (!f) {
        notify_post("CANNOT CREATE IMAGE: CHOOSE A NEW FILE NAME");
        return false;
    }
    /* Sparse, zero-reading file: a multi-GB image must not block the UI while
     * gigabytes of zeros are written. Large-file support is set by configure. */
    bool ok = fseeko(f, (off_t)(size - 1), SEEK_SET) == 0 && fputc(0, f) != EOF;
    if (fclose(f)) ok = false;
    if (!ok) {
        remove(result);
        notify_post("COULD NOT CREATE IMAGE");
    }
    return ok;
}
