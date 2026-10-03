/* overlay.c — F9 options overlay for 1989 (happy-years conventions).
 *
 * See overlay.h for the tab layout. The overlay reuses the existing
 * ConfigureParams structure for the machine state and keeps a small "[UI89]"
 * section for the UI-only settings (Tinker, GIF, notifications).
 */

#include "overlay.h"
#include "cfgopts.h"
#include "configuration.h"
#include "capture.h"
#include "ffmpeg_gif.h"
#include "floppy.h"
#include "grab.h"
#include "leds.h"
#include "log.h"
#include "mo.h"
#include "notify.h"
#include "file.h"
#include "paths.h"
#include "reset.h"
#include "scsi.h"
#include "screen.h"
#include "snd.h"
#include "main.h"
#include "sdlscreen.h"
#include "timing.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OV_SCALE      1.25f
#define OV_LINE_H     20
#define OV_VALUE_X    250

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif
#ifndef PROG_GIT_COMMIT
#define PROG_GIT_COMMIT "unknown"
#endif

extern char sConfigFileName[FILENAME_MAX];

/* ------------------------------------------------------------------ */
/* [UI89] settings                                                     */

UI89Config UI89Config_;

static const struct Config_Tag configs_UI89[] = {
    { "bTinker",     Bool_Tag,  &UI89Config_.bTinker },
    { "bSmoothing",  Bool_Tag,  &UI89Config_.bSmoothing },
    { "bCrtEnabled", Bool_Tag,  &UI89Config_.bCrtEnabled },
    { "nCrtScanlines", Int_Tag, &UI89Config_.nCrtScanlines },
    { "bDebug",      Bool_Tag,  &UI89Config_.bDebug },
    { "bRtcLocalTime", Bool_Tag, &UI89Config_.bRtcLocalTime },
    { "bGifFfmpeg",  Bool_Tag,  &UI89Config_.bGifFfmpeg },
    { "nWindowScale", Int_Tag,  &UI89Config_.nWindowScale },
    { "nGifWidth",   Int_Tag,   &UI89Config_.nGifWidth },
    { "nGifFps",     Int_Tag,   &UI89Config_.nGifFps },
    { "nNotifyMode", Int_Tag,   &UI89Config_.nNotifyMode },
    { "szLastDir1",  String_Tag, UI89Config_.szLastDir[1] },
    { "szLastDir2",  String_Tag, UI89Config_.szLastDir[2] },
    { "szLastDir3",  String_Tag, UI89Config_.szLastDir[3] },
    { "szLastDir4",  String_Tag, UI89Config_.szLastDir[4] },
    { "szLastDir5",  String_Tag, UI89Config_.szLastDir[5] },
    { "szLastDir6",  String_Tag, UI89Config_.szLastDir[6] },
    { "szLastDir7",  String_Tag, UI89Config_.szLastDir[7] },
    { "szLastDir8",  String_Tag, UI89Config_.szLastDir[8] },
    { "szLastDir9",  String_Tag, UI89Config_.szLastDir[9] },
    { "szLastDir10", String_Tag, UI89Config_.szLastDir[10] },
    { "szLastDir11", String_Tag, UI89Config_.szLastDir[11] },
    { "szLastDir12", String_Tag, UI89Config_.szLastDir[12] },
    { "szLastDir13", String_Tag, UI89Config_.szLastDir[13] },
    { "szLastDir14", String_Tag, UI89Config_.szLastDir[14] },
    { NULL, Error_Tag, NULL }
};

/* Apply the Debugging toggle to the emulator log level. */
static void overlay_apply_log_level(void) {
    Log_SetDebugEnabled(UI89Config_.bDebug);
}

void overlay_config_load(void) {
    UI89Config_.bTinker    = false;
    UI89Config_.bSmoothing = true;
    UI89Config_.bCrtEnabled = false;
    UI89Config_.nCrtScanlines = 35;
    UI89Config_.bDebug     = false;
    UI89Config_.bRtcLocalTime = true;
    UI89Config_.bGifFfmpeg = false;
    UI89Config_.nWindowScale = 0;
    UI89Config_.nGifWidth  = 480;
    UI89Config_.nGifFps    = 25;
    UI89Config_.nNotifyMode = NOTIFY_MODE_SCREEN;
    if (sConfigFileName[0])
        input_config(sConfigFileName, configs_UI89, "[UI89]");
    notify_set_mode((NotifyMode)UI89Config_.nNotifyMode);
    overlay_apply_log_level();
    Timing_SetLocalTime(UI89Config_.bRtcLocalTime);
}

void overlay_config_save(void) {
    if (sConfigFileName[0])
        update_config(sConfigFileName, configs_UI89, "[UI89]");
}

void overlay_save_config(void) {
    Configuration_Save();
}

/* ------------------------------------------------------------------ */

typedef struct {
    MACHINETYPE nMachineType;
    bool bTurbo;
    bool bColor;
    const char *name;
} MachineVariant;

static const MachineVariant machines[] = {
    { NEXT_CUBE030, false, false, "NeXT Computer" },
    { NEXT_CUBE040, false, false, "NeXTcube" },
    { NEXT_CUBE040, true,  false, "NeXTcube Turbo" },
    { NEXT_STATION, false, false, "NeXTstation" },
    { NEXT_STATION, true,  false, "NeXTstation Turbo" },
    { NEXT_STATION, false, true,  "NeXTstation Color" },
    { NEXT_STATION, true,  true,  "NeXTstation Turbo Color" },
};
#define MACHINE_COUNT ((int)(sizeof(machines) / sizeof(machines[0])))

static int current_machine_index(void) {
    for (int i = 0; i < MACHINE_COUNT; i++) {
        if (machines[i].nMachineType == ConfigureParams.System.nMachineType &&
            machines[i].bTurbo      == ConfigureParams.System.bTurbo &&
            machines[i].bColor      == ConfigureParams.System.bColor)
            return i;
    }
    return 0;
}

/* General section rows */
enum {
    GEN_MACHINE = 0,
    GEN_RAM,
    GEN_CPUCLOCK,
    GEN_FPU,
    GEN_DSP,
    GEN_MMU,
    GEN_ADB,
    GEN_TINKER,
    GEN_ABOUT,
    GEN_RESET,
    GEN_ROWS
};

/* Media section rows */
enum {
    MED_BOOT = 0,
    MED_SCSI0,
    MED_SCSI1,
    MED_SCSI2,
    MED_SCSI3,
    MED_SCSI4,
    MED_SCSI5,
    MED_SCSI6,
    MED_FLOPPY0,
    MED_FLOPPY1,
    MED_MO0,
    MED_MO1,
    MED_ROWS
};

/* Extensions section rows */
enum {
    EXT_ND = 0,
    EXT_PRINTER,
    EXT_ETHERNET,
    EXT_TABLET,
    EXT_MIC,
    EXT_ROWS
};

/* Advanced logical rows */
enum {
    ADV_SMOOTHING = 0,
    ADV_REAL_CRT,
    ADV_SCANLINES,      /* shown only while Real CRT is on */
    ADV_GIF_WIDTH,
    ADV_GIF_FPS,
    ADV_GIF_ENCODER,
    ADV_NOTIFICATIONS,
    ADV_DEBUG,
    ADV_RTC_CLOCK,
    ADV_FULLSCREEN,
    ADV_STATUSBAR,
    ADV_TITLEBAR,
    ADV_DRAMTEST,
    ADV_VERBOSE,
    ADV_VERSION,
    ADV_LOGICAL_COUNT
};

/* Number of selectable rows in the Advanced tab (Scanlines is conditional). */
static int adv_row_count(void) {
    return ADV_LOGICAL_COUNT - (UI89Config_.bCrtEnabled ? 0 : 1);
}

/* Map a displayed Advanced row to its logical ADV_* row. */
static int adv_logical_row(int row) {
    if (!UI89Config_.bCrtEnabled && row > ADV_REAL_CRT)
        return row + 1;   /* skip the hidden Scanlines row */
    return row;
}

static struct {
    bool      visible;
    bool      about_visible;
    OvSection section;
    int       row;

    /* Modal confirmation (happy-years style, replaces the legacy alert). */
    int       confirm_kind;   /* OV_CONFIRM_* */
    bool      confirm_ok;     /* true = OK selected, false = Cancel */

    /* True once a change has been made in this overlay session. */
    bool      dirty;
    /* Staged changes that need applying when the overlay is saved. */
    bool      need_reset;   /* machine/hardware change -> cold reset */
    bool      need_media;   /* media change -> reload the disks */

