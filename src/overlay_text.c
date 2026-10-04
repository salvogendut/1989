/* Shared text entry for overlay detail pages. Owners validate on acceptance. */
#include "overlay_text.h"
#include "overlay_view.h"
#include "sdlscreen.h"
#include <stdio.h>
#include <string.h>

void OverlayText_Close(OverlayText *s) {
    if (s->active && sdlWindow) SDL_StopTextInput(sdlWindow);
    s->active = false;
}

void OverlayText_Begin(OverlayText *s, const char *value) {
    memset(s, 0, sizeof(*s));
    snprintf(s->value, sizeof(s->value), "%s", value);
    snprintf(s->original, sizeof(s->original), "%s", value);
    s->active = s->replace = true;
    if (sdlWindow) SDL_StartTextInput(sdlWindow);
}

static void append(OverlayText *s, const char *value) {
    size_t len = s->replace ? 0 : strlen(s->value), extra = strlen(value);
    if (len + extra >= sizeof(s->value)) {
        snprintf(s->error, sizeof(s->error), "Maximum length is 63 characters.");
        return;
    }
    for (size_t i = 0; i < extra; i++)
        if ((unsigned char)value[i] < 32 || (unsigned char)value[i] > 126) {
            snprintf(s->error, sizeof(s->error), "Use ASCII letters, numbers and punctuation.");
            return;
        }
    memcpy(s->value + len, value, extra + 1);
    s->replace = false;
    s->error[0] = 0;
}

OverlayTextResult OverlayText_Event(OverlayText *s, const SDL_Event *event) {
    if (event->type == SDL_EVENT_TEXT_INPUT) append(s, event->text.text);
    if (event->type != SDL_EVENT_KEY_DOWN) return OV_TEXT_PENDING;
    SDL_Scancode key = event->key.scancode;
    if (key == SDL_SCANCODE_ESCAPE) { OverlayText_Close(s); return OV_TEXT_CANCEL; }
    if (key == SDL_SCANCODE_RETURN && !event->key.repeat) return OV_TEXT_ACCEPT;
    if ((event->key.mod & SDL_KMOD_CTRL) && key == SDL_SCANCODE_A) s->replace = true;
    else if ((event->key.mod & SDL_KMOD_CTRL) && key == SDL_SCANCODE_V) {
        char *value = SDL_GetClipboardText();
        if (value) { append(s, value); SDL_free(value); }
    } else if (key == SDL_SCANCODE_BACKSPACE || key == SDL_SCANCODE_DELETE) {
        size_t len = strlen(s->value);
        if (s->replace) s->value[0] = 0;
        else if (len) s->value[len - 1] = 0;
        s->replace = false;
        s->error[0] = 0;
    }
    return OV_TEXT_PENDING;
}

void OverlayText_Draw(const OverlayText *s, SDL_Renderer *r, const char *title) {
    if (!s->active) return;
    char value[72];
    snprintf(value, sizeof(value), s->replace ? "[%s]" : "%s_", s->value);
    const char *lines[] = {title, value,
        s->error[0] ? s->error : "Type to replace; Ctrl+A selects all; Ctrl+V pastes.",
        "Enter accepts the edit. Esc cancels it."};
    OverlayView_Dialog(r, lines, 4, "Enter", "Esc", true);
}
