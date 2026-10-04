/* Input, media and ROM migration through the overlay's actual edit session. */
#include "ui_test_support.h"
#include "overlay_input.h"
#include "overlay_controls.h"
#include "overlay_media.h"
#include "shortcut.h"
#include <assert.h>
#include <unistd.h>

static void key(SDL_Scancode sc, SDL_Keycode code) {
    SDL_Event event; SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = sc; event.key.key = code;
    assert(overlay_handle_event(&event));
}
#define KEY(k) key(SDL_SCANCODE_##k, SDLK_##k)
static void down(int n) { while (n-- > 0) KEY(DOWN); }
static void input(int row) {
    KEY(F9); KEY(RIGHT); KEY(RIGHT);
    down(OverlayControls_Count(OV_EXTENSIONS) + 1); KEY(RETURN); down(row);
}
static void text(const char *value) {
    SDL_Event event; SDL_zero(event);
    event.type = SDL_EVENT_TEXT_INPUT; event.text.text = value;
    assert(overlay_handle_event(&event));
}
static void save(bool restart) {
    KEY(F9); assert(overlay_confirm_visible());
    if (restart) KEY(LEFT);
    KEY(RETURN); assert(!overlay_is_visible());
}
static void media(int row) { KEY(F9); KEY(RIGHT); down(row); }
static void discard(bool hardware) {
    KEY(F9); assert(overlay_confirm_visible());
    if (!hardware) KEY(RIGHT);
    KEY(RETURN); assert(!overlay_is_visible());
}
static void no_disk_io(void) {
    for (int i = 0; i < ESP_MAX_DEVS; i++) assert(!scsi_in[i] && !scsi_out[i]);
    for (int i = 0; i < FLP_MAX_DRIVES; i++) assert(!floppy_in[i] && !floppy_out[i]);
    for (int i = 0; i < MO_MAX_DRIVES; i++) assert(!mo_in[i] && !mo_out[i]);
}

