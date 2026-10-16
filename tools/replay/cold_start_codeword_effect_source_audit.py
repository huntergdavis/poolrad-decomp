#!/usr/bin/env python3
"""Audit U19 codeword box and record sources from ignored DOS captures.

Print only addresses, hashes, and counts. Original code, RAM, font, strings,
and pixels remain in ignored inputs; this script writes no derived media.
"""

import argparse
import hashlib
from pathlib import Path
import struct

from PIL import Image

from slot_a_printer_page_audit import unpack


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = "b4dc553331795b63fb6e8fb6678df59b0a72799f4c386ccdc21ce81b1ec892a3"
RGB_SHA = "5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
RECORD_SHA = "f71d278190a55a274e7c24197bea1c12fc8c1e2981cd92910988e236d9cf84a1"
TABLE_SHA = "684f280b93fa5c3ea9fda9745dc3c5a45e05c6426252be9826659a535190717c"
PREFIX_SHA = "0bdfeb1cc77982d53834973b517f0913830f333b406255761ec7a5f9f3efb299"
FIELD_SHA = "173d2874ff166e11bd32941d15151f92b59598b4c53f8f00fdfa25c2ec85ca24"
RESULT_SHA = "59865d6b1da4499f2d8430aeb20cf71c0898ebe1733e9f4f418cf128318c7990"
U19_BASE = 0x09AC
DS = 0x14A0
SS = 0x1D16
BP = 0x3EF8
EGA = {0: (0, 0, 0), 10: (85, 255, 85), 12: (255, 85, 85)}

