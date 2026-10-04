# 1989 status

Status of the machine emulation. Unless noted, capability comes from the
upstream Previous 4.4 code base plus development fixes through SVN r1854.
See [upstream integration details](UPSTREAM.md).

## Machines

| Machine                      | Status                                  |
|------------------------------|-----------------------------------------|
| NeXT Computer (68030 Cube)   | Supported (Previous core)               |
| NeXTcube                     | Supported                               |
| NeXTcube Turbo               | Supported                               |
| NeXTstation                  | Supported                               |
| NeXTstation Turbo            | Supported                               |
| NeXTstation Color            | Supported                               |
| NeXTstation Turbo Color      | Supported                               |
| NeXTdimension Graphics Board | Supported (i860 emulation, optional)    |

## Hardware

| Component          | Status                                         |
|--------------------|------------------------------------------------|
| CPU                | 68030/68040 (WinUAE m68k core), MMU, 68882 FPU |
| Display            | Monochrome and color framebuffers via SDL3     |
| DMA                | NeXT DMA controller                           |
| SCSI               | NCR 53C90 controller, up to 7 targets         |
| Floppy             | 720 KiB/1.44 MiB/2.88 MiB raw images                  |
| Magneto-optical    | Previous MO sector/ECC format; see INTERFACE.md        |
| Sound              | 16-bit PCM, CD de-emphasis, DSP56001 serial input               |
| Ethernet           | SLiRP NAT; pcap when built with it            |
| ADB                | Keyboard and mouse                            |
| Tablet             | Supported                                     |
| Printer            | NeXT Laser Printer, PNG/TIFF output                  |
| RTC/NVRAM          | MC68HC68T1 / MCCS1850                         |

## Integration-specific notes

- The machine core tracks SDL3 `branch_filesharing` at r1854. Desktop integration changes
  and module boundaries are documented in [Development.md](../Development.md).
- The build system is autotools (the upstream build is CMake). Generated CPU
  sources are checked in, so no codegen step is required.
- `ditool` (NeXT disk image tool) builds alongside the emulator.
- The experimental [SDL3/WASM frontend](../web/README.md) offers all seven models
  before startup, locks the model while running, boots the ROM monitor,
  accepts keyboard input, and provides model-aware media slots with independent eject
  and session-image downloads. Full NEXTSTEP validation and persistent storage
  remain open; sound/networking are disabled in the browser profile.

## Desktop status

F9 is the primary desktop settings UI. F1 is retired and missing-file recovery
uses native host dialogs. See [interface coverage and apply rules](INTERFACE.md) for
the complete migration inventory. Settings/application tests cover reset
classification and per-drive media changes; OS/disk compatibility still
requires testing with a running NeXTstep guest.
