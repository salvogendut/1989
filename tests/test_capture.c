/* test_capture.c — exercise the 1989 screenshot and GIF-capture helpers.
 * The framebuffer is faked; the PPM/GIF encoding is real. */
#include "capture.h"
#include "gifcap.h"
#include "ffmpeg_gif.h"
#include "grab.h"
#include "screen.h"
#include "notify.h"
#include "overlay.h"

/* capture.c reads the GIF-encoder choice from the UI config; provide it
 * without linking the whole overlay. */
UI89Config UI89Config_;

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int screen_w = 1120;
int screen_h = 832;

bool Grab_FillBuffer(uint8_t *buf) {
    if (!buf) return false;
    for (int i = 0; i < screen_w * screen_h * 4; i++)
        buf[i] = (uint8_t)((i / 4 + i / 8) & 0xFF);
    return true;
}

int main(void) {
    int failures = 0;
    notify_init();
    notify_set_mode(NOTIFY_MODE_CONSOLE);

    /* GIF encode: 8 frames into a real file, then inspect it. */
    GifCap *g = gifcap_open("test.gif", 32, 24, 32, 24, 4);
    if (!g) { fprintf(stderr, "FAIL: gifcap_open\n"); return 1; }
    uint32_t *px = malloc(32 * 24 * sizeof(uint32_t));
    if (!px) return 1;
    for (int f = 0; f < 8; f++) {
        for (int i = 0; i < 32 * 24; i++) px[i] = (uint32_t)(f * 0x010101 + i);
        if (!gifcap_frame(g, px)) { fprintf(stderr, "FAIL: gifcap_frame\n"); return 1; }
    }
    if (gifcap_frame_count(g) != 8) { fprintf(stderr, "FAIL: frame count\n"); failures++; }
    gifcap_close(g);
    FILE *fp = fopen("test.gif", "rb");
    if (!fp) { fprintf(stderr, "FAIL: gif file not written\n"); failures++; }
    else {
        long len = 0;
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fclose(fp);
        if (len < 64) { fprintf(stderr, "FAIL: gif too small (%ld)\n", len); failures++; }
        unsigned char head[6] = {0};
        fp = fopen("test.gif", "rb");
        if (fp) { if (fread(head, 1, 6, fp) != 6) head[0] = 0; fclose(fp); }
        if (memcmp(head, "GIF89a", 6) != 0) {
            fprintf(stderr, "FAIL: gif magic missing\n"); failures++;
        }
    }

    /* Capture_Screenshot produces a 1989-<timestamp>.ppm in the CWD via the
     * real PPM writer (our stub fills the framebuffer). */
    if (!Capture_Screenshot()) { fprintf(stderr, "FAIL: Capture_Screenshot\n"); failures++; }

    /* High-level GIF capture through capture.c. */
    if (!Capture_GifStart(320, 25)) { fprintf(stderr, "FAIL: GifStart\n"); failures++; }
    Capture_Tick();
    for (int i = 0; i < 6; i++) {
        SDL_Delay(30);
        Capture_Tick();
    }
    if (!Capture_GifActive()) { fprintf(stderr, "FAIL: GifActive\n"); failures++; }
    if (!Capture_GifStop()) { fprintf(stderr, "FAIL: GifStop\n"); failures++; }

    /* Optional FFmpeg optimization pass (no-op when not built with ffmpeg). */
    if (FFMPEG_GIF_SUPPORTED) {
        if (!ffmpeg_gif_optimize("test.gif")) {
            fprintf(stderr, "FAIL: ffmpeg_gif_optimize\n");
            failures++;
        }
    }

    free(px);
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("test-capture: OK\n");
    return 0;
}