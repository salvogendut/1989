# Desktop interface and legacy-menu migration

The F9 overlay is the primary interface. F1 opens **1989 – Legacy options**
for the controls listed below that have not yet been migrated. The old menu
is still compiled and functional; migration is incomplete.
[Issue #1](https://github.com/salvogendut/1989/issues/1) tracks its retirement.
Controls are being distributed among the existing four tabs.

## Applying changes

F9 edits private copies of machine and UI settings. The guest continues
running while you browse. Nothing selected in the panel is applied until
**Save** or **Restart** is confirmed. **Discard** drops the draft; **Esc** in
the confirmation returns to editing. Changing a value and changing it back
produces no save prompt or restart.

| Change | Effect when confirmed |
| --- | --- |
| Smoothing, CRT, notifications, GIF settings, debug output, RTC local/UTC | Apply without a machine restart |
| Fullscreen, title bar, single/separate/grouped display mode, shown display and group positions | Update the windows without rebooting NeXT |
| Sound/microphone, Ethernet connection/cable, PCAP interface, NFS folders/names, tablet, printer connection | Update the affected host/peripheral subsystem; disks stay attached |
| Boot device, power-on diagnostics, verbose boot | Save for the next boot; the current OS keeps running |
| Media in an already connected removable drive | Exchange only the changed drive's media; other disks stay open |
| Model, CPU/FPU/DSP and DSP RAM, RAM banks/speed, SCSI/RTC chips, NBIC, active ROM, enabled NeXTdimension hardware/boot console, drive connections or SCSI device type | Require an explicit restart |
| Fixed SCSI hard-disk image, insertion/removal or write protection | Require an explicit restart |
| Network backend, network-time source or active custom MAC | Require an explicit restart |

ROM paths for inactive machine variants and disabled NeXTdimension boards can
be saved without restarting.
The MMU row is informational: Previous's CPU setup always enables it.
Inherited MMU/CPU/FPU compatibility flags that the core ignores do not
request a restart. ADB can be changed only on Turbo models.
RAM choices match the selected model, including the two-bank limit on
non-Turbo monochrome NeXTstations. NBIC and NeXTdimension are Cube-only;
NBIC shows as required while any board is enabled and cannot be disabled.
The core enables NBIC when the board change is confirmed. Toggling a board
on and back off leaves the draft's independent NBIC preference intact.

Shut down NeXT inside the guest before confirming a hardware/fixed-disk
restart or quitting the emulator. Eject/unmount removable media inside the
guest before changing it. A confirmation does not flush the guest OS's
filesystem caches. F5 and Ctrl+Alt+C now use the overlay restart confirmation,
with **Cancel** selected initially. Restart-requiring F9 saves initially
select **Discard**, and Esc lets you continue editing.

F1 uses the same restart policy and per-drive media application. Its media
selections are staged until the main dialog's OK; Cancel leaves attached
media unchanged. Its explicit **Save config** button still writes a file,
and its **Load config** button can change the active configuration filename.
F1 remains a blocking legacy dialog, with its own alerts and file browser.

There is no separate queue of hardware changes for a future launch. Save
boot options at any time; shut down the guest before editing hardware that
requires restarting. For preparing another machine entirely offline, edit
its configuration while the emulator is closed.

## Coverage against the legacy dialogs

This inventory is based on `src/gui-sdl/dlg*.c` and the F9 row/actions code.
“Remaining” means absent from F9, not absent from the emulation core.

| Legacy area | Available in F9 | Remaining in F1 |
| --- | --- | --- |
| System | General: seven model variants, 16/20/25/33 MHz (40 on Turbo), fixed/variable clock, FPU, DSP mode, MMU status, Turbo ADB and model defaults; Advanced: DSP RAM, SCSI/RTC chip and Cube NBIC selection | None of the effective system controls |
| Memory | General: model-appropriate total-RAM presets, individual bank editing and controller-specific memory speed | None |
| ROM | 68030, 68040 and Turbo ROM pickers in Advanced; D restores discovered defaults | None |
| Boot | Media: boot device, power-on test master switch, DRAM/sound/SCSI tests, repeat/extended tests, diagnostic video and verbose boot | None |
| SCSI | Suggested roles and next-boot sdN preview; seven image slots; T disk/CD/floppy type; W protection; E eject; Delete disconnect; blank HDD/floppy images | Temporary write overlay retired; legacy browser remains for recovery |
| Floppy | Native drive 0 on 68040 models, image selection, C connects empty, E eject, Delete disconnect, write protection, blank 720 KiB/1.44 MiB/2.88 MiB images; imported drive 1 can be removed | None of the supported hardware controls |
| Magneto-optical | Drives 0/1 on non-Turbo Cubes, image selection, C connects empty, E eject, Delete disconnect, write protection, blank-image creation and second-drive warning | None |
| Graphics | Extensions: slot-2 quick toggle; Advanced: slots 2/4/6, per-board ROM/RAM/defaults, boot console, single/separate/grouped displays, shown slot and 4 × 4 monitor arrangement; fullscreen, filtering, CRT and title bar | None of the effective controls (i860 threading is chosen automatically by the core) |
| Network | Extensions: connection plus Network/NFS detail page with SLiRP/optional PCAP, host interface, cable, ROM/custom MAC, network time, four NFS folders and names | None |
| Sound | Output enable in General; microphone in Extensions | No additional sound-menu toggle |
| Keyboard | Clipboard paste; Extensions: scancode/symbolic mapping, Command/Alt swap and Input details for shortcut bindings | None |
| Mouse/tablet | Extensions: tablet model, slow/fast motion presets, numeric linear/exponential scales, raw motion, automatic capture, wheel-to-arrow keys and Ctrl-click mapping; Ctrl+Enter release | None |
| Printer | Extensions: connection, paper size, PNG/TIFF format and native output-folder picker | None |
| Main menu | About; model hardware defaults; confirmed restart/quit | Configuration import/export; show-menu-at-startup retired |

The m68k/i860 debuggers, legacy PNG/TIFF screenshots and AIFF recording remain
available through their shortcuts. Missing-ROM/media recovery still uses
legacy dialogs. These are separate migration tasks, not removed features.

## Retired options

The duplicate legacy status bar has been retired. The existing model/hint
strip shows configured CPU frequency and total RAM; the LEDs handle activity,
and core status messages use shared notifications. The old visibility setting
and status-bar shortcut keys are accepted for configuration compatibility,
then cleared. They cannot trigger window recreation or bring back the bar.

**Show menu at startup** is retired: its old key is accepted but cleared, and
startup proceeds directly to emulation after checking required files.
Missing-file recovery remains available.

The testing-only **temporary SCSI write overlay** and its shadow-sector
storage have been removed. On configuration load, an old `[HardDisk]`
`nWriteProtection = 1` makes each configured SCSI target read-only, then
clears the global flag. This preserves disk protection without pretending
that temporary guest writes are still supported. Per-drive protection is
still editable with W; writable images receive persistent writes. Changing
a fixed disk's protection requires a confirmed restart after guest shutdown.
Saved configurations retain the obsolete keys as false/zero for compatibility.

## Input and native media details

Extensions → **Input details** edits exact linear/exponential mouse scales
and all effective legacy shortcut actions, with separate Ctrl+Alt and plain
bindings. Reserved desktop keys and duplicate assignments are rejected.
Text/key capture stays within the draft; Save applies without resetting the
machine. Esc cancels an edit or returns to Extensions. The fixed F4/F6/F9
desktop shortcuts remain available independently of legacy bindings.

Media's **C** connects an empty native drive. Native floppy 0 requires a
68040 model; MO drives require a non-Turbo Cube. These checks apply to new
connections and selected images, with validation before saving a model
change. Unsupported imported entries can still be ejected/disconnected;
unused legacy floppy controller slots 2/3 are preserved without blocking
Save because they are not exposed by the overlay. Connecting the second MO
drive includes the legacy NEXTSTEP kernel-crash warning in the existing
restart confirmation. Ejection keeps the drive connected and needs no reset.

**D** on a machine or NeXTdimension ROM row restores a default using the same
resource lookup as startup. Missing defaults leave the selection unchanged.
Saving a changed active ROM requires restart confirmation; inactive ROM
preferences do not.

## Network/NFS and NeXTdimension detail pages

Extensions → **Network / NFS** and Advanced → **NeXTdimension / displays**
keep these larger sets of controls within the existing tabs. Esc goes back
without dropping the draft; F9 opens the usual Save/Discard confirmation.
Text editing has its own Enter/Esc boundary and does not send keystrokes to
the guest. Native folder/ROM selections share the existing cancellable picker.

NFS exports are SLiRP-only. Share 0 keeps `nfs.home`; shares 1–3 accept unique
DNS labels, with `.home` supplied by SLiRP. `nfs`, `dns` and `previous` are
reserved. Delete on an export folder disables it. Unmount the export in the
guest first. Save restarts network services without resetting the CPU or
reopening disks. MAC editing changes only the final three bytes; the ROM
prefix remains intact. PCAP interface choices depend on build support and
host capture permissions; an unavailable backend is labelled explicitly.

Board hardware edits require restart confirmation. Disconnected boards'
ROM/RAM preferences can be saved without a reset. Board bank 0 must contain
4 or 16 MB; the other banks also support empty. Boot-console selection uses
the main display or a connected board. The page warns when the console board
exceeds 32 MB, matching the legacy ROM-compatibility guidance.

Display selection, mode and group positions update the display subsystem
without restarting NeXT. Grouped mode needs at least two visible monitors in
different cells of the 4 × 4 grid. Positions can be prepared before selecting
the mode. Save validation leaves invalid drafts open for correction, and
Discard remains available. Board connect/disconnect cycles do not change
NBIC, console or layout preferences as hidden side effects.

## SCSI layout and drive numbers

Media labels follow the suggested assignments in [NeXT Hardware Service,
Appendix C, page 153](https://www.nextcomputers.org/NeXTfiles/Docs/Hardware/NeXTServiceManualPages1-160_OCR.pdf).
These are conventions, not enforced device restrictions:

| SCSI ID | Suggested use |
| --- | --- |
| 0 | Alternate/external boot disk |
| 1 | Internal system disk |
| 2 | Data disk |
| 3 | CD-ROM |
| 4 | External SCSI floppy |
| 5 | Spare device |
| 6 | Extra/swap disk |
| 7 | Host controller, reserved and not selectable |

Opening Media changes nothing. Existing devices keep their IDs and types.
When loading an unused slot, F9 suggests CD-ROM at ID 3, SCSI floppy at ID 4
and HDD elsewhere. T overrides the type. Native floppy and MO entries use
separate controllers; neither is SCSI ID 4 or 5.

NEXTSTEP assigns drive numbers in ascending SCSI-ID order, starting at zero;
the factory system disk was normally ID 1. This is described in [Installing
and Configuring NEXTSTEP 3.3, “SCSI IDs and Drive Numbers,” page 7](https://bitsavers.org/pdf/next/Installing_and_Configuring_NeXTSTEP_Release_3.3_1994.pdf).
Thus IDs 1/2/3/6 map to sd0/sd1/sd2/sd3; IDs 1/6 alone map to sd0/sd1.
Adding ID 0 shifts higher-ID drives up. The selected-row preview uses the
draft's connected drives, including empty removable drives, to describe the
expected next boot; it does not read device names from the running OS.

**E ejects only removable media**, preserving the drive and its SCSI ID.
Close F9 and Save to apply it without a reset or reopening other disks.
Enter selects a replacement image. **Delete disconnects the drive** and
requires restart confirmation. Ejection alone does not require a restart;
other pending hardware edits still follow the normal restart policy.

## Media creation and remaining polish

New images are blank files, not formatted NeXT filesystems. The guest or
`ditool` must prepare them. Creation uses a sparse file where the host
filesystem supports holes, avoiding a long UI-blocking write of gigabytes
of zeros. Existing files are never overwritten. Discarding an options edit
does not delete an image already created on the host.

The offered MO image sizes are file-size choices, not a compatibility claim
for every format. The Previous MO core expects its own sector/ECC layout;
use a known-compatible image or the supplied `empty.ecc.od.zip` template.
F9 restricts native MO connections to non-Turbo Cubes and carries the legacy
second-drive warning; those checks do not validate an image's sector layout.

Remaining interface work includes configuration import/export,
replacing missing-file/legacy alerts, and moving
the optional FFmpeg post-processing pass off the UI thread. Browser/WASM is
still a placeholder. UI/component tests do not establish NeXTstep desktop or
disk-filesystem compatibility on every machine variant.
