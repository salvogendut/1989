/* Control definitions: labels, choices and draft accessors; no runtime effects. */
#include "main.h"
#include "overlay_controls.h"
#include <stdio.h>
#include <string.h>

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

typedef struct {
    int value;
    const char *label;
} Choice;

typedef struct {
    const char *heading;
    const char *label;
    const char *hint;
    int (*get)(const CNF_PARAMS *);
    void (*set)(CNF_PARAMS *, int);
    const Choice *choices;
    int choice_count;
    OvDialogKind dialog;
    bool decimal;
    const Choice *(*choices_for)(const CNF_PARAMS *, int *count);
    const char *(*unavailable)(const CNF_PARAMS *);
} Control;

/* Typed accessors avoid treating configuration enum/float storage as ints. */
#define FIELD(name, member) \
    static int name##_get(const CNF_PARAMS *p) { return p->member; } \
    static void name##_set(CNF_PARAMS *p, int v) { p->member = v; }
#define SCALE(name, member) \
    static int name##_get(const CNF_PARAMS *p) { return (int)(p->member * 1000.0f + .5f); } \
    static void name##_set(CNF_PARAMS *p, int v) { p->member = v / 1000.0f; }

FIELD(dimension, Dimension.board[0].bEnabled)
FIELD(printer, Printer.bPrinterConnected)
FIELD(ethernet, Ethernet.bEthernetConnected)
FIELD(tablet, Tablet.nTabletType)
FIELD(microphone, Sound.bEnableMicrophone)
FIELD(paper, Printer.nPaperSize)
FIELD(format, Printer.nFileFormat)
FIELD(keymap, Keyboard.nKeymapType)
FIELD(swap, Keyboard.bSwapCmdAlt)
SCALE(linear, Mouse.fLinScale)
SCALE(exponential, Mouse.fExpScale)
FIELD(raw, Mouse.bUseRawMotion)
FIELD(ctrl_click, Mouse.bEnableMacClick)
FIELD(scroll_keys, Mouse.bEnableMapToKey)
FIELD(auto_grab, Mouse.bEnableAutoGrab)
FIELD(pot, Boot.bEnablePot)
FIELD(dram_test, Boot.bEnableDRAMTest)
FIELD(sound_test, Boot.bEnableSoundTest)
FIELD(scsi_test, Boot.bEnableSCSITest)
FIELD(loop_test, Boot.bLoopPot)
FIELD(extended_test, Boot.bExtendedPot)
FIELD(visible_test, Boot.bVisible)
FIELD(verbose, Boot.bVerbose)
FIELD(variable_clock, System.bRealtime)
FIELD(bank0, Memory.nMemoryBankSize[0])
FIELD(bank1, Memory.nMemoryBankSize[1])
FIELD(bank2, Memory.nMemoryBankSize[2])
FIELD(bank3, Memory.nMemoryBankSize[3])
FIELD(memory_speed, Memory.nMemorySpeed)
FIELD(dsp_memory, System.bDSPMemoryExpansion)
FIELD(scsi_chip, System.nSCSI)
FIELD(rtc_chip, System.nRTC)
FIELD(nbic, System.bNBIC)

static const Choice toggle[] = {{false, "Off"}, {true, "On"}};
static const Choice papers[] = {
    {PAPER_A4, "A4"}, {PAPER_LETTER, "Letter"}, {PAPER_B5, "B5"}, {PAPER_LEGAL, "Legal"}
};
static const Choice formats[] = {
#if HAVE_LIBPNG
    {FORMAT_PNG, "PNG"},
#endif
    {FORMAT_TIFF, "TIFF"}
};
static const Choice keymaps[] = {{KEYMAP_SYMBOLIC, "Symbolic"}, {KEYMAP_SCANCODE, "Scancode"}};
static const Choice tablets[] = {
    {TABLET_NONE, "None"}, {TABLET_MM961, "Summagraphics MM961"},
    {TABLET_MM1201, "Summagraphics MM1201"}, {TABLET_SD210L, "Wacom SD-210L"},
    {TABLET_SD310E, "Wacom SD-310E"}, {TABLET_SD320E, "Wacom SD-320E"},
    {TABLET_SD420E, "Wacom SD-420E"}, {TABLET_SD510C, "Wacom SD-510C"}
};
/* Match the five presets in Previous's mouse dialog; imported custom values
 * remain unchanged until this control is deliberately edited. */