START_SPANS = (
    (0x3F10, 0x4047, "fae6cb41423bcd48237e75a08f5eda70642254eac8449373291d57f34120a8a4"),
    (0x4047, 0x41F8, "1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc"),
    (0x41F8, 0x4277, "71dfae69a38bb9aadad319291d7218cd65523ba14162fb45e5e699ca2d6816d1"),
)
OVR_SPANS = (
    (U19_BASE + 0x0119, U19_BASE + 0x013F,
     "42a556be90efe14d010ca2fedb02b7b73756fdca7472ccb3a135a7c89589ef6f"),
    (U19_BASE + 0x0196, U19_BASE + 0x026E,
     "1ffa11c58e23ad6286311b1a2a9b207dd84ab57ebc6589a4b7853ad0716f3a2f"),
    (U19_BASE + 0x028B, U19_BASE + 0x02C0,
     "f6413cdf89d8d57b2151230c05b40230e9a1b73719942b7289e6eaf6686f8cd1"),
    (0x32392, 0x324E2, "2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00"),
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(ok, message):
    if not ok:
        raise ValueError(message)


def far_target(data, at):
    require(data[at] == 0x9A, f"far-call opcode differs at {at:#x}")
    return struct.unpack_from("<HH", data, at + 1)


def audit_box(image, ovr, ram, rgb):
    for start, end, expected in START_SPANS:
        require(digest(image[start:end]) == expected,
                f"START box span {start:#x} differs")
    for start, end, expected in OVR_SPANS:
        require(digest(ovr[start:end]) == expected,
                f"GAME.OVR source span {start:#x} differs")
    require(far_target(ovr, U19_BASE + 0x013A) == (0x02E8, 0x03F1),
            "U19 box call differs")
    require(ovr[U19_BASE] == 0, "U19 box caption is no longer empty")
    pushed = tuple(ovr[U19_BASE + at + 1] for at in
                   (0x0119, 0x011C, 0x011F, 0x0122, 0x0125, 0x0128))
    require(pushed == (1, 1, 38, 22, 0, 0), "U19 box low-byte args differ")

    border_sites = (0x0038, 0x0052, 0x006C, 0x0086,
                    0x00B9, 0x00D2, 0x010C, 0x0124)
    for at in border_sites:
        require(far_target(image, 0x3F10 + at) == (0x002A, 0x0428),
                f"border tile call {at:#x} differs")
    for at in (0x01F1, 0x020C):
        require(far_target(image, 0x3F10 + at) == (0x00B7, 0x0709),
                f"box clear call {at:#x} differs")
    for at, target in ((0x0312, 0x0137), (0x032A, 0x0000)):
        absolute = 0x3F10 + at
        require(image[absolute] == 0xE8 and
                (at + 3 + struct.unpack_from("<h", image, absolute + 1)[0]) & 0xFFFF == target,
                f"outer wrapper near call {at:#x} differs")
    require(ram[(DS << 4) + 0x4A1E] == 1, "captured EGA mode differs")
    require(digest(ram[(DS << 4) + 0x0114:(DS << 4) + 0x0117]) == TABLE_SHA,
            "captured border style table differs")

    # The +0000 loops have four corner calls, two per interior column,
    # and two per interior row. This is a static call-count derivation.
    corners, horizontal, vertical = 4, 2 * 38, 2 * 22
    require(corners + horizontal + vertical == 124,
            "box tile call count differs")
    ring_pixels = 0
    nonblack = 0
    for y in range(24 * 8):
        for x in range(40 * 8):
            if x >= 8 and x < 39 * 8 and y >= 8 and y < 23 * 8:
                continue
            ring_pixels += 1
            nonblack += rgb.getpixel((x * 2, y * 2)) != EGA[0]
    require((ring_pixels, nonblack) == (7936, 3464),
            "observed box-cell ring differs")
    return corners, horizontal, vertical, ring_pixels, nonblack


def glyph_pixel(font, glyph, gy, gx, color):
    bit = font[glyph * 8 + gy] & (0x80 >> gx)
    return EGA[color] if bit else EGA[0]


def cell_mismatches(rgb, font, glyph, column, row, color):
    bad = 0
    for gy in range(8):
        for gx in range(8):
            expected = glyph_pixel(font, glyph, gy, gx, color)
            x, y = (column * 8 + gx) * 2, (row * 8 + gy) * 2
            for dy in (0, 1):
                for dx in (0, 1):
                    bad += rgb.getpixel((x + dx, y + dy)) != expected
    return bad


def upcase_glyph(ch):
    if 97 <= ch <= 122:
        ch -= 32
    return ch % 64


def audit_record(ovr, ram, rgb):
    index = ram[(SS << 4) + BP - 0x2A]
    require(index == 9, "captured selected-record index differs")
    record_at = (DS << 4) + index * 21
    require(digest(ram[record_at:record_at + 21]) == RECORD_SHA,
            "captured selected record differs")
    font_off, font_seg = struct.unpack_from("<HH", ram, (DS << 4) + 0x5E20)
    font_at = (font_seg << 4) + font_off
    font = ram[font_at:font_at + 1416]
    require(len(font) == 1416 and digest(font) == FONT_SHA,
            "captured font differs")

    cells = ((0x01BB, 9, 8, 6), (0x01E5, 10, 9, 6),
             (0x022C, 11, 8, 9), (0x0269, 12, 9, 9))
    drawn = 0
    glyph_bad = 0
    for call, field, column, row in cells:
        require(far_target(ovr, U19_BASE + call) == (0x01DF, 0x0709),
                f"selected glyph call {call:#x} differs")
        value = ram[record_at + field]
        if field == 12 and value == 0xFF:
            continue
        glyph = value + 0x40  # +0x40 is a WORD addition, upcase flag clear.
        require(glyph < 177, "selected glyph exceeds captured font")
        glyph_bad += cell_mismatches(rgb, font, glyph, column, row, 12)
        drawn += 1
    require(drawn == 3 and glyph_bad == 0,
            "selected glyph cells differ")

    prefix_at = U19_BASE + 0x00A3
    prefix_len = ovr[prefix_at]
    prefix = ovr[prefix_at:prefix_at + 1 + prefix_len]
    require(prefix_len == 5 and digest(prefix) == PREFIX_SHA,
            "U19 fixed dynamic prefix differs")
    field_len = ram[record_at + 13]
    field = ram[record_at + 13:record_at + 14 + field_len]
    require(field_len == 9 and digest(field) == FIELD_SHA,
            "selected Pascal field differs")
    result_at = (SS << 4) + BP - 0x12B
    result_len = ram[result_at]
    result = ram[result_at:result_at + 1 + result_len]
    expected = bytes((prefix_len + field_len,)) + prefix[1:] + field[1:]
    require(result_len == 14 and result == expected and digest(result) == RESULT_SHA,
            "dynamic Pascal append result differs")
    require(far_target(ovr, U19_BASE + 0x02B6) == (0x0B98, 0x0AF8) and
            far_target(ovr, U19_BASE + 0x02BB) == (0x0626, 0x0709),
            "dynamic append/printer calls differ")
    string_bad = 0
    for cell, ch in enumerate(result[1:]):
        string_bad += cell_mismatches(rgb, font, upcase_glyph(ch),
                                     1 + cell, 13, 12)
    require(string_bad == 0, "dynamic record string cells differ")

    # Five fixed rows remain exact at the pre-input stop; the sixth has an
    # intervening +02DD call and is retained as an unresolved effect.
    fixed_rows = ((0x01, 37, 1, 1), (0x27, 14, 1, 2),
                  (0x36, 35, 1, 5), (0x5A, 33, 1, 8),
                  (0x7C, 38, 1, 11), (0xA9, 36, 1, 14))
    row_bad = []
    row_cells_bad = []
    for offset, length, column, row in fixed_rows:
        source_at = U19_BASE + offset
        require(ovr[source_at] == length, "fixed Pascal row differs")
        bad = 0
        cells_bad = 0
        for cell, ch in enumerate(ovr[source_at + 1:source_at + 1 + length]):
            mismatch = cell_mismatches(rgb, font, upcase_glyph(ch),
                                       column + cell, row, 10)
            bad += mismatch
            cells_bad += mismatch != 0
        row_bad.append(bad // 4)
        row_cells_bad.append(cells_bad)
    require(row_bad == [0, 0, 0, 0, 0, 477] and
            row_cells_bad == [0, 0, 0, 0, 0, 19],
            "fixed-row comparison at pre-input stop differs")
    return index, drawn, drawn * 64 + result_len * 64, row_bad[-1], row_cells_bad[-1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--ram", type=Path, required=True)
    parser.add_argument("--png", type=Path, required=True)
    args = parser.parse_args()
    exe, ovr, ram = (p.read_bytes() for p in (args.exe, args.ovr, args.ram))
    require(digest(exe) == START_SHA, "START.EXE differs")
    require(digest(ovr) == OVR_SHA, "GAME.OVR differs")
    require(len(ram) == 655360 and digest(ram) == RAM_SHA,
            "U19 pre-input RAM capture differs")
    image, *_ = unpack(exe)
    with Image.open(args.png) as source:
        require(source.size == (640, 400), "native screenshot dimensions differ")
        rgb = source.convert("RGB")
    require(digest(rgb.tobytes()) == RGB_SHA, "native screenshot RGB differs")
    corners, horizontal, vertical, ring, colored = audit_box(image, ovr, ram, rgb)
    index, glyphs, pixels, last_bad, last_cells = audit_record(ovr, ram, rgb)
    print(f"box source: color-zero clears on two pages, {corners}+{horizontal}+{vertical}=124 tile calls")
    print(f"observed box-cell ring: {ring} source pixels, {colored} nonblack; writer attribution open")
    print(f"selected record {index}: {glyphs} glyphs plus 14 string cells, {pixels}/{pixels} source pixels match")
    print(f"fixed rows 1-5 match; row 14: {last_bad} differing source pixels in {last_cells} cells, unresolved")


if __name__ == "__main__":
    main()
