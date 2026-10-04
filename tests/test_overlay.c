#include "ui_test_support.h"
#include "overlay_media.h"
#include "overlay_view.h"
#include "overlay_controls.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static void key(SDL_Scancode sc) {
    SDL_Event ev;
    SDL_zero(ev); /* Initialize the whole union, including key.repeat. */
    ev.type = SDL_EVENT_KEY_DOWN; ev.key.scancode = sc;
    assert(overlay_handle_event(&ev));
}
static void down(int count) { for (int i = 0; i < count; i++) key(SDL_SCANCODE_DOWN); }

static void open_media(int row) {
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RIGHT); down(row);
}

static void save_media(void) {
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN); /* Default is Discard. */
    assert(!overlay_is_visible());
}

static void test_scsi_layout(const char *image) {
    static CNF_PARAMS p;
    const int ids[] = {1, 2, 3, 6};
    for (int i = 0; i < 4; i++) {
        assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI0 + ids[i], image));
        assert(OverlayMedia_ScsiDiskNumber(&p, ids[i]) == i);
    }
    assert(p.SCSI.target[3].nDeviceType == SD_CD && p.SCSI.target[3].bWriteProtected);
    assert(p.SCSI.target[0].nDeviceType == SD_NONE);
    assert(OverlayMedia_ScsiDiskNumber(&p, 7) == -1);
    assert(OverlayMedia_Eject(&p, OV_DIALOG_SCSI3));
    assert(OverlayMedia_ScsiDiskNumber(&p, 3) == 2); /* Empty CD drive is still attached. */
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI0, image));
    assert(OverlayMedia_ScsiDiskNumber(&p, 1) == 1); /* Alternate boot takes sd0. */
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI4, image));
    assert(p.SCSI.target[4].nDeviceType == SD_FLOPPY);
    /* Historical roles do not override an explicitly configured disk. */
    p.SCSI.target[3].nDeviceType = SD_HARDDISK;
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI3, image));
    assert(p.SCSI.target[3].nDeviceType == SD_HARDDISK);
    memset(&p, 0, sizeof(p));
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI1, image));
    assert(OverlayMedia_Set(&p, OV_DIALOG_SCSI6, image));
    assert(OverlayMedia_ScsiDiskNumber(&p, 1) == 0);
    assert(OverlayMedia_ScsiDiskNumber(&p, 6) == 1);
    assert(OverlayMedia_ScsiDiskNumber(&p, 3) == -1);
}

static void test_removable_eject(const char *image) {
    int resets_before = restarts;
    /* Keep an existing boot disk at ID 0 as well as one at the suggested ID 1. */
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI0, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI1, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI3, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_SCSI4, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, image));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_MO1, image));
    CNF_PARAMS before = ConfigureParams;

    open_media(4); /* ID 3: headers and the reserved host ID are not selectable. */
    key(SDL_SCANCODE_H); key(SDL_SCANCODE_E); key(SDL_SCANCODE_ESCAPE);
    key(SDL_SCANCODE_E);
    assert(!memcmp(&ConfigureParams, &before, sizeof(before))); /* Still a draft. */
    save_media();
    assert(!ConfigureParams.SCSI.target[3].bDiskInserted);
    assert(ConfigureParams.SCSI.target[3].nDeviceType == SD_CD);
    assert(scsi_out[3] == 1 && scsi_in[3] == 1);
    assert(restarts == resets_before);

    /* Reloading an ejected CD also avoids a restart. */
    open_media(4); key(SDL_SCANCODE_RETURN);
    const char *files[] = {image, NULL};
    picker_callback(picker_userdata, files, 0); overlay_tick();
    save_media();
    assert(ConfigureParams.SCSI.target[3].bDiskInserted);
    assert(scsi_out[3] == 2 && scsi_in[3] == 2 && restarts == resets_before);

    /* Enter on a loaded drive opens a replacement picker; cancellation is a no-op. */
    open_media(4); key(SDL_SCANCODE_RETURN);
    assert(OverlayMedia_Busy());
    picker_callback(picker_userdata, NULL, 0); overlay_tick();
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && ConfigureParams.SCSI.target[3].bDiskInserted);

    open_media(5); key(SDL_SCANCODE_E); save_media(); /* SCSI floppy, ID 4. */
    assert(ConfigureParams.SCSI.target[4].nDeviceType == SD_FLOPPY);
    assert(!ConfigureParams.SCSI.target[4].bDiskInserted && scsi_out[4] == 1);
    open_media(8); key(SDL_SCANCODE_E); save_media(); /* Native floppy 0. */
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected);
    assert(!ConfigureParams.Floppy.drive[0].bDiskInserted && floppy_out[0] == 1);
    open_media(11); key(SDL_SCANCODE_E); save_media(); /* Native MO 1. */
    assert(ConfigureParams.MO.drive[1].bDriveConnected);
    assert(!ConfigureParams.MO.drive[1].bDiskInserted && mo_out[1] == 1);
    assert(!floppy_out[1] && !mo_out[0] && !floppy_in[0] && !mo_in[1]);
    assert(restarts == resets_before && scsi_out[3] == 2);
    assert(!scsi_out[0] && !scsi_in[0] && !scsi_out[1] && !scsi_in[1]);
    assert(!memcmp(&ConfigureParams.SCSI.target[0], &before.SCSI.target[0], sizeof(SCSIDISK)));
    assert(!memcmp(&ConfigureParams.SCSI.target[1], &before.SCSI.target[1], sizeof(SCSIDISK)));

    open_media(1); key(SDL_SCANCODE_E); key(SDL_SCANCODE_F9); /* Fixed HDD cannot be ejected. */
    assert(!overlay_is_visible() && !overlay_confirm_visible());
    open_media(4); key(SDL_SCANCODE_DELETE); save_media(); /* Disconnect is different. */
    assert(ConfigureParams.SCSI.target[3].nDeviceType == SD_NONE);
    assert(restarts == resets_before + 1);
}

