#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-key-script-join"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_key_dispatch_bridge.c" \
    "$repo_dir/src/replay/slot_a_key_script_join.c" \
    "$repo_dir/tests/replay/test_slot_a_key_script_join.c" \
    -o "$build_dir/test_slot_a_key_script_join"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_slot_a_key_script_join"
elif [[ $# -eq 5 ]]; then
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in
            "$repo_dir"/resources/*) ;;
            *) echo "private inputs must stay in this clone's ignored resources/" >&2
               exit 2 ;;
        esac
    done
    python3 "$repo_dir/tests/replay/audit_slot_a_key_dispatch_bridge.py" "$1" "$3" "$4"
    python3 "$repo_dir/tests/replay/audit_slot_a_key_gate_boundary.py" "$1" "$2" "$4" "$5"
    "$build_dir/test_slot_a_key_script_join" "$3" "$4" "$5"
else
    echo "usage: $0 [IGNORED_GAME.OVR SAVGAMA.DAT RETURN_RAM GATE_RAM SCRIPT_RAM]" >&2
    exit 2
fi
