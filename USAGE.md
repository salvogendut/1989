# Using 1989

Run `./1989` from the source tree or the installed binary. Configuration is
stored in `~/.config/1989/1989.conf` on Linux and created on first run.
There are no per-run command-line options for machine or media setup.

## First boot

The default NeXT Computer boots its ROM monitor. ROMs are found in the data
or installed ROM directory, or `./roms` for a source-tree run. If a required
file is missing, a legacy recovery dialog asks for a replacement.

To boot an OS from the ROM monitor:

1. Open F9 → Media, choose **ID 1 System disk** and press Enter to select a compatible disk image.
2. For an installation CD, use **ID 3 CD-ROM**. An unused ID 3 loads as CD-ROM;
   T changes the type if the slot already has a different configuration.
3. Choose the boot device. Confirm Save/Restart when closing F9.
4. If only the boot preference changed, F5 explicitly requests a restart.

With an OS already running, shut it down before confirming a restart or
replacing a fixed disk. Eject/unmount removable media in the guest first.
Closing the emulator or resetting the CPU does not perform a guest shutdown.
To eject removable media, select its row, press **E**, then close F9 and choose
**Save**. Ejection keeps the drive connected, needs no reset, and leaves other
disks open. Enter loads another image. Delete disconnects the drive and requires
a restart. Native floppy/MO drives are grouped separately from the SCSI bus.
**C** connects an empty native drive, with restart confirmation. Native floppy
0 is available on 68040 models; native MO uses the Cube's separate optical
controller and is available only on non-Turbo Cubes.

The row labels are suggested roles, not forced assignments; existing disks
are never moved. H explains SCSI IDs versus NEXTSTEP `sdN` drive numbers.
See the [layout and examples](docs/INTERFACE.md#scsi-layout-and-drive-numbers).

## Options

F9 is the primary interface. General covers machine presets, individual RAM
banks/speed, CPU clock mode and sound;
Media covers boot selection, disks and next-boot diagnostics; Extensions
covers attached devices, printer output and keyboard/mouse controls. Enable
Tinker in General to see Advanced display, capture, logging, RTC and ROM controls,
plus DSP RAM, SCSI/RTC chip selection and Cube NBIC.

The panel edits a draft. Save applies routine changes without rebooting;
Discard leaves the runtime unchanged. Boot preferences apply on the next
boot. Hardware changes need a separate, clearly labelled restart confirmation.
See [CONTROLS.md](CONTROLS.md) and the
[interface inventory](docs/INTERFACE.md) for details.

F1 (also Ctrl+Alt+O by default) still provides config import/export. Its media
changes are staged until OK. The legacy Save config button explicitly writes
to the chosen file. Missing-file recovery and alerts also still use legacy
dialogs. “Show menu at startup” is retired.

The testing-only temporary SCSI write overlay is retired. Loading an old
configuration with it enabled makes configured SCSI drives read-only. Use
per-drive write protection in Media to change this; writable images now
always receive persistent writes. Shut down the guest before changing a
fixed disk's protection and confirming the required restart.

The default effective memory configuration is 64 MiB on the 68030 Cube.
Changing machine variants chooses model-specific CPU/RAM defaults.
`1989.conf.example` illustrates the current keys; `[UI89]` holds additional
desktop settings. Prefer editing the file while the emulator is closed,
since accepted UI changes and exit save the runtime configuration.

## Networking and peripherals

The SLiRP backend provides user-mode networking. F9 → Extensions toggles the
connection; **Network / NFS** selects SLiRP/PCAP, host interface, guest cable,
custom MAC, network time and up to four NFS shares. PCAP requires build support
and host capture permissions. NFS shares and network time use SLiRP.

Select each exported folder with Enter. The first export uses `nfs.home`;
additional exports use their configured name plus `.home`. Names must be
unique DNS labels; `nfs`, `dns` and `previous` are reserved. Delete on a folder
stops exporting it. Unmount exports inside NeXT before replacing/removing
them. Saving export or cable changes restarts only network services, leaving
guest disks attached. Backend, network-time and MAC edits require explicit
machine restart confirmation. MAC editing preserves the ROM prefix.

F9 → General → Tinker enables Advanced. Its **NeXTdimension / displays** page
configures Cube boards in slots 2/4/6, per-board ROM and RAM, boot console,
single/separate/grouped displays and monitor arrangement. Prepare at least
two grid positions for Grouped mode. Display selection and arrangement apply
without rebooting NeXT; connected-board hardware and boot-console changes
require restart confirmation. ROM/RAM settings for disconnected boards can
be saved without rebooting. Esc returns to the tab; F9 opens Save/Discard.

F9 toggles sound, microphone, tablet and printer connection without restarting
NeXT. Extensions also provides printer paper, PNG/TIFF format and output
folder, keyboard mapping, mouse motion presets and capture options.
**Input details** edits shortcut bindings and numeric mouse sensitivity.
These remain drafts until Save and apply without a restart. **D** on machine
or board ROM rows restores an available default path; active ROM changes
require restart confirmation.
The emulated printer is the NeXT Laser Printer.

## Capture, debuggers and disk tools

- F4 writes a PPM screenshot; F6 toggles GIF capture. Advanced selects GIF
  size, rate and optional FFmpeg optimization. Files go to the working
  directory. FFmpeg optimization currently runs synchronously when stopping.
- Ctrl+Alt+G uses the legacy screenshot path (PNG with libpng);
  Ctrl+Alt+R toggles AIFF sound recording.
- Ctrl+Alt+D opens the m68k debugger; Ctrl+Alt+I opens the i860 debugger.
- `ditool -h` describes the companion NeXT image tool. Blank images created
  in F9 contain no filesystem; they need preparation before use.

Window scale, fullscreen, media paths and accepted preferences persist across
launches. Browser/WASM support remains planned.
