#!/usr/bin/env python3
"""Audit original and loaded U1C +33EA JMP bytes."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
JMP_SHA = "081b3ef897938b3abd1047243d7466dffdd346bc2f81b28e925f18898f66ab8c"


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
    original = game[0x4130 : 0x4132]
    loaded = ram[0x38C0A : 0x38C0C]
    assert original == loaded == b"\xeb\x40"
    assert digest(original) == JMP_SHA
    print("U1C +33EA JMP original/loaded source: PASS")


if __name__ == "__main__":
    main()
