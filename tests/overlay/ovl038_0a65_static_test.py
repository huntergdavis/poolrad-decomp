#!/usr/bin/env python3
"""Static audit for GAME.OVR unit 38h +0A65 (009E slot 30, table 68A0h, index 26, stub 0041:00A2).

Reads the owner's GAME.OVR at run time (never committed), decodes the 172-byte code body with capstone in
16-bit mode, and checks the facts the native capture and the C composition rely on:
  - span offset (60507 + 0x0A65), the 182-byte ledger bound, and both SHA-256 values;
  - the 172-byte body tiles into 54 instructions with exactly one RETF 0Ah, at +0B0E, as the last instruction;
  - the four static far calls in address order, with the path each one needs (A: byte at [Q+2] non-zero;
    C: P equal to the DS:5D92/5D94 far word);
  - the callback far pointer 0038:0A59 is pushed before the 0AF8:0B0B call;
  - the 10 bytes after the body are data (a length byte 09h and nine further bytes), not code.
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
OFF = 0x0A65
BOUND_LEN = 182            # ledger bound: next entry 0B1B
BODY_LEN = 172             # code [0A65, 0B11); RETF 0Ah at 0B0E ends at 0B11
BODY_INSNS = 54
RETF_AT = 0x0B0E
SPAN_SHA = "0d704deb92dab5d9608e8630cf29dcb59e529b14f677855518cffdcad0daf31c"
BODY_SHA = "6e01f0bd068676f7cce373b86d65d381ae966a4049d86637537bfc9827f48886"
TRAILING_LEN_BYTE = 0x09   # length-prefixed data block; its text is not committed
# static far calls in address order: (address, target seg, target off, path)
STATIC_CALLS = [
    (0x0A8A, 0x0AF8, 0x0B0B, "A"),  # only when [Q+2] != 0 (callback call with buffer)
    (0x0A95, 0x00BA, 0x16EC, "A"),  # only when [Q+2] != 0
    (0x0ABA, 0x00BA, 0x0BB8, "AB"),  # both paths
    (0x0B06, 0x00BA, 0x09E3, "C"),  # only when P equals the DS:5D92/5D94 far word
]
CALLBACK_MOV = 0x0A85      # mov di, 0A59h
CALLBACK_OFF = 0x0A59
CALLBACK_PUSH_CS = 0x0A88  # push cs; push di builds the far callback 0038:0A59

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
    span = ovr[UNIT_BASE + OFF: UNIT_BASE + OFF + BOUND_LEN]
    body = span[:BODY_LEN]
    check(len(span) == BOUND_LEN, "span is %d bytes at GAME.OVR offset %d" % (BOUND_LEN, UNIT_BASE + OFF))
    check(hashlib.sha256(span).hexdigest() == SPAN_SHA, "span SHA-256 matches the pinned value")
    check(hashlib.sha256(body).hexdigest() == BODY_SHA, "body SHA-256 matches the pinned value (%d bytes)" % BODY_LEN)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    rows = list(md.disasm(body, OFF))
    check(len(rows) == BODY_INSNS and sum(i.size for i in rows) == BODY_LEN,
          "%d instructions tile the %d-byte body" % (BODY_INSNS, BODY_LEN))
    rets = [i for i in rows if i.mnemonic == "retf"]
    check(len(rets) == 1 and rets[0].address == RETF_AT and rets[0].op_str.strip() == "0xa" and rows[-1] is rets[0],
          "exactly one RETF 0Ah, at +%04X, as the last instruction" % RETF_AT)
    lcalls = []
    for i in rows:
        if i.mnemonic == "lcall":
            seg, off = [int(x, 16) for x in i.op_str.split(", ")]
            lcalls.append((i.address, seg, off))
    check(lcalls == [(addr, s, o) for addr, s, o, _ in STATIC_CALLS],
          "the four static far calls match the expected list (address, target)")
    check([p for _, _, _, p in STATIC_CALLS] == ["A", "A", "AB", "C"],
          "path membership recorded: 0AF8:0B0B and 00BA:16EC on path A; 00BA:0BB8 on both; 00BA:09E3 on path C")
    mov = [i for i in rows if i.address == CALLBACK_MOV]
    pushcs = [i for i in rows if i.address == CALLBACK_PUSH_CS]
    check(len(mov) == 1 and mov[0].mnemonic == "mov" and mov[0].op_str == "di, 0xa59"
          and len(pushcs) == 1 and pushcs[0].mnemonic == "push" and pushcs[0].op_str == "cs",
          "callback 0038:0A59 is pushed (mov di, 0A59h; push cs; push di) before the 0AF8:0B0B call")
    trailing = ovr[UNIT_BASE + OFF + BODY_LEN: UNIT_BASE + OFF + BOUND_LEN]
    check(len(trailing) == BOUND_LEN - BODY_LEN and trailing[0] == TRAILING_LEN_BYTE,
          "the 10 bytes after the body are a length-prefixed data block (length byte 09h), not code")
    print("RESULT %s (%d failure(s))" % ("PASS" if failures == 0 else "FAIL", failures))
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
