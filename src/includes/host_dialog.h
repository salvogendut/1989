/* Native host decisions and pickers. Call only on the SDL window thread. */
#ifndef HOST_DIALOG89_H
#define HOST_DIALOG89_H
#include <stdbool.h>
#include <stddef.h>

/* First choice is the safe Enter/Esc default. Close/error returns -1. */
int HostDialog_Choose(const char *title, const char *message,
                      const char *const *choices, int count);
typedef enum { HOST_PICK_CANCEL, HOST_PICK_SELECTED, HOST_PICK_ERROR, HOST_PICK_QUIT } HostPickResult;
/* Modal event pump; consumes input without forwarding it to the guest.
 * The callback owns no caller memory, including after window close/quit. */
HostPickResult HostDialog_Pick(char *path, size_t size, bool folder);
#endif
