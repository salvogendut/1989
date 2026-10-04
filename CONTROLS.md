# Controls

1989 uses the shared “happy years” F9 overlay, activity LEDs and function-key
hint strip. [Interface coverage](docs/INTERFACE.md) lists the controls still
available only in the F1 legacy menu.

## Default host shortcuts

| Key | Action |
| --- | --- |
| F1 | Legacy options |
| F4 | PPM screenshot |
| F5 | Restart confirmation |
| F6 | Start/stop GIF recording |
| F9 | Open/close options |
| F11 | Fullscreen |
| F12 | Quit confirmation |
| Ctrl++ / Ctrl+- | Increase/decrease window scale |
| Ctrl+V | Type host clipboard text into the guest keyboard |
| Ctrl+Enter | Release the captured mouse |

Click in the window to capture the mouse. Opening options or a confirmation
releases it. Restart and quit initially select **Cancel**; use Left/Right
and Enter to choose, or Esc to cancel. Shut down NeXT in the guest before
restarting or quitting.

The configurable legacy shortcuts require **Ctrl+Alt**, not Alt alone:
O options, F fullscreen, M mouse capture, C restart, G legacy screenshot,
R AIFF recording, S sound, P pause, D m68k debugger, I i860 debugger,
Q quit, N display switch and T title bar. F9 → Extensions → **Input details**
can rebind them, including the default F1/F5/F11/F12 bindings. The dedicated
F4/F6/F9 and Ctrl+V controls are handled separately.

Clipboard paste sends timed key presses with Shift where needed; unsupported
bytes are skipped. Opening F9 stops an in-progress paste. Window scale and
fullscreen are restored from `1989.conf` on the next launch.

## F9 navigation and saving

| Key | Action |
| --- | --- |
| Left/Right | Switch tab |
| Up/Down | Select a row; holding the key repeats |
| Enter | Change a setting; load/replace selected media |
| C | Connect an empty native floppy/MO drive; requires a restart |
| E | Eject removable media while keeping the drive connected |
| Delete | Disconnect the selected drive; requires a restart |
| N | Create a blank image for selected media |
| T | Cycle selected SCSI type: hard disk, CD, floppy |
| W | Toggle selected media's write protection; CDs stay read-only |
| H | Media help: SCSI roles, drive numbering and eject/disconnect |
| D | Restore the selected machine or NeXTdimension ROM's default path |
| Esc / F9 | Close or show Save/Discard confirmation |

