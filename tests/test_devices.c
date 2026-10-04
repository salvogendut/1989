/* Exercise detail-page editing and the actual overlay Save/Discard boundary.
 * The machine boundary is instrumented; no guest disk images are opened. */
#include "ui_test_support.h"
#include "overlay_devices.h"
#include "overlay_controls.h"
#include "overlay_media.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#ifdef TEST_PCAP_STUB
#include <pcap.h>
#endif

static SDL_Event event_key(SDL_Scancode key) {
    SDL_Event e; SDL_zero(e); e.type = SDL_EVENT_KEY_DOWN; e.key.scancode = key; return e;
}
static void key(SDL_Scancode k) { SDL_Event e = event_key(k); assert(overlay_handle_event(&e)); }
static void down(int n) { while (n-- > 0) key(SDL_SCANCODE_DOWN); }
static void text(const char *value) {
    SDL_Event e; SDL_zero(e); e.type = SDL_EVENT_TEXT_INPUT; e.text.text = value;
    assert(overlay_handle_event(&e));
}
static int device_row(OverlayDevices *s, CNF_PARAMS *p, const char *label) {
    OverlayView v = {0};
    OverlayDevices_AddRows(s, &v, p);
    assert(v.row_count < OVERLAY_MAX_ROWS);
    int row = 0;
    for (int i = 0; i < v.row_count; i++) {
        if (v.rows[i].heading) continue;
        if (!strcmp(v.rows[i].label, label)) return row;
        row++;
    }
    assert(!"missing device row"); return -1;
}
static int page_row(OvDevicePage page, const char *label) {
    OverlayDevices s = {0}; s.page = page;
    return device_row(&s, &ConfigureParams, label);
}
static OvDialogKind activate(OverlayDevices *s, CNF_PARAMS *p, const char *label) {
    s->row = device_row(s, p, label);
    SDL_Event e = event_key(SDL_SCANCODE_RETURN);
    return OverlayDevices_Event(s, &e, p);
}
static void page(OvDevicePage page, const char *label) {
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RIGHT);
    if (page == OV_DEVICES_DIMENSION) {
        key(SDL_SCANCODE_RIGHT);
        down((UI89Config_.bCrtEnabled ? 15 : 14) + OverlayControls_Count(OV_ADVANCED));
    } else down(OverlayControls_Count(OV_EXTENSIONS));
    key(SDL_SCANCODE_RETURN);
    down(page_row(page, label));
}
static void save(bool restart) {
    key(SDL_SCANCODE_F9); assert(overlay_confirm_visible());
    if (restart) key(SDL_SCANCODE_LEFT);
    key(SDL_SCANCODE_RETURN);
    if (overlay_is_visible()) fprintf(stderr, "save failed: restart=%d resets=%d net=%d screen=%d\n", restart, restarts, network, screen);
    assert(!overlay_is_visible());
}
static void pick(const char *path) {
    assert(OverlayMedia_Busy());
    const char *files[] = {path, NULL};
    picker_callback(picker_userdata, files, 0); overlay_tick();
}
static void no_disk_io(void) {
    for (int i = 0; i < ESP_MAX_DEVS; i++) assert(!scsi_in[i] && !scsi_out[i]);
    for (int i = 0; i < FLP_MAX_DRIVES; i++) assert(!floppy_in[i] && !floppy_out[i]);
    for (int i = 0; i < MO_MAX_DRIVES; i++) assert(!mo_in[i] && !mo_out[i]);
}

