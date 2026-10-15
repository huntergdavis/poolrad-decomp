#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-getmem-hook-b0b"
mkdir -p "$build_dir"
sources=("$repo_dir/src/replay/system_getmem_hook_b0b_join.c"
         "$repo_dir/src/replay/system_getmem_real_hook_abi.c"
         "$repo_dir/src/replay/slot_a_saved_copy_1965.c")
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -I"$repo_dir/include" \
    "${sources[@]}" "$repo_dir/tests/replay/test_system_getmem_hook_b0b_join.c" -o "$build_dir/test_join"
"$build_dir/test_join"
if [[ $# -eq 1 ]]; then
    private_path=$(realpath -e -- "$1")
    case "$private_path" in "$repo_dir"/resources/*) ;; *) echo "private inputs must stay in this clone resources/" >&2; exit 2 ;; esac
    cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -fPIC -shared -I"$repo_dir/include" \
        "${sources[@]}" -o "$build_dir/libjoin.so"
    timeout --foreground --kill-after=5s 120s "${POOLRAD_UNICORN_PYTHON:-python3}" -u \
        "$repo_dir/tests/replay/audit_system_getmem_hook_b0b_join.py" "$build_dir/libjoin.so" "$private_path"
elif [[ $# -ne 0 ]]; then echo "usage: $0 [NATIVE_CALLBACK_B0B_ROOT]" >&2; exit 2
fi
