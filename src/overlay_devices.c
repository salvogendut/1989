/* Networking and NeXTdimension detail pages. Runtime application remains in
 * settings/change; native selections use the existing cancellable picker. */
#include "main.h"
#include "overlay_devices.h"
#include "overlay_media.h"
#include "sdlscreen.h"
#include "file.h"
#include "slirp/ctl.h"
#if HAVE_PCAP
#include <pcap.h>
#endif

enum { NET_BACKEND, NET_INTERFACE, NET_CABLE, NET_TIME, NET_MAC_SOURCE,
       NET_MAC, NET_SHARE0, NET_BACK = NET_SHARE0 + 2 * EN_MAX_SHARES };
enum { ND_BOARD, ND_CONNECTED, ND_ROM, ND_BANK0, ND_BANK1, ND_BANK2, ND_BANK3,
       ND_DEFAULTS, ND_CONSOLE, ND_MODE, ND_SHOWN, ND_POSITION0,
       ND_BACK = ND_POSITION0 + NUM_MONITORS };

static bool monitor_present(const CNF_PARAMS *p, int slot) {
    return slot == 0 || (slot >= 2 && slot <= 6 && !(slot & 1) &&
        p->System.nMachineType != NEXT_STATION && p->Dimension.board[slot / 2 - 1].bEnabled);
}

static int next_slot(const CNF_PARAMS *p, int slot) {
    for (int n = 0; n < NUM_MONITORS; n++) {
        slot = (slot + 2) & 6;
        if (monitor_present(p, slot)) return slot;
    }
    return 0;
}

static const char *slot_name(int slot) {
    return slot == 0 ? "Main display" : slot == 2 ? "NeXTdimension slot 2" :
           slot == 4 ? "NeXTdimension slot 4" : slot == 6 ? "NeXTdimension slot 6" : "Invalid slot";
}

void OverlayDevices_Close(OverlayDevices *s) {
    if (s->editing && sdlWindow) SDL_StopTextInput(sdlWindow);
    memset(s, 0, sizeof(*s));
}

void OverlayDevices_Open(OverlayDevices *s, OvDevicePage page, const CNF_PARAMS *p) {
    OverlayDevices_Close(s);
    s->page = page;
    if (page != OV_DEVICES_NETWORK) return;
    const char *path = p->System.nMachineType == NEXT_CUBE030 ? p->Rom.szRom030FileName :
                       p->System.bTurbo ? p->Rom.szRomTurboFileName : p->Rom.szRom040FileName;
    FILE *rom = fopen(path, "rb");
    if (rom) {
        s->rom_mac_known = fseek(rom, 8, SEEK_SET) == 0 && fread(s->rom_mac, 1, 6, rom) == 6;
        fclose(rom);
    }
}

/* Names become DNS labels in SLiRP's .home domain. Its built-in host/DNS/NFS
 * names must remain unambiguous, regardless of letter case. */