static void test_network(const char *dir, const char *rom) {
    CNF_PARAMS original = ConfigureParams;
    for (int i = 0; i < EN_MAX_SHARES; i++) {
        char label[32]; snprintf(label, sizeof(label), "Share %d folder", i);
        page(OV_DEVICES_NETWORK, label); key(SDL_SCANCODE_RETURN);
        pick(dir);
        assert(!strcmp(ConfigureParams.Ethernet.nfs[i].szPathName, ""));
        save(false);
        assert(!strcmp(ConfigureParams.Ethernet.nfs[i].szPathName, dir));
        assert(!strcmp(UI89Config_.szLastDir[OV_DIALOG_NFS0 + i], dir));
        assert(!restarts && network == i + 1);
    }
    assert(folder_requests == 4);
    page(OV_DEVICES_NETWORK, "Share 0 host name"); key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible()); /* The mandatory nfs name is not editable. */

    page(OV_DEVICES_NETWORK, "Share 1 host name"); key(SDL_SCANCODE_RETURN);
    text("bad.name"); key(SDL_SCANCODE_RETURN); /* Invalid: editor stays open. */
    key(SDL_SCANCODE_F9); assert(!overlay_confirm_visible());
    key(SDL_SCANCODE_ESCAPE); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !strcmp(ConfigureParams.Ethernet.nfs[1].szHostName, "nfs1"));
    const char *invalid[] = {"NFS", "DNS", "previous", "NFS2", "-data", "data-", "", "bad_name"};
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
        page(OV_DEVICES_NETWORK, "Share 1 host name"); key(SDL_SCANCODE_RETURN);
        text(invalid[i]); key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_ESCAPE); key(SDL_SCANCODE_F9);
        assert(!overlay_is_visible() && !strcmp(ConfigureParams.Ethernet.nfs[1].szHostName, "nfs1"));
    }
    page(OV_DEVICES_NETWORK, "Share 1 host name"); key(SDL_SCANCODE_RETURN);
    text("workspace"); key(SDL_SCANCODE_RETURN); save(false);
    assert(!strcmp(ConfigureParams.Ethernet.nfs[1].szHostName, "workspace"));
    assert(network == 5 && !restarts);
    page(OV_DEVICES_NETWORK, "Share 1 folder"); key(SDL_SCANCODE_DELETE);
    save(false); assert(!ConfigureParams.Ethernet.nfs[1].szPathName[0] && network == 6);

    /* A failed native selection and a cancelled/late callback cannot edit exports. */
    page(OV_DEVICES_NETWORK, "Share 0 folder"); key(SDL_SCANCODE_RETURN); pick(rom);
    key(SDL_SCANCODE_F9); assert(!overlay_is_visible());
    page(OV_DEVICES_NETWORK, "Share 0 folder"); key(SDL_SCANCODE_RETURN);
    overlay_quit(); pick("/tmp"); key(SDL_SCANCODE_F9);
    assert(!overlay_is_visible() && !strcmp(ConfigureParams.Ethernet.nfs[0].szPathName, dir));

    page(OV_DEVICES_NETWORK, "Cable"); key(SDL_SCANCODE_RETURN); save(false);
    assert(ConfigureParams.Ethernet.bTwistedPair && network == 7 && !restarts);
    page(OV_DEVICES_NETWORK, "Network time"); key(SDL_SCANCODE_RETURN); save(true);
    assert(ConfigureParams.Ethernet.bNetworkTime && restarts == 1);

    page(OV_DEVICES_NETWORK, "MAC source"); key(SDL_SCANCODE_RETURN); down(1); key(SDL_SCANCODE_RETURN);
    text("zz:12:34"); key(SDL_SCANCODE_RETURN);
    assert(!ConfigureParams.Rom.bUseCustomMac);
    SDL_Event select = event_key(SDL_SCANCODE_A); select.key.mod = SDL_KMOD_CTRL;
    assert(overlay_handle_event(&select)); text("12:34:ab"); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); /* Discard the MAC draft. */
    assert(!ConfigureParams.Rom.bUseCustomMac && restarts == 1);
    page(OV_DEVICES_NETWORK, "MAC source"); key(SDL_SCANCODE_RETURN); down(1); key(SDL_SCANCODE_RETURN);
    text("12:34:ab"); key(SDL_SCANCODE_RETURN); save(true);
    assert(ConfigureParams.Rom.bUseCustomMac && restarts == 2);
    assert(ConfigureParams.Rom.nRomCustomMac[3] == 0x12 && ConfigureParams.Rom.nRomCustomMac[5] == 0xab);
    for (int i = 0; i < 3; i++) assert(ConfigureParams.Rom.nRomCustomMac[i] == original.Rom.nRomCustomMac[i]);

    /* Invalid drafts still offer Discard; Save returns to editing. */
    strcpy(ConfigureParams.Ethernet.nfs[2].szHostName, "workspace");
    page(OV_DEVICES_NETWORK, "Cable"); key(SDL_SCANCODE_RETURN);
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RETURN); assert(overlay_is_visible());
    key(SDL_SCANCODE_ESCAPE); /* Dismiss validation message. */
    key(SDL_SCANCODE_F9); key(SDL_SCANCODE_RIGHT); key(SDL_SCANCODE_RETURN);
    assert(!overlay_is_visible() && ConfigureParams.Ethernet.bTwistedPair);
    strcpy(ConfigureParams.Ethernet.nfs[2].szHostName, "nfs2");

    OverlayDevices s = {0};
    OverlayDevices_Open(&s, OV_DEVICES_NETWORK, &ConfigureParams);
    assert(s.rom_mac_known && s.rom_mac[0] == 0x08 && s.rom_mac[2] == 0x07);
    ConfigureParams.System.nMachineType = NEXT_CUBE030;
    original = ConfigureParams;
    activate(&s, &ConfigureParams, "Cable");
    assert(!memcmp(&original, &ConfigureParams, sizeof(original)));
    ConfigureParams.System.nMachineType = NEXT_CUBE040;
