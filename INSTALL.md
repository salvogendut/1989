# Installing 1989

## Dependencies

- SDL3 (runtime + development files)
- A C99/C++ compiler (GCC or Clang) and GNU make
- autoconf, automake, libtool, pkg-config (to regenerate the build system)
- Optional: libpng (screenshots as PNG), libpcap (raw Ethernet capture),
  readline (debugger line editing)

### Fedora

```bash
sudo dnf install gcc gcc-c++ make autoconf automake libtool pkgconf-pkg-config sdl3-devel
```

### Debian/Ubuntu

```bash
sudo apt install gcc g++ make autoconf automake libtool pkg-config libsdl3-dev
```

## Build

```bash
autoreconf -iv
./configure
make -j"$(nproc)"
```

To enable optional features, pass the corresponding flags to `./configure`
(`--help` lists all options). libpng and readline are picked up automatically
when the development packages are installed.

## Install

```bash
sudo make install
```

This installs:

- `1989` and `ditool` binaries under `$(bindir)`
- NeXT firmware images under `$(pkgdatadir)/roms`
- Empty floppy/hard-disk images under `$(pkgdatadir)/disks`
- Desktop launcher, AppStream metainfo and hicolor icons
- Man page `1989.1`

## Running without installing

`make` produces `./1989` in the build directory. The firmware images live in
`roms/` in the source tree; running from the source directory finds them
automatically. ROM paths can also be selected in F9 → Advanced (enable Tinker), or in F1 → ROM.

## Distribution packages

`make dist` produces `1989-<version>.tar.gz` for downstream packagers. An
RPM spec (`1989.spec`) and Debian build script (`packaging/debian/`) are
provided.
