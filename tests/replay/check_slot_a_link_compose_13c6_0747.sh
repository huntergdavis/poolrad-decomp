#!/usr/bin/env bash
# Run the run-030 13C6->0747 C composition audit, then every mutant of its branches.
# Private inputs stay under this tree's ignored resources/; only counts are printed.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 1 ]]; then
    echo "usage: $0 PRIVATE_RUN_t4492-slot4-logc-030_DIR" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run30=$(realpath -e -- "$1")
case "$run30" in
    "$resources"/*) ;;
    *) echo "private inputs must stay in this tree's ignored resources/" >&2; exit 2 ;;
esac
build="$resources/build-link-0747"
audit="$repo_dir/tests/replay/audit_slot_a_link_compose_13c6_0747.py"

echo "== baseline"
nice -n 15 python3 "$audit" "$run30" "$build"

# Mutants of the audit's branches.  Each must make it fail except the listed equivalent one: dos-flags-zf changes
# the ZF/PF bits of the FLAGS word the AH=25 service returns, which the following AND AL,FCh overwrites (the 030
# doc records the same equivalence), so no observable state changes.
killed=0
survived=()
for m in in61-03 dos-regorder dos-psp dos-vseg dos-flags-if dos-flags-zf bind-skip span-entry-sp tail-resume-ip; do
    if POOLRAD_LINK_MUTANT=$m nice -n 15 python3 "$audit" "$run30" "$build" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed"
    fi
done
echo "== mutants: killed $killed of 9, survived: ${survived[*]:-none}"
if [[ "${survived[*]:-}" != "dos-flags-zf" ]]; then
    echo "unexpected mutant survival set (expected exactly: dos-flags-zf)" >&2
    exit 1
fi
echo "link composition audit and mutants verified"
