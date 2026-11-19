#!/usr/bin/env python3
"""Audit private run-025 caller stops 345B/348D/3492; print no game or RAM payload."""

from hashlib import sha256
from pathlib import Path
import sys

STOPS = ("345b", "348d", "3492")
RAM_LOCKS = (
    "82f6f9c602dd9d40b3ff7b85041e6cbfd2a7add6347153fde8412c6350d2e981",
    "8c9aa3eee434e0bcc9ee3b260a588f2df645ee12a1fb6cbab0ea90b89eed0ea0",
    "f66a6d9a9234121c4d96e3b81adfddc818deb36c4973d87c4622a7628511c04c",
)
CPU_LOCKS = (
    "63e76717d312b802c063ba421fb74cab7fec81d9d2577a5e3c2be91e0c9d8826",
    "eacba5ce0db51b360c0ba676dd876e507f31a7733a4ed45bdb404e6f99dd3d8b",
    "ccbac0c055fc1868b5ceef5b4942d2eb72422111678d9cc7aebb0ae7d4e95433",
)
# Expected CPU register lines (hex); the hashes above lock the exact file bytes.
CPU_EXPECT = (
    {"AX": 0, "BX": 0x3ED4, "CX": 0, "DX": 0, "SI": 0x3E8A, "DI": 0x84CA, "BP": 0x3EF8,
     "SP": 0x3EF6, "CS": 0x3582, "IP": 0x345B, "DS": 0x14A0, "ES": 0x14A0, "SS": 0x1D16,
     "FLAGS": 0x7202},
    {"AX": 4, "BX": 0x3ED4, "CX": 0, "DX": 0, "SI": 0x3E8A, "DI": 0, "BP": 0x3EF8,
     "SP": 0x3EF6, "CS": 0x3582, "IP": 0x348D, "DS": 0x14A0, "ES": 0x3996, "SS": 0x1D16,
     "FLAGS": 0x7246},
    {"AX": 0, "BX": 0x3DA8, "CX": 0, "DX": 0x03C5, "SI": 0x3D9A, "DI": 0x6B06, "BP": 0x3EF8,
     "SP": 0x3EF6, "CS": 0x3582, "IP": 0x3492, "DS": 0x14A0, "ES": 0x14A0, "SS": 0x1D16,
     "FLAGS": 0x7246},
)
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
ORIGINAL_CALLER_HASH = "3970e585476250421f70a482f2315571c16ca249bf4395293efbdc14e5475df8"
LOADED_CALLER_HASH = "785a11da0a7c4bf8328f5865751fe96597ab3a1dc13ff84100b1337d366daf1a"
RAM_SIZE = 655360
DELTA_348D_TO_3492 = 6201


def check(condition, message):
    if not condition:
        raise SystemExit(message)


def address(segment, offset):
    return (segment << 4) + offset


def byte(ram, segment, offset):
    return ram[address(segment, offset)]


def word(ram, segment, offset):
    at = address(segment, offset)
    return int.from_bytes(ram[at:at + 2], "little")


def changes(before, after):
    return [(i, old, new) for i, (old, new) in enumerate(zip(before, after))
            if old != new]


def cpu_fields(text):
    parts = text.split()
    check(len(parts) == 14, "CPU line is not 14 fields")
    fields = {}
    for part in parts:
        name, _, value = part.partition("=")
        check(len(value) == 4 and name not in fields, "malformed CPU field")
        fields[name] = int(value, 16)
    return fields


check(len(sys.argv) == 8,
      "usage: audit_slot_a_run025_caller_stops.py GAME.OVR " +
      " ".join("RAM_" + stop.upper() for stop in STOPS) + " " +
      " ".join("CPU_" + stop.upper() for stop in STOPS))
game = Path(sys.argv[1]).read_bytes()
check(len(game) == 206105 and sha256(game).hexdigest() == GAME_HASH,
      "original GAME.OVR identity mismatch")
original = game[3398 + 0x345B:3398 + 0x3492]
check(sha256(original).hexdigest() == ORIGINAL_CALLER_HASH,
      "original caller bytes mismatch")

rams = {}
for stop, digest, path in zip(STOPS, RAM_LOCKS, sys.argv[2:5]):
    ram = Path(path).read_bytes()
    check(len(ram) == RAM_SIZE and sha256(ram).hexdigest() == digest,
          "native RAM identity mismatch at +" + stop)
    rams[stop] = ram
for stop, digest, expect, path in zip(STOPS, CPU_LOCKS, CPU_EXPECT, sys.argv[5:8]):
    raw = Path(path).read_bytes()
    check(sha256(raw).hexdigest() == digest, "CPU file identity mismatch at +" + stop)
    check(cpu_fields(raw.decode("ascii")) == expect, "CPU fields mismatch at +" + stop)

for stop in STOPS:
    loaded = rams[stop][address(0x3582, 0x345B):address(0x3582, 0x3492)]
    check(sha256(loaded).hexdigest() == LOADED_CALLER_HASH,
          "loaded caller bytes mismatch at +" + stop)
    check(loaded[:50] == original[:50], "loaded caller prefix mismatch at +" + stop)

check(changes(rams["345b"], rams["348d"]) == [(0x39B40, 1, 15)],
      "direct caller prefix full-RAM delta mismatch")
check(address(0x3996, 0x1E0) == 0x39B40, "window-0 +1E0 address mismatch")
check(len(changes(rams["348d"], rams["3492"])) == DELTA_348D_TO_3492,
      "full-RAM delta count mismatch: 348d to 3492")

for stop, party_x, minute, old_x in (("345b", 15, 0, 1), ("348d", 15, 0, 15),
                                     ("3492", 14, 1, 15)):
    ram = rams[stop]
    check((word(ram, 0x14A0, 0x49D2), word(ram, 0x14A0, 0x49D4)) == (0, 0x3996),
          "window-0 pointer mismatch at +" + stop)
    check(tuple(byte(ram, 0x14A0, off) for off in (0x442E, 0x442F, 0x49FF)) == (0, 0, 0),
          "caller gates mismatch at +" + stop)
    check(tuple(byte(ram, 0x14A0, off) for off in (0x6AAD, 0x6AAE, 0x6AAF)) ==
          (party_x, 4, 6), "party position mismatch at +" + stop)
    check(tuple(word(ram, 0x3996, off) for off in (0x18E, 0x190, 0x192, 0x1E0, 0x1E2)) ==
          (minute, 5, 10, old_x, 4), "window-0 clock/position mismatch at +" + stop)
    check((byte(ram, 0x14A0, 0x84E0), byte(ram, 0x14A0, 0x84E1)) == (0, 0),
          "caller stage mismatch at +" + stop)

print("run-025 caller stop identities, one-byte prefix delta and 6201-byte return delta passed")
