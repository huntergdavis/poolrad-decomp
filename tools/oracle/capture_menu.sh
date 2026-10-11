#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "usage: $0 prepared-run-directory" >&2
    exit 2
fi

run_dir=$(realpath -e "$1")
tools/oracle/launch.sh "$run_dir" -time-limit 45 \
    -c 'AUTOTYPE -w 28 enter' > "$run_dir/menu-attempt.log" 2>&1 &
emulator_pid=$!
trap 'kill "$emulator_pid" 2>/dev/null || true' EXIT

# DOSBox-X queues the key internally, independent of X11 window focus.
sleep 38
import -window root "$run_dir/menu-attempt.png"
wait "$emulator_pid"
printf '%s\n' "$run_dir/menu-attempt.png"
