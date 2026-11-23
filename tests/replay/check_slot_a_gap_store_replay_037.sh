#!/usr/bin/env bash
# Run the run-037 0165->13C6 store replay audit, then every mutant of its branches.
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
audit="$repo_dir/tests/replay/audit_slot_a_gap_store_replay_037.py"

echo "== baseline"
nice -n 15 python3 "$audit" "$run37" "$run30"

# Mutants of the audit's branches.  Each must make it fail except the two listed equivalent ones:
#  * first-row-applied re-applies the duplicated first line, whose instruction (xor ax,ax) stores nothing;
#  * ea-ds ignores the SS default of BP-based operands, but the window's only two BP-based operands are
#    register loads (mov si,[bp+06]), so no replayed store depends on it.
killed=0
survived=()
for m in apply-stop-row push-skip call-retaddr mov-skip dec-two ea-ds xr-expect-none xr-wrong-reg tick-hour first-row-applied; do
    if POOLRAD_GAP_MUTANT=$m nice -n 15 python3 "$audit" "$run37" "$run30" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed"
    fi
done
echo "== mutants: killed $killed of 10, survived: ${survived[*]:-none}"
if [[ "${survived[*]:-}" != "ea-ds first-row-applied" ]]; then
    echo "unexpected mutant survival set (expected exactly: ea-ds first-row-applied)" >&2
    exit 1
fi
echo "gap store replay audit and mutants verified"
