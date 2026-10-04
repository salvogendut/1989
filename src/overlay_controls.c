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

#define OPTION(group, label, hint, name, choices) \
    {group, label, hint, name##_get, name##_set, choices, COUNT(choices), OV_DIALOG_NONE, false}
#define SPEED(label, name, choices) \
    {NULL, label, "Enter cycles motion presets. Custom numeric scales remain available in F1.", \
     name##_get, name##_set, choices, COUNT(choices), OV_DIALOG_NONE, true}
#define BOOT(label, name) OPTION(NULL, label, boot_hint, name, toggle)

static const char boot_hint[] = "Saved for the next boot; the running guest is not restarted.";
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
    OPTION("Connections", "NeXTdimension", "Changing graphics hardware requires restart confirmation.", dimension, toggle),
    OPTION(NULL, "Printer", live_hint, printer, toggle),
    OPTION(NULL, "Ethernet", live_hint, ethernet, toggle),
    OPTION(NULL, "Tablet", "Disable or uninstall the tablet in NeXT before changing its model.", tablet, tablets),
    OPTION(NULL, "Microphone", live_hint, microphone, toggle),
    OPTION("Printer output", "Paper size", live_hint, paper, papers),
    OPTION(NULL, "Image format", "Also selects legacy screenshots; F4/F6 keep PPM/GIF. TIFF works without libpng.", format, formats),
    {NULL, "Output directory", "Enter chooses a folder. The selection applies only on Save.",
     NULL, NULL, NULL, 0, OV_DIALOG_PRINTER_DIR, false},
    OPTION("Keyboard and mouse", "Keyboard mapping", live_hint, keymap, keymaps),
    OPTION(NULL, "Swap Command / Alt", live_hint, swap, toggle),
    SPEED("Mouse slow motion", linear, linear_speeds),
    SPEED("Mouse fast motion", exponential, exponential_speeds),
    OPTION(NULL, "Raw mouse motion", live_hint, raw, toggle),
    OPTION(NULL, "Ctrl-click -> right", live_hint, ctrl_click, toggle),
    OPTION(NULL, "Wheel -> arrow keys", live_hint, scroll_keys, toggle),
    OPTION(NULL, "Automatic mouse grab", "Ctrl+Enter releases the mouse. Settings apply only on Save.", auto_grab, toggle)
};

static const Control *controls(OvSection section, int *count) {
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

void OverlayControls_AddRows(OverlayView *view, OvSection section,
                            int selected, const CNF_PARAMS *draft) {
    int count;
    const Control *list = controls(section, &count);
    for (int i = 0; i < count; i++) {
        const Control *c = &list[i];
        char value[FILENAME_MAX];
        if (c->heading) OverlayView_Heading(view, c->heading);
        if (c->dialog == OV_DIALOG_PRINTER_DIR) {
            snprintf(value, sizeof(value), "%s", draft->Printer.szPrintToFileName);
        } else {
            int current = c->get(draft);
            if (c->decimal) snprintf(value, sizeof(value), "Custom (%.3f)", current / 1000.0);
            else snprintf(value, sizeof(value), "Custom (%d)", current);
            for (int j = 0; j < c->choice_count; j++) {
                if (current == c->choices[j].value) {
                    snprintf(value, sizeof(value), "%s", c->choices[j].label);
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
    if (c->dialog != OV_DIALOG_NONE) return c->dialog;
    int current = c->get(draft), next = 0;
    for (int i = 0; i < c->choice_count; i++) {
        if (current == c->choices[i].value) { next = (i + 1) % c->choice_count; break; }
    }
    c->set(draft, c->choices[next].value);
    return OV_DIALOG_NONE;
}
