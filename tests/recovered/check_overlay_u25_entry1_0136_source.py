#!/usr/bin/env python3
"""Hash-locked original source for GAME.OVR U25 entry 1; no bytes emitted."""

from hashlib import sha256
from pathlib import Path
import sys


GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "0dd25983c54922e1ebe6f9f1b693f32c6a1c2b69199ddf110fc7d040134ec42b"


def main(path: Path) -> None:
    game = path.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    body = game[0x7609:0x7689]
    assert len(body) == 128 and sha256(body).hexdigest() == BODY_SHA

    def at(offset: int) -> int:
        return body[offset - 0x0136]

    assert at(0x018D) == 0x0E and at(0x018E) == 0xE8
    assert at(0x01A6) == 0x0E and at(0x01A7) == 0xE8
    assert at(0x01B3) == 0xCA and at(0x01B4) == 2 and at(0x01B5) == 0
    for call, successor, target in ((0x018E, 0x0191, 0x0E0C),
                                    (0x01A7, 0x01AA, 0x06AF)):
        relative = int.from_bytes(body[call + 1 - 0x0136:call + 3 - 0x0136],
                                  "little", signed=True)
        assert (successor + relative) & 0xFFFF == target
    print("U25 entry 1 original source PASS: 128 bytes, two PUSH CS/CALL "
          "near frames, outer RETF2")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    main(Path(sys.argv[1]) if len(sys.argv) > 1 else
         root / "resources/DOS/POOLRAD/GAME.OVR")
