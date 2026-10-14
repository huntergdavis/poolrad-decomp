#!/usr/bin/env python3
"""Audit private direct search-script return stops without publishing payloads."""

from hashlib import sha256
from pathlib import Path
import sys

STOPS = ("34e7", "3171", "026c", "34ea", "34f5", "3395")
HASHES = (
    "04ade382f947462c95505389a6b729ee9d0bf3a4836b807c29316264c28375c2",
    "6e1b669c855b74826bc6e4af1dbd3c91190ef649dab3ad9b1e55ba02a17c7a47",
    "2d4195a044bdcebff8c776cfb2990978d06062a70df8a537673e8d232a106ae9",
    "07be274372542cdb93d9454bd7013f0f04c78b7f2d3f53238cfbe60a34057a11",
    "07be274372542cdb93d9454bd7013f0f04c78b7f2d3f53238cfbe60a34057a11",
    "07be274372542cdb93d9454bd7013f0f04c78b7f2d3f53238cfbe60a34057a11",
)
INPUTS = (
    (65231, "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"),
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (13137, "4b64213a9d19d082af6a075b7aa54407c5cfe3883f6ade1f3862283c130d6ca9"),
)


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


def change_count(before, after):
    return sum(left != right for left, right in zip(before, after))


check(len(sys.argv) == 10,
      "usage: audit_slot_a_script_return_native.py START.EXE GAME.OVR "
      "SAVGAMJ.DAT RAM_34E7 RAM_3171 RAM_026C RAM_34EA RAM_34F5 RAM_3395")
start, game, saved = [Path(path).read_bytes() for path in sys.argv[1:4]]
for data, (size, digest) in zip((start, game, saved), INPUTS):
    check(len(data) == size and sha256(data).hexdigest() == digest,
          "private executable, overlay, or save identity mismatch")

rams = {}
for stop, digest, path in zip(STOPS, HASHES, sys.argv[4:]):
    ram = Path(path).read_bytes()
    check(len(ram) == 655360 and sha256(ram).hexdigest() == digest,
          "native RAM identity mismatch at +" + stop)
    rams[stop] = ram

for name, low, high, expected_hash in (
        ("runner", 0x3171, 0x31B3,
         "4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489"),
        ("caller gate", 0x34E7, 0x3508,
         "317fd3546f947679efeb34c4c490c9002e836323aabf40b5c35425700b40807e")):
    original = game[3398 + low:3398 + high]
    check(len(original) == high - low and
          sha256(original).hexdigest() == expected_hash,
          "original " + name + " bytes mismatch")
    for stop, ram in rams.items():
        loaded = ram[address(0x3582, low):address(0x3582, high)]
        check(loaded == original, "loaded " + name + " bytes mismatch at +" + stop)

before, entered = rams["34e7"], rams["3171"]
changed = [(index, left, right) for index, (left, right) in
           enumerate(zip(before, entered)) if left != right]
check(changed == [(address(0x1D16, 0x3EF0), 0xD8, 0xEA)],
      "near-call return-IP push mismatch")
check((word(entered, 0x1D16, 0x3EF0),
       word(entered, 0x1D16, 0x3EF2),
       word(entered, 0x1D16, 0x3EF4)) ==
      (0x34EA, 0x3582, 0x9975), "script-runner call frame mismatch")
check(change_count(entered, rams["026c"]) == 9550,
      "in-run dispatcher full-RAM delta mismatch")
check(change_count(rams["026c"], rams["34ea"]) == 42,
      "in-run dispatcher to return full-RAM delta mismatch")
check(change_count(entered, rams["34ea"]) == 9556,
      "script-runner return full-RAM delta mismatch")
check(rams["34ea"] == rams["34f5"] == rams["3395"],
      "caller post-script gates unexpectedly changed RAM")

for stop, pc, previous, opcode in (
        ("34e7", 0x9975, 0x2F, 0x00),
        ("3171", 0x9975, 0x2F, 0x00),
        ("026c", 0x9B40, 0x00, 0x08),
        ("34ea", 0x9B68, 0x1A, 0x00),
        ("34f5", 0x9B68, 0x1A, 0x00),
        ("3395", 0x9B68, 0x1A, 0x00)):
    ram = rams[stop]
    check(word(ram, 0x14A0, 0x49E5) == 0x9975 and
          word(ram, 0x14A0, 0x49ED) == pc,
          "ECL entry/PC mismatch at +" + stop)
    check((byte(ram, 0x14A0, 0x6F7E),
           byte(ram, 0x14A0, 0x6F7F)) == (previous, opcode),
          "opcode trail mismatch at +" + stop)
    check(tuple(byte(ram, 0x14A0, off) for off in
                (0x442E, 0x442F, 0x49FF)) == (0, 0, 0),
          "stop/branch gate mismatch at +" + stop)
    check(tuple(byte(ram, 0x14A0, off) for off in
                (0x6AAD, 0x6AAE, 0x6AAF, 0x84E0, 0x84E1)) ==
          (14, 4, 6, 0, 1), "party/stage mismatch at +" + stop)
    check(tuple(word(ram, 0x3996, off) for off in
                (0x18E, 0x190, 0x192, 0x1E0, 0x1E2)) ==
          (1, 5, 10, 15, 4), "window-0 state mismatch at +" + stop)

print("direct search-script return and zero-gate loop audit passed")
