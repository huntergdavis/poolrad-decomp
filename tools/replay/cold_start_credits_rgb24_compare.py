#!/usr/bin/env python3
"""Compare a private credits RGB24 render with the pinned native frame.

Only hashes and difference counts are printed. Inputs stay under ignored
resources/ directories. This command does not write an image or diff.
"""

import argparse
import hashlib
from pathlib import Path
import sys

from cold_start_title_rgb24_compare import (
    HEIGHT, WIDTH, candidate_rgb24, compare_rgb24, png_rgb24, private_input,
)


NATIVE_RGB_SHA256 = "624f69b5f7bce90ba859eee24aee2e4ac97daf66c405b145f20534123eb28698"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", required=True, type=Path,
                        help="private native stable-frames/0443-0595.png")
    parser.add_argument("--candidate", required=True, type=Path,
                        help="private rendered 640x400 PNG or raw RGB24 frame")
    parser.add_argument("--candidate-format", required=True, choices=("png", "rgb24"))
    args = parser.parse_args()
    try:
        reference = png_rgb24(private_input(args.reference))
        reference_hash = hashlib.sha256(reference).hexdigest()
        if reference_hash != NATIVE_RGB_SHA256:
            raise ValueError("reference RGB24 SHA-256 does not match native credits slot")
        candidate = candidate_rgb24(private_input(args.candidate),
                                    args.candidate_format)
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
