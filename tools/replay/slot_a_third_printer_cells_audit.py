#!/usr/bin/env python3
"""Audit one reached two-cell slot-A EGA printer call from ignored captures.

Only addresses, hashes, counts and parity summaries are printed. Original
text, font, RAM and EGA planes remain under ignored resources/.
"""

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
SOURCE_SHA = "fba5fe6efee9c741299411a77a7aa9e35505c7459f393499a6a3bf2f6beb4b32"
RAM_SHA = {
    "third-pre": "88f846a1401c0879dca04ff74b5cb25bc9776861e12a8f402f70e44f4638f12b",
    "third-post": "d06762ea7a5f6453db3f1225590a764c67977edfcc2cd644d234bd21dc0497b1",
    "first-view": "d5da21fea8d8c74a20ae3415873d38aaade64f714aac05f369e71d066b16b0b1",
}
PAGE_SHA = {
    "third-pre": (
        "accf1d70b948df184630639a8aa9afe7d87bbe3030fd4d5e8be464a580e58f8e",
        "e9723e209c36c775a8feaa0976fe9f12038b8c7d2579f7f1dbf269f12181655b",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "87b511b0a0f7426086cd5030da5f11e02732ef2bfc7dc4246d96dcc32b9e8f9b",
    ),
    "third-post": (
        "accf1d70b948df184630639a8aa9afe7d87bbe3030fd4d5e8be464a580e58f8e",
        "3ab4afbf196484baf33806393eae0899e63e2bc03615b17fc63a6902f0383bd4",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "c745614437e6223a0ec0c2008812abd7f9431ef6a843cf2f5e0b09be1f3878eb",
    ),
    "first-view": (
        "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
        "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
    ),
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def pinned(path, expected, length):
    data = path.read_bytes()
    require(len(data) == length, f"{path.name}: length")
    require(hashlib.sha256(data).hexdigest() == expected, f"{path.name}: SHA-256")
    return data


def delta(before, after):
    return {(p, at) for p in range(4) for at in range(3520)
            if before[p][at] != after[p][at]}


def audit(exe_path, run, probe):
    exe = pinned(exe_path, START_SHA, 65231)
    image, *_ = unpack(exe)
    call = image[0x155E:0x1563]
    require(hashlib.sha256(call).hexdigest() == CALL_SHA and
            struct.unpack_from("<2H", call, 1) == (0x0626, 0x0709),
            "original RES_00BA:09BE printer call")
    require(hashlib.sha256(image[0x76B6:0x7737]).hexdigest() == PRINTER_SHA,
            "original RES_0709:0626 printer")
    ram = {name: pinned(run / f"ram-{name}.bin", digest, 655360)
           for name, digest in RAM_SHA.items()}
    pre = ram["third-pre"]
    require(hashlib.sha256(pre[0x08DE0 + 0x09BE:0x08DE0 + 0x09C3]).hexdigest() ==
            LIVE_CALL_SHA, "live third-printer call")
    require(hashlib.sha256(pre[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7]).hexdigest() ==
            LIVE_PRINTER_SHA, "live printer body")
    sp = 0x1D160 + 0x3D94
    ret_ip, ret_cs, source_off, source_seg, color, row, column = (
        struct.unpack_from("<7H", pre, sp))
    require((ret_ip, ret_cs, source_off, source_seg, color, row, column) ==
            (0x09C3, 0x08DE, 0x3DA2, 0x1D16, 10, 4, 37), "third-printer ABI")
    source_at = source_seg * 16 + source_off
    length = pre[source_at]
    source = pre[source_at:source_at + length + 1]
    require(length == 2 and hashlib.sha256(source).hexdigest() == SOURCE_SHA,
            "two-cell Pascal source")
    ds = 0x14A00
    font_off, font_seg = struct.unpack_from("<2H", pre, ds + 0x5E20)
    font = pre[font_seg * 16 + font_off:font_seg * 16 + font_off + 1416]
    require(len(font) == 1416 and hashlib.sha256(font).hexdigest() == FONT_SHA,
            "live font")
    require(pre[ds + 0x4A1E] == 1 and
            struct.unpack_from("<2H", pre, ds + 0x4A16) == (0xA000, 0xA000),
            "live EGA page state")

    pages = {name: [pinned(run / f"page-{name}-plane{p}.bin", digest, 3520)
                    for p, digest in enumerate(hashes)]
             for name, hashes in PAGE_SHA.items()}
    before, after, final = (pages[name] for name in
                            ("third-pre", "third-post", "first-view"))
    cell = {(p, ((row * 8 + scanline) * 40 + column + index) - 0x3C0)
            for p in range(4) for scanline in range(8) for index in range(length)}
    invoked = delta(before, after)
    later = delta(after, final)
    require(len(invoked) == 28 and invoked <= cell and
            Counter(p for p, _ in invoked) == {1: 14, 3: 14},
            "two-cell native delta")
    require(len(later) == 994 and not later & cell and
            len(delta(before, final)) == 1022, "later page remainder")
    require(all(after[p][at] == final[p][at] for p, at in invoked),
            "third-printer values persist")

    private_input = run / "third-printer-audit-input-private.bin"
    private_output = run / "third-printer-audit-output-private.bin"
    record = (font + bytes((length, column, row, color)) + source[1:] +
              bytes(255 - length) + b"".join(before))
    require(len(record) == 15755, "private probe record")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True, capture_output=True,
                            text=True)
    require(result.stdout.strip() == "cells=2 stores=64 changed=28", "C report")
    require(private_output.read_bytes() == b"".join(after), "C/native page parity")
    print("RES_00BA:09BE -> RES_0709:0626 -> RES_00BA:09C3")
    print("third printer: 2 cells, 64 modeled stores, 28 changed plane bytes")
    print("C/native immediate page: 14080/14080 bytes; 28 persist at view return")
    print("994 later plane changes remain unattributed")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
