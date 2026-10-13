#!/usr/bin/env python3
"""Verify ignored source and native RAM identities before the C comparison."""

from hashlib import sha256
from pathlib import Path
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: audit_slot_a_key_dispatch_bridge.py GAME.OVR RETURN_RAM GATE_RAM")

expected = (
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (655360, "5ab1d3fa57949ce7c3e2b4dbd0ff1d6d2f96787f0dd13bc8cbc10b44713bda37"),
    (655360, "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"),
)
images = [Path(name).read_bytes() for name in sys.argv[1:]]
for image, identity in zip(images, expected):
    if (len(image), sha256(image).hexdigest()) != identity:
        raise SystemExit("private source or RAM identity mismatch")

span = images[0][0x40E0:0x40FF]
if (len(span), sha256(span).hexdigest()) != (
    31, "9526e20e6f32a143fc5647e2dd73efa5408bc67d1ab8e5c7004e25cbab650784"
):
    raise SystemExit("key-return instruction span mismatch")
loaded_start = (0x3582 << 4) + 0x339A
if any(ram[loaded_start:loaded_start + len(span)] != span for ram in images[1:]):
    raise SystemExit("loaded key-return instruction span mismatch")
print("hash-locked key-return source and RAM passed")
