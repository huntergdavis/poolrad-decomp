#!/usr/bin/env python3
"""Audit the reached saved slot-A row-nine seven-cell EGA printer call.

Media, source, font, guest RAM, and page planes stay under ignored resources/.
Only addresses, hashes, and counts are printed.
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
SOURCE_SHA = "df900d38b6c327a4530a20ce19d2cb15b823e82694f232ef6e448d05f22c89e3"
BEFORE_SHA = "dfd1c5681f895d4c5495ef166bc955db0a258d569dd19cc72fcbef56eac55b64"
AFTER_SHA = "01195f7efda5f563bf20ef0b3d97f687786cd4b6585f89f30ed5fc494f5dff9b"
FINAL_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

SPANS = (
    ("RES_00BA:0816..082B caller", 0x13B6, 0x13CB,
     "18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d",
     0x08DE, 0x0816, 0x082B,
     "18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d"),
    ("RES_00BA:0828 near call", 0x13C8, 0x13CB,
     "101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3",
     0x08DE, 0x0828, 0x082B,
     "101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3"),
    ("RES_00BA:1816 body", 0x23B6, 0x2432,
     "645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175",
     0x08DE, 0x1816, 0x1892,
     "ea633c656758bfd81c77f802f1ba830ede8f0973339f67d57a2880d3413d9d01"),
    ("RES_00BA:1853 far printer call", 0x23F3, 0x23F8,
     "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23",
     0x08DE, 0x1853, 0x1858,
     "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    ("RES_0709:0626 printer body", 0x76B6, 0x7737, PRINTER_IMAGE_SHA,
     0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
)

RAM_SHA = {
    "ram-row8-col38-09c3-private.bin":
        "357756788552b20386c3ae0b19683d268669b6f6a988a4cd062b0207536c0eb2",
    "ram-next-printer-entry-private.bin":
        "65120e0b2dcb4e3cf14088f1828d08bf8acb6dcf6da62cb04652bdf0c9a0e070",
    "ram-next-printer-return-private.bin":
        "2456f154a82e8e559c9aa90156e2be7ea4b9d5fb759a457877f3ed516b3f3e5c",
    "ram-first-view-private.bin":
        "46649c3fc3812efb5be23bd96402bebfccca08e0a3a8dc431f12839e6c8e7fa0",
}

BEFORE_PLANES = (
    "0ec506a07d3401d81dea67faab4f6cf010deaf4bb1e553a65b37af4979a637a3",
    "22c8989bbcec0236b2c72c155ecead83cd63a00ebc0e0d9cc340817e41bbaebc",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "b4207b761ae69dc53225efcf4acf47c9bfa77edd5ae5b5ec988da2ea5a12f218",
)
AFTER_PLANES = (
    "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
    "fc13282b8824aac005ee523eceb9bf4d8ca8e857820773ad4db09a02b87cc01f",
    BEFORE_PLANES[2],
    "82ba6a0cd0da3fbe53453171588007d24894ebcf8330be1d0d7caf5a5d977412",
)
FINAL_PLANES = (
    AFTER_PLANES[0],
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
    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    entry = ram["ram-next-printer-entry-private.bin"]
    for name, a, b, orig_sha, seg, c, d, loaded_sha in SPANS:
        check_hash(image[a:b], orig_sha, f"original {name}")
        check_hash(entry[seg * 16 + c:seg * 16 + d], loaded_sha,
                   f"loaded {name}")
    near = image[0x13C8:0x13CB]
    if near[0] != 0xE8 or 0x082B + struct.unpack_from("<h", near, 1)[0] != 0x1816:
        raise ValueError("original near callee changed")
    far = image[0x23F3:0x23F8]
    loaded_far = entry[0x08DE0 + 0x1853:0x08DE0 + 0x1858]
    if (far[0] != 0x9A or struct.unpack_from("<HH", far, 1) !=
            (0x0626, 0x0709) or loaded_far[0] != 0x9A or
            struct.unpack_from("<HH", loaded_far, 1) != (0x0626, 0x0F2D)):
        raise ValueError("original or loaded far printer target changed")
    if struct.unpack_from("<8H", ram["ram-row8-col38-09c3-private.bin"],
                          0x1D160 + 0x3EA4) != (
            0x3EC2, 0x08BA, 0x08DE, 0, 8, 38, 8, 0x4513):
        raise ValueError("row8 col38 anchor frame changed")
    if struct.unpack_from("<8H", entry, 0x1D160 + 0x3EA4) != (
            0x3EC2, 0x082B, 0x08DE, 0, 9, 17, 8, 0x4529):
        raise ValueError("1816 parent frame changed")
    ret_ip, ret_cs, src_off, src_seg, color, row, col = struct.unpack_from(
        "<7H", entry, 0x1D160 + 0x3E92)
    if (ret_ip, ret_cs, src_off, src_seg, color, row, col) != (
            0x1858, 0x08DE, 8, 0x4529, 11, 9, 17):
        raise ValueError("printer entry frame changed")
    source_at = src_seg * 16 + src_off
    length = entry[source_at]
    if (length, entry[source_at + 0x10D],
            entry[source_at + 0x10E]) != (7, 1, 0):
        raise ValueError("source length or color branch inputs changed")
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

    anchor = page(run, "row8-col38", BEFORE_PLANES, BEFORE_SHA)
    before = page(run, "next-entry", BEFORE_PLANES, BEFORE_SHA)
    after = page(run, "next-return", AFTER_PLANES, AFTER_SHA)
    final = page(run, "first-view", FINAL_PLANES, FINAL_SHA)
    if anchor != before:
        raise ValueError("page changed before next printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {
        lane * 3520 + scanline * 40 + col - 0x3C0
        for lane in (0, 1, 3) for scanline in range(72, 79)
        for col in range(17, 24)
    }
    if changed != expected or len(changed) != 147:
        raise ValueError("seven-cell immediate page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 28 or changed & later or len(total) != 175 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "row9-seven-probe-input-private.bin"
    private_output = run / "row9-seven-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              entry[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private C probe input size changed")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=7 stores=224 changed=147":
        raise ValueError("C probe report changed")
    if private_output.read_bytes() != after:
        raise ValueError("C probe differs from native immediate page")

    print("RES_00BA:0828 -> 1816 -> 1853 -> RES_0709:0626 -> 1858")
    print("row 9 columns 17-23; seven cells; color 11; 224 stores; 147 changed bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FINAL_SHA)
    print("147/147 changed bytes persist; 28 later net bytes remain")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