static void test_input(void) {
    input(0); KEY(RETURN); text("2.345678"); KEY(RETURN);
    assert(ConfigureParams.Mouse.fLinScale == 1.0f); discard(false);
    assert(ConfigureParams.Mouse.fLinScale == 1.0f);
    input(0); KEY(RETURN); text("2.345678"); KEY(RETURN); save(false);
    assert(ConfigureParams.Mouse.fLinScale == 2.345678f);
    input(0); KEY(RETURN); KEY(RETURN); KEY(F9);
    assert(!overlay_is_visible()); /* Browsing/accepting a rounded display preserves the float. */
    const char *invalid[] = {"nan", "inf", "-1", "10.001", "0", "1abc", "0x1", "1,2.3", ""};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
        input(0); KEY(RETURN); text(invalid[i]); KEY(RETURN);
        KEY(F9); assert(!overlay_confirm_visible()); /* Invalid text still being edited. */
        KEY(ESCAPE); KEY(F9); assert(!overlay_is_visible());
        assert(ConfigureParams.Mouse.fLinScale == 2.345678f);
    }
    input(1); KEY(RETURN); text("0,875"); KEY(RETURN); save(false);
    assert(ConfigureParams.Mouse.fExpScale == .875f);
    input(1); KEY(RETURN); text("1.01"); KEY(RETURN); KEY(ESCAPE); KEY(F9);
    assert(!overlay_is_visible() && ConfigureParams.Mouse.fExpScale == .875f);

    /* Capture does not leak F9 or key presses to the guest, and remains a draft. */
    input(3); KEY(RETURN); KEY(F9); assert(overlay_is_visible() && !overlay_confirm_visible());
    KEY(F4); KEY(F6); KEY(LCTRL); KEY(V); /* Reserved Ctrl+Alt combinations. */
    KEY(ESCAPE); KEY(F9); assert(!overlay_is_visible());
    input(3); KEY(RETURN); KEY(O); discard(false);
    assert(!ConfigureParams.Shortcut.withModifier[SHORTCUT_FULLSCREEN]);
    input(3); KEY(RETURN); KEY(O); save(false);
    assert(ConfigureParams.Shortcut.withModifier[SHORTCUT_FULLSCREEN] == SDLK_O);
    assert(ShortCut_CheckKeys(SDLK_O, true, true));
    assert(!ShortCut_CheckKeys(SDLK_O, false, true));
    input(2); KEY(RETURN); /* Mouse-capture action. */
    KEY(DOWN); KEY(RETURN); KEY(O); KEY(ESCAPE); KEY(F9);
    assert(!overlay_is_visible()); /* Duplicate rejected, previous binding intact. */
    assert(ConfigureParams.Shortcut.withModifier[SHORTCUT_FULLSCREEN] == SDLK_O);
    assert(!ConfigureParams.Shortcut.withModifier[SHORTCUT_MOUSEGRAB]);
    input(4); KEY(RETURN); KEY(F8); save(false);
    assert(ShortCut_CheckKeys(SDLK_F8, false, true));
    input(4); KEY(DELETE); save(false);
    assert(!ShortCut_CheckKeys(SDLK_F8, false, true));

    /* All effective actions are selectable; the retired status-bar entry is skipped. */
    OverlayInput panel = {0}; OverlayInput_Open(&panel); panel.row = 2;
    SDL_Event event; SDL_zero(event); event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_RETURN;
    unsigned seen = 0;
    for (int i = 0; i < SHORTCUT_KEYS - 2; i++) {
        assert(panel.action != SHORTCUT_STATUSBAR && panel.action != SHORTCUT_OPTIONS);
        assert(!(seen & (1u << panel.action))); seen |= 1u << panel.action;
        OverlayInput_Event(&panel, &event, &ConfigureParams);
    }
    assert(panel.action == SHORTCUT_FULLSCREEN);
    assert(!restarts && !network && !screen && !keymap_inits); no_disk_io();
}

