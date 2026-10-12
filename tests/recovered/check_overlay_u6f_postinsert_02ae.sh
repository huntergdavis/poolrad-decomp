#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-u6f-postinsert"
mkdir -p "$build_dir"

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/overlay_u6f_postinsert_02ae.c" \
    "$repo_dir/tests/recovered/test_overlay_u6f_postinsert_02ae.c" \
    -o "$build_dir/test_u6f_postinsert"
"$build_dir/test_u6f_postinsert"

if [[ $# -eq 0 ]]; then
    echo "synthetic post-insertion tests passed"
    exit 0
fi
if [[ $# -ne 2 ]]; then
    echo "usage: $0 [IGNORED_GAME.OVR IGNORED_START.EXE]" >&2
    exit 2
fi
game_path=$(realpath -e -- "$1")
start_path=$(realpath -e -- "$2")
for path in "$game_path" "$start_path"; do
    case "$path" in
        "$repo_dir"/resources/*) ;;
        *) echo "DOS inputs must remain in this clone's ignored resources/" >&2; exit 2 ;;
    esac
done

PYTHONPATH="$repo_dir/tools/overlay${PYTHONPATH:+:$PYTHONPATH}" \
PYTHONDONTWRITEBYTECODE=1 python3 - "$game_path" "$start_path" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys
from resident_loader_map import unpack

game = Path(sys.argv[1]).read_bytes()
start = Path(sys.argv[2]).read_bytes()
assert len(game) == 206105
assert sha256(game).hexdigest() == "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
assert len(start) == 65231
assert sha256(start).hexdigest() == "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
image = unpack(start)[0]
for name, data, begin, end, digest in (
    ("post-insertion", game, 0x1E294, 0x1E315, "ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539"),
    ("private-prefix", game, 0x1E280, 0x1E292, "cb2247d8e036127fdc14edbe785093db176bb7d8161838f29269dd151755cf80"),
    ("private-suffix", game, 0x1E292, 0x1E294, "63ded01247acc7888c74eae4707f8bd28a532c761ebcbe16d3e0bb7996945a28"),
    ("system-mkdir", image, 0xC756, 0xC769, "90731793b0ac2c364059b1e22581baf55595d3328f57b9257846664eb84b4f01"),
    ("system-asciiz", image, 0xC780, 0xC79B, "94899fa2b266515992d6ea94fef07edcd168dc03e772b3aea6f34c9bb655d5d9"),
    ("system-dos-call", image, 0xC79B, 0xC7AA, "978fabd0bca91df715152943c57b87c26e835dbd9533fbb60843e32bfc86908b"),
    ("resident-display", image, 0x7C85, 0x7CCF, "4a3f940caae95794f257a25f5241bcf3a04c2749b212f5b8d9def5befd63a4cc"),
):
    assert sha256(data[begin:end]).hexdigest() == digest, name
print("hash-locked post-insertion, literals, SYSTEM, resident spans: 7 passed")
PY
