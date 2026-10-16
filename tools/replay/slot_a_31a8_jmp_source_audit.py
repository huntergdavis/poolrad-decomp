#!/usr/bin/env python3
"""Check U1C +31A8 original and saved loaded JMP bytes; no native replay."""

from hashlib import sha256
from pathlib import Path
import sys

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
JMP_SHA = "5c4631109e1393c47d57ef4ea5192cde998e2db47aa8a6cbdea61d791910e060"


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: slot_a_31a8_jmp_source_audit.py GAME.OVR saved-ram.bin")
    game = Path(sys.argv[1]).read_bytes()
    ram = Path(sys.argv[2]).read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    original = game[0x3EEE:0x3EF0]
    loaded = ram[0x389C8:0x389CA]
    assert original == loaded == bytes((0xEB, 0xD5))
    assert sha256(original).hexdigest() == JMP_SHA
    displacement = int.from_bytes(original[1:], "little", signed=True)
    assert (0x31AA + displacement) & 0xFFFF == 0x317F
    print("U1C +31A8 original/loaded JMP to +317F: PASS")


if __name__ == "__main__":
    main()
