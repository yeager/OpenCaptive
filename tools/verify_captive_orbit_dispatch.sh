#!/bin/sh
set -eu

# Real-data-only CAPPO probe. This proves that the original keypad-7/ORBIT
# scan is delivered through DOSBox-X's emulated AT keyboard controller and
# CAPPO's original IRQ1 path. It deliberately does not claim that CAPPO has
# consumed the command, reached arrival/orbit, landed, or entered a dungeon.
data_dir=${1:?usage: $0 DATA_DIR}
[ -f "$data_dir/CAPTIVE.BAT" ] || {
    echo "original Captive data directory with CAPTIVE.BAT is required: $data_dir" >&2
    exit 2
}
command -v expect >/dev/null 2>&1 || { echo "expect is required" >&2; exit 2; }
command -v dosbox-x >/dev/null 2>&1 || { echo "DOSBox-X is required" >&2; exit 2; }

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-orbit-dispatch.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM
mkfifo "$tmp_dir/commands"

expect tools/captive_dosbox_sequence.expect \
    "$data_dir" - 240 "$tmp_dir" 8 "$tmp_dir/commands" \
    >"$tmp_dir/session.log" 2>&1 &
harness_pid=$!
exec 3>"$tmp_dir/commands"

for _ in $(seq 1 90); do
    if rg -q 'CAPPO live FIFO ready' "$tmp_dir/session.log" 2>/dev/null; then
        break
    fi
    kill -0 "$harness_pid" 2>/dev/null || {
        tail -80 "$tmp_dir/session.log" >&2
        exit 1
    }
    sleep 1
done
rg -q 'CAPPO live FIFO ready' "$tmp_dir/session.log"
printf '47\n' >&3

for _ in $(seq 1 90); do
    if rg -q 'CAPPO live scan delivered: 47' "$tmp_dir/session.log" 2>/dev/null; then
        break
    fi
    kill -0 "$harness_pid" 2>/dev/null || {
        tail -100 "$tmp_dir/session.log" >&2
        exit 1
    }
    sleep 1
done

rg -q 'CAPPO live scan delivered: 47' "$tmp_dir/session.log" || {
    tail -100 "$tmp_dir/session.log" >&2
    exit 1
}

printf 'quit\n' >&3
exec 3>&-
wait "$harness_pid" || true
echo "Authentic CAPPO ORBIT hardware-input gate passed (runtime consumption and arrival pending)"