/* Find a migrated control by its user-facing label, ignoring headings. */
static int control_row(OvSection section, const char *label) {
    OverlayView view = {0};
    OverlayControls_AddRows(&view, section, -1, &ConfigureParams);
    int row = 0;
    for (int i = 0; i < view.row_count; i++) {
        if (view.rows[i].heading) continue;
        if (!strcmp(view.rows[i].label, label)) return row;
        row++;
    }
    assert(!"control not found");
    return -1;
}

static void extension(const char *label) {
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RIGHT);
    down(control_row(OV_EXTENSIONS, label));
}

static void save_live(void) {
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_RETURN); /* Live edits default to Save. */
    assert(!overlay_is_visible());
}

static void test_migrated_controls(const char *dir, const char *image) {
    OverlayView formats = {0};
    OverlayControls_AddRows(&formats, OV_EXTENSIONS, -1, &ConfigureParams);
    bool saw_format = false;
    for (int i = 0; i < formats.row_count; i++) {
        if (strcmp(formats.rows[i].label, "Image format")) continue;
        saw_format = true;
#if HAVE_LIBPNG
        assert(!strcmp(formats.rows[i].value, "PNG"));
#else
        assert(!strcmp(formats.rows[i].value, "TIFF (PNG unavailable)"));
#endif
    }
    assert(saw_format);
    CNF_PARAMS before = ConfigureParams;
    int resets_before = restarts, printer_before = printer;
    int keymaps_before = keymap_inits;
    int disk_in[ESP_MAX_DEVS], disk_out[ESP_MAX_DEVS];
    memcpy(disk_in, scsi_in, sizeof(disk_in));
    memcpy(disk_out, scsi_out, sizeof(disk_out));

    /* Keyboard and printer edits share the draft, including Discard. */
    extension("Keyboard mapping"); key(SDL_SCANCODE_RETURN);
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RETURN);
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));

    extension("Keyboard mapping"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Keyboard.nKeymapType == KEYMAP_SCANCODE);
    extension("Swap Command / Alt"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Keyboard.bSwapCmdAlt);
    extension("Raw mouse motion"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Mouse.bUseRawMotion && keymap_inits == keymaps_before + 1);
    extension("Ctrl-click -> right"); key(SDL_SCANCODE_RETURN); save_live();
    extension("Wheel -> arrow keys"); key(SDL_SCANCODE_RETURN); save_live();
    extension("Automatic mouse grab"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Mouse.bEnableMacClick && ConfigureParams.Mouse.bEnableMapToKey);
    assert(ConfigureParams.Mouse.bEnableAutoGrab);

    /* A pre-existing custom sensitivity must survive browsing and other edits. */
    ConfigureParams.Mouse.fLinScale = 2.345f;
    extension("Mouse slow motion"); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && ConfigureParams.Mouse.fLinScale == 2.345f);
    extension("Mouse slow motion"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Mouse.fLinScale == .750f);
    ConfigureParams.Mouse.fExpScale = .750f;
    extension("Mouse fast motion"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Mouse.fExpScale == .875f);

    extension("Paper size"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Printer.nPaperSize == PAPER_LETTER);
    extension("Image format"); key(SDL_SCANCODE_RETURN); save_live();
    assert(ConfigureParams.Printer.nFileFormat == FORMAT_TIFF);

    /* Native folder results are staged, validated and cancellable. */
    extension("Output directory"); key(SDL_SCANCODE_RETURN);
    assert(OverlayMedia_Busy() && folder_requests == 1);
    const char *files[] = {dir, NULL};
    picker_callback(picker_userdata, files, 0); overlay_tick();
    assert(!strcmp(ConfigureParams.Printer.szPrintToFileName, before.Printer.szPrintToFileName));
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RETURN);
    assert(!strcmp(ConfigureParams.Printer.szPrintToFileName, before.Printer.szPrintToFileName));

    extension("Output directory"); key(SDL_SCANCODE_RETURN);
    picker_callback(picker_userdata, files, 0); overlay_tick(); save_live();
    assert(!strcmp(ConfigureParams.Printer.szPrintToFileName, dir));
    assert(!strcmp(UI89Config_.szLastDir[OV_DIALOG_PRINTER_DIR], dir));
    extension("Output directory"); key(SDL_SCANCODE_RETURN);
    files[0] = image; /* An image file is not a valid output folder. */
    picker_callback(picker_userdata, files, 0); overlay_tick(); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !strcmp(ConfigureParams.Printer.szPrintToFileName, dir));
    extension("Output directory"); key(SDL_SCANCODE_RETURN);
    picker_callback(picker_userdata, NULL, 0); overlay_tick(); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible());
    extension("Output directory"); key(SDL_SCANCODE_RETURN);
    overlay_quit(); /* Invalidate the outstanding callback, as on shutdown. */
    files[0] = "/tmp";
    picker_callback(picker_userdata, files, 0); overlay_tick(); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !strcmp(ConfigureParams.Printer.szPrintToFileName, dir));

    /* All power-on controls live in Media and only affect the next boot. */
    CNF_BOOT old_boot = ConfigureParams.Boot;
    open_media(12); /* Device rows precede the diagnostic controls. */
    for (int i = 0; i < OverlayControls_Count(OV_MEDIA); i++) {
        key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_DOWN);
    }
    assert(!memcmp(&old_boot, &ConfigureParams.Boot, sizeof(old_boot)));
    save_live();
    assert(ConfigureParams.Boot.bEnablePot != old_boot.bEnablePot);
    assert(ConfigureParams.Boot.bEnableDRAMTest != old_boot.bEnableDRAMTest);
    assert(ConfigureParams.Boot.bEnableSoundTest != old_boot.bEnableSoundTest);
    assert(ConfigureParams.Boot.bEnableSCSITest != old_boot.bEnableSCSITest);
    assert(ConfigureParams.Boot.bLoopPot != old_boot.bLoopPot);
    assert(ConfigureParams.Boot.bExtendedPot != old_boot.bExtendedPot);
    assert(ConfigureParams.Boot.bVisible != old_boot.bVisible);
    assert(ConfigureParams.Boot.bVerbose != old_boot.bVerbose);
    assert(ConfigureParams.Boot.nBootDevice == old_boot.nBootDevice);
    assert(restarts == resets_before && printer == printer_before);
    assert(!memcmp(disk_in, scsi_in, sizeof(disk_in)) && !memcmp(disk_out, scsi_out, sizeof(disk_out)));
    assert(!memcmp(&before.SCSI, &ConfigureParams.SCSI, sizeof(before.SCSI)));

    /* Protect layout/navigation from silently truncating a growing definition. */
    const int base_rows[] = {11, 15, 0, 15};
    for (int section = OV_GENERAL; section <= OV_ADVANCED; section++) {
        OverlayView view = {0};
        int count = OverlayControls_Count(section), selectable = 0, selected = 0;
        OverlayControls_AddRows(&view, section, count - 1, &ConfigureParams);
        for (int i = 0; i < view.row_count; i++) {
            if (!view.rows[i].heading) selectable++;
            if (view.rows[i].selected) selected++;
        }
        assert(count == selectable && selected == 1 && view.hint);
        assert(view.row_count + base_rows[section] <= OVERLAY_MAX_ROWS);
    }
}

