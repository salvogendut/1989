/* Instrumented machine boundary for settings and overlay regression tests. */
#include "ui_test_support.h"
#include "file.h"
#include "leds.h"
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <stdarg.h>

CNF_PARAMS ConfigureParams;
volatile bool bQuitProgram;
volatile bool bInFullScreen;
int restarts, network, tablet, sound, printer, screen;
int scsi_in[ESP_MAX_DEVS], scsi_out[ESP_MAX_DEVS];
int floppy_in[FLP_MAX_DRIVES], floppy_out[FLP_MAX_DRIVES];
int mo_in[MO_MAX_DRIVES], mo_out[MO_MAX_DRIVES];

void Configuration_Apply(bool reset) { (void)reset; }
bool recovery_ok = true;
bool Recovery_CheckFiles(CNF_PARAMS *p, bool startup) { (void)p; (void)startup; return recovery_ok; }
int Reset_Cold(void) { restarts++; return 0; }
void Ethernet_Reset(bool hard) { assert(!hard); network++; }
void Tablet_Reset(void) { tablet++; }
void Sound_Reset(void) { sound++; }
void Printer_Reset(void) { printer++; }
int keymap_inits;
void Keymap_Init(void) { keymap_inits++; }
void Screen_Reset(void) { screen++; }
void Screen_TitlebarChanged(void) {}
void Screen_EnterFullScreen(void) { bInFullScreen = true; }
void Screen_ReturnFromFullScreen(void) { bInFullScreen = false; }
void Statusbar_UpdateInfo(void) {}
void SCSI_Eject(uint8_t i) { scsi_out[i]++; }
void SCSI_Insert(uint8_t i) { scsi_in[i]++; }
void Floppy_Eject(int i) {
    floppy_out[i]++;
    ConfigureParams.Floppy.drive[i].bDiskInserted = false;
    ConfigureParams.Floppy.drive[i].szImageName[0] = 0;
}
int Floppy_Insert(int i) { floppy_in[i]++; assert(ConfigureParams.Floppy.drive[i].bDiskInserted); return 0; }
void MO_Eject(int i) {
    mo_out[i]++;
    ConfigureParams.MO.drive[i].bDiskInserted = false;
    ConfigureParams.MO.drive[i].szImageName[0] = 0;
}
void MO_Insert(int i) { mo_in[i]++; assert(ConfigureParams.MO.drive[i].bDiskInserted); }


volatile bool bEmulationActive = true;
volatile bool bGrabMouse;
UI89Config UI89Config_;
SDL_Window *sdlWindow;
int saved, ui_saved;
SDL_DialogFileCallback picker_callback;
void *picker_userdata;

bool Main_PauseEmulation(bool visualize) { (void)visualize; bool was = bEmulationActive; bEmulationActive = false; return was; }
bool Main_UnPauseEmulation(void) { bool was = bEmulationActive; bEmulationActive = true; return !was; }
void Main_RequestQuit(bool confirm) { (void)confirm; bQuitProgram = true; }
void Screen_SetMouseGrab(bool grab) { bGrabMouse = grab; }
void Screen_RequestRepaint(void) {}
void Configuration_Save(void) { saved++; }
void UI89_Save(void) { ui_saved++; }
void UI89_Apply(void) {}
void paste_stop(void) {}
void notify_post(const char *fmt, ...) { (void)fmt; }
void Configuration_SetSystemDefaultsFor(CNF_PARAMS *p) { p->System.nCpuFreq = 25; p->Memory.nMemoryBankSize[0] = 16; }
void leds_set_enabled(LedId id, bool value) { (void)id; (void)value; }
void leds_set_scsi_present(int target, bool value) { (void)target; (void)value; }
#ifndef TEST_OVERLAY
void overlay_update_leds(void) {}
#endif
bool File_Exists(const char *path) { struct stat st; return stat(path, &st) == 0; }
bool File_DirExists(const char *path) { struct stat st; return stat(path, &st) == 0 && S_ISDIR(st.st_mode); }
off_t File_Length(const char *path) { struct stat st; return stat(path, &st) == 0 ? st.st_size : -1; }
void SDL_ShowOpenFileDialog(SDL_DialogFileCallback cb, void *userdata, SDL_Window *window,
                           const SDL_DialogFileFilter *filters, int count, const char *path, bool multiple) {
    (void)window; (void)filters; (void)count; (void)path; (void)multiple;
    picker_callback = cb; picker_userdata = userdata;
}
void SDL_ShowSaveFileDialog(SDL_DialogFileCallback cb, void *userdata, SDL_Window *window,
                           const SDL_DialogFileFilter *filters, int count, const char *path) {
    SDL_ShowOpenFileDialog(cb, userdata, window, filters, count, path, false);
}

int folder_requests;
void SDL_ShowOpenFolderDialog(SDL_DialogFileCallback cb, void *userdata, SDL_Window *window,
                             const char *path, bool multiple) {
    folder_requests++;
    SDL_ShowOpenFileDialog(cb, userdata, window, NULL, 0, path, multiple);
}

char default_rom_dir[FILENAME_MAX];
void Rom_GetDefaultPath(char *path, int length, const char *name) {
    snprintf(path, length, "%s/%s.BIN", default_rom_dir, name);
}
