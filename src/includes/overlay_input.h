#ifndef OVERLAY_INPUT89_H
#define OVERLAY_INPUT89_H
#include "configuration.h"
#include "overlay_view.h"
#include "overlay_text.h"
typedef struct {
    bool visible, capturing;
    int row, action;
    OverlayText editor;
    char message[128];
} OverlayInput;
void OverlayInput_Open(OverlayInput *panel);
void OverlayInput_Close(OverlayInput *panel);
void OverlayInput_Event(OverlayInput *panel, const SDL_Event *event, CNF_PARAMS *draft);
void OverlayInput_AddRows(const OverlayInput *panel, OverlayView *view, const CNF_PARAMS *draft);
void OverlayInput_DrawEditor(const OverlayInput *panel, SDL_Renderer *renderer);
#endif
