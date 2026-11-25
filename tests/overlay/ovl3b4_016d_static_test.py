#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 0x3B4 +0x016D (view refresh, public entry 1).

Reads the owner's GAME.OVR at run time (never committed), takes the 307 bytes at unit +0x016D (file offset 0x2DCF4 = unit code 0x2DB87
+ 0x016D), and decodes them with capstone in 16-bit mode. Checks the facts the translation and the oracle rely on:
  - span length and SHA-256; 114 instructions tile the span; one plain RETF last at +0x029F; the span ends where the next prologue
    (55 89 E5) starts at +0x02A0;
  - the four far calls, at +0x019B, +0x0207, +0x0229 and +0x0293, to 0x3D5:0x34, 0x3D5:0x25, 0x3D5:0x39 and 0x3CD:0x25;
  - the three near case-map calls, at +0x01D9, +0x01E5 and +0x01F1 (to +0x00C7, +0x00FB and +0x012F), each preceded by push cs, and the
    near call to +0x0000 at +0x0299 (also preceded by push cs);
  - the flag, record and table reads by address and operand text, and the two CBW (capstone: cwde) instructions of the coordinate path.
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
UNIT_FILE = 0x2DB87
OFF, LEN = 0x016D, 307
SPAN_SHA = "c6bb0b220344d76f8924b0f96bcea506bb55dbdad753e2f945fa25bf782255e8"
INSNS = 114
RETF_AT = 0x029F
LCALLS = [(0x019B, 0x3D5, 0x34), (0x0207, 0x3D5, 0x25), (0x0229, 0x3D5, 0x39), (0x0293, 0x3CD, 0x25)]
NEAR_CALLS = [(0x01D9, 0x00C7), (0x01E5, 0x00FB), (0x01F1, 0x012F), (0x0299, 0x0000)]
PUSH_CS = [0x01D8, 0x01E4, 0x01F0, 0x0298]
TEXT_AT = [
    (0x0173, "al, byte ptr [0x49fa]"),
    (0x0179, "di, ptr [0x49d2]"),
    (0x017D, "word ptr es:[di + 0x1cc], 0"),
    (0x0185, "byte ptr [bp - 3], 1"),
    (0x0193, "al, byte ptr [0x6aad]"),
    (0x0197, "al, byte ptr [0x6aae]"),
    (0x01A0, "byte ptr [0x6ab1], al"),
    (0x01A3, "byte ptr [0x6ab1], 0x7f"),
    (0x01A8, "0x1bc"),
    (0x01AE, "di, word ptr es:[di + 0x1fc]"),
    (0x01C0, "di, word ptr es:[di + 0x1fa]"),
    (0x01B3, "al, byte ptr [di + 0xc26]"),
    (0x01C5, "al, byte ptr [di + 0xc26]"),
    (0x01CC, "byte ptr [0x84e5], 0"),
    (0x01DC, "byte ptr [0x6a61], al"),
    (0x0210, "word ptr es:[di + 0x1f6], 0"),
    (0x0218, "byte ptr [0x6aac], 0"),
    (0x0238, "al, byte ptr [bp - 3]"),
    (0x023B, "ah, ah"),
    (0x023F, "al, byte ptr [di + 0x3c76]"),
    (0x0249, "ax, word ptr es:[di + 0x186]"),
    (0x0250, "byte ptr [bp - 1], al"),
    (0x026B, "ax, word ptr es:[di + 0x188]"),
    (0x0272, "byte ptr [bp - 2], al"),
    (0x028D, "ax"),
]
CWDE_AT = [0x028C, 0x0291]

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
    span = ovr[UNIT_FILE + OFF:UNIT_FILE + OFF + LEN]
    check(len(span) == LEN, "span is %d bytes at GAME.OVR offset 0x%05X" % (LEN, UNIT_FILE + OFF))
    check(hashlib.sha256(span).hexdigest() == SPAN_SHA, "span SHA-256 matches the pinned value")
    nxt = ovr[UNIT_FILE + OFF + LEN:UNIT_FILE + OFF + LEN + 3]
    check(nxt == bytes([0x55, 0x89, 0xE5]), "the next prologue (55 89 E5) starts at +0x02A0")
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
    check(lcalls == LCALLS, "the four far calls are at +019B, +0207, +0229, +0293 to 3D5:34, 3D5:25, 3D5:39, 3CD:25")
    near = [(i.address, int(i.op_str, 0)) for i in rows if i.mnemonic == "call"]
    check(near == NEAR_CALLS, "the four near calls are at +01D9, +01E5, +01F1 (case maps) and +0299 (+0000)")
    for addr in PUSH_CS:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "push" and i.op_str == "cs"]
        check(len(hit) == 1, "push cs at +%04X (the pushed return CS of a near call)" % addr)
    for addr, text in TEXT_AT:
        hit = [i for i in rows if i.address == addr and i.op_str == text]
        check(len(hit) == 1, "%s at +%04X" % (text, addr))
    for addr in CWDE_AT:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "cwde"]
        check(len(hit) == 1, "sign-extension of AL into AX (capstone cwde, the byte 0x98; CBW in 16-bit mode) at +%04X" % addr)
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
