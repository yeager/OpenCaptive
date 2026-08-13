#!/bin/sh
set -eu

# Real-data-only CAPPO route gate. It drives the supplied original
# CAPTIVE.BAT chain and checks the resulting original VGA frame. It stops at
# FLIGHT PATH SET; Orbit arrival and LAND remain separate until CAPPO proves
# them.
data_dir=${1:?usage: $0 DATA_DIR [build_dir]}
build_dir=${2:-build}

[ -f "$data_dir/CAPTIVE.BAT" ] || {
    echo "original Captive data directory with CAPTIVE.BAT is required: $data_dir" >&2
    exit 2
}
[ -x "$build_dir/captive_runtime_render" ] || {
    echo "missing captive_runtime_render: $build_dir" >&2
    exit 2
}
command -v expect >/dev/null 2>&1 || { echo "expect is required" >&2; exit 2; }
command -v dosbox-x >/dev/null 2>&1 || { echo "DOSBox-X is required" >&2; exit 2; }

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-target.XXXXXX")
mkdir "$tmp_dir/out"
mkfifo "$tmp_dir/commands"

# CAPPO manual/disassembly mapping: 4D=Move Right, 48=Move Back,
# 47=Turn Left/fly to cursor. Ten right and ten back scans place the real
# Mission 0001 cursor on the real green target in the supplied CAPPO state.
expect tools/captive_dosbox_sequence.expect "$data_dir" \
    "4D,4D,4D,4D,4D,4D,4D,4D,4D,4D,48,48,48,48,48,48,48,48,48,48,47" \
    240 "$tmp_dir/out" 8 "$tmp_dir/commands" >"$tmp_dir/session.log" 2>&1 &
harness_pid=$!
exec 3>"$tmp_dir/commands"

while ! rg -q 'CAPPO live FIFO ready' "$tmp_dir/session.log" 2>/dev/null; do
    kill -0 "$harness_pid" 2>/dev/null || {
        echo "CAPPO harness exited before live route was ready" >&2
        exit 1
    }
    sleep 1
done

# Ask the original emulator for one short timer observation and use that
# complete dump; no local map, marker, status text, or destination is made.
printf 'WAIT:8\n' >&3
sleep 1
"$build_dir/captive_runtime_render" \
    "$tmp_dir/out/MEMDUMP.BIN" "$tmp_dir/route.ppm"

python3 - "$tmp_dir/route.ppm" <<'PY'
import sys
from pathlib import Path

path = Path(sys.argv[1])
data = path.read_bytes()
header, pixels = data.split(b"\n255\n", 1)
if header != b"P6\n320 200":
    raise SystemExit("unexpected native frame format")
if len(pixels) != 320 * 200 * 3:
    raise SystemExit("incomplete native frame")

def pixel(x, y):
    i = (y * 320 + x) * 3
    return pixels[i:i + 3]

# This is the real CAPPO VGA marker measured from the supplied original data.
expected = (48, 195, 48)
points = [(63, 150), (64, 150), (63, 151), (64, 151)]
if any(pixel(x, y) != bytes(expected) for x, y in points):
    raise SystemExit("authentic CAPPO route did not show the verified green target")
PY

printf 'quit\n' >&3
exec 3>&-
wait "$harness_pid"
echo "Authentic CAPPO target-route gate passed (FLIGHT PATH SET; Orbit/LAND still pending)"
