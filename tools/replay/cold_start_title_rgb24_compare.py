#!/usr/bin/env python3
"""Compare a private rendered title frame with the pinned native RGB24 frame.

Only hashes and difference counts are printed. Keep both inputs under ignored
resources/ directories; this program does not write an image or a diff file.
"""

import argparse
from dataclasses import dataclass
import hashlib
from pathlib import Path
import sys

from PIL import Image


WIDTH = 640
HEIGHT = 400
RGB_BYTES = WIDTH * HEIGHT * 3
NATIVE_RGB_SHA256 = "3714eba3761b6d8520c61b61bc60b844dc63a375fa9c2bad166dee509a15e2d8"


@dataclass(frozen=True)
class Difference:
    channel_bytes: int
    pixels: int
    bbox: tuple[int, int, int, int] | None  # x0, y0, x1, y1; end exclusive


def private_input(path: Path) -> Path:
    resolved = path.resolve(strict=True)
    if not resolved.is_file() or "resources" not in resolved.parts:
        raise ValueError("input must be a file in an ignored resources/ directory")
    return resolved


def png_rgb24(path: Path) -> bytes:
    with Image.open(path) as image:
        if image.format != "PNG" or image.size != (WIDTH, HEIGHT) or image.mode != "RGB":
            raise ValueError("PNG must be 640x400 in RGB mode")
        return image.tobytes()


def candidate_rgb24(path: Path, candidate_format: str) -> bytes:
    if candidate_format == "png":
        result = png_rgb24(path)
    elif candidate_format == "rgb24":
        result = path.read_bytes()
    else:
        raise ValueError("candidate format must be png or rgb24")
    if len(result) != RGB_BYTES:
        raise ValueError(f"candidate must contain exactly {RGB_BYTES} RGB bytes")
    return result


def compare_rgb24(reference: bytes, candidate: bytes) -> Difference:
    if len(reference) != RGB_BYTES or len(candidate) != RGB_BYTES:
        raise ValueError(f"both frames must contain exactly {RGB_BYTES} RGB bytes")
    changed_bytes = changed_pixels = 0
    min_x, min_y = WIDTH, HEIGHT
    max_x = max_y = -1
    for index in range(0, RGB_BYTES, 3):
        changes = sum(a != b for a, b in zip(reference[index:index + 3],
                                            candidate[index:index + 3]))
        if changes:
            changed_bytes += changes
            changed_pixels += 1
            pixel = index // 3
            x, y = pixel % WIDTH, pixel // WIDTH
            min_x, min_y = min(min_x, x), min(min_y, y)
            max_x, max_y = max(max_x, x), max(max_y, y)
    bbox = None if changed_pixels == 0 else (min_x, min_y, max_x + 1, max_y + 1)
    return Difference(changed_bytes, changed_pixels, bbox)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", required=True, type=Path,
                        help="private native stable-frames/0359-0433.png")
    parser.add_argument("--candidate", required=True, type=Path,
                        help="private rendered 640x400 PNG or raw RGB24 frame")
    parser.add_argument("--candidate-format", required=True, choices=("png", "rgb24"))
    args = parser.parse_args()
    try:
        reference = png_rgb24(private_input(args.reference))
        reference_hash = hashlib.sha256(reference).hexdigest()
        if reference_hash != NATIVE_RGB_SHA256:
            raise ValueError("reference RGB24 SHA-256 does not match native title slot")
        candidate = candidate_rgb24(private_input(args.candidate), args.candidate_format)
        candidate_hash = hashlib.sha256(candidate).hexdigest()
        difference = compare_rgb24(reference, candidate)
    except (OSError, ValueError) as error:
        parser.exit(2, f"error: {error}\n")
    bbox = "none" if difference.bbox is None else ",".join(map(str, difference.bbox))
    print(f"reference_rgb_sha256={reference_hash} candidate_rgb_sha256={candidate_hash} "
          f"width={WIDTH} height={HEIGHT} channel_bytes_different={difference.channel_bytes} "
          f"pixels_different={difference.pixels} bbox_xyxy_exclusive={bbox}")
    return 0 if difference.pixels == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
