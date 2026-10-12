#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-post-transfer"
mkdir -p "$build_dir"
if [[ $# -gt 2 ]]; then
    echo "usage: $0 [IGNORED_SAVGAMA.DAT [IGNORED_PRECALL_RAM]]" >&2
    exit 2
fi

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/replay/slot_a_post_transfer.c" \
    "$repo_dir/tests/replay/test_slot_a_post_transfer.c" \
    -o "$build_dir/test_slot_a_post_transfer"

if [[ $# -ge 1 ]]; then
    save_path=$(realpath -e -- "$1")
    case "$save_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "save must remain in this clone's ignored resources/" >&2; exit 2 ;;
    esac
    game_path="$repo_dir/resources/DOS/POOLRAD/GAME.OVR"
    [[ $(sha256sum "$save_path" | awk '{print $1}') == abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 ]]
    [[ $(sha256sum "$game_path" | awk '{print $1}') == 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 ]]
    "$build_dir/test_slot_a_post_transfer" "$save_path"
    if [[ $# -eq 2 ]]; then
        python3 "$repo_dir/tests/replay/audit_slot_a_post_transfer.py" "$save_path" "$game_path" "$2"
    fi
else
    "$build_dir/test_slot_a_post_transfer"
fi
