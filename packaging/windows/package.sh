#!/bin/sh
# Run in the MSYS2 MinGW shell after make.
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 OUTPUT_DIR" >&2
    exit 2
fi

output=$1
if [ -e "$output" ]; then
    echo "Windows bundle output already exists: $output" >&2
    exit 1
fi

mkdir -p "$output/roms" "$output/disks"
cp 1989.exe ditool.exe LICENSE README.md ROMS.md INSTALL.md USAGE.md \
    1989.conf.example "$output/"
cp roms/*.BIN roms/README "$output/roms/"
cp disks/*.zip "$output/disks/"

# ldd includes transitive dependencies. Copy only MinGW libraries; Windows
# system DLLs remain supplied by the OS. Inspect both shipped executables.
for executable in 1989.exe ditool.exe; do
    dependencies=$(ldd "$executable")
    if printf '%s\n' "$dependencies" | grep -q 'not found'; then
        printf '%s\n' "$dependencies" >&2
        exit 1
    fi
    printf '%s\n' "$dependencies" | awk '/\/mingw64\// {print $3}' |
        while IFS= read -r dll; do
            cp "$dll" "$output/"
        done
done

# The emulator has no --help mode. Exercise the CLI companion without
# launching a guest or opening any disk image.
"$output/ditool.exe" -h
