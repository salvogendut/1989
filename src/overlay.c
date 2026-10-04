/* overlay.c — F9 options overlay for 1989 (happy-years conventions).
 *
 * Navigation and edit-session controller. Drawing, media IO, persistence
 * and runtime application live in their respective modules.
 */

#include "overlay.h"
#include "ui_config.h"
#include "settings.h"
#include "overlay_media.h"
#include "overlay_view.h"
#include "overlay_controls.h"
#include "overlay_devices.h"
#include "overlay_input.h"
#include "paste.h"
#include "configuration.h"
#include "ffmpeg_gif.h"
#include "grab.h"
#include "leds.h"
#include "notify.h"
#include "reset.h"
#include "screen.h"
#include "main.h"
#include "sdlscreen.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif
#ifndef PROG_GIT_COMMIT
#define PROG_GIT_COMMIT "unknown"
#endif

static SettingsSession g_settings;
static OverlayDevices g_devices;
static OverlayInput g_input;
static const char *g_validation_error;
static UI89Config g_edit_ui, g_saved_ui89;

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

static int current_machine_index(const CNF_PARAMS *params) {
    for (int i = 0; i < MACHINE_COUNT; i++) {
        if (machines[i].nMachineType == params->System.nMachineType &&
            machines[i].bTurbo      == params->System.bTurbo &&
            machines[i].bColor      == params->System.bColor)
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
    GEN_SOUND,
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
    ADV_TITLEBAR,
    ADV_ROM030,
    ADV_ROM040,
    ADV_ROMTURBO,
    ADV_VERSION,
    ADV_LOGICAL_COUNT
};

/* Number of selectable rows in the Advanced tab (Scanlines is conditional). */
static int adv_row_count(void) {
    return ADV_LOGICAL_COUNT - (g_edit_ui.bCrtEnabled ? 0 : 1);
}

/* Map a displayed Advanced row to its logical ADV_* row. */
static int adv_logical_row(int row) {
    if (!g_edit_ui.bCrtEnabled && row > ADV_REAL_CRT)
        return row + 1;   /* skip the hidden Scanlines row */
    return row;
}

static struct {
    bool      visible;
    bool      about_visible;
    bool      media_help_visible;
    OvSection section;
    int       row;

    /* Modal confirmation (happy-years style, replaces the legacy alert). */
    int       confirm_kind;   /* OV_CONFIRM_* */
    bool      confirm_ok;     /* true = OK selected, false = Cancel */

    bool need_reset; /* derived from the current diff when closing */
    bool need_media;
    bool need_mo_warning;

    /* "N = new" size chooser for media entries. */
    bool         choice_visible;
    OvDialogKind choice_kind;    /* media entry being created */
    int          choice_index;   /* selected size */
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
    OV_CONFIRM_SAVE,    /* save (or discard) changes when closing */
    OV_CONFIRM_RESET    /* explicit F5/shortcut restart */
};


static const char *const about_lines[] = {
    "1989 NeXT (Motorola 68K) emulator",
    "(c) 2026 salvogendut",
    "Version " PACKAGE_VERSION " (commit " PROG_GIT_COMMIT ")",
    "Previous 4.4 + r1854 - WinUAE 68k, Hatari, i860 by Jason Eckhardt"
};
#define ABOUT_LINE_COUNT ((int)(sizeof(about_lines) / sizeof(about_lines[0])))

static const char *const media_help_lines[] = {
    "NeXT SCSI layout - suggested roles, not fixed assignments",
    "ID 1: system disk   ID 2: data disk   ID 3: CD-ROM",
    "ID 0: alternate boot   ID 4: SCSI floppy   ID 5: spare",
    "ID 6: extra/swap disk   ID 7: host controller (reserved)",
    "NEXTSTEP assigns sdN in ascending order of attached IDs.",
    "IDs 1, 2, 3, 6 become sd0, sd1, sd2, sd3 at boot.",
    "Adding ID 0 makes it sd0 and shifts higher-ID drives.",
    "The preview describes the next boot, not live guest names.",
    "Native floppy and MO drives use separate controllers.",
    "Enter loads/replaces. E ejects media, keeping its drive.",
    "C connects an empty native drive; Del disconnects a drive.",
    "Native floppy: drive 0, 68040 only. Native MO: non-Turbo Cube.",
    "A second native MO can hang/crash NEXTSTEP (kernel bug).",
    "Eject/unmount in NeXT first. Eject applies on Save, no reset."
};
#define MEDIA_HELP_LINE_COUNT ((int)(sizeof(media_help_lines) / sizeof(media_help_lines[0])))

/* ------------------------------------------------------------------ */
/* helpers                                                             */

static const char *machine_name(void) {
    return machines[current_machine_index(&g_settings.draft)].name;
}

static void machine_cycle(int dir) {
    int i = current_machine_index(&g_settings.draft);
    i = (i + dir + MACHINE_COUNT) % MACHINE_COUNT;
    g_settings.draft.System.nMachineType = machines[i].nMachineType;
    g_settings.draft.System.bTurbo       = machines[i].bTurbo;
    g_settings.draft.System.bColor       = machines[i].bColor;
}

static const char *ram_string(char *buf, size_t size) {
    int mb = 0;
    for (int i = 0; i < 4; i++)
        mb += g_settings.draft.Memory.nMemoryBankSize[i];
    snprintf(buf, size, "%d MB (%d/%d/%d/%d)", mb,
             g_settings.draft.Memory.nMemoryBankSize[0],
             g_settings.draft.Memory.nMemoryBankSize[1],
             g_settings.draft.Memory.nMemoryBankSize[2],
             g_settings.draft.Memory.nMemoryBankSize[3]);
    return buf;
}

static const char *cpu_freq_string(char *buf, size_t size) {
    snprintf(buf, size, "%d MHz (%s)", g_settings.draft.System.nCpuFreq,
             g_settings.draft.System.bRealtime ? "variable" : "fixed");
    return buf;
}

static const char *fpu_string(char *buf, size_t size) {
    switch (g_settings.draft.System.n_FPUType) {
        case FPU_NONE:  snprintf(buf, size, "None"); break;
        case FPU_68881: snprintf(buf, size, "68881"); break;
        case FPU_68882: snprintf(buf, size, "68882"); break;
        default:        snprintf(buf, size, "CPU (68040)"); break;
    }
    return buf;
}

static const char *dsp_string(char *buf, size_t size) {
    switch (g_settings.draft.System.nDSPType) {
        case DSP_TYPE_NONE:    snprintf(buf, size, "None"); break;
        case DSP_TYPE_ACCURATE:snprintf(buf, size, "Accurate"); break;
        default:               snprintf(buf, size, "Emulated"); break;
    }
    return buf;
}

static const char *boot_string(char *buf, size_t size) {
    switch (g_settings.draft.Boot.nBootDevice) {
        case BOOT_ROM:      snprintf(buf, size, "ROM monitor"); break;
        case BOOT_SCSI:     snprintf(buf, size, "SCSI disk"); break;
        case BOOT_ETHERNET: snprintf(buf, size, "Ethernet (netboot)"); break;
        case BOOT_MO:       snprintf(buf, size, "Magneto-optical"); break;
        default:            snprintf(buf, size, "Floppy"); break;
    }
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
    const SCSIDISK *disk = &g_settings.draft.SCSI.target[i];
    SCSI_DEVTYPE device = OverlayMedia_ScsiType(&g_settings.draft, i);
    const char *type = device == SD_CD ? "CD-ROM" : device == SD_FLOPPY ? "SCSI floppy" : "HDD";
    char pbuf[64];
    if (disk->nDeviceType == SD_NONE || (device == SD_HARDDISK && !disk->bDiskInserted))
        snprintf(buf, size, "Not connected - load as %s", type);
    else
        snprintf(buf, size, "%s [%s]  %s", type,
                 disk->bWriteProtected || device == SD_CD ? "read-only" : "read/write",
                 disk->bDiskInserted ? media_path(disk->szImageName, pbuf, sizeof(pbuf)) : "[empty drive]");
    return buf;
}

static const char *removable_value(bool connected, bool inserted, bool protected,
                                   const char *path, char *buf, size_t size) {
    char pbuf[64];
    if (!connected) snprintf(buf, size, "Not connected - C connects; Enter loads");
    else snprintf(buf, size, "[%s]  %s", protected ? "read-only" : "read/write",
                  inserted ? media_path(path, pbuf, sizeof(pbuf)) : "[empty drive]");
    return buf;
}

static const char *floppy_value(int i, char *buf, size_t size) {
    const FLPDISK *disk = &g_settings.draft.Floppy.drive[i];
    if (!disk->bDriveConnected && OverlayMedia_Unavailable(&g_settings.draft, OV_DIALOG_FLOPPY0 + i))
        return i ? "Unavailable - hardware has one native drive" : "Unavailable - requires a 68040 model";
    return removable_value(disk->bDriveConnected, disk->bDiskInserted,
                           disk->bWriteProtected, disk->szImageName, buf, size);
}

static const char *mo_value(int i, char *buf, size_t size) {
    const MODISK *disk = &g_settings.draft.MO.drive[i];
    if (!disk->bDriveConnected && OverlayMedia_Unavailable(&g_settings.draft, OV_DIALOG_MO0 + i))
        return "Unavailable - requires a non-Turbo Cube";
    return removable_value(disk->bDriveConnected, disk->bDiskInserted,
                           disk->bWriteProtected, disk->szImageName, buf, size);
}

/* Refresh the activity-LED enable/colour state from the running machine. */
void overlay_update_leds(void) {
	leds_set_enabled(LED_CPU, true);
	leds_set_enabled(LED_DSP,
	                 ConfigureParams.System.nDSPType != DSP_TYPE_NONE);
	leds_set_enabled(LED_SCSI, true);
	for (int i = 0; i < ESP_MAX_DEVS; i++)
		leds_set_scsi_present(i, ConfigureParams.SCSI.target[i].bDiskInserted);
	leds_set_enabled(LED_FLOPPY, true);
	leds_set_enabled(LED_MO, true);
	leds_set_enabled(LED_NET, ConfigureParams.Ethernet.bEthernetConnected);
	leds_set_enabled(LED_SND, ConfigureParams.Sound.bEnableSound);
	bool dimension = false;
	for (int i = 0; i < ND_MAX_BOARDS; i++) dimension |= ConfigureParams.Dimension.board[i].bEnabled;
	leds_set_enabled(LED_ND, dimension);
}

/* Name of the currently selected machine model. */
const char *overlay_machine_name(void) {
	return machines[current_machine_index(&ConfigureParams)].name;
}

/* Read the running configuration, never the unconfirmed options draft. */
void overlay_machine_summary(char *buffer, size_t size) {
    int ram = 0;
    for (int i = 0; i < 4; i++) ram += ConfigureParams.Memory.nMemoryBankSize[i];
    snprintf(buffer, size, "1989 %s | %d MHz%s | %d MB", overlay_machine_name(),
             ConfigureParams.System.nCpuFreq,
             ConfigureParams.System.bRealtime ? " (variable)" : "", ram);
}

/* ------------------------------------------------------------------ */
/* file dialogs                                                        */

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
        SCSI_DEVTYPE type = OverlayMedia_ScsiType(&g_settings.draft, kind - OV_DIALOG_SCSI0);
        if (type == SD_CD) return NULL;
        if (type == SD_FLOPPY) {
            *count = (int)(sizeof(new_floppy_sizes) / sizeof(new_floppy_sizes[0]));
            return new_floppy_sizes;
        }
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
    const char *reason = OverlayMedia_Unavailable(&g_settings.draft, kind);
    if (reason) { notify_post("%s", reason); return; }
    if (!overlay_new_sizes(kind, &count)) {
        if (kind != OV_DIALOG_NONE) notify_post("LOAD AN EXISTING CD IMAGE, OR USE T TO CHANGE DRIVE TYPE");
        return;
    }
    g_ov.choice_visible = true;
    g_ov.choice_kind = kind;
    g_ov.choice_index = 0;
}

