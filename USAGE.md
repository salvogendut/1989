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

The row labels are suggested roles, not forced assignments; existing disks
are never moved. H explains SCSI IDs versus NEXTSTEP `sdN` drive numbers.
See the [layout and examples](docs/INTERFACE.md#scsi-layout-and-drive-numbers).

## Options

F9 is the primary interface. General covers machine presets and sound;
Media covers boot selection and disks; Extensions covers attached devices.
Enable Tinker in General to see Advanced display, capture, logging, RTC,
boot-diagnostic and ROM controls.

The panel edits a draft. Save applies routine changes without rebooting;
Discard leaves the runtime unchanged. Boot preferences apply on the next
boot. Hardware changes need a separate, clearly labelled restart confirmation.
See [CONTROLS.md](CONTROLS.md) and the
[interface inventory](docs/INTERFACE.md) for details.

F1 (also Ctrl+Alt+O) opens the legacy options dialog for custom RAM banks and
speed, detailed keyboard/mouse controls, network/NFS configuration,
NeXTdimension boards/displays, printer paper/output directory, and config
import/export. Its media changes are staged until OK. The legacy Save config
button explicitly writes to the chosen file.

The default effective memory configuration is 64 MiB on the 68030 Cube.
Changing machine variants chooses model-specific CPU/RAM defaults.
`1989.conf.example` illustrates the current keys; `[UI89]` holds additional
desktop settings. Prefer editing the file while the emulator is closed,
since accepted UI changes and exit save the runtime configuration.

## Networking and peripherals

The SLiRP backend provides user-mode networking. F9 toggles the connection;
F1 → Network selects SLiRP/pcap, host interface, guest cable type, custom MAC,
network time and NFS shares. pcap controls depend on build support.

F9 toggles sound, microphone, tablet and printer connection without restarting
NeXT. F1 holds mouse sensitivity/key mapping and printer paper/output settings.
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