    /* Pending native file-dialog result. */
    OvDialogKind dialog_kind;
    bool         dialog_ready;
    char         dialog_path[FILENAME_MAX];

    /* "N = new" size chooser for media entries. */
    bool         choice_visible;
    OvDialogKind choice_kind;    /* media entry being created */
    int          choice_index;   /* selected size */
    long long    new_size;       /* pending blank image size in bytes */
} g_ov;

/* Sizes offered by "N = new" per media type. */
typedef struct {
    const char *label;
    long long   bytes;
} OvNewSize;

static const OvNewSize new_scsi_sizes[] = {
    { "1 GB",  1073741824LL },
    { "2 GB",  2147483648LL },
    { "4 GB",  4294967296LL },
    { "8 GB",  8589934592LL },
    { "16 GB", 17179869184LL },
    { "32 GB", 34359738368LL },
};
static const OvNewSize new_floppy_sizes[] = {
    { "400 KB",  409600 },
    { "720 KB",  737280 },
    { "1.44 MB", 1474560 },
    { "2.88 MB", 2949120 },
};
static const OvNewSize new_mo_sizes[] = {
    { "128 MB", 134217728 },
    { "230 MB", 241172480 },
    { "640 MB", 671088640 },
    { "1.3 GB", 1363148800LL },
};

enum {
    OV_CONFIRM_NONE = 0,
    OV_CONFIRM_QUIT,    /* quit the emulator */
    OV_CONFIRM_SAVE     /* save (or discard) changes when closing */
};

/* Snapshot taken when the overlay opens, restored when changes are
 * discarded. */
static CNF_PARAMS g_saved_params;
static UI89Config g_saved_ui89;

static const char *const about_lines[] = {
    "1989 NeXT (Motorola 68K) emulator",
    "(c) 2026 salvogendut",
    "Version " PACKAGE_VERSION " (commit " PROG_GIT_COMMIT ")",
    "Previous 4.3 core - WinUAE 68k, Hatari, i860 by Jason Eckhardt"
};
#define ABOUT_LINE_COUNT ((int)(sizeof(about_lines) / sizeof(about_lines[0])))

/* ------------------------------------------------------------------ */
/* helpers                                                             */

static const char *machine_name(void) {
    return machines[current_machine_index()].name;
}

static void machine_cycle(int dir) {
    int i = current_machine_index();
    i = (i + dir + MACHINE_COUNT) % MACHINE_COUNT;
    ConfigureParams.System.nMachineType = machines[i].nMachineType;
    ConfigureParams.System.bTurbo       = machines[i].bTurbo;
    ConfigureParams.System.bColor       = machines[i].bColor;
}

static const char *ram_string(char *buf, size_t size) {
    int mb = 0;
    for (int i = 0; i < 4; i++)
        mb += ConfigureParams.Memory.nMemoryBankSize[i];
    snprintf(buf, size, "%d MB (%dx%d)", mb,
             ConfigureParams.Memory.nMemoryBankSize[0],
             ConfigureParams.Memory.nMemoryBankSize[1]);
    return buf;
}

static const char *cpu_freq_string(char *buf, size_t size) {
    snprintf(buf, size, "%d MHz (%s)", ConfigureParams.System.nCpuFreq,
             ConfigureParams.System.bTurbo ? "turbo" : "stock");
    return buf;
}

static const char *fpu_string(char *buf, size_t size) {
    switch (ConfigureParams.System.n_FPUType) {
        case FPU_NONE:  snprintf(buf, size, "None"); break;
        case FPU_68881: snprintf(buf, size, "68881"); break;
        case FPU_68882: snprintf(buf, size, "68882"); break;
        default:        snprintf(buf, size, "CPU (68040)"); break;
    }
    return buf;
}

static const char *dsp_string(char *buf, size_t size) {
    switch (ConfigureParams.System.nDSPType) {
        case DSP_TYPE_NONE:    snprintf(buf, size, "None"); break;
        case DSP_TYPE_ACCURATE:snprintf(buf, size, "Accurate"); break;
        default:               snprintf(buf, size, "Emulated"); break;
    }
    return buf;
}

static const char *boot_string(char *buf, size_t size) {
    switch (ConfigureParams.Boot.nBootDevice) {
        case BOOT_ROM:      snprintf(buf, size, "ROM monitor"); break;
        case BOOT_SCSI:     snprintf(buf, size, "SCSI disk"); break;
        case BOOT_ETHERNET: snprintf(buf, size, "Ethernet (netboot)"); break;
        case BOOT_MO:       snprintf(buf, size, "Magneto-optical"); break;
        default:            snprintf(buf, size, "Floppy"); break;
    }
    return buf;
}

static const char *scsi_label(int i) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "SCSI %d", i);
    return buf;
}

/* Elide the leading directories of a long image path so the row hints stay
 * visible. */
static const char *media_path(const char *path, char *buf, size_t size) {
    size_t n = strlen(path);
    if (n <= 30) {
        snprintf(buf, size, "%s", path);
    } else {
        snprintf(buf, size, "...%s", path + n - 27);
    }
    return buf;
}

static const char *scsi_value(int i, char *buf, size_t size) {
    if (!ConfigureParams.SCSI.target[i].bDiskInserted)
        snprintf(buf, size, "[empty]  Enter=load, N=new, Del=clear");
    else {
        char pbuf[64];
        const char *type = ConfigureParams.SCSI.target[i].nDeviceType == SD_CD
                           ? "CD" :
                           ConfigureParams.SCSI.target[i].nDeviceType == SD_FLOPPY
                           ? "FLOPPY" : "DISK";
        snprintf(buf, size, "%s %s%s  Enter=eject, Del=clear", type,
                 media_path(ConfigureParams.SCSI.target[i].szImageName[0]
                    ? ConfigureParams.SCSI.target[i].szImageName : "(unnamed)",
                    pbuf, sizeof(pbuf)),
                 ConfigureParams.SCSI.target[i].bWriteProtected ? " [WP]" : "");
    }
    return buf;
}

static const char *floppy_value(int i, char *buf, size_t size) {
    if (!ConfigureParams.Floppy.drive[i].bDiskInserted)
        snprintf(buf, size, "[empty]  Enter=load, N=new, Del=clear");
    else {
        char pbuf[64];
        snprintf(buf, size, "%s%s  Enter=eject, Del=clear",
                 media_path(ConfigureParams.Floppy.drive[i].szImageName[0]
                    ? ConfigureParams.Floppy.drive[i].szImageName : "(unnamed)",
                    pbuf, sizeof(pbuf)),
                 ConfigureParams.Floppy.drive[i].bWriteProtected ? " [WP]" : "");
    }
    return buf;
}

static const char *mo_value(int i, char *buf, size_t size) {
    if (!ConfigureParams.MO.drive[i].bDiskInserted)
        snprintf(buf, size, "[empty]  Enter=load, N=new, Del=clear");
    else {
        char pbuf[64];
        snprintf(buf, size, "%s%s  Enter=eject, Del=clear",
                 media_path(ConfigureParams.MO.drive[i].szImageName[0]
                    ? ConfigureParams.MO.drive[i].szImageName : "(unnamed)",
                    pbuf, sizeof(pbuf)),
                 ConfigureParams.MO.drive[i].bWriteProtected ? " [WP]" : "");
    }
    return buf;
}

static const char *tablet_string(char *buf, size_t size) {
    switch (ConfigureParams.Tablet.nTabletType) {
        case TABLET_NONE:  snprintf(buf, size, "None"); break;
        case TABLET_MM961: snprintf(buf, size, "Summagraphics MM961"); break;
        case TABLET_MM1201:snprintf(buf, size, "Summagraphics MM1201"); break;
        default:           snprintf(buf, size, "SD series"); break;
    }
    return buf;
}

/* Pause the emulator thread, apply+reset the machine, resume. */
/* Stage a change that will need a hard reset when the overlay is saved.
 * Nothing is applied to the running machine until then, so discarding the
 * changes needs no reset. */
static void overlay_apply_reset(const char *message) {
	g_ov.need_reset = true;
	g_ov.dirty = true;
	if (message)
		notify_post("%s", message);
}

