# WebAssembly edition (planned)

[Issue #3](https://github.com/salvogendut/1989/issues/3) tracks the browser
edition on branch `3-webassembly-next-theme`. This directory currently contains
the implementation brief; there is no working browser build yet, and native
`make` does not build it.

The target is a standalone, client-side WebAssembly build of the same
Previous-derived C/C++ core, with a static HTML/CSS/JavaScript interface based
on `../1984/web/`. The guest display must come from the running emulator.

## Themes and branding

Keep 1984's **Retro CRT**, **Sapporo**, and **Sapporo Dark** themes. Replace
its CPC464 theme with **NeXT**, which becomes 1989's default.

Use [the supplied hardware reference](../POC/IllustratorScreenshot.jpg) for
the black monitor, recessed screen, rounded bezel, pedestal, and horizontally
ribbed NeXTstation slab. Use the existing [1989-logo.png](../1989-logo.png)
on the monitor where the original has its NeXT logo, preserving the artwork
and proportions. The Illustrator desktop in the photograph is reference
content, not a replacement for the live framebuffer.

Retain the theme picker, case-insensitive `?theme=` selection, and saved
preferences, with storage namespaced to `javascript1989`. Unknown theme names
fall back to NeXT. Preserve the NeXT framebuffer's aspect ratio when resizing
or entering fullscreen.

## Media and keyboard

The media panel has four independent image loaders and mounted-image status:

| Device | Connection | Controls |
| --- | --- | --- |
| Hard disk | SCSI ID 1 | Load/replace with restart confirmation |
| CD-ROM | SCSI ID 3 | Load and eject without a hard reset |
| Floppy | Native floppy controller | Load and eject without a hard reset |
| Magneto-optical (MO) | Native MO controller | Load and eject without a hard reset |

These are exactly two SCSI devices plus the native floppy and MO drives.
Ejecting one removable image must preserve the other attached media. The
browser implementation must explain where guest writes are stored, how they
are saved/exported, and what survives a page reload; changed writable disks
must not be silently discarded.

Provide a **collapsible 1989/NeXT on-screen keyboard**, with the NeXT layout,
key labels, working modifier combinations, and pointer/touch interaction.
Physical keyboard and mouse input should also work. Showing or hiding the
keyboard must preserve emulation and media state and release any held keys.
Replace 1984's CPC-specific controls with controls appropriate to NeXT.

## Implementation order

1. Establish an Emscripten build and browser host adapters for scheduling,
   display, input, audio, configuration, and file access. The existing core's
   blocking loops and threads need explicit handling; 1984's small SDL shim
   is a reference, not an assumed drop-in replacement. Reach the ROM monitor
   and then boot a compatible user-supplied NEXTSTEP image.
2. Adapt the shared browser shell and themes, implement the NeXT enclosure
   and collapsible keyboard, and connect them to the real emulator. Keep
   theme/layout code separate from emulator and media adapters.
3. Implement the four media controls, removable-media eject, explicit reset
   confirmation, and documented persistence/export behavior.
4. Add browser and compiled-core tests, a GitHub Actions WASM artifact, and
   build/serve documentation. The intended `make -C web` output is a complete
   static `web/dist/` tree; document hosting headers if the chosen threading
   approach requires them. Validate responsive layouts and preserve the
   native builds.

See the issue for acceptance criteria and the [roadmap](../ROADMAP.md) for
the rest of the project. Hosted-site publication follows a validated browser
build and documented hosting requirements.
