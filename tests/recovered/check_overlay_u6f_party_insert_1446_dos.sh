#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-u6f-insert-parity"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -I"$repo_dir/include" \
    "$repo_dir/src/recovered/overlay_u6f_party_insert_1446_dos.c" \
    "$repo_dir/tests/recovered/test_overlay_u6f_party_insert_1446_dos.c" \
    -o "$build_dir/test_u6f_insert_dos"
"$build_dir/test_u6f_insert_dos"

if [[ $# -eq 0 ]]; then
    exit 0
fi
if [[ $# -ne 4 ]]; then
    echo "usage: $0 [IGNORED_GAME.OVR IGNORED_START.EXE IGNORED_PRE_RAM IGNORED_POST_RAM]" >&2
    exit 2
fi
game_path=$(realpath -e -- "$1")
start_path=$(realpath -e -- "$2")
pre_path=$(realpath -e -- "$3")
post_path=$(realpath -e -- "$4")
for path in "$game_path" "$start_path" "$pre_path" "$post_path"; do
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
"$build_dir/test_u6f_insert_dos" "$pre_path" "$post_path"
echo "hash-locked saved insertion fields and synthetic parity cases passed"
