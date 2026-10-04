bin_PROGRAMS = 1989 ditool

# Man page — installed under $(mandir)/man1/, included in 'make dist'.
dist_man1_MANS = 1989.1

1989_SOURCES = \
	src/adb.c \
	src/bmap.c \
	src/cfgopts.c \
	src/change.c \
	src/configuration.c \
	src/cycInt.c \
	src/dma.c \
	src/enet_pcap.c \
	src/enet_slirp.c \
	src/esp.c \
	src/ethernet.c \
	src/file.c \
	src/floppy.c \
	src/grab.c \
	src/ioMem.c \
	src/ioMemTabNEXT.c \
	src/ioMemTabTurbo.c \
	src/kms.c \
	src/m68000.c \
	src/main.c \
	src/host_dialog.c \
	src/recovery.c \
	src/mo.c \
	src/nbic.c \
	src/ncc.c \
	src/NextBus.cpp \
	src/paths.c \
	src/printer.c \
	src/queue.c \
	src/ramdac.c \
	src/reset.c \
	src/rom.c \
	src/rs.c \
	src/rtcnvram.c \
	src/scandir.c \
	src/scc.c \
	src/scsi.c \
	src/shortcut.c \
	src/snd.c \
	src/str.c \
	src/sysReg.c \
	src/tablet.c \
	src/timing.c \
	src/tmc.c \
	src/video.c \
	src/overlay.c \
	src/overlay_controls.c \
	src/overlay_devices.c \
	src/overlay_input.c \
	src/overlay_text.c \
	src/overlay_media.c \
	src/overlay_view.c \
	src/ui_config.c \
	src/settings.c \
	src/leds.c \
	src/notify.c \
	src/paste.c \
	src/gifcap.c \
	src/ffmpeg_gif.c \
	src/capture.c \
	src/gui-sdl/sdlaudio.c \
	src/gui-sdl/sdlevent.c \
	src/gui-sdl/sdlhost.c \
	src/gui-sdl/sdlkeymap.c \
	src/gui-sdl/sdlscreen.c \
	src/gui-sdl/sdlstatusbar.c \
	src/debug/68kDisass.c \
	src/debug/breakcond.c \
	src/debug/debugcpu.c \
	src/debug/debugdsp.c \
	src/debug/debugInfo.c \
	src/debug/debugui.c \
	src/debug/evaluate.c \
	src/debug/history.c \
	src/debug/log.c \
	src/debug/profile.c \
	src/debug/profilecpu.c \
	src/debug/profiledsp.c \
	src/debug/symbols.c \
	src/debug/vars.c \
	src/softfloat/softfloat.c \
	src/softfloat/softfloat_decimal.c \
	src/softfloat/softfloat_fpsp.c \
	src/dsp/dsp.c \
	src/dsp/dsp_core.c \
	src/dsp/dsp_cpu.c \
	src/dsp/dsp_disasm.c \
	src/dimension/dimension.cpp \
	src/dimension/i860.cpp \
	src/dimension/i860dis.cpp \
	src/dimension/nd_devs.cpp \
	src/dimension/nd_mem.cpp \
	src/dimension/nd_nbic.cpp \
	src/dimension/nd_rom.cpp \
	src/dimension/nd_sdl.cpp \
	src/dimension/nd_vio.cpp \
	src/slirp/bootp.c \
	src/slirp/cksum.c \
	src/slirp/if.c \
	src/slirp/ip_icmp.c \
	src/slirp/ip_input.c \
	src/slirp/ip_output.c \
	src/slirp/mbuf.c \
	src/slirp/misc.c \
	src/slirp/ntp.c \
	src/slirp/rip.c \
	src/slirp/sbuf.c \
	src/slirp/slirp.c \
	src/slirp/slirpdebug.c \
	src/slirp/socket.c \
	src/slirp/tcp_input.c \
	src/slirp/tcp_output.c \
	src/slirp/tcp_subr.c \
	src/slirp/tcp_timer.c \
	src/slirp/tftp.c \
	src/slirp/udp.c \
	src/slirp/rpc/bootparam.c \
	src/slirp/rpc/csocket.c \
	src/slirp/rpc/dns.c \
	src/slirp/rpc/filetable.c \
	src/slirp/rpc/mount.c \
	src/slirp/rpc/netinfo.c \
	src/slirp/rpc/netinfobind.c \
	src/slirp/rpc/nfs.c \
	src/slirp/rpc/portmap.c \
	src/slirp/rpc/rpc.c \
	src/slirp/rpc/tcpsocket.c \
	src/slirp/rpc/udpsocket.c \
	src/slirp/rpc/vfs.c \
	src/slirp/rpc/xdr.c \
	src/cpu/cpudefs.c \
	src/cpu/cpuemu_31.c \
	src/cpu/cpuemu_32.c \
	src/cpu/cpummu.c \
	src/cpu/cpummu030.c \
	src/cpu/cpustbl.c \
	src/cpu/custom.c \
	src/cpu/debug.c \
	src/cpu/disasm.c \
	src/cpu/events.c \
	src/cpu/fpp.c \
	src/cpu/fpp_softfloat.c \
	src/cpu/hatari-glue.c \
	src/cpu/machdep/m68k.c \
	src/cpu/memory.c \
	src/cpu/newcpu.c \
	src/cpu/newcpu_common.c \
	src/cpu/readcpu.c \
	src/cpu/writelog.c

