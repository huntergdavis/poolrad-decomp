#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 1Ch +2E10 (ECL opcode 0x38, PROGRAM).

Reads the owner's GAME.OVR at run time (never committed), takes the 109 bytes at file offset 0x3B56 (unit base 0x0D46 + 0x2E10), and
decodes them with capstone in 16-bit mode. Checks the facts the translation and the oracle rely on:
  - span length and SHA-256; 40 instructions tile the span; one plain RETF last;
  - the five far calls, in order, at +2E33, +2E3B, +2E4A, +2E4F and +2E5A, to 0x2B:0x2A, 0x2B:0x25, 0x78:0x25, 0xBA:0x27D9 and 0x82:0x25;
  - the two CS-pushed near calls (push cs before each), at +2E6C to +2DD3 and at +2E76 to +0001;
  - the flag test and copy instructions, the [0x49ED] save and restore, and the dispatch compares, by address and operand text.
Prints one PASS/FAIL line per check and exits non-zero on any FAIL.
"""
import argparse
import hashlib
import os
import sys

try:
    import capstone
except ImportError:  # pragma: no cover
    print("FAIL capstone is required")
    sys.exit(2)

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_OVR = os.path.join(ROOT, "resources", "DOS", "POOLRAD", "GAME.OVR")
FILE_OFF = 0x3B56          # unit 1Ch base 0x0D46 + routine offset 0x2E10
OFF, LEN = 0x2E10, 109
SPAN_SHA = "839f9df6a713088c9889d3d9969424d858f757a7ab51e1d7e05f9a8f0bab100c"
INSNS = 40
RETF_AT = 0x2E7C
LCALLS = [(0x2E33, 0x2B, 0x2A), (0x2E3B, 0x2B, 0x25), (0x2E4A, 0x78, 0x25), (0x2E4F, 0xBA, 0x27D9), (0x2E5A, 0x82, 0x25)]
NEAR_CALLS = [(0x2E6C, 0x2DD3), (0x2E76, 0x0001)]
PUSH_CS = [0x2E6B, 0x2E75]
TEXT_AT = [
    (0x2E16, "byte ptr [0x4430], 0"),
    (0x2E1D, "ax, word ptr [0x4436]"),
    (0x2E20, "dx, word ptr [0x4438]"),
    (0x2E24, "word ptr [0x5d92], ax"),
    (0x2E27, "word ptr [0x5d94], dx"),
    (0x2E2B, "byte ptr [0x4430], 0"),
    (0x2E30, "al, 1"),
    (0x2E38, "al, 1"),
    (0x2E43, "al, byte ptr [bp - 1]"),
    (0x2E46, "al, 0"),
    (0x2E56, "al, 8"),
    (0x2E61, "al, 9"),
    (0x2E65, "ax, word ptr [0x49ed]"),
    (0x2E68, "word ptr [bp - 3], ax"),
    (0x2E6F, "ax, word ptr [bp - 3]"),
    (0x2E72, "word ptr [0x49ed], ax"),
]

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
    span = ovr[FILE_OFF:FILE_OFF + LEN]
    check(len(span) == LEN, "span is %d bytes at GAME.OVR offset 0x%04X" % (LEN, FILE_OFF))
    check(hashlib.sha256(span).hexdigest() == SPAN_SHA, "span SHA-256 matches the pinned value")
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    rows = list(md.disasm(span, OFF))
    check(len(rows) == INSNS and sum(i.size for i in rows) == LEN,
          "%d instructions tile the %d-byte span" % (INSNS, LEN))
    rets = [i for i in rows if i.mnemonic == "retf"]
    check(len(rets) == 1 and rets[0].address == RETF_AT and rets[0].op_str.strip() == "" and rows[-1] is rets[0],
          "exactly one plain RETF, at +%04X, as the last instruction" % RETF_AT)
    lcalls = []
    for i in rows:
        if i.mnemonic == "lcall":
            seg, off = [int(x, 16) for x in i.op_str.split(", ")]
            lcalls.append((i.address, seg, off))
    check(lcalls == LCALLS, "the five far calls are at +2E33, +2E3B, +2E4A, +2E4F, +2E5A to the pinned targets")
    near = [(i.address, int(i.op_str, 0)) for i in rows if i.mnemonic == "call"]
    check(near == NEAR_CALLS, "the two near calls are at +2E6C to +2DD3 and +2E76 to +0001")
    for addr in PUSH_CS:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "push" and i.op_str == "cs"]
        check(len(hit) == 1, "push cs at +%04X (the pushed return CS of a near call)" % addr)
    for addr, text in TEXT_AT:
        hit = [i for i in rows if i.address == addr and i.op_str == text]
        check(len(hit) == 1, "%s at +%04X" % (text, addr))
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
