# Development notes for 1989

1989 builds Previous 4.3's C/C++ emulation core through Autotools and adapts
its SDL3 frontend to the sibling emulator conventions. Keep machine-core
changes contained so upstream fixes remain practical to merge.

## Structure

- `configure.ac`, `Makefile.am`: SDL3, optional PNG/pcap/readline/FFmpeg,
  generated configuration, `1989` and the C `ditool` companion, installation
  and distribution. Generated CPU sources are checked in.
- `src/main.c`: initialization, configuration load/save, emulation lifecycle.
- `src/gui-sdl/sdlevent.c`: SDL events and host shortcuts. On Linux, ordinary
  guest input crosses a queue to the emulation thread.
- `src/gui-sdl/sdlscreen.c`: framebuffer composition, filtering/CRT, status
  and activity strips, overlay presentation. UI redraw can be requested
  independently of framebuffer updates, including while paused.
- `src/cpu`, `softfloat`, `dsp`, `dimension`, and the device modules: machine
  emulation inherited from Previous. `slirp/rpc` is the built C RPC/NFS
  implementation; `slirp/nfs` and `ditool_cpp` retain reference C++ sources.
- `roms`, `disks`, `icons`, `packaging`: firmware, blank-image templates,
  desktop artwork and distribution scripts. `web/` is a placeholder.

## Options architecture

| Module | Responsibility |
| --- | --- |
| `overlay.c` | Sections/rows, keyboard navigation, edit-session and confirmation flow |
| `overlay_view.c` | Stateless SDL drawing of copied rows, tabs, choices and dialogs; no device/configuration calls |
| `overlay_media.c` | Suggested SCSI roles/types, next-boot drive-number preview, native-picker handoff, eject/disconnect draft updates, image validation and exclusive sparse-file creation |
| `ui_config.c` | Load/save/apply `[UI89]` desktop preferences |
| `settings.c` | Private machine draft, merge with live state, restart policy and per-target media application |
| `change.c` | Apply configuration to runtime subsystems; shared by F9 and the legacy dialog |

The overlay never writes `ConfigureParams` while navigating. It snapshots
configuration briefly under pause, edits its own machine/UI copies, then
pauses again to apply a confirmed change. Unedited groups and individual
media targets are merged from live state so a guest eject during editing is
not undone. Discard has no runtime undo path because it has no runtime effects.

Restart decisions compare the resulting configuration, rather than trusting
flags set by individual row handlers. Boot options are saved for the next
boot. Network connection, tablet, printer, sound and display changes update
their own subsystems. Machine hardware and fixed-disk changes require explicit
confirmation. Live removable-media changes call the selected drive's
insert/eject functions; never reset every storage controller to apply one image.
The legacy dialog stages media as well and uses this same application path.
Media role suggestions apply only when loading an unused SCSI slot; they do
not initialize drives on panel open or migrate existing configurations.
Eject retains the device type/connection, while disconnect removes it and
uses the hardware restart path. Native floppy/MO entries do not consume SCSI IDs.

Native file callbacks publish a result under an SDL spinlock and hold no
pointer to an edit session. Only one request may be outstanding. Closing the
panel invalidates the result; cancelled/late results cannot edit a later
session. File creation never truncates an existing path. A created file is
an independent host artifact and remains when the panel draft is discarded.

The remaining legacy-menu inventory and user-facing apply rules are in
[docs/INTERFACE.md](docs/INTERFACE.md). F1, missing-file recovery and legacy
alerts remain; their full migration is not claimed by this refactor.

## Configuration and resources

Linux configuration lives in `~/.config/1989/1989.conf`; macOS and Windows
use the corresponding Application Support/AppData paths from `paths.c`.
`ConfigureParams` and `cfgopts` retain Previous's section/key format;
`[UI89]` stores the additional desktop preferences. ROM lookup includes the
configured data directory, installed ROM directory and source-tree `roms/`.
Internal Previous/Hatari names generally remain in core code.

## Verification

```sh
autoreconf -iv
./configure
make -j"$(nproc)"
make -C tests check
make dist
```

Settings tests instrument runtime device calls while executing the real
change/apply code. Overlay tests send key events through the real controller,
exercise software rendering, reject unconfirmed resets, preserve guest
ejects, cancel late file-picker results, and check exclusive large-image
creation. Capture tests use a synthetic framebuffer and real PPM/GIF encoding.
Tests do not boot NeXTstep or verify host-native dialogs on every platform.

## Upstream

Previous 4.3: https://previous.sourceforge.net/; WinUAE m68k core;
NeXTdimension i860 emulation by Jason Eckhardt. See the source headers and
LICENSE for licensing details.
