/* Browser host boundary. The CPU and device emulation remain the native core. */
#include "main.h"
#include "configuration.h"
#include "event.h"
#include "host.h"
#include "web_host.h"
#include "enet_slirp.h"
#include "m68000.h"
#include "timing.h"
#include "file.h"
#include "scsi.h"
#include "floppy.h"
#include "mo.h"
#include <SDL3/SDL.h>

static int ready;
static SDL_AtomicInt pause_requested, paused, present[4];

static const char *media_paths[] = {"/media/disk.img", "/media/cd.img", "/media/floppy.img", "/media/mo.img"};

static void publish_media(void) {
    SDL_SetAtomicInt(&present[0], ConfigureParams.SCSI.target[1].bDiskInserted);
    SDL_SetAtomicInt(&present[1], ConfigureParams.SCSI.target[3].bDiskInserted);
    SDL_SetAtomicInt(&present[2], ConfigureParams.Floppy.drive[0].bDiskInserted);
    SDL_SetAtomicInt(&present[3], ConfigureParams.MO.drive[0].bDiskInserted);
}

void Web_Configure(void) {
    /* A non-Turbo 040 Cube supports both native floppy and MO controllers. */
    ConfigureParams.System.nMachineType = NEXT_CUBE040;
    ConfigureParams.System.bTurbo = false;
    ConfigureParams.System.bColor = false;
    Configuration_SetSystemDefaults();
    ConfigureParams.Memory.nMemoryBankSize[0] = 16;
    ConfigureParams.Memory.nMemoryBankSize[1] = 16;
    ConfigureParams.Memory.nMemoryBankSize[2] = 0;
    ConfigureParams.Memory.nMemoryBankSize[3] = 0;
    ConfigureParams.Boot.nBootDevice = BOOT_ROM;
    ConfigureParams.Boot.bEnableSoundTest = false;
    ConfigureParams.Sound.bEnableSound = false;
    ConfigureParams.Sound.bEnableMicrophone = false;
    ConfigureParams.Ethernet.bEthernetConnected = false;
    ConfigureParams.Mouse.bEnableAutoGrab = false;
    ConfigureParams.Keyboard.nKeymapType = KEYMAP_SCANCODE;
    ConfigureParams.Log.bConfirmQuit = false;
    ConfigureParams.SCSI.target[1].nDeviceType = File_Exists(media_paths[0]) ? SD_HARDDISK : SD_NONE;
    ConfigureParams.SCSI.target[1].bDiskInserted = File_Exists(media_paths[0]);
    snprintf(ConfigureParams.SCSI.target[1].szImageName, FILENAME_MAX, "%s", media_paths[0]);
    ConfigureParams.SCSI.target[3].nDeviceType = SD_CD;
    ConfigureParams.SCSI.target[3].bDiskInserted = File_Exists(media_paths[1]);
    ConfigureParams.SCSI.target[3].bWriteProtected = true;
    snprintf(ConfigureParams.SCSI.target[3].szImageName, FILENAME_MAX, "%s", media_paths[1]);
    ConfigureParams.Floppy.drive[0].bDriveConnected = true;
    ConfigureParams.Floppy.drive[0].bDiskInserted = File_Exists(media_paths[2]);
    snprintf(ConfigureParams.Floppy.drive[0].szImageName, FILENAME_MAX, "%s", media_paths[2]);
    ConfigureParams.MO.drive[0].bDriveConnected = true;
    ConfigureParams.MO.drive[0].bDiskInserted = File_Exists(media_paths[3]);
    snprintf(ConfigureParams.MO.drive[0].szImageName, FILENAME_MAX, "%s", media_paths[3]);
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
}

void Web_Ready(void) { publish_media(); ready = 1; }
int web_ready(void) { return ready; }
double web_cycles(void) { return nCyclesMainCounter; }

/* Acknowledge pauses on the CPU thread, after outstanding I/O completes. The
 * browser must keep yielding while waiting: worker filesystem calls are proxied
 * there. Never wait for a worker from the browser thread. */
void Web_Poll(void) {
    publish_media();
    if (!SDL_GetAtomicInt(&pause_requested)) return;
    fflush(NULL);
    Timing_Pause(true);
    SDL_SetAtomicInt(&paused, 1);
    while (SDL_GetAtomicInt(&pause_requested)) host_sleep_ms(1);
    SDL_SetAtomicInt(&paused, 0);
    Timing_Pause(false);
}
void web_pause(int requested) { SDL_SetAtomicInt(&pause_requested, requested != 0); }
int web_paused(void) { return SDL_GetAtomicInt(&paused); }
int web_media_present(int device) { return device >= 0 && device < 4 ? SDL_GetAtomicInt(&present[device]) : 0; }

/* Fixed controller topology: exchange removable media without resetting CPU,
 * controller or other disks. Called by JS only after the CPU acknowledges pause. */
int web_eject(int device) {
    if (!ready || !web_paused() || device < 1 || device > 3) return 0;
    if (device == 1) {
        SCSI_Eject(3);
        ConfigureParams.SCSI.target[3].bDiskInserted = false;
    }
    if (device == 2) Floppy_Eject(0);
    if (device == 3) MO_Eject(0);
    publish_media();
    return 1;
}
int web_insert(int device) {
    if (!ready || !web_paused() || device < 1 || device > 3 || !File_Exists(media_paths[device])) return 0;
    if (device == 1) {
        snprintf(ConfigureParams.SCSI.target[3].szImageName, FILENAME_MAX, "%s", media_paths[1]);
        ConfigureParams.SCSI.target[3].bDiskInserted = true;
        SCSI_Insert(3);
    }
    if (device == 2) {
        snprintf(ConfigureParams.Floppy.drive[0].szImageName, FILENAME_MAX, "%s", media_paths[2]);
        ConfigureParams.Floppy.drive[0].bDiskInserted = true;
        if (Floppy_Insert(0)) ConfigureParams.Floppy.drive[0].bDiskInserted = false;
    }
    if (device == 3) {
        snprintf(ConfigureParams.MO.drive[0].szImageName, FILENAME_MAX, "%s", media_paths[3]);
        ConfigureParams.MO.drive[0].bDiskInserted = true;
        MO_Insert(0);
    }
    publish_media();
    return web_media_present(device);
}

/* Queue guest keys through the same synchronized path as physical SDL input. */
void web_key(int scancode, int down, int modifiers) {
    if (!ready || scancode <= 0 || scancode >= SDL_SCANCODE_COUNT) return;
    SDL_Event event = {0};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.key = SDL_GetKeyFromScancode(scancode, modifiers, false);
    event.key.mod = modifiers;
    event.key.down = down != 0;
    SDL_PushEvent(&event);
}

/* Browser networking needs its own transport; keep the cable disconnected. */
void enet_slirp_queue_poll(void) {}
void enet_slirp_input(uint8_t *packet, int length) { (void)packet; (void)length; }
void enet_slirp_stop(void) {}
void enet_slirp_start(uint8_t *mac) { (void)mac; }
void enet_slirp_uninit(void) {}
