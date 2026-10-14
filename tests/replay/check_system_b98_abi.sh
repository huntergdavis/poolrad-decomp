#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-b98-abi"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include" \
    "$repo_dir/src/replay/system_b98_abi.c" "$repo_dir/tests/replay/test_system_b98_abi.c" -o "$build_dir/test_b98"
"$build_dir/test_b98"
if [[ $# -eq 2 ]]; then
    roots=()
    for input in "$@"; do
        private_path=$(realpath -e -- "$input")
        case "$private_path" in "$repo_dir"/resources/*) ;; *) echo "native roots must stay in this clone resources/" >&2; exit 2 ;; esac
        roots+=("$private_path")
    done
    cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -I"$repo_dir/include" \
        "$repo_dir/src/replay/system_b98_abi.c" -o "$build_dir/libb98.so"
    "${POOLRAD_UNICORN_PYTHON:-python3}" "$repo_dir/tests/replay/audit_system_b98_abi.py" "$build_dir/libb98.so" "${roots[@]}"
elif [[ $# -ne 0 ]]; then
    echo "usage: $0 [NATURAL_B98_ROOT DELETE_MAP_ROOT]" >&2; exit 2
fi
