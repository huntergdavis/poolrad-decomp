#!/usr/bin/env python3
"""Original-byte contract for GAME.OVR unit 0x3B9 + 0x0708 (public entry 7).

Pins the executable span and its private CS literal pool by SHA-256 (the
literal text is not reproduced), every far-call site by opcode, offset and
segment word, the instruction bytes the model's semantics rest on (including
the only load of the literal, at +0737), and both GAME.OVR caller sites.
Reads the owner-held GAME.OVR only when present at
resources/DOS/POOLRAD/GAME.OVR (or $POOLRAD_GAME_OVR); otherwise prints the
contract and exits 0 with 'skipped'.
"""
import hashlib
import os
import sys

GAME_OVR_SHA256 = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
UNIT_FILE_OFFSET = 0x2E0CE          # unit 0x3B9
UNIT_SIZE = 1989
UNIT_SHA256 = "fd883756075e2fd9e6269cc6c262ec8b7870b13d2ee5d115cb307176bf387892"
SPAN = (0x0708, 0x07C0)             # 184 bytes, 74 instruction starts, retf 6 at +07BD
SPAN_SHA256 = "e86596a8c7b47ea2dd6ff9a00a8419242be12e2f1ae306605745938e10eb7a93"
POOL = (0x06E9, 0x0708)             # one 30-character Pascal string
POOL_SHA256 = "21de157c48f6fe8866352b63b056d43f1ddf1e49269b6ff33433863f0444adaa"
POOL_LENGTH = 30
CALLS = {0x071C: (0x0AF8, 0x0A3F), 0x072D: (0x0464, 0x00FF), 0x073F: (0x0AF8, 0x0919),
         0x0744: (0x0AF8, 0x084A), 0x0749: (0x0899, 0x0000), 0x07AC: (0x0464, 0x0D71),
         0x07B5: (0x0464, 0x1B52)}
PINS = {
    0x0708: "5589e583ec46",            # push bp; mov bp,sp; sub sp,0x46
    0x070E: "c47e06",                  # les di,[bp+6]: the record far pointer
    0x0718: "b84200",                  # mov ax,0x42: the copy size
    0x0721: "807e0a017206",            # cmp byte [bp+0xA],1; jb +072D
    0x0727: "807e0a037621",            # cmp byte [bp+0xA],3; jbe +074E
    0x0732: "bf0886",                  # mov di,0x8608: the Text record
    0x0737: "bfe9060e57",              # mov di,0x06E9; push cs; push di: the literal
    0x073C: "31c050",                  # xor ax,ax; push ax: width 0
    0x074E: "8a460a30e48bf8b103d3e7",  # di := index << 3
    0x0759: "8b43bc0b43be7452",        # [bp+di-0x44] | [bp+di-0x42]; je +07B3
    0x076C: "c47bbc897eba8c46bc",      # temporary := the slot's far pointer
    0x0778: "268b450405030048",        # x = es:[di+4] + 3 - 1
    0x0784: "268b450605030048",        # y = es:[di+6] + 3 - 1
    0x078D: "b00050b00150",            # k = AL 0, flags = AL 1 (AH carried)
    0x079E: "ff73beff73bc",            # push [bp+di-0x42]; push [bp+di-0x44]
    0x07A4: "ff36224aff36204a",        # push [0x4A22]; push [0x4A20]
    0x07B1: "eb02eb05",                # jmp +07B5 / jmp +07BA
    0x07BA: "89ec5dca0600",            # mov sp,bp; pop bp; retf 6
}
# Callers inside GAME.OVR, unit 0x25 (file 0x74D3) + 0x054E: mov di,0x6ABE; push ds; push di;
# lcall 0x3D0:0x0043 (entry 7) at +05D5 and +05E4.
CALLERS = ((0x74D3, 0x05D0), (0x74D3, 0x05DF))
CALLER_BYTES = "bfbe6a1e579a4300d003"


def main():
    root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    path = os.environ.get("POOLRAD_GAME_OVR", os.path.join(root, "resources", "DOS", "POOLRAD", "GAME.OVR"))
    print("contract: unit 0x3B9 entry 7 span [%04X,%04X) sha256 %s; pool [%04X,%04X) sha256 %s; %d far calls"
          % (SPAN[0], SPAN[1], SPAN_SHA256, POOL[0], POOL[1], POOL_SHA256, len(CALLS)))
    if not os.path.exists(path):
        print("skipped: GAME.OVR not present")
        return 0
    with open(path, "rb") as fh:
        data = fh.read()
    if hashlib.sha256(data).hexdigest() != GAME_OVR_SHA256:
        print("FAIL: GAME.OVR sha256 mismatch")
        return 1
    unit = data[UNIT_FILE_OFFSET:UNIT_FILE_OFFSET + UNIT_SIZE]
    checks = 0
    ok = True

    def expect(cond, what):
        nonlocal ok, checks
        checks += 1
        if not cond:
            print("FAIL: " + what)
            ok = False

    expect(hashlib.sha256(unit).hexdigest() == UNIT_SHA256, "unit sha256")
    expect(hashlib.sha256(unit[SPAN[0]:SPAN[1]]).hexdigest() == SPAN_SHA256, "span sha256")
    expect(hashlib.sha256(unit[POOL[0]:POOL[1]]).hexdigest() == POOL_SHA256, "literal pool sha256")
    expect(unit[POOL[0]] == POOL_LENGTH and POOL[0] + 1 + POOL_LENGTH == POOL[1], "literal length fills the pool")
    expect(unit.count(bytes.fromhex("bfe906")) == 1, "single mov di,0x06E9 in the unit")
    for site, (seg, off) in CALLS.items():
        b = unit[site:site + 5]
        expect(b[0] == 0x9A and int.from_bytes(b[1:3], "little") == off
               and int.from_bytes(b[3:5], "little") == seg, "lcall at +%04X" % site)
    for site, want in PINS.items():
        want = bytes.fromhex(want)
        expect(unit[site:site + len(want)] == want, "bytes at +%04X" % site)
    for unit_file, site in CALLERS:
        want = bytes.fromhex(CALLER_BYTES)
        expect(data[unit_file + site:unit_file + site + len(want)] == want, "U25 caller at +%04X" % site)
    print(("ok: %d checks" % checks) if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
