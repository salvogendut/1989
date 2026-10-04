/* Use real framebuffer conversion and image writers with tiny synthetic displays. */
#include "main.h"
#include "configuration.h"
#include "grab.h"
#include "screen.h"
#include "dimension.hpp"
#include "file.h"
#include "log.h"
#include "statusbar.h"
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#if HAVE_LIBPNG
#include <png.h>
#endif

CNF_PARAMS ConfigureParams;
const int NeXT_SCRN_W = 8, NeXT_SCRN_H = 2;
int screen_w = 8, screen_h = 2;
static uint8_t video[320], nd_video[320];
uint8_t *NEXTVideo = video;
uint32_t *nd_vram_for_slot(int slot) { return slot == 2 ? (uint32_t*)nd_video : NULL; }
bool Log_DebugEnabled(void) { return false; }
void Log_PrintfInt(LOGTYPE level, const char *fmt, ...) { (void)level; (void)fmt; }
void Statusbar_AddMessage(const char *msg, uint32_t ms) { (void)msg; (void)ms; }
bool File_DirExists(const char *path) { struct stat s; return stat(path, &s) == 0 && S_ISDIR(s.st_mode); }
bool File_Exists(const char *path) { return access(path, F_OK) == 0; }
char *File_MakePath(const char *dir, const char *name, const char *ext) {
    size_t len = strlen(dir) + strlen(name) + strlen(ext) + 3;
    char *path = malloc(len);
    assert(path);
    snprintf(path, len, "%s/%s.%s", dir, name, ext);
    return path;
}
FILE *File_Open(const char *path, const char *mode) { return fopen(path, mode); }
FILE *File_Close(FILE *fp) { fclose(fp); return NULL; }
bool File_Write(uint8_t *data, uint32_t size, off_t offset, FILE *fp) {
    return fseeko(fp, offset, SEEK_SET) == 0 && fwrite(data, 1, size, fp) == size;
}

static void pixel(const uint8_t *p, int r, int g, int b, int a) {
    assert(p[0] == r && p[1] == g && p[2] == b && p[3] == a);
}
static unsigned be16(const uint8_t *p) { return (p[0] << 8) | p[1]; }
static unsigned be32(const uint8_t *p) { return (be16(p) << 16) | be16(p + 2); }
static unsigned tiff_tag(const uint8_t *data, size_t len, unsigned tag) {
    unsigned ifd = be32(data + 4);
    assert(ifd + 2 <= len);
    for (unsigned i = 0; i < be16(data + ifd); i++) {
        unsigned pos = ifd + 2 + 12 * i;
        assert(pos + 12 <= len);
        if (be16(data + pos) == tag)
            return be16(data + pos + 2) == 3 ? be16(data + pos + 8) : be32(data + pos + 8);
    }
    assert(!"missing TIFF tag");
    return 0;
}

int main(void) {
    uint8_t rgba[8 * 2 * 4 * 3];
    for (int turbo = 0; turbo <= 1; turbo++) {
        ConfigureParams.System.bTurbo = turbo;
        ConfigureParams.System.bColor = false;
        memset(video, 0xff, sizeof(video));
        video[0] = 0x1b; /* white, light gray, dark gray, black */
        video[turbo ? 2 : 10] = 0x00;
        assert(Grab_FillBuffer(rgba));
        for (int x = 0; x < 4; x++) pixel(rgba + x * 4, 255 - x * 85, 255 - x * 85, 255 - x * 85, 255);
        pixel(rgba + 8 * 4, 255, 255, 255, 255); /* source row padding */
        ConfigureParams.System.bColor = true;
        memset(video, 0, sizeof(video));
        video[0] = 0x12; video[1] = 0x30;
        video[turbo ? 16 : 80] = 0x45; video[turbo ? 17 : 81] = 0x60;
        assert(Grab_FillBuffer(rgba));
        pixel(rgba, 0x11, 0x22, 0x33, 255);
        pixel(rgba + 8 * 4, 0x44, 0x55, 0x66, 255);
    }
    nd_video[0] = 0x33; nd_video[1] = 0x22; nd_video[2] = 0x11;
    nd_video[160] = 0x66; nd_video[161] = 0x55; nd_video[162] = 0x44;
    ConfigureParams.Screen.nSingleModeSlot = 2;
    assert(Grab_FillBuffer(rgba));
    pixel(rgba, 0x11, 0x22, 0x33, 255);
    pixel(rgba + 8 * 4, 0x44, 0x55, 0x66, 255);
    ConfigureParams.Screen.nMode = SCREEN_GROUP;
    for (int i = 0; i < NUM_MONITORS; i++) ConfigureParams.Screen.nGroupModePos[i] = -1;
    ConfigureParams.Screen.nGroupModePos[0] = 1;
    ConfigureParams.Screen.nGroupModePos[1] = 0;
    screen_w = 24;
    memset(rgba, 0, sizeof(rgba));
    assert(Grab_FillBuffer(rgba));
    pixel(rgba, 0x11, 0x22, 0x33, 255);
    pixel(rgba + 8 * 4, 0x11, 0x22, 0x33, 255);
    pixel(rgba + 16 * 4, 0, 0, 0, 0);
    screen_w = 8;
    assert(!Grab_FillBuffer(rgba)); /* refuse a group outside the destination */
    ConfigureParams.Screen.nMode = SCREEN_SINGLE;
    ConfigureParams.Screen.nSingleModeSlot = 4;
    assert(!Grab_FillBuffer(rgba)); /* unavailable NeXTdimension */
    ConfigureParams.Screen.nSingleModeSlot = 0;
    assert(!Grab_FillBuffer(NULL));

    char dir[] = "/tmp/1989-grab-test.XXXXXX";
    assert(mkdtemp(dir));
    snprintf(ConfigureParams.Printer.szPrintToFileName, FILENAME_MAX, "%s", dir);
    ConfigureParams.Printer.nFileFormat = FORMAT_TIFF;
    uint8_t page[] = {0xa5, 0x5a};
    Grab_Print(page, 8, 2, 400);
    char *path = File_MakePath(dir, "next_print_000000", "tiff");
    uint8_t data[512];
    FILE *fp = fopen(path, "rb");
    assert(fp);
    size_t len = fread(data, 1, sizeof(data), fp);
    fclose(fp);
    assert(len > 8 && memcmp(data, "MM\0*", 4) == 0);
    assert(tiff_tag(data, len, 256) == 8 && tiff_tag(data, len, 257) == 2);
    assert(tiff_tag(data, len, 258) == 1 && tiff_tag(data, len, 262) == 0);
    unsigned strip = tiff_tag(data, len, 273);
    assert(strip + sizeof(page) <= len && memcmp(data + strip, page, sizeof(page)) == 0);
    unlink(path); free(path);

#if HAVE_LIBPNG
    ConfigureParams.Printer.nFileFormat = FORMAT_PNG;
    Grab_Screen();
    path = File_MakePath(dir, "next_screen_000", "png");
    png_image image = {0};
    image.version = PNG_IMAGE_VERSION;
    assert(png_image_begin_read_from_file(&image, path));
    image.format = PNG_FORMAT_RGBA;
    assert(image.width == 8 && image.height == 2);
    assert(png_image_finish_read(&image, NULL, rgba, 0, NULL));
    pixel(rgba, 0x11, 0x22, 0x33, 255);
    pixel(rgba + 8 * 4, 0x44, 0x55, 0x66, 255);
    png_image_free(&image);
    unlink(path); free(path);
#endif
    assert(rmdir(dir) == 0);
    puts("test-grab: OK");
    return 0;
}
