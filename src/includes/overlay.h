/* overlay.h — F9 options overlay for 1989 (happy-years conventions).
 *
 * A translucent options panel drawn on top of the SDL renderer, opened with
 * F9. It has four tabs:
 *
 *   General    - machine model, RAM, CPU/FPU/DSP/MMU, sound, Tinker
 *   Media      - boot device/diagnostics, SCSI, floppy, magneto-optical
 *   Extensions - devices, printer output, keyboard/mouse controls
 *   Advanced   - display/capture/tinkering options (only with Tinker on)
 *
 * The controller edits private drafts; settings.c owns runtime application
 * and ui_config.c owns the additional [UI89] preferences. */

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
/* True while a save, restart or quit confirmation is shown. */
bool overlay_confirm_visible(void);
void overlay_close(void);

/* Show the happy-years style quit confirmation modal. The actual quit
 * happens when the user accepts (via Main_RequestQuit(false)). */
void overlay_confirm_quit(void);
void overlay_confirm_reset(void);

/* Refresh the activity-LED enable/colour state from ConfigureParams. */
void overlay_update_leds(void);

/* Name of the currently selected machine model (e.g. "NeXT Computer"). */
const char *overlay_machine_name(void);

#endif /* PREV_OVERLAY_H */
