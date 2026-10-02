# Roadmap

1989 is built by integrating the Previous 4.3 NeXT emulator into the shared
SDL3 "happy years" scaffolding. The initial milestone (this commit) is
scaffolding + integration; the machine emulation itself comes from Previous
and is mature.

## Milestone 0 — Scaffolding and integration (in progress)

- [x] Project scaffolding mirroring the sibling emulators (autotools, SDL3,
      desktop/metainfo/icons, man page, packaging).
- [x] Previous 4.3 code base integrated under `src/`.
- [x] Autotools build replacing Previous's CMake build, with the checked-in
      generated CPU sources.
- [x] ROM install layout (`$(pkgdatadir)/roms`) with `ROM_INSTALL_DIR`
      fallback.
- [x] First successful native build (`autoreconf`/`configure`/`make`), tests,
      and `make dist`; emulator initializes and runs its main loop.
- [ ] Boot a NeXTstep image to a usable desktop.

## Milestone 1 — Sibling conventions

- [ ] Screenshot/recording format parity (PPM/PNG already upstream).
- [ ] Browser (Emscripten/WASM) frontend using the shared web scaffolding.
- [ ] On-screen overlay help and CRT themes if desired.
- [ ] Test suite wired into `make check` (config parsing, ROM loading,
      CPU core self-tests).

## Milestone 2 — Polish

- [ ] Flatpak manifest (`io.github.salvogendut.Emulator1989.yml`).
- [ ] Continuous integration builds.
- [ ] Documentation of the NeXT hardware matrix in `docs/STATUS.md`.

## Upstream alignment

Keep `src/` close to the Previous 4.3 tree to ease merging upstream fixes.
Containerized changes are limited to `paths.c` (config dir name), `rom.c`
(`Rom_GetDefaultPath`), and branding strings.