static void test_media(const char *image) {
    media(8); KEY(C); assert(!ConfigureParams.Floppy.drive[0].bDriveConnected);
    discard(true); assert(!ConfigureParams.Floppy.drive[0].bDriveConnected);
    media(8); KEY(C); save(true);
    assert(ConfigureParams.Floppy.drive[0].bDriveConnected && !ConfigureParams.Floppy.drive[0].bDiskInserted);
    assert(restarts == 1); no_disk_io();
    media(8); KEY(C); KEY(F9); assert(!overlay_is_visible()); /* Connected already. */
    media(11); KEY(C); save(true); /* Second native MO carries warning + explicit restart. */
    assert(ConfigureParams.MO.drive[1].bDriveConnected && !ConfigureParams.MO.drive[1].bDiskInserted);
    assert(restarts == 2); no_disk_io();

    static CNF_PARAMS draft, old;
    /* Connecting an empty drive never revives stale media flags/paths. */
    draft = ConfigureParams;
    draft.Floppy.drive[0].bDriveConnected = false;
    draft.Floppy.drive[0].bDiskInserted = true;
    snprintf(draft.Floppy.drive[0].szImageName, FILENAME_MAX, "%s", image);
    assert(OverlayMedia_Connect(&draft, OV_DIALOG_FLOPPY0));
    assert(!draft.Floppy.drive[0].bDiskInserted && !draft.Floppy.drive[0].szImageName[0]);
    draft.MO.drive[0].bDiskInserted = true;
    snprintf(draft.MO.drive[0].szImageName, FILENAME_MAX, "%s", image);
    assert(OverlayMedia_Connect(&draft, OV_DIALOG_MO0));
    assert(!draft.MO.drive[0].bDiskInserted && !draft.MO.drive[0].szImageName[0]);
    /* Imported controller slots without an overlay row cannot block Save. */
    memset(&draft, 0, sizeof(draft));
    draft.Floppy.drive[2].bDriveConnected = draft.Floppy.drive[3].bDriveConnected = true;
    old = draft; draft.System.nMachineType = NEXT_CUBE040;
    assert(!OverlayMedia_Validate(&old, &draft));
    for (int machine = NEXT_CUBE030; machine <= NEXT_STATION; machine++) {
        for (int turbo = 0; turbo < 2; turbo++) {
            memset(&draft, 0, sizeof(draft));
            draft.System.nMachineType = machine; draft.System.bTurbo = turbo;
            old = draft;
            bool floppy = machine != NEXT_CUBE030;
            bool mo = machine != NEXT_STATION && !turbo;
            assert(OverlayMedia_Connect(&draft, OV_DIALOG_FLOPPY0) == floppy);
            assert(!OverlayMedia_Connect(&draft, OV_DIALOG_FLOPPY1));
            assert(OverlayMedia_Set(&draft, OV_DIALOG_FLOPPY0, image) == floppy);
            assert(!OverlayMedia_Set(&draft, OV_DIALOG_FLOPPY1, image));
            assert(OverlayMedia_Connect(&draft, OV_DIALOG_MO0) == mo);
            assert(OverlayMedia_Set(&draft, OV_DIALOG_MO1, image) == mo);
            assert(!OverlayMedia_Validate(&old, &draft));
            assert(OverlayMedia_Set(&draft, OV_DIALOG_SCSI4, image)); /* SCSI floppy unaffected. */
            assert(!OverlayMedia_Unavailable(&draft, OV_DIALOG_SCSI4));
        }
    }
    /* Existing unsupported media remain visible and removable. */
    draft = ConfigureParams; old = draft;
    draft.System.nMachineType = NEXT_STATION;
    assert(OverlayMedia_Validate(&old, &draft));
    assert(OverlayMedia_Disconnect(&draft, OV_DIALOG_MO1));
    assert(!OverlayMedia_Validate(&old, &draft));
    draft = ConfigureParams; draft.System.nMachineType = NEXT_CUBE030; old = draft;
    draft.Floppy.drive[0].bDiskInserted = true;
    assert(OverlayMedia_Eject(&draft, OV_DIALOG_FLOPPY0));
    assert(!OverlayMedia_Validate(&old, &draft));
    /* Block native pickers/creation before they can create unsupported images. */
    ConfigureParams = draft;
    media(8); KEY(RETURN); assert(!OverlayMedia_Busy()); KEY(N); KEY(C); KEY(F9);
    assert(!overlay_is_visible());
    ConfigureParams.System.nMachineType = NEXT_CUBE040;
    no_disk_io();
}

static void test_rom(const char *dir) {
    snprintf(default_rom_dir, sizeof(default_rom_dir), "%s", dir);
    const char *names[] = {"Rev_1.0_v41", "Rev_2.5_v66", "Rev_3.3_v74", "ND_step1_v43"};
    char files[4][FILENAME_MAX];
    for (int i = 0; i < 4; i++) {
        snprintf(files[i], sizeof(files[i]), "%s/%s.BIN", dir, names[i]);
        FILE *file = fopen(files[i], "wb"); assert(file); fputc(0, file); fclose(file);
    }
    int before = restarts;
    for (int crt = 0; crt < 2; crt++) {
        UI89Config_.bCrtEnabled = crt;
        for (int i = 0; i < 3; i++) {
            ConfigureParams.Rom.szRom030FileName[0] = 0;
            ConfigureParams.Rom.szRom040FileName[0] = 0;
            ConfigureParams.Rom.szRomTurboFileName[0] = 0;
            KEY(F9); KEY(RIGHT); KEY(RIGHT); KEY(RIGHT); down(10 + crt + i); KEY(D);
            save(i == 1);
            const char *path = i == 0 ? ConfigureParams.Rom.szRom030FileName : i == 1 ? ConfigureParams.Rom.szRom040FileName : ConfigureParams.Rom.szRomTurboFileName;
            assert(!strcmp(path, files[i]));
            if (i == 1) before++;
            assert(restarts == before);
        }
    }
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        assert(OverlayMedia_RestoreRom(&ConfigureParams, OV_DIALOG_NDROM0 + i));
        assert(!strcmp(ConfigureParams.Dimension.board[i].szRomFileName, files[3]));
    }
    for (int i = 0; i < 4; i++) remove(files[i]);
    CNF_PARAMS unchanged = ConfigureParams;
    assert(!OverlayMedia_RestoreRom(&ConfigureParams, OV_DIALOG_ROM040));
    assert(!memcmp(&unchanged, &ConfigureParams, sizeof(unchanged)));
    no_disk_io();
}

