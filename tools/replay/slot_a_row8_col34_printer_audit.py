#!/usr/bin/env python3
"""Audit one saved slot-A row-eight column-34 printer call.

Media, RAM, font, source, and page planes stay in ignored resources/.
Public output contains only addresses, hashes, and counts.
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
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
SOURCE_SHA = "06f647b157b8557a12979ba04cf5ba222b9747cf5bbe82f24a48e25c0c508042"
BEFORE_SHA = "8723a10b9ac3747fd9edc42003a59c2434d1aa734fab75db17483eebd0b95498"
AFTER_SHA = "f338875f479952bce7d45467ad8aed9d35bae76ef2a6f1dafe0b7a78273c09b9"
FINAL_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

ORIGINAL_SPANS = {
    "RES_00BA:0865..0878 caller context": (0x1405, 0x1418,
        "032f435749f75440b10833d3dc4d575882c2604972ed380b837e3f6ea15cc553"),
    "RES_00BA:0875 near call": (0x1415, 0x1418,
        "c7f558d3483fe565e125e715e980d1ea4b5a5364b42fd7d728ca95d0d3baff68"),
    "RES_00BA:08EF body": (0x148F, 0x1513,
        "c4d1ffe64d255ff80f1f812284a6145df3b56815fdf4a4aadff777798e32cb0d"),
    "RES_00BA:0968 far call": (0x1508, 0x150D,
        "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    "RES_0709:0626 printer body": (0x76B6, 0x7737, PRINTER_IMAGE_SHA),
}

RUNTIME_SPANS = {
    "08DE:0865..0878 caller context": (0x08DE, 0x0865, 0x0878,
        ORIGINAL_SPANS["RES_00BA:0865..0878 caller context"][2]),
    "08DE:0875 near call": (0x08DE, 0x0875, 0x0878,
        ORIGINAL_SPANS["RES_00BA:0875 near call"][2]),
    "08DE:08EF body": (0x08DE, 0x08EF, 0x0973,
        "75134177259836ec8b3357d2735e0063d38d50a6045ab145e6f432719ecbccbd"),
    "08DE:0968 far call": (0x08DE, 0x0968, 0x096D,
        "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    "0F2D:0626 printer body": (0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
}

RAM_SHA = {
    "ram-row8-1858-private.bin":
        "ae3404317ff0e08e2278f19b8299aeebb7919469d9259082353a5050e862976c",
    "ram-next-printer-entry-private.bin":
        "b427775af2de39870e34acf81478a8a4c2f2178ce57301f057723199d997ff86",
    "ram-next-printer-return-private.bin":
        "c43c251baf5b0ca7b25daac05c848e1bcb3678cb49e067d91e7c8951bd327bae",
}

BEFORE_PLANES = (
    "0ec506a07d3401d81dea67faab4f6cf010deaf4bb1e553a65b37af4979a637a3",
    "8f30a8d5aae950801d86fa8fb0fe79d03afda53453cd0f5df7cbf1c9c52cfcee",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "53e7cf6bbc9f0c566a67503ce63806a2ade4c975a8476f829a1fcf44c7d9e8b8",
)
AFTER_PLANES = (
    BEFORE_PLANES[0],
    "fadd543d9c7fe2c410a817fd2132aff6ebde20bd87a5b95fb69ac90f83fd1250",
    BEFORE_PLANES[2],
    "06a52a1acc2994dcc66855740421d33dd2705b7f399aa0e462f5c734ba282cfc",
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
        data = (run / f"page-{phase}-plane{lane}.bin").read_bytes()
        if len(data) != 3520:
            raise ValueError(f"{phase} EGA plane size changed")
        check_hash(data, digest, f"{phase} EGA plane {lane}")
        planes.append(data)
    result = b"".join(planes)
    check_hash(result, expected, f"{phase} whole page")
    return result


def audit(exe_path: Path, emulator_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash(emulator_path.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")
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
        raise ValueError("original 0968 printer target changed")

    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    entry = ram["ram-next-printer-entry-private.bin"]
    for name, (seg, start, end, digest) in RUNTIME_SPANS.items():
        check_hash(entry[seg * 16 + start:seg * 16 + end], digest,
                   f"loaded {name}")
    loaded_far = entry[0x08DE0 + 0x0968:0x08DE0 + 0x096D]
    if loaded_far[0] != 0x9A or struct.unpack_from("<HH", loaded_far, 1) != (
            0x0626, 0x0F2D):
        raise ValueError("loaded printer target changed")
    if struct.unpack_from("<7H", entry, 0x1D160 + 0x3EA6) != (
            0x3EC2, 0x0878, 0x08DE, 8, 34, 8, 0x4513):
        raise ValueError("08EF parent frame changed")
    ret_ip, ret_cs, src_off, src_seg, color, row, col = struct.unpack_from(
        "<7H", entry, 0x1D160 + 0x3C96)
    if (ret_ip, ret_cs, src_off, src_seg, color, row, col) != (
            0x096D, 0x08DE, 0x3DA6, 0x1D16, 10, 8, 34):
        raise ValueError("printer entry frame changed")
    source_at = src_seg * 16 + src_off
    length = entry[source_at]
    if length != 1:
        raise ValueError("private Pascal source length changed")
    check_hash(entry[source_at:source_at + length + 1], SOURCE_SHA,
               "private Pascal source")
    ds = 0x14A00
    font_at = word(entry, ds + 0x5E20) + 16 * word(entry, ds + 0x5E22)
    font = entry[font_at:font_at + 1416]
    if len(font) != 1416:
        raise ValueError("private font size changed")
    check_hash(font, FONT_SHA, "live font")
    if (word(entry, ds + 0x4A16), word(entry, ds + 0x4A18),
            entry[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
        raise ValueError("EGA page segment or mode changed")

    anchor = page(run, "row8-1858", BEFORE_PLANES, BEFORE_SHA)
    before = page(run, "next-entry", BEFORE_PLANES, BEFORE_SHA)
    after = page(run, "next-return", AFTER_PLANES, AFTER_SHA)
    final = page(run, "first-view", FINAL_PLANES, FINAL_SHA)
    if anchor != before:
        raise ValueError("page changed before next printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {
        lane * 3520 + scanline * 40 + 34 - 0x3C0
        for lane in (1, 3) for scanline in range(64, 71)
    }
    if changed != expected or len(changed) != 14:
        raise ValueError("one-cell immediate page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 189 or changed & later or len(total) != 203 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "row8-col34-probe-input-private.bin"
    private_output = run / "row8-col34-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              entry[source_at + 1:source_at + 1 + length] +
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

    print("RES_00BA:0875 -> 08EF -> 0968 -> RES_0709:0626 -> 096D")
    print("row 8 column 34; one cell; color 10; 32 stores; 14 changed bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FINAL_SHA)
    print("14/14 changed bytes persist; 189 later net bytes remain")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
