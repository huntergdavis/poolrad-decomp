#!/usr/bin/env python3
"""Audit U1C:+33BD CMP/JA bytes and the saved comparison operand."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
SPAN_SHA = "3ac45bdf90c77cb28ac33e47374a54322c23d4cf81b392c7738140d1b917f3be"


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
    original = game[0x4103 : 0x410B]
    loaded = ram[0x38BDD : 0x38BE5]
    assert original == loaded and digest(original) == SPAN_SHA, "unexpected CMP/JA bytes"
    assert original == bytes.fromhex("26 83 bd 94 05 01 77 0d")
    assert ram[(0x3A16 << 4) + 0x594 : (0x3A16 << 4) + 0x596] == b"\x00\x00"
    print("U1C +33BD CMP/JA source and saved zero operand: PASS")


if __name__ == "__main__":
    main()
