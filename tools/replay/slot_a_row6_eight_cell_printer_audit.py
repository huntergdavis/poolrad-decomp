#!/usr/bin/env python3
"""Audit one saved-slot row-six printer call from ignored native captures.

Print only addresses, hashes, and counts. DOS media, source, font, RAM, EGA
planes, and probe input/output stay in ignored resources/.
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
PARENT_CALL_SHA = "101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3"
DISPLAY_IMAGE_SHA = "47a999a7d5c19202877fb35003099aab45dac77ce4bb95e93d132020a2373918"
DISPLAY_RUNTIME_SHA = "35cdb1f71e2bc52f5f2d377d647594481f50921a7c09a0aed1b01057aa6087ac"
CALL_IMAGE_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
CALL_RUNTIME_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
SOURCE_SHA = "55d8fce47b663249e52bee88b9f200fac4b123e6f93a9c78dcd39566dca69cc0"
RAM_SHA = {
    "baseline": "a3cca0298f6be949aa02790da5733f50b105b860a67df558e3408503bef8c4c1",
    "next-entry": "fffde31cfe190e68a85452a9ac7f5428905dbf2b63c7a1446560a47b2bcdcd3d",
    "next-return": "ee88688e25e9be541e43adba0df579c565e35ec30747130978aeb7ca5abeb5e6",
}
PAGE_SHA = {
    "baseline": (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "8a8cedfdddb07fc46a32e7d22701833aa368116134a8de176296cf3cf97ae044",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "bcc58c5d0fcbcd6f50390b0e84e0e5ccb9178abd0f425754309e456138ce65e1",
    ),
    "next-entry": (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "8a8cedfdddb07fc46a32e7d22701833aa368116134a8de176296cf3cf97ae044",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "bcc58c5d0fcbcd6f50390b0e84e0e5ccb9178abd0f425754309e456138ce65e1",
    ),
    "next-return": (
        "d78a1c5ae193c6a5e21521f5f334a0bc9e4bd5008a1e167e482a1a8701de1ca7",
        "8b79188573854a3f7671cbe4e8f7709c3494cb0524bac76cf0e8954b6328d401",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "ef5c7e01e81f35c6a09d2bd3d94c5b7590b78b352ac9aedce04904c22d70209a",
    ),
    "first-view": (
        "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
        "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
    ),
}


def audit(exe_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA, "slot-A save")
    image, *_ = unpack(exe)
    near = image[0x13C8:0x13CB]
    call = image[0x23F3:0x23F8]
    check_hash(near, PARENT_CALL_SHA, "original RES_00BA:0828 near call")
    check_hash(image[0x23B6:0x2492], DISPLAY_IMAGE_SHA, "original RES_00BA:1816 body")
    check_hash(call, CALL_IMAGE_SHA, "original RES_00BA:1853 far call")
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA, "original printer body")
    if (len(near) != 3 or near[0] != 0xE8 or
            0x082B + struct.unpack_from("<h", near, 1)[0] != 0x1816):
        raise ValueError("parent call target changed")
    if len(call) != 5 or call[0] != 0x9A or struct.unpack_from("<HH", call, 1) != (0x0626, 0x0709):
        raise ValueError("original far call target changed")

    ram = {}
    for phase, expected in RAM_SHA.items():
        data = (run / f"ram-{phase}.bin").read_bytes()
        if len(data) != 655360:
            raise ValueError(f"{phase} RAM size")
        check_hash(data, expected, f"{phase} RAM")
        ram[phase] = data
    pre = ram["next-entry"]
    check_hash(pre[0x08DE0 + 0x0828:0x08DE0 + 0x082B],
               PARENT_CALL_SHA, "loaded parent near call")
    check_hash(pre[0x08DE0 + 0x1816:0x08DE0 + 0x18F2],
               DISPLAY_RUNTIME_SHA, "loaded display body")
    loaded_call = pre[0x08DE0 + 0x1853:0x08DE0 + 0x1858]
    check_hash(loaded_call, CALL_RUNTIME_SHA, "loaded printer far call")
    check_hash(pre[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
               PRINTER_RUNTIME_SHA, "loaded printer body")
    if (loaded_call[0] != 0x9A or
            struct.unpack_from("<HH", loaded_call, 1) != (0x0626, 0x0F2D)):
        raise ValueError("loaded printer target changed")

    ss = 0x1D160
    ret_ip, ret_cs, source_off, source_seg, color, row, col = struct.unpack_from(
        "<7H", pre, ss + 0x3E92)
    if (ret_ip, ret_cs, source_off, source_seg, color, row, col) != (
            0x1858, 0x08DE, 0x0008, 0x44D6, 11, 6, 17):
        raise ValueError("printer entry frame changed")
    if (word(pre, ss + 0x3EA4 + 2), word(pre, ss + 0x3EA4 + 4)) != (
            0x082B, 0x08DE):
        raise ValueError("parent return changed")
    source_at = source_seg * 16 + source_off
    length = pre[source_at]
    if length != 8:
        raise ValueError("source length changed")
    check_hash(pre[source_at:source_at + 1 + length], SOURCE_SHA, "Pascal source")
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
    for phase, hashes in PAGE_SHA.items():
        planes = [(run / f"page-{phase}-plane{lane}.bin").read_bytes()
                  for lane in range(4)]
        for lane, plane in enumerate(planes):
            if len(plane) != 3520:
                raise ValueError(f"{phase} plane {lane} size")
            check_hash(plane, hashes[lane], f"{phase} plane {lane}")
        pages[phase] = b"".join(planes)
    baseline, before, after, final = (pages[name] for name in
                                      ("baseline", "next-entry", "next-return", "first-view"))
    if baseline != before:
        raise ValueError("page changed before next printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {(lane * 3520 + (6 * 8 + scanline) * 40 + col - 0x3C0)
                for lane in (0, 1, 3) for scanline in range(7)
                for col in range(17, 25)}
    if changed != expected:
        raise ValueError("immediate eight-cell page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if len(later) != 672 or changed & later or len(total) != 840 or total != changed | later:
        raise ValueError("first-view persistence or later net delta changed")

    private_input = run / "row6-probe-input-private.bin"
    private_output = run / "row6-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              pre[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private probe input size")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=8 stores=256 changed=168":
        raise ValueError(f"C report changed: {result.stdout.strip()}")
    if private_output.read_bytes() != after:
        raise ValueError("C model differs from native immediate page")

    print("RES_00BA:0828 -> 1816; RES_00BA:1853 -> RES_0709:0626 -> 1858")
    print("eight cells, row 6 columns 17..24, color 11")
    print("page unchanged before call; immediate 168 changed plane bytes")
    print("168 persist at first view; 672 later net changes remain unattributed")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
