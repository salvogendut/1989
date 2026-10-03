/* paste.h — type the host clipboard into the emulated NeXT keyboard.
 *
 * Ctrl+V copies SDL_GetClipboardText() into paste_start(); paste_tick()
 * then types it one character at a time through the normal keymap (with a
 * short hold and gap, and Shift pressed around shifted characters), so the
 * machine sees ordinary key events. Call paste_tick() regularly from the
 * main event loop.
 */

#ifndef PREV_PASTE_H
#define PREV_PASTE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Start typing text, replacing any paste already in progress. */
void paste_start(const char *text);
/* Type the next step if it is time; call regularly. */
void paste_tick(void);
/* Stop and forget the current paste. */
void paste_stop(void);
/* True while text is still being typed. */
bool paste_active(void);

#ifdef __cplusplus
}
#endif

#endif /* PREV_PASTE_H */
