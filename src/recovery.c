/* 1989 resource recovery through native host choices and file/folder pickers. */
#include "main.h"
#include "recovery.h"
#include "host_dialog.h"
#include "file.h"
#include "rom.h"
#include "reset.h"
#include "log.h"
#include "paste.h"

static bool usable(const char *path, bool folder) {
    if (folder) return File_DirExists(path);
    if (!File_Exists(path) || File_DirExists(path)) return false;
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    fclose(file);
    return true;
}

/* Returns false to cancel the whole recovery; remove is always explicit. */
static bool recover(char *path, bool folder, const char *label, const char *default_path,
                    const char *remove_label, bool *remove, bool startup) {
    const char *problem = "The configured path is missing or unavailable.";
    *remove = false;
    while (!usable(path, folder)) {
        char message[FILENAME_MAX + 512];
        snprintf(message, sizeof(message), "%s\n\n%s\n%s\n\nChoose a replacement%s.",
                 label, problem, path[0] ? path : "(no path configured)",
                 remove_label ? " or explicitly remove/disable this resource" : " to continue");
        const char *choices[4] = {startup ? "Quit" : "Cancel changes", folder ? "Choose folder" : "Choose file"};
        int count = 2, default_button = -1, remove_button = -1;
        if (default_path && usable(default_path, folder)) {
            default_button = count; choices[count++] = "Use default";
        }
        if (remove_label) { remove_button = count; choices[count++] = remove_label; }
        int choice = HostDialog_Choose("1989 - resource unavailable", message, choices, count);
        if (choice < 1 || bQuitProgram) return false;
        if (choice == remove_button) { *remove = true; return true; }
        if (choice == default_button) snprintf(path, FILENAME_MAX, "%s", default_path);
        else if (choice == 1) {
            HostPickResult result = HostDialog_Pick(path, FILENAME_MAX, folder);
            if (result == HOST_PICK_QUIT) return false;
            problem = result == HOST_PICK_ERROR ?
                "The native picker failed. Retry, cancel, or edit 1989.conf while closed." :
                "The configured path is missing or unavailable.";
        }
    }
    return true;
}

bool Recovery_CheckFiles(CNF_PARAMS *params, bool startup) {
    CNF_PARAMS draft = *params;
    bool remove;
    char label[80], default_path[FILENAME_MAX];
    bool turbo = draft.System.bTurbo, cube030 = draft.System.nMachineType == NEXT_CUBE030;
    char *rom = cube030 ? draft.Rom.szRom030FileName : turbo ? draft.Rom.szRomTurboFileName : draft.Rom.szRom040FileName;
    Rom_GetDefaultPath(default_path, sizeof(default_path), cube030 ? "Rev_1.0_v41" : turbo ? "Rev_3.3_v74" : "Rev_2.5_v66");
    if (!recover(rom, false, "Machine ROM", default_path, NULL, &remove, startup)) return false;
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        NDBOARD *board = &draft.Dimension.board[i];
        if (!board->bEnabled) continue;
        snprintf(label, sizeof(label), "NeXTdimension ROM, slot %d", 2 * (i + 1));
        Rom_GetDefaultPath(default_path, sizeof(default_path), "ND_step1_v43");
        if (!recover(board->szRomFileName, false, label, default_path, "Disable board", &remove, startup)) return false;
        if (remove) {
            board->bEnabled = false;
            if (draft.Dimension.nConsoleSlot == 2 * (i + 1)) draft.Dimension.nConsoleSlot = 0;
            if (draft.Screen.nSingleModeSlot == 2 * (i + 1)) draft.Screen.nSingleModeSlot = 0;
        }
    }
    for (int i = 0; i < ESP_MAX_DEVS; i++) {
        SCSIDISK *disk = &draft.SCSI.target[i];
        if (disk->nDeviceType == SD_NONE || !disk->bDiskInserted) continue;
        snprintf(label, sizeof(label), "SCSI image, ID %d", i);
        if (!recover(disk->szImageName, false, label, NULL, disk->nDeviceType == SD_HARDDISK ?
                     "Disconnect disk" : "Leave drive empty", &remove, startup)) return false;
        if (remove) {
            disk->szImageName[0] = 0; disk->bDiskInserted = false;
            if (disk->nDeviceType == SD_HARDDISK) disk->nDeviceType = SD_NONE;
        }
    }
    for (int i = 0; i < MO_MAX_DRIVES; i++) {
        MODISK *disk = &draft.MO.drive[i];
        if (!disk->bDriveConnected || !disk->bDiskInserted) continue;
        snprintf(label, sizeof(label), "Native MO image, drive %d", i);
        if (!recover(disk->szImageName, false, label, NULL, "Leave drive empty", &remove, startup)) return false;
        if (remove) { disk->szImageName[0] = 0; disk->bDiskInserted = false; }
    }
    for (int i = 0; i < FLP_MAX_DRIVES; i++) {
        FLPDISK *disk = &draft.Floppy.drive[i];
        if (!disk->bDriveConnected || !disk->bDiskInserted) continue;
        snprintf(label, sizeof(label), "Native floppy image, drive %d", i);
        if (!recover(disk->szImageName, false, label, NULL, "Leave drive empty", &remove, startup)) return false;
        if (remove) { disk->szImageName[0] = 0; disk->bDiskInserted = false; }
    }
    if (draft.Ethernet.bEthernetConnected && draft.Ethernet.nHostInterface == ENET_SLIRP) {
        for (int i = 0; i < EN_MAX_SHARES; i++) {
            char *path = draft.Ethernet.nfs[i].szPathName;
            if (!path[0]) continue;
            snprintf(label, sizeof(label), "NFS shared folder %d", i);
            if (!recover(path, true, label, NULL, "Disable share", &remove, startup)) return false;
            if (remove) path[0] = 0;
        }
    }
    if (draft.Printer.bPrinterConnected) {
        if (!recover(draft.Printer.szPrintToFileName, true, "Printer output folder", NULL,
                     "Disable printer", &remove, startup)) return false;
        if (remove) draft.Printer.bPrinterConnected = false;
    }
    if (bQuitProgram) return false;
    *params = draft;
    return true;
}

void Recovery_Halt(void) {
    Main_PauseEmulation(true);
    paste_stop();
    Log_Printf(LOG_WARN, "Fatal error: CPU halted!");
    const char *choices[] = {"Quit", "Restart machine"};
    int choice = HostDialog_Choose("1989 - CPU halted",
        "The emulated CPU has halted.\n\nRestarting cannot flush the guest's disk caches. Restart the machine or quit?",
        choices, 2);
    if (choice != 1 || bQuitProgram || Reset_Cold()) Main_RequestQuit(false);
    else Main_UnPauseEmulation();
}
