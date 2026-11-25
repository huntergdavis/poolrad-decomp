#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 0x1c +0x2B7E (ECL opcode 0x36, ADD NPC).

Reads the owner's GAME.OVR at run time (never committed), takes the 133 bytes at file offset 0x38C4 and decodes them with
capstone in 16-bit mode. Checks the facts the model and the oracle rely on: span length and SHA-256; 43 instructions tile
the span; one plain RETF as the last instruction at +0x2C02; the six far calls in order; the slot and record operations.
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
FILE_OFF = 0x38C4
BASE = 0x2B7E
LEN = 133
SPAN_SHA = "5b711971cd12bfbcd60d87f2ac6a6cb836292b4729229417fa73962d48efeca1"
INSNS = 43
RETF_AT = 0x2C02
LCALLS = [(0x2B, 0x2A), (0x2B, 0x25), (0x7C, 0x43), (0x2B, 0x25), (0xBA, 0x0BB8), (0xBA, 0x0767)]
TEXT_AT = [
    (0x2B84, "mov al, 2"),
    (0x2B8C, "mov al, 1"),
    (0x2BA0, "mov al, 2"),
    (0x2BAB, "mov al, byte ptr [bp - 6]"),
    (0x2BB0, "shr ax, 1"),
    (0x2BB2, "mov byte ptr [bp - 6], al"),
    (0x2BB8, "or al, 0x80"),
    (0x2BC0, "les di, ptr [0x5d92]"),
    (0x2BC4, "mov byte ptr es:[di + 0x84], al"),
    (0x2BC9, "cmp byte ptr [bp - 5], 0x18"),
    (0x2BD3, "mov byte ptr es:[di + 0x10e], 0"),
    (0x2BDF, "mov byte ptr es:[di + 0x10e], 1"),
    (0x2BE5, "push word ptr [0x5d94]"),
    (0x2BE9, "push word ptr [0x5d92]"),
    (0x2BF2, "push word ptr [0x5d94]"),
    (0x2BF6, "push word ptr [0x5d92]"),
]
XOR_AH_AT = [0x2BAE]
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
    check(len(span) == LEN, "span is %d bytes at GAME.OVR offset 0x%05X" % (LEN, FILE_OFF))
    check(hashlib.sha256(span).hexdigest() == SPAN_SHA, "span SHA-256 matches the pinned value")
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    rows = list(md.disasm(span, BASE))
    check(len(rows) == INSNS and sum(i.size for i in rows) == LEN,
          "%d instructions tile the %d-byte span" % (INSNS, LEN))
    rets = [i for i in rows if i.mnemonic == "retf"]
    check(len(rets) == 1 and rets[0].address == RETF_AT and rets[0].op_str.strip() == "" and rows[-1] is rets[0],
          "exactly one plain RETF, at +%04X, as the last instruction" % RETF_AT)
    lcalls = []
    for i in rows:
        if i.mnemonic == "lcall":
            seg, off = [int(x, 16) for x in i.op_str.split(", ")]
            lcalls.append((seg, off))
    check(lcalls == LCALLS, "six far calls in order: 2B:2A, 2B:25, 7C:43, 2B:25, BA:0BB8, BA:0767")
    for addr, text in TEXT_AT:
        hit = [i for i in rows if i.address == addr and i.op_str == text.split(" ", 1)[1] and i.mnemonic == text.split(" ", 1)[0]]
        hit2 = [i for i in rows if i.address == addr and (i.mnemonic + " " + i.op_str) == text]
        check(len(hit) + len(hit2) >= 1, "%s at +%04X" % (text, addr))
    for addr in XOR_AH_AT:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "xor" and i.op_str == "ah, ah"]
        check(len(hit) == 1, "xor ah, ah (slot 2 zero-extended) at +%04X" % addr)
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
