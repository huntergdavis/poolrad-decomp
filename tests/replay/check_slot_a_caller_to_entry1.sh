#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-caller-to-entry1"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_caller_to_entry1.c" \
    "$repo_dir/tests/replay/test_slot_a_caller_to_entry1.c" \
    -o "$build_dir/test_slot_a_caller_to_entry1"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_slot_a_caller_to_entry1"
elif [[ $# -eq 3 ]]; then
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in
            "$repo_dir"/resources/*) ;;
            *) echo "private inputs must stay in this clone's ignored resources/" >&2
               exit 2 ;;
        esac
    done
    python3 "$repo_dir/tests/replay/audit_slot_a_caller_to_entry1.py" "$1" "$2" "$3"
    "$build_dir/test_slot_a_caller_to_entry1" "$2" "$3"
else
    echo "usage: $0 [IGNORED_GAME.OVR AND_RAM SEARCH_RAM]" >&2
    exit 2
fi
