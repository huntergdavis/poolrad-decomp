#!/usr/bin/env python3
"""Hash-locked private audit of one reached slot-A EGA string print.

Original binaries, text, RAM and planar pages stay under ignored resources/.
Only offsets, hashes, counts and match totals are printed.
"""

import argparse
import hashlib
from pathlib import Path
import struct
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PRINTER_IMAGE_SHA = "b9cd1d6249e9f10945fcf39c09ae313ba672693230d20b29d342bc04bd9894ed"
CALL_IMAGE_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
PRINTER_RUNTIME_SHA = "8b256fdb14206f0bd4772fd11f7f2ab145b3120954231038c12cf83a7b6c7164"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
STRING_SHA = "6c2e68d9723b65bacedc6effe4898a0fb1a437e902f4a08b5d0dc030221f6feb"
RAM_SHA = {
    "post-first-flush": "239b2d982b76ebf1c23b2e540c26d2b4f4d7d0d5b9637518c95c26f747f5d351",
    "pre-printer": "fa135bbaa31c53f482f7eb693dc95f1b1a051573adb9b3a12106b0a9269fb88e",
    "post-printer": "371f123afd0fe1a54dffd38797254c0b2a9fed93dc9806f40c12d5a83af5301a",
    "first-view": "9b7ea330309dafff495b2a91b2060c1580f18b3be1d09a7ec3a3422243575ebc",
}
PAGE_SHA = {
    "post-flush": (
        "d814a5546a0b639d0c938bf8329dedb24ec51fb58c9575ea8a77b40ef9dc542b",
        "70f07b00774fe17f46902e98dd97ace2e65fc3db2f646a7f787c8b31b17b80e4",
        "856d9c6df1956d1f341382133ecce008235a2ac9ad20410a4209cc9ec54574dd",
        "1bccbe447dfff5abd872cc3c3d84f05ef15ef941970879f6c5e8a2cf3daf10ac",
    ),
    "pre-printer": (
        "d814a5546a0b639d0c938bf8329dedb24ec51fb58c9575ea8a77b40ef9dc542b",
        "70f07b00774fe17f46902e98dd97ace2e65fc3db2f646a7f787c8b31b17b80e4",
        "856d9c6df1956d1f341382133ecce008235a2ac9ad20410a4209cc9ec54574dd",
        "1bccbe447dfff5abd872cc3c3d84f05ef15ef941970879f6c5e8a2cf3daf10ac",
    ),
    "post-printer": (
        "accf1d70b948df184630639a8aa9afe7d87bbe3030fd4d5e8be464a580e58f8e",
        "22f8ac3b1dec45c17ccadaae8f700c520b1bf8e1761f7cb6ce4da66b5e6a6471",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "3b2adda1500d1630d530a66f86ecf0c6f002f117248cee035a43dcca5bdec9d8",
    ),
    "first-view": (
        "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
        "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
        "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
        "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
    ),
}


def check_hash(data, expected, label):
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise ValueError(f"{label}: SHA-256 mismatch: {actual}")


def word(data, at):
    return struct.unpack_from("<H", data, at)[0]