#if !HAVE_PCAP
    original = ConfigureParams;
    activate(&s, &ConfigureParams, "Network backend");
    assert(s.message[0] && !memcmp(&original, &ConfigureParams, sizeof(original)));
#endif
    no_disk_io();
}

static void test_dimension(const char *rom) {
    int resets = restarts, net = network;
    /* All ROM pickers target the chosen board, even while disconnected. */
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        page(OV_DEVICES_DIMENSION, "Board");
        for (int n = 0; n < i; n++) key(SDL_SCANCODE_RETURN);
        down(2); key(SDL_SCANCODE_RETURN); pick(rom); save(false);
        assert(!strcmp(ConfigureParams.Dimension.board[i].szRomFileName, rom));
        assert(!ConfigureParams.Dimension.board[i].bEnabled && restarts == resets);
    }
    /* Slots 4 and 6 work without the original slot-2 quick toggle. */
    page(OV_DEVICES_DIMENSION, "Board"); key(SDL_SCANCODE_RETURN);
    down(1); key(SDL_SCANCODE_RETURN); save(true);
    assert(!ConfigureParams.Dimension.board[0].bEnabled && ConfigureParams.Dimension.board[1].bEnabled);
    assert(restarts == ++resets);
    page(OV_DEVICES_DIMENSION, "Board"); key(SDL_SCANCODE_RETURN); key(SDL_SCANCODE_RETURN);
    down(1); key(SDL_SCANCODE_RETURN); save(true);
    assert(ConfigureParams.Dimension.board[2].bEnabled && restarts == ++resets);

    page(OV_DEVICES_DIMENSION, "Shown display"); key(SDL_SCANCODE_RETURN); save(false);
    assert(ConfigureParams.Screen.nSingleModeSlot == 4 && screen == 1 && restarts == resets);
    page(OV_DEVICES_DIMENSION, "Display mode"); key(SDL_SCANCODE_RETURN); save(false);
    assert(ConfigureParams.Screen.nMode == SCREEN_ALL && screen == 2 && restarts == resets);
    page(OV_DEVICES_DIMENSION, "Group slot 4"); key(SDL_SCANCODE_RETURN); save(false);
    assert(ConfigureParams.Screen.nGroupModePos[2] == 1 && screen == 2);
    page(OV_DEVICES_DIMENSION, "Display mode"); key(SDL_SCANCODE_RETURN); save(false);
    assert(ConfigureParams.Screen.nMode == SCREEN_GROUP && screen == 3);
    page(OV_DEVICES_DIMENSION, "Group slot 4"); key(SDL_SCANCODE_RETURN); save(false);
    assert(ConfigureParams.Screen.nGroupModePos[2] == 2 && screen == 4 && restarts == resets);
    page(OV_DEVICES_DIMENSION, "Boot console"); key(SDL_SCANCODE_RETURN); save(true);
    assert(ConfigureParams.Dimension.nConsoleSlot == 4 && restarts == ++resets);
    assert(network == net); no_disk_io();

    /* Detail navigation and reversible toggles never change hidden preferences. */
    OverlayDevices s = {0};
    OverlayDevices_Open(&s, OV_DEVICES_DIMENSION, &ConfigureParams);
    CNF_PARAMS before = ConfigureParams;
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        activate(&s, &ConfigureParams, "Connected");
        activate(&s, &ConfigureParams, "Connected");
        activate(&s, &ConfigureParams, "Board");
    }
    for (int i = 0; i < 3; i++) activate(&s, &ConfigureParams, "Display mode");
    assert(!memcmp(&before, &ConfigureParams, sizeof(before)));
    ConfigureParams.Dimension.board[1].bEnabled = false;
    assert(OverlayDevices_Validate(&before, &ConfigureParams)); /* Dangling console. */
    ConfigureParams = before;
    ConfigureParams.Screen.nGroupModePos[2] = 0;
    assert(OverlayDevices_Validate(&before, &ConfigureParams)); /* Colliding displays. */
    ConfigureParams.Screen.nGroupModePos[2] = -1;
    assert(OverlayDevices_Validate(&before, &ConfigureParams)); /* Only one display. */
    ConfigureParams = before;
    ConfigureParams.System.nMachineType = NEXT_STATION;
    assert(OverlayDevices_Validate(&before, &ConfigureParams));
    /* Draft model changes can be cleaned up in the detail page itself. */
    for (int i = 0; i < ND_MAX_BOARDS; i++) {
        s.board = i;
        if (ConfigureParams.Dimension.board[i].bEnabled) activate(&s, &ConfigureParams, "Connected");
        CNF_PARAMS disconnected = ConfigureParams;
        activate(&s, &ConfigureParams, "Connected");
        assert(!memcmp(&disconnected, &ConfigureParams, sizeof(disconnected)));
    }
    activate(&s, &ConfigureParams, "Boot console");
    activate(&s, &ConfigureParams, "Shown display");
    activate(&s, &ConfigureParams, "Display mode");
    assert(!OverlayDevices_Validate(&before, &ConfigureParams));
    ConfigureParams = before;
}

