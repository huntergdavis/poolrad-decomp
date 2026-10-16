#!/usr/bin/env python3
"""Audit the U19 +02DD no-op return and following line-input calls.

This reads ignored DOS code and paired screenshots without starting DOS.
Only addresses and counts are printed; no guest EGA writer is attributed.
"""

import argparse
from pathlib import Path

from cold_start_codeword_row14_boundary_audit import (
    audit_captures, audit_original, digest, far_target, near_target, require,
)
from cold_start_codeword_effect_source_audit import U19_BASE
from slot_a_printer_page_audit import unpack


CALLER_SETUP_SHA = "6aa8dc6978771bd029e8fd923a232ef8cb405176b9571aa503c019b48e49966e"
CLEAR_HELPER_SHA = "a41af7187ac3e0bf5a09311bad9fae220c498f0f9aad9492438661381d4b1171"
ROW14_CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"

# The input routine's direct display effects use row 0x18. The clear helper
# calls the drawer; the printer calls the drawer from its own loop.
SCREEN_CALLS = (
    (0x0A0A, 0x05EB),
    (0x0A1D, 0x0626),
    (0x0A9C, 0x0626),
    (0x0AE6, 0x05EB),
    (0x0B0A, 0x05EB),
)


def audit(exe, ovr, run):
    audit_original(exe, ovr)
    image, *_ = unpack(exe)
    require(digest(ovr[U19_BASE + 0x02E2:U19_BASE + 0x02FD]) == CALLER_SETUP_SHA,
            "U19 +02E2..+02FD setup differs")
    require(far_target(ovr, U19_BASE + 0x02F2) == (0x0B0B, 0x0AF8),
            "U19 +02F2 Pascal copy target differs")
    require(far_target(ovr, U19_BASE + 0x02FD) == (0x09E3, 0x0709),
            "U19 +02FD line-input target differs")
    require(digest(ovr[U19_BASE + 0x02D8:U19_BASE + 0x02DD]) == ROW14_CALL_SHA and
            far_target(ovr, U19_BASE + 0x02D8) == (0x0626, 0x0709),
            "preceding U19 row-14 printer call differs")
    require(digest(image[0x767B:0x76B6]) == CLEAR_HELPER_SHA,
            "RES_0709:05EB clear helper differs")
    require(near_target(image, 0x061F, 0x7090) == 0x01DF,
            "clear-helper drawer call differs")
    for site, target in SCREEN_CALLS:
        require(near_target(image, site, 0x7090) == target,
                f"RES_0709:{site:04X} screen call target differs")
    mismatch, intervals = audit_captures(ovr, run)
    require(mismatch["02dd"] == mismatch["02e2"] ==
            mismatch["02fd"] == (477, 19) and mismatch["0302"] == (0, 0),
            "row-14 capture sequence differs")
    require([(a, b, host) for a, b, host, _ in intervals] == [
        ("02dd", "02e2", 0), ("02e2", "02fd", 0),
        ("02fd", "0302", 6612),
    ], "paired host RGB sequence differs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe.read_bytes(), args.ovr.read_bytes(), args.run)
    print("U19 +02DD -> RES_0899:0104 skip-to-epilogue -> U19 +02E2")
    print("U19 +02F2 Pascal copy; +02FD -> RES_0709:09E3 line input -> +0302")
    print("input direct screen calls: +0A0A/+0AE6/+0B0A clear; +0A1D/+0A9C print; row 24")
    print("first static row-14-capable U19 call in this suffix is the preceding +02D8 printer")
    print("guest first store and host presentation remain unresolved")


if __name__ == "__main__":
    main()
