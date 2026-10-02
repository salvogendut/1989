# Roadmap

1989 is built by integrating the Previous 4.3 NeXT emulator into the shared
SDL3 "happy years" scaffolding. The machine emulation itself comes from
Previous and is mature; the project work is about matching the sibling
conventions and polishing the experience.

## Milestone 0 — Scaffolding and integration (done)

- [x] Project scaffolding mirroring the sibling emulators (autotools, SDL3,
      desktop/metainfo/icons, man page, packaging).
- [x] Previous 4.3 code base integrated under `src/`.
- [x] Autotools build replacing Previous's CMake build, with the checked-in
      generated CPU sources.
- [x] ROM install layout (`$(pkgdatadir)/roms`) with `ROM_INSTALL_DIR`
      fallback and a source-tree `roms/` lookup.
- [x] First successful native build (`autoreconf`/`configure`/`make`), tests,
      and `make dist`; emulator boots the ROM monitor and runs its loop.
- [ ] Boot a NeXTstep image to a usable desktop.

## Milestone 1 — Happy-years UI (in progress)

- [x] F9 options overlay with General / Media / Extensions / Advanced tabs
      (Advanced gated by the General "Tinker" switch).
- [x] Activity LED bar and function-key hint strip at the bottom of the
      window, with pings from the device emulation.
- [x] Function keys: F4 screenshot (PPM), F6 GIF capture, F9 overlay,
      F11 fullscreen.
- [x] Toast notifications (screen/console/off).
- [x] Boot straight into emulation (no startup config dialog).
- [ ] GIF-capture polish: webp/AVI parity, optional ffmpeg optimize pass.
- [ ] Browser (Emscripten/WASM) frontend using the shared web scaffolding.
- [ ] Test suite growth: config parsing, ROM loading, CPU core self-tests.

## Milestone 2 — Polish

- [ ] Flatpak manifest (`io.github.salvogendut.Emulator1989.yml`).
- [ ] Continuous integration builds.
- [ ] Documentation of the NeXT hardware matrix in `docs/STATUS.md`.

## Upstream alignment

Keep `src/` close to the Previous 4.3 tree to ease merging upstream fixes.
Containerized changes are limited to `paths.c` (config dir name), `rom.c`
(`Rom_GetDefaultPath`), `configuration.c` (startup dialog default), branding
strings, and the new happy-years UI modules (`overlay.c`, `leds.c`,
`notify.c`, `capture.c`, `gifcap.c`).