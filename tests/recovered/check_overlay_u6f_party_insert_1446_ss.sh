#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-u6f-ss-hash-gate"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/overlay_u6f_party_insert_1446_ss.c" \
    "$repo_dir/src/recovered/overlay_u6f_insert_ss_probe.c" \
    "$repo_dir/src/recovered/overlay_u6f_party_insert_1446_dos.c" \
    "$repo_dir/tests/recovered/test_overlay_u6f_party_insert_1446_ss.c" \
    -o "$build_dir/test_u6f_ss"
"$build_dir/test_u6f_ss"

if [[ $# -eq 0 ]]; then
    exit 0
fi
if [[ $# -ne 4 && $# -ne 6 ]]; then
    echo "usage: $0 [IGNORED_GAME.OVR IGNORED_START.EXE IGNORED_FIRST_PRE_RAM IGNORED_FIRST_POST_RAM [IGNORED_SECOND_PRE_RAM IGNORED_SECOND_POST_RAM]]" >&2
    exit 2
fi
game_path=$(realpath -e -- "$1")
start_path=$(realpath -e -- "$2")
pre_path=$(realpath -e -- "$3")
post_path=$(realpath -e -- "$4")
private_paths=("$game_path" "$start_path" "$pre_path" "$post_path")
if [[ $# -eq 6 ]]; then
    second_pre_path=$(realpath -e -- "$5")
    second_post_path=$(realpath -e -- "$6")
    private_paths+=("$second_pre_path" "$second_post_path")
fi
for path in "${private_paths[@]}"; do
    case "$path" in
        */resources/*) ;;
        *) echo "private inputs must remain under ignored resources/" >&2; exit 2 ;;
    esac
done

bash "$repo_dir/tests/recovered/check_overlay_u6f_party_insert_1446.sh" \
    "$game_path" "$start_path"
pre_sum=$(sha256sum "$pre_path")
post_sum=$(sha256sum "$post_path")
[[ ${pre_sum%% *} == 6ceed694294d93c2cdbd2684626f2245b04af0665b0b5bfeeda284bda5d8948a ]]
[[ ${post_sum%% *} == 5d18bcca937401d24b77480351f0a3107305a1e97662aad55145385ea85095a0 ]]
[[ $(stat -c%s -- "$pre_path") -eq 655360 ]]
[[ $(stat -c%s -- "$post_path") -eq 655360 ]]
PYTHONPATH="$repo_dir/tools/overlay${PYTHONPATH:+:$PYTHONPATH}" \
PYTHONDONTWRITEBYTECODE=1 python3 - "$game_path" "$pre_path" <<'PY'
from hashlib import sha256
from pathlib import Path
import sys

game = Path(sys.argv[1]).read_bytes()
pre = Path(sys.argv[2]).read_bytes()
original = game[0x1f40c:0x1f42c]
loaded = pre[(0x2a8e << 4) + 0x1426:(0x2a8e << 4) + 0x1446]
assert sha256(original).hexdigest() == 'd22f8e3450655784853879c7ecfee4ff36192352f6ffe0d40593dee56f583350'
assert loaded == original
print('loaded 32-byte membership set: hash-pinned match')
PY
if [[ $# -eq 6 ]]; then
    second_pre_sum=$(sha256sum "$second_pre_path")
    second_post_sum=$(sha256sum "$second_post_path")
    [[ ${second_pre_sum%% *} == 243585b1f15620ddc8962ccdbffbbdade2cc85539ba2d903952b6f794f99c19a ]]
    [[ ${second_post_sum%% *} == 6a34b4eed8d7be5c44a369851a5aa7fb43322100726e842645a71c8ef532a10c ]]
    [[ $(stat -c%s -- "$second_pre_path") -eq 655360 ]]
    [[ $(stat -c%s -- "$second_post_path") -eq 655360 ]]
    "$build_dir/test_u6f_ss" "$pre_path" "$post_path" \
        "$second_pre_path" "$second_post_path"
else
    "$build_dir/test_u6f_ss" "$pre_path" "$post_path"
fi
echo "hash-locked SS-frame and saved insertion fields passed"
