#!/usr/bin/env python3
"""Audit original and saved loaded U1C +3193 MOV AX,DS:[49ED] bytes."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
SPAN_SHA = "a3a6c864923d7041eee399509b2db45372283dbeeba687188aa23cd14e817e4d"


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
    original = game[0x3ED9:0x3EDC]
    loaded = ram[0x389B3:0x389B6]
    assert original == loaded == bytes.fromhex("A1 ED 49")
    assert digest(original) == SPAN_SHA, "unexpected +3193 opcode bytes"
    print("U1C +3193 MOV AX,DS:[49ED] original/loaded source: PASS")


if __name__ == "__main__":
    main()
