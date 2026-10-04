/* Verify overlay choices against the real core's hardware normalization. */
#include "main.h"
#include "overlay_controls.h"
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

int main(void) {
    test_model(NEXT_CUBE030, false, false);
    test_model(NEXT_CUBE040, false, false);
    test_model(NEXT_CUBE040, true, false);
    test_model(NEXT_STATION, false, false);
    test_model(NEXT_STATION, true, false);
    test_model(NEXT_STATION, false, true);
    test_model(NEXT_STATION, true, true);
    test_nbic();
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
