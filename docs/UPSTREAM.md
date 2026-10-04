# Previous upstream integration

1989 currently incorporates **Previous 4.4 plus development fixes through
SVN r1854**, from the SDL3 `branches/branch_filesharing` branch. This is a
pinned source snapshot, not a released Previous 4.5. Upstream's release
history calls the post-4.4 work “4.5 (unreleased)”.

- Repository: `https://svn.code.sf.net/p/previous/code`
- Imported tree: [branch_filesharing at r1854](https://sourceforge.net/p/previous/code/1854/tree/branches/branch_filesharing/)
- Revision: [r1854, 1 October 2026](https://sourceforge.net/p/previous/code/1854/)
- Comparison baseline: Previous 4.3, `trunk@1833`, matching the original
  core import before 1989's local adaptations.
- CPU version: WinUAE 6.1.0 beta16+ (30 September 2026), as recorded in
  `src/cpu/winuae_readme.txt`.

The similarly updated `branch_softfloat` uses SDL2. Its frontend is not the
source for this SDL3 application. The branch name does not mean this update
introduces a new file-sharing UI: the existing SLiRP/NFS integration remains.

## Included changes

| Area | Changes brought across |
| --- | --- |
| CPU/MMU/FPU | Cumulative WinUAE updates, 68030 exception/retry and FMOVEM fixes, MOVES accesses across page boundaries, cache-disabled retry behavior and debugger corrections |
| Display | More efficient monochrome pixel expansion and simpler color/framebuffer blitting |
| Sound | Revised buffering and sample conversion, proper CD de-emphasis, playback timing with host sound disabled, audio device/format event handling |
| DSP | 44.1 kHz serial-port input and 24-bit recording fixes |
| Ethernet DMA | Clear-complete requests only clear the completion flag on enabled DMA channels; includes the final r1851 correction, not the superseded r1850 workaround |
| SCSI | Known-drive geometry sizes calculated using `off_t` to avoid intermediate integer overflow |
| Image output | PNG/TIFF printer and legacy screenshot output, packed image formats, printer finalization on emulator exit |
| Utilities | Safe unaligned accesses in AIFF output, recording naming/reliability, string/history and legacy dialog fixes |

The display/audio work is relevant to smoothness, while the CPU/DMA changes
primarily improve correctness. No measured frame-rate or latency gain is
claimed. The SCSI arithmetic fix does not establish that every guest OS or
filesystem supports arbitrary large disks.

## Local adaptations

1989 keeps its Autotools build, branding, SDL3 overlay, activity indicators,
resource lookup and configuration paths. Upstream CMake/macOS app packaging
and the Previous version banner are not substituted for those integrations.
F9 draft/save/discard, restart confirmation and per-drive removable-media
application remain in place; importing these changes does not migrate SCSI
IDs or change a user's configuration or disk images.

- `Grab_FillBuffer` still supplies RGBA for F4 PPM and F6 GIF capture. It
  reuses the upstream conversion path with an explicit RGBA request, separate
  from the packed PNG/TIFF file format. It also works without libpng.
- The sound activity LED is updated through the new common sample-output
  path. Upstream's new single-sample serializer used left shifts that lost
  the high bytes; 1989 uses right shifts to preserve big-endian stereo data.
- Starting a zero-width printer page clears the previous page pointer and
  counters after freeing it. This prevents use of freed memory and division
  by zero during finalization in the imported printer implementation.
- The CPU generator has been run, including the three new 68030 CCR trace
  checks in `cpuemu_32.c`. Other generated CPU files were checked and did
  not change. Ordinary builds still use the checked-in generated files.

Printer format is available in **F9 → Extensions → Printer output** and `[Printer] nFileFormat`
(`0` PNG, `1` TIFF). It also selects the legacy screenshot format; without
libpng, output falls back to TIFF. F4/F6 keep their existing formats.
Printer format, paper size and the output directory are also available in
F9 → Extensions; see [the interface inventory](INTERFACE.md).

## Reproducing an update

Inspect a pinned upstream delta rather than overwriting the fork:

```sh
svn diff --ignore-properties \
  https://svn.code.sf.net/p/previous/code/trunk@1833 \
  https://svn.code.sf.net/p/previous/code/branches/branch_filesharing@1854
```

Review local changes in each affected file, particularly `main.c`, `grab.c`,
`snd.c`, `configuration.c`, `gui-sdl/sdlevent.c` and `gui-sdl/sdlscreen.c`.
Keep the generator and generated sources synchronized:

```sh
tools/regenerate-cpu.sh
tools/regenerate-cpu.sh --check
```

The script uses `cc`, or the executable named by `CC_FOR_BUILD`, and runs
generation in a temporary directory. `--check` compares without rewriting.
The generator inputs and new SDL audio header are included in `make dist`.

## Verification

The integration adds tests of real framebuffer conversion (monochrome,
color, Turbo strides, NeXTdimension and grouped displays), PNG/TIFF output,
single-sample audio byte order, repeat/zero-fill modes, guest mute and
playback timing with host sound disabled, plus printer page lifecycle.
Existing settings/overlay tests continue to cover restart confirmation,
live per-drive eject and preservation of other attached media.

Native Linux builds, the full test suite, CPU regeneration and a fresh
out-of-tree build from `make distdir` are the integration checks. Framebuffer
and TIFF tests also run with libpng disabled. A disk-free 68030 ROM startup
was smoke-tested with dummy SDL video/audio and an isolated configuration.
These checks do not validate
a complete running NeXTstep installation, physical audio-device hotplug,
or every CPU fault sequence fixed upstream.
