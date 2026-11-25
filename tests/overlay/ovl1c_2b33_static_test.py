#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 1Ch +2B33 (ECL opcode 0x39, WHO).

Reads the owner's GAME.OVR at run time (never committed), takes the 75 bytes at file offset 0x3879 (unit base 0x0D46 + 0x2B33), and
decodes them with capstone in 16-bit mode. Checks the facts the translation and the oracle rely on:
  - span length and SHA-256;
  - 36 instructions tile the span exactly, with one plain RETF as the last instruction;
  - the four far calls, in order, at +2B3D, +2B4E, +2B62 and +2B75, to 0x2B:0x2A, 0x709:0x0DD3, 0xAF8:0x0B25 and 0xBA:0x2C48;
  - the frame reservation (sub sp,0x100) and the constants that build the argument words (mov al,1 / 0x11 / 0x26 / 0x16, mov ax,0xff,
    mov al,0);
  - the StrStore source offset (mov di,0x70c8) and the final pointer offset (mov di,0x5d92).
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
FILE_OFF = 0x3879          # unit 1Ch base 0x0D46 + routine offset 0x2B33
OFF, LEN = 0x2B33, 75
SPAN_SHA = "b373a049e2df103b77382331370aefb2a5378b0f9d66b947333c16298e646083"
INSNS = 36
RETF_AT = 0x2B7D
CALLS = [(0x2B3D, 0x2B, 0x2A), (0x2B4E, 0x709, 0xDD3), (0x2B62, 0xAF8, 0xB25), (0x2B75, 0xBA, 0x2C48)]
CONSTS = [(0x2B3A, "al, 1"), (0x2B42, "al, 1"), (0x2B45, "al, 0x11"), (0x2B48, "al, 0x26"),
          (0x2B4B, "al, 0x16"), (0x2B5E, "ax, 0xff"), (0x2B6D, "al, 0")]
MOVS = [(0x2B36, "sp, 0x100"), (0x2B53, "di, 0x70c8"), (0x2B70, "di, 0x5d92")]

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
    check(lcalls == CALLS, "the four far calls are at +2B3D, +2B4E, +2B62, +2B75 to 0x2B:0x2A, 0x709:0x0DD3, 0xAF8:0x0B25, 0xBA:0x2C48")
    for addr, text in CONSTS:
        hit = [i for i in rows if i.address == addr and i.op_str == text]
        check(len(hit) == 1, "constant %s at +%04X (argument word or AX load)" % (text, addr))
    for addr, text in MOVS:
        hit = [i for i in rows if i.address == addr and i.op_str == text]
        check(len(hit) == 1, "%s at +%04X" % (text, addr))
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
