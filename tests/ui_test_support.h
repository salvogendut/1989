#ifndef UI_TEST_SUPPORT_H
#define UI_TEST_SUPPORT_H
#include "main.h"
#include "settings.h"
#include "overlay.h"
#include "ui_config.h"
#include <SDL3/SDL.h>
extern int restarts, network, tablet, sound, printer, screen, saved, ui_saved;
extern int scsi_in[ESP_MAX_DEVS], scsi_out[ESP_MAX_DEVS];
extern int floppy_in[FLP_MAX_DRIVES], floppy_out[FLP_MAX_DRIVES];
extern int mo_in[MO_MAX_DRIVES], mo_out[MO_MAX_DRIVES];
extern SDL_DialogFileCallback picker_callback;
extern void *picker_userdata;
extern int folder_requests, keymap_inits;
extern char default_rom_dir[FILENAME_MAX];
#endif