/* Refresh the activity-LED enable/colour state from ConfigureParams. */
void overlay_update_leds(void) {
	leds_set_enabled(LED_CPU, true);
	leds_set_cpu_frequency((unsigned)ConfigureParams.System.nCpuFreq);
	leds_set_enabled(LED_DSP,
	                 ConfigureParams.System.nDSPType != DSP_TYPE_NONE);
	leds_set_enabled(LED_SCSI, true);
	for (int i = 0; i < ESP_MAX_DEVS; i++)
		leds_set_scsi_present(i, ConfigureParams.SCSI.target[i].bDiskInserted);
	leds_set_enabled(LED_FLOPPY, true);
	leds_set_enabled(LED_MO, true);
	leds_set_enabled(LED_NET, ConfigureParams.Ethernet.bEthernetConnected);
	leds_set_enabled(LED_SND, ConfigureParams.Sound.bEnableSound);
	leds_set_enabled(LED_ND, ConfigureParams.Dimension.board[0].bEnabled);
}

/* Name of the currently selected machine model. */
const char *overlay_machine_name(void) {
	return machine_name();
}

/* ------------------------------------------------------------------ */
/* file dialogs                                                        */

static void overlay_file_callback(void *userdata, const char * const *files,
                                  int filter) {
    (void)userdata;
    (void)filter;
    /* Publish even a cancelled dialog (empty path) so a pending "N = new"
     * request cannot linger and hijack the next file dialog. */
    if (!files || !files[0])
        g_ov.dialog_path[0] = '\0';
    else
        snprintf(g_ov.dialog_path, sizeof(g_ov.dialog_path), "%s", files[0]);
    /* The callback may run on another thread: publish the path before the
     * ready flag. */
    SDL_MemoryBarrierRelease();
    g_ov.dialog_ready = true;
}

static void open_file_dialog(OvDialogKind kind) {
    static const SDL_DialogFileFilter image_filters[] = {
        { "NeXT disk images", "sd;dsk;img;bin;iso;od" },
        { "All files", "*" },
    };
    static const SDL_DialogFileFilter rom_filters[] = {
        { "NeXT firmware ROM", "bin" },
        { "All files", "*" },
    };
    const SDL_DialogFileFilter *filters = image_filters;
    const char *location = NULL;
    g_ov.dialog_kind = kind;
    g_ov.dialog_ready = false;
    g_ov.new_size = 0;
    g_ov.choice_visible = false;
    if (kind == OV_DIALOG_ROM030 || kind == OV_DIALOG_ROM040 ||
        kind == OV_DIALOG_ROMTURBO)
        filters = rom_filters;
    /* Start in the last directory browsed for this entry, if any. */
    if (kind > OV_DIALOG_NONE && kind < OV_DIALOG_COUNT &&
        UI89Config_.szLastDir[kind][0] &&
        File_DirExists(UI89Config_.szLastDir[kind]))
        location = UI89Config_.szLastDir[kind];
    SDL_ShowOpenFileDialog(overlay_file_callback, NULL, sdlWindow, filters, 2,
                           location, false);
}

/* Remember the directory of the file just selected for a media entry, so
 * the next browse for that same entry starts there. */
static void remember_dir(OvDialogKind kind, const char *path) {
    char dir[FILENAME_MAX];
    char *slash;

    if (kind <= OV_DIALOG_NONE || kind >= OV_DIALOG_COUNT || !path || !path[0])
        return;
    snprintf(dir, sizeof(dir), "%s", path);
    slash = strrchr(dir, '/');
    if (!slash)
        return;
    if (slash == dir)
        slash[1] = '\0';   /* keep the root slash */
    else
        *slash = '\0';
    if (!File_DirExists(dir))
        return;
    if (strcmp(UI89Config_.szLastDir[kind], dir) == 0)
        return;
    snprintf(UI89Config_.szLastDir[kind], FILENAME_MAX, "%s", dir);
}

/* ------------------------------------------------------------------ */
/* media actions                                                       */

static void set_scsi_image(int i, const char *path) {
    if (!path) {
        ConfigureParams.SCSI.target[i].bDiskInserted = false;
        ConfigureParams.SCSI.target[i].szImageName[0] = '\0';
        notify_post("SCSI %d MEDIA EJECTED", i);
    } else {
        snprintf(ConfigureParams.SCSI.target[i].szImageName, FILENAME_MAX, "%s",
                 path);
        if (ConfigureParams.SCSI.target[i].nDeviceType == SD_NONE)
            ConfigureParams.SCSI.target[i].nDeviceType = SD_HARDDISK;
        ConfigureParams.SCSI.target[i].bDiskInserted = true;
        notify_post("SCSI %d: MEDIA INSERTED", i);
    }
    g_ov.need_media = true;
    g_ov.dirty = true;
}

static void set_floppy_image(int i, const char *path) {
    if (!path) {
        ConfigureParams.Floppy.drive[i].bDiskInserted = false;
        ConfigureParams.Floppy.drive[i].bDriveConnected = false;
        ConfigureParams.Floppy.drive[i].szImageName[0] = '\0';
        notify_post("FLOPPY %d MEDIA EJECTED", i);
    } else {
        snprintf(ConfigureParams.Floppy.drive[i].szImageName, FILENAME_MAX, "%s",
                 path);
        ConfigureParams.Floppy.drive[i].bDiskInserted = true;
        ConfigureParams.Floppy.drive[i].bDriveConnected = true;
        notify_post("FLOPPY %d: MEDIA INSERTED", i);
    }
    g_ov.need_media = true;
    g_ov.dirty = true;
}

static void set_mo_image(int i, const char *path) {
    if (!path) {
        ConfigureParams.MO.drive[i].bDiskInserted = false;
        ConfigureParams.MO.drive[i].szImageName[0] = '\0';
        notify_post("MAG-OPT %d MEDIA EJECTED", i);
    } else {
        snprintf(ConfigureParams.MO.drive[i].szImageName, FILENAME_MAX, "%s", path);
        ConfigureParams.MO.drive[i].bDiskInserted = true;
        notify_post("MAG-OPT %d: MEDIA INSERTED", i);
    }
    g_ov.need_media = true;
    g_ov.dirty = true;
}

/* Map a Media row to its file-dialog kind. */
static OvDialogKind media_row_dialog(int row) {
    if (row >= MED_SCSI0 && row <= MED_SCSI6)
        return (OvDialogKind)(OV_DIALOG_SCSI0 + (row - MED_SCSI0));
    if (row >= MED_FLOPPY0 && row <= MED_FLOPPY1)
        return (OvDialogKind)(OV_DIALOG_FLOPPY0 + (row - MED_FLOPPY0));
    if (row >= MED_MO0 && row <= MED_MO1)
        return (OvDialogKind)(OV_DIALOG_MO0 + (row - MED_MO0));
    return OV_DIALOG_NONE;
}

/* Sizes offered by "N = new" for a media entry, or NULL if it cannot be
 * created. */
static const OvNewSize *overlay_new_sizes(OvDialogKind kind, int *count) {
    if (kind >= OV_DIALOG_SCSI0 && kind <= OV_DIALOG_SCSI6) {
        *count = (int)(sizeof(new_scsi_sizes) / sizeof(new_scsi_sizes[0]));
        return new_scsi_sizes;
    }
    if (kind == OV_DIALOG_FLOPPY0 || kind == OV_DIALOG_FLOPPY1) {
        *count = (int)(sizeof(new_floppy_sizes) / sizeof(new_floppy_sizes[0]));
        return new_floppy_sizes;
    }
    if (kind == OV_DIALOG_MO0 || kind == OV_DIALOG_MO1) {
        *count = (int)(sizeof(new_mo_sizes) / sizeof(new_mo_sizes[0]));
        return new_mo_sizes;
    }
    return NULL;
}

/* "N" on a Media row: open the size chooser. */
static void overlay_new_media(void) {
    OvDialogKind kind;
    int count = 0;

    if (g_ov.section != OV_MEDIA)
        return;
    kind = media_row_dialog(g_ov.row);
    if (!overlay_new_sizes(kind, &count))
        return;
    g_ov.choice_visible = true;
    g_ov.choice_kind = kind;
    g_ov.choice_index = 0;
}

/* Size chosen: remember it and open the save dialog for the new image. */
static void overlay_choice_accept(void) {
    static const SDL_DialogFileFilter filters[] = {
        { "Disk images", "img;IMG;sd;SD;dsk;DSK" },
        { "All files", "*" },
    };
    int count = 0;
    const OvNewSize *sizes = overlay_new_sizes(g_ov.choice_kind, &count);
    const char *location = NULL;

    g_ov.choice_visible = false;
    if (!sizes || g_ov.choice_index < 0 || g_ov.choice_index >= count)
        return;
    g_ov.new_size = sizes[g_ov.choice_index].bytes;
    g_ov.dialog_kind = g_ov.choice_kind;
    g_ov.dialog_ready = false;
    if (g_ov.choice_kind > OV_DIALOG_NONE &&
        g_ov.choice_kind < OV_DIALOG_COUNT &&
        UI89Config_.szLastDir[g_ov.choice_kind][0] &&
        File_DirExists(UI89Config_.szLastDir[g_ov.choice_kind]))
        location = UI89Config_.szLastDir[g_ov.choice_kind];
    SDL_ShowSaveFileDialog(overlay_file_callback, NULL, sdlWindow, filters, 2,
                           location);
}

