#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 38h +07E0 (009E slot 23, public entry 21, stub 0041:0089).

Reads the owner's GAME.OVR at run time (never committed), checks the 312-byte span, decodes it with capstone in
16-bit mode, and checks the facts the native capture and the C composition rely on:
  - span offset (60507 + 0x07E0), length, SHA-256;
  - 103 instructions tile the span exactly;
  - exactly one RETF (0Ah) at +0915, reached by the epilogue;
  - the eight static far calls in address order, and the seven that the captured path executes, in order;
  - the composition's callee tables (tests/replay/compose_ovl0038_07e0_body.c) match the decode.
Prints one PASS/FAIL line per check and exits non-zero on any FAIL.
"""
import argparse
import hashlib
import os
import re
import sys

try:
    import capstone
except ImportError:  # pragma: no cover
    print("FAIL capstone is required")
    sys.exit(2)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_OVR = os.path.join(ROOT, "resources", "DOS", "POOLRAD", "GAME.OVR")
UNIT_BASE = 60507          # unit 38h code base in GAME.OVR (from the overlay inventory)
OFF, LEN = 0x07E0, 312
SPAN_SHA = "e69db1e310b0ec6e71b5b97975e799e122561e24d933abd9ba02922b011f4fd4"
RETF_AT = 0x0915
# static far calls in address order: (address, target seg, target off, executed on the captured path)
STATIC_CALLS = [
    (0x0852, 0x00BA, 0x1509, False),  # selector non-zero only; not on the selector-0 path
    (0x087B, 0x0AF8, 0x023F, True),
    (0x0892, 0x0AF8, 0x16A8, True),
    (0x08D3, 0x00BA, 0x15F7, True),
    (0x08E2, 0x0AF8, 0x0254, True),
    (0x08F7, 0x0AF8, 0x0B0B, True),
    (0x0902, 0x00BA, 0x16EC, True),
    (0x090D, 0x00BA, 0x0BB8, True),
]
COMPOSE_C = os.path.join(ROOT, "tests", "replay", "compose_ovl0038_07e0_body.c")

failures = 0


def check(ok, msg):
    global failures
    print(("PASS " if ok else "FAIL ") + msg)
    if not ok:
        failures += 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--game-ovr", default=DEFAULT_OVR, help="path to the owner GAME.OVR (ignored, not committed)")
    a = ap.parse_args()
    if not os.path.exists(a.game_ovr):
        print("FAIL owner media not present at the given --game-ovr path (ignored, not committed)")
        return 2
    ovr = open(a.game_ovr, "rb").read()
    span = ovr[UNIT_BASE + OFF: UNIT_BASE + OFF + LEN]
    check(len(span) == LEN, "span is %d bytes at GAME.OVR offset %d" % (LEN, UNIT_BASE + OFF))
    check(hashlib.sha256(span).hexdigest() == SPAN_SHA, "span SHA-256 matches the pinned value")
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    rows = list(md.disasm(span, OFF))
    check(len(rows) == 103 and sum(i.size for i in rows) == LEN, "103 instructions tile the 312-byte span")
    rets = [i for i in rows if i.mnemonic == "retf"]
    check(len(rets) == 1 and rets[0].address == RETF_AT and rets[0].op_str.strip() == "0xa",
          "exactly one RETF 0Ah, at +%04X" % RETF_AT)
    lcalls = []
    for i in rows:
        if i.mnemonic == "lcall":
            seg, off = [int(x, 16) for x in i.op_str.split(", ")]
            lcalls.append((i.address, seg, off))
    check(lcalls == [(a, s, o) for a, s, o, _ in STATIC_CALLS],
          "the eight static far calls match the expected list (address, target)")
    executed = [(a, s, o) for a, s, o, ex in STATIC_CALLS if ex]
    check(len(executed) == 7, "seven far calls lie on the captured selector-0 path")
    # composition callee table must match the seven executed calls in order
    src = open(COMPOSE_C).read() if os.path.exists(COMPOSE_C) else ""
    seg_m = re.search(r"CALLEE_SEG_OF\[NCALLS\] = \{([^}]*)\}", src)
    off_m = re.search(r"CALLEE_OFF_OF\[NCALLS\] = \{([^}]*)\}", src)
    segs = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]+)u", seg_m.group(1))] if seg_m else []
    offs = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]+)u", off_m.group(1))] if off_m else []
    check(segs == [s for _, s, _ in executed] and offs == [o for _, _, o in executed],
          "composition callee tables match the seven executed calls in order")
    check(os.path.exists(COMPOSE_C), "composition source present at tests/replay/compose_ovl0038_07e0_body.c")
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
