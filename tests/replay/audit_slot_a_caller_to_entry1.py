#!/usr/bin/env python3
"""Pin private caller bytes and same-run RAM fields without publishing media."""

from hashlib import sha256
from pathlib import Path
import sys

if len(sys.argv) != 4:
    raise SystemExit("usage: audit_slot_a_caller_to_entry1.py GAME.OVR AND_RAM SEARCH_RAM")

expected = (
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (655360, "9c82fae60d03939669c8f8a256a96ad0910376c80e07f31ade33eadc1ba04f95"),
    (655360, "cac6b2b97ea481dbb4bf4c165dec5035bc769beac80773e5fa661bab1600ee1d"),
)
game, and_ram, search_ram = [Path(name).read_bytes() for name in sys.argv[1:]]
for data, identity in zip((game, and_ram, search_ram), expected):
    if (len(data), sha256(data).hexdigest()) != identity:
        raise SystemExit("caller source or native RAM identity mismatch")

original = game[3398 + 0x345B:3398 + 0x3492]
if sha256(original).hexdigest() != \
        "3970e585476250421f70a482f2315571c16ca249bf4395293efbdc14e5475df8":
    raise SystemExit("original caller bytes mismatch")
if sha256(original[:50]).hexdigest() != \
        "2fd3cff4ce75d0d714af53fff8bc44caa2c331505414354a8245c8e73bdfe58e":
    raise SystemExit("gate/write prefix mismatch")

for ram in (and_ram, search_ram):
    loaded = ram[(0x3582 << 4) + 0x345B:(0x3582 << 4) + 0x3492]
    if sha256(loaded).hexdigest() != \
            "785a11da0a7c4bf8328f5865751fe96597ab3a1dc13ff84100b1337d366daf1a":
        raise SystemExit("loaded caller bytes mismatch")
    if loaded[:50] != original[:50] or \
            loaded[50:] != bytes((0x9A, 0x25, 0x00, 0x96, 0x08)):
        raise SystemExit("loaded entry-1 call boundary mismatch")

def byte(ram, seg, off):
    return ram[(seg << 4) + off]

def word(ram, seg, off):
    return int.from_bytes(ram[(seg << 4) + off:(seg << 4) + off + 2], "little")

for ram, party, old_xy, time, stage in (
    (and_ram, (15, 4, 6), (1, 4), (0, 5, 10), (0, 0)),
    (search_ram, (14, 4, 6), (15, 4), (1, 5, 10), (0, 1)),
):
    if (word(ram, 0x14A0, 0x49D2), word(ram, 0x14A0, 0x49D4)) != (0, 0x3996):
        raise SystemExit("window-0 pointer mismatch")
    if tuple(byte(ram, 0x14A0, x) for x in (0x442F, 0x49FF)) != (0, 0):
        raise SystemExit("caller gate fields mismatch")
    if tuple(byte(ram, 0x14A0, x) for x in (0x6AAD, 0x6AAE, 0x6AAF)) != party:
        raise SystemExit("party tuple mismatch")
    if tuple(word(ram, 0x3996, x) for x in (0x1E0, 0x1E2)) != old_xy:
        raise SystemExit("caller comparison words mismatch")
    if tuple(word(ram, 0x3996, x) for x in (0x18E, 0x190, 0x192)) != time:
        raise SystemExit("time words mismatch")
    if tuple(byte(ram, 0x14A0, x) for x in (0x84E0, 0x84E1)) != stage:
        raise SystemExit("closing-script stage mismatch")

if word(search_ram, 0x1D16, 0x3EF0) != 0x34EA or \
        word(search_ram, 0x1D16, 0x3EF2) != 0x3582 or \
        word(search_ram, 0x1D16, 0x3EF4) != 0x9975:
    raise SystemExit("later search call frame mismatch")

print("hash-locked caller prefix and same-run field projections passed")
