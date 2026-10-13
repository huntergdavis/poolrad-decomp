#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
build_dir="$repo_dir/resources/build-first-view-display"
mkdir -p "$build_dir"

cflags=(-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -I"$repo_dir/include")
src="$repo_dir/src/recovered/resident_video_init_4426.c"

cc "${cflags[@]}" "$src" "$repo_dir/tests/recovered/test_resident_video_init_4426.c" -o "$build_dir/test_resident_video_init_4426"
"$build_dir/test_resident_video_init_4426"

cc "${cflags[@]}" "$src" "$repo_dir/tools/replay/first_view_display_state.c" -o "$build_dir/first_view_display_state"

if [[ $# -gt 1 ]]; then
    echo "usage: $0 [IGNORED_DIR_WITH_ram-196f.bin_ram-3382.bin_ram-3387.bin]" >&2
    exit 2
fi
if [[ $# -eq 1 ]]; then
    dir=$(realpath -e -- "$1")
    case "$dir" in
        "$repo_dir"/resources/*) ;;
        *) echo "inputs must stay in this clone's ignored resources/" >&2
           exit 2 ;;
    esac
    # Native saved slot-A first-view RAM images (DOSBox-X debugger, DS=0x14A0), hash-pinned in SLOT_A_NATIVE_FIRST_VIEW_AFTER_196F.md.
    (cd "$dir" && sha256sum -c --quiet - <<'HASHES'
3e46c01d37b393bd88b558584f72f8f93b8ab545a6a607fdddbb738eb0a45844  ram-196f.bin
31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3  ram-3382.bin
d5cc4a1f61561bbb19766616f3addfa100fb63284bc248854ce13079d678f7a0  ram-3387.bin
HASHES
    )
    for n in 196f 3382 3387; do
        "$build_dir/first_view_display_state" "$dir/ram-$n.bin"
    done
fi