Settings are staged in private copies while NeXT keeps running. **Save**
applies them; **Discard** leaves the running configuration untouched.
**Esc in the confirmation returns to editing.** A change followed by its
inverse is a no-op. Hardware and fixed-disk changes show a **Restart**
confirmation with Discard selected initially. See [apply rules](docs/INTERFACE.md#applying-changes).

### General

Machine variant, RAM presets, CPU clock, FPU, DSP, MMU status, Turbo ADB, sound output,
Tinker, About and **Machine defaults**. Machine defaults restores the selected
model's hardware defaults; it does not clear media or desktop preferences.

CPU clock cycles 16/20/25/33 MHz, plus 40 MHz on Turbo. **Variable CPU clock**
selects the core's variable timing mode. **Memory banks** exposes each bank
and memory speed alongside the total-RAM presets. Monochrome banks offer
empty/1/4/16 MB, Color empty/2/8 MB and Turbo empty/2/8/32 MB. Non-Turbo
monochrome NeXTstations have only banks 0/1. Memory-speed labels follow the
selected controller: 120/100/80/60 ns normally, 60/70/80/100 ns on Turbo.
All these hardware edits require explicit restart confirmation.

### Media

Boot device, seven SCSI targets and a separate group for native floppy/MO drives.
SCSI rows show suggested NeXT roles: ID 1 system disk, 2 data, 3 CD-ROM,
4 external SCSI floppy, 0 alternate boot, 5 spare and 6 extra/swap.
ID 7 is displayed as the reserved host controller and cannot be selected.
Existing assignments stay intact. Loading an unused ID 3 defaults to CD-ROM,
ID 4 to SCSI floppy and other IDs to HDD; T overrides these suggestions.

Boot selection and the **Next-boot diagnostics** group are saved for the next
boot without restarting the current OS. Diagnostics include the power-on
master switch, DRAM/sound/SCSI tests, repeat/extended tests, diagnostic video
and verbose boot.
File pickers remember a directory per entry. Fixed hard-disk changes require a restart;
exchanging media in an already connected removable drive does not.

Eject/unmount media inside NeXT first, select the drive and press **E**, then
close F9 and choose **Save**. This ejects only that medium without resetting
the machine or reopening other disks. The CD, SCSI floppy, native floppy or
MO drive stays connected and can accept another image with Enter. E does
not remove a fixed hard disk. **Delete disconnects the drive**, which needs
restart confirmation. Loading an unconnected drive also requires a restart.

**C connects an empty native drive** without choosing an image. Native floppy
0 requires a 68040 model; hardware has only one native floppy drive. Native
MO requires a non-Turbo Cube and uses its own optical controller, without a
SCSI ID. These restrictions do not affect SCSI floppy devices. Existing
unsupported entries can still be ejected or disconnected. Connecting the
second MO drive adds the legacy NEXTSTEP kernel-crash warning to the existing
restart confirmation. No extra confirmation appears for ordinary ejection.

The selected SCSI row previews its expected `sdN` number on the next boot.
It is not a query of the running guest. H explains why adding a lower-ID
drive shifts later numbers; see [SCSI layout](docs/INTERFACE.md#scsi-layout-and-drive-numbers).

N offers blank hard-disk files of 1–32 GiB, 720 KiB/1.44 MiB/2.88 MiB native or SCSI floppies,
and MO file-size choices. Creation never overwrites an existing file; choose
a new name. Files need guest-side formatting, and existing files survive
Discard. See the [MO format caveat](docs/INTERFACE.md#media-creation-and-remaining-polish).

### Extensions

Connections cover the first NeXTdimension board, printer, Ethernet, tablet
model and microphone. NeXTdimension is available on Cubes and requires NBIC.
**Printer output** adds paper size, PNG/TIFF image
format and a native output-folder picker. The format also controls legacy
screenshots; F4/F6 keep PPM/GIF. Without libpng, image output uses TIFF.

**Keyboard and mouse** provides symbolic/scancode mapping, Command/Alt swap,
slow/fast motion presets, raw motion, Ctrl-click as right-click, wheel-to-arrow
mapping and automatic mouse capture. Imported custom sensitivity values are
shown and preserved until edited. Ctrl+Enter still releases the mouse.

**Input details** edits the linear mouse scale (0.01–10.0) and exponential
scale (0.50–1.00); a decimal point or comma is accepted. For shortcuts, cycle
**Action**, then edit **With Ctrl+Alt** or **Without Ctrl+Alt**. Enter starts
key capture; press the desired key, Esc to cancel or Delete to clear. Delete
also clears a selected binding without capture. Duplicate bindings are
rejected within each modifier group. F4/F6/F9 and modifier-only keys are
reserved, as are Ctrl+Alt combinations intercepted by desktop paste, mouse
release and zoom. Save applies input changes without a restart.

**Network / NFS** opens a detail page for SLiRP/PCAP, PCAP host interface,
thinwire/twisted-pair cable, network time, ROM/custom MAC and four NFS exports.
PCAP is selectable only when compiled in; Enter cycles available interfaces.
The 68030 Cube uses thinwire. NFS and network time are SLiRP features.
Share 0 is always named `nfs`; shares 1–3 have editable host names. Enter on a
folder opens a native picker; Delete stops exporting it. Unmount exports in
NeXT before changing them. NFS and cable changes apply without a machine
reset. Backend, network-time, MAC and NeXTdimension hardware changes require
restart confirmation.

In detail pages, Up/Down selects a row, Enter edits, Esc returns to the parent
tab and F9 opens Save/Discard. Text fields initially select the whole value:
type to replace, Ctrl+A selects all, Ctrl+V pastes, Backspace/Delete removes
the selection or last character, Enter accepts, Esc cancels. MAC editing
changes only the last three bytes (`aa:bb:cc`); the ROM's manufacturer prefix
is retained. Share names are unique single DNS labels, without `.home`.
Discard and cancelled folder selections leave the runtime unchanged.

### Advanced

Visible when Tinker is enabled: smoothing, CRT/scanline strength, GIF
resolution (320/480/640), frame rate (10/20/25), built-in/FFmpeg encoder,
notifications, debug output, RTC local/UTC, fullscreen, title bar,
68030/68040/Turbo ROM files and version information. Display preferences
apply on Save. Boot diagnostics are grouped in Media, including DRAM test
and verbose boot, and do not require Tinker.

Enter on a ROM row selects a file; **D restores its discovered default** from
the same installed/source-tree lookup used at startup. A missing default
leaves the current choice intact. Changes to the active machine ROM require
restart confirmation; inactive variants can be saved without a reset.

**Machine hardware** provides DSP RAM (24/96 KB), SCSI controller
(NCR53C90/NCR53C90A), RTC chip (MC68HC68T1/MCCS1850) and NBIC. NBIC is
Cube-only and cannot be disabled while any NeXTdimension board is enabled.
These hardware changes require a restart; RTC local/UTC remains a live
host-clock preference.

**NeXTdimension / displays** opens a detail page for slots 2, 4 and 6.
Choose the board, then its connection, ROM and four RAM banks. Bank 0 allows
4/16 MB; other banks also allow empty. Board RAM defaults restore 16 MB total.
Disconnected boards can have ROM/RAM preferences saved without a restart.
**D** on the board's ROM row restores its discovered default as well.

Boot console selects the main display or a connected NeXTdimension and
requires restart confirmation. Display mode selects one display, separate
windows, or a grouped display; Shown display selects the single-mode output.
Separate windows temporarily use a single display in fullscreen.
Each Group row cycles through free positions on a 4 × 4 grid, then Hidden.
Prepare at least two visible positions before saving Grouped mode. These
view/layout changes apply live. The editor reports disconnected console/view
selections and invalid groups before Save. The NeXTdimension boot console may not appear
with more than 32 MB on that board; the page shows a reminder.

## Activity display

The bottom LEDs cover 68K CPU, DSP, SCSI, floppy, MO, Ethernet,
sound and NeXTdimension. More than one attached SCSI target gets individually
labelled LEDs. Activity lights briefly brighten after device operations.
The function-key strip labels F1 as `legacy`, F9 as `options`, F5 as `reset`
and F12 as `quit`. Beside the machine model it shows the configured CPU
frequency and total RAM, for example `1989 NeXTcube | 25 MHz | 64 MB`.
The label uses the running configuration; unconfirmed hardware edits do not
change it. Variable CPU timing is identified explicitly.

The duplicate Previous status bar and Ctrl+Alt+B toggle are retired. Core
messages (media, printer, audio and debugger notices) use the existing
notification mode in Advanced: Screen, Console or Off. Old status-bar
configuration keys are accepted but cannot restore the bar.
