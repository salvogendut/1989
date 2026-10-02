/* overlay.h — F9 options overlay for 1989 (happy-years conventions).
 *
 * A translucent options panel drawn on top of the SDL renderer, opened with
 * F9. It has four tabs:
 *
 *   General    - machine model, RAM, CPU/FPU/DSP/MMU, boot device, Tinker
 *   Media      - boot device, SCSI targets, floppy drives, magneto-optical
 *   Extensions - NeXTdimension, printer, Ethernet, tablet, microphone
 *   Advanced   - display/capture/tinkering options (only with Tinker on)
 *
 * Settings are stored in the existing ConfigureParams structure plus a small
 * "[UI89]" config section (tinker, GIF, notifications) managed here. */

#ifndef PREV_OVERLAY_H
#define PREV_OVERLAY_H

#include <SDL3/SDL.h>
#include <stdbool.h>

typedef enum {
    OV_GENERAL = 0,
    OV_MEDIA,
    OV_EXTENSIONS,
    OV_ADVANCED,   /* shown only when tinker is enabled */
    OV_SECTION_COUNT
} OvSection;

/* Pending native file-dialog request. */
typedef enum {
    OV_DIALOG_NONE = 0,
    OV_DIALOG_SCSI0,
    OV_DIALOG_SCSI1,
    OV_DIALOG_SCSI2,
    OV_DIALOG_SCSI3,
    OV_DIALOG_FLOPPY0,
    OV_DIALOG_FLOPPY1,
    OV_DIALOG_MO0,
    OV_DIALOG_MO1,
    OV_DIALOG_ROM030,
    OV_DIALOG_ROM040,
    OV_DIALOG_ROMTURBO,
} OvDialogKind;

/* 1989 UI settings persisted in the [UI89] config section. */
typedef struct {
    bool bTinker;       /* gate the Advanced tab */
    bool bSmoothing;    /* linear framebuffer filtering */
    bool bDebug;        /* show emulator debug/log output on the terminal */
    int  nGifWidth;     /* recorded GIF width (320/480/640) */
    int  nGifFps;       /* recorded GIF frame rate (10/20/25) */
    int  nNotifyMode;   /* NotifyMode */
} UI89Config;

extern UI89Config UI89Config_;

void overlay_init(void);
void overlay_quit(void);

/* Returns true if the event was consumed by the overlay. */
bool overlay_handle_event(const SDL_Event *ev);

/* Draw the overlay on top of the current renderer frame. The renderer is
 * expected to be in logical window coordinates. */
void overlay_render(SDL_Renderer *r);

/* Call once per frame to process any pending file-dialog result. */
void overlay_tick(void);

bool overlay_is_visible(void);
/* True while the quit-confirmation modal is shown. */
bool overlay_confirm_visible(void);
void overlay_close(void);

/* Load/save the [UI89] section of the per-user configuration file. */
void overlay_config_load(void);
void overlay_config_save(void);

/* Show the happy-years style quit confirmation modal. The actual quit
 * happens when the user accepts (via Main_RequestQuit(false)). */
void overlay_confirm_quit(void);

/* Save the whole ConfigureParams configuration file (as the F12 dialog
 * does when a change is applied). */
void overlay_save_config(void);

/* Refresh the activity-LED enable/colour state from ConfigureParams. */
void overlay_update_leds(void);

#endif /* PREV_OVERLAY_H */