/* Verify overlay choices against the real core's hardware normalization. */
#include "main.h"
#include "overlay_controls.h"
#include "overlay_devices.h"
#include "file.h"
#include "host.h"
#include "m68000.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Configuration_Apply uses the real RAM/peripheral normalization; CPU setup
 * and host path canonicalization are outside this hardware-selector test. */
void M68000_CheckCpuSettings(void) {}
int host_num_cpus(void) { return 1; }
void File_CleanFileName(char *path) { (void)path; }
void File_MakeAbsoluteName(char *path) { (void)path; }
void File_MakeAbsoluteSpecialName(char *path) { (void)path; }
SDL_Window *sdlWindow;
bool File_Exists(const char *path) { (void)path; return true; }
bool File_DirExists(const char *path) { (void)path; return false; }
off_t File_Length(const char *path) { (void)path; return 0; }
void notify_post(const char *fmt, ...) { (void)fmt; }
void Rom_GetDefaultPath(char *path, int length, const char *name) { snprintf(path, length, "%s.BIN", name); }
void Log_PrintfInt(LOGTYPE level, const char *format, ...) { (void)level; (void)format; }

static int row(OvSection section, const char *label, const CNF_PARAMS *p) {
    OverlayView view = {0};
    OverlayControls_AddRows(&view, section, -1, p);
    int index = 0;
    for (int i = 0; i < view.row_count; i++) {
        if (view.rows[i].heading) continue;
        if (!strcmp(view.rows[i].label, label)) return index;
        index++;
    }
    assert(!"missing hardware control");
    return -1;
}

static void value(const CNF_PARAMS *p, const char *label, const char *expected) {
    OverlayView view = {0};
    OverlayControls_AddRows(&view, OV_GENERAL, -1, p);
    for (int i = 0; i < view.row_count; i++) {
        if (strcmp(view.rows[i].label, label)) continue;
        assert(!strcmp(view.rows[i].value, expected));
        return;
    }
    assert(!"missing hardware value");
}

static void edit(CNF_PARAMS *p, OvSection section, const char *label) {
    assert(OverlayControls_Activate(section, row(section, label, p), p) == OV_DIALOG_NONE);
}

static void test_model(int machine, bool turbo, bool color) {
    static CNF_PARAMS draft, live;
    memset(&draft, 0, sizeof(draft));
    draft.System.nMachineType = machine;
    draft.System.bTurbo = turbo;
    draft.System.bColor = color;
    Configuration_SetSystemDefaultsFor(&draft);
    ConfigureParams = draft; /* The real memory checker reads the active model. */
    live = ConfigureParams;

    int available = machine == NEXT_STATION && !turbo && !color ? 2 : 4;
    int capacity = turbo ? 32 : color ? 8 : 16;
    for (int bank = 0; bank < 4; bank++) {
        char label[32];
        snprintf(label, sizeof(label), "RAM bank %d", bank);
        unsigned long long seen = 0;
        /* One complete cycle must preserve valid choices and reach capacity. */
        int steps = bank < available ? (color && !turbo ? 3 : 4) : 1;
        for (int i = 0; i < steps; i++) {
            edit(&draft, OV_GENERAL, label);
            int banks[4];
            memcpy(banks, draft.Memory.nMemoryBankSize, sizeof(banks));
            int total = Configuration_CheckMemory(banks);
            assert(!memcmp(banks, draft.Memory.nMemoryBankSize, sizeof(banks)));
            assert(total <= available * capacity);
            int size = banks[bank];
            if (bank < available) {
                assert(!(seen & (1ULL << size))); /* No repeated size within a cycle. */
                seen |= 1ULL << size;
            } else assert(size == 0);
        }
        assert(draft.Memory.nMemoryBankSize[bank] == (bank < available ? capacity : 0));
    }

    const char *normal[] = {"120 ns", "100 ns", "80 ns", "60 ns"};
    const char *fast[] = {"60 ns", "70 ns", "80 ns", "100 ns"};
    for (int i = 0; i < 4; i++) {
        assert((int)draft.Memory.nMemorySpeed == i);
        value(&draft, "Memory speed", turbo ? fast[i] : normal[i]);
        edit(&draft, OV_GENERAL, "Memory speed");
    }
    assert(draft.Memory.nMemorySpeed == MEMORY_120NS);

    draft.System.nCpuFreq = 123; /* Imported custom clocks survive display. */
    OverlayView view = {0};
    OverlayControls_AddRows(&view, OV_GENERAL, -1, &draft);
    assert(draft.System.nCpuFreq == 123);
    const int clocks[] = {16, 20, 25, 33, 40};
    for (int i = 0; i < (turbo ? 5 : 4); i++) {
        OverlayControls_CycleCpuClock(&draft);
        assert(draft.System.nCpuFreq == clocks[i]);
    }
    OverlayControls_CycleCpuClock(&draft);
    assert(draft.System.nCpuFreq == 16);

    /* A NeXTstation cannot gain Cube-only expansion hardware. */
    if (machine == NEXT_STATION) {
        edit(&draft, OV_ADVANCED, "NBIC");
        edit(&draft, OV_EXTENSIONS, "NeXTdimension");
        assert(!draft.System.bNBIC && !draft.Dimension.board[0].bEnabled);
    }
    assert(!memcmp(&ConfigureParams, &live, sizeof(live)));
}