static void render_pages(void) {
    SDL_Surface *surface = SDL_CreateSurface(1120, 870, SDL_PIXELFORMAT_RGBA32);
    SDL_Renderer *r = SDL_CreateSoftwareRenderer(surface); assert(r);
    for (int page_id = OV_DEVICES_NETWORK; page_id <= OV_DEVICES_DIMENSION; page_id++) {
        page(page_id, page_id == OV_DEVICES_NETWORK ? "Network backend" : "Board");
        overlay_render(r);
        SDL_RenderPresent(r);
        const char *prefix = getenv("OVERLAY_PREVIEW_PREFIX");
        if (prefix) {
            char path[FILENAME_MAX]; snprintf(path, sizeof(path), "%s%d.bmp", prefix, page_id);
            assert(SDL_SaveBMP(surface, path));
        }
        key(SDL_SCANCODE_ESCAPE); key(SDL_SCANCODE_F9); assert(!overlay_is_visible());
    }
    SDL_DestroyRenderer(r); SDL_DestroySurface(surface);
}

#ifdef TEST_PCAP_STUB
static void test_pcap(void) {
    CNF_PARAMS original = ConfigureParams, draft = original;
    OverlayDevices s = {0}; s.page = OV_DEVICES_NETWORK;
    activate(&s, &draft, "Network backend");
    assert(draft.Ethernet.nHostInterface == ENET_PCAP);
    assert(Settings_NeedRestart(&original, &draft));
    assert(OverlayDevices_Validate(&original, &draft)); /* No interface selected. */
    activate(&s, &draft, "Host interface");
    assert(!strcmp(draft.Ethernet.szInterfaceName, "test0"));
    assert(!OverlayDevices_Validate(&original, &draft));
    activate(&s, &draft, "Host interface");
    assert(!strcmp(draft.Ethernet.szInterfaceName, "test1"));
    CNF_PARAMS before = draft;
    activate(&s, &draft, "Network time");
    assert(activate(&s, &draft, "Share 2 folder") == OV_DIALOG_NONE);
    activate(&s, &draft, "Share 2 host name");
    assert(!s.editing && !memcmp(&before, &draft, sizeof(draft)));
    pcap_test_mode = 1;
    activate(&s, &draft, "Host interface");
    assert(s.message[0] && !memcmp(&before, &draft, sizeof(draft)));
    int freed = pcap_test_freed;
    pcap_test_mode = 2;
    activate(&s, &draft, "Host interface");
    assert(pcap_test_freed == freed + 1 && !memcmp(&before, &draft, sizeof(draft)));
    pcap_test_mode = 0;
    activate(&s, &draft, "Host interface");
    assert(!strcmp(draft.Ethernet.szInterfaceName, "test0"));
    assert(!Settings_NeedRestart(&before, &draft)); /* Only host interface changed. */
    /* Round-trip backend selection alone must not clear the SLiRP time flag. */
    draft = original;
    activate(&s, &draft, "Network backend"); activate(&s, &draft, "Network backend");
    assert(!memcmp(&draft, &original, sizeof(draft)));
}
#endif

