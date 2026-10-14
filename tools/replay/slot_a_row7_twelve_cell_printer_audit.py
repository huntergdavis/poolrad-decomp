#!/usr/bin/env python3
"""Audit the saved slot-A row-seven printer call using ignored native captures.

Only addresses, hashes, and counts are printed. The DOS media, guest RAM,
EGA planes, Pascal source, font, and probe I/O stay under ignored resources/.
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
SOURCE_SHA = "b7be7afb7653c61771446859f1043ab2cd56400f35d7a097ad5dd62271f7f9c5"
BEFORE_SHA = "a6419841fb5aa3e5a40a401c081b69be718b692af57dd3a3d29b58e03ed8158d"
AFTER_SHA = "bd641160cb016bb6ff8ec5043235c972ae829e910a589afc3618ea7b7ab34f6f"
FIRST_VIEW_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

ORIGINAL_SPANS = {
    "RES_00BA:07EA far call": (0x138A, 0x138F,
        "395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f"),
    "RES_03F1:0137 body": (0x4047, 0x41F8,
        "1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc"),
    "RES_00BA:0828 near call": (0x13C8, 0x13CB,
        "101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3"),
    "RES_00BA:1816 body": (0x23B6, 0x2492,
        "47a999a7d5c19202877fb35003099aab45dac77ce4bb95e93d132020a2373918"),
    "RES_00BA:1853 far call": (0x23F3, 0x23F8,
        "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    "RES_0709:0626 body": (0x76B6, 0x7737, PRINTER_IMAGE_SHA),
}

RUNTIME_SPANS = {
    "08DE:07EA far call": (0x08DE, 0x07EA, 0x07EF,
        "2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850"),
    "0C15:0137 body": (0x0C15, 0x0137, 0x02E8,
        "f50a7fde3dcc5886f55beb81c5fcdc22efe4348b52d8df35c70a646d5a309d5a"),
    "08DE:0828 near call": (0x08DE, 0x0828, 0x082B,
        "101742b95d662bb42996389a5744d3371fb252318a3e91c03de2d6e0f01469d3"),
    "08DE:1816 body": (0x08DE, 0x1816, 0x18F2,
        "35cdb1f71e2bc52f5f2d377d647594481f50921a7c09a0aed1b01057aa6087ac"),
    "08DE:1853 far call": (0x08DE, 0x1853, 0x1858,
        "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    "0F2D:0626 body": (0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
}

RAM_SHA = {
    "ram-row7-07ea-pre.bin": "1236d97e2a334b2c23bebecc0bbe318df795384bae0bc630bec1c52bd9310d9c",
    "ram-row7-callee-entry.bin": "42fdde525fbec15e00c682a3204aab33c5dfa71a0df076a1f216f7f637c741ae",
    "ram-row7-callee-return.bin": "bfedf281d5a6dabda6eb2c700162e8cac18002ddd90680f141baac0c3db075da",
    "ram-row7-1816-call.bin": "73165ffa7fa9a89cefb319e1f4f4215908d9c9e6eb9b4f22d48adac2d401d3fa",
    "ram-row7-1816-entry.bin": "790292fb2ca764c8f555786b31d33ae88a39ae2ec1cf109ca5dcb7ee390f2324",
    "ram-row7-printer-entry.bin": "ad1a552c9bc8dec782ab1bc40f76ed0a1da1c934c29b8d129152fff77ce6d02a",
    "ram-row7-printer-return.bin": "2f22e8d1871f91499ccdd25ed02aaba56b41eb39699b6de986c365e1229840d1",
}

PRE_PLANE_SHA = (
    "d78a1c5ae193c6a5e21521f5f334a0bc9e4bd5008a1e167e482a1a8701de1ca7",
    "a44436ba43bbf1cbc6bd1bc6b0e99a2ee2ede204fe2bf0aa2a2a96439cee03b6",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "7ac34a17671de9374c1c5cf9ed012707789cff5ab42b579218527ed9497561c5",
)
POST_PLANE_SHA = (
    "6a33ada38659de42223137dc19137d392e8be25f3e42bfc6e96a141609559481",
    "d2ee66324a82c53f43ba526c679dc3af46ec8cb5c834d74b43b954b40bfe2501",
    PRE_PLANE_SHA[2],
    "ea05a4be8e638b33d27b9653e982b5d718b931eca3df9ceceaad655e4b32593f",
)
FIRST_VIEW_PLANE_SHA = (
    "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
    "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
    PRE_PLANE_SHA[2],
    "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
)


def page(run: Path, phase: str, hashes: tuple[str, ...], expected: str) -> bytes:
    prefix = "page-first-view-third" if phase == "first-view" else f"page-row7-{phase}"
    planes = []
    for lane, digest in enumerate(hashes):
        plane = (run / f"{prefix}-plane{lane}.bin").read_bytes()
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
    original_callee = image[0x4047:0x41F8]
    if original_callee[-3:] != b"\xCA\x08\x00":
        raise ValueError("RES_03F1:0137 return bound changed")
    if (image[0x138A] != 0x9A or
            struct.unpack_from("<HH", image, 0x138B) != (0x0137, 0x03F1)):
        raise ValueError("original 07EA callee changed")
    near = image[0x13C8:0x13CB]
    if near[0] != 0xE8 or 0x082B + struct.unpack_from("<h", near, 1)[0] != 0x1816:
        raise ValueError("original 0828 callee changed")

    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    pre = ram["ram-row7-07ea-pre.bin"]
    printer = ram["ram-row7-printer-entry.bin"]
    for name, (seg, start, end, digest) in RUNTIME_SPANS.items():
        check_hash(printer[seg * 16 + start:seg * 16 + end], digest,
                   f"loaded {name}")
    if (struct.unpack_from("<HH", printer, 0x08DE0 + 0x07EB) !=
            (0x0137, 0x0C15) or
            struct.unpack_from("<HH", printer, 0x08DE0 + 0x1854) !=
            (0x0626, 0x0F2D)):
        raise ValueError("loaded far-call target changed")
    if (pre[0x1D160 + 0x3EC2 - 6], pre[0x1D160 + 0x3EC2 - 5]) != (7, 17):
        raise ValueError("07EA caller row/column changed")
    ss = 0x1D160
    callee_frame = struct.unpack_from("<6H", ram["ram-row7-callee-entry.bin"],
                                      ss + 0x3EA8)
    if callee_frame[:2] != (0x07EF, 0x08DE) or [x & 0xFF for x in callee_frame[2:]] != [7, 38, 7, 17]:
        raise ValueError("03F1:0137 entry frame changed")
    display_frame = struct.unpack_from("<7H", ram["ram-row7-1816-entry.bin"],
                                       ss + 0x3EA6)
    if display_frame != (0x082B, 0x08DE, 0, 7, 17, 8, 0x44F5):
        raise ValueError("1816 entry frame changed")
    ret_ip, ret_cs, source_off, source_seg, color, row, col = struct.unpack_from(
        "<7H", printer, ss + 0x3E92)
    if (ret_ip, ret_cs, source_off, source_seg, color, row, col) != (
            0x1858, 0x08DE, 8, 0x44F5, 11, 7, 17):
        raise ValueError("nested printer entry frame changed")
    source_at = source_seg * 16 + source_off
    length = printer[source_at]
    if length != 12:
        raise ValueError("Pascal source length changed")
    check_hash(printer[source_at:source_at + 1 + length], SOURCE_SHA,
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

    before = page(run, "07ea-pre", PRE_PLANE_SHA, BEFORE_SHA)
    after_callee = page(run, "07ef-post", PRE_PLANE_SHA, BEFORE_SHA)
    display_entry = page(run, "1816-pre", PRE_PLANE_SHA, BEFORE_SHA)
    after_printer = page(run, "printer-return", POST_PLANE_SHA, AFTER_SHA)
    after_display = page(run, "082b-return", POST_PLANE_SHA, AFTER_SHA)
    first_view = page(run, "first-view", FIRST_VIEW_PLANE_SHA, FIRST_VIEW_SHA)
    if before != after_callee or before != display_entry:
        raise ValueError("page changed before row-seven printer")
    if after_printer != after_display:
        raise ValueError("1816 changed page after nested printer return")
    changed = {i for i, (a, b) in enumerate(zip(before, after_printer)) if a != b}
    expected = {
        lane * 3520 + ((7 * 8 + scanline) * 40 + col) - 0x3C0
        for lane in (0, 1, 3)
        for scanline in range(7)
        for col in (*range(17, 24), *range(25, 29))
    }
    if changed != expected or len(changed) != 231:
        raise ValueError("row-seven changed-byte set changed")
    if any(first_view[i] != after_printer[i] for i in changed):
        raise ValueError("row-seven printer pixels did not persist")
    if sum(a != b for a, b in zip(after_printer, first_view)) != 413:
        raise ValueError("later first-view page delta changed")

    private_input = run / "row7-audit-probe-input-private.bin"
    private_output = run / "row7-audit-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              printer[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private C probe input size changed")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=12 stores=384 changed=231":
        raise ValueError("C probe report changed")
    modeled = private_output.read_bytes()
    if modeled != after_printer:
        raise ValueError("C probe differs from native immediate page")

    print("RES_00BA:07EA -> RES_03F1:0137 -> 07EF: zero sampled page changes")
    print("RES_00BA:0828 -> 1816 -> RES_0709:0626 -> 1858 -> 082B")
    print("row 7 column 17; 12 cells; color 11; 384 stores; 231 changed page bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FIRST_VIEW_SHA)
    print("231/231 changed bytes persist; 413 later net bytes remain")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
