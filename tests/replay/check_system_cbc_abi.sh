#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-cbc-abi"
mkdir -p "$build_dir"
sources=("$repo_dir/src/replay/system_cbc_abi.c" "$repo_dir/src/replay/system_b98_abi.c"
         "$repo_dir/src/replay/slot_a_string_0356_abi.c")
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include" \
    "${sources[@]}" "$repo_dir/tests/replay/test_system_cbc_abi.c" -o "$build_dir/test_cbc"
"$build_dir/test_cbc"
if [[ $# -eq 1 ]]; then
    private_path=$(realpath -e -- "$1")
    case "$private_path" in "$repo_dir"/resources/*) ;; *) echo "private captures must stay in this clone resources/" >&2; exit 2 ;; esac
    cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -I"$repo_dir/include" \
        "${sources[@]}" -o "$build_dir/libcbc.so"
    "${POOLRAD_UNICORN_PYTHON:-python3}" "$repo_dir/tests/replay/audit_system_cbc_abi.py" \
        "$build_dir/libcbc.so" "$private_path"
elif [[ $# -ne 0 ]]; then echo "usage: $0 [DELETE_NATIVE_ROOT]" >&2; exit 2
fi
