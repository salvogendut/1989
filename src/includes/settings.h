/* Settings edit sessions and the policy for applying them to a running NeXT. */
#ifndef SETTINGS89_H
#define SETTINGS89_H

#include "configuration.h"

typedef struct {
    CNF_PARAMS original;
    CNF_PARAMS draft;
} SettingsSession;

void Settings_Begin(SettingsSession *session);
/* Merge only edited groups/targets, preserving guest ejects and other changes
 * made while the overlay was open. These functions have no runtime effects. */
void Settings_Merge(const SettingsSession *session, const CNF_PARAMS *live,
                    CNF_PARAMS *result);
bool Settings_NeedRestart(const CNF_PARAMS *current, const CNF_PARAMS *changed);
bool Settings_MediaChanged(const CNF_PARAMS *current, const CNF_PARAMS *changed);
/* Refuses restart-requiring changes unless explicitly confirmed. The caller
 * must pause emulation for the duration of this operation. */
bool Settings_Apply(SettingsSession *session, bool restart_confirmed);

#endif
