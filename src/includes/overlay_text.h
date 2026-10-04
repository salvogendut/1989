#ifndef OVERLAY_TEXT89_H
#define OVERLAY_TEXT89_H
#include <SDL3/SDL.h>
typedef struct {
    bool active, replace;
    char value[64], original[64], error[128];
} OverlayText;
typedef enum { OV_TEXT_PENDING, OV_TEXT_ACCEPT, OV_TEXT_CANCEL } OverlayTextResult;
void OverlayText_Begin(OverlayText *text, const char *value);
void OverlayText_Close(OverlayText *text);
OverlayTextResult OverlayText_Event(OverlayText *text, const SDL_Event *event);
void OverlayText_Draw(const OverlayText *text, SDL_Renderer *renderer, const char *title);
#endif
