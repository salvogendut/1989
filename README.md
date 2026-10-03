# 1989 — NeXT (Motorola 68K) emulator

![1989](1989.png)

1989 is an open-source, work-in-progress emulator of the NeXT family of
Motorola 68K based workstations, written in C/C++ with an SDL3 desktop
interface. It is built by integrating the code base of the
[Previous](https://previous.sourceforge.net/) NeXT emulator (itself derived
from Hatari) into the scaffolding conventions shared by the sibling
"happy years" emulators (1983, 1984, 1985, 1986).

## Emulated machines

- NeXT Computer (original 68030 Cube)
- NeXTcube
- NeXTcube Turbo
- NeXTstation
- NeXTstation Turbo
- NeXTstation Color
- NeXTstation Turbo Color
- NeXTdimension Graphics Board (i860-based, optional)

## Highlights

- Motorola 68030/68040 CPU core from the WinUAE m68k emulation core, with
  MMU and 68882 FPU support and cycle-exact bus emulation for the NeXT I/O
  subsystem.
- Monochrome and color NeXT display via SDL3 (4096 x 3072 and 1120 x 832
  monochrome framebuffer modes, plus the color 1120 x 832 mode).
- ADB keyboard and mouse, tablet support, DSP 56001 emulation, and the
  NeXTdimension i860 graphics board emulation.
- SCSI hard disks, magneto-optical drives, floppy drives, and Ethernet
  networking through SLiRP user-mode NAT (or pcap when built with it).
- SDL3 options overlay (F9) with General / Media / Extensions / Advanced
  tabs, an activity LED bar and function-key hint strip at the bottom of the
  window, F-key shortcuts for screenshot (F4), GIF capture (F6) and
  fullscreen (F11).
- The legacy SDL3 options dialog (F12), m68k and i860 debuggers, and sound
  recording.
- A `ditool` companion binary for manipulating NeXT filesystem/disk images.

## Quick start

1989 needs the NeXT firmware images; these are the ROM set distributed with
the upstream Previous project and are included under `roms/` (installed to
`$(pkgdatadir)/roms`). See [ROMS.md](ROMS.md).

On Fedora:

```bash
sudo dnf install gcc gcc-c++ make autoconf automake libtool pkgconf-pkg-config sdl3-devel
autoreconf -iv
./configure
make -j"$(nproc)"
./1989
```

The emulator boots straight into the NeXT ROM monitor/diagnostics. To boot an
operating system, attach a NeXTstep bootable hard-disk image via the options
dialog (F12) and select the boot device. See [USAGE.md](USAGE.md) and
[CONTROLS.md](CONTROLS.md).

## Project status

This is initial scaffolding: the Previous 4.3 code base has been integrated
and wired into the shared build/desktop/packaging conventions of the sibling
emulators. See [Development.md](Development.md) for the integration status and
[ROADMAP.md](ROADMAP.md) for the plan.

## Building from the Previous source

The bulk of the emulation code (CPU core, I/O, GUI, DSP, NeXTdimension,
SLiRP networking, softfloat, debugger) comes unmodified from the Previous
project. It is compiled here through the autotools build instead of
Previous's CMake build; generated CPU sources are used as checked in, so no
code generation step is needed. See [Development.md](Development.md).

## License

GPL-2.0-or-later. See [LICENSE](LICENSE). The `ditool` Virtual File System
portion is MIT-licensed (see the header of `src/slirp/rpc/vfs.c`).