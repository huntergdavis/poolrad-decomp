#!/usr/bin/env python3
"""Audit a reached six-cell slot-A printer call using ignored DOS captures."""

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
SOURCE_SHA = "e181017de583f193d09392267f0c03e789225c9bbe5265095326e792d5f3b6c5"
RAM = {
    "pre": ("ram-next-printer-pre.bin", "3e3ed4dd8f27ff6d338879260f7f6128069313bd3c7ac7bc8ceaf8fed71bb153"),
    "post": ("ram-next-printer-post.bin", "9be41c240e4896c3f98cf64bc6eab66ca2e1eeee9fb9bfed5dfe45b3710459f6"),
    "final": ("ram-first-view.bin", "f67e100ee3d16222bfa49beb5bd05c02529a2d740e2918b41e5f0b449e8fe660"),
}
PAGES = {
    "pre": ("next-printer-pre", (
        "accf1d70b948df184630639a8aa9afe7d87bbe3030fd4d5e8be464a580e58f8e",
        "3ab4afbf196484baf33806393eae0899e63e2bc03615b17fc63a6902f0383bd4",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "c745614437e6223a0ec0c2008812abd7f9431ef6a843cf2f5e0b09be1f3878eb")),
    "post": ("next-printer-post", (
        "6a6f41cc6faf11499fbd82fbdd32ad1c68381f079a1b20ad88be80753456bbe0",
        "11f211c59dbfccd37c3743e5a9bd94aebbeb9e17040a1e559e6aba8e35eede3a",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "32ab35dab09d52b1b3031315b480a388bfca4b5329c039680c64840c131dbc16")),
    "final": ("first-view", (
        "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
        "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37")),
}


def require(ok, reason):
    if not ok:
        raise ValueError(reason)


def pinned(path, digest, size):
    data = path.read_bytes()
    require(len(data) == size and hashlib.sha256(data).hexdigest() == digest,
            f"{path.name}: size/SHA-256")
    return data


def changed(a, b):
    return {(p, i) for p in range(4) for i in range(3520) if a[p][i] != b[p][i]}


def audit(exe_path, run, probe):
    exe = pinned(exe_path, START_SHA, 65231)
    image, *_ = unpack(exe)
    call = image[0x23F3:0x23F8]
    require(hashlib.sha256(call).hexdigest() == CALL_SHA and
            struct.unpack_from("<2H", call, 1) == (0x0626, 0x0709),
            "original RES_00BA:1853 far call")
    require(hashlib.sha256(image[0x76B6:0x7737]).hexdigest() == PRINTER_SHA,
            "original RES_0709:0626 body")
    ram = {name: pinned(run / filename, digest, 655360)
           for name, (filename, digest) in RAM.items()}
    pre = ram["pre"]
    require(hashlib.sha256(pre[0x08DE0 + 0x1853:0x08DE0 + 0x1858]).hexdigest() ==
            LIVE_CALL_SHA, "relocated six-cell far call")
    require(hashlib.sha256(pre[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7]).hexdigest() ==
            LIVE_PRINTER_SHA, "relocated printer body")
    sp = 0x1D160 + 0x3E92
    ret_ip, ret_cs, source_off, source_seg, color, row, column = (
        struct.unpack_from("<7H", pre, sp))
    require((ret_ip, ret_cs, source_off, source_seg, color, row, column) ==
            (0x1858, 0x08DE, 0x0008, 0x44BB, 11, 5, 17), "live caller frame")
    source_at = source_seg * 16 + source_off
    length = pre[source_at]
    source = pre[source_at:source_at + length + 1]
    require(length == 6 and hashlib.sha256(source).hexdigest() == SOURCE_SHA,
            "live Pascal source")
    font_off, font_seg = struct.unpack_from("<2H", pre, 0x14A00 + 0x5E20)
    font = pre[font_seg * 16 + font_off:font_seg * 16 + font_off + 1416]
    require(len(font) == 1416 and hashlib.sha256(font).hexdigest() == FONT_SHA,
            "live font")
    require(pre[0x14A00 + 0x4A1E] == 1 and
            struct.unpack_from("<2H", pre, 0x14A00 + 0x4A16) == (0xA000, 0xA000),
            "EGA page state")
    pages = {name: [pinned(run / f"page-{prefix}-plane{p}.bin", digest, 3520)
                    for p, digest in enumerate(hashes)]
             for name, (prefix, hashes) in PAGES.items()}
    before, after, final = (pages[name] for name in ("pre", "post", "final"))
    region = {(p, ((row * 8 + y) * 40 + column + x) - 0x3C0)
              for p in range(4) for y in range(8) for x in range(length)}
    immediate = changed(before, after)
    later = changed(after, final)
    require(len(immediate) == 126 and immediate <= region and
            Counter(p for p, _ in immediate) == {0: 42, 1: 42, 3: 42},
            "six-cell immediate delta")
    require(len(later) == 868 and not later & region and
            len(changed(before, final)) == 994,
            "first-view later remainder")
    require(all(after[p][i] == final[p][i] for p, i in immediate),
            "six-cell final boundary values")
    require(hashlib.sha256(b"".join(before)).hexdigest() ==
            "00a5a7693604b8f8487ced7ab081c04a3ba5dffd2b8f0b749c9ff80919267d96" and
            hashlib.sha256(b"".join(final)).hexdigest() ==
            "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b",
            "published page boundaries")

    private_input = run / "six-cell-probe-input-private.bin"
    private_output = run / "six-cell-probe-output-private.bin"
    record = (font + bytes((length, column, row, color)) + source[1:] +
              bytes(255 - length) + b"".join(before))
    require(len(record) == 15755, "private probe record")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], capture_output=True,
                            text=True, check=True)
    require(result.stdout.strip() == "cells=6 stores=192 changed=126", "C report")
    require(private_output.read_bytes() == b"".join(after), "C/native page parity")
    print("RES_00BA:1853 -> RES_0709:0626 -> RES_00BA:1858")
    print("six cells, 192 modeled stores, 126 changed plane bytes")
    print("C/native immediate page: 14080/14080 bytes; 868 later changes remain")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
