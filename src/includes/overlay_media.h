#ifndef OVERLAY_MEDIA89_H
#define OVERLAY_MEDIA89_H
#include "configuration.h"
#include "ui_config.h"

/* Suggested roles never move or retype an existing drive. */
const char *OverlayMedia_ScsiRole(int id);
SCSI_DEVTYPE OverlayMedia_ScsiType(const CNF_PARAMS *draft, int id);
/* Expected NEXTSTEP enumeration at the next boot, not a live guest query. */
int OverlayMedia_ScsiDiskNumber(const CNF_PARAMS *draft, int id);
bool OverlayMedia_Eject(CNF_PARAMS *draft, OvDialogKind kind);
bool OverlayMedia_Disconnect(CNF_PARAMS *draft, OvDialogKind kind);
/* Existing unsupported connections remain visible and can be ejected/removed. */
const char *OverlayMedia_Unavailable(const CNF_PARAMS *draft, OvDialogKind kind);
bool OverlayMedia_Connect(CNF_PARAMS *draft, OvDialogKind kind);
bool OverlayMedia_RestoreRom(CNF_PARAMS *draft, OvDialogKind kind);
const char *OverlayMedia_Validate(const CNF_PARAMS *original, const CNF_PARAMS *draft);

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