static void test_retired_drafts(void) {
    static SettingsSession session;
    Settings_Begin(&session);
    session.draft.ConfigDialog.bShowConfigDialogAtStartup = true;
    session.draft.SCSI.nWriteProtection = WRITEPROT_ON;
    assert(!Settings_NeedRestart(&ConfigureParams, &session.draft));
    int before = restarts;
    assert(Settings_Apply(&session, false));
    assert(!ConfigureParams.ConfigDialog.bShowConfigDialogAtStartup);
    assert(ConfigureParams.SCSI.nWriteProtection == WRITEPROT_OFF && restarts == before);
    no_disk_io();
}

static void render_preview(const char *name) {
    SDL_Surface *surface = SDL_CreateSurface(1120, 870, SDL_PIXELFORMAT_RGBA32);
    assert(surface);
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface); assert(renderer);
    overlay_render(renderer); SDL_RenderPresent(renderer);
    const char *prefix = getenv("OVERLAY_PREVIEW_PREFIX");
    if (prefix) {
        char path[FILENAME_MAX]; snprintf(path, sizeof(path), "%s%s.bmp", prefix, name);
        assert(SDL_SaveBMP(surface, path));
    }
    SDL_DestroyRenderer(renderer); SDL_DestroySurface(surface);
}

static void test_rendering(void) {
    input(0); render_preview("input");
    KEY(RETURN); render_preview("numeric"); KEY(ESCAPE);
    down(3); KEY(RETURN); KEY(F9); render_preview("binding"); KEY(ESCAPE);
    KEY(F9); assert(!overlay_is_visible());
    media(8); render_preview("media"); KEY(F9); assert(!overlay_is_visible());
    ConfigureParams.MO.drive[1].bDriveConnected = false;
    media(11); KEY(C); KEY(F9); render_preview("mo-warning");
    KEY(RETURN); assert(!overlay_is_visible()); /* Discard. */
}

int main(void) {
    char dir[] = "/tmp/1989-input-media-XXXXXX"; assert(mkdtemp(dir));
    char image[FILENAME_MAX]; snprintf(image, sizeof(image), "%s/test.fd", dir);
    FILE *file = fopen(image, "wb"); assert(file); assert(!ftruncate(fileno(file), 737280)); fclose(file);
    ConfigureParams.System.nMachineType = NEXT_CUBE040;
    ConfigureParams.Mouse.fLinScale = 1.0f; ConfigureParams.Mouse.fExpScale = .75f;
    ConfigureParams.SCSI.target[1].nDeviceType = SD_HARDDISK;
    ConfigureParams.SCSI.target[1].bDiskInserted = true;
    strcpy(ConfigureParams.SCSI.target[1].szImageName, "system.sd");
    SCSIDISK disk = ConfigureParams.SCSI.target[1];
    UI89Config_.bTinker = true;
    overlay_init();
    test_input(); test_media(image); test_rom(dir); test_retired_drafts(); test_rendering();
    assert(!memcmp(&disk, &ConfigureParams.SCSI.target[1], sizeof(disk)));
    remove(image); rmdir(dir);
    puts("test-input-media: OK");
    return 0;
}
