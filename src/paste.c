#include "paste.h"
#include "sdlkeymap.h"

#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

/* How long each step of the typing takes (ms). A character is typed as
 * [Shift down] key down, key up [Shift up], one step per tick. */
#define PASTE_STEP_MS 25

typedef struct {
    SDL_Scancode sc;
    bool         shift;
} PasteKey;

/* Printable ASCII -> US-layout SDL scancode. Unmapped entries are skipped. */
static const PasteKey ascii_map[128] = {
    ['\n'] = { SDL_SCANCODE_RETURN, false },
    ['\r'] = { SDL_SCANCODE_RETURN, false },
    ['\t'] = { SDL_SCANCODE_TAB, false },
    [' ']  = { SDL_SCANCODE_SPACE, false },

    ['0']  = { SDL_SCANCODE_0, false }, ['1'] = { SDL_SCANCODE_1, false },
    ['2']  = { SDL_SCANCODE_2, false }, ['3'] = { SDL_SCANCODE_3, false },
    ['4']  = { SDL_SCANCODE_4, false }, ['5'] = { SDL_SCANCODE_5, false },
    ['6']  = { SDL_SCANCODE_6, false }, ['7'] = { SDL_SCANCODE_7, false },
    ['8']  = { SDL_SCANCODE_8, false }, ['9'] = { SDL_SCANCODE_9, false },

    ['a'] = { SDL_SCANCODE_A, false }, ['b'] = { SDL_SCANCODE_B, false },
    ['c'] = { SDL_SCANCODE_C, false }, ['d'] = { SDL_SCANCODE_D, false },
    ['e'] = { SDL_SCANCODE_E, false }, ['f'] = { SDL_SCANCODE_F, false },
    ['g'] = { SDL_SCANCODE_G, false }, ['h'] = { SDL_SCANCODE_H, false },
    ['i'] = { SDL_SCANCODE_I, false }, ['j'] = { SDL_SCANCODE_J, false },
    ['k'] = { SDL_SCANCODE_K, false }, ['l'] = { SDL_SCANCODE_L, false },
    ['m'] = { SDL_SCANCODE_M, false }, ['n'] = { SDL_SCANCODE_N, false },
    ['o'] = { SDL_SCANCODE_O, false }, ['p'] = { SDL_SCANCODE_P, false },
    ['q'] = { SDL_SCANCODE_Q, false }, ['r'] = { SDL_SCANCODE_R, false },
    ['s'] = { SDL_SCANCODE_S, false }, ['t'] = { SDL_SCANCODE_T, false },
    ['u'] = { SDL_SCANCODE_U, false }, ['v'] = { SDL_SCANCODE_V, false },
    ['w'] = { SDL_SCANCODE_W, false }, ['x'] = { SDL_SCANCODE_X, false },
    ['y'] = { SDL_SCANCODE_Y, false }, ['z'] = { SDL_SCANCODE_Z, false },

    ['A'] = { SDL_SCANCODE_A, true }, ['B'] = { SDL_SCANCODE_B, true },
    ['C'] = { SDL_SCANCODE_C, true }, ['D'] = { SDL_SCANCODE_D, true },
    ['E'] = { SDL_SCANCODE_E, true }, ['F'] = { SDL_SCANCODE_F, true },
    ['G'] = { SDL_SCANCODE_G, true }, ['H'] = { SDL_SCANCODE_H, true },
    ['I'] = { SDL_SCANCODE_I, true }, ['J'] = { SDL_SCANCODE_J, true },
    ['K'] = { SDL_SCANCODE_K, true }, ['L'] = { SDL_SCANCODE_L, true },
    ['M'] = { SDL_SCANCODE_M, true }, ['N'] = { SDL_SCANCODE_N, true },
    ['O'] = { SDL_SCANCODE_O, true }, ['P'] = { SDL_SCANCODE_P, true },
    ['Q'] = { SDL_SCANCODE_Q, true }, ['R'] = { SDL_SCANCODE_R, true },
    ['S'] = { SDL_SCANCODE_S, true }, ['T'] = { SDL_SCANCODE_T, true },
    ['U'] = { SDL_SCANCODE_U, true }, ['V'] = { SDL_SCANCODE_V, true },
    ['W'] = { SDL_SCANCODE_W, true }, ['X'] = { SDL_SCANCODE_X, true },
    ['Y'] = { SDL_SCANCODE_Y, true }, ['Z'] = { SDL_SCANCODE_Z, true },

    ['!']  = { SDL_SCANCODE_1, true },
    ['@']  = { SDL_SCANCODE_2, true },
    ['#']  = { SDL_SCANCODE_3, true },
    ['$']  = { SDL_SCANCODE_4, true },
    ['%']  = { SDL_SCANCODE_5, true },
    ['^']  = { SDL_SCANCODE_6, true },
    ['&']  = { SDL_SCANCODE_7, true },
    ['*']  = { SDL_SCANCODE_8, true },
    ['(']  = { SDL_SCANCODE_9, true },
    [')']  = { SDL_SCANCODE_0, true },
    ['-']  = { SDL_SCANCODE_MINUS, false },
    ['_']  = { SDL_SCANCODE_MINUS, true },
    ['=']  = { SDL_SCANCODE_EQUALS, false },
    ['+']  = { SDL_SCANCODE_EQUALS, true },
    ['[']  = { SDL_SCANCODE_LEFTBRACKET, false },
    ['{']  = { SDL_SCANCODE_LEFTBRACKET, true },
    [']']  = { SDL_SCANCODE_RIGHTBRACKET, false },
    ['}']  = { SDL_SCANCODE_RIGHTBRACKET, true },
    ['\\'] = { SDL_SCANCODE_BACKSLASH, false },
    ['|']  = { SDL_SCANCODE_BACKSLASH, true },
    [';']  = { SDL_SCANCODE_SEMICOLON, false },
    [':']  = { SDL_SCANCODE_SEMICOLON, true },
    ['\''] = { SDL_SCANCODE_APOSTROPHE, false },
    ['"']  = { SDL_SCANCODE_APOSTROPHE, true },
    [',']  = { SDL_SCANCODE_COMMA, false },
    ['<']  = { SDL_SCANCODE_COMMA, true },
    ['.']  = { SDL_SCANCODE_PERIOD, false },
    ['>']  = { SDL_SCANCODE_PERIOD, true },
    ['/']  = { SDL_SCANCODE_SLASH, false },
    ['?']  = { SDL_SCANCODE_SLASH, true },
    ['`']  = { SDL_SCANCODE_GRAVE, false },
    ['~']  = { SDL_SCANCODE_GRAVE, true },
};

