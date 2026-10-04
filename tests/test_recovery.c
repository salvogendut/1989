/* Real recovery + native-dialog handoff, with scripted host responses.
 * No guest disks, host windows, or emulation threads are started. */
#include "main.h"
#include "recovery.h"
#include "host_dialog.h"
#include "log.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <unistd.h>

volatile bool bQuitProgram, bEmulationActive;
SDL_Window *sdlWindow;
static char default_dir[128];
static int resets, resumed, stopped_paste, reset_error;
static const char *buttons[64];
static int button_count, button_read;
static bool message_failure;
static struct { const char *path; HostPickResult result; bool folder; } picks[32];
static int pick_count, pick_read;
static SDL_DialogFileCallback pending;
static void *pending_data;
static bool quit_pending;
static char last_message[FILENAME_MAX + 512];

void Log_PrintfInt(LOGTYPE type, const char *fmt, ...) { (void)type; (void)fmt; }
bool Main_PauseEmulation(bool show) { (void)show; bool old = bEmulationActive; bEmulationActive = false; return old; }
bool Main_UnPauseEmulation(void) { bEmulationActive = true; resumed++; return true; }
void Main_RequestQuit(bool confirm) { assert(!confirm); bQuitProgram = true; }
void paste_stop(void) { stopped_paste++; }
int Reset_Cold(void) { resets++; return reset_error; }
void Screen_RequestRepaint(void) {}
bool Screen_Repaint(void) { return false; }
void Rom_GetDefaultPath(char *path, int size, const char *name) {
    snprintf(path, size, "%s/%s.BIN", default_dir, name);
}

bool SDL_ShowMessageBox(const SDL_MessageBoxData *box, int *selected) {
    assert(!bEmulationActive);
    assert(box->buttons[0].flags & SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT);
    assert(box->buttons[0].flags & SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT);
    snprintf(last_message, sizeof(last_message), "%s", box->message);
    if (message_failure) { *selected = 1; return SDL_SetError("Simulated message-box failure"); }
    assert(button_read < button_count);
    const char *choice = buttons[button_read++];
    if (!choice) { *selected = -1; return true; } /* Window close. */
    for (int i = 0; i < box->numbuttons; i++) {
        if (!strcmp(box->buttons[i].text, choice)) { *selected = box->buttons[i].buttonID; return true; }
    }
    fprintf(stderr, "Unexpected button: %s\n%s\n", choice, box->message);
    assert(false); return false;
}

static void start_picker(SDL_DialogFileCallback cb, void *data, bool folder) {
    assert(!pending && pick_read < pick_count);
    assert(picks[pick_read].folder == folder);
    pending = cb; pending_data = data;
    quit_pending = picks[pick_read].result == HOST_PICK_QUIT;
}
void SDL_ShowOpenFileDialog(SDL_DialogFileCallback cb, void *data, SDL_Window *win,
                          const SDL_DialogFileFilter *filters, int count, const char *dir, bool multiple) {
    (void)win; (void)filters; (void)count; (void)dir; assert(!multiple);
    start_picker(cb, data, false);
}
void SDL_ShowOpenFolderDialog(SDL_DialogFileCallback cb, void *data, SDL_Window *win, const char *dir, bool multiple) {
    (void)win; (void)dir; assert(!multiple); start_picker(cb, data, true);
}
static void finish_picker(void) {
    assert(pending);
    const char *files[] = {picks[pick_read].path, NULL};
    SDL_DialogFileCallback cb = pending; pending = NULL;
    if (picks[pick_read].result == HOST_PICK_ERROR) SDL_SetError("Simulated picker failure");
    cb(pending_data, picks[pick_read].result == HOST_PICK_ERROR ? NULL : files, 0);
    pick_read++;
}
bool SDL_WaitEventTimeout(SDL_Event *event, Sint32 timeout) {
    assert(timeout <= 20 && pending);
    SDL_zero(*event);
    if (quit_pending) { event->type = SDL_EVENT_QUIT; quit_pending = false; }
    else { finish_picker(); event->type = SDL_EVENT_KEY_DOWN; } /* Input is swallowed. */
    return true;
}

