#!/usr/bin/env python3
"""Bound a future U19 row-14 EGA plane watch with ignored original captures.

This reads original code, captured RAM, and host RGB. It does not read EGA
plane state or attribute a value-changing guest store.
"""

import argparse
from pathlib import Path
import struct

from PIL import Image

from cold_start_codeword_effect_source_audit import (
    DS, FONT_SHA, EGA, U19_BASE, glyph_pixel, upcase_glyph,
)
from cold_start_codeword_row14_boundary_audit import (
    audit_captures, audit_original, digest, far_target, require,
)
from slot_a_printer_page_audit import unpack


U19_ORIGINAL_SHA = "518c6f63fa4c0ed6ff7dcf6b986fb4748ecf3e51a366f9005685d7f7e630a00c"
U19_LOADED_SHA = "c5b6d514596572705787fe93b84b1ae74600ce8a59ce0ecf9fd6d589071c5a06"
DRAWER_ORIGINAL_SHA = "3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4"
U19_LIVE_SEG = 0x38D3
DRAWER_LIVE_SEG = 0x0F2D
CALLS = (
    (0x02D8, 0x0626, 0x0709),
    (0x02DD, 0x0104, 0x0899),
    (0x02F2, 0x0B0B, 0x0AF8),
    (0x02FD, 0x09E3, 0x0709),
)
DRAWER_STORES = (0x0412, 0x0438, 0x0445, 0x0451)


def font_from_ram(ram):
    font_off, font_seg = struct.unpack_from("<HH", ram, (DS << 4) + 0x5E20)
    font_at = (font_seg << 4) + font_off
    font = ram[font_at:font_at + 1416]
    require(len(font) == 1416 and digest(font) == FONT_SHA,
            "captured font differs")
    return font


def host_block(rgb, x, y):
    return tuple(rgb.getpixel((2 * x + dx, 2 * y + dy))
                 for dy in (0, 1) for dx in (0, 1))


def first_row14_mismatch(ovr, font, before, after):
    source_at = U19_BASE + 0x00A9
    require(ovr[source_at] == 36, "fixed row-14 source length differs")
    source = ovr[source_at + 1:source_at + 37]
    for y in range(14 * 8, 15 * 8):
        for x in range(8, 37 * 8):
            glyph = upcase_glyph(source[x // 8 - 1])
            expected = glyph_pixel(font, glyph, y % 8, x % 8, 10)
            if any(pixel != expected for pixel in host_block(before, x, y)):
                require(host_block(before, x, y) == (EGA[0],) * 4 and
                        host_block(after, x, y) == (EGA[10],) * 4 and
                        expected == EGA[10],
                        "selected source block color differs")
                return x, y, y * 40 + x // 8, 0x80 >> (x % 8)
    raise ValueError("row-14 source mismatch missing")


def audit(exe, ovr, run):
    audit_original(exe, ovr)
    mismatch, intervals = audit_captures(ovr, run)
    image, *_ = unpack(exe)
    ram = (run / "at-u19-02dd.ram").read_bytes()
    u19_original = ovr[U19_BASE + 0x02D8:U19_BASE + 0x0302]
    u19_loaded = ram[(U19_LIVE_SEG << 4) + 0x02D8:
                     (U19_LIVE_SEG << 4) + 0x0302]
    drawer_original = image[0x7417:0x7501]
    drawer_loaded = ram[(DRAWER_LIVE_SEG << 4) + 0x0387:
                        (DRAWER_LIVE_SEG << 4) + 0x0471]
    require(len(u19_original) == len(u19_loaded) == 42 and
            digest(u19_original) == U19_ORIGINAL_SHA and
            digest(u19_loaded) == U19_LOADED_SHA,
            "U19 original/loaded PC window differs")
    require(len(drawer_original) == len(drawer_loaded) == 234 and
            digest(drawer_original) == DRAWER_ORIGINAL_SHA and
            digest(drawer_loaded) == DRAWER_ORIGINAL_SHA and
            drawer_original == drawer_loaded,
            "drawer original/loaded EGA span differs")
    for site, offset, original_segment in CALLS:
        require(far_target(ovr, U19_BASE + site) ==
                (offset, original_segment),
                f"U19 +{site:04X} original far call differs")
        require(far_target(ram, (U19_LIVE_SEG << 4) + site) ==
                (offset, original_segment + 0x0824),
                f"U19 +{site:04X} loaded far call differs")
    require(tuple(0x7090 + site for site in DRAWER_STORES) ==
            (0x74A2, 0x74C8, 0x74D5, 0x74E1),
            "drawer store image mapping differs")
    with Image.open(run / "at-u19-02dd.png") as source:
        before = source.convert("RGB")
    with Image.open(run / "at-u19-0302.png") as source:
        after = source.convert("RGB")
    witness = first_row14_mismatch(ovr, font_from_ram(ram), before, after)
    require(witness == (122, 112, 0x118F, 0x20),
            "row-14 four-plane watch witness differs")
    require(mismatch == {"02dd": (477, 19), "02e2": (477, 19),
                         "02fd": (477, 19), "0302": (0, 0)} and
            [delta[2] for delta in intervals] == [0, 0, 6612],
            "host RGB provenance differs")
    return witness


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    witness = audit(args.exe.read_bytes(), args.ovr.read_bytes(), args.run)
    print("U19 original file +02D8..+0302 and loaded 38D3:+02D8..+0302 hash-locked")
    print("drawer original image 0x7417..0x7501 and loaded 0F2D:+0387..+0471 identical")
    print("four drawer candidate PCs: 0F2D:0412/0438/0445/0451")
    print(f"row-14 source/host witness: x={witness[0]} y={witness[1]} "
          f"A000:{witness[2]:04X} mask={witness[3]:02X}")
    print("host mismatch 477/477/477/0; host changes 0/0/6612")
    print("EGA planes, first changing store, and page selection unmeasured")


if __name__ == "__main__":
    main()
