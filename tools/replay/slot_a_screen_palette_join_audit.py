#!/usr/bin/env python3
"""Join a private saved-slot screenshot to same-run EGA planes and DOS state.

Only counts and hashes are printed. The inputs must remain under resources/.
"""

import argparse
import hashlib
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
PRIVATE = (ROOT / "resources").resolve()
PLAYER = PRIVATE / "dosbox-x-heavy-src/src/dosbox-x"
PLAYER_HASH = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
HASHES = {
    "POOLRAD/START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "POOLRAD/SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
    "post-load-private.png": "2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134",
    "stable-paused-private.png": "2a776265f92f4f0a82fe73a64b0ac8d84d1beb93217630840eeec3f0ffb1b134",
    "ram-stable-first-view.bin": "e257bcc50c50e252313ce1c6e31273729bfe3c9839811426679c016a9cedd6c6",
    "a000-plane0-stable-first-view.bin": "68638729c3ac5a483f4fb023cf839dd4fea8432b51ec8e96d96d1fb3a8b35da1",
    "a000-plane1-stable-first-view.bin": "975cb7c40471ebb947734deae142c526bf37e0a756f3cb49c409492d432add35",
    "a000-plane2-stable-first-view.bin": "8c29a55dbd09cc924a3295263172b6336dd2d1c69caf3524f800af4929920404",
    "a000-plane3-stable-first-view.bin": "d1f05b349fd8db17bbf4739a7869448c4e8252f093db9c9592b597dfbb83c351",
}
EGA_RGB = (
    (0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
)


def checked(run: Path, name: str) -> Path:
    path = (run / name).resolve(strict=True)
    if not path.is_relative_to(run) or not path.is_relative_to(PRIVATE):
        raise ValueError("all private inputs must remain in the selected resources/ run")
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != HASHES[name]:
        raise ValueError(f"private input hash mismatch: {name}")
    return path


def ega_indices(planes: tuple[bytes, bytes, bytes, bytes]) -> bytes:
    if len(planes) != 4 or any(len(plane) != 8000 for plane in planes):
        raise ValueError("four 8,000-byte EGA planes required")
    out = bytearray(64000)
    for byte_index in range(8000):
        for bit in range(8):
            mask = 0x80 >> bit
            out[byte_index * 8 + bit] = sum(
                (1 << plane_index) for plane_index, plane in enumerate(planes)
                if plane[byte_index] & mask
            )
    return bytes(out)


def screen_indices(path: Path) -> tuple[bytes, int]:
    with Image.open(path) as source:
        image = source.convert("RGB")
    if image.size != (1280, 1024):
        raise ValueError("expected a 1280x1024 DOSBox capture")
    image = image.crop((320, 312, 960, 712))
    pixels = image.load()
    palette = {rgb: index for index, rgb in enumerate(EGA_RGB)}
    out = bytearray(64000)
    for y in range(200):
        for x in range(320):
            rgb = pixels[2 * x, 2 * y]
            if (pixels[2 * x + 1, 2 * y] != rgb or
                    pixels[2 * x, 2 * y + 1] != rgb or
                    pixels[2 * x + 1, 2 * y + 1] != rgb):
                raise ValueError("screenshot is not an exact 2x display")
            if rgb not in palette:
                raise ValueError("screenshot RGB value is outside the observed EGA palette")
            out[y * 320 + x] = palette[rgb]
    return bytes(out), len(set(out))


def display_state(ram: bytes) -> int:
    if len(ram) != 655360:
        raise ValueError("expected 640 KiB conventional RAM")
    ds = memoryview(ram)[0x14A00:]
    # BIOS Registers record (AX=0500), active/draw A000, page 0,
    # old mode 3, current mode 0D, EGA selector 1.
    expected = {
        0x4A02: bytes.fromhex("0005" + "00" * 12),
        0x4A16: bytes.fromhex("00a000a00000030d01"),
    }
    compared = 0
    for offset, value in expected.items():
        if ds[offset:offset + len(value)] != value:
            raise ValueError(f"DOS display state differs at DS:{offset:04X}")
        compared += len(value)
    return compared


def audit(run: Path) -> None:
    run = run.resolve(strict=True)
    if not run.is_dir() or not run.is_relative_to(PRIVATE):
        raise ValueError("run directory must be under this clone's ignored resources/")
    player = PLAYER.resolve(strict=True)
    if not player.is_relative_to(PRIVATE):
        raise ValueError("private DOSBox-X binary must remain under resources/")
    if hashlib.sha256(player.read_bytes()).hexdigest() != PLAYER_HASH:
        raise ValueError("private DOSBox-X binary hash mismatch")
    inputs = {name: checked(run, name) for name in HASHES}
    planes = tuple(
        inputs[f"a000-plane{i}-stable-first-view.bin"].read_bytes()
        for i in range(4)
    )
    plane_pixels = ega_indices(planes)
    shown, colors = screen_indices(inputs["post-load-private.png"])
    paused, paused_colors = screen_indices(inputs["stable-paused-private.png"])
    if shown != paused or colors != paused_colors:
        raise ValueError("same-run screenshots differ across debugger pause")
    mismatches = sum(a != b for a, b in zip(plane_pixels, shown))
    if mismatches:
        raise ValueError(f"native EGA/screenshot index mismatches: {mismatches}")
    compared = display_state(inputs["ram-stable-first-view.bin"].read_bytes())
    print(f"same_run_pixels={len(shown)} index_mismatches={mismatches} "
          f"exact_2x=yes observed_rgb_colors={colors} "
          f"display_state_bytes={compared} display_state_mismatches=0")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path, help="ignored oracle run directory")
    audit(parser.parse_args().run)


if __name__ == "__main__":
    main()
