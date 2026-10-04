/* Draft-only controls shared by the existing overlay tabs. */
#ifndef OVERLAY_CONTROLS89_H
#define OVERLAY_CONTROLS89_H
#include "overlay.h"
#include "overlay_view.h"
#include "configuration.h"
#include "ui_config.h"

/* Presets match the legacy clock selector; 40 MHz is offered on Turbo. */
void OverlayControls_CycleCpuClock(CNF_PARAMS *draft);

/* General/Media/Advanced controls follow the existing rows in their tabs.
 * Extensions uses this whole list.
 * Row numbers count selectable controls only; headings are presentation. */
int OverlayControls_Count(OvSection section);
void OverlayControls_AddRows(OverlayView *view, OvSection section,
                            int selected, const CNF_PARAMS *draft);
/* Changes only the draft. Returns a picker request for path controls; the
 * controller owns dispatch and the asynchronous result. */
OvDialogKind OverlayControls_Activate(OvSection section, int row, CNF_PARAMS *draft);
#endif
