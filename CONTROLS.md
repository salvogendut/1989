# Controls

1989 follows the "happy years" emulator conventions: an **F9 options
overlay** with tabbed sections, an activity **LED bar** at the bottom of the
window, and dedicated F-key shortcuts for screenshot, GIF capture and
fullscreen.

## Emulator function keys

| Key   | Action                                          |
|-------|-------------------------------------------------|
| `F1`  | Legacy options dialog (until fully migrated)    |
| `F4`  | Screenshot (PPM)                                |
| `F5`  | Reset                                           |
| `F6`  | Toggle GIF capture                              |
| `F9`  | Open / close the options overlay                |
| `F11` | Toggle fullscreen                               |
| `F12` | Quit                                            |

Quitting shows a confirmation in the same overlay style as the F9 panel:
`Left`/`Right` selects OK or Cancel, `Enter` confirms and `Esc` cancels.

The legacy configurable shortcuts still work (Alt+O opens the old options
dialog, Alt+G screenshot, Alt+R sound recording, Alt+M mouse grab, Alt+C
cold reset, Alt+P pause, Alt+D m68k debugger, Alt+I i860 debugger, Alt+Q
quit, Alt+B status bar, Alt+T title bar). They can be rebound in the options
dialog.

## Options overlay (F9)

The overlay opens with **F9** and is driven entirely from the keyboard:

| Key          | Action                                    |
|--------------|-------------------------------------------|
| `Left/Right` | Switch section (tab)                      |
| `Up/Down`    | Move the selection                        |
| `Enter`      | Toggle / choose the selected row          |
| `Esc` / `F9` | Close (settings are saved)                |

### General tab

Machine model (NeXT Computer, NeXTcube, NeXTcube Turbo, NeXTstation,
NeXTstation Turbo/Color/Turbo Color), RAM size, CPU clock, FPU, DSP, MMU,
ADB, boot device, the **Tinker** master switch (gates the Advanced tab), an
About box, and a "Reset defaults" action.

### Media tab

Boot device and the attached media: up to four SCSI targets, two floppy
drives and two magneto-optical drives. `Enter` on an empty media row opens a
native file picker; `Enter` on a filled row ejects the media.

### Extensions tab

NeXTdimension board, printer, Ethernet, tablet and microphone. Changes are
applied with a cold reset.

### Advanced tab

Shown only while **Tinker** is enabled in General:

- **Smoothing** — linear framebuffer filtering.
- **Real CRT** — scanline effect; when on, a **Scanlines** row (0–95 %)
  controls its visibility.
- **GIF resolution / GIF frame rate / GIF encoder** — capture settings; the
  encoder is either the built-in one or an FFmpeg optimize pass (when built
  with ffmpeg).
- **Notifications**, **Debugging** (terminal log output), **Fullscreen**,
  **Status bar**, **Title bar**, **DRAM test**, **Verbose boot** and an
  About box.

## LED bar

The dark strip at the very bottom of the window shows activity LEDs:

- **68K** grey/white CPU activity (with clock label)
- **DSP** blue — DSP 56001 host I/O
- **SCSI** red — SCSI disk transfers
- **FLOPPY** green — floppy controller I/O
- **MAG-OPT** cyan — magneto-optical drive activity
- **ETHERNET** yellow — network traffic
- **SOUND** purple — audio generation
- **NEXTDIM** orange — NeXTdimension rendering

An LED glows bright for a short time after each burst of activity, then fades
back to its idle colour.

## Function-key hint strip

A thin strip above the LED bar reminds you of the shortcuts, in the shared
sibling style (red machine name, grey `key=action` list):
`1989  F1=menu  F4=screenshot  F5=reset  F6=gif  F9=options  F11=fullscreen  F12=quit`.

## Mouse

While running, the emulated machine captures the host mouse. **Alt+M** grabs
and releases it (the cursor is released automatically whenever the F9 overlay
is open).