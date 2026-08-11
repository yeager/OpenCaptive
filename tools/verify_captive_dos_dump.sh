#!/bin/sh
set -eu

dump=${1:?usage: $0 MEMDUMP.BIN DATA_DIR [build_dir]}
data_dir=${2:?usage: $0 MEMDUMP.BIN DATA_DIR [build_dir]}
build_dir=${3:-build}

if [ ! -f "$dump" ]; then
    echo "missing DOSBox-X dump: $dump" >&2
    exit 2
fi
if [ "$(wc -c < "$dump" | tr -d ' ')" -ne 1048576 ]; then
    echo "dump must be exactly 1048576 bytes: $dump" >&2
    exit 2
fi

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-verify.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$build_dir/opencaptive" \
    --data-dir "$data_dir" \
    --captive-dos-dump "$dump" \
    --capture-frame "$tmp_dir/application.ppm"

"$build_dir/opencaptive" \
    --extract-dos-vga "$dump" "$tmp_dir/vga.ppm"

"$build_dir/captive_runtime_render" \
    "$dump" "$tmp_dir/runtime.ppm"

cmp "$tmp_dir/application.ppm" "$tmp_dir/vga.ppm"
cmp "$tmp_dir/application.ppm" "$tmp_dir/runtime.ppm"

echo "CAPPO dump verified: application, DOS VGA and runtime frames are identical"
shasum -a 256 "$tmp_dir/application.ppm"
