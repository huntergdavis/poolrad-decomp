#!/usr/bin/env python3
"""Audit the static U19 row-14 drawer store order from ignored originals.

The retained host frames contain no synchronized EGA planes. This audit
identifies candidate instructions and a witness byte, never a live first
value-changing store.
"""

import argparse
from pathlib import Path
import struct

from cold_start_codeword_effect_source_audit import DS, U19_BASE
from cold_start_codeword_row14_boundary_audit import (
    digest, far_target, near_target, require,
)
from cold_start_codeword_row14_plane_watch_audit import audit as watch_audit
from cold_start_codeword_row14_printer_source_audit import (
    PRINTER_SHA, U19_ROW14_CALL_SHA,
)
from slot_a_printer_page_audit import unpack


DRAWER_IMAGE_BASE = 0x7090
DRAWER_SHA = "3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4"
BRANCH_SHA = "c0cc15948234896bc90404e6ecfb3a4a0340ba76a100fe663d50d7f11845a3b8"
GLYPH_STORE_SHA = "4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e"
ZERO_STORE_SHA = "5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29"
MASK_TABLE_SHA = "1ba1475eadf8ae7a3b170515ef3c887513a6feed761b5f6f889882b809cd553f"
SITES = ((0x0412, 3, GLYPH_STORE_SHA),
         (0x0438, 3, GLYPH_STORE_SHA),
         (0x0445, 4, ZERO_STORE_SHA),
         (0x0451, 4, ZERO_STORE_SHA))


def short_target(image, at, opcode):
    image_at = DRAWER_IMAGE_BASE + at
    require(image[image_at] == opcode,
            f"drawer branch opcode differs at +{at:04X}")
    return (at + 2 + struct.unpack_from("<b", image, image_at + 1)[0]) & 0xFFFF


def near_jump_target(image, at):
    image_at = DRAWER_IMAGE_BASE + at
    require(image[image_at] == 0xE9,
            f"drawer near-jump opcode differs at +{at:04X}")
    return (at + 3 + struct.unpack_from("<h", image, image_at + 1)[0]) & 0xFFFF


def audit(exe, ovr, run):
    witness = watch_audit(exe, ovr, run)
    image, *_ = unpack(exe)
    ram = (run / "at-u19-02dd.ram").read_bytes()
    require(digest(ovr[U19_BASE + 0x02C0:U19_BASE + 0x02E2]) ==
            U19_ROW14_CALL_SHA,
            "U19 fixed row-14 call setup differs")
    require(tuple(ovr[U19_BASE + at + 1] for at in
                  (0x02C0, 0x02C3, 0x02C6)) == (1, 14, 10),
            "U19 row-14 column/row/color differs")
    require(far_target(ovr, U19_BASE + 0x02D8) == (0x0626, 0x0709),
            "U19 row-14 printer target differs")
    require(digest(image[0x76B6:0x7737]) == PRINTER_SHA and
            near_target(image, 0x0691, DRAWER_IMAGE_BASE) == 0x01DF,
            "resident printer/drawer route differs")
    require(digest(image[0x7417:0x7501]) == DRAWER_SHA,
            "original drawer EGA path differs")
    require(digest(image[DRAWER_IMAGE_BASE + 0x03ED:
                         DRAWER_IMAGE_BASE + 0x03EF]) == BRANCH_SHA and
            short_target(image, 0x03ED, 0x76) == 0x043D,
            "color-zero branch differs")
    require(short_target(image, 0x0459, 0x74) == 0x045E and
            near_jump_target(image, 0x045B) == 0x03BD and
            short_target(image, 0x0466, 0x74) == 0x046B and
            near_jump_target(image, 0x0468) == 0x03B4,
            "plane/row loop bounds differ")
    for site, size, expected in SITES:
        require(digest(image[DRAWER_IMAGE_BASE + site:
                             DRAWER_IMAGE_BASE + site + size]) == expected,
                f"drawer store differs at +{site:04X}")
    table = ram[(DS << 4) + 0x25C9:(DS << 4) + 0x25CD]
    require(len(table) == 4 and digest(table) == MASK_TABLE_SHA and
            tuple(table[3 - plane] for plane in range(4)) == (1, 2, 4, 8),
            "captured EGA map-mask table differs")
    require(tuple(bool(10 & table[3 - plane]) for plane in range(4)) ==
            (False, True, False, True),
            "color-10 static plane branch order differs")
    require(witness == (122, 112, 0x118F, 0x20),
            "first source/host witness differs")
    return witness


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe.read_bytes(), args.ovr.read_bytes(), args.run)
    print("static route: U19 +02D8 -> RES_0709:0626 +0691 -> +01DF")
    print("row 14 color 10: plane 0 zero, plane 1 glyph, plane 2 zero, plane 3 glyph")
    print("per-plane first/second-page store PCs: zero +0445/+0451, glyph +0412/+0438")
    print("witness A000:118F bit 20; first actual value-changing plane store unresolved")


if __name__ == "__main__":
    main()