enum {
    PS_PICK = 0,      /* choose the next character */
    PS_SHIFT_DOWN,
    PS_KEY_DOWN,
    PS_KEY_UP,
    PS_SHIFT_UP,
    PS_NEXT
};

static char *p_buf;
static int   p_len;
static int   p_pos;
static int   p_state;
static PasteKey p_cur;
static Uint64 p_next_ms;

static void paste_send(SDL_Scancode sc, SDL_Keymod mod, bool down) {
    SDL_KeyboardEvent ev;

    SDL_zero(ev);
    ev.type         = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    ev.scancode     = sc;
    ev.mod          = mod;
    ev.key          = SDL_GetKeyFromScancode(sc, mod, false);
    if (down)
        Keymap_KeyDown(&ev);
    else
        Keymap_KeyUp(&ev);
}

static bool paste_mapped(unsigned char c) {
    return c < 128 && ascii_map[c].sc != SDL_SCANCODE_UNKNOWN;
}

void paste_start(const char *text) {
    paste_stop();
    if (!text || !text[0])
        return;
    p_len = (int)strlen(text);
    p_buf = malloc((size_t)p_len + 1);
    if (!p_buf) {
        p_len = 0;
        return;
    }
    memcpy(p_buf, text, (size_t)p_len + 1);
    p_pos     = 0;
    p_state   = PS_PICK;
    p_next_ms = SDL_GetTicks();
}

void paste_stop(void) {
    free(p_buf);
    p_buf = NULL;
    p_len = p_pos = 0;
    p_state = PS_PICK;
}

bool paste_active(void) {
    return p_buf != NULL && p_pos < p_len;
}

void paste_tick(void) {
    Uint64 now;

    if (!paste_active())
        return;
    now = SDL_GetTicks();
    if (now < p_next_ms)
        return;
    p_next_ms = now + PASTE_STEP_MS;

    switch (p_state) {
        case PS_PICK:
            while (p_pos < p_len &&
                   !paste_mapped((unsigned char)p_buf[p_pos]))
                p_pos++;
            if (p_pos >= p_len) {
                paste_stop();
                return;
            }
            p_cur = ascii_map[(unsigned char)p_buf[p_pos]];
            p_state = p_cur.shift ? PS_SHIFT_DOWN : PS_KEY_DOWN;
            break;

        case PS_SHIFT_DOWN:
            paste_send(SDL_SCANCODE_LSHIFT, SDL_KMOD_LSHIFT, true);
            p_state = PS_KEY_DOWN;
            break;

        case PS_KEY_DOWN:
            paste_send(p_cur.sc, p_cur.shift ? SDL_KMOD_LSHIFT : SDL_KMOD_NONE,
                       true);
            p_state = PS_KEY_UP;
            break;

        case PS_KEY_UP:
            paste_send(p_cur.sc, p_cur.shift ? SDL_KMOD_LSHIFT : SDL_KMOD_NONE,
                       false);
            p_state = p_cur.shift ? PS_SHIFT_UP : PS_NEXT;
            break;

        case PS_SHIFT_UP:
            paste_send(SDL_SCANCODE_LSHIFT, SDL_KMOD_LSHIFT, false);
            p_state = PS_NEXT;
            break;

        case PS_NEXT:
        default:
            p_pos++;
            p_state = PS_PICK;
            break;
    }
}
