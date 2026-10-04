#include "main.h"
#include "settings.h"
#include "change.h"
#include "scsi.h"
#include "floppy.h"
#include "mo.h"
#include <string.h>

void Settings_Begin(SettingsSession *session) {
    session->original = session->draft = ConfigureParams;
}

void Settings_Merge(const SettingsSession *s, const CNF_PARAMS *live,
                    CNF_PARAMS *result) {
    *result = *live;
    /* Both copies originate from the same snapshot; unchanged padding is
     * identical. Media are merged per target, not as a whole controller. */
#define MERGE(field) do { \
    if (memcmp(&s->original.field, &s->draft.field, sizeof(s->draft.field))) \
        result->field = s->draft.field; \
} while (0)
    MERGE(Log); MERGE(Debugger); MERGE(Screen);
    MERGE(Keyboard); MERGE(Shortcut); MERGE(Mouse); MERGE(Tablet);
    MERGE(Sound); MERGE(Memory); MERGE(Boot); MERGE(Ethernet);
    MERGE(Rom); MERGE(Printer); MERGE(System); MERGE(Dimension);
    result->ConfigDialog.bShowConfigDialogAtStartup = false;
    result->SCSI.nWriteProtection = WRITEPROT_OFF;
    for (int i = 0; i < ESP_MAX_DEVS; i++) {
        MERGE(SCSI.target[i]);
        if (result->SCSI.target[i].nDeviceType == SD_HARDDISK &&
            !result->SCSI.target[i].bDiskInserted)
            result->SCSI.target[i].nDeviceType = SD_NONE;
    }
    for (int i = 0; i < FLP_MAX_DRIVES; i++) MERGE(Floppy.drive[i]);
    for (int i = 0; i < MO_MAX_DRIVES; i++) MERGE(MO.drive[i]);
#undef MERGE
}

bool Settings_NeedRestart(const CNF_PARAMS *a, const CNF_PARAMS *b) {
    CNF_SYSTEM old_system = a->System, new_system = b->System;
    /* M68000_CheckCpuSettings hard-codes these inherited compatibility
     * options, including the MMU. They do not alter the running hardware. */
    old_system.bCompatibleCpu = new_system.bCompatibleCpu;
    old_system.bCompatibleFPU = new_system.bCompatibleFPU;
    old_system.bMMU = new_system.bMMU;
    if (!old_system.bTurbo) old_system.bADB = false;
    if (!new_system.bTurbo) new_system.bADB = false;
    /* Boot options are consumed on the NEXT boot. Changing them must not
     * interrupt the OS just to update its next boot device or diagnostics. */
    if (memcmp(&old_system, &new_system, sizeof(old_system)) ||
        memcmp(&a->Memory, &b->Memory, sizeof(a->Memory)) ||
        a->Ethernet.nHostInterface != b->Ethernet.nHostInterface ||
        a->Ethernet.bNetworkTime != b->Ethernet.bNetworkTime)
        return true;
    const char *old_rom = a->System.nMachineType == NEXT_CUBE030 ? a->Rom.szRom030FileName :
                         a->System.bTurbo ? a->Rom.szRomTurboFileName : a->Rom.szRom040FileName;
    const char *new_rom = b->System.nMachineType == NEXT_CUBE030 ? b->Rom.szRom030FileName :
                         b->System.bTurbo ? b->Rom.szRomTurboFileName : b->Rom.szRom040FileName;
    if (strcmp(old_rom, new_rom) || a->Rom.bUseCustomMac != b->Rom.bUseCustomMac ||
        (b->Rom.bUseCustomMac && memcmp(a->Rom.nRomCustomMac, b->Rom.nRomCustomMac, sizeof(a->Rom.nRomCustomMac))))
        return true;
    if (a->Dimension.bI860Thread != b->Dimension.bI860Thread ||
        a->Dimension.nConsoleSlot != b->Dimension.nConsoleSlot) return true;
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        const NDBOARD *x = &a->Dimension.board[i], *y = &b->Dimension.board[i];
        if (x->bEnabled != y->bEnabled ||
            (y->bEnabled && (strcmp(x->szRomFileName, y->szRomFileName) ||
             memcmp(x->nMemoryBankSize, y->nMemoryBankSize, sizeof(x->nMemoryBankSize)))))
            return true;
    }
    for (int i = 0; i < ESP_MAX_DEVS; i++) {
        const SCSIDISK *x = &a->SCSI.target[i], *y = &b->SCSI.target[i];
        if (x->nDeviceType != y->nDeviceType ||
            ((x->nDeviceType == SD_HARDDISK || y->nDeviceType == SD_HARDDISK) &&
             (x->bDiskInserted != y->bDiskInserted ||
              x->bWriteProtected != y->bWriteProtected ||
              strcmp(x->szImageName, y->szImageName))))
            return true;
    }
    for (int i = 0; i < FLP_MAX_DRIVES; i++)
        if (a->Floppy.drive[i].bDriveConnected != b->Floppy.drive[i].bDriveConnected)
            return true;
    for (int i = 0; i < MO_MAX_DRIVES; i++)
        if (a->MO.drive[i].bDriveConnected != b->MO.drive[i].bDriveConnected)
            return true;
    return false;
}

bool Settings_MediaChanged(const CNF_PARAMS *a, const CNF_PARAMS *b) {
    return memcmp(&a->SCSI, &b->SCSI, sizeof(a->SCSI)) ||
           memcmp(&a->Floppy, &b->Floppy, sizeof(a->Floppy)) ||
           memcmp(&a->MO, &b->MO, sizeof(a->MO));
}

bool Settings_Apply(SettingsSession *session, bool restart_confirmed) {
    CNF_PARAMS current = ConfigureParams, changed;
    Settings_Merge(session, &current, &changed);
    bool restart = Settings_NeedRestart(&current, &changed);
    if (restart && !restart_confirmed) return false;

    if (!Change_CopyChangedParamsToConfiguration(&current, &changed, false)) return false;
    if (!restart) {
        /* Only exchange the selected removable medium. In particular, never
         * reopen every SCSI hard disk when the user changes a floppy. */
        for (int i = 0; i < ESP_MAX_DEVS; i++) {
            if (!memcmp(&current.SCSI.target[i], &changed.SCSI.target[i], sizeof(SCSIDISK))) continue;
            SCSI_Eject(i);
            ConfigureParams.SCSI.target[i] = changed.SCSI.target[i];
            SCSI_Insert(i);
        }
        for (int i = 0; i < FLP_MAX_DRIVES; i++) {
            if (!memcmp(&current.Floppy.drive[i], &changed.Floppy.drive[i], sizeof(FLPDISK))) continue;
            Floppy_Eject(i);
            ConfigureParams.Floppy.drive[i] = changed.Floppy.drive[i];
            if (changed.Floppy.drive[i].bDiskInserted && Floppy_Insert(i))
                ConfigureParams.Floppy.drive[i].bDiskInserted = false;
        }
        for (int i = 0; i < MO_MAX_DRIVES; i++) {
            if (!memcmp(&current.MO.drive[i], &changed.MO.drive[i], sizeof(MODISK))) continue;
            MO_Eject(i);
            ConfigureParams.MO.drive[i] = changed.MO.drive[i];
            if (changed.MO.drive[i].bDiskInserted) MO_Insert(i);
        }
    }
    Settings_Begin(session);
    return true;
}