/* Create a blank (zero-filled) image and attach it to the media entry. */
static void overlay_create_media(OvDialogKind kind, const char *path,
                                 long long size) {
    static const char zeros[65536];
    char final[FILENAME_MAX];
    const char *base;
    FILE *f;
    long long remaining = size;

    snprintf(final, sizeof(final), "%s", path);
    base = strrchr(final, '/');
    base = base ? base + 1 : final;
    if (!strchr(base, '.') && strlen(final) + 4 < sizeof(final))
        strcat(final, ".img");

    f = fopen(final, "wb");
    if (!f) {
        notify_post("COULD NOT CREATE IMAGE");
        return;
    }
    while (remaining > 0) {
        size_t chunk = remaining > (long long)sizeof(zeros)
                       ? sizeof(zeros) : (size_t)remaining;
        if (fwrite(zeros, 1, chunk, f) != chunk) {
            fclose(f);
            notify_post("COULD NOT CREATE IMAGE");
            return;
        }
        remaining -= (long long)chunk;
    }
    fclose(f);

    if (kind >= OV_DIALOG_SCSI0 && kind <= OV_DIALOG_SCSI6)
        set_scsi_image(kind - OV_DIALOG_SCSI0, final);
    else if (kind == OV_DIALOG_FLOPPY0 || kind == OV_DIALOG_FLOPPY1)
        set_floppy_image(kind - OV_DIALOG_FLOPPY0, final);
    else if (kind == OV_DIALOG_MO0 || kind == OV_DIALOG_MO1)
        set_mo_image(kind - OV_DIALOG_MO0, final);
}

static void set_rom_path(OvDialogKind kind, const char *path) {
    if (!path) return;
    switch (kind) {
        case OV_DIALOG_ROM030:
            snprintf(ConfigureParams.Rom.szRom030FileName, FILENAME_MAX, "%s",
                     path);
            break;
        case OV_DIALOG_ROM040:
            snprintf(ConfigureParams.Rom.szRom040FileName, FILENAME_MAX, "%s",
                     path);
            break;
        default:
            snprintf(ConfigureParams.Rom.szRomTurboFileName, FILENAME_MAX, "%s",
                     path);
            break;
    }
    g_ov.need_reset = true;
    g_ov.dirty = true;
    notify_post("ROM PATH SET - COLD RESET TO RELOAD");
}

/* ------------------------------------------------------------------ */
/* row actions                                                         */

