#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-restore-view-join"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/formats/dax.c" \
    "$repo_dir/src/formats/geo.c" \
    "$repo_dir/src/formats/ega.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/replay/slot_a_static_view.c" \
    "$repo_dir/src/replay/slot_a_restore_view_join.c" \
    "$repo_dir/tests/replay/test_slot_a_restore_view_join.c" \
    -o "$build_dir/test_slot_a_restore_view_join"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_slot_a_restore_view_join"
elif [[ $# -eq 5 ]]; then
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in
            "$repo_dir"/resources/*) ;;
            *) echo "private inputs must stay in this clone's ignored resources/" >&2
               exit 2 ;;
        esac
    done
    python3 "$repo_dir/tests/replay/audit_slot_a_restore_view_join.py" "$@"
    "$build_dir/test_slot_a_restore_view_join" "$2" "$3" "$4"
else
    echo "usage: $0 [IGNORED_GAME.OVR SAVE GEO_DAX VIEW_RAM CPU_TSV]" >&2
    exit 2
fi
