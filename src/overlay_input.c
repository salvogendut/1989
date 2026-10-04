/* Input details in Extensions. Capture and text entry never reach the guest. */
#include "main.h"
#include "overlay_input.h"

enum { INPUT_LINEAR, INPUT_EXPONENTIAL, INPUT_ACTION, INPUT_MOD, INPUT_PLAIN, INPUT_BACK };
static const char *const actions[SHORTCUT_KEYS] = {
    NULL, "Fullscreen", "Mouse capture", "Restart (confirmed)",
    "Screenshot (PNG/TIFF)", "Sound recording (AIFF)", "Sound on/off", "68k debugger",
    "i860 debugger", "Pause", "Quit (confirmed)", "Switch display", NULL, "Title bar"
};

void OverlayInput_Close(OverlayInput *s) {
    OverlayText_Close(&s->editor);
    memset(s, 0, sizeof(*s));
}
void OverlayInput_Open(OverlayInput *s) { OverlayInput_Close(s); s->visible = true; s->action = SHORTCUT_FULLSCREEN; }

static int *binding(OverlayInput *s, CNF_PARAMS *p) {
    return s->row == INPUT_MOD ? &p->Shortcut.withModifier[s->action] : &p->Shortcut.withoutModifier[s->action];
}

static bool reserved(SDL_Keycode key, SDL_Scancode sc, bool modifier) {
    if (key == SDLK_F4 || key == SDLK_F6 || key == SDLK_F9 ||
        sc == SDL_SCANCODE_F4 || sc == SDL_SCANCODE_F6 || sc == SDL_SCANCODE_F9) return true;
    if (sc >= SDL_SCANCODE_LCTRL && sc <= SDL_SCANCODE_RGUI) return true;
    /* Desktop handlers precede Ctrl+Alt shortcuts, on physical scancodes. */
    return modifier && (sc == SDL_SCANCODE_RETURN || sc == SDL_SCANCODE_V ||
        sc == SDL_SCANCODE_EQUALS || sc == SDL_SCANCODE_MINUS ||
        sc == SDL_SCANCODE_KP_PLUS || sc == SDL_SCANCODE_KP_MINUS);
}

static void capture(OverlayInput *s, const SDL_KeyboardEvent *key, CNF_PARAMS *p) {
    if (key->repeat) return;
    if (key->scancode == SDL_SCANCODE_ESCAPE) { s->capturing = false; return; }
    if (key->scancode == SDL_SCANCODE_DELETE) {
        *binding(s, p) = 0; s->capturing = false; return;
    }
    bool modifier = s->row == INPUT_MOD;
    if (!key->key || reserved(key->key, key->scancode, modifier)) {
        snprintf(s->message, sizeof(s->message), "That key is reserved by the desktop or is a modifier. Choose another.");
        return;
    }
    int *keys = modifier ? p->Shortcut.withModifier : p->Shortcut.withoutModifier;
    for (int i = 0; i < SHORTCUT_KEYS; i++) {
        if (i == s->action || !actions[i] || keys[i] != (int)key->key) continue;
        snprintf(s->message, sizeof(s->message), "Already assigned to %s. Clear that binding first.", actions[i]);
        return;
    }
    *binding(s, p) = (int)key->key;
    s->capturing = false;
    s->message[0] = 0;
}

/* Decimal only, locale independent, accepts the legacy comma separator.
 * Reject invalid/out-of-range input instead of clamping a mistyped value. */
static bool scale_value(const char *text, double min, double max, float *out) {
    double value = 0, place = 0;
    bool digit = false;
    for (; *text; text++) {
        if (*text == '.' || *text == ',') { if (place) return false; place = .1; }
        else if (*text >= '0' && *text <= '9') {
            digit = true;
            if (place) { value += (*text - '0') * place; place *= .1; }
            else value = value * 10 + (*text - '0');
        } else return false;
    }
    if (!digit || !isfinite(value) || value < min || value > max) return false;
    *out = (float)value;
    return true;
}

