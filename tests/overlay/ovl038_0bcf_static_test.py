#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 38h +0BCF (009E slot 34, table DS:68B0, stub 0041:00B1, public entry 29).

Reads the owner's GAME.OVR at run time (never committed), decodes the 55-byte span with capstone in 16-bit mode, and
checks the facts the native capture and the C composition rely on:
  - span offset (60507 + 0x0BCF), length 55 (next entry +0C06), SHA-256;
  - 22 instructions tile the span exactly, with one RETF 0Ah as the last instruction at +0C03;
  - the two far calls, at +0BE5 and +0BFD, both to 00B0:0025 (the same target);
  - the item numbers pushed before each call (2Bh at +0BD2, 2Ch at +0BEA) and the selector byte read at [BP+6].
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
UNIT_BASE = 60507          # unit 38h code base in GAME.OVR (from the overlay inventory)
OFF, LEN = 0x0BCF, 55      # next entry OVL_0038_0C06 at +0C06
SPAN_SHA = "9f1ec667c9bd2c225500980e91f7ea7112afcd9858ac407b2433fbd8c23dec4a"
INSNS = 22
RETF_AT = 0x0C03
CALLS = [(0x0BE5, 0x00B0, 0x0025), (0x0BFD, 0x00B0, 0x0025)]
ITEM_MOVES = [(0x0BD2, "al, 0x2b"), (0x0BEA, "al, 0x2c")]

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
    check(len(rows) == INSNS and sum(i.size for i in rows) == LEN,
          "%d instructions tile the %d-byte span" % (INSNS, LEN))
    rets = [i for i in rows if i.mnemonic == "retf"]
    check(len(rets) == 1 and rets[0].address == RETF_AT and rets[0].op_str.strip() == "0xa" and rows[-1] is rets[0],
          "exactly one RETF 0Ah, at +%04X, as the last instruction" % RETF_AT)
    lcalls = []
    for i in rows:
        if i.mnemonic == "lcall":
            seg, off = [int(x, 16) for x in i.op_str.split(", ")]
            lcalls.append((i.address, seg, off))
    check(lcalls == CALLS, "the two far calls are at +0BE5 and +0BFD, both to 00B0:0025")
    for addr, text in ITEM_MOVES:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "mov" and i.op_str == text]
        check(len(hit) == 1, "item number moved at +%04X (%s)" % (addr, text))
    sel = [i for i in rows if i.mnemonic == "mov" and i.op_str == "al, byte ptr [bp + 6]"]
    check(len(sel) == 2, "the selector byte [BP+6] is read twice (before each call)")
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
