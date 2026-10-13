#!/usr/bin/env python3
"""Audit one reached row-five slot-A printer call from ignored DOS captures."""

import argparse
from collections import Counter
import hashlib
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
LIVE_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
PRINTER_SHA = "b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed"
LIVE_PRINTER_SHA = "8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
SOURCE_SHA = "9057f89eb79a4aa2452527d567cb057bbdcd4d23c5e6cc2039bc3bc61f1230c6"
RAM = {
    "pre": ("ram-next-printer-pre.bin", "bfcf562d4f20c22f684a7862121fee9d785ddd1bdfa4fd8096ccaa6d14133b11"),
    "post": ("ram-row5-post.bin", "bf0084b78468585522761302efaeef3406a513fd3ca6c47bbe3100ae2c4ad766"),
    "final": ("ram-first-view.bin", "25a38fd9014ef8368ca377203a59035768f9bfce242b83a2f5aa120dec0b139d"),
}
PAGES = {
    "pre": ("row5-pre", (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "11f211c59dbfccd37c3743e5a9bd94aebbeb9e17040a1e559e6aba8e35eede3a",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "32ab35dab09d52b1b3031315b480a388bfca4b5329c039680c64840c131dbc16")),
    "post": ("row5-post", (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "c2723e21c95b047443e8373503995696dd6351ba6980c34cb526a441d551e97d",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "3c70ba628cc2148e46bab880f349b4f481dfe244d4e03a1592b14972fa61328c")),
    "final": ("first-view", (
        "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
        "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37")),
}


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def pinned(path, digest, size):
    data = path.read_bytes()
    require(len(data) == size and hashlib.sha256(data).hexdigest() == digest,
            f"{path.name}: size/SHA-256")
    return data


def changed(a, b):
    return {(p, at) for p in range(4) for at in range(3520)
            if a[p][at] != b[p][at]}


def audit(exe_path, run, probe):
    exe = pinned(exe_path, START_SHA, 65231)
    image, *_ = unpack(exe)
    call = image[0x1508:0x150D]
    require(hashlib.sha256(call).hexdigest() == CALL_SHA and
            struct.unpack_from("<2H", call, 1) == (0x0626, 0x0709),
            "original RES_00BA:0968 far call")
    require(hashlib.sha256(image[0x76B6:0x7737]).hexdigest() == PRINTER_SHA,
            "original RES_0709:0626 printer body")
    ram = {name: pinned(run / filename, digest, 655360)
           for name, (filename, digest) in RAM.items()}
    pre = ram["pre"]
    require(hashlib.sha256(pre[0x08DE0 + 0x0968:0x08DE0 + 0x096D]).hexdigest() ==
            LIVE_CALL_SHA, "relocated printer call")
    require(hashlib.sha256(pre[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7]).hexdigest() ==
            LIVE_PRINTER_SHA, "relocated printer body")
    sp = 0x1D160 + 0x3C96
    ret_ip, ret_cs, source_off, source_seg, color, row, column = (
        struct.unpack_from("<7H", pre, sp))
    require((ret_ip, ret_cs, source_off, source_seg, color, row, column) ==
            (0x096D, 0x08DE, 0x3DA6, 0x1D16, 10, 5, 34), "row-five caller frame")
    source_at = source_seg * 16 + source_off
    length = pre[source_at]
    source = pre[source_at:source_at + length + 1]
    require(length == 1 and hashlib.sha256(source).hexdigest() == SOURCE_SHA,
            "one-cell Pascal source")
    font_off, font_seg = struct.unpack_from("<2H", pre, 0x14A00 + 0x5E20)
    font = pre[font_seg * 16 + font_off:font_seg * 16 + font_off + 1416]
    require(len(font) == 1416 and hashlib.sha256(font).hexdigest() == FONT_SHA,
            "live font")
    require(pre[0x14A00 + 0x4A1E] == 1 and
            struct.unpack_from("<2H", pre, 0x14A00 + 0x4A16) == (0xA000, 0xA000),
            "live EGA page state")
    pages = {name: [pinned(run / f"page-{prefix}-plane{p}.bin", digest, 3520)
                    for p, digest in enumerate(hashes)]
             for name, (prefix, hashes) in PAGES.items()}
    before, after, final = (pages[name] for name in ("pre", "post", "final"))
    region = {(p, ((row * 8 + y) * 40 + column) - 0x3C0)
              for p in range(4) for y in range(8)}
    immediate = changed(before, after)
    later = changed(after, final)
    require(len(immediate) == 14 and immediate <= region and
            Counter(p for p, _ in immediate) == {1: 7, 3: 7},
            "row-five native one-cell delta")
    require(len(later) == 854 and not later & region and
            len(changed(before, final)) == 868,
            "later first-view remainder")
    require(all(after[p][at] == final[p][at] for p, at in immediate),
            "one-cell final boundary values")
    require(hashlib.sha256(b"".join(before)).hexdigest() ==
            "b48537bb34935271cef3797cd3eff166d70eebcec2e08db3fd0c6300a511a128" and
            hashlib.sha256(b"".join(final)).hexdigest() ==
            "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b",
            "published page boundaries")

    private_input = run / "row5-audit-input-private.bin"
    private_output = run / "row5-audit-output-private.bin"
    record = (font + bytes((length, column, row, color)) + source[1:] +
              bytes(255 - length) + b"".join(before))
    require(len(record) == 15755, "private probe record")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    require(result.stdout.strip() == "cells=1 stores=32 changed=14", "C report")
    require(private_output.read_bytes() == b"".join(after), "C/native page parity")
    print("RES_00BA:0968 -> RES_0709:0626 -> RES_00BA:096D")
    print("row 5 column 34: one cell, 32 modeled stores, 14 changes")
    print("C/native immediate page: 14080/14080 bytes; 854 later changes remain")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