def audit(exe_path, run, probe):
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START")
    image, *_ = unpack(exe)
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA, "printer image")
    check_hash(image[0x13AF:0x13B4], CALL_IMAGE_SHA, "resident caller")

    ram = {}
    for phase, expected in RAM_SHA.items():
        data = (run / f"ram-{phase}.bin").read_bytes()
        if len(data) != 655360:
            raise ValueError(f"{phase} RAM length")
        check_hash(data, expected, f"{phase} RAM")
        ram[phase] = data
    pre = ram["pre-printer"]
    post = ram["post-printer"]
    check_hash(pre[0x0F2D0 + 0x626:0x0F2D0 + 0x6A7],
               PRINTER_RUNTIME_SHA, "runtime printer")
    if pre[0x08DE0 + 0x80F:0x08DE0 + 0x814] != bytes.fromhex("9a26062d0f"):
        raise ValueError("runtime caller instruction changed")

    sp = 0x1D160 + 0x3EAA
    source_off, source_seg, color, row, column = struct.unpack_from("<5H", pre, sp)
    if (source_off, source_seg, color, row, column) != (8, 0x449A, 15, 4, 17):
        raise ValueError("pre-call printer ABI changed")
    source_at = source_off + 16 * source_seg
    length = pre[source_at]
    raw_string = pre[source_at:source_at + length + 1]
    check_hash(raw_string, STRING_SHA, "source Pascal string")
    if length != 14:
        raise ValueError("printer input length changed")
    ds = 0x14A00
    font_at = word(pre, ds + 0x5E20) + 16 * word(pre, ds + 0x5E22)
    font = pre[font_at:font_at + 1416]
    check_hash(font, FONT_SHA, "live font")
    if word(pre, ds + 0x4A16) != 0xA000 or word(pre, ds + 0x4A18) != 0xA000:
        raise ValueError("display page segments changed")
    if pre[ds + 0x4A1E] != 1:
        raise ValueError("display is not EGA mode")
    bp = 0x1D160 + 0x3EA4
    if (post[bp - 0x100], post[bp - 0x102], post[bp - 0x103],
            word(post, bp + 0x0E), word(post, bp + 2), word(post, bp + 4)) != (
            14, 14, 14, 31, 0x0814, 0x08DE):
        raise ValueError("post-call iteration or return frame changed")

    pages = {}
    for phase, hashes in PAGE_SHA.items():
        planes = []
        for lane, expected in enumerate(hashes):
            data = (run / f"page-{phase}-plane{lane}.bin").read_bytes()
            if len(data) != 3520:
                raise ValueError(f"{phase} plane {lane} length")
            check_hash(data, expected, f"{phase} plane {lane}")
            planes.append(data)
        pages[phase] = planes
    clean = {(lane, at) for lane in range(4) for at in range(3520)
             if at % 40 < 3 or at % 40 > 13}
    cell = {(lane, at) for lane in range(4) for at in range(3520)
            if 8 <= at // 40 <= 15 and 17 <= at % 40 <= 30}

    def changed(before, after):
        return {(lane, at) for lane in range(4) for at in range(3520)
                if pages[before][lane][at] != pages[after][lane][at]}

    first = changed("post-flush", "pre-printer")
    invoked = changed("pre-printer", "post-printer")
    later = changed("post-printer", "first-view")
    total = changed("post-flush", "first-view")
    if first or len(invoked) != 368 or invoked - cell or invoked - clean:
        raise ValueError("isolated printer contribution changed")
    if len(later) != 1036 or later & cell or later - clean:
        raise ValueError("post-printer clean changes changed")
    if len(total) != 1404 or total != invoked | later:
        raise ValueError("first-view clean total changed")

    private_input = run / "printer-input-private.bin"
    private_output = run / "printer-model-post-private.bin"
    record = (font + bytes((length, column, row, color)) +
              raw_string[1:] + bytes(255 - length) +
              b"".join(pages["pre-printer"]))
    if len(record) != 1416 + 4 + 255 + 4 * 3520:
        raise ValueError("private probe record size")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=14 stores=448 changed=368":
        raise ValueError(f"C report changed: {result.stdout.strip()}")
    native_after = b"".join(pages["post-printer"])
    model_after = private_output.read_bytes()
    if model_after != native_after:
        mismatches = sum(a != b for a, b in zip(model_after, native_after))
        raise ValueError(f"C model mismatch: {mismatches} plane bytes")
    print("reached RES_00BA:080F -> RES_0709:0626 -> RES_00BA:0814")
    print("native printer: 14 cells, 448 plane-byte targets, 368 value changes")
    print("first-view clean changes: 368 printer + 1036 later = 1404")
    print("C model: 14080/14080 plane bytes at immediate printer return")
    print("all original inputs and captures remain under ignored resources/")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
