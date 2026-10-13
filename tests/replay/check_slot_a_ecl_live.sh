#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-ecl-live"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/ecl_helpers.c" \
    "$repo_dir/src/recovered/overlay_u1c_and_or_0def.c" \
    "$repo_dir/src/replay/slot_a_ecl_live.c" \
    "$repo_dir/tests/replay/test_slot_a_ecl_live.c" \
    -o "$build_dir/test_slot_a_ecl_live"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_slot_a_ecl_live"
elif [[ $# -eq 3 ]]; then
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in
            "$repo_dir"/resources/*) ;;
            *) echo "private inputs must stay in this clone's ignored resources/" >&2
               exit 2 ;;
        esac
    done
    python3 "$repo_dir/tests/replay/audit_slot_a_ecl_live.py" "$1" "$2" "$3"
    "$build_dir/test_slot_a_ecl_live" "$3"
else
    echo "usage: $0 [IGNORED_GAME.OVR SAVGAMA.DAT UP_RAM]" >&2
    exit 2
fi
