CC ?= cc
.DEFAULT_GOAL := all
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra
CPPFLAGS ?= -I../src -I../src/includes -I../src/cpu -I../src/debug \
	-I../src/softfloat -I../src/dsp -I../src/dimension -I../src/slirp \
	-I../src/gui-sdl -I..

SDL3_CFLAGS ?= $(shell pkg-config --cflags sdl3)
SDL3_LIBS ?= $(shell pkg-config --libs sdl3)

TESTS = test-str

all: $(TESTS) test-headers test-capture

test-str: test_str.c ../src/str.c ../src/includes/str.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) test_str.c ../src/str.c -o $@

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