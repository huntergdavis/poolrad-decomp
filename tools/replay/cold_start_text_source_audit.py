#!/usr/bin/env python3
"""Audit cold-start text sources against private native PNGs; emit counts only.

The decoded font argument is block ID 201 of 8X8D1.DAX, obtained with the
existing C DAX reader. All input and derived image bytes stay private.
"""

import argparse
import hashlib
from pathlib import Path

from PIL import Image


OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
LOADING_SHA = "be0ff6f19aa525049a12a3794c3fe4475ce7549030ed6d5e8f7d7cb41635100f"
CODEWORD_SHA = "a9dc46ae3435d144cb26bb6afdd47e729de7719ce325bcc64e56fedca428aed5"
LOADING_STRING_SHA = "3268e5ee46c67c0bb66ab90e8f2c5b34da5aa23c6ab52d5b19fb80ceba9071af"
CODEWORD_BASE = 0x09AC
FIXED_ROWS = (
    (0x01, 37, 1, 1),
    (0x27, 14, 1, 2),
    (0x36, 35, 1, 5),
    (0x5A, 33, 1, 8),
    (0x7C, 38, 1, 11),
    (0xA9, 36, 1, 14),
)
BLACK = (0, 0, 0)
GREEN = (85, 255, 85)  # standard EGA color index 10


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def native_rgb(path, expected_sha):
    with Image.open(path) as source:
        require(source.size == (640, 400), "native screen dimensions differ")
        image = source.convert("RGB")
    require(sha(image.tobytes()) == expected_sha, "native RGB digest differs")
    for y in range(400):
        for x in range(0, 640, 2):
            require(image.getpixel((x, y)) == image.getpixel((x + 1, y)),
                    "native horizontal doubling differs")
    for y in range(0, 400, 2):
        for x in range(640):
            require(image.getpixel((x, y)) == image.getpixel((x, y + 1)),
                    "native vertical doubling differs")
    return image


def glyph_row(font, ch, row):
    # RES_0709:01DF with its upcase flag set: UpCase(ch) mod 64.
    if ord("a") <= ch <= ord("z"):
        ch -= 32
    return font[(ch % 64) * 8 + row]


def source_pixel(font, ch, glyph_y, glyph_x):
    return GREEN if glyph_row(font, ch, glyph_y) & (0x80 >> glyph_x) else BLACK


def loading_contract(ovr, font, native):
    source = ovr[0x0E5D3:0x0E5E9]
    require(len(source) == 22 and source[0] == 21 and
            sha(source) == LOADING_STRING_SHA, "unit 0x36 string differs")
    rgb = bytearray(640 * 400 * 3)
    set_source_pixels = 0
    for cell, ch in enumerate(source[1:]):
        for gy in range(8):
            bits = glyph_row(font, ch, gy)
            for gx in range(8):
                if not bits & (0x80 >> gx):
                    continue
                set_source_pixels += 1
                x = (cell * 8 + gx) * 2
                y = (24 * 8 + gy) * 2
                for dy in (0, 1):
                    for dx in (0, 1):
                        at = ((y + dy) * 640 + x + dx) * 3
                        rgb[at:at + 3] = bytes(GREEN)
    expected = native.tobytes()
    differing_bytes = sum(a != b for a, b in zip(rgb, expected))
    require(sha(rgb) == LOADING_SHA and differing_bytes == 0,
            "loading source and native frame differ")
    require(set_source_pixels == 479, "loading foreground count differs")
    return len(rgb), set_source_pixels, differing_bytes


def codeword_fixed_contract(ovr, font, native):
    total = 0
    differing = 0
    for offset, length, column, row in FIXED_ROWS:
        start = CODEWORD_BASE + offset
        require(ovr[start] == length and column + length <= 40,
                "unit 0x19 fixed string boundary differs")
        chars = ovr[start + 1:start + 1 + length]
        require(len(chars) == length, "unit 0x19 fixed string is truncated")
        for cell, ch in enumerate(chars):
            for gy in range(8):
                for gx in range(8):
                    x = ((column + cell) * 8 + gx) * 2
                    y = (row * 8 + gy) * 2
                    differing += native.getpixel((x, y)) != source_pixel(font, ch, gy, gx)
                    total += 1
    require(total == 12352 and differing == 0,
            "codeword fixed text cells differ from native")
    return total, differing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("ovr", type=Path, help="ignored DOS GAME.OVR")
    parser.add_argument("font201", type=Path, help="ignored decoded 8X8D1.DAX block 201")
    parser.add_argument("loading_png", type=Path, help="ignored saved native loading frame")
    parser.add_argument("codeword_png", type=Path, help="ignored saved native codeword frame")
    args = parser.parse_args()
    ovr = args.ovr.read_bytes()
    font = args.font201.read_bytes()
    require(sha(ovr) == OVR_SHA and len(ovr) == 206105, "GAME.OVR differs")
    require(sha(font) == FONT_SHA and len(font) == 1416, "font block differs")
    loading = native_rgb(args.loading_png, LOADING_SHA)
    codeword = native_rgb(args.codeword_png, CODEWORD_SHA)
    nbytes, foreground, bad = loading_contract(ovr, font, loading)
    tested, mismatches = codeword_fixed_contract(ovr, font, codeword)
    print(f"loading RGB: {nbytes} bytes, {foreground} set source pixels, {bad} differing bytes, SHA-256 {LOADING_SHA}")
    print(f"codeword fixed text: {tested} source pixels, {mismatches} mismatches, SHA-256 {CODEWORD_SHA} native frame")


if __name__ == "__main__":
    main()
