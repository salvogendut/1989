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
    [LED_SCSI]   = { 18, 70, 18,   80, 255,  80 },
    [LED_FLOPPY] = { 70, 18, 18,  255,  70,  70 },
    [LED_MO]     = { 15, 60, 60,   80, 220, 220 },
    [LED_NET]    = { 80, 70, 18,  255, 224,  32 },
    [LED_SND]    = { 70, 30, 70,  230,  90, 230 },
    [LED_ND]     = { 80, 45, 18,  255, 160,  60 },
};

static bool    g_enabled[LED_COUNT];
static unsigned g_cpu_mhz = 25;
static Uint64  g_last_ms[LED_COUNT];
static bool    g_scsi_present[LED_SCSI_TARGETS];
static Uint64  g_scsi_last_ms[LED_SCSI_TARGETS];

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

void leds_set_enabled(LedId id, bool enabled) {
    if ((unsigned)id < LED_COUNT) g_enabled[id] = enabled;
}

void leds_set_cpu_frequency(unsigned mhz) {
    if (mhz >= 20 && mhz <= 100) g_cpu_mhz = mhz;
}

void leds_set_scsi_present(int target, bool present) {
    if (target >= 0 && target < LED_SCSI_TARGETS)
        g_scsi_present[target] = present;
}

void leds_ping(LedId id) {
    if ((unsigned)id < LED_COUNT) g_last_ms[id] = SDL_GetTicks();
}

void leds_ping_scsi(int target) {
    if (target < 0 || target >= LED_SCSI_TARGETS) return;
    g_scsi_last_ms[target] = SDL_GetTicks();
    /* Keep the single "SCSI" lamp alive for the one-disk case. */
    g_last_ms[LED_SCSI] = g_scsi_last_ms[target];
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

    /* Collect the lamps to draw. The single SCSI lamp is replaced by one
     * lamp per attached target when more than one disk is in use. */
    char labels[LED_COUNT + LED_SCSI_TARGETS][32];
    const LedPalette *pals[LED_COUNT + LED_SCSI_TARGETS];
    Uint64 lasts[LED_COUNT + LED_SCSI_TARGETS];
    int n = 0;

    for (int i = 0; i < LED_COUNT; i++) {
        if (!g_enabled[i]) continue;
        if (i == LED_SCSI) {
            int present[LED_SCSI_TARGETS], np = 0;
            for (int t = 0; t < LED_SCSI_TARGETS; t++)
                if (g_scsi_present[t]) present[np++] = t;
            if (np > 1) {
                for (int k = 0; k < np; k++) {
                    int t = present[k];
                    pals[n]  = &palette[LED_SCSI];
                    lasts[n] = g_scsi_last_ms[t];
                    snprintf(labels[n], sizeof(labels[n]), "SCSI %d", t);
                    n++;
                }
                continue;
            }
        }
        pals[n]  = &palette[i];
        lasts[n] = g_last_ms[i];
        if (i == LED_CPU)
            snprintf(labels[n], sizeof(labels[n]), "68K %uM", g_cpu_mhz);
        else
            snprintf(labels[n], sizeof(labels[n]), "%s", led_label((LedId)i));
        n++;
    }
    if (n == 0) return;

    /* Each lamp is a lamp plus a text label; width = lamp + gap + text. */
    int total_w = 0;
    for (int i = 0; i < n; i++)
        total_w += 14 + 6 + (int)strlen(labels[i]) * 8;
    total_w += (n - 1) * pad;

    int cx = x + (w - total_w) / 2;
    int cy = y + (h - led_h) / 2;

    Uint64 now = SDL_GetTicks();
    for (int i = 0; i < n; i++) {
        const LedPalette *p = pals[i];
        bool active = lasts[i] != 0 && (now - lasts[i]) < LED_GLOW_MS;

        SDL_FRect lamp = { (float)cx, (float)cy, 14.0f, (float)led_h };
        SDL_SetRenderDrawColor(r,
            active ? p->br : p->dr,
            active ? p->bg : p->dg,
            active ? p->bb : p->db, 255);
        SDL_RenderFillRect(r, &lamp);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderRect(r, &lamp);

        SDL_SetRenderDrawColor(r, 205, 205, 205, 255);
        SDL_RenderDebugText(r, (float)(cx + 20), (float)(cy + 1), labels[i]);

        cx += 14 + 6 + (int)strlen(labels[i]) * 8 + pad;
    }
}