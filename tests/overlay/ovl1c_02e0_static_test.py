#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 1Ch +02E0 (ECL opcode 0x0A, LOAD CHARACTER).

Reads the owner's GAME.OVR at run time (never committed), takes the 212 bytes at file offset 0x1026 (unit base 0x0D46 + 0x02E0),
and decodes them with capstone in 16-bit mode. Checks the facts the translation and the oracle rely on:
  - span length and SHA-256;
  - 70 instructions tile the span exactly, with one RETF as the last instruction (no argument bytes);
  - the four far calls, in order, at +02EE, +02FA, +0394 and +03A1, to 0x2B:0x2A, 0x2B:0x25, 0x78:0x2A and 0xBA:0x767;
  - the two items pushed before the first two calls are constants 1 (MOV AL,1), and the flag-path jumps (JE to +03B0) exist.
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
FILE_OFF = 0x1026          # unit 1Ch base 0x0D46 + routine offset 0x02E0
OFF, LEN = 0x02E0, 212     # next routine is outside this span; the span is the routine's own bytes
SPAN_SHA = "07903667cd47edfcd46ca97d08b10a6bb494b07008fd02290b22b642d7e42333"
INSNS = 70
RETF_AT = 0x03B3
CALLS = [(0x02EE, 0x2B, 0x2A), (0x02FA, 0x2B, 0x25), (0x0394, 0x78, 0x2A), (0x03A1, 0xBA, 0x767)]
MOV_AL_1 = [0x02EB, 0x02F7, 0x0391]   # mov al,1 before each push of an argument word (the 0x78 call's second word)
FLAG_JUMP_TARGET = 0x03B0             # the three conditional jumps that skip the calls go to the frame teardown

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
    check(lcalls == CALLS, "the four far calls are at +02EE, +02FA, +0394, +03A1 to 0x2B:0x2A, 0x2B:0x25, 0x78:0x2A, 0xBA:0x767")
    for addr in MOV_AL_1:
        hit = [i for i in rows if i.address == addr and i.mnemonic == "mov" and i.op_str == "al, 1"]
        check(len(hit) == 1, "mov al,1 at +%04X (argument word low byte)" % addr)
    skips = [i for i in rows if i.mnemonic.startswith("j") and i.op_str == "0x%x" % FLAG_JUMP_TARGET]
    check(len(skips) == 3, "three conditional jumps go to +%04X (the flag path's skip)" % FLAG_JUMP_TARGET)
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
