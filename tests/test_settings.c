/* Apply the real settings/legacy change code against instrumented devices. */
#include "main.h"
#include "settings.h"
#include "change.h"
#include <assert.h>
#include <string.h>

#include "ui_test_support.h"

int main(void) {
    static SettingsSession session;
    CNF_PARAMS before;
    ConfigureParams.System.nMachineType = NEXT_CUBE030;
    ConfigureParams.SCSI.target[0].nDeviceType = SD_HARDDISK;
    ConfigureParams.SCSI.target[0].bDiskInserted = true;
    strcpy(ConfigureParams.SCSI.target[0].szImageName, "system.sd");
    ConfigureParams.Floppy.drive[0].bDriveConnected = true;
    ConfigureParams.Floppy.drive[1].bDriveConnected = true;
    ConfigureParams.Floppy.drive[1].bDiskInserted = true;
    ConfigureParams.MO.drive[0].bDriveConnected = true;

    Settings_Begin(&session);
    before = ConfigureParams;
    session.draft.Boot.nBootDevice = BOOT_SCSI;
    session.draft.Boot.bEnableDRAMTest = true;
    session.draft.Ethernet.bEthernetConnected = true;
    session.draft.Tablet.nTabletType = TABLET_MM961;
    session.draft.Sound.bEnableMicrophone = true;
    session.draft.Printer.bPrinterConnected = true;
    session.draft.Screen.nMode = SCREEN_ALL;
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    assert(!Settings_NeedRestart(&before, &session.draft));
    assert(Settings_Apply(&session, false));
    assert(!restarts && network == 1 && tablet == 1 && sound == 1 && printer == 1 && screen == 1);
    assert(scsi_out[0] == 0 && scsi_in[0] == 0);

    Settings_Begin(&session);
    session.draft.Screen.bShowStatusbar = true; /* Old imported preference: ignored. */
    assert(Settings_Apply(&session, false));
    assert(!restarts && screen == 1); /* No window rebuild or hardware reset. */

    Settings_Begin(&session);
    session.draft.System.nCpuFreq = 33;
    before = ConfigureParams;
    assert(!Settings_Apply(&session, false));
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)) && restarts == 0);
    recovery_ok = false; /* Native recovery cancelled before applying hardware. */
    assert(!Settings_Apply(&session, true));
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)) && restarts == 0);
    assert(network == 1 && screen == 1 && scsi_in[0] == 0 && scsi_out[0] == 0);
    recovery_ok = true;
    assert(Settings_Apply(&session, true));
    assert(restarts == 1);

    Settings_Begin(&session);
    session.draft.Floppy.drive[0].bDiskInserted = true;
    strcpy(session.draft.Floppy.drive[0].szImageName, "data.fd");
    /* A guest eject while the panel is open must not get undone by Save. */
    ConfigureParams.Floppy.drive[1].bDiskInserted = false;
    ConfigureParams.Floppy.drive[1].szImageName[0] = 0;
    assert(Settings_Apply(&session, false));
    assert(restarts == 1 && floppy_out[0] == 1 && floppy_in[0] == 1);
    assert(floppy_out[1] == 0 && !ConfigureParams.Floppy.drive[1].bDiskInserted);
    assert(!strcmp(ConfigureParams.Floppy.drive[0].szImageName, "data.fd"));
    assert(scsi_out[0] == 0 && scsi_in[0] == 0 && mo_out[0] == 0);

    Settings_Begin(&session);
    session.draft.MO.drive[0].bDiskInserted = true;
    strcpy(session.draft.MO.drive[0].szImageName, "data.od");
    assert(Settings_Apply(&session, false));
    assert(mo_out[0] == 1 && mo_in[0] == 1 && restarts == 1);
    assert(!strcmp(ConfigureParams.MO.drive[0].szImageName, "data.od"));

    Settings_Begin(&session);
    strcpy(session.draft.SCSI.target[0].szImageName, "other.sd");
    assert(Settings_NeedRestart(&ConfigureParams, &session.draft));
    assert(!Settings_Apply(&session, false));
    assert(!strcmp(ConfigureParams.SCSI.target[0].szImageName, "system.sd"));
    assert(scsi_out[0] == 0);

    Settings_Begin(&session);
    session.draft.Ethernet.bEthernetConnected = !session.draft.Ethernet.bEthernetConnected;
    session.draft.Ethernet.bEthernetConnected = !session.draft.Ethernet.bEthernetConnected;
    assert(Settings_Apply(&session, false));
    assert(restarts == 1 && network == 1);

    /* Shared policy also covers the legacy dialog's hardware controls. */
    Settings_Begin(&session);
    session.draft.System.bDSPMemoryExpansion = !session.draft.System.bDSPMemoryExpansion;
    assert(Change_DoNeedReset(&ConfigureParams, &session.draft));
    Settings_Begin(&session);
    session.draft.System.bCompatibleCpu = !session.draft.System.bCompatibleCpu;
    session.draft.System.bCompatibleFPU = !session.draft.System.bCompatibleFPU;
    session.draft.System.bMMU = !session.draft.System.bMMU;
    session.draft.System.bADB = true; /* Not supported on non-Turbo models. */
    assert(!Change_DoNeedReset(&ConfigureParams, &session.draft));
    Settings_Begin(&session);
    strcpy(session.draft.Rom.szRom040FileName, "inactive-040.BIN");
    strcpy(session.draft.Dimension.board[1].szRomFileName, "unused-nd.BIN");
    assert(!Settings_NeedRestart(&ConfigureParams, &session.draft));
    strcpy(session.draft.Rom.szRom030FileName, "active-030.BIN");
    assert(Settings_NeedRestart(&ConfigureParams, &session.draft));
    puts("test-settings: OK");
    return 0;
}