static bool ascii_alnum(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

static const char *name_error(const CNF_PARAMS *p, int share, const char *name) {
    size_t len = strlen(name);
    if (!len || len > 63 || !ascii_alnum(name[0]) || !ascii_alnum(name[len - 1]))
        return "Use 1-63 letters/digits, with hyphens only inside the name.";
    for (size_t i = 0; i < len; i++)
        if (!ascii_alnum(name[i]) && name[i] != '-')
            return "Use a single host name, without spaces, dots or underscores.";
    if (!SDL_strcasecmp(name, NAME_HOST) || !SDL_strcasecmp(name, NAME_DNS) ||
        !SDL_strcasecmp(name, NAME_NFSD))
        return "That name is reserved by SLiRP. Choose a different NFS name.";
    for (int i = 1; i < EN_MAX_SHARES; i++)
        if (i != share && !SDL_strcasecmp(name, p->Ethernet.nfs[i].szHostName))
            return "Each NFS share needs a different host name.";
    return NULL;
}

static void edit_begin(OverlayDevices *s, const char *text) {
    snprintf(s->text, sizeof(s->text), "%s", text);
    s->editing = s->replace_text = true;
    s->message[0] = 0;
    if (sdlWindow) SDL_StartTextInput(sdlWindow);
}

static void edit_end(OverlayDevices *s) {
    s->editing = false;
    s->message[0] = 0;
    if (sdlWindow) SDL_StopTextInput(sdlWindow);
}

static int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void edit_accept(OverlayDevices *s, CNF_PARAMS *p) {
    if (s->row == NET_MAC) {
        int bytes[3];
        bool valid = strlen(s->text) == 8 && s->text[2] == ':' && s->text[5] == ':';
        for (int i = 0; valid && i < 3; i++) {
            int hi = hex_value(s->text[i * 3]), lo = hex_value(s->text[i * 3 + 1]);
            valid = hi >= 0 && lo >= 0;
            bytes[i] = hi * 16 + lo;
        }
        if (!valid) {
            snprintf(s->message, sizeof(s->message), "Enter three hexadecimal bytes, for example 12:34:56.");
            return;
        }
        /* Previous preserves the manufacturer prefix stored in the ROM. */
        for (int i = 0; i < 3; i++) p->Rom.nRomCustomMac[i + 3] = bytes[i];
    } else {
        int share = (s->row - NET_SHARE0) / 2;
        const char *error = name_error(p, share, s->text);
        if (error) { snprintf(s->message, sizeof(s->message), "%s", error); return; }
        snprintf(p->Ethernet.nfs[share].szHostName, sizeof(p->Ethernet.nfs[share].szHostName), "%s", s->text);
    }
    edit_end(s);
}

static void edit_append(OverlayDevices *s, const char *text) {
    size_t len = s->replace_text ? 0 : strlen(s->text);
    size_t extra = strlen(text);
    if (len + extra >= sizeof(s->text)) {
        snprintf(s->message, sizeof(s->message), "Maximum length is 63 characters.");
        return;
    }
    /* Reject unsupported input intact; never turn a pasted invalid name into
     * a different valid one by silently dropping characters. */
    for (size_t i = 0; i < extra; i++)
        if ((unsigned char)text[i] < 32 || (unsigned char)text[i] > 126) {
            snprintf(s->message, sizeof(s->message), "Use ASCII letters, numbers and punctuation.");
            return;
        }
    memcpy(s->text + len, text, extra + 1);
    s->replace_text = false;
    s->message[0] = 0;
}

static void edit_event(OverlayDevices *s, const SDL_Event *event, CNF_PARAMS *p) {
    if (event->type == SDL_EVENT_TEXT_INPUT) { edit_append(s, event->text.text); return; }
    if (event->type != SDL_EVENT_KEY_DOWN) return;
    SDL_Scancode key = event->key.scancode;
    if (key == SDL_SCANCODE_ESCAPE) edit_end(s);
    else if (key == SDL_SCANCODE_RETURN) edit_accept(s, p);
    else if ((event->key.mod & SDL_KMOD_CTRL) && key == SDL_SCANCODE_A) s->replace_text = true;
    else if ((event->key.mod & SDL_KMOD_CTRL) && key == SDL_SCANCODE_V) {
        char *text = SDL_GetClipboardText();
        if (text) { edit_append(s, text); SDL_free(text); }
    } else if (key == SDL_SCANCODE_BACKSPACE || key == SDL_SCANCODE_DELETE) {
        size_t len = strlen(s->text);
        if (s->replace_text) s->text[0] = 0;
        else if (len) s->text[len - 1] = 0;
        s->replace_text = false;
        s->message[0] = 0;
    }
}

static void next_interface(OverlayDevices *s, CNF_PARAMS *p) {
#if HAVE_PCAP
    pcap_if_t *devices = NULL;
    char error[PCAP_ERRBUF_SIZE] = "";
    if (pcap_findalldevs(&devices, error) || !devices) {
        snprintf(s->message, sizeof(s->message), "No PCAP interfaces available; check capture permissions.");
    } else {
        pcap_if_t *next = devices;
        for (pcap_if_t *dev = devices; dev; dev = dev->next)
            if (dev->name && !strcmp(p->Ethernet.szInterfaceName, dev->name)) {
                next = dev->next ? dev->next : devices;
                break;
            }
        if (next->name) snprintf(p->Ethernet.szInterfaceName, FILENAME_MAX, "%s", next->name);
    }
    if (devices) pcap_freealldevs(devices);
#else
    (void)p;
    snprintf(s->message, sizeof(s->message), "PCAP support is not included in this build.");
#endif
}

static OvDialogKind network_activate(OverlayDevices *s, CNF_PARAMS *p) {
    bool slirp = p->Ethernet.nHostInterface == ENET_SLIRP;
    switch (s->row) {
        case NET_BACKEND:
#if HAVE_PCAP
            p->Ethernet.nHostInterface = slirp ? ENET_PCAP : ENET_SLIRP;
#else
            if (!slirp) p->Ethernet.nHostInterface = ENET_SLIRP;
            else snprintf(s->message, sizeof(s->message), "PCAP support is not included in this build.");
#endif
            break;
        case NET_INTERFACE: if (!slirp) next_interface(s, p); break;
        case NET_CABLE:
            if (p->System.nMachineType != NEXT_CUBE030) p->Ethernet.bTwistedPair = !p->Ethernet.bTwistedPair;
            break;
        case NET_TIME: if (slirp) p->Ethernet.bNetworkTime = !p->Ethernet.bNetworkTime; break;
        case NET_MAC_SOURCE: p->Rom.bUseCustomMac = !p->Rom.bUseCustomMac; break;
        case NET_MAC:
            if (p->Rom.bUseCustomMac) {
                char suffix[16];
                snprintf(suffix, sizeof(suffix), "%02x:%02x:%02x", p->Rom.nRomCustomMac[3] & 255,
                         p->Rom.nRomCustomMac[4] & 255, p->Rom.nRomCustomMac[5] & 255);
                edit_begin(s, suffix);
            }
            break;
        default:
            if (slirp && s->row >= NET_SHARE0 && s->row < NET_BACK) {
                int share = (s->row - NET_SHARE0) / 2;
                if (!((s->row - NET_SHARE0) & 1)) return OV_DIALOG_NFS0 + share;
                if (share) edit_begin(s, p->Ethernet.nfs[share].szHostName);
            }
            break;
    }
    return OV_DIALOG_NONE;
}

static void next_position(CNF_PARAMS *p, int monitor) {
    int pos = p->Screen.nGroupModePos[monitor];
    if (pos < -1 || pos >= NUM_MONITORS * NUM_MONITORS) pos = -1;
    for (int n = 0; n <= NUM_MONITORS * NUM_MONITORS; n++) {
        if (++pos == NUM_MONITORS * NUM_MONITORS) pos = -1;
        bool occupied = false;
        for (int i = 0; i < NUM_MONITORS; i++)
            if (pos >= 0 && i != monitor && monitor_present(p, 2 * i) && p->Screen.nGroupModePos[i] == pos)
                occupied = true;
        if (!occupied) { p->Screen.nGroupModePos[monitor] = pos; return; }
    }
}

static OvDialogKind dimension_activate(OverlayDevices *s, CNF_PARAMS *p) {
    if (s->row == ND_BOARD) { s->board = (s->board + 1) % ND_MAX_BOARDS; return OV_DIALOG_NONE; }
    NDBOARD *board = &p->Dimension.board[s->board];
    if (p->System.nMachineType == NEXT_STATION) {
        /* Permit cleanup after a draft model change, without silently changing
         * these preferences when the user cycles away from a Cube and back. */
        if (s->row == ND_CONNECTED && board->bEnabled) board->bEnabled = false;
        else if (s->row == ND_CONSOLE) p->Dimension.nConsoleSlot = 0;
        else if (s->row == ND_SHOWN) p->Screen.nSingleModeSlot = 0;
        else if (s->row == ND_MODE) p->Screen.nMode = SCREEN_SINGLE;
        snprintf(s->message, sizeof(s->message), "NeXTdimension requires a NeXT Computer or NeXTcube.");
        return OV_DIALOG_NONE;
    }
    switch (s->row) {
        case ND_CONNECTED: board->bEnabled = !board->bEnabled; break;
        case ND_ROM: return OV_DIALOG_NDROM0 + s->board;
        case ND_DEFAULTS:
            for (int i = 0; i < 4; i++) board->nMemoryBankSize[i] = 4;
            break;
        case ND_CONSOLE: p->Dimension.nConsoleSlot = next_slot(p, p->Dimension.nConsoleSlot); break;
        case ND_MODE:
            p->Screen.nMode = p->Screen.nMode == SCREEN_SINGLE ? SCREEN_ALL :
                              p->Screen.nMode == SCREEN_ALL ? SCREEN_GROUP : SCREEN_SINGLE;
            break;
        case ND_SHOWN:
            if (p->Screen.nMode == SCREEN_SINGLE) p->Screen.nSingleModeSlot = next_slot(p, p->Screen.nSingleModeSlot);
            break;
        default:
            if (s->row >= ND_BANK0 && s->row <= ND_BANK3) {
                int bank = s->row - ND_BANK0;
                int *size = &board->nMemoryBankSize[bank];
                *size = *size == 4 ? 16 : *size == 16 && bank ? 0 : 4;
            } else if (s->row >= ND_POSITION0 && s->row < ND_BACK) {
                int monitor = s->row - ND_POSITION0;
                if (monitor_present(p, 2 * monitor)) next_position(p, monitor);
            }
            break;
    }
    return OV_DIALOG_NONE;
}

OvDialogKind OverlayDevices_Event(OverlayDevices *s, const SDL_Event *event, CNF_PARAMS *p) {
    if (s->editing) { edit_event(s, event, p); return OV_DIALOG_NONE; }
    if (event->type != SDL_EVENT_KEY_DOWN || s->page == OV_DEVICES_NONE) return OV_DIALOG_NONE;
    SDL_Scancode key = event->key.scancode;
    int back = s->page == OV_DEVICES_NETWORK ? NET_BACK : ND_BACK;
    if (key == SDL_SCANCODE_ESCAPE || (key == SDL_SCANCODE_RETURN && s->row == back)) {
        OverlayDevices_Close(s);
        return OV_DIALOG_NONE;
    }
    s->message[0] = 0;
    if (key == SDL_SCANCODE_UP && s->row > 0) s->row--;
    else if (key == SDL_SCANCODE_DOWN && s->row < back) s->row++;
    else if (key == SDL_SCANCODE_RETURN)
        return s->page == OV_DEVICES_NETWORK ? network_activate(s, p) : dimension_activate(s, p);
    else if (key == SDL_SCANCODE_DELETE && s->page == OV_DEVICES_NETWORK &&
             p->Ethernet.nHostInterface == ENET_SLIRP && s->row >= NET_SHARE0 && s->row < NET_BACK &&
             !((s->row - NET_SHARE0) & 1))
        OverlayMedia_Set(p, OV_DIALOG_NFS0 + (s->row - NET_SHARE0) / 2, NULL);
    return OV_DIALOG_NONE;
}

static void network_rows(const OverlayDevices *s, OverlayView *v, const CNF_PARAMS *p) {
    bool slirp = p->Ethernet.nHostInterface == ENET_SLIRP;
    int row = 0;
#define ADD(label, value) OverlayView_Add(v, label, value, s->row == row++)
    OverlayView_Heading(v, "Networking / NFS");
#if HAVE_PCAP
    ADD("Network backend", slirp ? "SLiRP (shared networking)" : "PCAP (host interface)");
#else
    ADD("Network backend", slirp ? "SLiRP (PCAP unavailable in this build)" : "PCAP unavailable - Enter selects SLiRP");
#endif
    ADD("Host interface", slirp ? "PCAP only" : p->Ethernet.szInterfaceName[0] ? p->Ethernet.szInterfaceName : "Enter to choose");
    ADD("Cable", p->System.nMachineType == NEXT_CUBE030 ? "Thinwire (68030 Cube)" : p->Ethernet.bTwistedPair ? "Twisted pair" : "Thinwire");
    ADD("Network time", !slirp ? "SLiRP only" : p->Ethernet.bNetworkTime ? "On" : "Off");
    ADD("MAC source", p->Rom.bUseCustomMac ? "Custom suffix (ROM prefix retained)" : "ROM default");
    char mac[64], prefix[16];
    if (s->rom_mac_known) snprintf(prefix, sizeof(prefix), "%02x:%02x:%02x", s->rom_mac[0], s->rom_mac[1], s->rom_mac[2]);
    else snprintf(prefix, sizeof(prefix), "??:??:??");
    if (p->Rom.bUseCustomMac) snprintf(mac, sizeof(mac), "%s:%02x:%02x:%02x", prefix,
        p->Rom.nRomCustomMac[3] & 255, p->Rom.nRomCustomMac[4] & 255, p->Rom.nRomCustomMac[5] & 255);
    else if (s->rom_mac_known) snprintf(mac, sizeof(mac), "%s:%02x:%02x:%02x", prefix, s->rom_mac[3], s->rom_mac[4], s->rom_mac[5]);
    else snprintf(mac, sizeof(mac), "ROM unavailable");
    ADD("MAC address", mac);
    OverlayView_Heading(v, "NFS exports (SLiRP only)");
    for (int i = 0; i < EN_MAX_SHARES; i++) {
        char label[32];
        snprintf(label, sizeof(label), "Share %d folder", i);
        ADD(label, !slirp ? "SLiRP only" : p->Ethernet.nfs[i].szPathName[0] ? p->Ethernet.nfs[i].szPathName : "Not exported");
        snprintf(label, sizeof(label), "Share %d host name", i);
        ADD(label, i ? p->Ethernet.nfs[i].szHostName : "nfs (fixed)");
    }
    ADD("Back", "Extensions");
    v->hint = "Unmount guest NFS shares before editing exports. Del on a folder stops exporting it.";
#undef ADD
}

static void dimension_rows(const OverlayDevices *s, OverlayView *v, const CNF_PARAMS *p) {
    const NDBOARD *board = &p->Dimension.board[s->board];
    bool station = p->System.nMachineType == NEXT_STATION;
    int row = 0, total = 0;
    char value[64];
#define ADD(label, text) OverlayView_Add(v, label, text, s->row == row++)
    OverlayView_Heading(v, "NeXTdimension hardware");
    ADD("Board", slot_name(2 * (s->board + 1)));
    ADD("Connected", station ? (board->bEnabled ? "On - Enter disconnects before model change" : "Cube models only") : board->bEnabled ? "On (NBIC required)" : "Off");
    ADD("Board ROM", board->szRomFileName[0] ? board->szRomFileName : "Choose a ROM image");
    for (int i = 0; i < 4; i++) {
        char label[32];
        total += board->nMemoryBankSize[i];
        snprintf(label, sizeof(label), "RAM bank %d", i);
        snprintf(value, sizeof(value), "%d MB%s", board->nMemoryBankSize[i], i ? "" : " (4 MB minimum)");
        ADD(label, value);
    }
    snprintf(value, sizeof(value), "Restore 16 MB (current total: %d MB)", total);
    ADD("Board RAM defaults", value);
    OverlayView_Heading(v, "Console / displays");
    ADD("Boot console", slot_name(p->Dimension.nConsoleSlot));
    ADD("Display mode", p->Screen.nMode == SCREEN_SINGLE ? "Single display" : p->Screen.nMode == SCREEN_ALL ? "Separate windows (windowed only)" : "Grouped displays");
    ADD("Shown display", p->Screen.nMode == SCREEN_SINGLE ? slot_name(p->Screen.nSingleModeSlot) : "Single display mode only");
    for (int i = 0; i < NUM_MONITORS; i++) {
        char label[32];
        snprintf(label, sizeof(label), i ? "Group slot %d" : "Group main display", 2 * i);
        int pos = p->Screen.nGroupModePos[i];
        if (!monitor_present(p, 2 * i)) snprintf(value, sizeof(value), "Board not connected");
        else if (pos < 0) snprintf(value, sizeof(value), "Hidden - Enter to place");
        else snprintf(value, sizeof(value), "Row %d, column %d (4 x 4 grid)", pos / 4 + 1, pos % 4 + 1);
        ADD(label, value);
    }
    ADD("Back", "Advanced");
    v->hint = station ? "NeXTdimension requires a Cube. Advanced hardware options are unavailable on NeXTstation." :
        "Place at least two displays for Grouped mode. Display changes apply without a restart.";
    if (p->Dimension.nConsoleSlot > 0 && monitor_present(p, p->Dimension.nConsoleSlot)) {
        int ram = 0;
        for (int i = 0; i < 4; i++) ram += p->Dimension.board[p->Dimension.nConsoleSlot / 2 - 1].nMemoryBankSize[i];
        if (ram > 32) v->hint = "The NeXTdimension boot console may not appear with more than 32 MB on that board.";
    }
#undef ADD
}

void OverlayDevices_AddRows(const OverlayDevices *s, OverlayView *v, const CNF_PARAMS *p) {
    if (s->page == OV_DEVICES_NETWORK) network_rows(s, v, p);
    else if (s->page == OV_DEVICES_DIMENSION) dimension_rows(s, v, p);
    v->footer = "Up/Down=navigate  Enter=change  Esc=back  F9=Save/Discard";
    if (s->message[0]) v->hint = s->message;
}

void OverlayDevices_DrawEditor(const OverlayDevices *s, SDL_Renderer *renderer) {
    if (!s->editing) return;
    char value[72];
    snprintf(value, sizeof(value), s->replace_text ? "[%s]" : "%s_", s->text);
    const char *lines[] = {s->row == NET_MAC ? "Custom MAC suffix (last three bytes)" : "NFS host name (.home is added by SLiRP)",
        value, s->message[0] ? s->message : "Type to replace; Ctrl+A selects all; Ctrl+V pastes.", "Enter accepts the edit. Esc cancels it."};
    OverlayView_Dialog(renderer, lines, 4, "Enter", "Esc", true);
}

const char *OverlayDevices_Validate(const CNF_PARAMS *old, const CNF_PARAMS *p) {
    if (memcmp(&old->Ethernet, &p->Ethernet, sizeof(p->Ethernet))) {
        if (p->Ethernet.nHostInterface == ENET_PCAP) {
#if !HAVE_PCAP
            return "Extensions / Network: this build has no PCAP support. Select SLiRP.";
#endif
            if (!p->Ethernet.szInterfaceName[0]) return "Extensions / Network: choose a PCAP host interface.";
        } else {
            for (int i = 0; i < EN_MAX_SHARES; i++) {
                if (!p->Ethernet.nfs[i].szPathName[0]) continue;
                if (!File_DirExists(p->Ethernet.nfs[i].szPathName))
                    return "Extensions / Network: each enabled NFS share needs an existing folder.";
                if (i && name_error(p, i, p->Ethernet.nfs[i].szHostName))
                    return "Extensions / Network: use distinct valid NFS names (nfs, dns and previous are reserved).";
            }
        }
    }
    bool hardware = old->System.nMachineType != p->System.nMachineType ||
                    memcmp(&old->Dimension, &p->Dimension, sizeof(p->Dimension));
    bool display = old->Screen.nMode != p->Screen.nMode ||
                   old->Screen.nSingleModeSlot != p->Screen.nSingleModeSlot ||
                   memcmp(old->Screen.nGroupModePos, p->Screen.nGroupModePos, sizeof(p->Screen.nGroupModePos));
    if (hardware) {
        for (int i = 0; i < ND_MAX_BOARDS; i++) {
            const NDBOARD *b = &p->Dimension.board[i];
            if (!b->bEnabled) continue;
            if (p->System.nMachineType == NEXT_STATION)
                return "Disconnect NeXTdimension boards before changing to NeXTstation.";
            if (!File_Exists(b->szRomFileName) || File_DirExists(b->szRomFileName))
                return "Advanced / NeXTdimension: each connected board needs an existing ROM image.";
            for (int j = 0; j < 4; j++)
                if ((j == 0 && b->nMemoryBankSize[j] == 0) ||
                    (b->nMemoryBankSize[j] != 0 && b->nMemoryBankSize[j] != 4 && b->nMemoryBankSize[j] != 16))
                    return "Advanced / NeXTdimension: bank 0 needs 4 or 16 MB; other banks allow 0, 4 or 16 MB.";
        }
    }
    if (hardware || display) {
        if (!monitor_present(p, p->Dimension.nConsoleSlot))
            return "Advanced / NeXTdimension: choose a connected board or the main display as boot console.";
        if (p->Screen.nMode == SCREEN_SINGLE && !monitor_present(p, p->Screen.nSingleModeSlot))
            return "Advanced / NeXTdimension: choose a connected board or the main display to show.";
        if (p->Screen.nMode == SCREEN_GROUP) {
            unsigned positions = 0;
            int visible = 0;
            for (int i = 0; i < NUM_MONITORS; i++) {
                int pos = p->Screen.nGroupModePos[i];
                if (!monitor_present(p, i * 2) || pos < 0) continue;
                if (pos >= 16 || (positions & (1u << pos)))
                    return "Advanced / NeXTdimension: place grouped displays in different grid cells.";
                positions |= 1u << pos;
                visible++;
            }
            if (visible < 2) return "Advanced / NeXTdimension: place at least two displays, or select Single display mode.";
        }
    }
    return NULL;
}