/* Size chosen: remember it and open the save dialog for the new image. */
static void overlay_choice_accept(void) {
    int count = 0;
    const OvNewSize *sizes = overlay_new_sizes(g_ov.choice_kind, &count);
    g_ov.choice_visible = false;
    if (sizes && g_ov.choice_index >= 0 && g_ov.choice_index < count)
        OverlayMedia_Request(g_ov.choice_kind, &g_edit_ui, sizes[g_ov.choice_index].bytes);
}

/* ------------------------------------------------------------------ */
/* row actions                                                         */

static void overlay_activate(void) {
    switch (g_ov.section) {
        case OV_GENERAL:
            if (g_ov.row >= GEN_ROWS) {
                OverlayControls_Activate(OV_GENERAL, g_ov.row - GEN_ROWS, &g_settings.draft);
                break;
            }
            switch (g_ov.row) {
                case GEN_MACHINE:
                    machine_cycle(1);
                    Configuration_SetSystemDefaultsFor(&g_settings.draft);
                    notify_post("MACHINE CHANGED - RESTART REQUIRED");
                    break;
                case GEN_RAM: {
                    static const int totals[] = {8, 16, 32, 64, 128};
                    int total = 0;
                    for (int i = 0; i < 4; i++) total += g_settings.draft.Memory.nMemoryBankSize[i];
                    int limit = g_settings.draft.System.bTurbo ? 128 :
                        (g_settings.draft.System.bColor || g_settings.draft.System.nMachineType == NEXT_STATION) ? 32 : 64;
                    int next = 8;
                    for (int i = 0; i < 5; i++)
                        if (totals[i] > total && totals[i] <= limit) { next = totals[i]; break; }
                    int bank = g_settings.draft.System.bTurbo && next >= 64 ? 32 :
                        (g_settings.draft.System.bTurbo || g_settings.draft.System.bColor) ? 8 : next == 8 ? 4 : 16;
                    for (int i = 0; i < 4; i++) {
                        g_settings.draft.Memory.nMemoryBankSize[i] = next > 0 ? bank : 0;
                        next -= bank;
                    }
                    break;
                }
                case GEN_CPUCLOCK:
                    OverlayControls_CycleCpuClock(&g_settings.draft);
                    break;
                case GEN_FPU: {
                    FPUTYPE f = g_settings.draft.System.n_FPUType;
                    f = f == FPU_NONE ? FPU_68881 :
                        f == FPU_68881 ? FPU_68882 :
                        f == FPU_68882 ? FPU_CPU : FPU_NONE;
                    g_settings.draft.System.n_FPUType = f;
                    notify_post("FPU CHANGED - RESTART REQUIRED");
                    break;
                }
                case GEN_DSP: {
                    DSPTYPE d = g_settings.draft.System.nDSPType;
                    d = d == DSP_TYPE_NONE ? DSP_TYPE_EMU :
                        d == DSP_TYPE_EMU ? DSP_TYPE_ACCURATE : DSP_TYPE_NONE;
                    g_settings.draft.System.nDSPType = d;
                    notify_post("DSP CHANGED - RESTART REQUIRED");
                    break;
                }
                case GEN_MMU:
                    notify_post("THE NEXT CPU CORE ALWAYS ENABLES THE MMU");
                    break;
                case GEN_ADB:
                    if (!g_settings.draft.System.bTurbo) {
                        notify_post("ADB IS AVAILABLE ON TURBO MACHINES");
                        break;
                    }
                    g_settings.draft.System.bADB =
                        !g_settings.draft.System.bADB;
                    notify_post("ADB CHANGED - RESTART REQUIRED");
                    break;
                case GEN_SOUND:
                    g_settings.draft.Sound.bEnableSound = !g_settings.draft.Sound.bEnableSound;
                    break;
                case GEN_TINKER:
                    g_edit_ui.bTinker = !g_edit_ui.bTinker;
                    if (!g_edit_ui.bTinker && g_ov.section == OV_ADVANCED)
                        g_ov.section = OV_GENERAL;
                    break;
                case GEN_ABOUT:
                    g_ov.about_visible = true;
                    break;
                case GEN_RESET:
                    Configuration_SetSystemDefaultsFor(&g_settings.draft);
                    g_ov.row = 0;
                    break;
                default:
                    break;
            }
            break;

        case OV_MEDIA:
            if (g_ov.row == MED_BOOT) {
                BOOT_DEVICE b = g_settings.draft.Boot.nBootDevice;
                b = (BOOT_DEVICE)(((int)b + 1) % 5);
                g_settings.draft.Boot.nBootDevice = b;
                notify_post("BOOT DEVICE SELECTED FOR THE NEXT BOOT");
            } else if (g_ov.row >= MED_ROWS) {
                OverlayControls_Activate(OV_MEDIA, g_ov.row - MED_ROWS, &g_settings.draft);
            } else {
                OvDialogKind kind = media_row_dialog(g_ov.row);
                const char *reason = OverlayMedia_Unavailable(&g_settings.draft, kind);
                if (reason) notify_post("%s", reason);
                else OverlayMedia_Request(kind, &g_edit_ui, 0);
            }
            break;

        case OV_EXTENSIONS: {
            if (g_ov.row == OverlayControls_Count(OV_EXTENSIONS) + 1) {
                OverlayInput_Open(&g_input);
                break;
            }
            if (g_ov.row == OverlayControls_Count(OV_EXTENSIONS)) {
                OverlayDevices_Open(&g_devices, OV_DEVICES_NETWORK, &g_settings.draft);
                break;
            }
            OvDialogKind picker = OverlayControls_Activate(OV_EXTENSIONS, g_ov.row, &g_settings.draft);
            if (picker != OV_DIALOG_NONE) OverlayMedia_Request(picker, &g_edit_ui, 0);
            break;
        }

        case OV_ADVANCED:
            if (g_ov.row == adv_row_count() + OverlayControls_Count(OV_ADVANCED)) {
                OverlayDevices_Open(&g_devices, OV_DEVICES_DIMENSION, &g_settings.draft);
                break;
            }
            if (g_ov.row >= adv_row_count()) {
                OverlayControls_Activate(OV_ADVANCED, g_ov.row - adv_row_count(), &g_settings.draft);
                break;
            }
            switch (adv_logical_row(g_ov.row)) {
                case ADV_SMOOTHING:
                    g_edit_ui.bSmoothing = !g_edit_ui.bSmoothing;
                    break;
                case ADV_REAL_CRT:
                    g_edit_ui.bCrtEnabled = !g_edit_ui.bCrtEnabled;
                    notify_post(g_edit_ui.bCrtEnabled
                                ? "REAL CRT ON" : "REAL CRT OFF");
                    break;
                case ADV_SCANLINES:
                    g_edit_ui.nCrtScanlines += 5;
                    if (g_edit_ui.nCrtScanlines > 95)
                        g_edit_ui.nCrtScanlines = 0;
                    break;
                case ADV_GIF_WIDTH:
                    g_edit_ui.nGifWidth =
                        g_edit_ui.nGifWidth >= 640 ? 320 :
                        g_edit_ui.nGifWidth + 160;
                    break;
                case ADV_GIF_FPS:
                    g_edit_ui.nGifFps =
                        g_edit_ui.nGifFps >= 25 ? 10 :
                        g_edit_ui.nGifFps < 20 ? 20 : 25;
                    break;
                case ADV_GIF_ENCODER:
                    if (!FFMPEG_GIF_SUPPORTED) {
                        notify_post("FFMPEG NOT AVAILABLE");
                        break;
                    }
                    g_edit_ui.bGifFfmpeg = !g_edit_ui.bGifFfmpeg;
                    notify_post(g_edit_ui.bGifFfmpeg
                                ? "GIF ENCODER: FFMPEG OPTIMIZE"
                                : "GIF ENCODER: BUILT-IN");
                    break;
                case ADV_NOTIFICATIONS: {
                    int m = g_edit_ui.nNotifyMode + 1;
                    if (m > NOTIFY_MODE_CONSOLE) m = NOTIFY_MODE_OFF;
                    g_edit_ui.nNotifyMode = m;
                    break;
                }
                case ADV_DEBUG:
                    g_edit_ui.bDebug = !g_edit_ui.bDebug;
                    notify_post(g_edit_ui.bDebug
                                ? "DEBUG OUTPUT ON" : "DEBUG OUTPUT OFF");
                    break;
                case ADV_RTC_CLOCK:
                    g_edit_ui.bRtcLocalTime = !g_edit_ui.bRtcLocalTime;
                    notify_post(g_edit_ui.bRtcLocalTime
                                ? "RTC CLOCK: LOCAL TIME"
                                : "RTC CLOCK: UTC");
                    break;
                case ADV_FULLSCREEN:
                    g_settings.draft.Screen.bFullScreen = !g_settings.draft.Screen.bFullScreen;
                    break;
                case ADV_TITLEBAR:
                    g_settings.draft.Screen.bShowTitlebar =
                        !g_settings.draft.Screen.bShowTitlebar;
                    break;
                case ADV_ROM030:
                case ADV_ROM040:
                case ADV_ROMTURBO:
                    OverlayMedia_Request(OV_DIALOG_ROM030 + adv_logical_row(g_ov.row) - ADV_ROM030,
                                         &g_edit_ui, 0);
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

/* ------------------------------------------------------------------ */
/* rendering                                                           */

static int section_rows(void) {
    switch (g_ov.section) {
        case OV_GENERAL:    return GEN_ROWS + OverlayControls_Count(OV_GENERAL);
        case OV_MEDIA:      return MED_ROWS + OverlayControls_Count(OV_MEDIA);
        case OV_EXTENSIONS: return OverlayControls_Count(OV_EXTENSIONS) + 2;
        case OV_ADVANCED:   return adv_row_count() + OverlayControls_Count(OV_ADVANCED) + 1;
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
    if (s == OV_ADVANCED) return g_edit_ui.bTinker;
    return true;
}

/* Happy-years style modal confirmation (dark panel, dimmed backdrop). */
static void overlay_render_confirm(SDL_Renderer *r) {
    static const char *const quit_lines[] = {
        "Shut down NeXT before quitting.", "Quit the emulator now?"
    };
    static const char *const reset_lines[] = {
        "Shut down NeXT before restarting.", "Restart the emulated machine now?"
    };
    static const char *const save_lines[] = {
        "Save the changes to 1989.conf?", "Esc returns to editing."
    };
    static const char *const hardware_lines[] = {
        "Hardware or fixed-disk changes need a restart.",
        "Shut down NeXT first. Restart now?", "Esc returns to editing."
    };
    static const char *const mo_hardware_lines[] = {
        "Hardware changes need a restart. Shut down NeXT first.",
        "A second native MO drive can hang/crash NEXTSTEP (kernel bug).",
        "Connect it and restart now?", "Esc returns to editing."
    };
    static const char *const media_lines[] = {
        "Eject or unmount changed media in NeXT first.",
        "Apply the selected media changes?", "Esc returns to editing."
    };
    bool save = g_ov.confirm_kind == OV_CONFIRM_SAVE;
    const char *const *lines = save ?
        (g_ov.need_reset ? (g_ov.need_mo_warning ? mo_hardware_lines : hardware_lines) : g_ov.need_media ? media_lines : save_lines) :
        g_ov.confirm_kind == OV_CONFIRM_RESET ? reset_lines : quit_lines;
    const char *accept = save ? (g_ov.need_reset ? "Restart" : "Save") :
        g_ov.confirm_kind == OV_CONFIRM_RESET ? "Restart" : "Quit";
    OverlayView_Dialog(r, lines, save && g_ov.need_mo_warning ? 4 : save && (g_ov.need_reset || g_ov.need_media) ? 3 : 2,
                       accept, save ? "Discard" : "Cancel", g_ov.confirm_ok);
}

/* Render the "N = new" size chooser: a small centred panel listing the
 * available image sizes for the selected media entry. */
static void overlay_render_choice(SDL_Renderer *r) {
    int count = 0;
    const OvNewSize *sizes = overlay_new_sizes(g_ov.choice_kind, &count);
    const char *choices[8];
    if (!sizes || count > 8) return;
    for (int i = 0; i < count; i++) choices[i] = sizes[i].label;
    OverlayView_Choices(r, "New blank image: choose size", choices, count, g_ov.choice_index);
}

static void overlay_media_rows(OverlayView *view, char *hint, size_t hint_size) {
    char label[64], value[128];
    OverlayView_Add(view, "Next boot device", boot_string(value, sizeof(value)), g_ov.row == MED_BOOT);
    OverlayView_Heading(view, "SCSI bus - suggested roles; actual drive types are on the right");
    for (int id = 0; id < ESP_MAX_DEVS; id++) {
        snprintf(label, sizeof(label), "ID %d  %s", id, OverlayMedia_ScsiRole(id));
        OverlayView_Add(view, label, scsi_value(id, value, sizeof(value)), g_ov.row == MED_SCSI0 + id);
    }
    OverlayView_Add(view, "ID 7  Host controller", "Reserved - no peripheral", false);
    OverlayView_Heading(view, "Native removable drives - separate from the SCSI bus");
    for (int i = 0; i < 2; i++) {
        snprintf(label, sizeof(label), "Floppy drive %d", i);
        OverlayView_Add(view, label, floppy_value(i, value, sizeof(value)), g_ov.row == MED_FLOPPY0 + i);
    }
    for (int i = 0; i < 2; i++) {
        snprintf(label, sizeof(label), "Native MO drive %d", i);
        OverlayView_Add(view, label, mo_value(i, value, sizeof(value)), g_ov.row == MED_MO0 + i);
    }
    view->hint = "SCSI IDs are not sdN numbers. H explains drive order and suggested roles.";
    view->footer = "Arrows=navigate  Enter=change  H=help  F9/Esc=close";
    if (g_ov.row >= MED_SCSI0 && g_ov.row <= MED_SCSI6) {
        int id = g_ov.row - MED_SCSI0;
        int number = OverlayMedia_ScsiDiskNumber(&g_settings.draft, id);
        if (number >= 0) {
            snprintf(hint, hint_size, "Expected next boot: ID %d -> sd%d. Adding/removing lower IDs shifts drive numbers.", id, number);
            view->hint = hint;
        }
        SCSI_DEVTYPE type = OverlayMedia_ScsiType(&g_settings.draft, id);
        view->footer = type == SD_HARDDISK
            ? "Enter=load/replace  N=new  T=type  W=protect  Del=disconnect  H=help  F9=close"
            : type == SD_CD
            ? "Enter=load/replace  E=eject  T=type  Del=disconnect  H=help  F9=close"
            : "Enter=load/replace  E=eject  N=new  T=type  W=protect  Del=disconnect  H=help";
    } else if (g_ov.row >= MED_FLOPPY0 && g_ov.row <= MED_MO1) {
        view->hint = "Eject removes only the medium. The connected drive stays available; Save applies.";
        OvDialogKind kind = media_row_dialog(g_ov.row);
        const char *reason = OverlayMedia_Unavailable(&g_settings.draft, kind);
        if (reason) view->hint = reason;
        else if (kind == OV_DIALOG_MO1)
            view->hint = "Connecting a second native MO needs a restart and can hang/crash NEXTSTEP (kernel bug).";
        view->footer = reason ? "E=eject  Del=disconnect  H=help  F9=close" :
            "Enter=load  C=connect  E=eject  N=new  W=protect  Del=disconnect  H=help  F9=close";
    }
}

void overlay_render(SDL_Renderer *r) {
    if (g_validation_error) {
        const char *lines[] = {"Settings need attention", g_validation_error, "Return to editing, or Discard changes when closing."};
        OverlayView_Dialog(r, lines, 3, "OK", NULL, true);
        return;
    }
    if (g_ov.confirm_kind != OV_CONFIRM_NONE) {
        overlay_render_confirm(r);
        return;
    }
    if (g_ov.choice_visible) {
        overlay_render_choice(r);
        return;
    }
    if (!g_ov.visible) return;
    if (g_ov.media_help_visible) {
        OverlayView_Dialog(r, media_help_lines, MEDIA_HELP_LINE_COUNT, "OK", NULL, true);
        return;
    }

    OverlayView view = {0};
    for (int section = 0; section < OV_SECTION_COUNT; section++) {
        if (!section_available(section)) continue;
        if (section == (int)g_ov.section) view.active_tab = view.tab_count;
        view.tabs[view.tab_count++] = section_name(section);
    }
    if (g_input.visible) {
        OverlayInput_AddRows(&g_input, &view, &g_settings.draft);
        OverlayView_Draw(r, &view);
        OverlayInput_DrawEditor(&g_input, r);
        return;
    }
    if (g_devices.page != OV_DEVICES_NONE) {
        OverlayDevices_AddRows(&g_devices, &view, &g_settings.draft);
        OverlayView_Draw(r, &view);
        OverlayDevices_DrawEditor(&g_devices, r);
        return;
    }
    char vbuf[FILENAME_MAX + 8];
    char s1[64], media_hint[128];
    view.footer = "Arrows=navigate  Enter=change  F9/Esc=close";

    if (g_ov.section == OV_GENERAL) {
        OverlayView_Add(&view, "Machine", machine_name(), g_ov.row == GEN_MACHINE);
        OverlayView_Add(&view, "RAM", ram_string(vbuf, sizeof(vbuf)),
                 g_ov.row == GEN_RAM);
        OverlayView_Add(&view, "CPU clock", cpu_freq_string(s1, sizeof(s1)),
                 g_ov.row == GEN_CPUCLOCK);
        OverlayView_Add(&view, "FPU", fpu_string(s1, sizeof(s1)),
                 g_ov.row == GEN_FPU);
        OverlayView_Add(&view, "DSP", dsp_string(s1, sizeof(s1)),
                 g_ov.row == GEN_DSP);
        OverlayView_Add(&view, "MMU", "On (required by core)",
                 g_ov.row == GEN_MMU);
        OverlayView_Add(&view, "ADB", !g_settings.draft.System.bTurbo ? "Turbo models only" : g_settings.draft.System.bADB ? "On" : "Off",
                 g_ov.row == GEN_ADB);
        OverlayView_Add(&view, "Sound", g_settings.draft.Sound.bEnableSound ? "On" : "Off",
                        g_ov.row == GEN_SOUND);
        OverlayView_Add(&view, "Tinker", g_edit_ui.bTinker ? "On" : "Off",
                 g_ov.row == GEN_TINKER);
        OverlayView_Add(&view, "About", "Program details", g_ov.row == GEN_ABOUT);
        OverlayView_Add(&view, "Machine defaults", "Restore this model's hardware defaults", g_ov.row == GEN_RESET);
        OverlayControls_AddRows(&view, OV_GENERAL, g_ov.row - GEN_ROWS, &g_settings.draft);
    } else if (g_ov.section == OV_MEDIA) {
        overlay_media_rows(&view, media_hint, sizeof(media_hint));
        OverlayControls_AddRows(&view, OV_MEDIA, g_ov.row - MED_ROWS, &g_settings.draft);
    } else if (g_ov.section == OV_EXTENSIONS) {
        OverlayControls_AddRows(&view, OV_EXTENSIONS, g_ov.row, &g_settings.draft);
        OverlayView_Add(&view, "Network / NFS", "Backend, cable, MAC and shared folders...",
                        g_ov.row == OverlayControls_Count(OV_EXTENSIONS));
        OverlayView_Add(&view, "Input details", "Exact mouse scales and shortcuts...",
                        g_ov.row == OverlayControls_Count(OV_EXTENSIONS) + 1);
    } else {
        char vbuf2[64];
        int dr = 0;
        OverlayView_Add(&view, "Smoothing",
                 g_edit_ui.bSmoothing ? "On" : "Off",
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "Real CRT",
                 g_edit_ui.bCrtEnabled ? "On" : "Off",
                 g_ov.row == dr); dr++;
        if (g_edit_ui.bCrtEnabled) {
            snprintf(vbuf2, sizeof(vbuf2), "%d%%", g_edit_ui.nCrtScanlines);
            OverlayView_Add(&view, "Scanlines", vbuf2,
                     g_ov.row == dr); dr++;
        }
        {
            char gline[64];
            snprintf(gline, sizeof(gline), "%dx%d",
                     g_edit_ui.nGifWidth,
                     g_edit_ui.nGifWidth * 832 / 1120);
            OverlayView_Add(&view, "GIF resolution", gline,
                     g_ov.row == dr); dr++;
        }
        {
            char fps[32];
            snprintf(fps, sizeof(fps), "%d fps", g_edit_ui.nGifFps);
            OverlayView_Add(&view, "GIF frame rate", fps,
                     g_ov.row == dr); dr++;
        }
        if (!FFMPEG_GIF_SUPPORTED)
            snprintf(vbuf2, sizeof(vbuf2), "built-in [ffmpeg unavailable]");
        else
            snprintf(vbuf2, sizeof(vbuf2), "%s",
                     g_edit_ui.bGifFfmpeg ? "FFmpeg optimize" : "built-in");
        OverlayView_Add(&view, "GIF encoder", vbuf2,
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "Notifications",
                 g_edit_ui.nNotifyMode == NOTIFY_MODE_OFF ? "Off" :
                 g_edit_ui.nNotifyMode == NOTIFY_MODE_CONSOLE ? "Console" :
                 "Screen",
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "Debugging",
                 g_edit_ui.bDebug ? "On" : "Off",
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "RTC clock",
                 g_edit_ui.bRtcLocalTime ? "Local time" : "UTC",
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "Fullscreen", g_settings.draft.Screen.bFullScreen ? "On" : "Off",
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "Title bar",
                 g_settings.draft.Screen.bShowTitlebar ? "On" : "Off",
                 g_ov.row == dr); dr++;
        OverlayView_Add(&view, "68030 ROM", media_path(g_settings.draft.Rom.szRom030FileName, vbuf2, sizeof(vbuf2)), g_ov.row == dr++);
        OverlayView_Add(&view, "68040 ROM", media_path(g_settings.draft.Rom.szRom040FileName, vbuf2, sizeof(vbuf2)), g_ov.row == dr++);
        OverlayView_Add(&view, "Turbo ROM", media_path(g_settings.draft.Rom.szRomTurboFileName, vbuf2, sizeof(vbuf2)), g_ov.row == dr++);
        if (g_ov.row >= ADV_ROM030 - (!g_edit_ui.bCrtEnabled) && g_ov.row <= ADV_ROMTURBO - (!g_edit_ui.bCrtEnabled))
            view.hint = "Enter selects a ROM. D restores the discovered default path if its image exists.";
        OverlayView_Add(&view, "Version", PACKAGE_VERSION,
                 g_ov.row == dr);
        OverlayControls_AddRows(&view, OV_ADVANCED, g_ov.row - adv_row_count(), &g_settings.draft);
        OverlayView_Add(&view, "NeXTdimension / displays", "Boards, RAM, ROMs and monitor layout...",
                        g_ov.row == adv_row_count() + OverlayControls_Count(OV_ADVANCED));
    }

    OverlayView_Draw(r, &view);
    if (g_ov.about_visible)
        OverlayView_Dialog(r, about_lines, ABOUT_LINE_COUNT, "OK", NULL, true);
}

/* ------------------------------------------------------------------ */
/* lifecycle                                                           */

void overlay_init(void) {
    memset(&g_ov, 0, sizeof(g_ov));
    g_ov.section = OV_GENERAL;
    UI89_Apply();
}

void overlay_quit(void) { OverlayInput_Close(&g_input); OverlayDevices_Close(&g_devices); OverlayMedia_Cancel(); }
bool overlay_is_visible(void) { return g_ov.visible; }
bool overlay_confirm_visible(void) { return g_ov.confirm_kind != OV_CONFIRM_NONE; }

static void overlay_release_mouse(void) {
    if (bGrabMouse) {
        bGrabMouse = false;
        Screen_SetMouseGrab(false);
    }
}

void overlay_confirm_quit(void) {
    g_ov.confirm_kind = OV_CONFIRM_QUIT;
    g_ov.confirm_ok = false;
    overlay_release_mouse();
}

void overlay_confirm_reset(void) {
    g_ov.confirm_kind = OV_CONFIRM_RESET;
    g_ov.confirm_ok = false;
    overlay_release_mouse();
}

static void overlay_close_now(void) {
    OverlayInput_Close(&g_input);
    OverlayDevices_Close(&g_devices);
    g_validation_error = NULL;
    OverlayMedia_Cancel();
    g_ov.about_visible = g_ov.visible = g_ov.choice_visible = false;
    g_ov.media_help_visible = false;
    g_ov.need_reset = g_ov.need_media = false;
}

static bool overlay_changed(void) {
    return memcmp(&g_settings.original, &g_settings.draft, sizeof(CNF_PARAMS)) ||
           memcmp(&g_edit_ui, &g_saved_ui89, sizeof(g_edit_ui));
}

static bool overlay_apply_pending(void) {
    g_validation_error = OverlayMedia_Validate(&g_settings.original, &g_settings.draft);
    if (!g_validation_error) g_validation_error = OverlayDevices_Validate(&g_settings.original, &g_settings.draft);
    if (g_validation_error) return false;
    bool active = Main_PauseEmulation(false);
    bool ok = Settings_Apply(&g_settings, g_ov.need_reset);
    if (ok) {
        UI89Config_ = g_edit_ui;
        UI89_Apply();
        overlay_update_leds();
        Configuration_Save();
        UI89_Save();
        notify_post("SETTINGS SAVED");
    }
    if (active) Main_UnPauseEmulation();
    return ok;
}

void overlay_close(void) {
    if (!g_ov.visible || OverlayMedia_Busy()) return;
    if (overlay_changed()) {
        CNF_PARAMS changed;
        Settings_Merge(&g_settings, &ConfigureParams, &changed);
        g_ov.need_reset = Settings_NeedRestart(&ConfigureParams, &changed);
        g_ov.need_media = Settings_MediaChanged(&ConfigureParams, &changed);
        g_ov.need_mo_warning = !ConfigureParams.MO.drive[1].bDriveConnected && changed.MO.drive[1].bDriveConnected;
        g_ov.confirm_kind = OV_CONFIRM_SAVE;
        g_ov.confirm_ok = !g_ov.need_reset && !g_ov.need_media;
    } else overlay_close_now();
}

bool overlay_handle_event(const SDL_Event *ev) {
    bool modal = g_ov.visible || overlay_confirm_visible();
    if (g_validation_error) {
        if (ev->type == SDL_EVENT_KEY_DOWN &&
            (ev->key.scancode == SDL_SCANCODE_RETURN || ev->key.scancode == SDL_SCANCODE_ESCAPE))
            g_validation_error = NULL;
        return true;
    }
    if (g_ov.visible && !overlay_confirm_visible() && (g_input.capturing || g_input.editor.active)) {
        OverlayInput_Event(&g_input, ev, &g_settings.draft);
        return true;
    }
    if (g_ov.visible && !overlay_confirm_visible() && g_devices.editor.active) {
        OverlayDevices_Event(&g_devices, ev, &g_settings.draft);
        return true;
    }
    if (ev->type != SDL_EVENT_KEY_DOWN) return modal;
    SDL_Scancode sc = ev->key.scancode;
    if (ev->key.repeat && sc != SDL_SCANCODE_UP && sc != SDL_SCANCODE_DOWN) return modal;
    if (g_ov.confirm_kind != OV_CONFIRM_NONE) {
        if (sc == SDL_SCANCODE_LEFT || sc == SDL_SCANCODE_RIGHT) {
            g_ov.confirm_ok = !g_ov.confirm_ok;
        } else if (sc == SDL_SCANCODE_RETURN) {
            int kind = g_ov.confirm_kind;
            bool ok = g_ov.confirm_ok;
            g_ov.confirm_kind = OV_CONFIRM_NONE;
            if (kind == OV_CONFIRM_QUIT) {
                if (ok) Main_RequestQuit(false);
            } else if (kind == OV_CONFIRM_RESET) {
                if (ok) {
                    bool active = Main_PauseEmulation(false);
                    paste_stop();
                    if (Reset_Cold()) Main_RequestQuit(false);
                    if (active) Main_UnPauseEmulation();
                }
            } else if (!ok || overlay_apply_pending()) {
                overlay_close_now();
            }
        } else if (sc == SDL_SCANCODE_ESCAPE) {
            /* Escape returns to editing; Discard is an explicit choice. */
            g_ov.confirm_kind = OV_CONFIRM_NONE;
        }
        return true;
    }
    if (modal && OverlayMedia_Busy()) return true;
    if (g_ov.choice_visible) {
        int count = 0;
        overlay_new_sizes(g_ov.choice_kind, &count);
        if (sc == SDL_SCANCODE_UP && g_ov.choice_index > 0) g_ov.choice_index--;
        else if (sc == SDL_SCANCODE_DOWN && g_ov.choice_index < count - 1) g_ov.choice_index++;
        else if (sc == SDL_SCANCODE_RETURN) overlay_choice_accept();
        else if (sc == SDL_SCANCODE_ESCAPE) g_ov.choice_visible = false;
        return true;
    }
    if (g_ov.media_help_visible) {
        if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_ESCAPE || sc == SDL_SCANCODE_H)
            g_ov.media_help_visible = false;
        return true;
    }
    if (sc == SDL_SCANCODE_F9) {
        if (!g_ov.visible) {
            bool active = Main_PauseEmulation(false);
            paste_stop();
            Settings_Begin(&g_settings);
            g_saved_ui89 = g_edit_ui = UI89Config_;
            g_ov.visible = true;
            g_ov.section = OV_GENERAL;
            g_ov.row = 0;
            overlay_release_mouse();
            if (active) Main_UnPauseEmulation();
        } else overlay_close();
        return true;
    }
    if (!g_ov.visible) return false;
    if (g_ov.about_visible) {
        if (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_ESCAPE) g_ov.about_visible = false;
        return true;
    }
    if (g_input.visible) {
        OverlayInput_Event(&g_input, ev, &g_settings.draft);
        return true;
    }
    if (g_devices.page != OV_DEVICES_NONE) {
        OvDialogKind picker = OverlayDevices_Event(&g_devices, ev, &g_settings.draft);
        if (picker != OV_DIALOG_NONE) OverlayMedia_Request(picker, &g_edit_ui, 0);
        return true;
    }
    switch (sc) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_RIGHT: {
            int dir = sc == SDL_SCANCODE_RIGHT ? 1 : -1;
            int section = g_ov.section;
            do { section = (section + dir + OV_SECTION_COUNT) % OV_SECTION_COUNT; }
            while (!section_available((OvSection)section));
            g_ov.section = (OvSection)section;
            g_ov.row = 0;
            break;
        }
        case SDL_SCANCODE_UP: if (g_ov.row > 0) g_ov.row--; break;
        case SDL_SCANCODE_DOWN: if (g_ov.row < section_rows() - 1) g_ov.row++; break;
        case SDL_SCANCODE_RETURN: overlay_activate(); break;
        case SDL_SCANCODE_DELETE:
            if (g_ov.section == OV_MEDIA &&
                OverlayMedia_Disconnect(&g_settings.draft, media_row_dialog(g_ov.row)))
                notify_post("DRIVE DISCONNECT STAGED - RESTART REQUIRED");
            break;
        case SDL_SCANCODE_C:
            if (g_ov.section == OV_MEDIA && OverlayMedia_Connect(&g_settings.draft, media_row_dialog(g_ov.row)))
                notify_post("EMPTY DRIVE CONNECTION STAGED - RESTART REQUIRED");
            break;
        case SDL_SCANCODE_D:
            if (g_ov.section == OV_ADVANCED && g_ov.row < adv_row_count()) {
                int row = adv_logical_row(g_ov.row);
                if (row >= ADV_ROM030 && row <= ADV_ROMTURBO &&
                    OverlayMedia_RestoreRom(&g_settings.draft, OV_DIALOG_ROM030 + row - ADV_ROM030))
                    notify_post("DEFAULT ROM STAGED - APPLY WHEN CLOSING OPTIONS");
            }
            break;
        case SDL_SCANCODE_E:
            if (g_ov.section == OV_MEDIA &&
                OverlayMedia_Eject(&g_settings.draft, media_row_dialog(g_ov.row)))
                notify_post("EJECT STAGED - DRIVE STAYS CONNECTED");
            break;
        case SDL_SCANCODE_H:
            if (g_ov.section == OV_MEDIA) g_ov.media_help_visible = true;
            break;
        case SDL_SCANCODE_N: overlay_new_media(); break;
        case SDL_SCANCODE_T:
            if (g_ov.section == OV_MEDIA && g_ov.row >= MED_SCSI0 && g_ov.row <= MED_SCSI6) {
                SCSIDISK *disk = &g_settings.draft.SCSI.target[g_ov.row - MED_SCSI0];
                SCSI_DEVTYPE type = OverlayMedia_ScsiType(&g_settings.draft, g_ov.row - MED_SCSI0);
                disk->nDeviceType = type == SD_FLOPPY ? SD_HARDDISK : type + 1;
                if (disk->nDeviceType == SD_CD) disk->bWriteProtected = true;
            }
            break;
        case SDL_SCANCODE_W:
            if (g_ov.section == OV_MEDIA) {
                OvDialogKind kind = media_row_dialog(g_ov.row);
                bool *protect = NULL;
                if (kind >= OV_DIALOG_SCSI0 && kind <= OV_DIALOG_SCSI6) {
                    SCSIDISK *disk = &g_settings.draft.SCSI.target[kind - OV_DIALOG_SCSI0];
                    if (OverlayMedia_ScsiType(&g_settings.draft, kind - OV_DIALOG_SCSI0) != SD_CD)
                        protect = &disk->bWriteProtected;
                } else if (kind == OV_DIALOG_FLOPPY0 || kind == OV_DIALOG_FLOPPY1)
                    protect = &g_settings.draft.Floppy.drive[kind - OV_DIALOG_FLOPPY0].bWriteProtected;
                else if (kind == OV_DIALOG_MO0 || kind == OV_DIALOG_MO1)
                    protect = &g_settings.draft.MO.drive[kind - OV_DIALOG_MO0].bWriteProtected;
                if (protect) *protect = !*protect;
            }
            break;
        case SDL_SCANCODE_ESCAPE: overlay_close(); break;
        default: break;
    }
    return true;
}

void overlay_tick(void) {
    OvDialogKind kind;
    long long size;
    char path[FILENAME_MAX], created[FILENAME_MAX];
    if (!OverlayMedia_Poll(&kind, path, &size) || !g_ov.visible ||
        kind == OV_DIALOG_NONE || !path[0]) return;
    if (size > 0) {
        if (!OverlayMedia_Create(path, size, created)) return;
        snprintf(path, sizeof(path), "%s", created);
    }
    if (OverlayMedia_Set(&g_settings.draft, kind, path)) {
        OverlayMedia_Remember(&g_edit_ui, kind, path);
        notify_post("SELECTION STAGED - APPLY WHEN CLOSING OPTIONS");
        Screen_RequestRepaint();
    }
}
