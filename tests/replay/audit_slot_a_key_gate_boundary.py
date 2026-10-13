#!/usr/bin/env python3
"""Pin original and loaded caller code plus the two private native stops."""

from hashlib import sha256
from pathlib import Path
import sys

if len(sys.argv) != 5:
    raise SystemExit("usage: audit_slot_a_key_gate_boundary.py GAME.OVR SAVGAMA.DAT GATE_RAM SCRIPT_RAM")

expected = (
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (13137, "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"),
    (655360, "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"),
    (655360, "f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558"),
)
images = [Path(name).read_bytes() for name in sys.argv[1:]]
for image, identity in zip(images, expected):
    if (len(image), sha256(image).hexdigest()) != identity:
        raise SystemExit("private source, save, or RAM identity mismatch")

for lo, hi, original_hash, loaded_hash, changed_offsets in (
    (0x33B9, 0x33D2, "1ddd391622c3fa430154267e383c5e87b98c345b5be24045c6b0fe73ef6f5dc6",
     "3c9de3e73f45621e739bad3af041d182cdfdc8f300b55015917fb1a01921e758",
     (0x33CC, 0x33CD)),
    (0x344C, 0x345B, "78e63a36dc9b316d491208384157139d4d075881476e03b2e7bd870e4b2a313d",
     "78e63a36dc9b316d491208384157139d4d075881476e03b2e7bd870e4b2a313d", ()),
):
    original = images[0][0xD46 + lo:0xD46 + hi]
    if sha256(original).hexdigest() != original_hash:
        raise SystemExit("original key-gate code mismatch")
    for ram in images[2:]:
        loaded = ram[(0x3582 << 4) + lo:(0x3582 << 4) + hi]
        if (sha256(loaded).hexdigest() != loaded_hash or
                tuple(lo + i for i, (a, b) in enumerate(zip(original, loaded))
                      if a != b) != changed_offsets):
            raise SystemExit("loaded key-gate code mismatch")
print("hash-locked key gate and script-call spans passed")