if OS_MACOS
1989_SOURCES += src/gui-osx/paths.m
endif
if OS_WINDOWS
1989_SOURCES += src/gui-win/opencon.c
endif

ditool_SOURCES = \
	src/ditool/ditool.c \
	src/ditool/im.c \
	src/ditool/netboot.c \
	src/ditool/part.c \
	src/ditool/ufs.c \
	src/rs.c \
	src/slirp/rpc/vfs.c

noinst_HEADERS = \
	src/gui-win/opencon.h \
	src/cpu/compat.h \
	src/cpu/cpummu030.h \
	src/cpu/cpummu.h \
	src/cpu/cpu_prefetch.h \
	src/cpu/cputbl.h \
	src/cpu/custom.h \
	src/cpu/debug.h \
	src/cpu/debugmem.h \
	src/cpu/disasm.h \
	src/cpu/events.h \
	src/cpu/fpp.h \
	src/cpu/hatari-glue.h \
	src/cpu/maccess.h \
	src/cpu/machdep/m68k.h \
	src/cpu/machdep/rpt.h \
	src/cpu/memory.h \
	src/cpu/mmu_common.h \
	src/cpu/newcpu.h \
	src/cpu/options_cpu.h \
	src/cpu/readcpu.h \
	src/cpu/savestate.h \
	src/cpu/sysconfig.h \
	src/cpu/sysdeps.h \
	src/cpu/uae/attributes.h \
	src/cpu/uae/likely.h \
	src/cpu/uae/string.h \
	src/cpu/uae/time.h \
	src/cpu/uae/types.h \
	src/cpu/uae/vm.h \
	src/debug/68kDisass.h \
	src/debug/breakcond.h \
	src/debug/debugcpu.h \
	src/debug/debugdsp.h \
	src/debug/debugInfo.h \
	src/debug/debug_priv.h \
	src/debug/debugui.h \
	src/debug/evaluate.h \
	src/debug/history.h \
	src/debug/log.h \
	src/debug/profile.h \
	src/debug/symbols.h \
	src/debug/vars.h \
	src/dimension/dimension.hpp \
	src/dimension/i860cfg.h \
	src/dimension/i860.hpp \
	src/dimension/nd_mem.hpp \
	src/dimension/nd_nbic.hpp \
	src/dimension/nd_sdl.hpp \
	src/ditool/fsdir.h \
	src/ditool/fs.h \
	src/ditool/im.h \
	src/ditool/inode.h \
	src/ditool/netboot.h \
	src/ditool/part.h \
	src/ditool/ufs.h \
	src/dsp/dsp_core.h \
	src/dsp/dsp_cpu.h \
	src/dsp/dsp_disasm.h \
	src/dsp/dsp.h \
	src/gui-sdl/sdlaudio.h \
	src/gui-sdl/sdlkeymap.h \
	src/gui-sdl/sdlscreen.h \
	src/includes/adb.h \
	src/includes/audio.h \
	src/includes/bmap.h \
	src/includes/cfgopts.h \
	src/includes/change.h \
	src/includes/configuration.h \
	src/includes/cycInt.h \
	src/includes/dma.h \
	src/includes/endianswap.h \
	src/includes/enet_pcap.h \
	src/includes/enet_slirp.h \
	src/includes/esp.h \
	src/includes/ethernet.h \
	src/includes/event.h \
	src/includes/file_archive.h \
	src/includes/file.h \
	src/includes/floppy.h \
	src/includes/grab.h \
	src/includes/host.h \
	src/includes/ioMem.h \
	src/includes/ioMemTables.h \
	src/includes/keymap.h \
	src/includes/kms.h \
	src/includes/m68000.h \
	src/includes/main.h \
	src/includes/host_dialog.h \
	src/includes/recovery.h \
	src/includes/mo.h \
	src/includes/nbic.h \
	src/includes/ncc.h \
	src/includes/NextBus.hpp \
	src/includes/paths.h \
	src/includes/printer.h \
	src/includes/queue.h \
	src/includes/ramdac.h \
	src/includes/reset.h \
	src/includes/rom.h \
	src/includes/rs.h \
	src/includes/rtcnvram.h \
	src/includes/scandir.h \
	src/includes/scc.h \
	src/includes/screen.h \
	src/includes/scsi.h \
	src/includes/shortcut.h \
	src/includes/snd.h \
	src/includes/statusbar.h \
	src/includes/str.h \
	src/includes/sysReg.h \
	src/includes/tablet.h \
	src/includes/timing.h \
	src/includes/tmc.h \
	src/includes/video.h \
	src/includes/vs-fix.h \
	src/includes/overlay.h \
	src/includes/overlay_controls.h \
	src/includes/overlay_devices.h \
	src/includes/overlay_input.h \
	src/includes/overlay_text.h \
	src/includes/leds.h \
	src/includes/notify.h \
	src/includes/paste.h \
	src/includes/gifcap.h \
	src/includes/ffmpeg_gif.h \
	src/includes/capture.h \
	src/slirp/slirp_log.h \
	src/slirp/bootp.h \
	src/slirp/ctl.h \
	src/slirp/icmp_var.h \
	src/slirp/if.h \
	src/slirp/ip.h \
	src/slirp/ip_icmp.h \
	src/slirp/libslirp.h \
	src/slirp/mbuf.h \
	src/slirp/misc.h \
	src/slirp/nfs/BootparamProg.h \
	src/slirp/nfs/compat.h \
	src/slirp/nfs/CSocket.h \
	src/slirp/nfs/FileTableNFSD.h \
	src/slirp/nfs/MountProg.h \
	src/slirp/nfs/NetInfoBindProg.h \
	src/slirp/nfs/NetInfoProg.h \
	src/slirp/nfs/NFS2Prog.h \
	src/slirp/nfs/nfsd.h \
	src/slirp/nfs/NFSProg.h \
	src/slirp/nfs/PortmapProg.h \
	src/slirp/nfs/RPCProg.h \
	src/slirp/nfs/RPCServer.h \
	src/slirp/nfs/SocketListener.h \
	src/slirp/nfs/TCPServerSocket.h \
	src/slirp/nfs/UDPServerSocket.h \
	src/slirp/nfs/VDNS.h \
	src/slirp/nfs/XDRStream.h \
	src/slirp/ntp.h \
	src/slirp/rip.h \
	src/slirp/rpc/bootparam.h \
	src/slirp/rpc/csocket.h \
	src/slirp/rpc/dns.h \
	src/slirp/rpc/filetable.h \
	src/slirp/rpc/mount.h \
	src/slirp/rpc/netinfobind.h \
	src/slirp/rpc/netinfo.h \
	src/slirp/rpc/nfs.h \
	src/slirp/rpc/portmap.h \
	src/slirp/rpc/rpc.h \
	src/slirp/rpc/tcpsocket.h \
	src/slirp/rpc/udpsocket.h \
	src/slirp/rpc/vfs.h \
	src/slirp/rpc/xdr.h \
	src/slirp/sbuf.h \
	src/slirp/slirp_config.h \
	src/slirp/slirpdebug.h \
	src/slirp/slirp.h \
	src/slirp/slirpmain.h \
	src/slirp/socket.h \
	src/slirp/tcp.h \
	src/slirp/tcpip.h \
	src/slirp/tcp_timer.h \
	src/slirp/tcp_var.h \
	src/slirp/tftp.h \
	src/slirp/udp.h \
	src/softfloat/softfloat_fpsp_tables.h \
	src/softfloat/softfloat.h \
	src/softfloat/softfloat-macros.h \
	src/softfloat/softfloat-specialize.h \
	src/includes/settings.h \
	src/includes/overlay_view.h \
	src/includes/ui_config.h \
	src/includes/overlay_media.h

