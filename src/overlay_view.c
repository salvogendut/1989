#include "overlay_view.h"
#include <stdio.h>
#include <string.h>

#define LINE_H 20
#define VALUE_X 250

/* All dimensions are logical coordinates, including fullscreen letterboxes. */
static bool begin(SDL_Renderer *r, int min_w, int min_h, int *w, int *h) {
    SDL_RendererLogicalPresentation mode;
    int rw, rh;
    if (!SDL_GetRenderLogicalPresentation(r, &rw, &rh, &mode) || rw <= 0 || rh <= 0)
        if (!SDL_GetRenderOutputSize(r, &rw, &rh)) return false;
    float scale = SDL_min(1.25f, SDL_min((float)rw / min_w, (float)rh / min_h));
    if (scale <= 0) return false;
    *w = (int)(rw / scale); *h = (int)(rh / scale);
    SDL_SetRenderScale(r, scale, scale);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_FRect shade = { 0, 0, (float)*w, (float)*h };
    SDL_RenderFillRect(r, &shade);
    return true;
}

static void finish(SDL_Renderer *r) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    SDL_SetRenderScale(r, 1, 1);
}

static void panel(SDL_Renderer *r, SDL_FRect rect) {
    SDL_SetRenderDrawColor(r, 0x19, 0x20, 0x34, 255);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, 0x89, 0xA3, 0xCB, 255);
    SDL_RenderRect(r, &rect);
}

static void button(SDL_Renderer *r, float x, float y, int w, const char *label, bool selected) {
    SDL_FRect rect = { x, y, (float)w, 22 };
    if (selected) {
        SDL_SetRenderDrawColor(r, 0x80, 0x60, 0x20, 255);
        SDL_RenderFillRect(r, &rect);
    }
    SDL_SetRenderDrawColor(r, selected ? 255 : 137, selected ? 255 : 163, selected ? 255 : 203, 255);
    SDL_RenderRect(r, &rect);
    SDL_RenderDebugText(r, x + (w - strlen(label) * 8) * .5f, y + 7, label);
}

void OverlayView_Add(OverlayView *view, const char *label, const char *value, bool selected) {
    if (view->row_count >= OVERLAY_MAX_ROWS) return;
    OverlayRow *row = &view->rows[view->row_count++];
    snprintf(row->label, sizeof(row->label), "%s", label);
    snprintf(row->value, sizeof(row->value), "%s", value ? value : "");
    row->selected = selected;
    row->heading = false;
}

void OverlayView_Heading(OverlayView *view, const char *title) {
    if (view->row_count >= OVERLAY_MAX_ROWS) return;
    OverlayView_Add(view, title, NULL, false);
    view->rows[view->row_count - 1].heading = true;
}

void OverlayView_Draw(SDL_Renderer *r, const OverlayView *view) {
    int w, h, panel_h = 48 + view->row_count * LINE_H + 42 + (view->hint ? LINE_H : 0);
    if (!begin(r, 860, SDL_max(panel_h + 8, 510), &w, &h)) return;
    int panel_w = SDL_min(w - 20, 840);
    panel(r, (SDL_FRect){ 8, 8, (float)panel_w, (float)panel_h });
    SDL_SetRenderDrawColor(r, 0x30, 0x40, 0x60, 255);
    SDL_FRect tabs = { 10, 10, (float)panel_w - 4, 22 };
    SDL_RenderFillRect(r, &tabs);
    float x = 20;
    for (int i = 0; i < view->tab_count; i++) {
        Uint8 color = i == view->active_tab ? 255 : 192;
        SDL_SetRenderDrawColor(r, color, color, color, 255);
        SDL_RenderDebugText(r, x, 14, view->tabs[i]);
        x += strlen(view->tabs[i]) * 8 + 20;
    }
    for (int i = 0; i < view->row_count; i++) {
        const OverlayRow *row = &view->rows[i];
        float y = 48 + i * LINE_H;
        if (row->heading) {
            SDL_SetRenderDrawColor(r, 0x30, 0x40, 0x60, 255);
            SDL_FRect rect = { 10, y, (float)panel_w - 4, LINE_H - 3 };
            SDL_RenderFillRect(r, &rect);
            SDL_SetRenderDrawColor(r, 170, 195, 230, 255);
            SDL_RenderDebugText(r, 20, y + 2, row->label);
            continue;
        }
        if (row->selected) {
            SDL_SetRenderDrawColor(r, 0x80, 0x60, 0x20, 255);
            SDL_FRect rect = { 10, y, (float)panel_w - 4, LINE_H - 4 };
            SDL_RenderFillRect(r, &rect);
        }
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDebugText(r, 20, y, row->label);
        char shown[96];
        snprintf(shown, sizeof(shown), "%s", row->value);
        int max_chars = SDL_min((panel_w - VALUE_X - 18) / 8, (int)sizeof(shown) - 1);
        if ((int)strlen(shown) > max_chars && max_chars > 3)
            memcpy(shown + max_chars - 3, "...", 4);
        SDL_SetRenderDrawColor(r, 255, row->selected ? 255 : 208, row->selected ? 255 : 128, 255);
        SDL_RenderDebugText(r, VALUE_X, y, shown);
    }
    SDL_SetRenderDrawColor(r, 170, 170, 170, 255);
    if (view->hint) SDL_RenderDebugText(r, 20, panel_h - 40, view->hint);
    SDL_RenderDebugText(r, 20, panel_h - 20, view->footer);
    finish(r);
}

void OverlayView_Dialog(SDL_Renderer *r, const char *const *lines, int count,
                        const char *accept, const char *cancel, bool selected) {
    int text_w = 0;
    for (int i = 0; i < count; i++) text_w = SDL_max(text_w, (int)strlen(lines[i]) * 8);
    int pw = text_w + 64, ph = 24 + count * 16 + 20 + 22 + 32;
    int w, h;
    if (!begin(r, pw + 16, ph + 16, &w, &h)) return;
    float x = (w - pw) * .5f, y = (h - ph) * .5f;
    panel(r, (SDL_FRect){ x, y, (float)pw, (float)ph });
    SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
    for (int i = 0; i < count; i++) SDL_RenderDebugText(r, x + 32, y + 24 + i * 16, lines[i]);
    int aw = strlen(accept) * 8 + 20;
    int cw = cancel ? strlen(cancel) * 8 + 20 : 0;
    int total = aw + (cancel ? 32 + cw : 0);
    float bx = x + (pw - total) * .5f, by = y + 24 + count * 16 + 20;
    button(r, bx, by, aw, accept, selected);
    if (cancel) button(r, bx + aw + 32, by, cw, cancel, !selected);
    finish(r);
}

void OverlayView_Choices(SDL_Renderer *r, const char *title,
                         const char *const *choices, int count, int selected) {
    int w, h, pw = 360, ph = 80 + count * 30;
    if (!begin(r, pw + 16, ph + 16, &w, &h)) return;
    float x = (w - pw) * .5f, y = (h - ph) * .5f;
    panel(r, (SDL_FRect){ x, y, (float)pw, (float)ph });
    SDL_SetRenderDrawColor(r, 240, 240, 240, 255);
    SDL_RenderDebugText(r, x + 16, y + 16, title);
    for (int i = 0; i < count; i++) button(r, x + 24, y + 40 + i * 30, pw - 48, choices[i], i == selected);
    SDL_SetRenderDrawColor(r, 137, 163, 203, 255);
    SDL_RenderDebugText(r, x + 16, y + ph - 20, "Enter=create  Esc=cancel");
    finish(r);
}
