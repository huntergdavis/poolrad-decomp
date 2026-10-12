#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-party-entry"
mkdir -p "$build_dir"
if [[ $# -gt 1 ]]; then
    echo "usage: $0 [IGNORED_SAVGAMA.DAT]" >&2
    exit 2
fi

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/replay/slot_a_party_entry.c" \
    "$repo_dir/tests/replay/test_slot_a_party_entry.c" \
    -o "$build_dir/test_slot_a_party_entry"

if [[ $# -eq 1 ]]; then
    save_path=$(realpath -e -- "$1")
    case "$save_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "save must remain in this clone's ignored resources/" >&2; exit 2 ;;
    esac
    game_path="$repo_dir/resources/DOS/POOLRAD/GAME.OVR"
    [[ $(sha256sum "$save_path" | awk '{print $1}') == abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 ]]
    [[ $(sha256sum "$game_path" | awk '{print $1}') == 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 ]]
    python3 - "$game_path" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys
game = Path(sys.argv[1]).read_bytes()
span = game[0x1F904:0x1F94B]
assert len(span) == 71
assert sha256(span).hexdigest() == "e609e0ce57d0eeba1936bb2c5ce6bcbde32d6f55e5dea90484c29f2e8da1a4b7"
print("hash-locked party-entry span: 71 bytes")
PY
    "$build_dir/test_slot_a_party_entry" "$save_path"
else
    "$build_dir/test_slot_a_party_entry"
fi
