#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-native"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/tests/replay/test_slot_a_native_save.c" \
    -o "$build_dir/test_slot_a_native_save"
"$build_dir/test_slot_a_native_save"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/tools/replay/slot_a_native_smoke.c" \
    -o "$build_dir/slot_a_native_smoke"

if [[ $# -gt 1 ]]; then
    echo "usage: $0 [IGNORED_SLOT_A_SAVGAMA.DAT]" >&2
    exit 2
fi
if [[ $# -eq 1 ]]; then
    save_path=$(realpath -e -- "$1")
    case "$save_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "save must remain in this clone's ignored resources/" >&2
           exit 2 ;;
    esac
    actual_hash=$(sha256sum "$save_path" | awk '{print $1}')
    expected_hash=abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1
    if [[ $actual_hash != "$expected_hash" ]]; then
        echo "private slot-A save hash mismatch" >&2
        exit 2
    fi
    "$build_dir/slot_a_native_smoke" "$save_path"
fi
