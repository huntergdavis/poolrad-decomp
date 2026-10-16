#!/usr/bin/env python3
"""Static U7D +03A3 caller-route gate; it performs no native replay."""

import argparse
import hashlib
from pathlib import Path

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BASE = 0x23DCC
U6A_BASE = 0x17831
U1C_BASE = 0x0D46
SPANS = {
    (0x03A3, 0x03D5): "3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad",
    (0x047A, 0x0565): "0c687ca966432b6c241673264978796173f6f31e91905a8a67ee3dee5110bc4e",
    (0x0721, 0x0871): "1d3e90cba8a713d25f6d090da93c5d5af02ffa2adf1df7de27e088dcb3409f16",
    (0x0C9A, 0x0E60): "9964836108967641b80c7c0983446af0b66055ff0bb2ad2302c626846ed8d631",
}
CALLS = {
    0x055A: 0x03A3,
    0x0813: 0x03A3,
    0x083A: 0x03A3,
    0x0D1C: 0x0721,
    0x0829: 0x047A,
    0x0836: 0x047A,
    0x0D9D: 0x047A,
}
U6A_SPANS = {
    (0x077A, 0x0824): "ffd763031e2e9fd1ddb8ce34b9789a383443844c57ef3901170fedd2790990c4",
    (0x13F1, 0x14A9): "86249c90195a22cbb52900f698e696c3ccda5ceac7056d10276e7e5a8a33203d",
    (0x1E6F, 0x1FD9): "7dbde3d1fb76a60c2a4d4be2d0eac524908f9e82635996e831ad23dfb0d96d5b",
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--original", required=True, type=Path)
    args = parser.parse_args()
    game = args.original.read_bytes()
    assert sha256(game) == GAME_SHA
    for (start, end), digest in SPANS.items():
        assert sha256(game[BASE + start : BASE + end]) == digest
    for (start, end), digest in U6A_SPANS.items():
        assert sha256(game[U6A_BASE + start : U6A_BASE + end]) == digest
    assert sha256(game[U1C_BASE + 0x2DD3 : U1C_BASE + 0x2E10]) == (
        "2097c463a248c594fa9d715b4c46d24335343fb630be7c9106df9a9369ae147e"
    )
    for offset, target in CALLS.items():
        code = game[BASE + offset : BASE + offset + 3]
        assert len(code) == 3 and code[0] == 0xE8
        relative = int.from_bytes(code[1:], "little", signed=True)
        assert (offset + 3 + relative) & 0xFFFF == target
    # These byte comparisons classify menu branches; they do not prove
    # any saved input reaches either branch in a native game run.
    assert game[BASE + 0x07F5 : BASE + 0x07F7] == bytes.fromhex("3c 49")
    assert game[BASE + 0x0818 : BASE + 0x081A] == bytes.fromhex("3c 44")
    assert game[BASE + 0x03D4] == 0xCB  # outer RETF
    assert game[U1C_BASE + 0x33E2 : U1C_BASE + 0x33E4] == bytes.fromhex(
        "3c 45"
    )
    assert game[U1C_BASE + 0x33E7 : U1C_BASE + 0x33EA] == bytes.fromhex(
        "e8 e9 f9"
    )
    assert game[U1C_BASE + 0x2DEA : U1C_BASE + 0x2DEF] == bytes.fromhex(
        "9a 25 00 75 00"
    )
    for compare_at, call_at in ((0x1493, 0x149D), (0x1F98, 0x1FA2)):
        assert game[U6A_BASE + compare_at : U6A_BASE + compare_at + 2] == (
            bytes.fromhex("3c 52")
        )
        code = game[U6A_BASE + call_at : U6A_BASE + call_at + 3]
        assert code[0] == 0xE8
        assert (call_at + 3 + int.from_bytes(code[1:], "little", signed=True)) & 0xFFFF == 0x077A
    assert game[U6A_BASE + 0x07FD : U6A_BASE + 0x0802] == bytes.fromhex(
        "9a 2a 00 8c 00"
    )
    print("U7D +03A3 static public-entry/menu/caller route: PASS; natural reach unproven")


if __name__ == "__main__":
    main()
