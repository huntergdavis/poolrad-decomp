#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-first-exit"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/ecl_helpers.c" \
    "$repo_dir/src/recovered/overlay_u1c_and_or_0def.c" \
    "$repo_dir/src/recovered/overlay_u1c_exit_0001.c" \
    "$repo_dir/src/replay/slot_a_ecl_live.c" \
    "$repo_dir/src/replay/slot_a_first_exit.c" \
    "$repo_dir/tests/replay/test_slot_a_first_exit.c" \
    -o "$build_dir/test_slot_a_first_exit"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_slot_a_first_exit"
elif [[ $# -eq 5 ]]; then
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in
            "$repo_dir"/resources/*) ;;
            *) echo "private inputs must stay in this clone's ignored resources/" >&2
               exit 2 ;;
        esac
    done
    python3 "$repo_dir/tests/replay/audit_slot_a_first_exit.py" "$1" "$2" "$3" "$4" "$5"
    "$build_dir/test_slot_a_first_exit" "$3" "$4" "$5"
else
    echo "usage: $0 [IGNORED_GAME.OVR SAVGAMA.DAT PRE_RAM AND_RAM SEARCH_RAM]" >&2
    exit 2
fi
