#!/usr/bin/env python3
"""Hash-lock the static U19 row-14 printer route and one private stop.

This audit does not claim that any candidate EGA store was the first
value-changing guest store. It prints only addresses, hashes, and counts.
"""

import argparse
from pathlib import Path

from PIL import Image

from cold_start_codeword_row14_boundary_audit import (
    RAM_SHA, RGB_SHA, audit_original, digest, display_state, far_target,
    near_target, require, row14_mismatch,
)
from cold_start_codeword_effect_source_audit import U19_BASE
from slot_a_printer_page_audit import unpack


U19_ROW14_CALL_SHA = "144b7073fef51cd92e155eaf08dd5c679772c7788a989432d2bafc95356647ce"
PRINTER_SHA = "b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed"
DRAWER_EGA_SHA = "3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4"
PRINTER_CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"


def audit(exe, ovr, ram, rgb):
    audit_original(exe, ovr)
    image, *_ = unpack(exe)
    require(digest(ovr[U19_BASE + 0x02C0:U19_BASE + 0x02E2]) == U19_ROW14_CALL_SHA,
            "U19 row-14 setup span differs")
    require(digest(ovr[U19_BASE + 0x02D8:U19_BASE + 0x02DD]) == PRINTER_CALL_SHA,
            "U19 +02D8 printer call differs")
    require(far_target(ovr, U19_BASE + 0x02D3) == (0x0B0B, 0x0AF8),
            "U19 row-14 Pascal copy target differs")
    require(far_target(ovr, U19_BASE + 0x02D8) == (0x0626, 0x0709),
            "U19 row-14 printer target differs")
    require(digest(image[0x76B6:0x7737]) == PRINTER_SHA,
            "RES_0709:0626 printer body differs")
    require(near_target(image, 0x0691, 0x7090) == 0x01DF,
            "RES_0709:0691 drawer target differs")
    require(digest(image[0x7417:0x7501]) == DRAWER_EGA_SHA,
            "RES_0709 drawer EGA span differs")
    require(len(ram) == 655360 and digest(ram) == RAM_SHA["02dd"],
            "U19 +02DD RAM differs")
    require(rgb.size == (640, 400) and digest(rgb.tobytes()) == RGB_SHA["02dd"],
            "U19 +02DD canonical RGB differs")
    require(display_state(ram) == (0xA000, 0xA000, 0, 1),
            "U19 +02DD display words differ")
    require(row14_mismatch(ovr, ram, rgb) == (477, 19),
            "U19 +02DD row-14 source mismatch differs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--ram", type=Path, required=True)
    parser.add_argument("--png", type=Path, required=True)
    args = parser.parse_args()
    with Image.open(args.png) as source:
        rgb = source.convert("RGB")
    audit(args.exe.read_bytes(), args.ovr.read_bytes(),
          args.ram.read_bytes(), rgb)
    print("U19 +02D8 -> RES_0709:0626 -> +0691 -> +01DF")
    print("drawer EGA candidate store image sites: 0x74A2, 0x74C8, 0x74D5, 0x74E1")
    print("+02DD screenshot: 477 differing source pixels across 19 row-14 cells")
    print("first value-changing guest EGA store and host presentation unresolved")


if __name__ == "__main__":
    main()
