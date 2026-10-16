#!/usr/bin/env python3
"""Bound the U19 row-14 display change with ignored, paired native captures.

Read original DOS code, RAM and screenshots; print only addresses and counts.
The script writes no media, copied strings, or derived image data.
"""

import argparse
from collections import Counter
import hashlib
from pathlib import Path
import struct

from PIL import Image

from cold_start_codeword_effect_source_audit import (
    DS, FONT_SHA, OVR_SHA, START_SHA, U19_BASE, cell_mismatches,
    upcase_glyph,
)
from slot_a_printer_page_audit import unpack


BODY_0899_SHA = "1a9b687c96c69b4b791be00bd72730f6df05513ce522a18d53041f8846ce4d93"
BODY_0709_SHA = "822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf"
CALL_02DD_SHA = "ca3aa4c3b05c30afded7501c51739379fbd4c291e8886a81c097f97313530356"
CALL_02FD_SHA = "32ca391ccf9a6a85b6399ab782aa4a24a56c5030ffca617a8a31265fee3c8541"
RAM_SHA = {
    "02dd": "84191adc6aab0f6272fe0dc128b1cbff06668a01c9c40fd5fc1202d490b06699",
    "02e2": "081901c405f2d67a3e5fa68a34bfa67a536f25b39c9641acfdcc7b5863d66f18",
    "02fd": "b4dc553331795b63fb6e8fb6678df59b0a72799f4c386ccdc21ce81b1ec892a3",
    "0302": "c35e6bf7920805fe864d5f31aa59b65fa1a321c9e2a6efadc29d1d931f40eeb5",
}
RGB_SHA = {
    "02dd": "5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb",
    "02e2": "5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb",
    "02fd": "5044229ac07b046aaf144113a90e8fec64edcfef47f978d843319991603dabcb",
    "0302": "b01aed35573d243847675d969712a3606336ca28dd9e2d34885e965c8040a683",
}
STOPS = tuple(RAM_SHA)


def require(ok, message):
    if not ok:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def far_target(data, at):
    require(data[at] == 0x9A, f"far-call opcode differs at {at:#x}")
    return struct.unpack_from("<HH", data, at + 1)


def near_target(data, at, segment_base):
    image_at = segment_base + at
    require(data[image_at] == 0xE8, f"near-call opcode differs at {at:#x}")
    return (at + 3 + struct.unpack_from("<h", data, image_at + 1)[0]) & 0xFFFF


def audit_original(exe, ovr):
    require(digest(exe) == START_SHA, "START.EXE differs")
    require(digest(ovr) == OVR_SHA, "GAME.OVR differs")
    image, *_ = unpack(exe)
    require(digest(image[0x8A94:0x8AB2]) == BODY_0899_SHA,
            "RES_0899:0104 body differs")
    require(digest(image[0x7A73:0x7BF6]) == BODY_0709_SHA,
            "RES_0709:09E3 input body differs")
    require(digest(ovr[U19_BASE + 0x02DD:U19_BASE + 0x02E2]) == CALL_02DD_SHA,
            "U19 +02DD call differs")
    require(digest(ovr[U19_BASE + 0x02FD:U19_BASE + 0x0302]) == CALL_02FD_SHA,
            "U19 +02FD call differs")
    require(far_target(ovr, U19_BASE + 0x02DD) == (0x0104, 0x0899),
            "U19 +02DD target differs")
    require(far_target(ovr, U19_BASE + 0x02FD) == (0x09E3, 0x0709),
            "U19 +02FD target differs")
    # The reached 0899 path skips directly from +010A to the +011E epilogue.
    require(image[0x8A9A] == 0xEB and
            0x010C + struct.unpack_from("<b", image, 0x8A9B)[0] == 0x011E,
            "RES_0899:010A jump target differs")
    for at in (0x0A0A, 0x0B0A):
        require(near_target(image, at, 0x7090) == 0x05EB,
                f"RES_0709:{at:04X} local clear target differs")


