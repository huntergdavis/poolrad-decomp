#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-u6f-party-insert"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/overlay_u9b_record_incr_01af.c" \
    "$repo_dir/src/recovered/overlay_u6f_party_insert_1446.c" \
    "$repo_dir/tests/recovered/test_overlay_u6f_party_insert_1446.c" \
    -o "$build_dir/test_u6f_party_insert"
"$build_dir/test_u6f_party_insert"

if [[ $# -eq 0 ]]; then
    echo "synthetic party-insertion tests passed"
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
    ("set-constant", game, 0x1F40C, 0x1F42C, "d22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350"),
    ("insertion-code", game, 0x1F42C, 0x1F565, "370b29767e075c947ae418f44cb57f92e50127c1d519ba029cd7eacb882aa245"),
    ("inline-data", game, 0x1F565, 0x1F5E8, "1dfa5f64639ccd4fec61babcf5419475c3c8317f0d3ba3b69f5ab0731dd192ee"),
    ("caller", game, 0x1F997, 0x1F9D0, "76347194a33838c597b3fba609cb594dcd17a78bf4d74883330276032344a9c5"),
    ("fill", image, 0xC628, 0xC63C, "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"),
    ("membership", image, 0xBD40, 0xBD61, "753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c"),
):
    assert sha256(data[begin:end]).hexdigest() == digest, name
members = [n for n in range(256) if game[0x1F40C + n // 8] & (1 << (n % 8))]
assert members == list(range(8))
print("hash-locked insertion/caller/data/runtime spans: 6 passed; set membership verified")
PY
