#!/bin/sh
# Rebuild Previous's checked-in instruction tables using a host C compiler.
set -eu
case "${1:-}" in
    ''|--check) ;;
    *) echo "Usage: $0 [--check]" >&2; exit 2 ;;
esac
task_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
task_cpu="$task_root/src/cpu"
task_tmp=$(mktemp -d "${TMPDIR:-/tmp}/1989-cpu.XXXXXX")
trap 'rm -rf "$task_tmp"' EXIT HUP INT TERM
task_cc=${CC_FOR_BUILD:-cc}
"$task_cc" -O2 -I"$task_cpu" -I"$task_root" \
    "$task_cpu/build68k.c" -o "$task_tmp/build68k"
"$task_tmp/build68k" < "$task_cpu/table68k" > "$task_tmp/cpudefs.c"
"$task_cc" -O2 -I"$task_cpu" -I"$task_root" \
    "$task_cpu/gencpu.c" "$task_cpu/readcpu.c" "$task_tmp/cpudefs.c" \
    -o "$task_tmp/gencpu"
(cd "$task_tmp" && ./gencpu)
for task_file in cpudefs.c cpuemu_31.c cpuemu_32.c cpustbl.c cputbl.h; do
    if [ "${1:-}" = --check ]; then
        cmp "$task_tmp/$task_file" "$task_cpu/$task_file"
    else
        cp "$task_tmp/$task_file" "$task_cpu/$task_file"
    fi
done
echo "CPU sources ${1:-regenerated}: OK"
