# Controls

1989 uses the shared “happy years” F9 overlay, activity LEDs and function-key
hint strip. [Interface coverage](docs/INTERFACE.md) lists the controls still
available only in the F1 legacy menu.

## Host shortcuts

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
Q quit, N display switch, B status bar and T title bar. F1 → Keyboard can
rebind them. The dedicated F4/F6/F9 and Ctrl+V controls are handled separately.

Clipboard paste sends timed key presses with Shift where needed; unsupported
bytes are skipped. Opening F9 stops an in-progress paste. Window scale and
fullscreen are restored from `1989.conf` on the next launch.

## F9 navigation and saving

| Key | Action |
| --- | --- |
| Left/Right | Switch tab |
| Up/Down | Select a row; holding the key repeats |
| Enter | Change a setting; load/replace selected media |
| E | Eject removable media while keeping the drive connected |
| Delete | Disconnect the selected drive; requires a restart |
| N | Create a blank image for selected media |
| T | Cycle selected SCSI type: hard disk, CD, floppy |
| W | Toggle selected media's write protection; CDs stay read-only |
| H | Media help: SCSI roles, drive numbering and eject/disconnect |
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

### Media

Boot device, seven SCSI targets and a separate group for native floppy/MO drives.
SCSI rows show suggested NeXT roles: ID 1 system disk, 2 data, 3 CD-ROM,
4 external SCSI floppy, 0 alternate boot, 5 spare and 6 extra/swap.
ID 7 is displayed as the reserved host controller and cannot be selected.
Existing assignments stay intact. Loading an unused ID 3 defaults to CD-ROM,
ID 4 to SCSI floppy and other IDs to HDD; T overrides these suggestions.

Boot selection is saved for the next boot and does not restart the current OS.
File pickers remember a directory per entry. Fixed hard-disk changes require a restart;
exchanging media in an already connected removable drive does not.

Eject/unmount media inside NeXT first, select the drive and press **E**, then
close F9 and choose **Save**. This ejects only that medium without resetting
the machine or reopening other disks. The CD, SCSI floppy, native floppy or
MO drive stays connected and can accept another image with Enter. E does
not remove a fixed hard disk. **Delete disconnects the drive**, which needs
restart confirmation. Loading an unconnected drive also requires a restart.

The selected SCSI row previews its expected `sdN` number on the next boot.
It is not a query of the running guest. H explains why adding a lower-ID
drive shifts later numbers; see [SCSI layout](docs/INTERFACE.md#scsi-layout-and-drive-numbers).

N offers blank hard-disk files of 1–32 GiB, 720 KiB/1.44 MiB/2.88 MiB native or SCSI floppies,
and MO file-size choices. Creation never overwrites an existing file; choose
a new name. Files need guest-side formatting, and existing files survive
Discard. See the [MO format caveat](docs/INTERFACE.md#media-creation-and-remaining-polish).

### Extensions

First NeXTdimension board, printer connection, Ethernet connection, tablet
model and microphone. Only NeXTdimension requires a machine restart here.
Other changes update the affected subsystem when saved.

### Advanced

Visible when Tinker is enabled: smoothing, CRT/scanline strength, GIF
resolution (320/480/640), frame rate (10/20/25), built-in/FFmpeg encoder,
notifications, debug output, RTC local/UTC, fullscreen, status/title bars,
DRAM test, verbose boot, 68030/68040/Turbo ROM files and version information.
Display preferences apply on Save. Boot diagnostics apply on the next boot.

## Activity display

The bottom LEDs cover 68K CPU (clock label), DSP, SCSI, floppy, MO, Ethernet,
sound and NeXTdimension. More than one attached SCSI target gets individually
labelled LEDs. Activity lights briefly brighten after device operations.
The function-key strip labels F1 as `legacy`, F9 as `options`, F5 as `reset`
and F12 as `quit`.
