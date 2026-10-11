#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "usage: $0 prepared-run-directory" >&2
    exit 2
fi

run_dir=$(realpath -e "$1")
tools/oracle/launch.sh "$run_dir" -time-limit 45 > "$run_dir/menu-attempt.log" 2>&1 &
emulator_pid=$!
trap 'kill "$emulator_pid" 2>/dev/null || true' EXIT

# The credits appeared by about 25 seconds on the first local smoke run.
sleep 29
window_id=$(xdotool search --onlyvisible --name 'DOSBox-X' | head -1)
# Direct window delivery works under Xvfb without a window manager.
xdotool key --window "$window_id" Return
sleep 9
import -window root "$run_dir/menu-attempt.png"
wait "$emulator_pid"
printf '%s\n' "$run_dir/menu-attempt.png"