static void overlay_activate(void) {
    /* Mark the session dirty unless the row is purely informational. */
    if (!(g_ov.section == OV_GENERAL && g_ov.row == GEN_ABOUT) &&
        !(g_ov.section == OV_ADVANCED &&
          adv_logical_row(g_ov.row) == ADV_VERSION))
        g_ov.dirty = true;
    switch (g_ov.section) {
        case OV_GENERAL:
            switch (g_ov.row) {
                case GEN_MACHINE:
                    machine_cycle(1);
                    Configuration_SetSystemDefaults();
                    overlay_apply_reset("MACHINE CHANGED - COLD RESET");
                    break;
                case GEN_RAM: {
                    int size = ConfigureParams.Memory.nMemoryBankSize[0];
                    size = size >= 32 ? 4 : size + 4;
                    for (int i = 0; i < 4; i++)
                        ConfigureParams.Memory.nMemoryBankSize[i] = size;
                    overlay_apply_reset("RAM CHANGED - COLD RESET");
                    break;
                }
                case GEN_CPUCLOCK:
                    ConfigureParams.System.nCpuFreq =
                        ConfigureParams.System.nCpuFreq >= 40 ? 25
                        : ConfigureParams.System.nCpuFreq + 8;
                    overlay_apply_reset("CPU CLOCK CHANGED - COLD RESET");
                    break;
                case GEN_FPU: {
                    FPUTYPE f = ConfigureParams.System.n_FPUType;
                    f = f == FPU_NONE ? FPU_68881 :
                        f == FPU_68881 ? FPU_68882 :
                        f == FPU_68882 ? FPU_CPU : FPU_NONE;
                    ConfigureParams.System.n_FPUType = f;
                    overlay_apply_reset("FPU CHANGED - COLD RESET");
                    break;
                }
                case GEN_DSP: {
                    DSPTYPE d = ConfigureParams.System.nDSPType;
                    d = d == DSP_TYPE_NONE ? DSP_TYPE_EMU :
                        d == DSP_TYPE_EMU ? DSP_TYPE_ACCURATE : DSP_TYPE_NONE;
                    ConfigureParams.System.nDSPType = d;
                    overlay_apply_reset("DSP CHANGED - COLD RESET");
                    break;
                }
                case GEN_MMU:
                    ConfigureParams.System.bMMU =
                        !ConfigureParams.System.bMMU;
                    overlay_apply_reset("MMU CHANGED - COLD RESET");
                    break;
                case GEN_ADB:
                    ConfigureParams.System.bADB =
                        !ConfigureParams.System.bADB;
                    overlay_apply_reset("ADB CHANGED - COLD RESET");
                    break;
                case GEN_TINKER:
                    UI89Config_.bTinker = !UI89Config_.bTinker;
                    if (!UI89Config_.bTinker && g_ov.section == OV_ADVANCED)
                        g_ov.section = OV_GENERAL;
                    break;
                case GEN_ABOUT:
                    g_ov.about_visible = true;
                    break;
                case GEN_RESET:
                    overlay_apply_reset("SYSTEM DEFAULTS RESTORED");
                    g_ov.row = 0;
                    break;
                default:
                    break;
            }
            break;

        case OV_MEDIA:
            if (g_ov.row == MED_BOOT) {
                BOOT_DEVICE b = ConfigureParams.Boot.nBootDevice;
                b = (BOOT_DEVICE)(((int)b + 1) % 5);
                ConfigureParams.Boot.nBootDevice = b;
                overlay_apply_reset("BOOT DEVICE CHANGED - COLD RESET");
            } else if (g_ov.row >= MED_SCSI0 && g_ov.row <= MED_SCSI6) {
                int i = g_ov.row - MED_SCSI0;
                if (ConfigureParams.SCSI.target[i].bDiskInserted)
                    set_scsi_image(i, NULL);
                else
                    open_file_dialog((OvDialogKind)(OV_DIALOG_SCSI0 + i));
            } else if (g_ov.row >= MED_FLOPPY0 && g_ov.row <= MED_FLOPPY1) {
                int i = g_ov.row - MED_FLOPPY0;
                if (ConfigureParams.Floppy.drive[i].bDiskInserted)
                    set_floppy_image(i, NULL);
                else
                    open_file_dialog((OvDialogKind)(OV_DIALOG_FLOPPY0 + i));
            } else if (g_ov.row >= MED_MO0 && g_ov.row <= MED_MO1) {
                int i = g_ov.row - MED_MO0;
                if (ConfigureParams.MO.drive[i].bDiskInserted)
                    set_mo_image(i, NULL);
                else
                    open_file_dialog((OvDialogKind)(OV_DIALOG_MO0 + i));
            }
            break;

        case OV_EXTENSIONS:
            switch (g_ov.row) {
                case EXT_ND:
                    ConfigureParams.Dimension.board[0].bEnabled =
                        !ConfigureParams.Dimension.board[0].bEnabled;
                    overlay_apply_reset("NEXTDIMENSION CHANGED - COLD RESET");
                    break;
                case EXT_PRINTER:
                    ConfigureParams.Printer.bPrinterConnected =
                        !ConfigureParams.Printer.bPrinterConnected;
                    overlay_apply_reset("PRINTER CHANGED - COLD RESET");
                    break;
                case EXT_ETHERNET:
                    ConfigureParams.Ethernet.bEthernetConnected =
                        !ConfigureParams.Ethernet.bEthernetConnected;
                    overlay_apply_reset("ETHERNET CHANGED - COLD RESET");
                    break;
                case EXT_TABLET:
                    ConfigureParams.Tablet.nTabletType =
                        (TABLET_TYPE)(((int)ConfigureParams.Tablet.nTabletType + 1)
                                      % 8);
                    overlay_apply_reset("TABLET CHANGED - COLD RESET");
                    break;
                case EXT_MIC:
                    ConfigureParams.Sound.bEnableMicrophone =
                        !ConfigureParams.Sound.bEnableMicrophone;
                    notify_post(ConfigureParams.Sound.bEnableMicrophone
                                ? "MICROPHONE ON" : "MICROPHONE OFF");
                    break;
                default:
                    break;
            }
            break;

        case OV_ADVANCED:
            switch (adv_logical_row(g_ov.row)) {
                case ADV_SMOOTHING:
                    UI89Config_.bSmoothing = !UI89Config_.bSmoothing;
                    break;
                case ADV_REAL_CRT:
                    UI89Config_.bCrtEnabled = !UI89Config_.bCrtEnabled;
                    notify_post(UI89Config_.bCrtEnabled
                                ? "REAL CRT ON" : "REAL CRT OFF");
                    break;
                case ADV_SCANLINES:
                    UI89Config_.nCrtScanlines += 5;
                    if (UI89Config_.nCrtScanlines > 95)
                        UI89Config_.nCrtScanlines = 0;
                    break;
                case ADV_GIF_WIDTH:
                    UI89Config_.nGifWidth =
                        UI89Config_.nGifWidth >= 640 ? 320 :
                        UI89Config_.nGifWidth + 160;
                    break;
                case ADV_GIF_FPS:
                    UI89Config_.nGifFps =
                        UI89Config_.nGifFps >= 25 ? 10 :
                        UI89Config_.nGifFps + 5;
                    break;
                case ADV_GIF_ENCODER:
                    if (!FFMPEG_GIF_SUPPORTED) {
                        notify_post("FFMPEG NOT AVAILABLE");
                        break;
                    }
                    UI89Config_.bGifFfmpeg = !UI89Config_.bGifFfmpeg;
                    notify_post(UI89Config_.bGifFfmpeg
                                ? "GIF ENCODER: FFMPEG OPTIMIZE"
                                : "GIF ENCODER: BUILT-IN");
                    break;
                case ADV_NOTIFICATIONS: {
                    int m = UI89Config_.nNotifyMode + 1;
                    if (m > NOTIFY_MODE_CONSOLE) m = NOTIFY_MODE_OFF;
                    UI89Config_.nNotifyMode = m;
                    notify_set_mode((NotifyMode)m);
                    break;
                }
                case ADV_DEBUG:
                    UI89Config_.bDebug = !UI89Config_.bDebug;
                    overlay_apply_log_level();
                    notify_post(UI89Config_.bDebug
                                ? "DEBUG OUTPUT ON" : "DEBUG OUTPUT OFF");
                    break;
                case ADV_RTC_CLOCK:
                    UI89Config_.bRtcLocalTime = !UI89Config_.bRtcLocalTime;
                    Timing_SetLocalTime(UI89Config_.bRtcLocalTime);
                    notify_post(UI89Config_.bRtcLocalTime
                                ? "RTC CLOCK: LOCAL TIME"
                                : "RTC CLOCK: UTC");
                    break;
                case ADV_FULLSCREEN:
                    if (bInFullScreen)
                        Screen_ReturnFromFullScreen();
                    else
                        Screen_EnterFullScreen();
                    break;
                case ADV_STATUSBAR:
                    ConfigureParams.Screen.bShowStatusbar =
                        !ConfigureParams.Screen.bShowStatusbar;
                    Screen_Reset();
                    break;
                case ADV_TITLEBAR:
                    ConfigureParams.Screen.bShowTitlebar =
                        !ConfigureParams.Screen.bShowTitlebar;
                    Screen_TitlebarChanged();
                    break;
                case ADV_DRAMTEST:
                    ConfigureParams.Boot.bEnableDRAMTest =
                        !ConfigureParams.Boot.bEnableDRAMTest;
                    overlay_apply_reset("DRAM TEST CHANGED - COLD RESET");
                    break;
                case ADV_VERBOSE:
                    ConfigureParams.Boot.bVerbose =
                        !ConfigureParams.Boot.bVerbose;
                    overlay_apply_reset("VERBOSE BOOT CHANGED - COLD RESET");
                    break;
                case ADV_VERSION:
                    g_ov.about_visible = true;
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

/* Delete on a Media row clears (ejects) the attached image. */
static void overlay_clear_media(void) {
    if (g_ov.section != OV_MEDIA)
        return;
    if (g_ov.row >= MED_SCSI0 && g_ov.row <= MED_SCSI6) {
        int i = g_ov.row - MED_SCSI0;
        if (ConfigureParams.SCSI.target[i].bDiskInserted)
            set_scsi_image(i, NULL);
    } else if (g_ov.row >= MED_FLOPPY0 && g_ov.row <= MED_FLOPPY1) {
        int i = g_ov.row - MED_FLOPPY0;
        if (ConfigureParams.Floppy.drive[i].bDiskInserted)
            set_floppy_image(i, NULL);
    } else if (g_ov.row >= MED_MO0 && g_ov.row <= MED_MO1) {
        int i = g_ov.row - MED_MO0;
        if (ConfigureParams.MO.drive[i].bDiskInserted)
            set_mo_image(i, NULL);
    } else {
        return;
    }
    g_ov.dirty = true;
}

/* ------------------------------------------------------------------ */
/* rendering                                                           */

static void draw_row(SDL_Renderer *r, int lw, float y,
                     const char *label, const char *value, bool highlight) {
    if (highlight) {
        SDL_SetRenderDrawColor(r, 0x80, 0x60, 0x20, 255);
        SDL_FRect hl = { 10, y, (float)lw - 4, OV_LINE_H - 4 };
        SDL_RenderFillRect(r, &hl);
    }
    SDL_SetRenderDrawColor(r, 0xFF, 0xFF, 0xFF, 255);
    SDL_RenderDebugText(r, 20, y, label);
    if (value) {
        char shown[96];
        size_t max_chars = (size_t)(lw - OV_VALUE_X - 18) / 8;
        if (max_chars >= sizeof(shown)) max_chars = sizeof(shown) - 1;
        size_t n = strlen(value);
        if (n > max_chars && max_chars > 3) {
            memcpy(shown, value, max_chars - 3);
            memcpy(shown + max_chars - 3, "...", 4);
        } else {
            snprintf(shown, sizeof(shown), "%s", value);
        }
        SDL_SetRenderDrawColor(r, 0xFF, highlight ? 0xFF : 0xD0,
                               highlight ? 0xFF : 0x80, 255);
        SDL_RenderDebugText(r, (float)OV_VALUE_X, y, shown);
    }
}

static int section_rows(void) {
    switch (g_ov.section) {
        case OV_GENERAL:    return GEN_ROWS;
        case OV_MEDIA:      return MED_ROWS;
        case OV_EXTENSIONS: return EXT_ROWS;
        case OV_ADVANCED:   return adv_row_count();
        default:            return 0;
    }
}

static const char *section_name(OvSection s) {
    switch (s) {
        case OV_GENERAL:    return "General";
        case OV_MEDIA:      return "Media";
        case OV_EXTENSIONS: return "Extensions";
        case OV_ADVANCED:   return "Advanced";
        default:            return "";
    }
}

static bool section_available(OvSection s) {
    if (s == OV_ADVANCED) return UI89Config_.bTinker;
    return true;
}

static void draw_confirm_button(SDL_Renderer *r, float x, float y, int w,
                                int h, const char *label, bool selected) {
    SDL_FRect rect = { x, y, (float)w, (float)h };
    if (selected) {
        SDL_SetRenderDrawColor(r, 0x80, 0x60, 0x20, 255);
        SDL_RenderFillRect(r, &rect);
    }
    SDL_SetRenderDrawColor(r, selected ? 0xFF : 0x89,
                           selected ? 0xFF : 0xA3,
                           selected ? 0xFF : 0xCB, 255);
    SDL_RenderRect(r, &rect);
    SDL_SetRenderDrawColor(r, selected ? 0xFF : 0xC0,
                           selected ? 0xFF : 0xC0,
                           selected ? 0xFF : 0xC0, 255);
    float tw = (float)strlen(label) * 8.0f;
    SDL_RenderDebugText(r, x + ((float)w - tw) * 0.5f,
                        y + ((float)h - 8.0f) * 0.5f, label);
}

/* Return the renderer's logical (presentation) size. Under a logical
 * presentation (fullscreen letterbox, windowed stretch) render coordinates
 * are in that space, not in output pixels; using the output size would push
 * centred dialogs off-screen in fullscreen. */
static void overlay_get_render_size(SDL_Renderer *r, int *w, int *h) {
    SDL_RendererLogicalPresentation mode;
    if (SDL_GetRenderLogicalPresentation(r, w, h, &mode) && *w > 0 && *h > 0)
        return;
    if (!SDL_GetRenderOutputSize(r, w, h)) {
        *w = 0;
        *h = 0;
    }
}

/* Happy-years style modal confirmation (dark panel, dimmed backdrop). */
static void overlay_render_confirm(SDL_Renderer *r) {
    int rw, rh;
    overlay_get_render_size(r, &rw, &rh);
    if (rw <= 0 || rh <= 0) return;

    float scale = OV_SCALE;
    if ((float)rw / scale < 520.0f) scale = (float)rw / 520.0f;
    if ((float)rh / scale < 300.0f) scale = (float)rh / 300.0f;
    if (scale <= 0.0f) return;
    SDL_SetRenderScale(r, scale, scale);
    int lw = (int)(rw / scale);
    int lh = (int)(rh / scale);

    static const char *const quit_lines[] = {
        "All unsaved data will be lost.",
        "Do you really want to quit?"
    };
    static const char *const save_lines[] = {
        "Save the changes to 1989.conf?",
        "Cancel discards them."
    };
    static const char *const save_reset_lines[] = {
        "Saving resets the emulated machine.",
        "Save the changes?"
    };
    static const char *const save_media_lines[] = {
        "Saving reloads the attached media.",
        "Save the changes?"
    };
    const char *const *lines = quit_lines;
    if (g_ov.confirm_kind == OV_CONFIRM_SAVE)
        lines = g_ov.need_reset ? save_reset_lines :
                g_ov.need_media ? save_media_lines : save_lines;
    const int nlines = 2;
    int text_w = 0;
    for (int i = 0; i < nlines; i++) {
        int w = (int)strlen(lines[i]) * 8;
        if (w > text_w) text_w = w;
    }
    int panel_w = text_w + 64;
    int panel_h = 24 + nlines * 16 + 20 + 22 + 18;
    float px = (float)(lw - panel_w) * 0.5f;
    float py = (float)(lh - panel_h) * 0.5f;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect dim = { 0, 0, (float)lw, (float)lh };
    SDL_RenderFillRect(r, &dim);

    SDL_SetRenderDrawColor(r, 0x19, 0x20, 0x34, 255);
    SDL_FRect box = { px, py, (float)panel_w, (float)panel_h };
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, 0x89, 0xA3, 0xCB, 255);
    SDL_RenderRect(r, &box);

    SDL_SetRenderDrawColor(r, 0xF0, 0xF0, 0xF0, 255);
    for (int i = 0; i < nlines; i++)
        SDL_RenderDebugText(r, px + 32, py + 24 + i * 16, lines[i]);

    const char *ok_label = "OK";
    const char *cancel_label = "Cancel";
    int ok_w = (int)strlen(ok_label) * 8 + 20;
    int cancel_w = (int)strlen(cancel_label) * 8 + 20;
    int gap = 32;
    int total = ok_w + gap + cancel_w;
    float bx = px + (float)(panel_w - total) * 0.5f;
    float by = py + 24 + nlines * 16 + 20;
    draw_confirm_button(r, bx, by, ok_w, 22, ok_label, g_ov.confirm_ok);
    draw_confirm_button(r, bx + ok_w + gap, by, cancel_w, 22, cancel_label,
                        !g_ov.confirm_ok);

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

/* Render the "N = new" size chooser: a small centred panel listing the
 * available image sizes for the selected media entry. */
static void overlay_render_choice(SDL_Renderer *r) {
    int rw, rh, count = 0;
    const OvNewSize *sizes;
    const char *title;

    overlay_get_render_size(r, &rw, &rh);
    if (rw <= 0 || rh <= 0) return;
    sizes = overlay_new_sizes(g_ov.choice_kind, &count);
    if (!sizes || count <= 0) return;

    if (g_ov.choice_kind >= OV_DIALOG_SCSI0 &&
        g_ov.choice_kind <= OV_DIALOG_SCSI6)
        title = "New hard disk image";
    else if (g_ov.choice_kind == OV_DIALOG_FLOPPY0 ||
             g_ov.choice_kind == OV_DIALOG_FLOPPY1)
        title = "New floppy image";
    else
        title = "New magneto-optical image";

    float scale = OV_SCALE;
    if ((float)rw / scale < 360.0f) scale = (float)rw / 360.0f;
    if ((float)rh / scale < (float)(count * 30 + 120))
        scale = (float)rh / (float)(count * 30 + 120);
    if (scale <= 0.0f) return;
    SDL_SetRenderScale(r, scale, scale);
    int lw = (int)(rw / scale);
    int lh = (int)(rh / scale);

    int panel_w = 320;
    int panel_h = 24 + 16 + 12 + count * 30 + 16 + 8;
    float px = (float)(lw - panel_w) * 0.5f;
    float py = (float)(lh - panel_h) * 0.5f;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect dim = { 0, 0, (float)lw, (float)lh };
    SDL_RenderFillRect(r, &dim);

    SDL_SetRenderDrawColor(r, 0x19, 0x20, 0x34, 255);
    SDL_FRect box = { px, py, (float)panel_w, (float)panel_h };
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, 0x89, 0xA3, 0xCB, 255);
    SDL_RenderRect(r, &box);

    SDL_SetRenderDrawColor(r, 0xF0, 0xF0, 0xF0, 255);
    SDL_RenderDebugText(r, px + 16, py + 16, title);
    SDL_SetRenderDrawColor(r, 0x89, 0xA3, 0xCB, 255);
    SDL_RenderDebugText(r, px + 16, py + 32, "Choose the image size:");

    for (int i = 0; i < count; i++) {
        bool sel = (i == g_ov.choice_index);
        draw_confirm_button(r, px + 24, py + 56 + i * 30,
                            panel_w - 48, 22, sizes[i].label, sel);
    }

    SDL_SetRenderDrawColor(r, 0x89, 0xA3, 0xCB, 255);
    SDL_RenderDebugText(r, px + 16, py + (float)panel_h - 18,
                        "Enter=create  Esc=cancel");

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

void overlay_render(SDL_Renderer *r) {
    if (g_ov.confirm_kind != OV_CONFIRM_NONE) {
        overlay_render_confirm(r);
        return;
    }
    if (g_ov.choice_visible) {
        overlay_render_choice(r);
        return;
    }
    if (!g_ov.visible) return;

    int rw, rh;
    overlay_get_render_size(r, &rw, &rh);
    if (rw <= 0 || rh <= 0) return;

    int rows = section_rows();
    int panel_h = 48 + rows * OV_LINE_H + 42;
    float min_logical_h = panel_h + 8 > 510 ? (float)(panel_h + 8) : 510.0f;
    float scale = OV_SCALE;
    if ((float)rw / scale < 860.0f) scale = (float)rw / 860.0f;
    if ((float)rh / scale < min_logical_h) scale = (float)rh / min_logical_h;
    if (scale <= 0.0f) return;
    SDL_SetRenderScale(r, scale, scale);
    int lw = (int)(rw / scale);
    int panel_w = lw - 20 < 840 ? lw - 20 : 840;

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 90);
    SDL_FRect shade = { 0, 0, (float)lw, (float)(rh / scale) };
    SDL_RenderFillRect(r, &shade);
    SDL_SetRenderDrawColor(r, 8, 10, 24, 235);
    SDL_FRect bg = { 8, 8, (float)panel_w, (float)panel_h };
    SDL_RenderFillRect(r, &bg);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    /* Section tabs */
    SDL_SetRenderDrawColor(r, 0x30, 0x40, 0x60, 255);
    SDL_FRect tabbar = { 10, 10, (float)panel_w - 4, 22 };
    SDL_RenderFillRect(r, &tabbar);
    float tx = 20;
    for (int s = 0; s < OV_SECTION_COUNT; s++) {
        if (!section_available((OvSection)s)) continue;
        bool active = (s == (int)g_ov.section);
        SDL_SetRenderDrawColor(r, active ? 0xFF : 0xC0,
                               active ? 0xFF : 0xC0,
                               active ? 0xFF : 0xC0, 255);
        SDL_RenderDebugText(r, tx, 14, section_name((OvSection)s));
        tx += (float)((int)strlen(section_name((OvSection)s)) * 8 + 20);
    }

    float y = 48;
    char vbuf[FILENAME_MAX + 8];
    char s1[64], s2[64], s3[64];

    if (g_ov.section == OV_GENERAL) {
        draw_row(r, panel_w, y, "Machine", machine_name(), g_ov.row == GEN_MACHINE);
        y += OV_LINE_H;
        draw_row(r, panel_w, y, "RAM", ram_string(vbuf, sizeof(vbuf)),
                 g_ov.row == GEN_RAM); y += OV_LINE_H;
        draw_row(r, panel_w, y, "CPU clock", cpu_freq_string(s1, sizeof(s1)),
                 g_ov.row == GEN_CPUCLOCK); y += OV_LINE_H;
        draw_row(r, panel_w, y, "FPU", fpu_string(s1, sizeof(s1)),
                 g_ov.row == GEN_FPU); y += OV_LINE_H;
        draw_row(r, panel_w, y, "DSP", dsp_string(s1, sizeof(s1)),
                 g_ov.row == GEN_DSP); y += OV_LINE_H;
        draw_row(r, panel_w, y, "MMU", ConfigureParams.System.bMMU ? "On" : "Off",
                 g_ov.row == GEN_MMU); y += OV_LINE_H;
        draw_row(r, panel_w, y, "ADB", ConfigureParams.System.bADB ? "On" : "Off",
                 g_ov.row == GEN_ADB); y += OV_LINE_H;
        draw_row(r, panel_w, y, "Tinker", UI89Config_.bTinker ? "On" : "Off",
                 g_ov.row == GEN_TINKER); y += OV_LINE_H;
        draw_row(r, panel_w, y, "About", "Program details", g_ov.row == GEN_ABOUT);
        y += OV_LINE_H;
        draw_row(r, panel_w, y, "Reset defaults", NULL, g_ov.row == GEN_RESET);
    } else if (g_ov.section == OV_MEDIA) {
        draw_row(r, panel_w, y, "Boot device", boot_string(s1, sizeof(s1)),
                 g_ov.row == MED_BOOT); y += OV_LINE_H;
        for (int i = 0; i < 7; i++) {
            draw_row(r, panel_w, y, scsi_label(i),
                     scsi_value(i, vbuf, sizeof(vbuf)), g_ov.row == MED_SCSI0 + i);
            y += OV_LINE_H;
        }
        for (int i = 0; i < 2; i++) {
            snprintf(s2, sizeof(s2), "Floppy %d", i);
            draw_row(r, panel_w, y, s2, floppy_value(i, vbuf, sizeof(vbuf)),
                     g_ov.row == MED_FLOPPY0 + i);
            y += OV_LINE_H;
        }
        for (int i = 0; i < 2; i++) {
            snprintf(s2, sizeof(s2), "Mag-opt %d", i);
            draw_row(r, panel_w, y, s2, mo_value(i, vbuf, sizeof(vbuf)),
                     g_ov.row == MED_MO0 + i);
            y += OV_LINE_H;
        }
    } else if (g_ov.section == OV_EXTENSIONS) {
        draw_row(r, panel_w, y, "NeXTdimension",
                 ConfigureParams.Dimension.board[0].bEnabled ? "On" : "Off",
                 g_ov.row == EXT_ND); y += OV_LINE_H;
        draw_row(r, panel_w, y, "Printer",
                 ConfigureParams.Printer.bPrinterConnected ? "On" : "Off",
                 g_ov.row == EXT_PRINTER); y += OV_LINE_H;
        draw_row(r, panel_w, y, "Ethernet",
                 ConfigureParams.Ethernet.bEthernetConnected ? "On" : "Off",
                 g_ov.row == EXT_ETHERNET); y += OV_LINE_H;
        draw_row(r, panel_w, y, "Tablet", tablet_string(s1, sizeof(s1)),
                 g_ov.row == EXT_TABLET); y += OV_LINE_H;
        draw_row(r, panel_w, y, "Microphone",
                 ConfigureParams.Sound.bEnableMicrophone ? "On" : "Off",
                 g_ov.row == EXT_MIC); y += OV_LINE_H;
    } else {
        char vbuf2[64];
        int dr = 0;
        draw_row(r, panel_w, y, "Smoothing",
                 UI89Config_.bSmoothing ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Real CRT",
                 UI89Config_.bCrtEnabled ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        if (UI89Config_.bCrtEnabled) {
            snprintf(vbuf2, sizeof(vbuf2), "%d%%", UI89Config_.nCrtScanlines);
            draw_row(r, panel_w, y, "Scanlines", vbuf2,
                     g_ov.row == dr); y += OV_LINE_H; dr++;
        }
        {
            char gline[64];
            snprintf(gline, sizeof(gline), "%dx%d",
                     UI89Config_.nGifWidth,
                     UI89Config_.nGifWidth * 832 / 1120);
            draw_row(r, panel_w, y, "GIF resolution", gline,
                     g_ov.row == dr); y += OV_LINE_H; dr++;
        }
        {
            char fps[32];
            snprintf(fps, sizeof(fps), "%d fps", UI89Config_.nGifFps);
            draw_row(r, panel_w, y, "GIF frame rate", fps,
                     g_ov.row == dr); y += OV_LINE_H; dr++;
        }
        if (!FFMPEG_GIF_SUPPORTED)
            snprintf(vbuf2, sizeof(vbuf2), "built-in [ffmpeg unavailable]");
        else
            snprintf(vbuf2, sizeof(vbuf2), "%s",
                     UI89Config_.bGifFfmpeg ? "FFmpeg optimize" : "built-in");
        draw_row(r, panel_w, y, "GIF encoder", vbuf2,
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Notifications",
                 UI89Config_.nNotifyMode == NOTIFY_MODE_OFF ? "Off" :
                 UI89Config_.nNotifyMode == NOTIFY_MODE_CONSOLE ? "Console" :
                 "Screen",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Debugging",
                 UI89Config_.bDebug ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "RTC clock",
                 UI89Config_.bRtcLocalTime ? "Local time" : "UTC",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Fullscreen", bInFullScreen ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Status bar",
                 ConfigureParams.Screen.bShowStatusbar ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Title bar",
                 ConfigureParams.Screen.bShowTitlebar ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "DRAM test",
                 ConfigureParams.Boot.bEnableDRAMTest ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Verbose boot",
                 ConfigureParams.Boot.bVerbose ? "On" : "Off",
                 g_ov.row == dr); y += OV_LINE_H; dr++;
        draw_row(r, panel_w, y, "Version", PACKAGE_VERSION,
                 g_ov.row == dr);
    }

    /* Footer */
    SDL_SetRenderDrawColor(r, 0xAA, 0xAA, 0xAA, 255);
    const char *footer = g_ov.section == OV_MEDIA
        ? "Left/Right section  Up/Down select  Enter choose/eject  F9/Esc close"
        : "Left/Right section  Up/Down select  Enter toggle  F9/Esc close";
    SDL_RenderDebugText(r, 20, (float)(panel_h - 20), footer);

    if (g_ov.about_visible) {
        int lh = (int)(rh / scale);
        int text_w = 0;
        for (int i = 0; i < ABOUT_LINE_COUNT; ++i) {
            int w = (int)strlen(about_lines[i]) * 8;
            if (w > text_w) text_w = w;
        }
        int box_w = text_w + 32;
        int box_h = 28 + ABOUT_LINE_COUNT * 16 + 28;
        float bx = (float)(lw - box_w) * 0.5f;
        float by = (float)(lh - box_h) * 0.5f;
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
        SDL_FRect dim = { 0, 0, (float)lw, (float)lh };
        SDL_RenderFillRect(r, &dim);
        SDL_SetRenderDrawColor(r, 0x19, 0x20, 0x34, 255);
        SDL_FRect box = { bx, by, (float)box_w, (float)box_h };
        SDL_RenderFillRect(r, &box);
        SDL_SetRenderDrawColor(r, 0x89, 0xA3, 0xCB, 255);
        SDL_RenderRect(r, &box);
        SDL_SetRenderDrawColor(r, 0xF0, 0xF0, 0xF0, 255);
        for (int i = 0; i < ABOUT_LINE_COUNT; ++i)
            SDL_RenderDebugText(r, bx + 16, by + 16 + i * 16,
                                about_lines[i]);
        SDL_SetRenderDrawColor(r, 0xFF, 0xDA, 0x79, 255);
        SDL_RenderDebugText(r, bx + (box_w - 16) * 0.5f,
                            by + box_h - 20, "OK");
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    SDL_SetRenderScale(r, 1.0f, 1.0f);
}

/* ------------------------------------------------------------------ */
/* lifecycle                                                           */

void overlay_init(void) {
    memset(&g_ov, 0, sizeof(g_ov));
    g_ov.section = OV_GENERAL;
    g_ov.dialog_kind = OV_DIALOG_NONE;
    overlay_config_load();
}

void overlay_quit(void) {
    /* nothing to tear down */
}



bool overlay_is_visible(void) {
    return g_ov.visible;
}

bool overlay_confirm_visible(void) {
    return g_ov.confirm_kind != OV_CONFIRM_NONE;
}

void overlay_confirm_quit(void) {
    g_ov.confirm_kind = OV_CONFIRM_QUIT;
    g_ov.confirm_ok = true;
    /* Release the emulated mouse so the host cursor/keys can reach the modal. */
    if (bGrabMouse) {
        bGrabMouse = false;
        Screen_SetMouseGrab(false);
    }
}

/* Hide the overlay without saving or discarding. */
static void overlay_close_now(void) {
    g_ov.about_visible = false;
    g_ov.visible = false;
    g_ov.dirty = false;
    g_ov.need_reset = false;
    g_ov.need_media = false;
}

/* Apply the staged changes to the running machine (on save). */
static void overlay_apply_pending(void) {
    if (!g_ov.dirty)
        return;
    Configuration_Apply(false);
    if (g_ov.need_reset) {
        bool was_active = Main_PauseEmulation(false);
        Reset_Cold();
        if (was_active)
            Main_UnPauseEmulation();
    } else if (g_ov.need_media) {
        SCSI_Reset();
        Floppy_Reset();
        MO_Reset();
    }
    overlay_update_leds();
    g_ov.need_reset = false;
    g_ov.need_media = false;
}

/* Restore the state captured when the overlay was opened. Nothing was
 * applied to the running machine while staging, so no reset is needed. */
static void overlay_discard(void) {
    ConfigureParams = g_saved_params;
    UI89Config_ = g_saved_ui89;
    g_ov.new_size = 0;
    g_ov.choice_visible = false;
    Log_SetDebugEnabled(UI89Config_.bDebug);
    Timing_SetLocalTime(UI89Config_.bRtcLocalTime);
    notify_set_mode((NotifyMode)UI89Config_.nNotifyMode);
    overlay_update_leds();
}

void overlay_close(void) {
    if (!g_ov.visible)
        return;
    /* Ask before dropping the session's changes. */
    if (g_ov.dirty) {
        g_ov.confirm_kind = OV_CONFIRM_SAVE;
        g_ov.confirm_ok = true;
        return;
    }
    overlay_close_now();
}

bool overlay_handle_event(const SDL_Event *ev) {
    bool modal = g_ov.visible || g_ov.confirm_kind != OV_CONFIRM_NONE;
    if (ev->type != SDL_EVENT_KEY_DOWN)
        return modal;   /* consume everything while open */

    if (ev->key.repeat)
        return modal;

    SDL_Scancode sc = ev->key.scancode;

    /* Modal confirmation has priority over the options overlay. */
    if (g_ov.confirm_kind != OV_CONFIRM_NONE) {
        if (sc == SDL_SCANCODE_LEFT || sc == SDL_SCANCODE_RIGHT) {
            g_ov.confirm_ok = !g_ov.confirm_ok;
        } else if (sc == SDL_SCANCODE_RETURN) {
            int kind = g_ov.confirm_kind;
            bool ok = g_ov.confirm_ok;
            g_ov.confirm_kind = OV_CONFIRM_NONE;
            if (kind == OV_CONFIRM_QUIT) {
                if (ok)
                    Main_RequestQuit(false);
            } else if (kind == OV_CONFIRM_SAVE) {
                if (ok) {
                    overlay_apply_pending();
                    overlay_save_config();
                    overlay_config_save();
                } else {
                    overlay_discard();
                }
                overlay_close_now();
            }
        } else if (sc == SDL_SCANCODE_ESCAPE) {
            if (g_ov.confirm_kind == OV_CONFIRM_SAVE) {
                overlay_discard();
                overlay_close_now();
            }
            g_ov.confirm_kind = OV_CONFIRM_NONE;
        }
        return true;
    }

    /* "N = new" size chooser. */
    if (g_ov.choice_visible) {
        int count = 0;
        overlay_new_sizes(g_ov.choice_kind, &count);
        if (sc == SDL_SCANCODE_UP) {
            if (g_ov.choice_index > 0) g_ov.choice_index--;
        } else if (sc == SDL_SCANCODE_DOWN) {
            if (g_ov.choice_index < count - 1) g_ov.choice_index++;
        } else if (sc == SDL_SCANCODE_RETURN) {
            overlay_choice_accept();
        } else if (sc == SDL_SCANCODE_ESCAPE) {
            g_ov.choice_visible = false;
        }
        return true;
    }

/* F9 always toggles the overlay. */
	if (sc == SDL_SCANCODE_F9) {
		if (!g_ov.visible) {
			g_ov.visible = true;
			g_ov.section = OV_GENERAL;
			g_ov.row     = 0;
			/* Snapshot the state so the changes can be discarded. */
			g_saved_params = ConfigureParams;
			g_saved_ui89   = UI89Config_;
			g_ov.dirty     = false;
			/* Release the emulated mouse so the host cursor can navigate. */
			if (bGrabMouse) {
				bGrabMouse = false;
				Screen_SetMouseGrab(false);
			}
		} else {
			overlay_close();
		}
		return true;
	}

    if (!g_ov.visible) return false;

    if (g_ov.about_visible) {
        if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_ESCAPE)
            g_ov.about_visible = false;
        return true;
    }

    switch (sc) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_RIGHT: {
            int dir = (sc == SDL_SCANCODE_RIGHT) ? 1 : -1;
            int s = g_ov.section;
            do {
                s = (s + dir + OV_SECTION_COUNT) % OV_SECTION_COUNT;
            } while (!section_available((OvSection)s));
            g_ov.section = (OvSection)s;
            g_ov.row = 0;
            break;
        }
        case SDL_SCANCODE_UP:
            if (g_ov.row > 0) g_ov.row--;
            break;
        case SDL_SCANCODE_DOWN:
            if (g_ov.row < section_rows() - 1) g_ov.row++;
            break;
        case SDL_SCANCODE_RETURN:
            overlay_activate();
            break;
        case SDL_SCANCODE_DELETE:
            overlay_clear_media();
            break;
        case SDL_SCANCODE_N:
            overlay_new_media();
            break;
        case SDL_SCANCODE_ESCAPE:
            overlay_close();
            break;
        default:
            break;
    }
    return true;
}

void overlay_tick(void) {
    if (!g_ov.dialog_ready) return;
    SDL_MemoryBarrierAcquire();
    g_ov.dialog_ready = false;
    OvDialogKind kind = g_ov.dialog_kind;
    g_ov.dialog_kind = OV_DIALOG_NONE;

    remember_dir(kind, g_ov.dialog_path);

    if (g_ov.new_size > 0) {
        if (g_ov.dialog_path[0])
            overlay_create_media(kind, g_ov.dialog_path, g_ov.new_size);
        g_ov.new_size = 0;
        return;
    }
    if (!g_ov.dialog_path[0])
        return;

    switch (kind) {
        case OV_DIALOG_SCSI0:
        case OV_DIALOG_SCSI1:
        case OV_DIALOG_SCSI2:
        case OV_DIALOG_SCSI3:
        case OV_DIALOG_SCSI4:
        case OV_DIALOG_SCSI5:
        case OV_DIALOG_SCSI6:
            set_scsi_image(kind - OV_DIALOG_SCSI0, g_ov.dialog_path);
            break;
        case OV_DIALOG_FLOPPY0:
        case OV_DIALOG_FLOPPY1:
            set_floppy_image(kind - OV_DIALOG_FLOPPY0, g_ov.dialog_path);
            break;
        case OV_DIALOG_MO0:
        case OV_DIALOG_MO1:
            set_mo_image(kind - OV_DIALOG_MO0, g_ov.dialog_path);
            break;
        case OV_DIALOG_ROM030:
        case OV_DIALOG_ROM040:
        case OV_DIALOG_ROMTURBO:
            set_rom_path(kind, g_ov.dialog_path);
            break;
        default:
            break;
    }
}