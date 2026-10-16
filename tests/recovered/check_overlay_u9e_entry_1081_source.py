#!/usr/bin/env python3
"""Hash-locked source boundary for GAME.OVR U9E +1081; no game bytes emitted."""

from hashlib import sha256
from pathlib import Path
import sys


GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "a75691411c86d02b21a163db942f0564c574b6ab4f97f4cecb4b90c6c5b4c0fc"
STARTS = (
    0x1081, 0x1082, 0x1084, 0x1087, 0x108B, 0x108E, 0x1091,
    0x1094, 0x1095, 0x1098, 0x1099, 0x109A, 0x109F, 0x10A1,
    0x10A3, 0x10A6, 0x10A9, 0x10AC, 0x10AD, 0x10AE, 0x10B1,
    0x10B2, 0x10B3, 0x10B8, 0x10BA, 0x10BB, 0x10BD, 0x10BE,
    0x10C3, 0x10C6, 0x10C9, 0x10CC, 0x10CD, 0x10D0, 0x10D3,
    0x10D4, 0x10D7, 0x10DB, 0x10DE, 0x10E0, 0x10E1,
)


def main(path: Path) -> None:
    game = path.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    body = game[0x2BF6B:0x2BFCE]
    assert len(body) == 99 and sha256(body).hexdigest() == BODY_SHA
    assert len(STARTS) == 41 and STARTS[0] == 0x1081 and STARTS[-1] == 0x10E1

    def at(offset: int) -> int:
        return body[offset - 0x1081]

    assert tuple(at(i) for i in (0x109A, 0x10B3, 0x10BE)) == (0x9A,) * 3
    assert at(0x10D4) == 0xE8 and at(0x10A1) == 0x74
    assert 0x10A3 + (at(0x10A2) ^ 0x80) - 0x80 == 0x10DB
    assert at(0x10E1) == 0xCA and at(0x10E2) == 6 and at(0x10E3) == 0
    assert 0x10D7 + int.from_bytes(body[0x10D5 - 0x1081:0x10D7 - 0x1081],
                                  "little", signed=True) == 0x0026
    print("U9E +1081 original source PASS: 99 bytes, 41 starts, 3 far calls, "
          "1 near call, 1 RETF6")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else
         root / "resources/DOS/POOLRAD/GAME.OVR")
