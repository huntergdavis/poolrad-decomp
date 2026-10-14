#!/usr/bin/env python3
"""Audit private direct post-EXIT stops; print no game or RAM payload."""

from hashlib import sha256
from pathlib import Path
import sys

STOPS = ("345b", "348d", "3492", "3497", "34cf", "34d8", "34e7")
HASHES = (
    "fa6a4a111160c6116b0303428b6d20552ffa44b7f7d5b67863ceadfe01859299",
    "0faaf7d0c7af057339dcafb6af540d29d2ae98749d62c235d499bd1a596a8c13",
    "eb692580c5952ab6398cbdbe8044a5a71d562df9155047c42e989f771f273a5f",
    "e6445efc550a21a435a96823791dd1cd74b25fb5103560815e6fb56026592bc8",
    "e6445efc550a21a435a96823791dd1cd74b25fb5103560815e6fb56026592bc8",
    "1e17583b6d78d99e629a59436d201626c01b93aee9f0c45e0f519b14aee82d50",
    "04ade382f947462c95505389a6b729ee9d0bf3a4836b807c29316264c28375c2",
)
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LOADED_CALLER_HASH = "785a11da0a7c4bf8328f5865751fe96597ab3a1dc13ff84100b1337d366daf1a"


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


check(len(sys.argv) == 9,
      "usage: audit_slot_a_post_exit_native.py GAME.OVR " +
      " ".join("RAM_" + stop.upper() for stop in STOPS))
game = Path(sys.argv[1]).read_bytes()
check(len(game) == 206105 and sha256(game).hexdigest() == GAME_HASH,
      "original GAME.OVR identity mismatch")
rams = {}
for stop, digest, path in zip(STOPS, HASHES, sys.argv[2:]):
    ram = Path(path).read_bytes()
    check(len(ram) == 655360 and sha256(ram).hexdigest() == digest,
          "native RAM identity mismatch at +" + stop)
    rams[stop] = ram

original = game[3398 + 0x345B:3398 + 0x3492]
check(sha256(original).hexdigest() ==
      "3970e585476250421f70a482f2315571c16ca249bf4395293efbdc14e5475df8",
      "original caller bytes mismatch")
for stop in STOPS:
    loaded = rams[stop][address(0x3582, 0x345B):address(0x3582, 0x3492)]
    check(sha256(loaded).hexdigest() == LOADED_CALLER_HASH,
          "loaded caller bytes mismatch at +" + stop)
    check(loaded[:50] == original[:50], "loaded caller prefix mismatch")

check(changes(rams["345b"], rams["348d"]) ==
      [(address(0x3996, 0x1E0), 1, 15)],
      "direct caller prefix full-RAM delta mismatch")
for before, after, count in (("348d", "3492", 6198),
                             ("3492", "3497", 832),
                             ("3497", "34cf", 0),
                             ("34cf", "34d8", 95)):
    check(len(changes(rams[before], rams[after])) == count,
          "full-RAM delta count mismatch: " + before + " to " + after)
check(changes(rams["34d8"], rams["34e7"]) ==
      [(address(0x14A0, 0x84E1), 0, 1),
       (address(0x1D16, 0x3EF4), 0x0A, 0x75),
       (address(0x1D16, 0x3EF5), 0, 0x99)],
      "caller stage and script-argument writes mismatch")

for stop, party_x, minute, old_x, stage in (
        ("345b", 15, 0, 1, 0), ("348d", 15, 0, 15, 0),
        ("3492", 14, 1, 15, 0), ("3497", 14, 1, 15, 0),
        ("34cf", 14, 1, 15, 0), ("34d8", 14, 1, 15, 0),
        ("34e7", 14, 1, 15, 1)):
    ram = rams[stop]
    check((word(ram, 0x14A0, 0x49D2), word(ram, 0x14A0, 0x49D4)) ==
          (0, 0x3996), "window-0 pointer mismatch at +" + stop)
    check(tuple(byte(ram, 0x14A0, off) for off in
                (0x442E, 0x442F, 0x49FF)) == (0, 0, 0),
          "caller gates mismatch at +" + stop)
    check(tuple(byte(ram, 0x14A0, off) for off in
                (0x6AAD, 0x6AAE, 0x6AAF)) == (party_x, 4, 6),
          "party position mismatch at +" + stop)
    check(tuple(word(ram, 0x3996, off) for off in
                (0x18E, 0x190, 0x192, 0x1E0, 0x1E2)) ==
          (minute, 5, 10, old_x, 4),
          "window-0 clock/position mismatch at +" + stop)
    check((byte(ram, 0x14A0, 0x84E0),
           byte(ram, 0x14A0, 0x84E1)) == (0, stage),
          "caller stage mismatch at +" + stop)
    check(word(ram, 0x14A0, 0x49E5) == 0x9975 and
          word(ram, 0x14A0, 0x49ED) == 0x9975,
          "script entry/PC mismatch at +" + stop)

check(word(rams["34e7"], 0x1D16, 0x3EF4) == 0x9975,
      "pre-call script argument mismatch")
print("direct post-EXIT native stop identities and write attribution passed")
