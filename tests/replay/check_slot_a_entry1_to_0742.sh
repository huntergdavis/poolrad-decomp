#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 5 ]]; then
    echo "usage: $0 IGNORED_GAME.OVR RAM_0D6D RAM_0742 CPU_0D6D CPU_0742" >&2
    exit 2
fi
# Private inputs must resolve under this tree's ignored resources/ (a symlinked
# resources/ is resolved first, so a shared private store is accepted).
resources=$(realpath -e -- "$repo_dir/resources")
for input in "$@"; do
    private_path=$(realpath -e -- "$input")
    case "$private_path" in
        "$resources"/*) ;;
        *) echo "private inputs must stay in this tree's ignored resources/" >&2
           exit 2 ;;
    esac
done

build_dir="$resources/build-slot-a-entry1-to-0742"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_entry1_to_0742.c" \
    "$repo_dir/tests/replay/test_slot_a_entry1_to_0742_native.c" \
    "$repo_dir/src/recovered/overlay_u68_entry1_prefix.c" \
    "$repo_dir/src/recovered/overlay_u3bd_field.c" \
    "$repo_dir/src/recovered/overlay_u3bd_nibble.c" \
    "$repo_dir/src/recovered/overlay_u3bd_cell.c" \
    -o "$build_dir/test_slot_a_entry1_to_0742_native"
python3 -I "$repo_dir/tests/replay/audit_slot_a_entry1_to_0742.py" "$@"
"$build_dir/test_slot_a_entry1_to_0742_native" "$2" "$3" "$4" "$5"
