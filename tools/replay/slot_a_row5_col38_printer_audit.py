#!/usr/bin/env python3
"""Check one saved-slot printer call using ignored DOS and EGA captures.

Only addresses, counts, and hashes are printed. Original media, text, font,
RAM, page data, and generated probe records stay under ignored resources/.
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
NEAR_CALL_SHA = "e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4"
BODY_IMAGE_SHA = "a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0"
BODY_RUNTIME_SHA = "c7577505151147a243592d02b42335233680a27273dc5df3f427d4e778a3c3d3"
SOURCE_SHA = "06f647b157b8557a12979ba04cf5ba222b9747cf5bbe82f24a48e25c0c508042"
RAM_SHA = {
    "next-entry": "d8e994f159c9d4ad6f59ef3cde511b769e1b87a2b18ca7c04ced592c46b7d0e5",
    "next-return": "b65100d911d1fe2d457fd47707046d11e63271474490b1e6885139d43b304244",
}
PAGE_SHA = {
    "next-entry": (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "c2723e21c95b047443e8373503995696dd6351ba6980c34cb526a441d551e97d",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "3c70ba628cc2148e46bab880f349b4f481dfe244d4e03a1592b14972fa61328c",
    ),
    "next-return": (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "8a8cedfdddb07fc46a32e7d22701833aa368116134a8de176296cf3cf97ae044",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "bcc58c5d0fcbcd6f50390b0e84e0e5ccb9178abd0f425754309e456138ce65e1",
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
    near = image[0x1457:0x145A]
    check_hash(near, NEAR_CALL_SHA, "original 00BA:08B7 near call")
    check_hash(image[0x1513:0x1569], BODY_IMAGE_SHA, "original 00BA:0973 body")
    if len(near) != 3 or near[0] != 0xE8 or 0x08BA + struct.unpack_from("<h", near, 1)[0] != 0x0973:
        raise ValueError("near parent call does not target RES_00BA:0973")
    call = image[0x155E:0x1563]
    check_hash(call, CALL_IMAGE_SHA, "original 00BA:09BE far call")
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA, "original printer body")
    if len(call) != 5 or call[0] != 0x9A or struct.unpack_from("<HH", call, 1) != (0x0626, 0x0709):
        raise ValueError("original call does not target RES_0709:0626")

    ram = {}
    for phase, expected in RAM_SHA.items():
        data = (run / f"ram-{phase}.bin").read_bytes()
        if len(data) != 655360:
            raise ValueError(f"{phase} RAM size")
        check_hash(data, expected, f"{phase} RAM")
        ram[phase] = data
    pre = ram["next-entry"]
    check_hash(pre[0x08DE0 + 0x08B7:0x08DE0 + 0x08BA],
               NEAR_CALL_SHA, "loaded 08DE:08B7 near call")
    check_hash(pre[0x08DE0 + 0x0973:0x08DE0 + 0x09C9],
               BODY_RUNTIME_SHA, "loaded 08DE:0973 body")
    loaded = pre[0x08DE0 + 0x09BE:0x08DE0 + 0x09C3]
    check_hash(loaded, CALL_RUNTIME_SHA, "loaded 08DE:09BE far call")
    check_hash(pre[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
               PRINTER_RUNTIME_SHA, "loaded printer body")
    if loaded[0] != 0x9A or struct.unpack_from("<HH", loaded, 1) != (0x0626, 0x0F2D):
        raise ValueError("loaded call does not target 0F2D:0626")

    ss = 0x1D160
    ret_ip, ret_cs, source_off, source_seg, color, row, col = struct.unpack_from(
        "<7H", pre, ss + 0x3D94)
    if (ret_ip, ret_cs, source_off, source_seg, color, row, col) != (
            0x09C3, 0x08DE, 0x3DA2, 0x1D16, 10, 5, 38):
        raise ValueError("live printer frame changed")
    if (word(pre, ss + 0x3EA4 + 2), word(pre, ss + 0x3EA4 + 4)) != (0x08BA, 0x08DE):
        raise ValueError("parent caller return changed")
    source_at = source_seg * 16 + source_off
    length = pre[source_at]
    if length != 1:
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
    before, after, final = (pages[name] for name in
                            ("next-entry", "next-return", "first-view"))
    prior = b"".join((run / f"page-baseline-plane{lane}.bin").read_bytes()
                     for lane in range(4))
    if prior != before:
        raise ValueError("page changed between row-five return and next printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    cell = {(lane * 3520 + (5 * 8 + scanline) * 40 + 38 - 0x3C0)
            for lane in (1, 3) for scanline in range(7)}
    if changed != cell:
        raise ValueError("immediate one-cell page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if len(later) != 840 or changed & later or len(total) != 854 or total != changed | later:
        raise ValueError("first-view persistence or later net delta changed")

    private_input = run / "col38-probe-input-private.bin"
    private_output = run / "col38-probe-output-private.bin"
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

    print("RES_00BA:09BE -> RES_0709:0626 -> RES_00BA:09C3")
    print("parent return RES_00BA:08BA; row 5 column 38 color 10 length 1")
    print("page unchanged before call; immediate 14 changed plane bytes")
    print("14 persist at first view; 840 later net changes remain unattributed")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
