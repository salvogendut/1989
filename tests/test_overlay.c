#include "ui_test_support.h"
#include "overlay_media.h"
#include "overlay_view.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static void key(SDL_Scancode sc) {
    SDL_Event ev = {0};
    ev.type = SDL_EVENT_KEY_DOWN; ev.key.scancode = sc;
    assert(overlay_handle_event(&ev));
}
static void down(int count) { for (int i = 0; i < count; i++) key(SDL_SCANCODE_DOWN); }

int main(void) {
    ConfigureParams.System.nCpuFreq = 25;
    UI89Config_.bTinker = true;
    UI89Config_.nGifFps = 25;
    overlay_init();
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); /* Media, boot device. */
    key(SDL_SCANCODE_RETURN);
    assert(ConfigureParams.Boot.nBootDevice == BOOT_ROM);
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_RIGHT); /* Discard. */
    key(SDL_SCANCODE_RETURN);
    assert(!overlay_is_visible() && !saved && !restarts);
    assert(ConfigureParams.Boot.nBootDevice == BOOT_ROM);

    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Save boot change. */
    assert(saved == 1 && ConfigureParams.Boot.nBootDevice == BOOT_SCSI && !restarts);

    key(SDL_SCANCODE_F9);
    down(2); key(SDL_SCANCODE_RETURN); /* Clock is staged, requires restart. */
    assert(ConfigureParams.System.nCpuFreq == 25);
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_ESCAPE); /* Return to editing with the draft intact. */
    assert(overlay_is_visible() && !overlay_confirm_visible());
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Default is Discard. */
    assert(ConfigureParams.System.nCpuFreq == 25 && restarts == 0 && saved == 1);

    key(SDL_SCANCODE_F9);
    down(2); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN);
    assert(ConfigureParams.System.nCpuFreq == 33 && restarts == 1 && saved == 2);
    overlay_confirm_reset(); key(SDL_SCANCODE_RETURN); /* Default Cancel. */
    assert(restarts == 1);
    overlay_confirm_reset(); key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN);
    assert(restarts == 2);

    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RIGHT); down(2); /* Ethernet. */
    key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_RETURN); /* A no-op edit. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible() && saved == 2);

    key(SDL_SCANCODE_F9);
    down(5); key(SDL_SCANCODE_RETURN); /* MMU is always on: no false edit. */
    down(1); key(SDL_SCANCODE_RETURN); /* ADB requires a Turbo machine. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible() && saved == 2);

    /* Software rendering exercises all tabs and modals without a real display. */
    SDL_Surface *surface = SDL_CreateSurface(1120, 870, SDL_PIXELFORMAT_RGBA32);
    assert(surface);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    key(SDL_SCANCODE_F9);
    for (int i = 0; i < 4; i++) {
        overlay_render(renderer);
        SDL_RenderPresent(renderer);
        const char *preview = getenv("OVERLAY_PREVIEW_PREFIX");
        if (preview) {
            char filename[FILENAME_MAX];
            snprintf(filename, sizeof(filename), "%s%d.bmp", preview, i);
            assert(SDL_SaveBMP(surface, filename));
        }
        key(SDL_SCANCODE_RIGHT);
    }
    overlay_confirm_reset(); overlay_render(renderer); key(SDL_SCANCODE_ESCAPE);
    key(SDL_SCANCODE_F9);
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);

    /* A late native picker result cannot be delivered to a newer session. */
    assert(OverlayMedia_Request(OV_DIALOG_SCSI0, &UI89Config_, 0));
    assert(!OverlayMedia_Request(OV_DIALOG_SCSI1, &UI89Config_, 0));
    OverlayMedia_Cancel();
    const char *files[] = { "late.sd", NULL };
    picker_callback(picker_userdata, files, 0);
    OvDialogKind kind; char path[FILENAME_MAX]; long long size;
    assert(OverlayMedia_Poll(&kind, path, &size) && kind == OV_DIALOG_NONE);
    assert(!OverlayMedia_Busy());

    char dir[] = "/tmp/1989-media-XXXXXX";
    assert(mkdtemp(dir));
    char name[FILENAME_MAX], created[FILENAME_MAX];
    snprintf(name, sizeof(name), "%s/blank", dir);
    assert(OverlayMedia_Create(name, 2147483648LL, created));
    struct stat st;
    assert(stat(created, &st) == 0 && st.st_size == 2147483648LL);
    assert(!OverlayMedia_Create(name, 737280, path)); /* Never overwrite. */
    assert(stat(created, &st) == 0 && st.st_size == 2147483648LL);
    assert(!OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, created));
    remove(created);
    assert(OverlayMedia_Create(name, 737280, created));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, created));
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected);
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, NULL));
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected); /* Disk != drive. */
    remove(created); rmdir(dir);
    puts("test-overlay: OK");
    return 0;
}
