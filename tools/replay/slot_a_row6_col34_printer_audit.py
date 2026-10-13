#!/usr/bin/env python3
"""Audit one saved-slot printer call from ignored native captures.

Only addresses, hashes, and counts are printed. DOS binaries, save, text,
RAM, EGA planes, and probe records remain under ignored resources/.
"""

import argparse
from pathlib import Path
import struct
import subprocess

from slot_a_printer_page_audit import (
    FONT_SHA, PRINTER_IMAGE_SHA, PRINTER_RUNTIME_SHA, START_SHA,
    check_hash, unpack, word,
)

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
CALL_IMAGE_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
CALL_RUNTIME_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
SOURCE_SHA = "44c808fd166dbb8961fb795587e5ee0082f8a2dc781ff06a3f02223dcc06a7da"
RAM_SHA = {
    "baseline": "112205ca2ddd224e34f04fc75f6a97f75fad4720c9cb809cddcba7578e0a019d",
    "next-entry": "98865054fac0c11e35964cab5a34cc6fd3c96996712c374bd48fc3e67d2dd4a4",
    "next-return": "f85c9dce37608f333ef8bbae4c0f1ae65a098d571d27dcee6f771371a25c1097",
}
PAGE_SHA = {
    "baseline": "421bb334ebb286c22f254a05d1d8e872c6a0c321714c248c2e1c16149ddb21d5",
    "next-entry": "421bb334ebb286c22f254a05d1d8e872c6a0c321714c248c2e1c16149ddb21d5",
    "next-return": "29a78a982037738bc707b584563799657f3ab8ba15981552438e9452e8650512",
    "first-view": "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b",
}


def audit(exe_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA, "slot-A save")
    image, *_ = unpack(exe)
    call = image[0x1508:0x150D]
    check_hash(call, CALL_IMAGE_SHA, "original RES_00BA:0968 far call")
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA, "original printer body")
    if (len(call) != 5 or call[0] != 0x9A or
            struct.unpack_from("<HH", call, 1) != (0x0626, 0x0709)):
        raise ValueError("original far call target changed")

    ram = {}
    for phase, expected in RAM_SHA.items():
        data = (run / f"ram-{phase}.bin").read_bytes()
        if len(data) != 655360:
            raise ValueError(f"{phase} RAM size")
        check_hash(data, expected, f"{phase} RAM")
        ram[phase] = data
    pre = ram["next-entry"]
    loaded_call = pre[0x08DE0 + 0x0968:0x08DE0 + 0x096D]
    check_hash(loaded_call, CALL_RUNTIME_SHA, "loaded far call")
    check_hash(pre[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
               PRINTER_RUNTIME_SHA, "loaded printer body")
    if (loaded_call[0] != 0x9A or
            struct.unpack_from("<HH", loaded_call, 1) != (0x0626, 0x0F2D)):
        raise ValueError("loaded far call target changed")

    ss = 0x1D160
    frame = struct.unpack_from("<7H", pre, ss + 0x3C96)
    if frame != (0x096D, 0x08DE, 0x3DA6, 0x1D16, 10, 6, 34):
        raise ValueError("printer entry frame changed")
    _, _, source_off, source_seg, color, row, col = frame
    source_at = source_seg * 16 + source_off
    length = pre[source_at]
    if length != 1:
        raise ValueError("Pascal source length changed")
    check_hash(pre[source_at:source_at + length + 1], SOURCE_SHA,
               "Pascal source")
    ds = 0x14A00
    font_at = word(pre, ds + 0x5E20) + 16 * word(pre, ds + 0x5E22)
    font = pre[font_at:font_at + 1416]
    if len(font) != 1416:
        raise ValueError("font size changed")
    check_hash(font, FONT_SHA, "live font")
    if (word(pre, ds + 0x4A16), word(pre, ds + 0x4A18), pre[ds + 0x4A1E]) != (
            0xA000, 0xA000, 1):
        raise ValueError("page segments or EGA mode changed")

    pages = {}
    for phase, expected in PAGE_SHA.items():
        planes = [(run / f"page-{phase}-plane{lane}.bin").read_bytes()
                  for lane in range(4)]
        if any(len(plane) != 3520 for plane in planes):
            raise ValueError(f"{phase} plane size")
        pages[phase] = b"".join(planes)
        check_hash(pages[phase], expected, f"{phase} page")
    baseline, before, after, final = (pages[name] for name in
                                      ("baseline", "next-entry", "next-return",
                                       "first-view"))
    if baseline != before:
        raise ValueError("page changed before this printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {(lane * 3520 + (6 * 8 + scanline) * 40 + 34 - 0x3C0)
                for lane in (1, 3) for scanline in range(7)}
    if changed != expected:
        raise ValueError("immediate one-cell page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 658 or changed & later or len(total) != 672 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "next-probe-input-private.bin"
    private_output = run / "next-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              pre[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private probe input size")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=1 stores=32 changed=14":
        raise ValueError(f"C report changed: {result.stdout.strip()}")
    if private_output.read_bytes() != after:
        raise ValueError("C model differs from native immediate page")

    print("RES_00BA:0968 -> RES_0709:0626 -> RES_00BA:096D")
    print("one cell, row 6 column 34, color 10")
    print("page unchanged before call; immediate 14 changed plane bytes")
    print("14 persist at first view; 658 later net changes remain unattributed")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
