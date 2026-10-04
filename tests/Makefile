CC ?= cc
.DEFAULT_GOAL := all
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra
CPPFLAGS ?= -I../src -I../src/includes -I../src/cpu -I../src/debug \
	-I../src/softfloat -I../src/dsp -I../src/dimension -I../src/slirp \
	-I../src/gui-sdl -I..

SDL3_CFLAGS ?= $(shell pkg-config --cflags sdl3)
SDL3_LIBS ?= $(shell pkg-config --libs sdl3)
PCAP_CFLAGS ?= $(shell pkg-config --cflags libpcap 2>/dev/null)
PCAP_LIBS ?= $(shell pkg-config --libs libpcap 2>/dev/null)

PNG_CFLAGS ?= $(shell pkg-config --cflags libpng 2>/dev/null)
PNG_LIBS ?= $(shell pkg-config --libs libpng 2>/dev/null)

TESTS = test-str test-paste test-settings test-overlay test-devices test-devices-pcap test-input-media test-scsi test-hardware test-status test-sound test-grab test-printer

all: $(TESTS) test-headers test-capture

test-str: test_str.c ../src/str.c ../src/includes/str.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) test_str.c ../src/str.c -o $@

# Paste state machine (keymap calls are stubbed in the test).
test-paste: test_paste.c ../src/paste.c ../src/includes/paste.h \
	../src/gui-sdl/sdlkeymap.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) \
		test_paste.c ../src/paste.c $(SDL3_LIBS) -o $@

test-settings: test_settings.c ui_test_support.c ui_test_support.h ../src/settings.c ../src/change.c ../src/includes/settings.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -ffunction-sections -fdata-sections \
		test_settings.c ui_test_support.c ../src/settings.c ../src/change.c -Wl,--gc-sections -o $@

UI_SOURCES = ../src/overlay_input.c ../src/overlay_text.c ../src/overlay_devices.c ../src/overlay.c ../src/overlay_controls.c ../src/overlay_view.c ../src/overlay_media.c ../src/settings.c ../src/change.c

test-overlay: test_overlay.c ui_test_support.c ui_test_support.h $(UI_SOURCES) ../src/includes/overlay_devices.h ../src/includes/overlay.h ../src/includes/overlay_controls.h ../src/includes/ui_config.h ../src/includes/configuration.h ../src/includes/overlay_media.h ../src/includes/overlay_view.h ../src/includes/settings.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) $(PCAP_CFLAGS) -DTEST_OVERLAY -ffunction-sections -fdata-sections \
		test_overlay.c ui_test_support.c $(UI_SOURCES) $(SDL3_LIBS) $(PCAP_LIBS) -Wl,--gc-sections -o $@

test-devices: test_devices.c ui_test_support.c ui_test_support.h $(UI_SOURCES) ../src/includes/overlay_devices.h ../src/includes/configuration.h ../src/includes/ui_config.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) $(PCAP_CFLAGS) -DTEST_OVERLAY -ffunction-sections -fdata-sections \
		test_devices.c ui_test_support.c $(UI_SOURCES) $(SDL3_LIBS) $(PCAP_LIBS) -Wl,--gc-sections -o $@

# New input controls, native drive constraints, empty connections and ROM defaults.
test-input-media: test_input_media.c ui_test_support.c ui_test_support.h $(UI_SOURCES) ../src/shortcut.c ../src/includes/overlay_input.h ../src/includes/overlay_text.h ../src/includes/overlay_media.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) $(PCAP_CFLAGS) -DTEST_OVERLAY -ffunction-sections -fdata-sections \
		test_input_media.c ui_test_support.c $(UI_SOURCES) ../src/shortcut.c $(SDL3_LIBS) $(PCAP_LIBS) -Wl,--gc-sections -o $@

# Real SCSI sector reads/writes and guest-visible per-disk write protection.
test-scsi: test_scsi.c ../src/scsi.c ../src/file.c ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -ffunction-sections -fdata-sections \
		test_scsi.c ../src/file.c -Wl,--gc-sections -o $@

# Compile and exercise optional PCAP discovery without host capture privileges.
test-devices-pcap: test_devices.c pcap_test_support.c stubs/pcap.h ui_test_support.c $(UI_SOURCES) ../src/includes/overlay_devices.h ../src/includes/ui_config.h ../config.h
	$(CC) -Istubs $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -DHAVE_PCAP=1 -DTEST_PCAP_STUB -DTEST_OVERLAY -ffunction-sections -fdata-sections \
		test_devices.c pcap_test_support.c ui_test_support.c $(UI_SOURCES) $(SDL3_LIBS) -Wl,--gc-sections -o $@

# Overlay RAM choices checked against the real core for all seven models.
DEVICE_SOURCES = ../src/overlay_text.c ../src/overlay_devices.c ../src/overlay_media.c

test-hardware: test_hardware.c $(DEVICE_SOURCES) ../src/includes/overlay_devices.h ../src/configuration.c ../src/overlay_controls.c ../src/overlay_view.c ../src/includes/overlay_controls.h ../src/includes/configuration.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) $(PCAP_CFLAGS) -ffunction-sections -fdata-sections \
		test_hardware.c $(DEVICE_SOURCES) ../src/configuration.c ../src/overlay_controls.c ../src/overlay_view.c $(SDL3_LIBS) $(PCAP_LIBS) -Wl,--gc-sections -o $@

# Core status messages/LEDs and the retired shortcut, with threaded posting.
test-status: test_status.c ../src/notify.c ../src/gui-sdl/sdlstatusbar.c ../src/shortcut.c ../src/includes/notify.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -ffunction-sections -fdata-sections \
		test_status.c ../src/notify.c ../src/gui-sdl/sdlstatusbar.c ../src/shortcut.c $(SDL3_LIBS) -Wl,--gc-sections -o $@

# Audio byte order, double-rate modes, activity and timing with host sound off.
test-sound: test_sound.c ../src/snd.c ../src/includes/snd.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -ffunction-sections -fdata-sections \
		test_sound.c ../src/snd.c -Wl,--gc-sections -lm -o $@

# Real framebuffer conversion and PNG/TIFF writers, independent of host display.
test-grab: test_grab.c ../src/grab.c ../src/includes/grab.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) $(PNG_CFLAGS) -ffunction-sections -fdata-sections \
		test_grab.c ../src/grab.c -Wl,--gc-sections $(PNG_LIBS) -o $@

# Printer page replacement and finishing, including zero-width guest commands.
test-printer: test_printer.c ../src/printer.c ../src/includes/printer.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -ffunction-sections -fdata-sections \
		test_printer.c -Wl,--gc-sections -o $@

# Compile-time sanity: pulls in the core headers as compiled by the emulator.
test-headers: test_headers.c ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only test_headers.c

# Screenshot + GIF encoding exercise (gifcap, notify, capture, ffmpeg pass).
test-capture: test_capture.c ../src/gifcap.c ../src/notify.c ../src/capture.c \
	../src/ffmpeg_gif.c ../src/includes/gifcap.h ../src/includes/notify.h \
	../src/includes/capture.h ../src/includes/ffmpeg_gif.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) \
		test_capture.c ../src/gifcap.c ../src/notify.c ../src/capture.c \
		../src/ffmpeg_gif.c $(SDL3_LIBS) -lm -o $@

.PHONY: all check clean

check: $(TESTS) test-headers test-capture
	@set -e; for t in $(TESTS) test-capture; do ./$$t; done; echo "all tests passed"

clean:
	rm -f $(TESTS) test-capture test.gif 1989-*.ppm
