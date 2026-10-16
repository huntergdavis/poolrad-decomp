#!/usr/bin/env python3
"""Audit source page selection and retained U19 row-14 display state.

This reads ignored originals and existing captures. It neither starts DOS nor
attributes a live guest store, page flip, or host presentation event.
"""

import argparse
from pathlib import Path
import struct

from cold_start_codeword_row14_boundary_audit import (
    audit_captures, audit_original, digest, far_target, require,
)
from cold_start_codeword_effect_source_audit import U19_BASE
from slot_a_printer_page_audit import unpack


STARTUP_SHA = "eb428dc50cedede319fd7f0fc936ba592548a3125d07a100e7d08176ec21457b"
ACTIVE_SHA = "26854c57a4d438a0e37a6c00ab8a7a6b9bff9567da09f681ee4247aeccd42b77"
ACTIVE_LOADED_SHA = "62df52ae567e90dc75b8b235ec70a39cae47256482d2af99bd5d4c4394417310"
DRAW_SHA = "6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d"
MODE_SHA = "57cfe9bad55e340af1a58eb1bbfe3d09f43f9b60d9efb2ee3125af38a3c6abf2"
ACTIVE_READ_SHA = "b943c1b3643260df5752433710278f125c28aa88986e05d00adbdc7a84be2183"
DRAW_READ_SHA = "763b0ebbc17fdbde3f3031c431ed1c9b57653b5e3eccdc605d71abaf1cb5edda"
INPUT_SHA = "822b4aeac02e3396c177fbdb18187c6040862ec3595d7ffde16b8eb221565baf"
U19_SUFFIX_SHA = "518c6f63fa4c0ed6ff7dcf6b986fb4748ecf3e51a366f9005685d7f7e630a00c"
PAGE_STATE_SHA = "52c20cdb7fceeb8f44f77d9d25201b13b4e056958430161515115c310de4d9d3"

DRAWER_SEG = 0x0F2D
DS = 0x14A0
PAGE_READS = (
    ("mode", 0x72C8, 0x0238, 7, MODE_SHA),
    ("glyph active", 0x749A, 0x040A, 8, ACTIVE_READ_SHA),
    ("glyph draw", 0x74C0, 0x0430, 8, DRAW_READ_SHA),
    ("zero active", 0x74CD, 0x043D, 8, ACTIVE_READ_SHA),
    ("zero draw", 0x74D9, 0x0449, 8, DRAW_READ_SHA),
)


def exact_direct_page_calls(span):
    """Find exact far targets at any 0x9A byte, without claiming a full CFG."""
    return [(site, far_target(span, site)) for site in range(len(span) - 4)
            if span[site] == 0x9A and
            far_target(span, site) in ((0x0071, 0x0709), (0x00A0, 0x0709))]


def audit(exe, ovr, run):
    audit_original(exe, ovr)
    image, *_ = unpack(exe)
    startup = image[0x4426:0x44B5]
    require(len(startup) == 143 and digest(startup) == STARTUP_SHA,
            "startup selection image differs")
    require(far_target(image, 0x4466) == (0x0071, 0x0709) and
            far_target(image, 0x446F) == (0x00A0, 0x0709),
            "startup page-helper calls differ")

    ram = (run / "at-u19-02dd.ram").read_bytes()
    require(len(ram) == 655360, "retained RAM length differs")
    loaded_base = DRAWER_SEG << 4
    active = image[0x7101:0x7130]
    active_loaded = ram[loaded_base + 0x0071:loaded_base + 0x00A0]
    draw = image[0x7130:0x7147]
    draw_loaded = ram[loaded_base + 0x00A0:loaded_base + 0x00B7]
    require(len(active) == len(active_loaded) == 47 and
            digest(active) == ACTIVE_SHA and
            digest(active_loaded) == ACTIVE_LOADED_SHA,
            "active-page helper identity differs")
    require([i for i, (a, b) in enumerate(zip(active, active_loaded))
             if a != b] == [0x19, 0x1A] and
            far_target(image, 0x7117) == (0x000B, 0x0A32) and
            far_target(ram, loaded_base + 0x0087) == (0x000B, 0x1256),
            "active-page runtime-call relocation differs")
    require(len(draw) == len(draw_loaded) == 23 and
            digest(draw) == DRAW_SHA and draw == draw_loaded,
            "draw-page helper differs")

    for name, file_at, live_at, length, expected in PAGE_READS:
        original = image[file_at:file_at + length]
        loaded = ram[loaded_base + live_at:loaded_base + live_at + length]
        require(len(original) == length and digest(original) == expected and
                original == loaded, f"{name} original/loaded page read differs")

    input_body = image[0x7A73:0x7BF6]
    u19_suffix = ovr[U19_BASE + 0x02D8:U19_BASE + 0x0302]
    require(len(input_body) == 387 and digest(input_body) == INPUT_SHA and
            len(u19_suffix) == 42 and digest(u19_suffix) == U19_SUFFIX_SHA,
            "bounded input or U19 suffix source differs")
    require(not exact_direct_page_calls(input_body) and
            not exact_direct_page_calls(u19_suffix),
            "unexpected direct page-helper encoding in bounded source")

    for stop in ("02dd", "02e2", "02fd", "0302"):
        capture = (run / f"at-u19-{stop}.ram").read_bytes()
        page_state = capture[(DS << 4) + 0x4A16:(DS << 4) + 0x4A1F]
        require(len(capture) == 655360 and
                digest(page_state) == PAGE_STATE_SHA and
                struct.unpack_from("<HHBBBBB", page_state) ==
                (0xA000, 0xA000, 0, 0, 3, 0x0D, 1) and
                struct.unpack_from("<H", capture, (DS << 4) + 0x4A02)[0] ==
                0x0500, f"{stop} page-state endpoint differs")

    mismatch, intervals = audit_captures(ovr, run)
    require(mismatch == {"02dd": (477, 19), "02e2": (477, 19),
                         "02fd": (477, 19), "0302": (0, 0)} and
            [delta[2] for delta in intervals] == [0, 0, 6612],
            "row-14 host capture boundary differs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe.read_bytes(), args.ovr.read_bytes(), args.run)
    print("startup calls RES_0709:0071 active and :00A0 draw; helpers hash-locked")
    print("loaded active helper differs at runtime-call segment only; draw helper identical")
    print("four U19 stops: active/draw A000, indexes 0/0, EGA mode 0D, AX 0500")
    print("bounded U19 suffix/input: no exact direct far call to either page helper")
    print("live page selection, four-plane stores, and host presentation unmeasured")


if __name__ == "__main__":
    main()
