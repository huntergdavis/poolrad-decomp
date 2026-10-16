#!/usr/bin/env python3
"""Join original page-helper returns to the static row-14 witness stores.

This reads ignored originals and retained captures. It does not launch DOS,
observe a page-helper invocation, or identify a value-changing EGA store.
"""

import argparse
from pathlib import Path

from cold_start_codeword_row14_boundary_audit import digest, require
from cold_start_codeword_row14_page_selector_audit import audit as page_audit
from cold_start_codeword_row14_store_candidate_audit import audit as store_audit
from slot_a_printer_page_audit import unpack


IMAGE_BASE = 0x7090
WINDOWS = (
    ("active runtime call", 0x0087, 0x008C,
     "6c44fa171cea0b9d6aa7502458b0d6c591ffa871bf7c75dc27e31b545b4415cf"),
    ("active post-call segment", 0x008C, 0x009C,
     "6a4544c7307fe99e0c26729e2b79c3db2ae6501923901ed98c82a82cf0b3bf1e"),
    ("active return", 0x009C, 0x00A0,
     "99328d20d43f7037d7e84a250f7a942f00f9a85f9c05f034df7214b3fd0264e3"),
    ("draw segment and return", 0x00B0, 0x00B7,
     "5e2fcb9a2f578e16512c2304ba793b55b28e46b6d33385df439b38adbc22b28f"),
    ("glyph active-page store setup", 0x040A, 0x0415,
     "668ad60906de51402939c895a2687eddece913e70b023ea7c7be51d646e9d180"),
    ("glyph draw-page store setup", 0x0430, 0x043B,
     "15ee7df4e841c4218395d9c9c64d6e847bd0c7c9e1625122733e29fe04d0401b"),
    ("zero active-page store setup", 0x043D, 0x0449,
     "2bc2aee2f658b6638259de2bc2be7a5de36efd43f248f77fcc251818fe12389b"),
    ("zero draw-page store setup", 0x0449, 0x0455,
     "da009d58775ce7c015d1be321a188200dfaf054ab0096febfb08fede5c5b80e7"),
)


def audit(exe, ovr, run):
    page_audit(exe, ovr, run)
    witness = store_audit(exe, ovr, run)
    image, *_ = unpack(exe)
    for name, lo, hi, expected in WINDOWS:
        span = image[IMAGE_BASE + lo:IMAGE_BASE + hi]
        require(len(span) == hi - lo and digest(span) == expected,
                f"{name} original image differs")

    x, y, offset, mask = witness
    require((x, y, offset, mask) == (122, 112, 0x118F, 0x20) and
            y * 40 + x // 8 == offset and 0x80 >> (x & 7) == mask,
            "retained row-14 witness address differs")
    return len(WINDOWS), witness


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    count, witness = audit(args.exe.read_bytes(), args.ovr.read_bytes(),
                           args.run)
    print(f"original page-return and drawer setup windows: {count} SHA-256 matches")
    print(f"retained source/host witness A000:{witness[2]:04X} mask {witness[3]:02X}")
    print("page-helper reach, first changed plane, and host presentation unresolved")


if __name__ == "__main__":
    main()
