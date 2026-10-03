# Using 1989

1989 is a dialog-driven emulator: there are no per-run command-line options
for media or machine setup. Everything is configured through the options
dialog and stored in the per-user configuration file.

## First start

Run `./1989` (or the installed binary). The emulator loads the machine ROM
(see [ROMS.md](ROMS.md)) and boots into the NeXT ROM monitor. If a ROM file
is missing you are prompted for its location with a file picker.

## Configuration

Press **F1** (or **Alt+O**) to open the legacy options dialog, or **F9** to
open the happy-years options overlay (see [CONTROLS.md](CONTROLS.md)). The
overlay has four tabs:

- **General**: machine type, RAM, CPU clock, FPU, DSP, MMU, ADB and the
  **Tinker** switch.
- **Media**: boot device and the attached SCSI / floppy / magneto-optical
  media. The file picker remembers the last directory used for each entry
  and every selection is written to `1989.conf`.
- **Extensions**: NeXTdimension, printer, Ethernet, tablet and microphone.
- **Advanced**: display options (smoothing, Real CRT with scanlines), GIF
  capture settings (resolution, frame rate, built-in or FFmpeg encoder),
  notifications, the Debugging terminal-log toggle, the RTC clock source
  (host local time or UTC) and window/bar toggles (needs Tinker on).

The main configuration file is `~/.config/1989/1989.conf`; it is created
with defaults on first run. `1989.conf.example` shows the format. UI-only
settings (Tinker, GIF, notifications) live in the `[UI89]` section.

### Legacy options dialog (F1)

The full options dialog opened with **F1** covers every machine detail:
- **System**: machine type (NeXT Computer, NeXTcube, NeXTstation, ...),
  CPU (68030/68040) level and clock, FPU, MMU, ADB, NBIC, SCSI controller,
  RTC chip, DSP 56001 emulation.
- **Memory**: main memory bank sizes (default 4 x 4 MiB = 16 MiB).
- **ROM**: paths of the 030/040/Turbo firmware images and custom MAC address.
- **Boot**: boot device and diagnostics/test toggles shown at power-on.
- **HardDisk / MagnetoOptical / Floppy**: attach disk images. Floppy formats
  400 KiB, 720 KiB, 1.44 MiB and 2.88 MiB are supported; SCSI targets take
  `sd`/raw disk images and ISO/ECC optical disk images. Blank images are
  shipped under `disks/` (`make dist` installs them to `$(pkgdatadir)/disks`).
- **Ethernet**: SLiRP user-mode networking (NAT) or pcap when built with it.
- **Sound**: enable/disable sound and the microphone input.
- **Screen**: window mode, fullscreen, status bar and title bar.
- **Keyboard / Mouse / Tablet**: input device configuration.
- **Shortcuts**: rebind the hot keys.

## Booting NeXTstep

To boot NeXTstep:

1. Obtain a NeXTstep install/bootable disk image and attach it as a SCSI
   target or to a floppy drive.
2. Make sure the machine type and the ROM match the disk's target machine.
3. In the options dialog under **Boot**, select the boot device.
4. Cold reset with **Alt+C** (or change the boot device in the F9 overlay).

The diagnostics screens shown by the ROM before the OS takes over can be
toggled in the **Boot** section.

## Networking

With the SLiRP NAT backend the emulated machine gets outbound network access
through a virtual 10.0.2.x network without host privileges. Configure the
NeXT side with a static address in that range (or use DHCP if available).
See the upstream Previous documentation for NFS mounting details.

## Debuggers

- **Alt+D**: m68k debugger.
- **Alt+I**: i860 (NeXTdimension) debugger.

Both are interactive with a command prompt; `help` lists the commands.

## ditool

`ditool` manipulates NeXT disk images (filesystem creation, file injection,
netboot image creation). Run `ditool -h` for its usage. It is a port of the
tool distributed with Previous.

## Screenshots, GIF capture and recording

- **F4**: save a screenshot (PPM `1989-<timestamp>.ppm`).
- **F6**: toggle GIF capture (`1989-<timestamp>.gif`); resolution and frame
  rate are set in the overlay's Advanced tab.
- **Ctrl++ / Ctrl+-**: increase / decrease the window scale.
- **Alt+G**: legacy screenshot shortcut (PNG when built with libpng).
- **Alt+R**: legacy sound recording (AIFF).

Files are written to the current working directory by default.