# Common include path for the whole Previous-derived core
AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/includes \
	-I$(top_srcdir)/src/cpu \
	-I$(top_srcdir)/src/debug \
	-I$(top_srcdir)/src/softfloat \
	-I$(top_srcdir)/src/dsp \
	-I$(top_srcdir)/src/dimension \
	-I$(top_srcdir)/src/slirp \
	-I$(top_srcdir)/src/gui-sdl \
	-I$(top_builddir)

1989_CPPFLAGS = $(AM_CPPFLAGS) $(SDL3_CFLAGS) $(PNG_CFLAGS) $(PCAP_CFLAGS) $(READLINE_CFLAGS)
1989_CFLAGS = $(AM_CFLAGS) -Wall -Wextra \
	-Wno-unused-variable -Wno-unused-function -Wno-unused-label \
	-Wno-sign-compare -Wno-implicit-fallthrough -Wno-conversion \
	-Wno-missing-braces -Wno-unused-but-set-variable -Wno-shadow=local \
	-fwrapv \
	-DROM_INSTALL_DIR=\"$(pkgdatadir)/roms\" \
	-DCONFDIR=\"$(sysconfdir)\" \
	-DPROG_GIT_COMMIT=\"$(PROG_GIT_COMMIT)\"
1989_CXXFLAGS = $(AM_CXXFLAGS) -Wall -Wextra -Wno-unused-parameter \
	-fwrapv \
	-DROM_INSTALL_DIR=\"$(pkgdatadir)/roms\" \
	-DCONFDIR=\"$(sysconfdir)\" \
	-DPROG_GIT_COMMIT=\"$(PROG_GIT_COMMIT)\"
