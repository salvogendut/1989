/* Stateless presentation: no configuration, device calls or file operations. */
#ifndef OVERLAY_VIEW89_H
#define OVERLAY_VIEW89_H
#include <SDL3/SDL.h>

#define OVERLAY_MAX_ROWS 32
typedef struct {
    char label[64];
    char value[256];
    bool selected;
    bool heading;
} OverlayRow;
typedef struct {
    const char *tabs[4];
    int tab_count, active_tab;
    OverlayRow rows[OVERLAY_MAX_ROWS];
    int row_count;
    const char *footer;
    const char *hint;
} OverlayView;

void OverlayView_Add(OverlayView *view, const char *label, const char *value, bool selected);
void OverlayView_Heading(OverlayView *view, const char *title);
void OverlayView_Draw(SDL_Renderer *r, const OverlayView *view);
void OverlayView_Dialog(SDL_Renderer *r, const char *const *lines, int count,
                        const char *accept, const char *cancel, bool selected);
void OverlayView_Choices(SDL_Renderer *r, const char *title,
                         const char *const *choices, int count, int selected);
#endif
