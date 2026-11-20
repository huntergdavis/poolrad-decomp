#!/usr/bin/env bash
# Run the run-034 window store replay audit, then every mutant of its new branches.
# Private inputs stay under this tree's ignored resources/; nothing is printed from them but counts.
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "$0")/../.." && pwd -P)
if [[ $# -ne 2 ]]; then
    echo "usage: $0 PRIVATE_RUN_t4492-slot6-window-034_DIR PRIVATE_oracle-runs_DIR" >&2
    exit 2
fi
resources=$(realpath -e -- "$repo_dir/resources")
run=$(realpath -e -- "$1")
runs=$(realpath -e -- "$2")
for d in "$run" "$runs"; do
    case "$d" in
        "$resources"/*) ;;
        *) echo "private inputs must stay in this tree's ignored resources/" >&2; exit 2 ;;
    esac
done
game="$resources/DOS/POOLRAD/GAME.OVR"
audit="$repo_dir/tests/replay/audit_slot_a_window_store_replay_034.py"

echo "== baseline (with the Run B intermediate-stop control)"
nice -n 15 python3 "$audit" "$run" "$game" "$runs"

# Mutants of the new branches.  Each must make the audit fail, except the listed equivalent one:
# no-hw-frame drops the hardware IRQ0 frame stores, whose bytes are all overwritten before 3492.
killed=0
survived=()
for m in video-wrap apply-stop-row regsave-order psp-ptr seek-high read-dest-seg read-pos read-short \
         read-skip tick-two tick-skip no-hw-frame rep-chunk4 movs-src-seg stos-width xchg-skip; do
    if POOLRAD_AUDIT_MUTANT=$m nice -n 15 python3 "$audit" "$run" "$game" >/dev/null 2>&1; then
        survived+=("$m")
        echo "START mutant $m -> survived"
    else
        killed=$((killed + 1))
        echo "START mutant $m -> killed"
    fi
done
echo "== mutants: killed $killed of 16, survived: ${survived[*]:-none}"
if [[ "${survived[*]:-}" != "no-hw-frame" ]]; then
    echo "unexpected mutant survival set (expected exactly: no-hw-frame)" >&2
    exit 1
fi
echo "window store replay audit and mutants verified"
