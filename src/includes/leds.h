/* leds.h — activity LED bar rendered at the bottom of the 1989 window.
 *
 * Categories (color coded):
 *   68K CPU     - grey / white (with CPU clock label)
 *   DSP 56001   - dark blue / bright blue
 *   SCSI disk   - dark green / bright green (one lamp per attached target
 *                 when more than one disk is in use)
 *   Floppy      - dark red / bright red
 *   Magneto-opt - dark cyan / bright cyan
 *   Ethernet    - dark yellow / bright yellow
 *   Sound       - dark purple / bright magenta
 *   NeXTdim     - dark orange / bright orange
 *
 * Activity is signalled with leds_ping() from the device emulation; the LED
 * glows bright for LED_GLOW_MS milliseconds after each ping, then fades back
 * to its dark idle colour.
 */

#ifndef PREV_LEDS_H
#define PREV_LEDS_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LED_BAR_H     22
#define FUNCTION_KEY_BAR_H 16

#define LED_GLOW_MS   120

/* Number of SCSI targets that can get their own lamp when several disks are
 * attached (matches ESP_MAX_DEVS). */
#define LED_SCSI_TARGETS 7

typedef enum {
    LED_CPU = 0,
    LED_DSP,
    LED_SCSI,
    LED_FLOPPY,
    LED_MO,
    LED_NET,
    LED_SND,
    LED_ND,
    LED_COUNT
} LedId;

/* Configure which LEDs to display in the bar. Call after reading config. */
void leds_set_enabled(LedId id, bool enabled);
/* Update the CPU clock label (MHz) shown beside the CPU lamp. */
void leds_set_cpu_frequency(unsigned mhz);
/* Mark a SCSI target as attached. When more than one target is attached the
 * single SCSI lamp is replaced by one lamp per target ("SCSI 0", ...). */
void leds_set_scsi_present(int target, bool present);
/* Signal one frame of activity for the given LED. */
void leds_ping(LedId id);
/* Signal one frame of activity for a SCSI target's lamp. */
void leds_ping_scsi(int target);

/* Render the LED bar across (x,y,w,h) in logical window coordinates. */
void leds_render(SDL_Renderer *r, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* PREV_LEDS_H */