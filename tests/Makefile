CC ?= cc
.DEFAULT_GOAL := all
CFLAGS ?= -O2 -std=gnu99 -Wall -Wextra
CPPFLAGS ?= -I../src -I../src/includes -I../src/cpu -I../src/debug \
	-I../src/softfloat -I../src/dsp -I../src/dimension -I../src/slirp \
	-I../src/gui-sdl -I..

TESTS = test-str

all: $(TESTS) test-headers

test-str: test_str.c ../src/str.c ../src/includes/str.h ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) test_str.c ../src/str.c -o $@

# Compile-time sanity: pulls in the core headers as compiled by the emulator.
test-headers: test_headers.c ../config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only test_headers.c

.PHONY: all check clean

check: $(TESTS) test-headers
	@set -e; for t in $(TESTS); do ./$$t; done; echo "all tests passed"

clean:
	rm -f $(TESTS)