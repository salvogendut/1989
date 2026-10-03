#include "capture.h"
#include "gifcap.h"
#include "ffmpeg_gif.h"
#include "grab.h"
#include "screen.h"
#include "configuration.h"
#include "ui_config.h"
#include "main.h"
#include "notify.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CAP_FRAME_W  1120
#define CAP_FRAME_H  832

static GifCap   *g_gif;
static char      g_gif_path[512];
static Uint64    g_last_frame_ms;
static int       g_gif_fps = 25;

static void capture_timestamp(char *out, size_t size, const char *ext) {
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    if (lt)
        snprintf(out, size, "1989-%04d%02d%02d-%02d%02d%02d.%s",
                 lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
                 lt->tm_hour, lt->tm_min, lt->tm_sec, ext);
    else
        snprintf(out, size, "1989-capture.%s", ext);
}

bool Capture_Screenshot(void) {
    int w = screen_w, h = screen_h;
    if (w <= 0 || h <= 0) {
        notify_post("SCREENSHOT FAILED - NO FRAMEBUFFER");
        return false;
    }
    uint8_t *rgba = malloc((size_t)w * h * 4);
    if (!rgba) return false;
    if (!Grab_FillBuffer(rgba)) {
        free(rgba);
        notify_post("SCREENSHOT FAILED - FRAMEBUFFER BUSY");
        return false;
    }

    char path[512];
    capture_timestamp(path, sizeof(path), "ppm");
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        free(rgba);
        notify_post("SCREENSHOT FAILED - CANNOT WRITE %s", path);
        return false;
    }
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    /* PPM wants 0xRRGGBB per pixel; Grab_FillBuffer gives 0xRRGGBBAA. */
    const uint8_t *p = rgba;
    for (int i = 0; i < w * h; i++, p += 4) {
        fputc(p[0], fp);
        fputc(p[1], fp);
        fputc(p[2], fp);
    }
    fclose(fp);
    free(rgba);
    notify_post("SCREENSHOT SAVED: %s", path);
    return true;
}

bool Capture_GifStart(int out_w, int fps) {
    if (g_gif) Capture_GifStop();

    int w = out_w > 0 ? out_w : 480;
    if (w > CAP_FRAME_W) w = CAP_FRAME_W;
    int h = w * CAP_FRAME_H / CAP_FRAME_W;
    int delay_cs = fps >= 25 ? 4 : fps >= 20 ? 5 : 10;
    g_gif_fps = fps >= 25 ? 25 : fps >= 20 ? 20 : 10;

    capture_timestamp(g_gif_path, sizeof(g_gif_path), "gif");
    g_gif = gifcap_open(g_gif_path, CAP_FRAME_W, CAP_FRAME_H, w, h, delay_cs);
    if (!g_gif) {
        g_gif_path[0] = '\0';
        notify_post("GIF CAPTURE FAILED");
        return false;
    }
    g_last_frame_ms = 0;
    notify_post("GIF CAPTURE RECORDING: %s", g_gif_path);
    return true;
}

bool Capture_GifStop(void) {
    if (!g_gif) return false;
    int frames = gifcap_frame_count(g_gif);
    gifcap_close(g_gif);
    g_gif = NULL;
    notify_post("GIF CAPTURE SAVED: %s (%d frames)", g_gif_path, frames);
    /* Optional FFmpeg optimization pass (Advanced > GIF encoder). */
    if (UI89Config_.bGifFfmpeg && FFMPEG_GIF_SUPPORTED)
        ffmpeg_gif_optimize(g_gif_path);
    return true;
}

bool Capture_GifActive(void) {
    return g_gif != NULL;
}

int Capture_GifFrames(void) {
    return g_gif ? gifcap_frame_count(g_gif) : 0;
}

void Capture_Tick(void) {
    if (!g_gif) return;

    Uint64 now = SDL_GetTicks();
    if (g_last_frame_ms != 0 && (now - g_last_frame_ms) <
        (Uint64)(1000 / g_gif_fps))
        return;
    g_last_frame_ms = now;

    int w = screen_w, h = screen_h;
    uint8_t *rgba = malloc((size_t)w * h * 4);
    if (!rgba) return;
    if (!Grab_FillBuffer(rgba)) {
        free(rgba);
        return;
    }
    /* gifcap_frame wants 0x00RRGGBB; convert from 0xRRGGBBAA. */
    uint32_t *px = malloc((size_t)CAP_FRAME_W * CAP_FRAME_H * 4);
    if (!px) {
        free(rgba);
        return;
    }
    const uint8_t *p = rgba;
    uint32_t *out = px;
    for (int i = 0; i < w * h; i++, p += 4)
        *out++ = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | p[2];
    /* Only the first CAP_FRAME_W x CAP_FRAME_H region is meaningful when
     * group mode spreads monitors; clamp to the real logical size. */
    bool ok = gifcap_frame(g_gif, px);
    free(px);
    free(rgba);
    if (!ok) {
        fprintf(stderr, "1989: GIF frame encoding failed\n");
        Capture_GifStop();
    }
}
