#!/usr/bin/env python3
"""Static original-byte boundary gate for U9E entry8 +0E05..+0E0D."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"
LOOP_SHA = "dc1422933023de40130adfaddf73ed03e684a443f89d39b69fc4f3c27d1b107d"
BASE = 0x2AEEA


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--original", required=True, type=Path)
    args = parser.parse_args()
    game = args.original.read_bytes()
    assert sha256(game) == GAME_SHA
    body = game[BASE + 0x0DE3 : BASE + 0x0E2E]
    loop = game[BASE + 0x0E05 : BASE + 0x0E0E]
    assert len(body) == 75 and sha256(body) == BODY_SHA
    assert len(loop) == 9 and sha256(loop) == LOOP_SHA
    assert loop == bytes.fromhex("fe 46 fe 8a 46 06 30 e4 50")
    assert game[BASE + 0x0E0E : BASE + 0x0E13] == bytes.fromhex(
        "9a c0 11 f8 0a"
    )
    branch = game[BASE + 0x0E1D : BASE + 0x0E1F]
    assert branch[0] == 0x75 and 0x0E1F + int.from_bytes(
        branch[1:], "little", signed=True
    ) == 0x0E05
    assert game[BASE + 0x0E28 : BASE + 0x0E2E] == bytes.fromhex(
        "89 ec 5d ca 04 00"
    )
    print("U9E +0E05 original-byte, JNE target, call and RETF4 boundaries: PASS")


if __name__ == "__main__":
    main()
