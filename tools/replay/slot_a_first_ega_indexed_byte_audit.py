#!/usr/bin/env python3
"""Check one saved slot-A EGA display byte against a private DOS view."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
PRIVATE = (ROOT / "resources").resolve()
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
CAPTURE_HASHES = {
    "ram-flush-entry.bin": "9cc1961687b4f52bd357cbb2eaf20882772cf35b334cbaf69d3dcdcdde70dff6",
    "ram-view-return.bin": "6a67bd0fd7d3b027cffe96c7f1261eaf52196657a5a526ac7d613e837dcf4708",
    "view-return-private.png": "6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e",
}
ZERO_BYTE_SHA = "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"
FULL_BYTE_SHA = "a8100ae6aa1940d0b663bb31cd466142ebbdbd5187131b92d93818987832eb89"
EGA_RGB = (
    (0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
)


def check(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def pinned(path: Path, expected: str) -> bytes:
    path = path.resolve(strict=True)
    check(path.is_relative_to(PRIVATE), "input outside ignored resources")
    data = path.read_bytes()
    check(sha256(data).hexdigest() == expected, f"hash mismatch: {path.name}")
    return data


def word(data: bytes, offset: int) -> int:
    check(0 <= offset <= len(data) - 2, "word outside RAM")
    return data[offset] | data[offset + 1] << 8


def c_indices(probe: Path, files: list[Path]) -> bytes:
    result = subprocess.run([str(probe), *(str(p) for p in files)],
                            check=True, capture_output=True)
    check(len(result.stdout) == 8 and not result.stderr, "C probe output size")
    return result.stdout


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    run = args.run.resolve(strict=True)
    probe = args.probe.resolve(strict=True)
    check(run.is_relative_to(PRIVATE) and probe.is_relative_to(PRIVATE),
          "run/probe outside ignored resources")
    start = pinned(args.exe, START_SHA)
    pinned(args.ovr, OVR_SHA)
    check(sha256(start[0x61CB:0x64D5]).hexdigest() == FLUSH_SHA,
          "original flush span hash")
    captured = {name: pinned(run / name, digest)
                for name, digest in CAPTURE_HASHES.items()}
    pre, post = captured["ram-flush-entry.bin"], captured["ram-view-return.bin"]
    check(len(pre) == len(post) == 0xA0000, "RAM image size")
    ss, ds, row = 0x1D160, 0x14A00, 56
    check((word(pre, ss + 0x3E94), word(pre, ss + 0x3E96),
           word(pre, ss + 0x3E9A), word(pre, ss + 0x3E9C)) ==
          (0x05FB, 0x2D14, 0x0DAE, 0x2D14), "post-wall flush caller frame")
    check(pre[ds + 0x4A1E] == post[ds + 0x4A1E] == 1,
          "EGA video-state gate")
    check((pre[ds + 0x4A1A], pre[ds + 0x4A1B],
           word(pre, ds + 0x4A16), word(pre, ds + 0x4A18)) ==
          (0, 0, 0xA000, 0xA000), "active and draw page")
    check(pre[ds + 0x4A28 + row] == 1 and post[ds + 0x4A28 + row] == 0,
          "row-56 dirty transition")
    first = word(pre, ds + 0x4C20 + 2 * row)
    last = word(pre, ds + 0x4D70 + 2 * row)
    low = word(pre, ds + 0x4AD0 + 2 * row)
    check((first, last, low) == (4712, 4755, 2), "row-56 extent")
    object_addr = word(pre, ds + 0x4A20) + 16 * word(pre, ds + 0x4A22)
    check(object_addr == 0x3DE48 and
          pre[object_addr + 0x17 + 4734] ==
          post[object_addr + 0x17 + 4734] == 255,
          "first changed object byte")
    byte_offset = word(pre, ds + 0x5050 + 2 * (row + 8)) + low + 1 + (4734 - first) // 4
    transfer_base = word(pre, ds + 0x5060) + 1 + row * 40 + low
    transfer_len = (last - first) // 4 + 1
    check((byte_offset, transfer_base, transfer_len) == (0x0A08, 0x0A03, 11) and
          transfer_base <= byte_offset < transfer_base + transfer_len,
          "A200-to-A000 row-56 address")
    x, y = (byte_offset % 40) * 8, byte_offset // 40
    check((x, y) == (64, 64), "mode-0D EGA byte coordinate")

    sample_files = {}
    for phase in ("pre", "post"):
        sample_files[phase] = []
        for plane in range(4):
            name = f"vram-a000-plane{plane}-{phase}.bin"
            expected = FULL_BYTE_SHA if phase == "post" and plane < 3 else ZERO_BYTE_SHA
            sample = pinned(run / name, expected)
            check(len(sample) == 1, "EGA plane sample size")
            sample_files[phase].append(run / name)
    before = c_indices(probe, sample_files["pre"])
    after = c_indices(probe, sample_files["post"])
    image = Image.open(run / "view-return-private.png").convert("RGB")
    check(image.size == (640, 400), "DOSBox view size")
    palette = {rgb: index for index, rgb in enumerate(EGA_RGB)}
    screen = []
    for column in range(x, x + 8):
        color = image.getpixel((2 * column, 2 * y))
        check(all(image.getpixel((2 * column + dx, 2 * y + dy)) == color
                  for dx in (0, 1) for dy in (0, 1)), "nonuniform 2x EGA pixel")
        check(color in palette, "DOS pixel outside EGA palette")
        screen.append(palette[color])
    mismatches = sum(actual != measured for actual, measured in zip(after, screen))
    changed = sum(a != b for a, b in zip(before, after))
    check(changed == 8 and mismatches == 0, "bounded C/DOS indexed pixel parity")
    print(f"A000:{byte_offset:04X} mode 0D page 0: x={x}..{x+7} y={y}; "
          f"changed={changed}/8 compared=8 mismatches={mismatches} "
          "uncovered=63992")


if __name__ == "__main__":
    main()
