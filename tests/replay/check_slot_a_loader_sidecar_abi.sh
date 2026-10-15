#!/usr/bin/env bash
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-loader-sidecar"
mkdir -p "$build_dir"
sources=("$repo_dir/src/replay/slot_a_loader_sidecar_abi.c" "$repo_dir/src/replay/slot_a_saved_copy_1965.c"
 "$repo_dir/src/replay/slot_a_file_probe_02eb_abi.c" "$repo_dir/src/replay/slot_a_string_0356_abi.c"
 "$repo_dir/src/replay/system_cbc_abi.c" "$repo_dir/src/replay/system_b98_abi.c")
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Wconversion -Werror -fsyntax-only -I"$repo_dir/include" "$repo_dir/src/replay/slot_a_loader_sidecar_abi.c" "$repo_dir/tests/replay/test_slot_a_loader_sidecar_abi.c"
cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include" "${sources[@]}" "$repo_dir/tests/replay/test_slot_a_loader_sidecar_abi.c" -o "$build_dir/test_abi"
"$build_dir/test_abi"
if [[ $# -eq 1 ]];then
 private_path=$(realpath -e -- "$1")
 case "$private_path" in "$repo_dir"/resources/*) ;; *) echo 'inputs must stay under resources/' >&2;exit 2;;esac
 cc -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -fPIC -shared -I"$repo_dir/include" "${sources[@]}" -o "$build_dir/libabi.so"
 timeout --foreground --kill-after=5s 180s "${POOLRAD_UNICORN_PYTHON:-python3}" -u "$repo_dir/tests/replay/audit_slot_a_loader_sidecar_abi.py" "$build_dir/libabi.so" "$private_path"
elif [[ $# -ne 0 ]];then echo 'usage: runner [NATIVE_LOADER_CONTEXT_ROOT]' >&2;exit 2;fi
