#!/usr/bin/env python3
"""Audit the next saved slot-A printer page after row seven column 37.

Original media, guest RAM, source, font, page planes, and probe records stay
under ignored resources/. Output contains only addresses, hashes, and counts.
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
SOURCE_SHA = "571899d9b8851d7601f2c6f985a0af1dd75f5788993877c71107d3c27bdf86be"
BEFORE_SHA = "99ce3c3496cf59d0f0e43b67ac66d03ad0e09af30168d19ea0bf55877500cbc0"
AFTER_SHA = "8723a10b9ac3747fd9edc42003a59c2434d1aa734fab75db17483eebd0b95498"
FINAL_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

ORIGINAL_SPANS = {
    "RES_00BA:0816..082B outer call context": (0x13B6, 0x13CB,
        "18cf9b0353ef4afd377dce4e69f5ba20a64db1aa5e723eeb5611ff3ce3396b1d"),
    "RES_00BA:0828 near call": (0x13C8, 0x13CB,
        "101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3"),
    "RES_00BA:1816 body": (0x23B6, 0x2432,
        "645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175"),
    "RES_00BA:1853 far call": (0x23F3, 0x23F8,
        "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    "RES_0709:0626 body": (0x76B6, 0x7737, PRINTER_IMAGE_SHA),
}

RUNTIME_SPANS = {
    "08DE:0816..082B outer call context": (0x08DE, 0x0816, 0x082B,
        ORIGINAL_SPANS["RES_00BA:0816..082B outer call context"][2]),
    "08DE:0828 near call": (0x08DE, 0x0828, 0x082B,
        ORIGINAL_SPANS["RES_00BA:0828 near call"][2]),
    "08DE:1816 body": (0x08DE, 0x1816, 0x1892,
        "ea633c656758bfd81c77f802f1ba830ede8f0973339f67d57a2880d3413d9d01"),
    "08DE:1853 far call": (0x08DE, 0x1853, 0x1858,
        "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    "0F2D:0626 body": (0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
}

RAM_SHA = {
    "ram-row7-08ba-private.bin":
        "5c6bbc9e2445ddc0bc7c3731d8532ad7f67a4a3dffc7f994c864c2b8b93f75b5",
    "ram-next-printer-entry-private.bin":
        "f25f8873af1a1dcebafb4ede1ba48186b181d0f714a9e8c5310d7c25d9a5e21b",
    "ram-next-printer-return-private.bin":
        "678788385e3517c24208d738c6e461bac2d9ec2fe842e6471170f90f080e7e1c",
}

BEFORE_PLANES = (
    "6a33ada38659de42223137dc19137d392e8be25f3e42bfc6e96a141609559481",
    "a4062ba707d1f1aec7ec67d41273add61e541730e87e72ba1a66e49874e9159c",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "0a07bc1e8e99a1210c423308db6bdea61c8f7af42406b7483777966e1e1f3c94",
)
AFTER_PLANES = (
    "0ec506a07d3401d81dea67faab4f6cf010deaf4bb1e553a65b37af4979a637a3",
    "8f30a8d5aae950801d86fa8fb0fe79d03afda53453cd0f5df7cbf1c9c52cfcee",
    BEFORE_PLANES[2],
    "53e7cf6bbc9f0c566a67503ce63806a2ade4c975a8476f829a1fcf44c7d9e8b8",
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
        plane = (run / f"page-{phase}-plane{lane}.bin").read_bytes()
        if len(plane) != 3520:
            raise ValueError(f"{phase} EGA plane size changed")
        check_hash(plane, digest, f"{phase} EGA plane {lane}")
        planes.append(plane)
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
    near = image[0x13C8:0x13CB]
    if near[0] != 0xE8 or 0x082B + struct.unpack_from("<h", near, 1)[0] != 0x1816:
        raise ValueError("original 0828 callee changed")
    far = image[0x23F3:0x23F8]
    if far[0] != 0x9A or struct.unpack_from("<HH", far, 1) != (0x0626, 0x0709):
        raise ValueError("original 1853 callee changed")

    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    anchor = ram["ram-row7-08ba-private.bin"]
    entry = ram["ram-next-printer-entry-private.bin"]
    if word(anchor, 0x1D160 + 0x3EBC) != 0x1107:
        raise ValueError("row-seven anchor frame changed")
    for name, (seg, start, end, digest) in RUNTIME_SPANS.items():
        check_hash(entry[seg * 16 + start:seg * 16 + end], digest,
                   f"loaded {name}")
    loaded_far = entry[0x08DE0 + 0x1853:0x08DE0 + 0x1858]
    if loaded_far[0] != 0x9A or struct.unpack_from("<HH", loaded_far, 1) != (
            0x0626, 0x0F2D):
        raise ValueError("loaded printer target changed")
    if struct.unpack_from("<8H", entry, 0x1D160 + 0x3EA4) != (
            0x3EC2, 0x082B, 0x08DE, 0, 8, 17, 8, 0x4513):
        raise ValueError("1816 parent frame changed")
    ret_ip, ret_cs, src_off, src_seg, color, row, col = struct.unpack_from(
        "<7H", entry, 0x1D160 + 0x3E92)
    if (ret_ip, ret_cs, src_off, src_seg, color, row, col) != (
            0x1858, 0x08DE, 8, 0x4513, 11, 8, 17):
        raise ValueError("printer entry frame changed")
    source_at = src_seg * 16 + src_off
    length = entry[source_at]
    if length != 8 or entry[source_at + 0x10D:source_at + 0x10F] != bytes((1, 0)):
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

    anchor_page = page(run, "row7-08ba", BEFORE_PLANES, BEFORE_SHA)
    before = page(run, "next-entry", BEFORE_PLANES, BEFORE_SHA)
    after = page(run, "next-return", AFTER_PLANES, AFTER_SHA)
    final = page(run, "first-view", FINAL_PLANES, FINAL_SHA)
    if anchor_page != before:
        raise ValueError("page changed before next printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {
        lane * 3520 + scanline * 40 + col - 0x3C0
        for lane in (0, 1, 3) for scanline in range(64, 71)
        for col in range(17, 25)
    }
    if changed != expected or len(changed) != 168:
        raise ValueError("eight-cell immediate page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 203 or changed & later or len(total) != 371 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "row8-probe-input-private.bin"
    private_output = run / "row8-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              entry[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private C probe input size changed")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=8 stores=256 changed=168":
        raise ValueError("C probe report changed")
    if private_output.read_bytes() != after:
        raise ValueError("C probe differs from native immediate page")

    print("RES_00BA:0828 -> 1816 -> 1853 -> RES_0709:0626 -> 1858")
    print("row 8 columns 17-24; eight cells; color 11; 256 stores; 168 changed bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FINAL_SHA)
    print("168/168 changed bytes persist; 203 later net bytes remain")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