void OverlayInput_Event(OverlayInput *s, const SDL_Event *event, CNF_PARAMS *p) {
    if (s->capturing) {
        if (event->type == SDL_EVENT_KEY_DOWN) capture(s, &event->key, p);
        return;
    }
    if (s->editor.active) {
        if (OverlayText_Event(&s->editor, event) == OV_TEXT_ACCEPT) {
            bool linear = s->row == INPUT_LINEAR;
            float value;
            if (!scale_value(s->editor.value, linear ? MOUSE_LIN_MIN : MOUSE_EXP_MIN,
                             linear ? MOUSE_LIN_MAX : MOUSE_EXP_MAX, &value)) {
                snprintf(s->editor.error, sizeof(s->editor.error), "%s",
                    linear ? "Enter a number from 0.01 to 10.0." : "Enter a number from 0.50 to 1.00.");
                return;
            }
            if (strcmp(s->editor.value, s->editor.original)) {
                if (linear) p->Mouse.fLinScale = value; else p->Mouse.fExpScale = value;
            }
            OverlayText_Close(&s->editor);
        }
        return;
    }
    if (event->type != SDL_EVENT_KEY_DOWN) return;
    SDL_Scancode key = event->key.scancode;
    if (key == SDL_SCANCODE_ESCAPE || (key == SDL_SCANCODE_RETURN && s->row == INPUT_BACK)) {
        OverlayInput_Close(s); return;
    }
    s->message[0] = 0;
    if (key == SDL_SCANCODE_UP && s->row > 0) s->row--;
    else if (key == SDL_SCANCODE_DOWN && s->row < INPUT_BACK) s->row++;
    else if (key == SDL_SCANCODE_DELETE && (s->row == INPUT_MOD || s->row == INPUT_PLAIN)) *binding(s, p) = 0;
    else if (key == SDL_SCANCODE_RETURN) {
        if (s->row == INPUT_LINEAR || s->row == INPUT_EXPONENTIAL) {
            char value[32];
            snprintf(value, sizeof(value), "%.6g", s->row == INPUT_LINEAR ? p->Mouse.fLinScale : p->Mouse.fExpScale);
            OverlayText_Begin(&s->editor, value);
        } else if (s->row == INPUT_ACTION) {
            do { s->action = (s->action + 1) % SHORTCUT_KEYS; } while (!actions[s->action]);
        } else s->capturing = true;
    }
}

void OverlayInput_AddRows(const OverlayInput *s, OverlayView *view, const CNF_PARAMS *p) {
    char value[80];
    OverlayView_Heading(view, "Exact mouse sensitivity");
    snprintf(value, sizeof(value), "%.6g (0.01 to 10.0)", p->Mouse.fLinScale);
    OverlayView_Add(view, "Linear scale", value, s->row == INPUT_LINEAR);
    snprintf(value, sizeof(value), "%.6g (0.50 to 1.00)", p->Mouse.fExpScale);
    OverlayView_Add(view, "Exponential scale", value, s->row == INPUT_EXPONENTIAL);
    OverlayView_Heading(view, "Configurable shortcuts");
    OverlayView_Add(view, "Action", actions[s->action], s->row == INPUT_ACTION);
    int key = p->Shortcut.withModifier[s->action];
    OverlayView_Add(view, "With Ctrl+Alt", key ? SDL_GetKeyName((SDL_Keycode)key) : "Not assigned", s->row == INPUT_MOD);
    key = p->Shortcut.withoutModifier[s->action];
    OverlayView_Add(view, "Without Ctrl+Alt", key ? SDL_GetKeyName((SDL_Keycode)key) : "Not assigned", s->row == INPUT_PLAIN);
    OverlayView_Add(view, "Back", "Extensions", s->row == INPUT_BACK);
    view->hint = s->message[0] ? s->message : "F4 screenshot, F6 GIF and F9 options are fixed. Duplicate bindings are rejected.";
    view->footer = "Up/Down=navigate  Enter=edit  Del=clear binding  Esc=back  F9=Save/Discard";
}

void OverlayInput_DrawEditor(const OverlayInput *s, SDL_Renderer *r) {
    OverlayText_Draw(&s->editor, r, s->row == INPUT_LINEAR ? "Linear mouse scale" : "Exponential mouse scale");
    if (s->capturing) {
        const char *lines[] = {actions[s->action], s->row == INPUT_MOD ? "Choose the key to use with Ctrl+Alt." : "Choose the key to use without Ctrl+Alt.",
            s->message[0] ? s->message : "Press a key. Esc cancels; Delete clears the binding."};
        OverlayView_Dialog(r, lines, 3, "Press key", "Esc", true);
    }
}
