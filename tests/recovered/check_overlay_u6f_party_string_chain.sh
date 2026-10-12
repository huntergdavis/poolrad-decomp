#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-u6f-party-string-chain"
mkdir -p "$build_dir"
if [[ $# -ne 0 && $# -ne 3 ]]; then
    echo "usage: $0 [IGNORED_SAVGAMA.DAT IGNORED_GAME.OVR IGNORED_LIVE_TABLE]" >&2
    exit 2
fi

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/resident_string_0356.c" \
    "$repo_dir/src/recovered/resident_string_0425.c" \
    "$repo_dir/src/recovered/overlay_u6f_party_string_chain.c" \
    "$repo_dir/src/replay/slot_a_party_entry.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/tests/recovered/test_overlay_u6f_party_string_chain.c" \
    -o "$build_dir/test_u6f_party_string_chain"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_u6f_party_string_chain"
    exit 0
fi

save_path=$(realpath -e -- "$1")
game_path=$(realpath -e -- "$2")
table_path=$(realpath -e -- "$3")
for path in "$save_path" "$game_path" "$table_path"; do
    case "$path" in
        "$repo_dir"/resources/*) ;;
        *) echo "original inputs must remain in this clone's ignored resources/" >&2; exit 2 ;;
    esac
done
start_path="$repo_dir/resources/DOS/POOLRAD/START.EXE"
[[ $(sha256sum "$save_path" | awk '{print $1}') == abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 ]]
[[ $(sha256sum "$game_path" | awk '{print $1}') == 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 ]]
[[ $(sha256sum "$start_path" | awk '{print $1}') == d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd ]]
[[ $(stat -c '%s' "$table_path") == 10 ]]
[[ $(sha256sum "$table_path" | awk '{print $1}') == cf4e5025c04cb19b9596b886668127dababab7e9b18c05532b426d82bcb89959 ]]

python3 - "$start_path" "$game_path" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys
start = Path(sys.argv[1]).read_bytes()
game = Path(sys.argv[2]).read_bytes()
spans = (
    (game, 0x1F921, 0x1F969, "9dd1291f5159418e576116974a60010371dddd79a4be1a5f68f2ec074eac0700"),
    (game, 0x1DFE6 + 0x15E7, 0x1DFE6 + 0x15E9, "5598b3f30c8e047bfd81991dedffa70603c790fcf6f4414d144e6b0d8caeab5f"),
    (start, 0xB781, 0xB79B, "4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f"),
    (start, 0xB79B, 0xB7BF, "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"),
    (start, 0x83AD, 0x847C, "ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889"),
    (start, 0x847C, 0x8566, "28c69009d8bd4218b1eb50754c3f32a6f36ef40d3927d630fc8481d10b99e5a3"),
    (start, 0xB7CD, 0xB80E, "18383182a06f0521f0873061e61004f0b1c6bec8f3b30b1158ec7ad7822940fa"),
    (start, 0xB8B1, 0xB8C3, "b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484"),
    (start, 0xC4A0, 0xC4B3, "738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c"),
)
for data, begin, end, expected in spans:
    assert sha256(data[begin:end]).hexdigest() == expected
print("hash-locked caller/literal/runtime/resident spans: 9 passed")
PY
output_path="$build_dir/private-after-0425.pas"
"$build_dir/test_u6f_party_string_chain" "$save_path" "$game_path" \
    "$table_path" "$output_path"
[[ $(stat -c '%s' "$output_path") == 9 ]]
[[ $(sha256sum "$output_path" | awk '{print $1}') == 6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd ]]
echo "C +1974 live Pascal hash matches pinned native watch (one input case)"
