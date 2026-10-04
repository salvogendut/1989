CC ?= cc
.DEFAULT_GOAL := all
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra
CPPFLAGS ?= -I../src -I../src/includes -I../src/cpu -I../src/debug \
	-I../src/softfloat -I../src/dsp -I../src/dimension -I../src/slirp \
	-I../src/gui-sdl -I..

SDL3_CFLAGS ?= $(shell pkg-config --cflags sdl3)
SDL3_LIBS ?= $(shell pkg-config --libs sdl3)

TESTS = test-str test-paste test-settings test-overlay

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

UI_SOURCES = ../src/overlay.c ../src/overlay_view.c ../src/overlay_media.c ../src/settings.c ../src/change.c

test-overlay: test_overlay.c ui_test_support.c ui_test_support.h $(UI_SOURCES) ../src/includes/overlay.h ../src/includes/overlay_media.h ../src/includes/overlay_view.h ../src/includes/settings.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SDL3_CFLAGS) -DTEST_OVERLAY -ffunction-sections -fdata-sections \
		test_overlay.c ui_test_support.c $(UI_SOURCES) $(SDL3_LIBS) -Wl,--gc-sections -o $@

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
