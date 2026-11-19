#!/usr/bin/env python3
"""Audit private direct +0D6D/+0742 stops for the slice-1 adapter; print no game or RAM payload."""

from hashlib import sha256
from pathlib import Path
import sys

GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LOCKS = (
    ("RAM_0D6D", 655360, "ded9ac1d1db89fead04ed13a6515be9152662478e10d2edc721e6bb8cc5a868e"),
    ("RAM_0742", 655360, "2ef55fd5fd893f3e5397130a35bfc77777c4e126eda939bebda5da3cace7de4e"),
    ("CPU_0D6D", 115, "54564c50156141b2e550a2d59ad6c34df44be42425cc1de68ee18d9178d25cd3"),
    ("CPU_0742", 115, "0fcc08571fcf5771e1d9c177554ce9b192e75f5090789c8e93d2f38b94d89efd"),
)
ENTRY1_SHA = "a28055269b7373f984337022d88bfdea89993f8eb4a44ee69d6d8c23b2240c55"
UNIT68_FILE = 0x166CB          # unit 0x68 file offset (entry 1 +0x0D6D is file 0x17438)
# Original-byte skeletons the frame arithmetic relies on: (unit offset, length, SHA-256).
# Digests only; no original bytes are published.
SKELETONS = (
    (0x0D6D, 7, "ac7b855675a4ac988fe3f9399cd6a294dd72ab5e4b673dad25449f1d7844efdf"),   # push bp; mov bp,sp; sub sp,0x12C
    (0x0DA7, 4, "14470acec7c07571af0efb073e074fab9fb7da33c4f9f14d7fdf22181ddc8ee2"),   # mov byte [bp-1],1
    (0x1027, 10, "95d26270a6325429435a650b58bb6205bb9cd67983b0f455044c726123917676"),  # cmp [bp-1],0; je; push cs; call near +073B
    (0x073B, 10, "6e8c87ccf30d55ea8536ecba2d8557f7d84b75865717d271d504e2e4488158e1"),  # push bp; mov bp,sp; push [2628]
)


def check(condition, message):
    if not condition:
        raise SystemExit(message)


def cpu(path):
    fields = dict(part.split("=") for part in Path(path).read_text().split())
    return {k: int(v, 16) for k, v in fields.items()}


check(len(sys.argv) == 6, "usage: audit_slot_a_entry1_to_0742.py GAME.OVR RAM_0D6D RAM_0742 CPU_0D6D CPU_0742")
game = Path(sys.argv[1]).read_bytes()
check(len(game) == 206105 and sha256(game).hexdigest() == GAME_HASH, "original GAME.OVR identity mismatch")
blobs = {}
for (name, size, digest), path in zip(LOCKS, sys.argv[2:]):
    data = Path(path).read_bytes()
    check(len(data) == size and sha256(data).hexdigest() == digest, f"{name} identity mismatch")
    blobs[name] = data
span = game[0x17438:0x17438 + 826]
check(sha256(span).hexdigest() == ENTRY1_SHA, "original entry 1 span mismatch")
for offset, length, digest in SKELETONS:
    check(sha256(game[UNIT68_FILE + offset:UNIT68_FILE + offset + length]).hexdigest() == digest,
          f"original bytes at +{offset:04X} do not match the frame skeleton")

c0, c1 = cpu(sys.argv[4]), cpu(sys.argv[5])
check((c0["CS"], c0["IP"], c1["CS"], c1["IP"]) == (0x2E60, 0x0D6D, 0x2E60, 0x0742), "CPU stops are not +0D6D/+0742")
check(c0["SP"] == 0x3EF2 and c0["BP"] == 0x3EF8 and c1["SP"] == 0x3DBC and c1["BP"] == 0x3DBE, "unexpected SP/BP")
check(c0["DS"] == c1["DS"] == 0x14A0 and c0["SS"] == c1["SS"] == 0x1D16, "unexpected DS/SS")
for label, ram in (("+0D6D", blobs["RAM_0D6D"]), ("+0742", blobs["RAM_0742"])):
    base = (0x2E60 << 4)
    for offset, length, digest in SKELETONS:
        at = base + offset
        check(sha256(ram[at:at + length]).hexdigest() == digest, f"loaded code at 2E60:{offset:04X} differs at {label}")
print("slot-A entry1 +0D6D/+0742 private inputs, original skeletons and loaded code verified")
