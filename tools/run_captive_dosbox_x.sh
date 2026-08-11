#!/bin/sh
set -eu

data_dir=${1:?usage: $0 DATA_DIR [COMMAND]}
command=${2:-CAPTIVE.BAT 1}
profile_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
profile="$profile_dir/dosbox-x-captive.conf"
dosbox_x_bin=${DOSBOX_X_BIN:-}

if [ -z "$dosbox_x_bin" ]; then
    if [ -x /opt/homebrew/bin/dosbox-x ]; then
        dosbox_x_bin=/opt/homebrew/bin/dosbox-x
    else
        dosbox_x_bin=$(command -v dosbox-x || true)
    fi
fi

if [ ! -d "$data_dir" ]; then
    echo "Captive data directory does not exist: $data_dir" >&2
    exit 2
fi
if [ ! -f "$profile" ]; then
    echo "DOSBox-X profile is missing: $profile" >&2
    exit 2
fi
if [ -z "$dosbox_x_bin" ] || [ ! -x "$dosbox_x_bin" ]; then
    echo "DOSBox-X executable not found; set DOSBOX_X_BIN explicitly" >&2
    exit 2
fi

case "$command" in
    CAPPO.EXE*|cappo.exe*|*\\CAPPO.EXE*|*\\cappo.exe*)
        echo "Refusing direct CAPPO.EXE launch: run the authentic CAPTIVE.BAT 1 chain" >&2
        exit 2
        ;;
esac

echo "Using DOSBox-X: $($dosbox_x_bin -version 2>&1 | sed -n '2p')" >&2
echo "Using isolated profile: $profile" >&2

exec "$dosbox_x_bin" -conf "$profile" \
    -c "mount c $data_dir" \
    -c "c:" \
    -c "$command"
