#!/usr/bin/env python3
"""Audit the next reached slot-A printer call using ignored native captures.

The tool prints only addresses, counts, and checksums. DOS data, text, font,
RAM and EGA planes are read from ignored resources/ and never printed.
"""

import argparse
from pathlib import Path
import struct
import subprocess

from slot_a_printer_page_audit import (
    FONT_SHA, PAGE_SHA, PRINTER_IMAGE_SHA, PRINTER_RUNTIME_SHA, START_SHA,
    check_hash, unpack, word,
)

FIELD_IMAGE_SHA = "c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d"
CALL_IMAGE_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
CALL_RUNTIME_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
SOURCE_SHA = "f16ba6fa61da3398815be2a6c0f7cb1351982dbcc6c64bbeb9b65f672a8b102a"
RAM_SHA = {
    "post-printer": "9f5e61575b69399cdf1f4a5c8d4c1a8e9a370872a3d2f36ae805e9f0a9eea05d",
    "pre-next-printer": "bd456b051f26cad2b77a4760e29528c22cc03e7f98f56716a2b05a0228c32e9e",
    "post-next-printer": "37a1a6a8d113268bb24b3df9d2a21aa547bd7c722f135a6e3b4ac54139595555",
    "first-view": "e0d8642621bdc1df657197485fdb7ddaf153eb8f8eb8e154a99f2cf995096a92",
}
POST_NEXT_PAGE_SHA = (
    "accf1d70b948df184630639a8aa9afe7d87bbe3030fd4d5e8be464a580e58f8e",
    "e9723e209c36c775a8feaa0976fe9f12038b8c7d2579f7f1dbf269f12181655b",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "87b511b0a0f7426086cd5030da5f11e02732ef2bfc7dc4246d96dcc32b9e8f9b",
)


def audit(exe_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START")
    image, *_ = unpack(exe)
    check_hash(image[0x148F:0x1513], FIELD_IMAGE_SHA, "field-display body")
    check_hash(image[0x1508:0x150D], CALL_IMAGE_SHA, "second printer callsite")
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA, "printer body")

    ram = {}
    for phase, expected in RAM_SHA.items():
        data = (run / f"ram-{phase}.bin").read_bytes()
        if len(data) != 655360:
            raise ValueError(f"{phase} RAM length")
        check_hash(data, expected, f"{phase} RAM")
        ram[phase] = data
    pre = ram["pre-next-printer"]
    check_hash(pre[0x08DE0 + 0x968:0x08DE0 + 0x96D],
               CALL_RUNTIME_SHA, "relocated caller")
    check_hash(pre[0x0F2D0 + 0x626:0x0F2D0 + 0x6A7],
               PRINTER_RUNTIME_SHA, "relocated printer")
    if struct.unpack_from("<HH", pre, 0x08DE0 + 0x968 + 1) != (
            0x0626, 0x0F2D):
        raise ValueError("runtime call does not target 0F2D:0626")

    # Stopped at 0F2D:0626. The far return is at SS:SP=1D16:3C96.
    frame = 0x1D160 + 0x3C96
    ret_ip, ret_cs, source_off, source_seg, color, row, column = (
        struct.unpack_from("<7H", pre, frame))
    if (ret_ip, ret_cs, source_off, source_seg, color, row, column) != (
            0x096D, 0x08DE, 0x3DA6, 0x1D16, 10, 4, 34):
        raise ValueError("second printer call frame changed")
    # The RES_00BA:08EF parent returns to the reached 0767 field caller.
    parent = 0x1D160 + 0x3EA6
    if (word(pre, parent + 2), word(pre, parent + 4),
            word(pre, parent + 10), word(pre, parent + 12)) != (
            0x0878, 0x08DE, 0x0008, 0x449A):
        raise ValueError("field-display parent frame changed")
    source_at = 16 * source_seg + source_off
    length = pre[source_at]
    raw = pre[source_at:source_at + 1 + length]
    if length != 1:
        raise ValueError("second printer source length changed")
    check_hash(raw, SOURCE_SHA, "one-cell Pascal source")
    ds = 0x14A00
    font_at = word(pre, ds + 0x5E20) + 16 * word(pre, ds + 0x5E22)
    font = pre[font_at:font_at + 1416]
    check_hash(font, FONT_SHA, "live font")
    if (word(pre, ds + 0x4A16), word(pre, ds + 0x4A18),
            pre[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
        raise ValueError("display page or EGA mode changed")

    page_hashes = {
        "post-printer": PAGE_SHA["post-printer"],
        "pre-next-printer": PAGE_SHA["post-printer"],
        "post-next-printer": POST_NEXT_PAGE_SHA,
        "first-view": PAGE_SHA["first-view"],
    }
    pages = {}
    for phase, expected_hashes in page_hashes.items():
        planes = []
        for lane, expected in enumerate(expected_hashes):
            data = (run / f"page-{phase}-plane{lane}.bin").read_bytes()
            if len(data) != 3520:
                raise ValueError(f"{phase} plane {lane} length")
            check_hash(data, expected, f"{phase} plane {lane}")
            planes.append(data)
        pages[phase] = planes

    def changes(before: str, after: str) -> set[tuple[int, int]]:
        return {(lane, at) for lane in range(4) for at in range(3520)
                if pages[before][lane][at] != pages[after][lane][at]}

    between = changes("post-printer", "pre-next-printer")
    invoked = changes("pre-next-printer", "post-next-printer")
    later = changes("post-next-printer", "first-view")
    total = changes("post-printer", "first-view")
    cell = {(lane, at) for lane in range(4) for at in range(3520)
            if 8 <= at // 40 <= 15 and at % 40 == 34}
    clean = {(lane, at) for lane in range(4) for at in range(3520)
             if at % 40 < 3 or at % 40 > 13}
    if between or len(invoked) != 14 or invoked - cell or invoked - clean:
        raise ValueError("isolated one-cell contribution changed")
    if (len(later) != 1022 or later & cell or later - clean or
            len(total) != 1036):
        raise ValueError("remaining first-view page contribution changed")
    if total != invoked | later or any(
            pages["post-next-printer"][lane][at] !=
            pages["first-view"][lane][at] for lane, at in invoked):
        raise ValueError("one-cell contribution did not persist")

    # The existing C page projection receives private bytes only in resources/.
    private_input = run / "next-printer-private-input.bin"
    private_output = run / "next-printer-private-output.bin"
    record = (font + bytes((length, column, row, color)) + raw[1:] +
              bytes(255 - length) + b"".join(pages["pre-next-printer"]))
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private probe record length")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=1 stores=32 changed=14":
        raise ValueError(f"C report changed: {result.stdout.strip()}")
    native_after = b"".join(pages["post-next-printer"])
    model_after = private_output.read_bytes()
    if model_after != native_after:
        raise ValueError("C model differs from native post-call page")

    print("reached RES_00BA:08EF +0x79 -> RES_0709:0626 -> RES_00BA:096D")
    print("parent return RES_00BA:0878; one cell at row 4, column 34")
    print("second call: 32 plane-byte targets, 14 value changes, 14 persist")
    print("remaining first-view clean changes: 1022; total after first printer: 1036")
    print("C projection: 14080/14080 native post-call page bytes")
    print("all original inputs and captures remain under ignored resources/")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
