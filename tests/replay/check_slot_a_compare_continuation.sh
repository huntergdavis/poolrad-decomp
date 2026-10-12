#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-compare"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/recovered/ecl_helpers.c" \
    "$repo_dir/src/recovered/overlay_u9e_entry8_roll.c" \
    "$repo_dir/src/recovered/overlay_u1c_random_0234.c" \
    "$repo_dir/tests/replay/test_slot_a_compare_continuation.c" \
    -o "$build_dir/test_slot_a_compare_continuation"

if [[ $# -gt 1 ]]; then
    echo "usage: $0 [IGNORED_SLOT_A_SAVGAMA.DAT]" >&2
    exit 2
fi
if [[ $# -eq 1 ]]; then
    save_path=$(realpath -e -- "$1")
    case "$save_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "save must remain in this clone's ignored resources/" >&2; exit 2 ;;
    esac
    actual_save_hash=$(sha256sum "$save_path" | awk '{print $1}')
    actual_game_hash=$(sha256sum "$repo_dir/resources/DOS/POOLRAD/GAME.OVR" | awk '{print $1}')
    if [[ $actual_save_hash != abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 ||
          $actual_game_hash != 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 ]]; then
        echo "private DOS input hash mismatch" >&2
        exit 2
    fi
    "$build_dir/test_slot_a_compare_continuation" "$save_path"
else
    "$build_dir/test_slot_a_compare_continuation"
fi
