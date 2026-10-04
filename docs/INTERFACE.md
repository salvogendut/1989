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
| Fullscreen, title bar | Update the window without rebooting NeXT |
| Sound/microphone, Ethernet connection, tablet, printer connection | Update the affected host/peripheral subsystem; disks stay attached |
| Boot device, power-on diagnostics, verbose boot | Save for the next boot; the current OS keeps running |
| Media in an already connected removable drive | Exchange only the changed drive's media; other disks stay open |
| Model, CPU/FPU/DSP and DSP RAM, RAM banks/speed, SCSI/RTC chips, NBIC, active ROM, enabled NeXTdimension hardware, drive connections or SCSI device type | Require an explicit restart |
| Fixed SCSI hard-disk image, insertion/removal or write protection | Require an explicit restart |
| Legacy network backend or network-time source | Require an explicit restart |

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
| ROM | 68030, 68040 and Turbo ROM pickers in Advanced | Restore-default-path buttons |
| Boot | Media: boot device, power-on test master switch, DRAM/sound/SCSI tests, repeat/extended tests, diagnostic video and verbose boot | None |
| SCSI | Suggested roles and next-boot sdN preview; seven image slots; T disk/CD/floppy type; W protection; E eject; Delete disconnect; blank HDD/floppy images | Legacy browser and testing-only global disk-write overlay |
| Floppy | Drives 0/1, image selection, E eject, Delete disconnect, write protection, blank 720 KiB/1.44 MiB/2.88 MiB images | Connect an empty drive without loading media |
| Magneto-optical | Drives 0/1, image selection, E eject, Delete disconnect, write protection, blank-image creation | Connect an empty drive without media; legacy second-drive warning |
| Graphics | First NeXTdimension board enable; fullscreen, filtering, CRT and title bar | Boards in slots 4/6; per-board ROM/RAM; console/display slot; separate/grouped displays and monitor arrangement |
| Network | Connected/disconnected | SLiRP/pcap selection, host interface, twisted-pair selection, MAC address, network time, NFS shares and names |
| Sound | Output enable in General; microphone in Extensions | No additional sound-menu toggle |
| Keyboard | Clipboard paste; Extensions: scancode/symbolic mapping and Command/Alt swap | Configurable legacy shortcuts |
| Mouse/tablet | Extensions: tablet model, slow/fast motion presets, raw motion, automatic capture, wheel-to-arrow keys and Ctrl-click mapping; Ctrl+Enter release | Exact numeric linear/exponential scales |
| Printer | Extensions: connection, paper size, PNG/TIFF format and native output-folder picker | None |
| Main menu | About; model hardware defaults; confirmed restart/quit | Configuration import/export and show-legacy-menu-at-startup setting |

The m68k/i860 debuggers, legacy PNG/TIFF screenshots and AIFF recording remain
available through their shortcuts. Missing-ROM/media recovery still uses
legacy dialogs. These are separate migration tasks, not removed features.

The duplicate legacy status bar has been retired. The existing model/hint
strip shows configured CPU frequency and total RAM; the LEDs handle activity,
and core status messages use shared notifications. The old visibility setting
and status-bar shortcut keys are accepted for configuration compatibility,
then cleared. They cannot trigger window recreation or bring back the bar.

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
The legacy menu restricts MO to non-Turbo Cubes and warns about a second MO
drive; F9 does not yet reproduce all of those hardware-specific affordances.

Remaining interface work includes shortcut editing, exact numeric mouse
scales, NFS controls, richer
NeXTdimension configuration, replacing missing-file/legacy alerts, and moving
the optional FFmpeg post-processing pass off the UI thread. Browser/WASM is
still a placeholder. UI/component tests do not establish NeXTstep desktop or
disk-filesystem compatibility on every machine variant.
