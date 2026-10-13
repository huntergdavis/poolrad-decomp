#!/usr/bin/env python3
"""Pin original/loaded EXIT code and three private same-run stops."""

from hashlib import sha256
from pathlib import Path
import sys

if len(sys.argv) != 6:
    raise SystemExit("usage: audit_slot_a_first_exit.py GAME.OVR SAVGAMA.DAT PRE_RAM AND_RAM SEARCH_RAM")

expected = (
    (206105, "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"),
    (13137, "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"),
    (655360, "f71571a2f22f6264dc39f52424a9d4d300a3429f2d1c7932cec8735a98ae0558"),
    (655360, "9c82fae60d03939669c8f8a256a96ad0910376c80e07f31ade33eadc1ba04f95"),
    (655360, "cac6b2b97ea481dbb4bf4c165dec5035bc769beac80773e5fa661bab1600ee1d"),
)
images = [Path(name).read_bytes() for name in sys.argv[1:]]
for image, identity in zip(images, expected):
    if (len(image), sha256(image).hexdigest()) != identity:
        raise SystemExit("source, save, or native RAM identity mismatch")

game, save = images[:2]
exit_source = game[0x0D47:0x0E3A]
runner_source = game[0x3EB7:0x3EF9]
if sha256(exit_source).hexdigest() != "9f2483f23b51d3dbc54a83592241cbd6f5bc27e1173e62f17c54c3b6672ca493":
    raise SystemExit("original EXIT body mismatch")
if sha256(runner_source).hexdigest() != "4055cdf54a816923efbebe1bed325b67193264be3f6c20491f4f383d692aa489":
    raise SystemExit("original runner body mismatch")

changed_offsets = (0x2D, 0x2E, 0x32, 0x33, 0x5F, 0x60, 0x64, 0x65, 0x8F, 0x90, 0xCF, 0xD0)
for ram in images[2:]:
    exit_loaded = ram[(0x3582 << 4) + 1:(0x3582 << 4) + 0xF4]
    runner_loaded = ram[(0x3582 << 4) + 0x3171:(0x3582 << 4) + 0x31B3]
    if sha256(exit_loaded).hexdigest() != "c09b0f2dd45cb0ccad3b25afa54ea0e885dff87873679d630319ee47310d70ff":
        raise SystemExit("loaded EXIT body mismatch")
    if tuple(1 + i for i, (a, b) in enumerate(zip(exit_source, exit_loaded)) if a != b) != changed_offsets:
        raise SystemExit("loaded EXIT difference offsets mismatch")
    if runner_loaded != runner_source:
        raise SystemExit("loaded runner body mismatch")
    ds = 0x14A0 << 4
    off = int.from_bytes(ram[ds + 0x49DE:ds + 0x49E0], "little")
    seg = int.from_bytes(ram[ds + 0x49E0:ds + 0x49E2], "little")
    script = ram[(seg << 4) + off:(seg << 4) + off + 0x1E00]
    if script != save[0x1401:0x3201]:
        raise SystemExit("native script differs from saved transfer")

print("hash-locked first EXIT code, script, and same-run RAM stops passed")
