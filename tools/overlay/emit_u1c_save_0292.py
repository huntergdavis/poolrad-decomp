#!/usr/bin/env python3
"""Generate src/recovered/overlay_u1c_save_0292_cpu.c and its header from GAME.OVR.

python3 tools/overlay/emit_u1c_save_0292.py [--check]
Candidate-scout draft, no credit claimed. Needs the owner media under resources/.
"""
from __future__ import annotations

import sys
from hashlib import sha256
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz  # noqa: E402
import candidate_ecl_template as T  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
UNIT, START, END = 0x1C, 0x0292, 0x02E0
STEM = "overlay_u1c_save_0292_cpu"
FN = "pr_u1c_save_0292"
DESCRIPTION = ("GAME.OVR unit 1Ch +[0292,02E0): ECL opcode 0x09 SAVE. Far Pascal, no "
               "arguments, RETF. Loads 2 operands; slot 1 is the value, slot 2 the "
               "destination; tag[1] >= 80h is a string SAVE, else a word SAVE.")
CALLS = {0x2A: "pr_candidate_load_operands", 0x4D: "pr_candidate_operand_word",
         0x25: "pr_candidate_slot_value", 0x52: "pr_candidate_store_word",
         0x5C: "pr_candidate_store_string"}


def main(argv):
    exe, ovr = EXE.read_bytes(), OVR.read_bytes()
    if sha256(exe).hexdigest() != EXE_SHA or sha256(ovr).hexdigest() != OVR_SHA:
        sys.exit("REFUSED: media provenance differs")
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[UNIT]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    header, source = T.render(code, STEM, FN, START, END, CALLS, DESCRIPTION)
    targets = {ROOT / "include/poolrad/recovered" / (STEM + ".h"): header,
               ROOT / "src/recovered" / (STEM + ".c"): source}
    if "--check" in argv:
        bad = [str(p) for p, text in targets.items() if not p.exists() or p.read_text() != text]
        print("EMIT_CHECK", "MISMATCH " + ", ".join(bad) if bad else "OK")
        return 1 if bad else 0
    for path, text in targets.items():
        path.write_text(text)
    print("cases", source.count("case 0x"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
