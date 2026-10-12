#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-u6f-party-filename"
mkdir -p "$build_dir"
if [[ $# -ne 0 && $# -ne 4 ]]; then
    echo "usage: $0 [IGNORED_SAVE IGNORED_GAME IGNORED_TABLE IGNORED_LIVE_PREFIX]" >&2
    exit 2
fi

cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/resident_string_0356.c" \
    "$repo_dir/src/recovered/resident_string_0425.c" \
    "$repo_dir/src/recovered/overlay_u6f_party_string_chain.c" \
    "$repo_dir/src/recovered/resident_dax_selector_02eb.c" \
    "$repo_dir/src/recovered/overlay_u6f_party_filename.c" \
    "$repo_dir/src/replay/slot_a_party_entry.c" \
    "$repo_dir/src/replay/slot_a_native_save.c" \
    "$repo_dir/src/formats/dos_savgam.c" \
    "$repo_dir/tests/recovered/test_overlay_u6f_party_filename.c" \
    -o "$build_dir/test_u6f_party_filename"

if [[ $# -eq 0 ]]; then
    "$build_dir/test_u6f_party_filename"
    exit 0
fi

save_path=$(realpath -e -- "$1")
game_path=$(realpath -e -- "$2")
table_path=$(realpath -e -- "$3")
prefix_path=$(realpath -e -- "$4")
for path in "$save_path" "$game_path" "$table_path" "$prefix_path"; do
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
[[ $(stat -c '%s' "$prefix_path") == 12 ]]
[[ $(sha256sum "$prefix_path" | awk '{print $1}') == 903c2ad4e1bb2d3871d2880ff2bb4961eafb7a8d95bcb9d609da2464e3a4cfde ]]

python3 - "$start_path" "$game_path" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys
start = Path(sys.argv[1]).read_bytes()
game = Path(sys.argv[2]).read_bytes()
spans = (
    (game, 0x1F969, 0x1F997, "bcbb88786a899c12bc85bf2bee9ec8843a863d284535da2c6b452710086c65a7"),
    (game, 0x1F5CF, 0x1F5D4, "b62e1e6d72460352cdcf1a56fc80b48afadc448d103ec7e9a739d6f2a99b7402"),
    (start, 0xB781, 0xB79B, "4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f"),
    (start, 0xB80E, 0xB83A, "c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64"),
    (start, 0x8342, 0x83AD, "141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474"),
)
for data, begin, end, expected in spans:
    assert sha256(data[begin:end]).hexdigest() == expected
print("hash-locked filename caller/suffix/runtime/selector spans: 5 passed")
PY

bash "$repo_dir/tests/recovered/check_overlay_u6f_party_string_chain.sh" \
    "$save_path" "$game_path" "$table_path"
output_path="$build_dir/private-first-filename.pas"
"$build_dir/test_u6f_party_filename" "$save_path" "$game_path" \
    "$table_path" "$prefix_path" "$output_path"
[[ $(stat -c '%s' "$output_path") == 24 ]]
[[ $(sha256sum "$output_path" | awk '{print $1}') == 74c0568877934bb1fdedebf542d12210bd5fa43a70d557a32ef1630daa6ea843 ]]
echo "private C filename hash checked; native filename parity remains open"
