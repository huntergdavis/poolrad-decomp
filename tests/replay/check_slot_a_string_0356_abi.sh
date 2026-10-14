#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-string0356-abi"
mkdir -p "$build_dir"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_string_0356_abi.c" \
    "$repo_dir/tests/replay/test_slot_a_string_0356_abi.c" \
    -o "$build_dir/test_slot_a_string_0356_abi"
"$build_dir/test_slot_a_string_0356_abi"
if [[ $# -eq 1 ]]; then
    private_path=$(realpath -e -- "$1")
    case "$private_path" in
        "$repo_dir"/resources/*) ;;
        *) echo "private native inputs must stay under this clone's resources/" >&2; exit 2 ;;
    esac
    cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -fPIC -shared \
        -I"$repo_dir/include" "$repo_dir/src/replay/slot_a_string_0356_abi.c" \
        -o "$build_dir/libstring0356.so"
    "${POOLRAD_UNICORN_PYTHON:-python3}" "$repo_dir/tests/replay/audit_slot_a_string_0356_abi.py" \
        "$build_dir/libstring0356.so" "$private_path"
elif [[ $# -ne 0 ]]; then
    echo "usage: $0 [IGNORED_NATIVE_RUN_ROOT]" >&2; exit 2
fi
