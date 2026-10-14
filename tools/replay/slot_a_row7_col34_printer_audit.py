#!/usr/bin/env python3
"""Audit the next saved slot-A UI printer call after row-seven column 17.

Only addresses, hashes, and counts are printed. Guest code, RAM, source,
font, EGA planes, and probe records stay under ignored resources/.
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
SOURCE_SHA = "7f774bb46e7e342a2d9d0514b27cee622012f7415645345054f8cd31e1dda661"
BEFORE_SHA = "bd641160cb016bb6ff8ec5043235c972ae829e910a589afc3618ea7b7ab34f6f"
AFTER_SHA = "1dccb5ff277e6a49bbeb8392f4faa000ab7f71357caf87e9d7cc852b8eef2574"
FINAL_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

ORIGINAL_SPANS = {
    "RES_00BA:082B..0878 caller": (0x13CB, 0x1418,
        "c0ea76675dc4a2d237422f1aeb7174f95e9e4d8af00f3e9ab6179f1b38fa3349"),
    "RES_00BA:0875 near call": (0x1415, 0x1418,
        "c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68"),
    "RES_00BA:08EF body": (0x148F, 0x1513,
        "c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d"),
    "RES_00BA:0968 far call": (0x1508, 0x150D,
        "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    "RES_0709:0626 body": (0x76B6, 0x7737, PRINTER_IMAGE_SHA),
}

RUNTIME_SPANS = {
    "08DE:082B..0878 caller": (0x08DE, 0x082B, 0x0878,
        "c0ea76675dc4a2d237422f1aeb7174f95e9e4d8af00f3e9ab6179f1b38fa3349"),
    "08DE:0875 near call": (0x08DE, 0x0875, 0x0878,
        "c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68"),
    "08DE:08EF body": (0x08DE, 0x08EF, 0x0973,
        "75134177259836ec8b3357d2735e0063d38d50a6045ab145e6f432719ecbccbd"),
    "08DE:0968 far call": (0x08DE, 0x0968, 0x096D,
        "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    "0F2D:0626 body": (0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
}

RAM_SHA = {
    "ram-row7-082b-private.bin": "f15f53221cbf425af5a0a412884a2efb7b3882b55dcb0188d100ea19993d331c",
    "ram-row7-0875-private.bin": "ce66824d002eb2263789af47c2f1e33067f02e2480f34d6954fdf4c6ab0244c4",
    "ram-row7-0968-private.bin": "fbde8b3fc8adf1fcba4c1d5052334d01c1cea3e7f687314ab93f27e6bf51969e",
    "ram-row7-next-printer-entry-private.bin": "20cc290927d76dbfbe853258ecce7b03d9d266ec8503346039255f2672cc85e8",
    "ram-row7-next-printer-return-private.bin": "bef034b3336d603e68e8235489a08b277664ade51574dafe9138e6f0894006e5",
}

BEFORE_PLANES = (
    "6a33ada38659de42223137dc19137d392e8be25f3e42bfc6e96a141609559481",
    "d2ee66324a82c53f43ba526c679dc3af46ec8cb5c834d74b43b954b40bfe2501",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "ea05a4be8e638b33d27b9653e982b5d718b931eca3df9ceceaad655e4b32593f",
)
AFTER_PLANES = (
    BEFORE_PLANES[0],
    "03e1e4ba047085cfa9ef60a7553840a40b70c45190306b14e186805c646e1555",
    BEFORE_PLANES[2],
    "3839e3ac059707cbe57c2228dec73e9a297cef997e783510d1b53f0f6fdcf873",
)
FINAL_PLANES = (
    "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
    "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
    BEFORE_PLANES[2],
    "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
)


def page(run: Path, phase: str, hashes: tuple[str, ...], expected: str) -> bytes:
    planes = []
    for lane, digest in enumerate(hashes):
        plane = (run / f"{phase}-plane{lane}.bin").read_bytes()
        if len(plane) != 3520:
            raise ValueError(f"{phase} EGA plane size changed")
        check_hash(plane, digest, f"{phase} EGA plane {lane}")
        planes.append(plane)
    result = b"".join(planes)
    check_hash(result, expected, f"{phase} whole page")
    return result


def audit(exe_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA, "slot-A save")
    image, *_ = unpack(exe)
    for name, (start, end, digest) in ORIGINAL_SPANS.items():
        check_hash(image[start:end], digest, f"original {name}")
    near = image[0x1415:0x1418]
    if near[0] != 0xE8 or 0x0878 + struct.unpack_from("<h", near, 1)[0] != 0x08EF:
        raise ValueError("original 0875 callee changed")
    far = image[0x1508:0x150D]
    if far[0] != 0x9A or struct.unpack_from("<HH", far, 1) != (0x0626, 0x0709):
        raise ValueError("original 0968 callee changed")
    if image[0x1510:0x1513] != b"\xCA\x08\x00":
        raise ValueError("08EF return bound changed")

    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    caller = ram["ram-row7-0875-private.bin"]
    pre = ram["ram-row7-0968-private.bin"]
    printer = ram["ram-row7-next-printer-entry-private.bin"]
    for name, (seg, start, end, digest) in RUNTIME_SPANS.items():
        check_hash(printer[seg * 16 + start:seg * 16 + end], digest,
                   f"loaded {name}")
    loaded_far = printer[0x08DE0 + 0x0968:0x08DE0 + 0x096D]
    if loaded_far[0] != 0x9A or struct.unpack_from("<HH", loaded_far, 1) != (
            0x0626, 0x0F2D):
        raise ValueError("loaded 0968 callee changed")

    ss = 0x1D160
    if (caller[ss + 0x3EBC:ss + 0x3EBE] != bytes((7, 17)) or
            struct.unpack_from("<2H", caller, ss + 0x3EBE) != (8, 0x44F5)):
        raise ValueError("0875 loop frame changed")
    node_at = 0x44F50 + 8
    if caller[node_at + 0x111] != 0x3A:
        raise ValueError("row-seven branch input changed")
    field_frame = struct.unpack_from("<7H", pre, ss + 0x3EA6)
    if field_frame != (0x3EC2, 0x0878, 0x08DE, 7, 0x22, 8, 0x44F5):
        raise ValueError("08EF field-display frame changed")
    ret_ip, ret_cs, source_off, source_seg, color, row, col = struct.unpack_from(
        "<7H", printer, ss + 0x3C96)
    if (ret_ip, ret_cs, source_off, source_seg, color, row, col) != (
            0x096D, 0x08DE, 0x3DA6, 0x1D16, 10, 7, 34):
        raise ValueError("printer entry frame changed")
    source_at = source_seg * 16 + source_off
    length = printer[source_at]
    if length != 1:
        raise ValueError("Pascal source length changed")
    check_hash(printer[source_at:source_at + length + 1], SOURCE_SHA,
               "private Pascal source")
    ds = 0x14A00
    font_at = word(printer, ds + 0x5E20) + 16 * word(printer, ds + 0x5E22)
    font = printer[font_at:font_at + 1416]
    if len(font) != 1416:
        raise ValueError("private font size changed")
    check_hash(font, FONT_SHA, "live font")
    if (word(printer, ds + 0x4A16), word(printer, ds + 0x4A18),
            printer[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
        raise ValueError("EGA page segment or mode changed")

    baseline = page(run, "page-row7-082b", BEFORE_PLANES, BEFORE_SHA)
    caller_page = page(run, "page-row7-0875", BEFORE_PLANES, BEFORE_SHA)
    before = page(run, "page-row7-0968", BEFORE_PLANES, BEFORE_SHA)
    after = page(run, "page-row7-096d", AFTER_PLANES, AFTER_SHA)
    parent = page(run, "page-row7-0878", AFTER_PLANES, AFTER_SHA)
    final = page(run, "page-first-view", FINAL_PLANES, FINAL_SHA)
    if baseline != caller_page or baseline != before or after != parent:
        raise ValueError("caller page phase changed")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {
        lane * 3520 + (7 * 8 + scanline) * 40 + 34 - 0x3C0
        for lane in (1, 3) for scanline in range(7)
    }
    if changed != expected or len(changed) != 14:
        raise ValueError("one-cell immediate delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 399 or changed & later or len(total) != 413 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "next-audit-probe-input-private.bin"
    private_output = run / "next-audit-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              printer[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private C probe input size changed")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=1 stores=32 changed=14":
        raise ValueError("C probe report changed")
    if private_output.read_bytes() != after:
        raise ValueError("C probe differs from native immediate page")

    print("RES_00BA:0875 -> 08EF -> 0968 -> RES_0709:0626 -> 096D -> 0878")
    print("row 7 column 34; one cell; color 10; 32 stores; 14 changed page bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FINAL_SHA)
    print("14/14 changed bytes persist; 399 later net bytes remain")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