int main(void) {
    char dir[] = "/tmp/1989-devices-XXXXXX"; assert(mkdtemp(dir));
    char rom[FILENAME_MAX]; snprintf(rom, sizeof(rom), "%s/rom.bin", dir);
    FILE *f = fopen(rom, "wb"); assert(f);
    const unsigned char bytes[] = {0,0,0,0,0,0,0,0,8,0,7,0x11,0x22,0x33};
    assert(fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes)); assert(!fclose(f));
    ConfigureParams.System.nMachineType = NEXT_CUBE040;
    strcpy(ConfigureParams.Rom.szRom040FileName, rom);
    for (int i = 1; i < EN_MAX_SHARES; i++) snprintf(ConfigureParams.Ethernet.nfs[i].szHostName, 64, "nfs%d", i);
    for (int i = 0; i < ND_MAX_BOARDS; i++)
        for (int j = 0; j < 4; j++) ConfigureParams.Dimension.board[i].nMemoryBankSize[j] = 4;
    for (int i = 1; i < NUM_MONITORS; i++) ConfigureParams.Screen.nGroupModePos[i] = -1;
    ConfigureParams.SCSI.target[1].nDeviceType = SD_HARDDISK;
    ConfigureParams.SCSI.target[1].bDiskInserted = true;
    strcpy(ConfigureParams.SCSI.target[1].szImageName, "system.sd");
    CNF_SCSI scsi = ConfigureParams.SCSI;
    UI89Config_.bTinker = true;
    overlay_init();
    test_network(dir, rom);
    test_dimension(rom);
#ifdef TEST_PCAP_STUB
    test_pcap();
#endif
    render_pages();
    no_disk_io(); assert(!memcmp(&scsi, &ConfigureParams.SCSI, sizeof(scsi)));
    remove(rom); rmdir(dir);
    puts("test-devices: OK");
    return 0;
}
