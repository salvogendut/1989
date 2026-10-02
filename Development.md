# Development notes for 1989

This document tracks how the project is organized and how the upstream
"Previous" code base is integrated into the shared "happy years" scaffolding.

## Layout

```
configure.ac        autotools build configuration (SDL3, libpng, ...)
Makefile.am         single-Makefile build for the 1989 and ditool binaries
src/                the Previous 4.3 emulation core, integrated as-is
  main.c ...        NeXT machine, I/O memory, SCSI, floppy, Ethernet, sound
  includes/         core headers
  overlay.c/h       F9 options overlay (General/Media/Extensions/Advanced)
  leds.c/h          activity LED bar at the bottom of the window
  notify.c/h        fading toast notifications
  gifcap.c/h        in-tree GIF89a encoder (LZW)
  capture.c/h       screenshot (PPM) and GIF recording helpers
  gui-sdl/          SDL3 GUI: legacy options dialogs, screen, keyboard
  cpu/              WinUAE m68k CPU core (checked-in generated sources)
  debug/            m68k/i860 debuggers and logging
  softfloat/        soft-float support for the 68040 FPU
  dsp/              Motorola DSP56001 emulation
  dimension/        NeXTdimension i860 board emulation (C++)
  slirp/            SLiRP user-mode network stack (+ NFS RPC)
  ditool/           C port of the NeXT disk image tool
  ditool_cpp/       newer C++ ditool (reference only, not built)
roms/               NeXT firmware images (from the Previous distribution)
disks/              blank floppy/hard-disk image templates
icons/              hicolor PNG set generated from the Previous bitmap
docs/               machine/ROM reference material
tests/              build smoke tests (str, capture/GIF)
web/                (placeholder) future Emscripten/WASM frontend
```

## Happy-years UI conventions

1989 follows the sibling emulator (1983-1986) conventions:

- **F9 options overlay** (`overlay.c`): tabbed panel drawn on the SDL
  renderer just before present. Reads/writes the existing `ConfigureParams`
  plus a small `[UI89]` config section (Tinker, GIF, notifications) managed
  in `overlay.c` via `cfgopts`.
- **LED activity bar** (`leds.c`): dark strip at the bottom of the window,
  centred LEDs pinged from the device emulation (`leds_ping`). A
  function-key hint strip sits above it.
- **Toast notifications** (`notify.c`): fading messages, tri-state mode.
- **Function keys**: F4 screenshot, F6 GIF, F9 overlay, F11 fullscreen.
  The legacy F12 options dialog and Alt+... shortcuts remain available.

The bottom strips reserve `FUNCTION_KEY_BAR_H + LED_BAR_H` window rows below
the legacy status bar (`Screen_Reset` in `gui-sdl/sdlscreen.c`); the overlay,
LEDs, toasts and GIF capture are rendered by `Screen89_RenderExtras()` before
`SDL_RenderPresent`.

## Integration notes

- The upstream project builds with CMake; 1989 builds the same sources with
  autotools so it shares the conventions of the sibling emulators. The
  generated CPU files (`cpu/cpuemu_31.c`, `cpu/cpuemu_32.c`,
  `cpu/cpustbl.c`, `cpu/cpudefs.c`) are used as checked in — no `gencpu`/
  `build68k` code generation step is required.
- `config.h` is produced by `./configure` (see `configure.ac`) and mirrors
  the macros the upstream CMake `config.h` provided (`HAVE_*`,
  `ENABLE_DSP_EMU`, `ENABLE_TRACING`, `BIN2DATADIR`, ...).
- The install-time ROM path is exposed as `ROM_INSTALL_DIR`
  (`$(pkgdatadir)/roms`); `Rom_GetDefaultPath()` in `src/rom.c` falls back
  to it when a firmware image is not found in the data directory. This is a
  small, contained addition on top of the upstream code.
- Branding: the binary and package are named `1989`; the user config
  directory is `~/.config/1989` (`HATARI_HOME_DIR` in `src/paths.c`) and the
  config file is `1989.conf`. The internal "Previous" identifiers are
  otherwise kept intact to ease future upstream merges.

## Verification

```bash
autoreconf -iv && ./configure && make -j"$(nproc)"
make -C tests check
```

## Upstream

- Previous 4.3: https://previous.sourceforge.net/ (GPL-2.0-or-later)
- CPU core: WinUAE m68k emulation
- NeXTdimension: i860 emulation by Jason Eckhardt
- `ditool` Virtual File System: MIT (see `src/slirp/rpc/vfs.c`)