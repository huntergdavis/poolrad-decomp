#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-saved-copy-1965"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_saved_copy_1965.c" \
    "$repo_dir/tests/replay/test_slot_a_saved_copy_1965.c" \
    -o "$build_dir/test_slot_a_saved_copy_1965"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_slot_a_saved_copy_1965"
elif [[ $# -eq 2 ]]; then
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in
            "$repo_dir"/resources/*) ;;
            *) echo "private inputs must stay in this clone's ignored resources/" >&2
               exit 2 ;;
        esac
    done
    python3 "$repo_dir/tests/recovered/check_slot_a_u6f_1965_native.py" "$1" "$2"
    "$build_dir/test_slot_a_saved_copy_1965" "$1" "$2"
else
    echo "usage: $0 [IGNORED_FIRST_NATIVE_RUN SECOND_NATIVE_RUN]" >&2
    exit 2
fi
