#!/usr/bin/env bash
# Run the run-037 0165->13C6 C composition audit, then every mutant of its branches.
# Private inputs stay under this tree's ignored resources/; only counts are printed.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 2 ]]; then
    echo "usage: $0 PRIVATE_RUN_t4492-slot8-gap-037_DIR PRIVATE_RUN_t4492-slot4-logc-030_DIR" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run37=$(realpath -e -- "$1")
run30=$(realpath -e -- "$2")
for d in "$run37" "$run30"; do
    case "$d" in
        "$resources"/*) ;;
        *) echo "private inputs must stay in this tree's ignored resources/" >&2; exit 2 ;;
    esac
done
build="$resources/build-gap-compose-037"
audit="$repo_dir/tests/replay/audit_slot_a_gap_compose_037.py"

echo "== baseline"
nice -n 15 python3 "$audit" "$run37" "$run30" "$build"

# Mutants of the audit's branches.  Every one must make it fail (no equivalent survivor in this window).
killed=0
survived=()
for m in entry-ds glue-ip drop-dispatch-0 drop-17AE drop-dispatch-1 drop-1742 drop-dispatch-2 drop-13B1 frame-keep \
         span-entry-sp frame-overlap frame-ref-shift; do
    if POOLRAD_GCOMP_MUTANT=$m nice -n 15 python3 "$audit" "$run37" "$run30" "$build" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed"
    fi
done
echo "== mutants: killed $killed of 12, survived: ${survived[*]:-none}"
if [[ ${#survived[@]} -ne 0 ]]; then
    echo "unexpected mutant survival: ${survived[*]}" >&2
    exit 1
fi
echo "gap composition audit and mutants verified"
