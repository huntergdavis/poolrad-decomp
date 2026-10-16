#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
flags=(-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror)
mode=strict
if [[ $# -eq 1 && $1 == --sanitize ]]; then
    flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    mode=sanitize
elif [[ $# -ne 0 ]]; then
    echo "usage: $0 [--sanitize]" >&2
    exit 2
fi
build_dir="$repo_dir/resources/build-u6f-02ae-return-join-$mode"
mkdir -p "$build_dir"
cc "${flags[@]}" -I"$repo_dir/include" \
    "$repo_dir/src/replay/u6f_02ae_return_join_fixture.c" \
    "$repo_dir/src/replay/u6f_01b8_entry_abi.c" \
    "$repo_dir/src/replay/u6f_02ae_abi.c" \
    "$repo_dir/src/replay/slot_a_string_0356_abi.c" \
    "$repo_dir/src/replay/system_cbc_abi.c" \
    "$repo_dir/src/replay/system_b98_abi.c" \
    "$repo_dir/src/recovered/slot_a_directory_fullstate.c" \
    "$repo_dir/tests/replay/test_u6f_02ae_return_join_fixture.c" \
    -o "$build_dir/test_u6f_02ae_return_join_fixture"
"$build_dir/test_u6f_02ae_return_join_fixture"
