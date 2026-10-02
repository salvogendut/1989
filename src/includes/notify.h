/* notify.h — short informational toast messages (1989 happy-years UI).
 *
 * Tri-state notification sink for short informational messages:
 *   NOTIFY_MODE_SCREEN  - fading bottom-left "smoked" toast overlay (default)
 *   NOTIFY_MODE_CONSOLE - stderr only
 *   NOTIFY_MODE_OFF     - silent
 *
 * Single global singleton - call-sites need no context handle. */

#ifndef PREV_NOTIFY_H
#define PREV_NOTIFY_H

struct SDL_Renderer;

typedef enum {
    NOTIFY_MODE_OFF = 0,
    NOTIFY_MODE_SCREEN,
    NOTIFY_MODE_CONSOLE,
} NotifyMode;

void notify_init(void);
void notify_set_mode(NotifyMode mode);

void notify_post(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void notify_tick(int dt_ms);
void notify_render(struct SDL_Renderer *r);

#endif /* PREV_NOTIFY_H */