static void hardware(OvSection section, const char *label) {
    key(SDL_SCANCODE_F9);
    if (section == OV_ADVANCED) key(SDL_SCANCODE_LEFT);
    int base = section == OV_GENERAL ? 11 : (UI89Config_.bCrtEnabled ? 15 : 14);
    down(base + control_row(section, label));
}

static void test_hardware_controls(void) {
    CNF_PARAMS before = ConfigureParams;
    int resets_before = restarts, saves_before = saved;
    hardware(OV_GENERAL, "RAM bank 0"); key(SDL_SCANCODE_RETURN);
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_ESCAPE); /* Editing resumes; hardware remains staged. */
    assert(overlay_is_visible() && !overlay_confirm_visible());
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Default Discard. */
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    assert(restarts == resets_before && saved == saves_before);

    /* Every new hardware control must go through the restart confirmation.
     * Test Advanced with the conditional scanline row both shown and hidden. */
    for (int crt = 0; crt < 2; crt++) {
        UI89Config_.bCrtEnabled = crt;
        const OvSection sections[] = {OV_GENERAL, OV_ADVANCED};
        for (int s = 0; s < 2; s++) {
            OvSection section = sections[s];
            OverlayView view = {0};
            OverlayControls_AddRows(&view, section, -1, &ConfigureParams);
            for (int i = 0; i < view.row_count; i++) {
                if (view.rows[i].heading) continue;
                before = ConfigureParams;
                resets_before = restarts;
                hardware(section, view.rows[i].label); key(SDL_SCANCODE_RETURN);
                assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
                key(SDL_SCANCODE_F9);
                assert(overlay_confirm_visible() && restarts == resets_before);
                key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN); /* Explicit Restart. */
                assert(!overlay_is_visible() && restarts == resets_before + 1);
                assert(memcmp(&before, &ConfigureParams, sizeof(before)));
                assert(!memcmp(&before.SCSI, &ConfigureParams.SCSI, sizeof(before.SCSI)));
                assert(!memcmp(&before.Floppy, &ConfigureParams.Floppy, sizeof(before.Floppy)));
                assert(!memcmp(&before.MO, &ConfigureParams.MO, sizeof(before.MO)));
            }
        }
    }

    resets_before = restarts;
    saves_before = saved;
    hardware(OV_GENERAL, "Variable CPU clock");
    key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_RETURN); /* Revert to original. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible());
    assert(restarts == resets_before && saved == saves_before);

    ConfigureParams.System.bNBIC = false;
    before = ConfigureParams;
    extension("NeXTdimension");
    key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible());
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    assert(restarts == resets_before && saved == saves_before);

    /* Inactive model-specific controls produce neither an edit nor a restart. */
    ConfigureParams.System.nMachineType = NEXT_STATION;
    ConfigureParams.System.bTurbo = ConfigureParams.System.bColor = false;
    ConfigureParams.Memory.nMemoryBankSize[2] = ConfigureParams.Memory.nMemoryBankSize[3] = 0;
    ConfigureParams.System.bNBIC = false;
    before = ConfigureParams;
    hardware(OV_GENERAL, "RAM bank 2");
    key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_DOWN); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible());
    hardware(OV_ADVANCED, "NBIC"); key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible());
    extension("NeXTdimension"); key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible());
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    assert(restarts == resets_before && saved == saves_before);
}