static void test_nbic(void) {
    static CNF_PARAMS draft, before;
    draft.System.nMachineType = NEXT_CUBE040;
    before = draft;
    edit(&draft, OV_EXTENSIONS, "NeXTdimension");
    assert(draft.Dimension.board[0].bEnabled);
    edit(&draft, OV_EXTENSIONS, "NeXTdimension");
    assert(!memcmp(&draft, &before, sizeof(draft))); /* No hidden NBIC edit. */
    for (int board = 0; board < ND_MAX_BOARDS; board++) {
        memset(&draft.Dimension, 0, sizeof(draft.Dimension));
        draft.Dimension.board[board].bEnabled = true;
        before = draft;
        edit(&draft, OV_ADVANCED, "NBIC");
        assert(!memcmp(&draft, &before, sizeof(draft)));
        OverlayView view = {0};
        OverlayControls_AddRows(&view, OV_ADVANCED, -1, &draft);
        bool required = false;
        for (int i = 0; i < view.row_count; i++)
            if (!strcmp(view.rows[i].label, "NBIC"))
                required = !strcmp(view.rows[i].value, "On (required by NeXTdimension)");
        assert(required);
        ConfigureParams = draft;
        Configuration_Apply(true);
        assert(ConfigureParams.System.bNBIC); /* The core enables the required bus. */
        assert(ConfigureParams.Dimension.board[board].bEnabled);
    }
    memset(&draft.Dimension, 0, sizeof(draft.Dimension));
    draft.System.bNBIC = true;
    edit(&draft, OV_ADVANCED, "NBIC");
    assert(!draft.System.bNBIC);
}

static void dimension_edit(OverlayDevices *s, CNF_PARAMS *p, const char *label) {
    OverlayView view = {0};
    OverlayDevices_AddRows(s, &view, p);
    int row = 0;
    for (int i = 0; i < view.row_count; i++) {
        if (view.rows[i].heading) continue;
        if (!strcmp(view.rows[i].label, label)) {
            s->row = row;
            SDL_Event event; SDL_zero(event);
            event.type = SDL_EVENT_KEY_DOWN; event.key.scancode = SDL_SCANCODE_RETURN;
            OverlayDevices_Event(s, &event, p);
            return;
        }
        row++;
    }
    assert(!"missing dimension control");
}

