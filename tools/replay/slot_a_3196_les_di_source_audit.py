#!/usr/bin/env python3
"""Audit original and saved loaded U1C +3196 LES DI,DS:[49DE] bytes."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
SPAN_SHA = "759845587fe686131425f382f9450fa52081a01a49a025d4561d0402247c93cc"


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
    original = game[0x3EDC:0x3EE0]
    loaded = ram[0x389B6:0x389BA]
    assert original == loaded == bytes.fromhex("C4 3E DE 49")
    assert digest(original) == SPAN_SHA, "unexpected +3196 opcode bytes"
    print("U1C +3196 LES DI,DS:[49DE] original/loaded source: PASS")


if __name__ == "__main__":
    main()