static void test_machine_summary(void) {
    CNF_PARAMS before = ConfigureParams;
    char label[128], draft_label[128];
    ConfigureParams.System.nMachineType = NEXT_STATION;
    ConfigureParams.System.bTurbo = ConfigureParams.System.bColor = true;
    ConfigureParams.System.nCpuFreq = 40;
    for (int i = 0; i < 4; i++) ConfigureParams.Memory.nMemoryBankSize[i] = 32;
    overlay_machine_summary(label, sizeof(label));
    assert(!strcmp(label, "1989 NeXTstation Turbo Color | 40 MHz | 128 MB"));
    key(SDL_SCANCODE_F9); down(2); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_UP); key(SDL_SCANCODE_RETURN); /* Edit RAM as well. */
    overlay_machine_summary(draft_label, sizeof(draft_label));
    assert(!strcmp(label, draft_label)); /* Display active hardware, not the draft. */
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Discard. */
    ConfigureParams.System.bRealtime = true;
    overlay_machine_summary(label, sizeof(label));
    assert(strstr(label, "40 MHz (variable) | 128 MB"));
    ConfigureParams = before;
}

int main(void) {
    ConfigureParams.System.nCpuFreq = 25;
    ConfigureParams.Mouse.fLinScale = 1.0f;
    ConfigureParams.Mouse.fExpScale = .75f;
    snprintf(ConfigureParams.Printer.szPrintToFileName, FILENAME_MAX, "/tmp");
    UI89Config_.bTinker = true;
    UI89Config_.nGifFps = 25;
    overlay_init();
    test_machine_summary();
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); /* Media, boot device. */
    key(SDL_SCANCODE_RETURN);
    assert(ConfigureParams.Boot.nBootDevice == BOOT_ROM);
    key(SDL_SCANCODE_F9);
    assert(overlay_confirm_visible());
    key(SDL_SCANCODE_RIGHT); /* Discard. */
    key(SDL_SCANCODE_RETURN);
    assert(!overlay_is_visible() && !saved && !restarts);
    assert(ConfigureParams.Boot.nBootDevice == BOOT_ROM);

    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Save boot change. */
    assert(saved == 1 && ConfigureParams.Boot.nBootDevice == BOOT_SCSI && !restarts);

    key(SDL_SCANCODE_F9);
    down(2); key(SDL_SCANCODE_RETURN); /* Clock is staged, requires restart. */
    assert(ConfigureParams.System.nCpuFreq == 25);
    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_ESCAPE); /* Return to editing with the draft intact. */
    assert(overlay_is_visible() && !overlay_confirm_visible());
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Default is Discard. */
    assert(ConfigureParams.System.nCpuFreq == 25 && restarts == 0 && saved == 1);

    key(SDL_SCANCODE_F9);
    down(2); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN);
    assert(ConfigureParams.System.nCpuFreq == 33 && restarts == 1 && saved == 2);
    overlay_confirm_reset(); key(SDL_SCANCODE_RETURN); /* Default Cancel. */
    assert(restarts == 1);
    overlay_confirm_reset(); key(SDL_SCANCODE_LEFT); key(SDL_SCANCODE_RETURN);
    assert(restarts == 2);

    key(SDL_SCANCODE_F9);
    key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RIGHT); down(2); /* Ethernet. */
    key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_RETURN); /* A no-op edit. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible() && saved == 2);

    key(SDL_SCANCODE_F9);
    down(5); key(SDL_SCANCODE_RETURN); /* MMU is always on: no false edit. */
    down(1); key(SDL_SCANCODE_RETURN); /* ADB requires a Turbo machine. */
    key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !overlay_confirm_visible() && saved == 2);

    /* Software rendering exercises all tabs and modals without a real display. */
    SDL_Surface *surface = SDL_CreateSurface(1120, 870, SDL_PIXELFORMAT_RGBA32);
    assert(surface);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    key(SDL_SCANCODE_F9);
    for (int i = 0; i < 4; i++) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        overlay_render(renderer);
        SDL_RenderPresent(renderer);
        const char *preview = getenv("OVERLAY_PREVIEW_PREFIX");
        if (preview) {
            char filename[FILENAME_MAX];
            snprintf(filename, sizeof(filename), "%s%d.bmp", preview, i);
            assert(SDL_SaveBMP(surface, filename));
        }
        key(SDL_SCANCODE_RIGHT);
    }
    overlay_confirm_reset(); overlay_render(renderer); key(SDL_SCANCODE_ESCAPE);
    key(SDL_SCANCODE_F9);
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);

    /* A late native picker result cannot be delivered to a newer session. */
    assert(OverlayMedia_Request(OV_DIALOG_SCSI0, &UI89Config_, 0));
    assert(!OverlayMedia_Request(OV_DIALOG_SCSI1, &UI89Config_, 0));
    OverlayMedia_Cancel();
    const char *files[] = { "late.sd", NULL };
    picker_callback(picker_userdata, files, 0);
    OvDialogKind kind; char path[FILENAME_MAX]; long long size;
    assert(OverlayMedia_Poll(&kind, path, &size) && kind == OV_DIALOG_NONE);
    assert(!OverlayMedia_Busy());

    char dir[] = "/tmp/1989-media-XXXXXX";
    assert(mkdtemp(dir));
    char name[FILENAME_MAX], created[FILENAME_MAX];
    snprintf(name, sizeof(name), "%s/blank", dir);
    assert(OverlayMedia_Create(name, 2147483648LL, created));
    struct stat st;
    assert(stat(created, &st) == 0 && st.st_size == 2147483648LL);
    assert(!OverlayMedia_Create(name, 737280, path)); /* Never overwrite. */
    assert(stat(created, &st) == 0 && st.st_size == 2147483648LL);
    assert(!OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, created));
    remove(created);
    assert(OverlayMedia_Create(name, 737280, created));
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, created));
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected);
    assert(OverlayMedia_Set(&ConfigureParams, OV_DIALOG_FLOPPY0, NULL));
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected); /* Disk != drive. */
    test_scsi_layout(created);
    test_removable_eject(created);
    test_migrated_controls(dir, created);
    test_hardware_controls();
    remove(created); rmdir(dir);
    puts("test-overlay: OK");
    return 0;
}