static void test_dimension(void) {
    static CNF_PARAMS draft;
    draft.System.nMachineType = NEXT_CUBE040;
    draft.Screen.nGroupModePos[0] = 0;
    OverlayDevices s = {0}; s.page = OV_DEVICES_DIMENSION;
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        s.board = i;
        draft.Dimension.board[i].bEnabled = true;
        dimension_edit(&s, &draft, "Board RAM defaults");
        for (int bank = 0; bank < 4; bank++) {
            char label[32]; snprintf(label, sizeof(label), "RAM bank %d", bank);
            for (int j = 0; j < (bank ? 3 : 2); j++) {
                dimension_edit(&s, &draft, label);
                int banks[4]; memcpy(banks, draft.Dimension.board[i].nMemoryBankSize, sizeof(banks));
                assert(Configuration_CheckDimensionMemory(banks) >= 4);
                assert(!memcmp(banks, draft.Dimension.board[i].nMemoryBankSize, sizeof(banks)));
            }
            assert(draft.Dimension.board[i].nMemoryBankSize[bank] == 4);
        }
        draft.Screen.nGroupModePos[i + 1] = i + 1;
    }
    for (int i = 0; i < 4; i++) {
        dimension_edit(&s, &draft, "Boot console");
        dimension_edit(&s, &draft, "Shown display");
        ConfigureParams = draft; Configuration_Apply(false);
        assert(ConfigureParams.Dimension.nConsoleSlot == draft.Dimension.nConsoleSlot);
        assert(ConfigureParams.Screen.nSingleModeSlot == draft.Screen.nSingleModeSlot);
    }
    dimension_edit(&s, &draft, "Display mode"); assert(draft.Screen.nMode == SCREEN_ALL);
    ConfigureParams = draft; Configuration_Apply(false);
    assert(ConfigureParams.Screen.nMode == SCREEN_ALL);
    dimension_edit(&s, &draft, "Display mode"); assert(draft.Screen.nMode == SCREEN_GROUP);
    /* Cycling positions skips occupied cells. The real core must preserve the
     * resulting layout, including hiding a board while others remain visible. */
    for (int i = 0; i < 17; i++) {
        dimension_edit(&s, &draft, "Group slot 6");
        ConfigureParams = draft; Configuration_Apply(false);
        assert(!memcmp(&ConfigureParams.Screen, &draft.Screen, sizeof(draft.Screen)));
    }
}

int main(void) {
    test_model(NEXT_CUBE030, false, false);
    test_model(NEXT_CUBE040, false, false);
    test_model(NEXT_CUBE040, true, false);
    test_model(NEXT_STATION, false, false);
    test_model(NEXT_STATION, true, false);
    test_model(NEXT_STATION, false, true);
    test_model(NEXT_STATION, true, true);
    test_nbic();
    test_dimension();
    ConfigureParams.ConfigDialog.bShowConfigDialogAtStartup = true;
    ConfigureParams.Shortcut.withModifier[SHORTCUT_OPTIONS] = SDLK_O;
    ConfigureParams.Shortcut.withoutModifier[SHORTCUT_OPTIONS] = SDLK_F1;
    ConfigureParams.SCSI.nWriteProtection = WRITEPROT_ON;
    ConfigureParams.SCSI.target[1].nDeviceType = SD_HARDDISK;
    ConfigureParams.SCSI.target[3].nDeviceType = SD_CD;
    ConfigureParams.SCSI.target[4].nDeviceType = SD_FLOPPY;
    Configuration_Apply(false);
    assert(!ConfigureParams.ConfigDialog.bShowConfigDialogAtStartup);
    assert(!ConfigureParams.Shortcut.withModifier[SHORTCUT_OPTIONS]);
    assert(!ConfigureParams.Shortcut.withoutModifier[SHORTCUT_OPTIONS]);
    assert(ConfigureParams.SCSI.nWriteProtection == WRITEPROT_OFF);
    assert(ConfigureParams.SCSI.target[1].bWriteProtected);
    assert(ConfigureParams.SCSI.target[3].bWriteProtected);
    assert(ConfigureParams.SCSI.target[4].bWriteProtected);
    assert(!ConfigureParams.SCSI.target[0].bWriteProtected);
    /* Migration is one-shot; an explicit per-disk unprotect remains possible. */
    ConfigureParams.SCSI.target[1].bWriteProtected = false;
    Configuration_Apply(false);
    assert(!ConfigureParams.SCSI.target[1].bWriteProtected);
    /* Imported legacy preferences cannot resurrect the duplicate bar. */
    ConfigureParams.Screen.bShowStatusbar = true;
    ConfigureParams.Shortcut.withModifier[SHORTCUT_STATUSBAR] = SDLK_B;
    ConfigureParams.Shortcut.withoutModifier[SHORTCUT_STATUSBAR] = SDLK_F2;
    Configuration_Apply(false);
    assert(!ConfigureParams.Screen.bShowStatusbar);
    assert(!ConfigureParams.Shortcut.withModifier[SHORTCUT_STATUSBAR]);
    assert(!ConfigureParams.Shortcut.withoutModifier[SHORTCUT_STATUSBAR]);
    puts("test-hardware: OK");
    return 0;
}
