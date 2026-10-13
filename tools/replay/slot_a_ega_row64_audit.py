#!/usr/bin/env python3
"""Audit one complete saved slot-A EGA row against a private DOS view."""

import argparse
from hashlib import sha256
from pathlib import Path
import subprocess

from PIL import Image
from slot_a_first_ega_indexed_byte_audit import (
    EGA_RGB, FLUSH_SHA, OVR_SHA, PRIVATE, START_SHA, check, pinned, word,
)

RUN_HASHES = {
    "ram-flush-entry.bin": "129b45a994d3a1a9263b572b5b22e379022f61d1d82deec535bbbf8120261aff",
    "ram-view-return.bin": "4027d48d95a8910ec631859fb2b445b7f939d55d0b312d334246e74e2117c20a",
    "view-return-private.png": "6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e",
}
ROW_HASHES = {
    "pre": (
        "2c34ce1df23b838c5abf2a7f6437cca3d3067ed509ff25f11df6b11b582b51eb",
        "2c34ce1df23b838c5abf2a7f6437cca3d3067ed509ff25f11df6b11b582b51eb",
        "d86fbf3f5491f417912ed0b27bff4e7abb3147ccf6bfc44083f4155dca318ed3",
        "1a944a711bd2364040f83c60228a59f1341269d7c071f3e48e00e9ef64950836",
    ),
    "post": (
        "c931e183e5ae2064c1d20ad14e7b0e3bc181075d2a780e6c47cc31c3ac855563",
        "8fcbdf290d674ef92d4edb6fd3e10f47c6c2a9026921d6f81881f34950c88d30",
        "e730d203e4328142a9dd837381893d6649bb6846fab568a6124f0a399efe450c",
        "ac8dd0dd945b65adea7bb0a314d9503133da608d42a8f8d13fbc3aac391718d1",
    ),
}


def c_row(probe: Path, paths: list[Path]) -> bytes:
    result = subprocess.run([str(probe), *(str(path) for path in paths)],
                            capture_output=True, check=True)
    check(len(result.stdout) == 320 and not result.stderr, "C row probe output")
    check(all(index < 16 for index in result.stdout), "C row index range")
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
                for name, digest in RUN_HASHES.items()}
    before, after = captured["ram-flush-entry.bin"], captured["ram-view-return.bin"]
    check(len(before) == len(after) == 0xA0000, "RAM image size")
    ss, ds, object_row = 0x1D160, 0x14A00, 56
    check((word(before, ss + 0x3E94), word(before, ss + 0x3E96),
           word(before, ss + 0x3E9A), word(before, ss + 0x3E9C)) ==
          (0x05FB, 0x2D14, 0x0DAE, 0x2D14), "post-wall caller frame")
    check(before[ds + 0x4A1E] == after[ds + 0x4A1E] == 1,
          "EGA video-state gate")
    check((before[ds + 0x4A1A], before[ds + 0x4A1B],
           word(before, ds + 0x4A16), word(before, ds + 0x4A18)) ==
          (0, 0, 0xA000, 0xA000), "active page 0")
    check(before[ds + 0x4A28 + object_row] == 1 and
          after[ds + 0x4A28 + object_row] == 0, "dirty row transition")
    first = word(before, ds + 0x4C20 + 2 * object_row)
    last = word(before, ds + 0x4D70 + 2 * object_row)
    low = word(before, ds + 0x4AD0 + 2 * object_row)
    check((first, last, low) == (4712, 4755, 2), "row-56 source extent")
    row_base = word(before, ds + 0x5050 + 2 * (object_row + 8))
    check(row_base == (object_row + 8) * 40 == 0x0A00,
          "original mode-0D scanline mapping")
    changed_offset = row_base + low + 1 + (4734 - first) // 4
    transfer_base = word(before, ds + 0x5060) + 1 + object_row * 40 + low
    transfer_length = (last - first) // 4 + 1
    check(changed_offset == 0x0A08 and transfer_base == 0x0A03 and
          transfer_length == 11 and
          transfer_base <= changed_offset < transfer_base + transfer_length,
          "first changed byte within display transfer")

    rows = {}
    for phase in ("pre", "post"):
        paths = []
        for plane, digest in enumerate(ROW_HASHES[phase]):
            path = run / f"vram-a000-row64-plane{plane}-{phase}.bin"
            check(len(pinned(path, digest)) == 40, "EGA plane row length")
            paths.append(path)
        rows[phase] = c_row(probe, paths)
    image = Image.open(run / "view-return-private.png").convert("RGB")
    check(image.size == (640, 400), "DOSBox view size")
    palette = {rgb: index for index, rgb in enumerate(EGA_RGB)}
    matched = mismatched = unknown = nonuniform = 0
    y = row_base // 40
    for x in range(320):
        color = image.getpixel((2 * x, 2 * y))
        if any(image.getpixel((2 * x + dx, 2 * y + dy)) != color
               for dx in (0, 1) for dy in (0, 1)):
            nonuniform += 1
            unknown += 1
        elif color not in palette:
            unknown += 1
        elif rows["post"][x] == palette[color]:
            matched += 1
        else:
            mismatched += 1
    changed = sum(a != b for a, b in zip(rows["pre"], rows["post"]))
    check(matched + mismatched + unknown == 320, "row coverage accounting")
    check(mismatched == unknown == 0, "C/DOS row parity or palette coverage")
    print(f"A000:{row_base:04X}..{row_base+39:04X} y={y} x=0..319 "
          f"matched={matched} mismatched={mismatched} unknown={unknown} "
          f"nonuniform={nonuniform} changed={changed}")


if __name__ == "__main__":
    main()
