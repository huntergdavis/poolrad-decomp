#!/usr/bin/env python3
"""Audit the saved slot-A row-seven two-cell printer after column 34.

Only addresses, hashes, and counts are printed. DOS media, guest RAM,
source, font, EGA planes, and C probe records stay under ignored resources/.
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
SOURCE_SHA = "69f57e18a3f92716d6ead17fcc66a626dddade0965bbde25f5f7accebd719a30"
BEFORE_SHA = "1dccb5ff277e6a49bbeb8392f4faa000ab7f71357caf87e9d7cc852b8eef2574"
AFTER_SHA = "99ce3c3496cf59d0f0e43b67ac66d03ad0e09af30168d19ea0bf55877500cbc0"
FINAL_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

ORIGINAL_SPANS = {
    "RES_00BA:0878..08BA caller": (0x1418, 0x145A,
        "a0912afb2db94228642cb606dc750911d43961fc928e5549d0c19c8a3c3ae95a"),
    "RES_00BA:08B7 near call": (0x1457, 0x145A,
        "e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4"),
    "RES_00BA:0973 body": (0x1513, 0x1569,
        "a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0"),
    "RES_00BA:09BE far call": (0x155E, 0x1563,
        "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    "RES_0709:0626 body": (0x76B6, 0x7737, PRINTER_IMAGE_SHA),
}

RUNTIME_SPANS = {
    "08DE:0878..08BA caller": (0x08DE, 0x0878, 0x08BA,
        "a0912afb2db94228642cb606dc750911d43961fc928e5549d0c19c8a3c3ae95a"),
    "08DE:08B7 near call": (0x08DE, 0x08B7, 0x08BA,
        "e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4"),
    "08DE:0973 body": (0x08DE, 0x0973, 0x09C9,
        "c7577505151147a243592d02b42335233680a27273dc5df3f427d4e778a3c3d3"),
    "08DE:09BE far call": (0x08DE, 0x09BE, 0x09C3,
        "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    "0F2D:0626 body": (0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
}

RAM_SHA = {
    "ram-row7-0878-private.bin": "c48523298e51483f88ccbc89e906728a4b34029c183ec95482c1431f9560ae81",
    "ram-row7-08b7-private.bin": "4168592fa43cc2914a45847c91ac1c9b0d2f5161bd84534a11f8368f23579dd9",
    "ram-row7-0973-entry-private.bin": "2cc5f3821f4d0b7ba8deba9fc6e48fb318d724dfec03ea95921f66a739694f92",
    "ram-row7-09be-private.bin": "b6f345b8c10b8cfe7602da5e14a12f6f6489d64d3eae650ff4651497e583a1e2",
    "ram-row7-next-printer-entry-private.bin": "9156b5baccfcfc17b1dff59eec027c5643c822526fbe98e9f198e3c8a889c7f6",
    "ram-row7-next-printer-return-private.bin": "74aa9568f9881eb9f03ea4290760adb159b6e2a70f8122d3e6d00872af1ffe76",
}

BEFORE_PLANES = (
    "6a33ada38659de42223137dc19137d392e8be25f3e42bfc6e96a141609559481",
    "03e1e4ba047085cfa9ef60a7553840a40b70c45190306b14e186805c646e1555",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "3839e3ac059707cbe57c2228dec73e9a297cef997e783510d1b53f0f6fdcf873",
)
AFTER_PLANES = (
    BEFORE_PLANES[0],
    "a4062ba707d1f1aec7ec67d41273add61e541730e87e72ba1a66e49874e9159c",
    BEFORE_PLANES[2],
    "0a07bc1e8e99a1210c423308db6bdea61c8f7af42406b7483777966e1e1f3c94",
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
        plane = (run / f"{phase}-plane{lane}.bin").read_bytes()
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
    near = image[0x1457:0x145A]
    if near[0] != 0xE8 or 0x08BA + struct.unpack_from("<h", near, 1)[0] != 0x0973:
        raise ValueError("original 08B7 callee changed")
    far = image[0x155E:0x1563]
    if far[0] != 0x9A or struct.unpack_from("<HH", far, 1) != (0x0626, 0x0709):
        raise ValueError("original 09BE callee changed")
    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    anchor = ram["ram-row7-0878-private.bin"]
    field = ram["ram-row7-0973-entry-private.bin"]
    printer = ram["ram-row7-next-printer-entry-private.bin"]
    for name, (seg, start, end, digest) in RUNTIME_SPANS.items():
        check_hash(printer[seg * 16 + start:seg * 16 + end], digest,
                   f"loaded {name}")
    loaded_far = printer[0x08DE0 + 0x09BE:0x08DE0 + 0x09C3]
    if loaded_far[0] != 0x9A or struct.unpack_from("<HH", loaded_far, 1) != (
            0x0626, 0x0F2D):
        raise ValueError("loaded 09BE callee changed")

    ss = 0x1D160
    if (anchor[ss + 0x3EBC:ss + 0x3EBE] != bytes((7, 17)) or
            struct.unpack_from("<2H", anchor, ss + 0x3EBE) != (8, 0x44F5)):
        raise ValueError("0878 loop frame changed")
    node_at = 0x44F50 + 8
    if anchor[node_at + 0x11B] != 0x0A:
        raise ValueError("row-seven branch input changed")
    field_frame = struct.unpack_from("<7H", field, ss + 0x3EA6)
    if field_frame != (0x08BA, 0x08DE, 0, 7, 0x25, 8, 0x44F5):
        raise ValueError("0973 field-display frame changed")
    ret_ip, ret_cs, source_off, source_seg, color, row, col = struct.unpack_from(
        "<7H", printer, ss + 0x3D94)
    if (ret_ip, ret_cs, source_off, source_seg, color, row, col) != (
            0x09C3, 0x08DE, 0x3DA2, 0x1D16, 10, 7, 37):
        raise ValueError("printer entry frame changed")
    source_at = source_seg * 16 + source_off
    length = printer[source_at]
    if length != 2:
        raise ValueError("Pascal source length changed")
    check_hash(printer[source_at:source_at + length + 1], SOURCE_SHA,
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

    baseline = page(run, "page-row7-0878", BEFORE_PLANES, BEFORE_SHA)
    caller = page(run, "page-row7-08b7", BEFORE_PLANES, BEFORE_SHA)
    before = page(run, "page-row7-09be", BEFORE_PLANES, BEFORE_SHA)
    after = page(run, "page-row7-09c3", AFTER_PLANES, AFTER_SHA)
    parent = page(run, "page-row7-08ba", AFTER_PLANES, AFTER_SHA)
    final = page(run, "page-first-view", FINAL_PLANES, FINAL_SHA)
    if baseline != caller or baseline != before or after != parent:
        raise ValueError("caller page phase changed")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {
        lane * 3520 + (7 * 8 + scanline) * 40 + col - 0x3C0
        for lane in (1, 3) for scanline in range(7) for col in (37, 38)
    }
    if changed != expected or len(changed) != 28:
        raise ValueError("two-cell immediate delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 371 or changed & later or len(total) != 399 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "next-audit-probe-input-private.bin"
    private_output = run / "next-audit-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              printer[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private C probe input size changed")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=2 stores=64 changed=28":
        raise ValueError("C probe report changed")
    if private_output.read_bytes() != after:
        raise ValueError("C probe differs from native immediate page")

    print("RES_00BA:08B7 -> 0973 -> 09BE -> RES_0709:0626 -> 09C3 -> 08BA")
    print("row 7 columns 37-38; two cells; color 10; 64 stores; 28 changed bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FINAL_SHA)
    print("28/28 changed bytes persist; 371 later net bytes remain")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
