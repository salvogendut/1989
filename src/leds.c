#include "leds.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    uint8_t dr, dg, db;   /* idle (dark)  colour */
    uint8_t br, bg, bb;   /* active (bright) colour */
} LedPalette;

static const LedPalette palette[LED_COUNT] = {
    [LED_CPU]    = { 55, 55, 55,  255, 255, 255 },
    [LED_DSP]    = { 20, 35, 75,   80, 150, 255 },
    [LED_SCSI]   = { 70, 18, 18,  255,  70,  70 },
    [LED_FLOPPY] = { 18, 70, 18,   80, 255,  80 },
    [LED_MO]     = { 15, 60, 60,   80, 220, 220 },
    [LED_NET]    = { 80, 70, 18,  255, 224,  32 },
    [LED_SND]    = { 70, 30, 70,  230,  90, 230 },
    [LED_ND]     = { 80, 45, 18,  255, 160,  60 },
};

static bool    g_enabled[LED_COUNT];
static unsigned g_cpu_mhz = 25;
static Uint64  g_last_ms[LED_COUNT];

static const char *led_label(LedId id) {
    switch (id) {
    case LED_CPU:    return "68K";
    case LED_DSP:    return "DSP";
    case LED_SCSI:   return "SCSI";
    case LED_FLOPPY: return "FLOPPY";
    case LED_MO:     return "MAG-OPT";
    case LED_NET:    return "ETHERNET";
    case LED_SND:    return "SOUND";
    case LED_ND:     return "NEXTDIM";
    case LED_COUNT:  break;
    }
    return "";
}

static int led_lamp_width(LedId id) {
    (void)id;
    return 14;
}

void leds_set_enabled(LedId id, bool enabled) {
    if ((unsigned)id < LED_COUNT) g_enabled[id] = enabled;
}

void leds_set_cpu_frequency(unsigned mhz) {
    if (mhz >= 20 && mhz <= 100) g_cpu_mhz = mhz;
}

void leds_ping(LedId id) {
    if ((unsigned)id < LED_COUNT) g_last_ms[id] = SDL_GetTicks();
}

void leds_render(SDL_Renderer *r, int x, int y, int w, int h) {
    /* Bar background */
    SDL_SetRenderDrawColor(r, 18, 18, 18, 255);
    SDL_FRect bg = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(r, &bg);

    /* Top hairline separator */
    SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
    SDL_FRect line = { (float)x, (float)y, (float)w, 1.0f };
    SDL_RenderFillRect(r, &line);

    const int led_h = 10;
    const int pad   = 8;

    /* Each LED is a lamp plus a text label; width = lamp + gap + text. */
    int n = 0, total_w = 0;
    for (int i = 0; i < LED_COUNT; i++) {
        if (!g_enabled[i]) continue;
        int text_w = (int)strlen(led_label((LedId)i)) * 8;
        total_w += led_lamp_width((LedId)i) + 6 + text_w;
        n++;
    }
    if (n == 0) return;
    total_w += (n - 1) * pad;

    int cx = x + (w - total_w) / 2;
    int cy = y + (h - led_h) / 2;

    Uint64 now = SDL_GetTicks();
    for (int i = 0; i < LED_COUNT; i++) {
        if (!g_enabled[i]) continue;

        const LedPalette *p = &palette[i];
        Uint64 dt = now - g_last_ms[i];
        bool active = g_last_ms[i] != 0 && dt < LED_GLOW_MS;

        SDL_FRect lamp = { (float)cx, (float)cy, 14.0f, (float)led_h };
        SDL_SetRenderDrawColor(r,
            active ? p->br : p->dr,
            active ? p->bg : p->dg,
            active ? p->bb : p->db, 255);
        SDL_RenderFillRect(r, &lamp);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderRect(r, &lamp);

        char label[32];
        if (i == LED_CPU)
            snprintf(label, sizeof(label), "68K %uM", g_cpu_mhz);
        else
            snprintf(label, sizeof(label), "%s", led_label((LedId)i));
        SDL_SetRenderDrawColor(r, 205, 205, 205, 255);
        SDL_RenderDebugText(r, (float)(cx + 20), (float)(cy + 1), label);

        int text_w = (int)strlen(label) * 8;
        cx += 14 + 6 + text_w + pad;
    }
}