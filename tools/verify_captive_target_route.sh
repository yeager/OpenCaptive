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

wait_for_complete_dump() {
    dump=$1
    for _ in $(seq 1 60); do
        if [ -f "$dump" ] && [ "$(wc -c < "$dump" | tr -d ' ')" -eq 1048576 ]; then
            return 0
        fi
        sleep 0.5
    done
    echo "DOSBox-X did not produce a complete 1 MiB dump: $dump" >&2
    return 1
}

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-target.XXXXXX")
mkdir "$tmp_dir/out"
mkfifo "$tmp_dir/commands"

# CAPPO manual/disassembly mapping: 47=Turn Left/fly to cursor. In the
# supplied authentic Mission 0001 state the green destination marker is
# already under CAPPO's magenta cursor at startup. Moving the holomap before
# ORBIT moves away from the real destination and is not part of this proof.
expect tools/captive_dosbox_sequence.expect "$data_dir" \
    "47" \
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
wait_for_complete_dump "$tmp_dir/out/MEMDUMP.BIN"
"$build_dir/captive_runtime_render" \
    "$tmp_dir/out/MEMDUMP.BIN" "$tmp_dir/route.ppm"

python3 tools/check_captive_target_frame.py "$tmp_dir/route.ppm"

printf 'quit\n' >&3
exec 3>&-
wait "$harness_pid"
echo "Authentic CAPPO target-route gate passed (FLIGHT PATH SET; Orbit/LAND still pending)"
