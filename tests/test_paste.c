/* Exercise the clipboard paste state machine: the characters, the Shift
 * wrappers and the skipping of unmapped bytes. Keymap_KeyDown/Keymap_KeyUp
 * are stubbed so the generated key events can be inspected. */
#include "paste.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    SDL_Scancode sc;
    SDL_Keymod   mod;
    bool         down;
} Rec;

#define MAX_REC 256
static Rec rec[MAX_REC];
static int nrec;

void Keymap_KeyDown(const SDL_KeyboardEvent *e) {
    if (nrec < MAX_REC) rec[nrec++] = (Rec){ e->scancode, e->mod, true };
}

void Keymap_KeyUp(const SDL_KeyboardEvent *e) {
    if (nrec < MAX_REC) rec[nrec++] = (Rec){ e->scancode, e->mod, false };
}

static void drain(void) {
    for (int i = 0; i < 1000 && paste_active(); i++)
        SDL_Delay(5), paste_tick();
}

static int expect(int idx, SDL_Scancode sc, bool shift, bool down) {
    SDL_Keymod want = shift ? SDL_KMOD_LSHIFT : SDL_KMOD_NONE;
    if (idx >= nrec) {
        fprintf(stderr, "FAIL: missing event %d (%s %s)\n", idx,
                SDL_GetScancodeName(sc), down ? "down" : "up");
        return 1;
    }
    if (rec[idx].sc != sc || rec[idx].mod != want || rec[idx].down != down) {
        fprintf(stderr,
                "FAIL: event %d is %s mod=0x%x %s, expected %s shift=%d %s\n",
                idx, SDL_GetScancodeName(rec[idx].sc), (unsigned)rec[idx].mod,
                rec[idx].down ? "down" : "up", SDL_GetScancodeName(sc),
                shift, down ? "down" : "up");
        return 1;
    }
    return 0;
}

int main(void) {
    int fails = 0;

    /* "Hi!" -> shift+H, H, H up, shift up, i, i up, shift+1, 1, 1 up, shift up */
    nrec = 0;
    paste_start("Hi!");
    drain();
    if (nrec != 10) {
        fprintf(stderr, "FAIL: expected 10 events, got %d\n", nrec);
        return 1;
    }
    fails += expect(0, SDL_SCANCODE_LSHIFT, true, true);
    fails += expect(1, SDL_SCANCODE_H, true, true);
    fails += expect(2, SDL_SCANCODE_H, true, false);
    fails += expect(3, SDL_SCANCODE_LSHIFT, true, false);
    fails += expect(4, SDL_SCANCODE_I, false, true);
    fails += expect(5, SDL_SCANCODE_I, false, false);
    fails += expect(6, SDL_SCANCODE_LSHIFT, true, true);
    fails += expect(7, SDL_SCANCODE_1, true, true);
    fails += expect(8, SDL_SCANCODE_1, true, false);
    fails += expect(9, SDL_SCANCODE_LSHIFT, true, false);

    /* Unmapped bytes are skipped; \n becomes Return. */
    nrec = 0;
    paste_start("a\xc3\xa9\n");
    drain();
    if (nrec != 4) {
        fprintf(stderr, "FAIL: expected 4 events, got %d\n", nrec);
        return 1;
    }
    fails += expect(0, SDL_SCANCODE_A, false, true);
    fails += expect(1, SDL_SCANCODE_A, false, false);
    fails += expect(2, SDL_SCANCODE_RETURN, false, true);
    fails += expect(3, SDL_SCANCODE_RETURN, false, false);

    /* Empty text is inactive and generates nothing. */
    nrec = 0;
    paste_start("");
    drain();
    if (paste_active() || nrec != 0) {
        fprintf(stderr, "FAIL: empty paste should be inactive\n");
        fails++;
    }

    /* paste_stop cancels a running paste. */
    paste_start("hello world");
    paste_tick();
    paste_stop();
    if (paste_active()) {
        fprintf(stderr, "FAIL: paste_stop left paste active\n");
        fails++;
    }

    if (fails) return 1;
    printf("test-paste: OK\n");
    return 0;
}
