#include "ui_test_support.h"
#include "overlay_media.h"
#include "overlay_view.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static void key(SDL_Scancode sc) {
    SDL_Event ev;
    SDL_zero(ev); /* Initialize the whole union, including key.repeat. */
    ev.type = SDL_EVENT_KEY_DOWN; ev.key.scancode = sc;
    assert(overlay_handle_event(&ev));
}
static void down(int count) { for (int i = 0; i < count; i++) key(SDL_SCANCODE_DOWN); }

static void open_media(int row) {
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RIGHT); down(row);
}

static void save_media(void) {
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN); /* Default is Discard. */
    assert(!overlay_is_visible());
}

static void test_scsi_layout(const char *image) {
    static CNF_PARAMS p;
    const int ids[] = {1, 2, 3, 6};
    for (int i = 0; i < 4; i++) {
        assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI0 + ids[i], image));
        assert(OverlayMedia_ScsiDiskNumber(&p, ids[i]) == i);
    }
    assert(p.SCSI.target[3].nDeviceType == SD_CD && p.SCSI.target[3].bWriteProtected);
    assert(p.SCSI.target[0].nDeviceType == SD_NONE);
    assert(OverlayMedia_ScsiDiskNumber(&p, 7) == -1);
    assert(OverlayMedia_Eject(&p, OV_DIALOG_SCSI3));
    assert(OverlayMedia_ScsiDiskNumber(&p, 3) == 2); /* Empty CD drive is still attached. */
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI0, image));
    assert(OverlayMedia_ScsiDiskNumber(&p, 1) == 1); /* Alternate boot takes sd0. */
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI4, image));
    assert(p.SCSI.target[4].nDeviceType == SD_FLOPPY);
    /* Historical roles do not override an explicitly configured disk. */
    p.SCSI.target[3].nDeviceType = SD_HARDDISK;
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI3, image));
    assert(p.SCSI.target[3].nDeviceType == SD_HARDDISK);
    memset(&p, 0, sizeof(p));
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI1, image));
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI6, image));
    assert(OverlayMedia_ScsiDiskNumber(&p, 1) == 0);
    assert(OverlayMedia_ScsiDiskNumber(&p, 6) == 1);
    assert(OverlayMedia_ScsiDiskNumber(&p, 3) == -1);
}

static void test_removable_eject(const char *image) {
    int resets_before = restarts;
    /* Keep an existing boot disk at ID 0 as well as one at the suggested ID 1. */
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI0, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI1, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI3, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI4, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_MO1, image));
    CNF_PARAMS before = ConfigureParams;

    open_media(4); /* ID 3: headers and the reserved host ID are not selectable. */
    key(SDL_SCANCODE_H); key(SDL_SCANCODE_E); key(SDL_SCANCODE_ESCAPE);
    key(SDL_SCANCODE_E);
    assert(!memcmp(&ConfigureParams, &before, sizeof(before))); /* Still a draft. */
    save_media();
    assert(!ConfigureParams.SCSI.target[3].bDiskInserted);
    assert(ConfigureParams.SCSI.target[3].nDeviceType == SD_CD);
    assert(scsi_out[3] == 1 && scsi_in[3] == 1);
    assert(restarts == resets_before);

    /* Reloading an ejected CD also avoids a restart. */
    open_media(4); key(SDL_SCANCODE_RETURN);
    const char *files[] = {image, NULL};
    picker_callback(picker_userdata, files, 0); overlay_tick();
    save_media();
    assert(ConfigureParams.SCSI.target[3].bDiskInserted);
    assert(scsi_out[3] == 2 && scsi_in[3] == 2 && restarts == resets_before);

    /* Enter on a loaded drive opens a replacement picker; cancellation is a no-op. */
    open_media(4); key(SDL_SCANCODE_RETURN);
    assert(OverlayMedia_Busy());
    picker_callback(picker_userdata, NULL, 0); overlay_tick();
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && ConfigureParams.SCSI.target[3].bDiskInserted);

    open_media(5); key(SDL_SCANCODE_E); save_media(); /* SCSI floppy, ID 4. */
    assert(ConfigureParams.SCSI.target[4].nDeviceType == SD_FLOPPY);
    assert(!ConfigureParams.SCSI.target[4].bDiskInserted && scsi_out[4] == 1);
    open_media(8); key(SDL_SCANCODE_E); save_media(); /* Native floppy 0. */
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected);
    assert(!ConfigureParams.Floppy.drive[0].bDiskInserted && floppy_out[0] == 1);
    open_media(11); key(SDL_SCANCODE_E); save_media(); /* Native MO 1. */
    assert(ConfigureParams.MO.drive[1].bDriveConnected);
    assert(!ConfigureParams.MO.drive[1].bDiskInserted && mo_out[1] == 1);
    assert(!floppy_out[1] && !mo_out[0] && !floppy_in[0] && !mo_in[1]);
    assert(restarts == resets_before && scsi_out[3] == 2);
    assert(!scsi_out[0] && !scsi_in[0] && !scsi_out[1] && !scsi_in[1]);
    assert(!memcmp(&ConfigureParams.SCSI.target[0], &before.SCSI.target[0], sizeof(SCSIDISK)));
    assert(!memcmp(&ConfigureParams.SCSI.target[1], &before.SCSI.target[1], sizeof(SCSIDISK)));

    open_media(1); key(SDL_SCANCODE_E); key(SDL_SCANCODE_F9); /* Fixed HDD cannot be ejected. */
    assert(!overlay_is_visible() && !overlay_confirm_visible());
    open_media(4); key(SDL_SCANCODE_DELETE); save_media(); /* Disconnect is different. */
    assert(ConfigureParams.SCSI.target[3].nDeviceType == SD_NONE);
    assert(restarts == resets_before + 1);
}

