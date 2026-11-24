#!/bin/bash
# Composition check for OVL_0038_07E0: compiles the C composition with strict warnings, runs the baseline and the four
# mutants from the replayed entry state, and compares each end state with the replayed post-body snapshot.
# usage: check_ovl0038_07e0_composition.sh BODY_BIN RUN_COMP_DIR COMPARATOR_PY OUT_DIR
#   BODY_BIN      the 312-byte span extracted from the owner GAME.OVR (+0x07E0), not committed
#   RUN_COMP_DIR  directory holding handler-entry-ram.bin, handler-entry-regs14.txt, effects-07e0.txt,
#                 post-body-ram.bin and post-body-regs14.txt (the replay output)
#   COMPARATOR_PY tests/replay/compare_ovl0038_017f_composition.py
#   OUT_DIR       scratch directory for the compiled binary and per-mutant outputs
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="$HERE/compose_ovl0038_07e0_body.c"
BODY="$1"; COMP="$2"; CMP="$3"; OUT="$4"
mkdir -p "$OUT"
gcc -std=c11 -Wall -Wextra -Werror -O2 -o "$OUT/compose07e0" "$SRC" || { echo "FAIL compile"; exit 1; }
fail=0
for m in none retf-pop-0C retaddr-call arg-byte-16EC record-3E-value; do
  D="$OUT/$m"; mkdir -p "$D"
  "$OUT/compose07e0" "$BODY" "$COMP/handler-entry-ram.bin" "$COMP/handler-entry-regs14.txt" "$COMP/effects-07e0.txt" "$D/c-final-ram.bin" "$m" > "$D/compose.out" 2> "$D/compose.err"; rc=$?
  python3 "$CMP" --comp "$COMP" --c-ram "$D/c-final-ram.bin" --c-regs "$D/c-final-ram.bin.regs" --out "$D/composition-result.json" > "$D/compare.out" 2> "$D/compare.err"; crc=$?
  verdict=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['verdict'])" "$D/composition-result.json" 2>/dev/null)
  echo "$m compose_rc=$rc compare_rc=$crc verdict=${verdict:-none}"
  if [ "$m" = none ] && [ "$verdict" != "EQUAL" ]; then fail=1; fi
done
exit $fail
