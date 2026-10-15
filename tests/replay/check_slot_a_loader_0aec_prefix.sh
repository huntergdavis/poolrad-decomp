#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-loader0aec-prefix"
mkdir -p "$build_dir"
sources=("$repo_dir/src/replay/slot_a_loader_0aec_prefix.c"
    "$repo_dir/src/replay/slot_a_saved_copy_1965.c"
    "$repo_dir/src/replay/slot_a_string_0425_abi.c"
    "$repo_dir/src/replay/slot_a_string_0356_abi.c"
    "$repo_dir/src/replay/system_cbc_abi.c"
    "$repo_dir/src/replay/system_b98_abi.c")
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -fsyntax-only -I"$repo_dir/include" \
    "$repo_dir/src/replay/slot_a_loader_0aec_prefix.c" "$repo_dir/tests/replay/test_slot_a_loader_0aec_prefix.c"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include" \
    "${sources[@]}" "$repo_dir/tests/replay/test_slot_a_loader_0aec_prefix.c" -o "$build_dir/test_prefix"
"$build_dir/test_prefix"
if [[ $# -eq 1 ]]; then
    private_path=$(realpath -e -- "$1")
    case "$private_path" in "$repo_dir"/resources/*) ;; *) echo "private inputs must stay in this clone resources/" >&2; exit 2 ;; esac
    cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -I"$repo_dir/include" \
        "${sources[@]}" -o "$build_dir/libprefix.so"
    timeout --foreground --kill-after=5s 1200s "${POOLRAD_UNICORN_PYTHON:-python3}" -u \
        "$repo_dir/tests/replay/audit_slot_a_loader_0aec_prefix.py" "$build_dir/libprefix.so" "$private_path"
elif [[ $# -ne 0 ]]; then echo "usage: $0 [NATIVE_LOADER_ROOT]" >&2; exit 2
fi
