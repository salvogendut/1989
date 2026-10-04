/* SDL3 host dialogs, independent of overlay drafts and emulator settings. */
#include "main.h"
#include "host_dialog.h"
#include "sdlscreen.h"
#include "screen.h"
#include <SDL3/SDL.h>

int HostDialog_Choose(const char *title, const char *message,
                      const char *const *choices, int count) {
    if (count < 1 || count > 4 || bQuitProgram) return -1;
    SDL_MessageBoxButtonData buttons[4];
    for (int i = 0; i < count; i++)
        buttons[i] = (SDL_MessageBoxButtonData){i ? 0 :
            SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT | SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, i, choices[i]};
    SDL_MessageBoxData box = {SDL_MESSAGEBOX_WARNING, sdlWindow, title, message, count, buttons, NULL};
    int selected = -1;
    if (!SDL_ShowMessageBox(&box, &selected)) {
        fprintf(stderr, "%s: %s\nCannot show dialog: %s\n", title, message, SDL_GetError());
        selected = -1;
    }
    return selected >= 0 && selected < count ? selected : -1;
}

/* One request at a time. Static storage survives a callback arriving after
 * the caller has quit; ready is published only after copying the result. */
static SDL_SpinLock picker_lock;
static struct {
    bool busy, ready, failed;
    char path[FILENAME_MAX], error[256];
} picker;

static void SDLCALL picked(void *unused, const char *const *files, int filter) {
    (void)unused; (void)filter;
    SDL_LockSpinlock(&picker_lock);
    picker.path[0] = picker.error[0] = 0;
    picker.failed = !files;
    if (!files) snprintf(picker.error, sizeof(picker.error), "%s", SDL_GetError());
    else if (files[0]) {
        if (strlen(files[0]) >= sizeof(picker.path)) {
            picker.failed = true;
            snprintf(picker.error, sizeof(picker.error), "Selected path is too long.");
        } else snprintf(picker.path, sizeof(picker.path), "%s", files[0]);
    }
    picker.ready = true;
    SDL_UnlockSpinlock(&picker_lock);
}

HostPickResult HostDialog_Pick(char *path, size_t size, bool folder) {
#ifdef __EMSCRIPTEN__
    /* Browser file selection is asynchronous and belongs to the HTML shell. */
    (void)path; (void)size; (void)folder;
    return HOST_PICK_ERROR;
#else
    if (bQuitProgram) return HOST_PICK_QUIT;
    SDL_LockSpinlock(&picker_lock);
    if (picker.busy && !picker.ready) {
        SDL_UnlockSpinlock(&picker_lock);
        return HOST_PICK_ERROR;
    }
    memset(&picker, 0, sizeof(picker));
    picker.busy = true;
    SDL_UnlockSpinlock(&picker_lock);
    if (folder) SDL_ShowOpenFolderDialog(picked, NULL, sdlWindow, NULL, false);
    else SDL_ShowOpenFileDialog(picked, NULL, sdlWindow, NULL, 0, NULL, false);
    while (!bQuitProgram) {
        SDL_LockSpinlock(&picker_lock);
        if (picker.ready) {
            HostPickResult result = picker.failed ? HOST_PICK_ERROR :
                picker.path[0] ? HOST_PICK_SELECTED : HOST_PICK_CANCEL;
            if (result == HOST_PICK_SELECTED) {
                if (strlen(picker.path) >= size) result = HOST_PICK_ERROR;
                else snprintf(path, size, "%s", picker.path);
            }
            if (picker.failed) fprintf(stderr, "1989 file picker: %s\n", picker.error);
            picker.busy = false;
            SDL_UnlockSpinlock(&picker_lock);
            return result;
        }
        SDL_UnlockSpinlock(&picker_lock);
        SDL_Event event;
        if (SDL_WaitEventTimeout(&event, 20)) {
            if (event.type == SDL_EVENT_QUIT ||
                (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && sdlWindow &&
                 event.window.windowID == SDL_GetWindowID(sdlWindow))) Main_RequestQuit(false);
            /* Do not recursively enter the options/event controller here. */
            if (event.type == SDL_EVENT_WINDOW_EXPOSED) Screen_RequestRepaint();
        }
#ifndef ENABLE_RENDERING_THREAD
        Screen_Repaint();
#endif
    }
    return HOST_PICK_QUIT;
#endif
}
