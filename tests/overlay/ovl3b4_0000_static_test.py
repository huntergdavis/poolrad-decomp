#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 0x3B4 +0x0000 (the unit's entry in ledger slot 2).

Reads the owner's GAME.OVR at run time (never committed), takes the 199 bytes at file offset 0x2DB87 (unit code, +0x0000), and decodes
them with capstone in 16-bit mode. Checks the facts the translation and the oracle rely on:
  - span length and SHA-256; 72 instructions tile the span; one plain RETF last at +0x00C6;
  - the two external calls, at +0x00A9 (0x3E9:0x39) and +0x00BE (0x464:0x1B52);
  - the record reads (les di,[0x49D2] and the words at +0x186 and +0x188), the table lookup, the signed clamps (cmp with 2 and 0x2A), the
    unsigned bands (0x22, 0x23), the division steps (cdq and idiv cx with CX = 2 and 6), and the DS:0x6DDC update.
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
FILE_OFF = 0x2DB87
OFF, LEN = 0x0000, 199
SPAN_SHA = "4a0b67222fd84d903b9a817b68b7cd6398cbc864b7737e198427034682788880"
INSNS = 72
RETF_AT = 0x00C6
LCALLS = [(0x00A9, 0x3E9, 0x39), (0x00BE, 0x464, 0x1B52)]
TEXT_AT = [
    (0x0006, "al, byte ptr [0x49fa]"),
    (0x0009, "ah, ah"),
    (0x000D, "al, byte ptr [di + 0x3c76]"),
    (0x0013, "di, ptr [0x49d2]"),
    (0x0043, "di, ptr [0x49d2]"),
    (0x004F, "di, ptr [0x49d2]"),
    (0x0053, "al, byte ptr es:[di + 0x188]"),
    (0x005D, "di, ptr [0x49d2]"),
    (0x006F, "di, ptr [0x49d2]"),
    (0x0073, "word ptr es:[di + 0x188], 0x23"),
    (0x0017, "ax, word ptr es:[di + 0x186]"),
    (0x001C, "byte ptr [bp - 1], al"),
    (0x001F, "byte ptr [bp - 1], 2"),
    (0x0027, "byte ptr [bp - 1], 0x2a"),
    (0x0047, "word ptr es:[di + 0x188], 2"),
    (0x0061, "word ptr es:[di + 0x188], 0x22"),
    (0x0073, "word ptr es:[di + 0x188], 0x23"),
    (0x008D, "al, byte ptr [0x6aaf]"),
    (0x0091, "al, byte ptr [0x6ddc]"),
    (0x0097, "cx, 2"),
    (0x00A3, "ax, 1"),
    (0x00A5, "ax, 9"),
    (0x00B5, "cx, 6"),
    (0x00BB, "byte ptr [0x6ddc], al"),
]
CDQ_AT = [0x0096, 0x00B4]
IDIV_AT = [0x009A, 0x00B8]

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
    check(lcalls == LCALLS, "the two external calls are at +00A9 (3E9:39) and +00BE (464:1B52)")
    for addr, text in TEXT_AT:
        hit = [i for i in rows if i.address == addr and i.op_str == text]
        check(len(hit) == 1, "%s at +%04X" % (text, addr))
    for addr in CDQ_AT:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "cdq"]
        check(len(hit) == 1, "sign-extension of AX into DX (capstone cdq; CWD in 16-bit mode) at +%04X" % addr)
    for addr in IDIV_AT:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "idiv" and i.op_str == "cx"]
        check(len(hit) == 1, "signed divide by CX (idiv cx) at +%04X" % addr)
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
