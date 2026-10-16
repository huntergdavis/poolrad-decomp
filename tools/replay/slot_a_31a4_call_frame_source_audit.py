#!/usr/bin/env python3
"""Audit original and saved loaded U1C +31A4 PUSH CS/near CALL bytes."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
SPAN_SHA = "778ab0e467c211ff7983557c9aa5b3917ecefb983bb2f59f7e0da05c8fe59179"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("game_ovr", type=Path)
    parser.add_argument("saved_ram", type=Path)
    args = parser.parse_args()
    game = args.game_ovr.read_bytes()
    ram = args.saved_ram.read_bytes()
    assert len(game) == 206105 and digest(game) == GAME_SHA, "unexpected GAME.OVR"
    assert len(ram) == 655360 and digest(ram) == RAM_SHA, "unexpected +33BD RAM"
    original = game[0x3EEA:0x3EEE]
    loaded = ram[0x389C4:0x389C8]
    assert original == loaded == bytes.fromhex("0E E8 85 FD")
    assert digest(original) == SPAN_SHA, "unexpected +31A4 opcode bytes"
    print("U1C +31A4 PUSH CS/near CALL original/loaded source: PASS")


if __name__ == "__main__":
    main()
