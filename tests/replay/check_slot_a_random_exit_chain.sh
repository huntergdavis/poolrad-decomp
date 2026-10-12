#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-random-exit-chain"
mkdir -p "$build_dir"
if [[ $# -gt 2 ]]; then
    echo "usage: $0 [IGNORED_SAVGAMA.DAT [IGNORED_RUN010_DIR]]" >&2
    exit 2
fi

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/recovered/ecl_helpers.c" \
    "$repo_dir/src/recovered/overlay_u9e_entry8_roll.c" \
    "$repo_dir/src/recovered/overlay_u1c_random_0234.c" \
    "$repo_dir/src/recovered/overlay_u1c_exit_0001.c" \
    "$repo_dir/tests/replay/test_slot_a_random_exit_chain.c" \
    -o "$build_dir/test_slot_a_random_exit_chain"

if [[ $# -ge 1 ]]; then
    save_path=$(realpath -e -- "$1")
    case "$save_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "save must be in this clone's ignored resources/" >&2; exit 2 ;;
    esac
    game_path="$repo_dir/resources/DOS/POOLRAD/GAME.OVR"
    [[ $(sha256sum "$save_path" | awk '{print $1}') == abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 ]]
    [[ $(sha256sum "$game_path" | awk '{print $1}') == 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 ]]
    "$build_dir/test_slot_a_random_exit_chain" "$save_path"
    if [[ $# -eq 2 ]]; then
        python3 "$repo_dir/tests/replay/audit_slot_a_random_exit_chain.py" "$2" "$game_path"
        python3 "$repo_dir/tests/replay/audit_slot_a_exit_snapshots.py" "$2" "$game_path"
    fi
else
    "$build_dir/test_slot_a_random_exit_chain"
fi
