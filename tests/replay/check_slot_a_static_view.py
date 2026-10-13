#!/usr/bin/env python3
"""Compare a bounded C art region with an ignored DOSBox slot-A capture."""

import hashlib
import subprocess
import sys
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
PRIVATE = (ROOT / "resources").resolve()
HASHES = {
    "save": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
    "geo": "2035246014efdb26de3114f4737d52a935ad8834f79b4a8e4880da2922b6d595",
    "art": "cce9313f23a600200fec60b5d46cd08aa9535a6143eee5d5d768a1d4e0415021",
    "screen": "2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134",
}
EGA_RGB = (
    (0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
)


def checked(path: Path, expected: str) -> Path:
    path = path.resolve(strict=True)
    if not path.is_relative_to(PRIVATE):
        raise ValueError("all private inputs must remain under resources/")
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise ValueError("private input hash mismatch")
    return path


def normalized_screen(path: Path) -> bytes:
    image = Image.open(path).convert("RGB")
    if image.size != (1280, 1024):
        raise ValueError("expected the pinned 1280x1024 DOSBox window capture")
    image = image.crop((320, 312, 960, 712))
    palette = {rgb: index for index, rgb in enumerate(EGA_RGB)}
    pixels = image.load()
    output = bytearray(320 * 200)
    for y in range(200):
        for x in range(320):
            color = pixels[2 * x, 2 * y]
            if (pixels[2 * x + 1, 2 * y] != color or
                    pixels[2 * x, 2 * y + 1] != color or
                    pixels[2 * x + 1, 2 * y + 1] != color):
                raise ValueError("capture is not exact 2x EGA pixels")
            if color not in palette:
                raise ValueError("capture color is outside the EGA palette")
            output[y * 320 + x] = palette[color]
    return bytes(output)


def main() -> None:
    if len(sys.argv) != 6:
        raise SystemExit("usage: check_slot_a_static_view.py PROBE SAVE GEO_DAX EGA_DAX PRIVATE_PNG")
    probe = Path(sys.argv[1]).resolve(strict=True)
    if not probe.is_relative_to(PRIVATE):
        raise ValueError("probe build must remain under resources/")
    save = checked(Path(sys.argv[2]), HASHES["save"])
    geo = checked(Path(sys.argv[3]), HASHES["geo"])
    art = checked(Path(sys.argv[4]), HASHES["art"])
    screen = normalized_screen(checked(Path(sys.argv[5]), HASHES["screen"]))
    # Empirical placement leads from the private screen, passed into a generic
    # blitter. This does not claim the DOS draw-selection path is translated.
    drawn = subprocess.run(
        [str(probe), str(save), str(geo), str(art),
         "40", "40", "64", "41", "48", "64"],
        check=True, capture_output=True,
    ).stdout
    if len(drawn) != 128000:
        raise ValueError("unexpected C frame size")
    index, covered = drawn[:64000], drawn[64000:]
    count = sum(covered)
    mismatch = sum(index[i] != screen[i] for i, mark in enumerate(covered) if mark)
    if count != 128 or mismatch:
        raise ValueError("bounded C art region differs from DOSBox")
    print(f"normalized=320x200 covered={count} mismatches={mismatch} unknown={64000-count}")


if __name__ == "__main__":
    main()
