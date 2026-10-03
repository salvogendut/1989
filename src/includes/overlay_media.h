#ifndef OVERLAY_MEDIA89_H
#define OVERLAY_MEDIA89_H
#include "configuration.h"
#include "ui_config.h"

/* The callback owns no pointer into an edit session. Closing the panel
 * invalidates its result; only the UI thread applies a completed selection. */
bool OverlayMedia_Request(OvDialogKind kind, const UI89Config *ui, long long size);
bool OverlayMedia_Busy(void);
void OverlayMedia_Cancel(void);
bool OverlayMedia_Poll(OvDialogKind *kind, char *path, long long *size);
bool OverlayMedia_Set(CNF_PARAMS *draft, OvDialogKind kind, const char *path);
void OverlayMedia_Remember(UI89Config *ui, OvDialogKind kind, const char *path);
/* Exclusive creation: never truncate an existing (possibly mounted) disk. */
bool OverlayMedia_Create(const char *path, long long size, char *result);
#endif