int main(void) {
    ConfigureParams.System.nCpuFreq = 25;
    UI89Config_.bTinker = true;
    UI89Config_.nGifFps = 25;
    overlay_init();
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); /* Media, boot device. */
    key(SDL_SCANCODE_RETURN);
    assert(ConfigureParams.Boot.nBootDevice == BOOT_ROM);
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_RIGHT); /* Discard. */
    key(SDL_SCANCODE_RETURN);
    assert(!overlay_is_visible() && !saved && !restarts);
    assert(ConfigureParams.Boot.nBootDevice == BOOT_ROM);

    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Save boot change. */
    assert(saved == 1 && ConfigureParams.Boot.nBootDevice == BOOT_SCSI && !restarts);

    key(SDL_SCANCODE_F9);
    down(2); key(SDL_SCANCODE_RETURN); /* Clock is staged, requires restart. */
    assert(ConfigureParams.System.nCpuFreq == 25);
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_ESCAPE); /* Return to editing with the draft intact. */
    assert(overlay_is_visible() && !overlay_confirm_visible());
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Default is Discard. */
    assert(ConfigureParams.System.nCpuFreq == 25 && restarts == 0 && saved == 1);

    key(SDL_SCANCODE_F9);
    down(2); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN);
    assert(ConfigureParams.System.nCpuFreq == 33 && restarts == 1 && saved == 2);
    overlay_confirm_reset(); key(SDL_SCANCODE_RETURN); /* Default Cancel. */
    assert(restarts == 1);
    overlay_confirm_reset(); key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN);
    assert(restarts == 2);

    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RIGHT); down(2); /* Ethernet. */
    key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_RETURN); /* A no-op edit. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible() && saved == 2);

    key(SDL_SCANCODE_F9);
    down(5); key(SDL_SCANCODE_RETURN); /* MMU is always on: no false edit. */
    down(1); key(SDL_SCANCODE_RETURN); /* ADB requires a Turbo machine. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible() && saved == 2);

    /* Software rendering exercises all tabs and modals without a real display. */
    SDL_Surface *surface = SDL_CreateSurface(1120, 870, SDL_PIXELFORMAT_RGBA32);
    assert(surface);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    key(SDL_SCANCODE_F9);
    for (int i = 0; i < 4; i++) {
        overlay_render(renderer);
        SDL_RenderPresent(renderer);
        const char *preview = getenv("OVERLAY_PREVIEW_PREFIX");
        if (preview) {
            char filename[FILENAME_MAX];
            snprintf(filename, sizeof(filename), "%s%d.bmp", preview, i);
            assert(SDL_SaveBMP(surface, filename));
        }
        key(SDL_SCANCODE_RIGHT);
    }
    overlay_confirm_reset(); overlay_render(renderer); key(SDL_SCANCODE_ESCAPE);
    key(SDL_SCANCODE_F9);
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);

    /* A late native picker result cannot be delivered to a newer session. */
    assert(OverlayMedia_Request(OV_DIALOG_SCSI0, &UI89Config_, 0));
    assert(!OverlayMedia_Request(OV_DIALOG_SCSI1, &UI89Config_, 0));
    OverlayMedia_Cancel();
    const char *files[] = { "late.sd", NULL };
    picker_callback(picker_userdata, files, 0);
    OvDialogKind kind; char path[FILENAME_MAX]; long long size;
    assert(OverlayMedia_Poll(&kind, path, &size) && kind == OV_DIALOG_NONE);
    assert(!OverlayMedia_Busy());

    char dir[] = "/tmp/1989-media-XXXXXX";
    assert(mkdtemp(dir));
    char name[FILENAME_MAX], created[FILENAME_MAX];
    snprintf(name, sizeof(name), "%s/blank", dir);
    assert(OverlayMedia_Create(name, 2147483648LL, created));
    struct stat st;
    assert(stat(created, &st) == 0 && st.st_size == 2147483648LL);
    assert(!OverlayMedia_Create(name, 737280, path)); /* Never overwrite. */
    assert(stat(created, &st) == 0 && st.st_size == 2147483648LL);
    assert(!OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, created));
    remove(created);
    assert(OverlayMedia_Create(name, 737280, created));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, created));
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected);
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, NULL));
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected); /* Disk != drive. */
    test_scsi_layout(created);
    test_removable_eject(created);
    remove(created); rmdir(dir);
    puts("test-overlay: OK");
    return 0;
}