1989_LDADD = $(SDL3_LIBS) $(PNG_LIBS) $(PCAP_LIBS) $(READLINE_LIBS) -lm $(MAC_LIBS) $(WIN_LIBS) $(WIN_RC_OBJ)
1989_DEPENDENCIES = $(WIN_RC_OBJ)

ditool_CPPFLAGS = -I$(top_srcdir)/src/ditool -I$(top_srcdir)/src/includes \
	-I$(top_srcdir)/src/slirp -I$(top_builddir)
ditool_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -Wno-unused-but-set-variable
ditool_LDADD = -lm $(WIN_LIBS)

icons/1989-rc.o: $(srcdir)/icons/1989.rc $(srcdir)/icons/1989.ico
	$(WINDRES) -I$(srcdir)/icons -i $< -O coff -o $@

# Machine ROMs: installed under $(pkgdatadir)/roms. The ROM images are
# distributed with the upstream Previous project; a guidance file is installed
# alongside them.
romsdir       = $(pkgdatadir)/roms
dist_roms_DATA = roms/README roms/ND_step1_v43.BIN roms/Rev_0.8_v31.BIN \
	roms/Rev_1.0_v41.BIN roms/Rev_2.5_v66.BIN roms/Rev_3.3_v74.BIN

# Empty floppy/hard-disk images shipped for convenience
disksdir = $(pkgdatadir)/disks
dist_disks_DATA = \
	disks/empty105.sd.zip disks/empty1400.sd.zip disks/empty1440.fd.zip \
	disks/empty250.sd.zip disks/empty2800.sd.zip disks/empty2880.fd.zip \
	disks/empty330.sd.zip disks/empty349x.sd.zip disks/empty400.sd.zip \
	disks/empty660.sd.zip disks/empty660x.sd.zip disks/empty720.fd.zip \
	disks/empty.ecc.od.zip

