#!/usr/bin/env python3
"""Original-file boundary check for GAME.OVR U5E entry 2; emits no bytes."""

from hashlib import sha256
from pathlib import Path
import sys


GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "adb2fe5a76a3a1ebad49a09614e60fc89080130eafab392b01bcfb2c70ba4f1e"


def main(path: Path) -> None:
    game = path.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    body = game[0x1291E:0x1298C]
    assert len(body) == 110 and sha256(body).hexdigest() == BODY_SHA

    def at(offset: int) -> int:
        return body[offset - 0x0123]

    assert at(0x0178) == 0x9A and at(0x018E) == 0xCA
    assert at(0x018F) == 4 and at(0x0190) == 0
    assert int.from_bytes(body[0x0179 - 0x0123:0x017B - 0x0123],
                          "little") == 0x002F
    assert int.from_bytes(body[0x017B - 0x0123:0x017D - 0x0123],
                          "little") == 0x00B0
    print("U5E entry 2 original source PASS: 110 bytes, far call +0178, "
          "outer RETF4 +018E")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else
         root / "resources/DOS/POOLRAD/GAME.OVR")
