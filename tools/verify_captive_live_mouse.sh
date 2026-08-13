#!/bin/sh
set -eu

# Real-data-only CAPPO mouse transport probe. The commands below become
# DOSBox-X integration-device events; they never write CAPPO memory with SM.
# This is deliberately not an interactive mouse-parity gate: DOSBox-X ignores
# Mouse_CursorMoved() while its debugger is paused for MEMDUMPBIN.
data_dir=${1:?usage: $0 DATA_DIR [build_dir]}
build_dir=${2:-build}
[ -f "$data_dir/CAPTIVE.BAT" ] || { echo "original Captive data required" >&2; exit 2; }
command -v expect >/dev/null 2>&1 || { echo "expect is required" >&2; exit 2; }
command -v dosbox-x >/dev/null 2>&1 || { echo "DOSBox-X is required" >&2; exit 2; }
[ -x "$build_dir/captive_runtime_render" ] || { echo "missing runtime renderer" >&2; exit 2; }

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-mouse.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM
mkdir "$tmp_dir/out"
mkfifo "$tmp_dir/commands"
expect tools/captive_dosbox_sequence.expect "$data_dir" - 240 "$tmp_dir/out" 8 \
    "$tmp_dir/commands" >"$tmp_dir/session.log" 2>&1 &
harness_pid=$!
exec 3>"$tmp_dir/commands"
while [ ! -s "$tmp_dir/out/MEMDUMP.BIN" ]; do
    kill -0 "$harness_pid" 2>/dev/null || { cat "$tmp_dir/session.log" >&2; exit 1; }
    sleep 1
done

cp "$tmp_dir/out/MEMDUMP.BIN" "$tmp_dir/start.bin"
send_and_capture() {
    command=$1
    marker=$2
    printf '%s\n' "$command" >&3
    while ! rg -q "$marker" "$tmp_dir/session.log"; do
        kill -0 "$harness_pid" 2>/dev/null || { cat "$tmp_dir/session.log" >&2; exit 1; }
        sleep 1
    done
    cp "$tmp_dir/out/MEMDUMP.BIN" "$tmp_dir/$command.bin"
}
send_and_capture MOUSE_DX:40 "CAPPO live mouse dx delivered: 40"
send_and_capture MOUSE_DY:20 "CAPPO live mouse dy delivered: 20"
send_and_capture MOUSE_DOWN "CAPPO live mouse button delivered: MOUSE_DOWN"
send_and_capture MOUSE_UP "CAPPO live mouse button delivered: MOUSE_UP"
printf 'quit\n' >&3
exec 3>&-
wait "$harness_pid" || true

for name in start MOUSE_DX:40 MOUSE_DY:20 MOUSE_DOWN MOUSE_UP; do
    [ "$(wc -c < "$tmp_dir/$name.bin" | tr -d ' ')" -eq 1048576 ] || exit 1
done

changed=0
for name in MOUSE_DX:40 MOUSE_DY:20 MOUSE_DOWN MOUSE_UP; do
    if ! cmp "$tmp_dir/start.bin" "$tmp_dir/$name.bin" >/dev/null 2>&1; then
        changed=$((changed + 1))
    fi
done
[ "$changed" -ge 2 ] || { echo "CAPPO mouse produced too few frame changes: $changed" >&2; exit 1; }
echo "Authentic CAPPO mouse transport probe passed ($changed changed original dumps); interactive mouse parity remains unverified"
