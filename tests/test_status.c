/* Core status hooks must use existing LEDs/notifications, including messages
 * from emulator threads. The retired shortcut must not consume guest input. */
#include "main.h"
#include "configuration.h"
#include "statusbar.h"
#include "shortcut.h"
#include "leds.h"
#include "notify.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

CNF_PARAMS ConfigureParams;
static int activity[LED_COUNT];
void leds_ping(LedId id) { assert(id < LED_COUNT); activity[id]++; }

/* Observe text sent to SDL while using its real software renderer otherwise. */
static char drawn[16][128];
static int drawn_count;
bool SDL_RenderDebugText(SDL_Renderer *r, float x, float y, const char *text) {
    (void)r; (void)x; (void)y;
    assert(drawn_count < 16);
    snprintf(drawn[drawn_count++], sizeof(drawn[0]), "%s", text);
    return true;
}
static int render(SDL_Renderer *r) {
    drawn_count = 0;
    notify_render(r);
    return drawn_count;
}
static int messages(void *id) {
    for (int i = 0; i < 1000; i++) {
        char text[64];
        snprintf(text, sizeof(text), "%s message %d", (const char *)id, i);
        Statusbar_AddMessage(text, 0);
    }
    return 0;
}

int main(void) {
    ConfigureParams.Shortcut.withModifier[SHORTCUT_OPTIONS] = SDLK_O;
    ConfigureParams.Shortcut.withoutModifier[SHORTCUT_OPTIONS] = SDLK_F1;
    assert(!ShortCut_CheckKeys(SDLK_F1, false, true));
    assert(!ShortCut_CheckKeys(SDLK_O, true, true));
    SDL_Surface *surface = SDL_CreateSurface(1120, 870, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surface);
    assert(surface && r);
    notify_init();
    /* Forward as literal text, even if the core message contains '%' signs. */
    Statusbar_AddMessage("Disk 100% ready", 0);
    assert(render(r) == 1 && !strcmp(drawn[0], "Disk 100% ready"));
    notify_tick(3500);
    assert(render(r) == 0);
    notify_set_mode(NOTIFY_MODE_OFF);
    Statusbar_AddMessage("Hidden", 0);
    notify_set_mode(NOTIFY_MODE_SCREEN);
    assert(render(r) == 0);

    FILE *log = tmpfile();
    assert(log);
    int old_stderr = dup(STDERR_FILENO);
    assert(old_stderr >= 0 && dup2(fileno(log), STDERR_FILENO) >= 0);
    notify_set_mode(NOTIFY_MODE_CONSOLE);
    Statusbar_AddMessage("Cannot open SCSI disk", 5000);
    fflush(stderr);
    assert(dup2(old_stderr, STDERR_FILENO) >= 0);
    close(old_stderr);
    rewind(log);
    char line[128];
    assert(fgets(line, sizeof(line), log));
    assert(!strcmp(line, "Cannot open SCSI disk\n"));
    fclose(log);
    assert(render(r) == 0);

    notify_set_mode(NOTIFY_MODE_SCREEN);
    SDL_Thread *a = SDL_CreateThread(messages, "status-a", "device-a");
    SDL_Thread *b = SDL_CreateThread(messages, "status-b", "device-b");
    assert(a && b);
    for (int i = 0; i < 1000; i++) {
        notify_tick(1);
        assert(render(r) <= 5);
        for (int j = 0; j < drawn_count; j++) assert(!strncmp(drawn[j], "device-", 7));
    }
    SDL_WaitThread(a, NULL); SDL_WaitThread(b, NULL);
    notify_tick(3500);
    assert(render(r) == 0);

    for (int i = 0; i < NUM_DEVICE_LEDS; i++) Statusbar_BlinkLed(i);
    Statusbar_SetSystemLed(true); Statusbar_SetDspLed(true); Statusbar_SetNdLed(2);
    assert(activity[LED_NET] == 1 && activity[LED_MO] == 1 && activity[LED_SCSI] == 1);
    assert(activity[LED_FLOPPY] == 1 && activity[LED_CPU] == 1);
    assert(activity[LED_DSP] == 1 && activity[LED_ND] == 1);

    ConfigureParams.Shortcut.withModifier[SHORTCUT_STATUSBAR] = SDLK_B;
    ConfigureParams.Shortcut.withoutModifier[SHORTCUT_STATUSBAR] = SDLK_F2;
    ConfigureParams.Shortcut.withModifier[SHORTCUT_TITLEBAR] = SDLK_T;
    assert(!ShortCut_CheckKeys(SDLK_B, true, true));
    assert(!ShortCut_CheckKeys(SDLK_F2, false, true));
    assert(ShortCut_CheckKeys(SDLK_T, true, true));

    SDL_DestroyRenderer(r); SDL_DestroySurface(surface);
    puts("test-status: OK");
    return 0;
}
