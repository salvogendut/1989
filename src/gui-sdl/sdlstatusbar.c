/*
  Previous - sdlstatusbar.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  Compatibility hooks for the Previous core. 1989 uses one activity strip
  and the shared notifications; there is no separate statusbar surface.
*/
#include "main.h"
#include "statusbar.h"
#include "leds.h"
#include "notify.h"

void Statusbar_BlinkLed(drive_index_t drive)
{
    static const LedId device[] = {LED_NET, LED_MO, LED_SCSI, LED_FLOPPY};
    if ((unsigned)drive < NUM_DEVICE_LEDS) leds_ping(device[drive]);
}

void Statusbar_SetSystemLed(bool state) { if (state) leds_ping(LED_CPU); }
void Statusbar_SetDspLed(bool state) { if (state) leds_ping(LED_DSP); }
void Statusbar_SetNdLed(int state) { if (state) leds_ping(LED_ND); }

void Statusbar_AddMessage(const char *msg, uint32_t msecs)
{
    /* All desktop messages use the shared lifetime and notification mode. */
    (void)msecs;
    if (msg && *msg) notify_post("%s", msg);
}

void Statusbar_UpdateInfo(void)
{
    /* The bottom strip reads the active model/clock/RAM when rendering. */
}