def display_state(ram):
    ds_at = DS << 4
    return tuple(struct.unpack_from("<H", ram, ds_at + at)[0]
                 for at in (0x4A16, 0x4A18, 0x4A1A, 0x4A1E))


def row14_mismatch(ovr, ram, rgb):
    ds_at = DS << 4
    font_off, font_seg = struct.unpack_from("<HH", ram, ds_at + 0x5E20)
    font_at = (font_seg << 4) + font_off
    font = ram[font_at:font_at + 1416]
    require(len(font) == 1416 and digest(font) == FONT_SHA,
            "captured font differs")
    source_at = U19_BASE + 0x00A9
    require(ovr[source_at] == 36, "U19 fixed row-14 length differs")
    bad_pixels = 0
    bad_cells = 0
    for cell, char in enumerate(ovr[source_at + 1:source_at + 37]):
        mismatch = cell_mismatches(rgb, font, upcase_glyph(char),
                                   1 + cell, 14, 10)
        require(mismatch % 4 == 0, "row-14 host doubling differs")
        bad_pixels += mismatch // 4
        bad_cells += mismatch != 0
    return bad_pixels, bad_cells


def frame_delta(before, after):
    a, b = before.tobytes(), after.tobytes()
    host = sum(a[i:i + 3] != b[i:i + 3]
               for i in range(0, len(a), 3))
    # A source block can contain 2 or 4 changed host pixels at an emulator
    # presentation boundary; do not silently divide the host count by four.
    rows = {}
    for row in range(25):
        distribution = Counter()
        for y in range(row * 8, (row + 1) * 8):
            for x in range(320):
                changed = sum(before.getpixel((x * 2 + dx, y * 2 + dy)) !=
                              after.getpixel((x * 2 + dx, y * 2 + dy))
                              for dy in (0, 1) for dx in (0, 1))
                if changed:
                    distribution[changed] += 1
        if distribution:
            rows[row] = dict(distribution)
    return host, rows


def audit_captures(ovr, run):
    rgb = {}
    mismatch = {}
    states = []
    for stop in STOPS:
        ram = (run / f"at-u19-{stop}.ram").read_bytes()
        require(len(ram) == 655360 and digest(ram) == RAM_SHA[stop],
                f"U19 +{stop} RAM differs")
        with Image.open(run / f"at-u19-{stop}.png") as source:
            require(source.size == (640, 400),
                    f"U19 +{stop} screenshot dimensions differ")
            rgb[stop] = source.convert("RGB")
        require(digest(rgb[stop].tobytes()) == RGB_SHA[stop],
                f"U19 +{stop} canonical RGB differs")
        states.append(display_state(ram))
        mismatch[stop] = row14_mismatch(ovr, ram, rgb[stop])
    require(states == [(0xA000, 0xA000, 0, 1)] * 4,
            "captured display segment/page state differs")
    require(mismatch == {"02dd": (477, 19), "02e2": (477, 19),
                         "02fd": (477, 19), "0302": (0, 0)},
            "row-14 fixed-source comparison differs")
    intervals = [(a, b, *frame_delta(rgb[a], rgb[b]))
                 for a, b in zip(STOPS, STOPS[1:])]
    require(intervals == [
        ("02dd", "02e2", 0, {}),
        ("02e2", "02fd", 0, {}),
        ("02fd", "0302", 6612, {14: {4: 477},
                                 24: {2: 4, 4: 1174}}),
    ], "paired native RGB delta differs")
    return mismatch, intervals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    exe, ovr = args.exe.read_bytes(), args.ovr.read_bytes()
    audit_original(exe, ovr)
    mismatch, intervals = audit_captures(ovr, args.run)
    print("U19 +02DD -> 0899:0104: reached +010A jump to +011E epilogue")
    print("U19 +02FD -> 0709:09E3: input call; local clear calls at +0A0A/+0B0A")
    for before, after, host, rows in intervals:
        print(f"+{before}->+{after}: {host} changed host RGB pixels; "
              f"source-block row distributions {rows}")
    print(f"row14 fixed-source mismatch by stop: {mismatch}; guest writer unresolved")


if __name__ == "__main__":
    main()
