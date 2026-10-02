/* leds.h — activity LED bar rendered at the bottom of the 1989 window.
 *
 * Categories (color coded):
 *   68K CPU     - grey / white (with CPU clock label)
 *   DSP 56001   - dark blue / bright blue
 *   SCSI disk   - dark red / bright red
 *   Floppy      - dark green / bright green
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
/* Signal one frame of activity for the given LED. */
void leds_ping(LedId id);

/* Render the LED bar across (x,y,w,h) in logical window coordinates. */
void leds_render(SDL_Renderer *r, int x, int y, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* PREV_LEDS_H */