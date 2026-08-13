#!/bin/sh
set -eu

# Local-only CAPPO navigation/render gate. It requires the player's original
# Captive media and DOSBox-X; no game file, map, marker, text, or frame is
# generated here.
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
orbit_dir="$tmp_dir/orbit"
move_dir="$tmp_dir/move"
mkdir -p "$orbit_dir" "$move_dir"

# CAPPO help/disassembly mapping: keypad 7 records the flight-path action in
# this Mission 0001 state. This gate verifies the authentic VGA handoff and
# input response only; it does not promote a changed coordinate to Orbit.
expect tools/captive_dosbox_sequence.expect "$data_dir" 47 240 "$orbit_dir" 120 \
    >"$orbit_dir/sequence.log" 2>&1
tools/verify_captive_dos_dump.sh "$orbit_dir/MEMDUMP.BIN" "$data_dir" "$build_dir"

# Keypad 8 is CAPPO's authentic Forward command. The second runtime image
# must differ from the orbit checkpoint; equality indicates that the input
# queue or the original runtime handoff stopped accepting real input.
expect tools/captive_dosbox_sequence.expect "$data_dir" 47,48 240 "$move_dir" 120 \
    >"$move_dir/sequence.log" 2>&1
tools/verify_captive_dos_dump.sh "$move_dir/MEMDUMP.BIN" "$data_dir" "$build_dir"

"$build_dir/captive_runtime_render" "$orbit_dir/MEMDUMP.BIN" "$orbit_dir/runtime.ppm"
"$build_dir/captive_runtime_render" "$move_dir/MEMDUMP.BIN" "$move_dir/runtime.ppm"
if cmp "$orbit_dir/runtime.ppm" "$move_dir/runtime.ppm" >/dev/null 2>&1; then
    echo "CAPPO keypad-8 probe did not change the authentic VGA surface" >&2
    exit 1
fi

# Keypad-8 is an authentic CAPPO navigation byte. CAPPO changes its live
# Mission 0001 VGA surface, not the dungeon map at DS:7CB3; requiring that map
# table to change would reject a correct original navigation transition.
"$build_dir/captive_map_dump" "$orbit_dir/MEMDUMP.BIN" 1663 0824 >"$orbit_dir/map.txt"
grep -q '^CAPPO raw 5x5 window:' "$orbit_dir/map.txt"
grep -q '^CAPPO dispatch handlers:' "$orbit_dir/map.txt"

echo "Authentic CAPPO input/render parity gate passed (arrival/landing still pending)"
