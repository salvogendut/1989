/* Resource recovery is transactional and never opens a guest disk for writing. */
#ifndef RECOVERY89_H
#define RECOVERY89_H
#include "configuration.h"
/* Caller keeps emulation paused. False leaves params unchanged. */
bool Recovery_CheckFiles(CNF_PARAMS *params, bool startup);
/* Called on the SDL window thread after a core halt. */
void Recovery_Halt(void);
#endif
