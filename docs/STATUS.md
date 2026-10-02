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
| Floppy             | 400k/720k/1.44M/2.88M images                  |
| Magneto-optical    | 128 MiB/230 MiB/640 MiB/1.3 GiB images        |
| Sound              | 16-bit PCM + DSP56001 emulation               |
| Ethernet           | SLiRP NAT; pcap when built with it            |
| ADB                | Keyboard and mouse                            |
| Tablet             | Supported                                     |
| Printer            | Dot-matrix printer emulation                  |
| RTC/NVRAM          | MC68HC68T1 / MCCS1850                         |

## Integration-specific notes

- The `src/` tree is the upstream Previous 4.3 code; compile-time changes
  made for 1989 are documented in `Development.md`.
- The build system is autotools (the upstream build is CMake). Generated CPU
  sources are checked in, so no codegen step is required.
- `ditool` (NeXT disk image tool) builds alongside the emulator.
- A browser/WASM frontend is planned; `web/` is a placeholder.