#!/bin/sh
set -eu

# Local-only replay gate. It drives the original CAPPO Mission 0001 runtime through
# DOSBox-X's emulated AT keyboard controller and IRQ1 handler and verifies every
# resulting VGA dump. No game data,
# dungeon, position, or frame is created by this script.
data_dir=${1:?usage: $0 DATA_DIR [build_dir]}
build_dir=${2:-build}

if [ ! -d "$data_dir" ] || [ ! -f "$data_dir/CAPTIVE.BAT" ]; then
    echo "original Captive data directory with CAPTIVE.BAT is required: $data_dir" >&2
    exit 2
fi
command -v expect >/dev/null 2>&1 || { echo "expect is required" >&2; exit 2; }
command -v dosbox-x >/dev/null 2>&1 || { echo "DOSBox-X is required" >&2; exit 2; }
[ -x "$build_dir/captive_runtime_render" ] || {
    echo "missing captive_runtime_render: $build_dir" >&2
    exit 2
}

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/opencaptive-captive-replay.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM
fifo="$tmp_dir/commands"
mkfifo "$fifo"

# Each raw command now includes the authentic make+break pair. Keep the
# observation pacing short but deterministic; the original CAPPO timer still
# owns every state transition and the dump is copied only after its response.
expect tools/captive_dosbox_sequence.expect "$data_dir" - 240 "$tmp_dir" 8 "$fifo" \
    >"$tmp_dir/session.log" 2>&1 &
harness_pid=$!
exec 3>"$fifo"

while [ ! -s "$tmp_dir/MEMDUMP.BIN" ]; do
    kill -0 "$harness_pid" 2>/dev/null || {
        echo "CAPPO harness exited before its first dump" >&2
        exit 1
    }
    sleep 1
done

dump_no=0
capture_step() {
    name=$1
    cp "$tmp_dir/MEMDUMP.BIN" "$tmp_dir/$name.BIN"
    tools/verify_captive_dos_dump.sh "$tmp_dir/$name.BIN" "$data_dir" "$build_dir" \
        >"$tmp_dir/$name.verify.log" 2>&1
    "$build_dir/captive_runtime_render" "$tmp_dir/$name.BIN" "$tmp_dir/$name.ppm"
    shasum -a 256 "$tmp_dir/$name.ppm" | awk '{print $1}'
}

previous_hash=$(capture_step "00-start")
changed_frames=0
for scan in 47 49 48 4B 4D 48; do
    before_count=$(rg -o "CAPPO live scan delivered: $scan" "$tmp_dir/session.log" 2>/dev/null | wc -l | tr -d ' ' || true)
    printf '%s\n' "$scan" >&3
    dump_no=$((dump_no + 1))
    delivered=$before_count
    while [ "$delivered" -le "$before_count" ]; do
        delivered=$(rg -o "CAPPO live scan delivered: $scan" "$tmp_dir/session.log" 2>/dev/null | wc -l | tr -d ' ' || true)
        kill -0 "$harness_pid" 2>/dev/null || {
            echo "CAPPO harness exited while delivering scan $scan" >&2
            exit 1
        }
        sleep 1
    done
    current_hash=$(capture_step "$(printf '%02d-%s' "$dump_no" "$scan")")
    if [ "$current_hash" != "$previous_hash" ]; then
        changed_frames=$((changed_frames + 1))
    fi
    previous_hash=$current_hash
done

printf 'quit\n' >&3
exec 3>&-
wait "$harness_pid"

[ "$changed_frames" -ge 3 ] || {
    echo "CAPPO replay produced too few authentic frame changes: $changed_frames" >&2
    exit 1
}
echo "Authentic CAPPO live replay gate passed ($changed_frames frame changes)"