static const Choice linear_speeds[] = {
    {750, "Very slow (0.750)"}, {875, "Slow (0.875)"}, {1000, "Normal (1.000)"},
    {1125, "Fast (1.125)"}, {1250, "Very fast (1.250)"}
};
static const Choice exponential_speeds[] = {
    {500, "Very slow (0.500)"}, {625, "Slow (0.625)"}, {750, "Normal (0.750)"},
    {875, "Fast (0.875)"}, {1000, "Very fast (1.000)"}
};

/* These sizes are already valid for Configuration_CheckMemory; opening the
 * overlay never normalizes the user's configuration or writes to live state. */
static const Choice monochrome_banks[] = {{0, "Empty"}, {1, "1 MB"}, {4, "4 MB"}, {16, "16 MB"}};
static const Choice color_banks[] = {{0, "Empty"}, {2, "2 MB"}, {8, "8 MB"}};
static const Choice turbo_banks[] = {{0, "Empty"}, {2, "2 MB"}, {8, "8 MB"}, {32, "32 MB"}};
static const Choice memory_speeds[] = {
    {MEMORY_120NS, "120 ns"}, {MEMORY_100NS, "100 ns"}, {MEMORY_80NS, "80 ns"}, {MEMORY_60NS, "60 ns"}
};
/* The original enum names predate Turbo: keep the stored values while
 * displaying the timing encoded by the Turbo memory controller. */
static const Choice turbo_speeds[] = {
    {MEMORY_120NS, "60 ns"}, {MEMORY_100NS, "70 ns"}, {MEMORY_80NS, "80 ns"}, {MEMORY_60NS, "100 ns"}
};
static const Choice dsp_sizes[] = {{false, "24 KB"}, {true, "96 KB"}};
static const Choice scsi_chips[] = {{NCR53C90, "NCR53C90"}, {NCR53C90A, "NCR53C90A"}};
static const Choice rtc_chips[] = {{MC68HC68T1, "MC68HC68T1"}, {MCCS1850, "MCCS1850"}};

static const Choice *bank_choices(const CNF_PARAMS *p, int *count) {
    if (p->System.bTurbo) { *count = COUNT(turbo_banks); return turbo_banks; }
    if (p->System.bColor) { *count = COUNT(color_banks); return color_banks; }
    *count = COUNT(monochrome_banks);
    return monochrome_banks;
}
static const Choice *speed_choices(const CNF_PARAMS *p, int *count) {
    *count = COUNT(memory_speeds);
    return p->System.bTurbo ? turbo_speeds : memory_speeds;
}
static const char *upper_banks_unavailable(const CNF_PARAMS *p) {
    return p->System.nMachineType == NEXT_STATION && !p->System.bColor && !p->System.bTurbo
        ? "Not fitted on monochrome NeXTstation" : NULL;
}
static const char *dimension_unavailable(const CNF_PARAMS *p) {
    return p->System.nMachineType == NEXT_STATION ? "Cubes only" : NULL;
}
static const char *nbic_unavailable(const CNF_PARAMS *p) {
    if (p->System.nMachineType == NEXT_STATION) return "Cubes only";
    /* Show the effective bus state. Configuration_Apply enables NBIC when
     * a board is committed; do not change the independent draft preference,
     * so enabling then disabling a board remains a no-op. */
    for (int i = 0; i < ND_MAX_BOARDS; i++)
        if (p->Dimension.board[i].bEnabled) return "On (required by NeXTdimension)";
    return NULL;
}

