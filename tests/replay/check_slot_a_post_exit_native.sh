#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 8 ]]; then
    echo "usage: $0 IGNORED_GAME.OVR RAM_345B RAM_348D RAM_3492 RAM_3497 RAM_34CF RAM_34D8 RAM_34E7" >&2
    exit 2
fi
for input in "$@"; do
    private_path=$(realpath -e -- "$input")
    case "$private_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "private inputs must stay in this clone's ignored resources/" >&2
           exit 2 ;;
    esac
done

build_dir="$repo_dir/resources/build-slot-a-post-exit-native"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_caller_to_entry1.c" \
    "$repo_dir/tests/replay/test_slot_a_post_exit_native.c" \
    -o "$build_dir/test_slot_a_post_exit_native"
python3 "$repo_dir/tests/replay/audit_slot_a_post_exit_native.py" "$@"
"$build_dir/test_slot_a_post_exit_native" "$2" "$3"
