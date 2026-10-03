# 1989 status

Status of the machine emulation. Unless noted, capability comes from the
upstream Previous 4.3 code base integrated into 1989.

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
| Sound              | 16-bit PCM + DSP56001 emulation               |
| Ethernet           | SLiRP NAT; pcap when built with it            |
| ADB                | Keyboard and mouse                            |
| Tablet             | Supported                                     |
| Printer            | NeXT Laser Printer emulation                  |
| RTC/NVRAM          | MC68HC68T1 / MCCS1850                         |

## Integration-specific notes

- The machine core derives from Previous 4.3. Desktop integration changes
  and module boundaries are documented in [Development.md](../Development.md).
- The build system is autotools (the upstream build is CMake). Generated CPU
  sources are checked in, so no codegen step is required.
- `ditool` (NeXT disk image tool) builds alongside the emulator.
- A browser/WASM frontend is planned; `web/` is a placeholder.

## Desktop status

F9 is the primary settings UI; F1 and missing-file recovery still use the
legacy dialogs. See [interface coverage and apply rules](INTERFACE.md) for
the complete migration inventory. Settings/application tests cover reset
classification and per-drive media changes; OS/disk compatibility still
requires testing with a running NeXTstep guest.
