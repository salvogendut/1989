/*
  Previous - change.c

  This file is distributed under the GNU General Public License, version 2
  or at your option any later version. Read the file gpl.txt for details.

  This code handles run-time configuration changes. We keep all our
  configuration details in a structure called 'ConfigureParams'.  Before
  doing he changes, a backup copy is done of this structure. When
  the changes are done, these are compared to see whether emulator
  needs to be rebooted
*/
const char Change_fileid[] = "Previous change.c";

#include "main.h"
#include "configuration.h"
#include "change.h"
#include "settings.h"
#include "printer.h"
#include "recovery.h"
#include "ioMem.h"
#include "m68000.h"
#include "reset.h"
#include "screen.h"
#include "statusbar.h"
#include "overlay.h"
#include "video.h"
#include "hatari-glue.h"
#include "scsi.h"
#include "mo.h"
#include "floppy.h"
#include "ethernet.h"
#include "snd.h"
#include "tablet.h"
#include "keymap.h"

#define DEBUG 0
#if DEBUG
#define Dprintf(...) printf(__VA_ARGS__)
#else
#define Dprintf(...)
#endif

/*-----------------------------------------------------------------------*/
/**
 * Check if user needs to be warned that changes will take place after reset.
 * Return true if wants to reset.
 */
bool Change_DoNeedReset(CNF_PARAMS *current, CNF_PARAMS *changed)
{
    return Settings_NeedRestart(current, changed);
}


/*-----------------------------------------------------------------------*/
/**
 * Copy details back to configuration and perform reset.
 */
bool Change_CopyChangedParamsToConfiguration(CNF_PARAMS *current, CNF_PARAMS *changed, bool bForceReset)
{
	bool NeedReset;
	bool bReInitKeymap = false;
	bool bReInitTablet = false;
	bool bReInitEnetEmu = false;
	bool bReInitSoundEmu = false;
	bool bScreenModeChange = false;
	bool bReInitPrinter = false;
	bool bTitlebarChange = current->Screen.bShowTitlebar != changed->Screen.bShowTitlebar;

	Dprintf("Changes for:\n");
	/* Do we need to warn user that changes will only take effect after reset? */
	if (bForceReset)
		NeedReset = bForceReset;
	else
		NeedReset = Change_DoNeedReset(current, changed);

    /* Resolve unavailable resources before touching live settings/subsystems.
     * Cancelling native recovery leaves the running machine untouched. */
    CNF_PARAMS recovered;
    if (NeedReset) {
        recovered = *changed;
        if (!Recovery_CheckFiles(&recovered, false)) return false;
        changed = &recovered;
    }

	/* Settings_Apply owns per-target removable-media operations. */

	if (!NeedReset) {
		int i;

		/* Do we need to change Keymap configuration? */
		if (current->Mouse.bUseRawMotion != changed->Mouse.bUseRawMotion) {
			bReInitKeymap = true;
		}

		/* Do we need to change Tablet configuration? */
		if (current->Tablet.nTabletType != changed->Tablet.nTabletType) {
			bReInitTablet = true;
		}

		/* Do we need to change Ethernet configuration? */
		for (i = 0; i < EN_MAX_SHARES; i++) {
			if (current->Ethernet.bEthernetConnected != changed->Ethernet.bEthernetConnected ||
				current->Ethernet.bTwistedPair != changed->Ethernet.bTwistedPair ||
				strcmp(current->Ethernet.szInterfaceName, changed->Ethernet.szInterfaceName) ||
				strcmp(current->Ethernet.nfs[i].szHostName, changed->Ethernet.nfs[i].szHostName) ||
				strcmp(current->Ethernet.nfs[i].szPathName, changed->Ethernet.nfs[i].szPathName)) {
				bReInitEnetEmu = true;
				break;
			}
		}

		/* Do we need to change Sound configuration? */
		if (current->Sound.bEnableSound != changed->Sound.bEnableSound ||
			current->Sound.bEnableMicrophone != changed->Sound.bEnableMicrophone) {
			bReInitSoundEmu = true;
		}

		bReInitPrinter = current->Printer.bPrinterConnected != changed->Printer.bPrinterConnected;

		/* Do we need to change Screen configuration? */
		if (current->Screen.nMode != changed->Screen.nMode ||
			(changed->Screen.nMode == SCREEN_SINGLE &&
			 current->Screen.nSingleModeSlot != changed->Screen.nSingleModeSlot)) {
			bScreenModeChange = true;
		} else if (current->Screen.nMode == SCREEN_GROUP) {
			for (i = 0; i < NUM_MONITORS; i++) {
				if (current->Screen.nGroupModePos[i] != changed->Screen.nGroupModePos[i]) {
					bScreenModeChange = true;
				}
			}
		}
	}

	/* Copy details to configuration,
	 * so it can be saved out or set on reset
	 */
	if (changed != &ConfigureParams)
	{
		ConfigureParams = *changed;
	}

	/* Copy details to global, if we reset copy them all */
	Configuration_Apply(NeedReset);

	/* Re-init Ethernet? */
	if (bReInitEnetEmu) {
		Dprintf("- Ethernet\n");
		Ethernet_Reset(false);
	}

	/* Re-init Sound? */
	if (bReInitSoundEmu) {
		Dprintf("- Sound\n");
		Sound_Reset();
	}

	/* Re-init Keymap? */
	if (bReInitKeymap) {
		Dprintf("- Keymap\n");
		Keymap_Init();
	}

	/* Re-init Tablet? */
	if (bReInitTablet) {
		Dprintf("- Tablet\n");
		Tablet_Reset();
	}

	if (bReInitPrinter) Printer_Reset();
	if (bTitlebarChange) Screen_TitlebarChanged();

	/* Force things associated with screen change */
	if (bScreenModeChange)
	{
		Dprintf("- Screen\n");
		Screen_Reset();
	}

	/* Do we need to perform reset? */
	if (NeedReset)
	{
		Dprintf("- Reset\n");
        if (Reset_Cold()) {
            Main_RequestQuit(false);
            return false;
        }
	}

	/* Go into/return from full screen if flagged */
	if (!bInFullScreen && ConfigureParams.Screen.bFullScreen)
		Screen_EnterFullScreen();
	else if (bInFullScreen && !ConfigureParams.Screen.bFullScreen)
		Screen_ReturnFromFullScreen();

	/* update statusbar info (CPU, MHz, mem etc) */
	Statusbar_UpdateInfo();
	/* Keep the activity-LED bar in sync with the attached media. */
	overlay_update_leds();
	Dprintf("done.\n");
    return true;
}
