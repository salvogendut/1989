/* Detail pages within Extensions/Advanced. Edits only the settings draft. */
#ifndef OVERLAY_DEVICES89_H
#define OVERLAY_DEVICES89_H
#include "configuration.h"
#include "overlay_view.h"
#include "ui_config.h"

typedef enum { OV_DEVICES_NONE, OV_DEVICES_NETWORK, OV_DEVICES_DIMENSION } OvDevicePage;
typedef struct {
    OvDevicePage page;
    int row, board;
    bool editing, replace_text, rom_mac_known;
    unsigned char rom_mac[6];
    char text[64], message[128];
} OverlayDevices;

void OverlayDevices_Open(OverlayDevices *panel, OvDevicePage page, const CNF_PARAMS *draft);
void OverlayDevices_Close(OverlayDevices *panel);
OvDialogKind OverlayDevices_Event(OverlayDevices *panel, const SDL_Event *event, CNF_PARAMS *draft);
void OverlayDevices_AddRows(const OverlayDevices *panel, OverlayView *view, const CNF_PARAMS *draft);
void OverlayDevices_DrawEditor(const OverlayDevices *panel, SDL_Renderer *renderer);
/* Check edited settings before Save, without normalizing or mutating them. */
const char *OverlayDevices_Validate(const CNF_PARAMS *original, const CNF_PARAMS *draft);
#endif
