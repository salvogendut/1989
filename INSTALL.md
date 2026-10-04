# Installing 1989

## Release packages

Get the appropriate asset from [GitHub Releases](https://github.com/salvogendut/1989/releases).
`SHA256SUMS` contains checksums for all six downloads. Firmware, blank-image
templates and `ditool` are included; NEXTSTEP itself is not included.

- **Debian 13 (trixie), amd64:** `sudo apt install ./1989_0.2.0_amd64.deb`.
- **Fedora 44, x86_64:** `sudo dnf install ./1989-0.2.0-1.fc44.x86_64.rpm`.
- **Flatpak, x86_64:** add [Flathub](https://flathub.org/setup) for the
  Freedesktop 25.08 runtime, then `flatpak install --user ./1989-0.2.0-x86_64.flatpak`.
  Start with `flatpak run io.github.salvogendut.Emulator1989`. Home-directory
  access allows disk images and NFS shares; networking supports guest Ethernet.
  Run the helper with `flatpak run --command=ditool io.github.salvogendut.Emulator1989 -h`.
- **Windows, x86_64:** extract the whole ZIP into a writable directory and
  run `1989.exe`. Keep its DLLs and `roms` directory alongside it.
- **macOS 15 or later:** choose `arm64` for Apple Silicon or `x86_64` for
  Intel, extract the ZIP and move `1989.app` to Applications. The app is
  ad-hoc signed, not notarized; macOS may require approval in Privacy & Security
  after the first launch attempt. `ditool` is in `1989.app/Contents/MacOS`;
  firmware and blank-image templates are in `Contents/Resources`.

Release packages use the built-in GIF encoder. Optional external FFmpeg
optimization is available in source builds when FFmpeg is installed at configure time.

## Dependencies

- SDL3 3.2 or later (runtime + development files)
- A C99/C++ compiler (GCC or Clang) and GNU make
- autoconf, automake, libtool, pkg-config (to regenerate the build system)
- Optional: libpng (screenshots as PNG), libpcap (raw Ethernet capture),
  readline (debugger line editing)

### Fedora

```bash
sudo dnf install gcc gcc-c++ make autoconf automake libtool pkgconf-pkg-config sdl3-devel
```

### Debian 13 / distributions with SDL3 development packages

```bash
sudo apt install gcc g++ make autoconf automake libtool pkg-config libsdl3-dev
```

### macOS

```bash
brew install autoconf automake pkg-config sdl3 libpng libpcap readline dylibbundler
```

Use `make -j"$(sysctl -n hw.logicalcpu)"` instead of `nproc` below.
After building, `sh packaging/macos/package.sh ./1989 dist/1989.app 0.2.0`
creates a relocatable app with its libraries and firmware.

### Windows

Use the MSYS2 **MINGW64** shell with `base-devel`, `autoconf`, `automake`,
`mingw-w64-x86_64-gcc`, `mingw-w64-x86_64-pkgconf`,
`mingw-w64-x86_64-sdl3` and `mingw-w64-x86_64-libpng`. Run the build commands
below, then `sh packaging/windows/package.sh dist/windows` to collect the
executables, MinGW runtime DLLs and resources.

## Build

```bash
autoreconf -iv
./configure
make -j"$(nproc)"
```

libpng, libpcap and readline are picked up automatically when the development
packages are installed. Use `./configure --without-ffmpeg` to disable external
GIF optimization, including for portable packages.

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
automatically. ROM paths can also be selected in F9 → Advanced (enable Tinker).
Portable bundles find their included firmware regardless of the launch directory.

## Distribution packages

`make dist` produces `1989-<version>.tar.gz` for downstream packagers. An
RPM spec (`1989.spec`) and Debian build script (`packaging/debian/`) are
provided.

## Releases

`.github/workflows/build.yml` builds and tests on `main`, pull requests and
manual dispatch. It creates six downloadable Actions artifacts: Fedora RPM,
Debian DEB, Flatpak, Windows ZIP contents, and separate macOS ARM/Intel ZIPs.
Linux and macOS run the regression suite; portable bundles also check `ditool`.
CI never boots or modifies a user-provided NeXT disk image.

To publish a new release:

1. Update the version in `configure.ac`, `1989.spec`, `1989.1` and the AppStream
   metainfo, adding a dated changelog/release entry. Update examples above.
2. Commit and push; wait for every platform build to pass.
3. Tag that commit `v<version>` and push the tag. CI checks version agreement,
   builds all platforms, and publishes the six packages plus `SHA256SUMS`.

The Flatpak manifest builds the current checkout and pins its SDL3 source
and checksum. Its SDK version must match the workflow's Flatpak builder image.
The workflow's Fedora/Debian images and macOS runner labels define the
supported release baselines; update these together with these instructions.
