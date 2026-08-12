#!/bin/sh
set -eu

# Local-only parity gate. It requires the player's original Captive media and
# DOSBox-X; no game file, map, marker, text, or frame is generated here.
data_dir=${1:?usage: $0 DATA_DIR [build_dir]}
build_dir=${2:-build}

if [ ! -d "$data_dir" ] || [ ! -f "$data_dir/CAPTIVE.BAT" ]; then
    echo "original Captive data directory with CAPTIVE.BAT is required: $data_dir" >&2
    exit 2
fi
command -v expect >/dev/null 2>&1 || { echo "expect is required" >&2; exit 2; }
command -v dosbox-x >/dev/null 2>&1 || { echo "DOSBox-X is required" >&2; exit 2; }
[ -x "$build_dir/opencaptive" ] || { echo "missing OpenCaptive build: $build_dir/opencaptive" >&2; exit 2; }

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-navigation.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM
land_dir="$tmp_dir/land"
move_dir="$tmp_dir/move"
mkdir -p "$land_dir" "$move_dir"

# CAPPO help/disassembly mapping: keypad 7 = ORBIT, keypad 9 = LAND.
expect tools/captive_dosbox_sequence.expect "$data_dir" 47,49 12 "$land_dir" 8 \
    >"$land_dir/sequence.log" 2>&1
tools/verify_captive_dos_dump.sh "$land_dir/MEMDUMP.BIN" "$data_dir" "$build_dir"

# Keypad 4 is CAPPO's authentic move-left command. The second runtime image
# must differ from the landing checkpoint; equality indicates that the input
# queue or the original runtime handoff stopped accepting real input.
expect tools/captive_dosbox_sequence.expect "$data_dir" 47,49,4B 12 "$move_dir" 8 \
    >"$move_dir/sequence.log" 2>&1
tools/verify_captive_dos_dump.sh "$move_dir/MEMDUMP.BIN" "$data_dir" "$build_dir"

"$build_dir/captive_runtime_render" "$land_dir/MEMDUMP.BIN" "$land_dir/runtime.ppm"
"$build_dir/captive_runtime_render" "$move_dir/MEMDUMP.BIN" "$move_dir/runtime.ppm"
if cmp "$land_dir/runtime.ppm" "$move_dir/runtime.ppm" >/dev/null 2>&1; then
    echo "CAPPO keypad-4 probe did not change the authentic VGA surface" >&2
    exit 1
fi

"$build_dir/captive_map_dump" "$land_dir/MEMDUMP.BIN" 2942 0824 >"$land_dir/map.txt"
"$build_dir/captive_map_dump" "$move_dir/MEMDUMP.BIN" 2942 0824 >"$move_dir/map.txt"
grep -q '^CAPPO raw 5x5 window:' "$land_dir/map.txt"
grep -q '^CAPPO dispatch handlers:' "$land_dir/map.txt"
if cmp "$land_dir/map.txt" "$move_dir/map.txt" >/dev/null 2>&1; then
    echo "CAPPO keypad-4 probe did not change decoded runtime map state" >&2
    exit 1
fi

echo "Authentic CAPPO navigation parity gate passed"