# Desktop integration: .desktop launcher, AppStream metainfo.
desktopdir         = $(datadir)/applications
dist_desktop_DATA  = io.github.salvogendut.Emulator1989.desktop

metainfodir        = $(datadir)/metainfo
dist_metainfo_DATA = io.github.salvogendut.Emulator1989.metainfo.xml

# Icons: hicolor PNG set (generated from the bitmap shipped by Previous) plus
# the original large bitmap for reference.
icon16dir          = $(datadir)/icons/hicolor/16x16/apps
dist_icon16_DATA   = icons/16x16/apps/io.github.salvogendut.Emulator1989.png
icon32dir          = $(datadir)/icons/hicolor/32x32/apps
dist_icon32_DATA   = icons/32x32/apps/io.github.salvogendut.Emulator1989.png
icon48dir          = $(datadir)/icons/hicolor/48x48/apps
dist_icon48_DATA   = icons/48x48/apps/io.github.salvogendut.Emulator1989.png
icon64dir          = $(datadir)/icons/hicolor/64x64/apps
dist_icon64_DATA   = icons/64x64/apps/io.github.salvogendut.Emulator1989.png
icon128dir         = $(datadir)/icons/hicolor/128x128/apps
dist_icon128_DATA  = icons/128x128/apps/io.github.salvogendut.Emulator1989.png
icon256dir         = $(datadir)/icons/hicolor/256x256/apps
dist_icon256_DATA  = icons/256x256/apps/io.github.salvogendut.Emulator1989.png
icon512dir         = $(datadir)/icons/hicolor/512x512/apps
dist_icon512_DATA  = icons/512x512/apps/io.github.salvogendut.Emulator1989.png

pixmapsdir       = $(datadir)/pixmaps
dist_pixmaps_DATA = icons/1989.png

# Files shipped in the dist tarball but not built (license, docs, examples).
EXTRA_DIST = LICENSE README.md ROMS.md Development.md USAGE.md CONTROLS.md INSTALL.md \
	ROADMAP.md docs/STATUS.md docs/INTERFACE.md docs/UPSTREAM.md docs/ROMV66-0001E-02588.ASM \
	1989.conf.example 1989.spec 1989.png 1989-logo.png \
	roms/README \
	icons/1989.rc icons/1989.ico \
	tools/regenerate-cpu.sh src/cpu/build68k.c src/cpu/gencpu.c src/cpu/table68k \
	src/cpu/winuae_readme.txt \
	src/dimension/i860dec.cpp src/dimension/i860dbg.cpp \
	src/ditool_cpp/compat.h \
	src/ditool_cpp/DiskImage.h \
	src/ditool_cpp/ditool.h \
	src/ditool_cpp/fsdir.h \
	src/ditool_cpp/fs.h \
	src/ditool_cpp/inode.h \
	src/ditool_cpp/Partition.h \
	src/ditool_cpp/UFS.h \
	src/ditool_cpp/VirtualFS.h \
	packaging/debian/build-deb.sh packaging/macos/package.sh packaging/windows/package.sh \
	packaging/io.github.salvogendut.Emulator1989.yml \
	web/README.md web/Makefile web/config.h web/web_host.h web/web_host.c \
	web/assemble.py web/serve.py web/app.js web/shell.js web/keyboard.js web/media.js web/models.js \
	web/runtime.css web/package.json web/package-lock.json web/test-unit.js web/test-browser.js \
	web/preview/index.html web/preview/preview.css web/preview/preview.js \
	web/preview/themes/theme-retro-crt.css web/preview/themes/theme-sapporo.css \
	web/preview/themes/theme-sapporo-dark.css web/preview/themes/preview-controls.css \
	POC/IllustratorScreenshot.jpg \
	tests/Makefile tests/test_str.c tests/test_headers.c tests/test_capture.c tests/test_paste.c tests/test_settings.c tests/test_overlay.c tests/test_devices.c tests/test_input_media.c tests/test_scsi.c tests/test_recovery.c tests/pcap_test_support.c tests/stubs/pcap.h \
	tests/ui_test_support.c tests/ui_test_support.h tests/test_hardware.c tests/test_status.c tests/test_sound.c tests/test_grab.c tests/test_printer.c tests/test_rom_paths.c
