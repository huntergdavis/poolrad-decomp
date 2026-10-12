#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-resident-string-0356"
mkdir -p "$build_dir"
if [[ $# -ne 0 && $# -ne 2 ]]; then
    echo "usage: $0 [IGNORED_SAVGAMA.DAT IGNORED_GAME.OVR]" >&2
    exit 2
fi

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/resident_string_0356.c" \
    "$repo_dir/tests/recovered/test_resident_string_0356.c" \
    -o "$build_dir/test_resident_string_0356"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_resident_string_0356"
    exit 0
fi

save_path=$(realpath -e -- "$1")
game_path=$(realpath -e -- "$2")
for path in "$save_path" "$game_path"; do
    case "$path" in
        "$repo_dir"/resources/*) ;;
        *) echo "original inputs must remain in this clone's ignored resources/" >&2; exit 2 ;;
    esac
done
start_path="$repo_dir/resources/DOS/POOLRAD/START.EXE"
[[ $(sha256sum "$save_path" | awk '{print $1}') == abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 ]]
[[ $(sha256sum "$game_path" | awk '{print $1}') == 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 ]]
[[ $(sha256sum "$start_path" | awk '{print $1}') == d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd ]]

python3 - "$start_path" "$game_path" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys
start = Path(sys.argv[1]).read_bytes()
game = Path(sys.argv[2]).read_bytes()
spans = (
    (start, 0x83AD, 0x847C, "ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889"),
    (start, 0xB79B, 0xB7BF, "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"),
    (start, 0xB7CD, 0xB80E, "18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa"),
    (start, 0xB80E, 0xB83A, "c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64"),
    (start, 0xB886, 0xB8B1, "11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58"),
    (start, 0xB932, 0xB988, "14be20a0c6b74485ec3dcc683266dc07bc2c6e130d9ddcb2f83434b926996a33"),
    (game, 0x1F94B, 0x1F95A, "8caf30b0059e878ceaacb6f7fc0ec602ddd1838f9410c950805ad63d3c4e5126"),
)
for data, begin, end, expected in spans:
    assert sha256(data[begin:end]).hexdigest() == expected
print("hash-locked resident/runtime/caller spans: 7 passed")
PY
"$build_dir/test_resident_string_0356" "$save_path" "$game_path"
