#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-slot-a-party"
mkdir -p "$build_dir"

cflags=(-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include")
srcs=("$repo_dir/src/recovered/ecl_helpers.c"
      "$repo_dir/src/recovered/overlay_u68_party_byte_0236.c"
      "$repo_dir/src/recovered/overlay_u1c_party_strength_12c6.c"
      "$repo_dir/src/recovered/overlay_u9b_record_incr_01af.c"
      "$repo_dir/src/recovered/overlay_u6f_party_load.c")

cc "${cflags[@]}" "${srcs[@]}" "$repo_dir/tests/recovered/test_overlay_u6f_party_load.c" \
    -o "$build_dir/test_overlay_u6f_party_load"
"$build_dir/test_overlay_u6f_party_load"

cc "${cflags[@]}" "${srcs[@]}" "$repo_dir/tools/replay/slot_a_party_input.c" \
    -o "$build_dir/slot_a_party_input"

if [[ $# -gt 1 ]]; then
    echo "usage: $0 [IGNORED_RECORD_DIR_WITH_SAVGAMA.DAT_AND_CHRDATA*.SAV]" >&2
    exit 2
fi
if [[ $# -eq 1 ]]; then
    dir=$(realpath -e -- "$1")
    case "$dir" in
        "$repo_dir"/resources/*) ;;
        *) echo "inputs must stay in this clone's ignored resources/" >&2
           exit 2 ;;
    esac
    # Hash-pinned inputs: the slot-A world save and the six character records it names.
    (cd "$dir" && sha256sum -c --quiet - <<'HASHES'
abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1  SAVGAMA.DAT
d7ba1321e1fca8464d1220e77c26ce92173d60cb67b0f155c71868926cfa6c8e  CHRDATA1.SAV
6e82edf77f8c213f76b184f0c0eb2af6a42d2ed5bd7150d330031f78edb44a12  CHRDATA2.SAV
0dd9737e34c75c0a3c41bb1ba3f60a627d00c391e4e261751369961ce9fd726d  CHRDATA3.SAV
a8596f9be05c439a674f4c420053a9d0d3f27e5cb2106c2b17a496c1847e012a  CHRDATA4.SAV
0dd24d3201adc5d50db4aa4c2ab121d55eaf1bcc1a0a491a93743f8871d56062  CHRDATA5.SAV
51a2a026381993f4d2fb8a1a3bb9254eb43450e4061a34640433a619dd2e45b8  CHRDATA6.SAV
HASHES
    ) || { echo "private input hash mismatch" >&2; exit 2; }
    "$build_dir/slot_a_party_input" "$dir/SAVGAMA.DAT" "$dir"
fi
