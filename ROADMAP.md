# Roadmap

1989 integrates the Previous 4.4 core plus r1854 development fixes with the shared SDL3 desktop experience.
The [interface inventory](docs/INTERFACE.md) and
[issue #1](https://github.com/salvogendut/1989/issues/1) track F1 migration.

## Implemented

- Previous core, checked-in generated CPU sources and Autotools native build.
- Firmware/resource installation, desktop integration, RPM/Debian/macOS
  packaging scaffolding and `ditool`.
- F9 General/Media/Extensions/Advanced tabs, activity LEDs and function hints.
- Private settings drafts; Save/Discard; shared restart policy; confirmed
  F5 restart; per-drive removable-media updates in F9 and F1.
- Live display/audio/network-connection/tablet/printer settings; boot options
  saved for the next boot without automatically resetting the guest.
- Window/fullscreen persistence, relative mouse capture, clipboard typing.
- PPM/GIF capture, optional FFmpeg optimization, CRT, notifications.
- Native image/ROM pickers, SCSI type and media protection, exclusive sparse
  blank-image creation, and tests for edit/apply/file-picker boundaries.
- Media roles matching NeXT's suggested SCSI layout, next-boot drive-number
  preview, separate native drives, and explicit eject versus disconnect.
- Shared draft-only control definitions; printer paper/format/folder,
  keyboard mapping and mouse motion/capture controls in Extensions.
- Complete next-boot diagnostics in Media, including controls previously
  behind Tinker. The four top-level tabs remain unchanged.
- General: individual RAM banks/speed and fixed/variable CPU clock;
  Advanced: DSP RAM, SCSI/RTC chips and NBIC, with model-specific choices
  and restart confirmation.

## Next interface work

- Migrate shortcut editing, custom numeric mouse scales, NFS/network detail
  and full NeXTdimension configuration.
- Replace legacy missing-file dialogs, alerts and config import/export.
- Match hardware-specific media restrictions and second-MO-drive guidance
  throughout F9; validate created images against supported guest formats.
- Provide a separate save-for-next-launch workflow for hardware changes.
- Run FFmpeg post-processing asynchronously; extend capture beyond GIF.
- Improve UI-only coverage on macOS/Windows and native file-dialog backends.

## Compatibility and distribution

- Validate a usable NeXTstep desktop and disk operations across machine
  variants; component tests alone do not establish OS compatibility.
- Grow ROM, CPU and device integration tests.
- Browser/Emscripten frontend (`web/` is currently a placeholder).
- Flatpak manifest and continuous integration builds.

## Upstream alignment

Keep emulator changes separate from desktop policy. `settings.c` owns apply
classification, `change.c` owns runtime subsystem changes, and overlay drawing,
media IO and UI persistence have their own modules. See [Development.md](Development.md).