void OverlayControls_CycleCpuClock(CNF_PARAMS *draft) {
    static const int clocks[] = {16, 20, 25, 33, 40};
    int count = draft->System.bTurbo ? COUNT(clocks) : COUNT(clocks) - 1;
    int next = 0;
    for (int i = 0; i < count; i++)
        if (draft->System.nCpuFreq == clocks[i]) { next = (i + 1) % count; break; }
    draft->System.nCpuFreq = clocks[next];
}

#define OPTION(group_, label_, hint_, name_, choices_) \
    {.heading = group_, .label = label_, .hint = hint_, .get = name_##_get, .set = name_##_set, \
     .choices = choices_, .choice_count = COUNT(choices_)}
#define SPEED(label_, name_, choices_) \
    {.label = label_, .hint = "Enter cycles motion presets. Custom numeric scales remain available in F1.", \
     .get = name_##_get, .set = name_##_set, .choices = choices_, \
     .choice_count = COUNT(choices_), .decimal = true}
#define BANK(group_, label_, name_, unavailable_) \
    {.heading = group_, .label = label_, .hint = hardware_hint, \
     .get = name_##_get, .set = name_##_set, .choices_for = bank_choices, .unavailable = unavailable_}
#define BOOT(label, name) OPTION(NULL, label, boot_hint, name, toggle)

static const char boot_hint[] = "Saved for the next boot; the running guest is not restarted.";
static const char hardware_hint[] = "Hardware changes require restart confirmation. Discard leaves the guest running.";
static const char live_hint[] = "Changes apply on Save without restarting the machine.";
static const Control boot[] = {
    OPTION("Next-boot diagnostics", "Power-on tests", boot_hint, pot, toggle),
    BOOT("DRAM test", dram_test),
    BOOT("Sound test", sound_test),
    BOOT("SCSI test", scsi_test),
    BOOT("Repeat tests", loop_test),
    BOOT("Extended tests", extended_test),
    BOOT("Diagnostic video", visible_test),
    BOOT("Verbose boot", verbose)
};
static const Control extensions[] = {
    {.heading = "Connections", .label = "NeXTdimension", .hint = hardware_hint,
     .get = dimension_get, .set = dimension_set, .choices = toggle, .choice_count = COUNT(toggle),
     .unavailable = dimension_unavailable},
    OPTION(NULL, "Printer", live_hint, printer, toggle),
    OPTION(NULL, "Ethernet", live_hint, ethernet, toggle),
    OPTION(NULL, "Tablet", "Disable or uninstall the tablet in NeXT before changing its model.", tablet, tablets),
    OPTION(NULL, "Microphone", live_hint, microphone, toggle),
    OPTION("Printer output", "Paper size", live_hint, paper, papers),
    OPTION(NULL, "Image format", "Also selects legacy screenshots; F4/F6 keep PPM/GIF. TIFF works without libpng.", format, formats),
    {.label = "Output directory", .hint = "Enter chooses a folder. The selection applies only on Save.",
     .dialog = OV_DIALOG_PRINTER_DIR},
    OPTION("Keyboard and mouse", "Keyboard mapping", live_hint, keymap, keymaps),
    OPTION(NULL, "Swap Command / Alt", live_hint, swap, toggle),
    SPEED("Mouse slow motion", linear, linear_speeds),
    SPEED("Mouse fast motion", exponential, exponential_speeds),
    OPTION(NULL, "Raw mouse motion", live_hint, raw, toggle),
    OPTION(NULL, "Ctrl-click -> right", live_hint, ctrl_click, toggle),
    OPTION(NULL, "Wheel -> arrow keys", live_hint, scroll_keys, toggle),
    OPTION(NULL, "Automatic mouse grab", "Ctrl+Enter releases the mouse. Settings apply only on Save.", auto_grab, toggle)
};

static const Control general[] = {
    OPTION("CPU timing", "Variable CPU clock", hardware_hint, variable_clock, toggle),
    BANK("Memory banks", "RAM bank 0", bank0, NULL),
    BANK(NULL, "RAM bank 1", bank1, NULL),
    BANK(NULL, "RAM bank 2", bank2, upper_banks_unavailable),
    BANK(NULL, "RAM bank 3", bank3, upper_banks_unavailable),
    {.label = "Memory speed", .hint = hardware_hint, .get = memory_speed_get,
     .set = memory_speed_set, .choices_for = speed_choices}
};
static const Control advanced[] = {
    OPTION("Machine hardware", "DSP memory", hardware_hint, dsp_memory, dsp_sizes),
    OPTION(NULL, "SCSI controller", hardware_hint, scsi_chip, scsi_chips),
    OPTION(NULL, "RTC chip", hardware_hint, rtc_chip, rtc_chips),
    {.label = "NBIC", .hint = hardware_hint, .get = nbic_get, .set = nbic_set,
     .choices = toggle, .choice_count = COUNT(toggle), .unavailable = nbic_unavailable}
};

static const Control *controls(OvSection section, int *count) {
    if (section == OV_GENERAL) { *count = COUNT(general); return general; }
    if (section == OV_ADVANCED) { *count = COUNT(advanced); return advanced; }
    if (section == OV_MEDIA) { *count = COUNT(boot); return boot; }
    if (section == OV_EXTENSIONS) { *count = COUNT(extensions); return extensions; }
    *count = 0;
    return NULL;
}

int OverlayControls_Count(OvSection section) {
    int count;
    controls(section, &count);
    return count;
}

static const Choice *control_choices(const Control *c, const CNF_PARAMS *p, int *count) {
    if (c->choices_for) return c->choices_for(p, count);
    *count = c->choice_count;
    return c->choices;
}

void OverlayControls_AddRows(OverlayView *view, OvSection section,
                            int selected, const CNF_PARAMS *draft) {
    int count;
    const Control *list = controls(section, &count);
    for (int i = 0; i < count; i++) {
        const Control *c = &list[i];
        char value[FILENAME_MAX];
        if (c->heading) OverlayView_Heading(view, c->heading);
        const char *disabled = c->unavailable ? c->unavailable(draft) : NULL;
        if (disabled) {
            snprintf(value, sizeof(value), "%s", disabled);
        } else if (c->dialog == OV_DIALOG_PRINTER_DIR) {
            snprintf(value, sizeof(value), "%s", draft->Printer.szPrintToFileName);
        } else {
            int current = c->get(draft);
            if (c->decimal) snprintf(value, sizeof(value), "Custom (%.3f)", current / 1000.0);
            else snprintf(value, sizeof(value), "Custom (%d)", current);
            int choice_count;
            const Choice *choices = control_choices(c, draft, &choice_count);
            for (int j = 0; j < choice_count; j++) {
                if (current == choices[j].value) {
                    snprintf(value, sizeof(value), "%s", choices[j].label);
                    break;
                }
            }
#if !HAVE_LIBPNG
            if (c->get == format_get) snprintf(value, sizeof(value), "TIFF (PNG unavailable)");
#endif
        }
        OverlayView_Add(view, c->label, value, selected == i);
        if (selected == i) view->hint = c->hint;
    }
}

OvDialogKind OverlayControls_Activate(OvSection section, int row, CNF_PARAMS *draft) {
    int count;
    const Control *list = controls(section, &count);
    if (row < 0 || row >= count) return OV_DIALOG_NONE;
    const Control *c = &list[row];
    if (c->unavailable && c->unavailable(draft)) return OV_DIALOG_NONE;
    if (c->dialog != OV_DIALOG_NONE) return c->dialog;
    int choice_count;
    const Choice *choices = control_choices(c, draft, &choice_count);
    int current = c->get(draft), next = 0;
    for (int i = 0; i < choice_count; i++) {
        if (current == choices[i].value) { next = (i + 1) % choice_count; break; }
    }
    c->set(draft, choices[next].value);
    return OV_DIALOG_NONE;
}