static void choose(const char *label) { buttons[button_count++] = label; }
static void pick(const char *path, HostPickResult result, bool folder) {
    picks[pick_count].path = path; picks[pick_count].result = result;
    picks[pick_count++].folder = folder;
}
static void consumed(void) {
    assert(button_read == button_count && pick_read == pick_count && !pending);
    button_count = button_read = pick_count = pick_read = 0;
}
static void missing(char *path) { snprintf(path, FILENAME_MAX, "%s/missing", default_dir); }
static void baseline(CNF_PARAMS *p, const char *rom) {
    memset(p, 0, sizeof(*p)); p->System.nMachineType = NEXT_CUBE040;
    snprintf(p->Rom.szRom040FileName, FILENAME_MAX, "%s", rom);
}
static void putfile(const char *path) {
    FILE *f = fopen(path, "wb"); assert(f); assert(fputs("test resource", f) >= 0); fclose(f);
}

int main(void) {
    char dir[] = "/tmp/1989-recovery-XXXXXX"; assert(mkdtemp(dir));
    snprintf(default_dir, sizeof(default_dir), "%s", dir);
    char rom[FILENAME_MAX], disk[FILENAME_MAX], def[FILENAME_MAX];
    snprintf(rom, sizeof(rom), "%s/custom.rom", dir); putfile(rom);
    snprintf(disk, sizeof(disk), "%s/system.sd", dir); putfile(disk);
    Rom_GetDefaultPath(def, sizeof(def), "Rev_2.5_v66"); putfile(def);
    static CNF_PARAMS p, before;
    baseline(&p, rom);
    /* Empty/disabled NFS, printer and board paths do not prompt or share HOME. */
    missing(p.Printer.szPrintToFileName); missing(p.Ethernet.nfs[0].szPathName);
    assert(Recovery_CheckFiles(&p, true)); consumed();

    missing(p.Rom.szRom040FileName); before = p;
    choose("Quit"); assert(!Recovery_CheckFiles(&p, true));
    assert(!memcmp(&p, &before, sizeof(p))); consumed();
    choose("Use default"); assert(Recovery_CheckFiles(&p, true));
    assert(!strcmp(p.Rom.szRom040FileName, def)); consumed();

    /* A cancelled picker changes nothing; invalid paths/directories re-prompt. */
    missing(p.Rom.szRom040FileName); before = p;
    choose("Choose file"); pick(NULL, HOST_PICK_CANCEL, false);
    choose("Choose file"); pick(dir, HOST_PICK_SELECTED, false);
    choose("Cancel changes"); assert(!Recovery_CheckFiles(&p, false));
    assert(!memcmp(&p, &before, sizeof(p))); consumed();
    choose("Choose file"); pick(NULL, HOST_PICK_ERROR, false);
    choose("Choose file"); pick(rom, HOST_PICK_SELECTED, false);
    assert(Recovery_CheckFiles(&p, false)); consumed();

    /* Cancelling a later resource also rolls back an earlier explicit edit. */
    missing(p.Rom.szRom040FileName);
    p.SCSI.target[1].nDeviceType = SD_HARDDISK; p.SCSI.target[1].bDiskInserted = true;
    p.SCSI.target[1].bWriteProtected = true; missing(p.SCSI.target[1].szImageName);
    before = p;
    choose("Use default"); choose("Cancel changes"); assert(!Recovery_CheckFiles(&p, false));
    assert(!memcmp(&p, &before, sizeof(p))); consumed();
    choose("Use default"); choose("Choose file"); pick(disk, HOST_PICK_SELECTED, false);
    assert(Recovery_CheckFiles(&p, false)); assert(p.SCSI.target[1].bWriteProtected); consumed();

    /* Removables keep their drive/type/protection; only fixed disks disconnect. */
    p.SCSI.target[3].nDeviceType = SD_CD; p.SCSI.target[3].bDiskInserted = true;
    missing(p.SCSI.target[3].szImageName);
    p.MO.drive[0].bDriveConnected = p.MO.drive[0].bDiskInserted = p.MO.drive[0].bWriteProtected = true;
    missing(p.MO.drive[0].szImageName);
    p.Floppy.drive[0].bDriveConnected = p.Floppy.drive[0].bDiskInserted = true;
    missing(p.Floppy.drive[0].szImageName);
    choose("Leave drive empty"); choose("Leave drive empty"); choose("Leave drive empty");
    assert(Recovery_CheckFiles(&p, true)); consumed();
    assert(p.SCSI.target[3].nDeviceType == SD_CD && !p.SCSI.target[3].bDiskInserted);
    assert(p.MO.drive[0].bDriveConnected && !p.MO.drive[0].bDiskInserted && p.MO.drive[0].bWriteProtected);
    assert(p.Floppy.drive[0].bDriveConnected && !p.Floppy.drive[0].bDiskInserted);
    missing(p.SCSI.target[1].szImageName); choose("Disconnect disk");
    assert(Recovery_CheckFiles(&p, true)); consumed();
    assert(p.SCSI.target[1].nDeviceType == SD_NONE && p.SCSI.target[1].bWriteProtected);

    p.Dimension.board[1].bEnabled = true; missing(p.Dimension.board[1].szRomFileName);
    p.Dimension.nConsoleSlot = p.Screen.nSingleModeSlot = 4;
    choose("Disable board"); assert(Recovery_CheckFiles(&p, true)); consumed();
    assert(!p.Dimension.board[1].bEnabled && !p.Dimension.nConsoleSlot && !p.Screen.nSingleModeSlot);

    p.Ethernet.bEthernetConnected = true; p.Ethernet.nHostInterface = ENET_SLIRP;
    for (int i = 0; i < EN_MAX_SHARES; i++) missing(p.Ethernet.nfs[i].szPathName);
    p.Printer.bPrinterConnected = true;
    choose("Disable share"); choose("Choose folder"); pick(dir, HOST_PICK_SELECTED, true);
    choose("Disable share"); choose("Disable share"); choose("Disable printer");
    assert(Recovery_CheckFiles(&p, true)); consumed();
    assert(!p.Ethernet.nfs[0].szPathName[0] && !strcmp(p.Ethernet.nfs[1].szPathName, dir));
    assert(!p.Printer.bPrinterConnected);

    /* Native dialog failure and window close abort; no implicit default/reset. */
    missing(p.Rom.szRom040FileName); before = p;
    message_failure = true; assert(!Recovery_CheckFiles(&p, true)); message_failure = false;
    assert(!memcmp(&p, &before, sizeof(p))); consumed();
    choose(NULL); assert(!Recovery_CheckFiles(&p, true)); consumed();
    choose("Choose file"); pick(rom, HOST_PICK_QUIT, false);
    assert(!Recovery_CheckFiles(&p, true) && bQuitProgram);
    assert(!memcmp(&p, &before, sizeof(p)));
    finish_picker(); /* Callback arrives after caller returned: no stale pointer. */
    assert(!memcmp(&p, &before, sizeof(p))); consumed(); bQuitProgram = false;
    choose("Choose file"); pick(rom, HOST_PICK_SELECTED, false);
    assert(Recovery_CheckFiles(&p, true)); consumed();

    assert(!resets && !resumed); /* Resource recovery never resets the machine. */
    bEmulationActive = true; choose("Quit"); Recovery_Halt(); consumed();
    assert(bQuitProgram && !bEmulationActive && !resets && !resumed);
    bQuitProgram = false; bEmulationActive = true;
    choose("Restart machine"); Recovery_Halt(); consumed();
    assert(!bQuitProgram && bEmulationActive && resets == 1 && resumed == 1 && stopped_paste == 2);
    assert(strstr(last_message, "disk caches"));
    message_failure = true; Recovery_Halt(); message_failure = false; consumed();
    assert(bQuitProgram && !bEmulationActive && resets == 1 && resumed == 1);
    bQuitProgram = false; bEmulationActive = true; reset_error = 1;
    choose("Restart machine"); Recovery_Halt(); consumed();
    assert(bQuitProgram && !bEmulationActive && resets == 2 && resumed == 1);

    FILE *f = fopen(disk, "rb"); char check[32] = {0}; assert(f);
    assert(fread(check, 1, sizeof(check), f) == strlen("test resource")); fclose(f);
    assert(!strcmp(check, "test resource"));
    remove(rom); remove(disk); remove(def); rmdir(dir);
    puts("test-recovery: OK");
    return 0;
}
