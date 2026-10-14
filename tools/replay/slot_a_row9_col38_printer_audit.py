#!/usr/bin/env python3
"""Hash-locked private audit of the saved slot-A row9 col38 printer call.

Only addresses, hashes, and counts are emitted. Media and page payloads stay
under ignored resources/.
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
SOURCE_SHA = "17333b15b4a5afd16cac55a104b554fc63cc8731c76249dddce5ec3363fe4f38"
BEFORE_SHA = "4abd88a43973afe33f43c772d6e7988cf83da3106159d802490f14db73dd32d6"
AFTER_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"
FINAL_SHA = "d35779b30d1c0ec4df4339d30858847c354f280d49adce91ded3524437b5354b"

ORIGINAL_SPANS = {
    "RES_00BA:08A0..08BA caller context": (0x1440, 0x145A,
        "056fc9fc372c8e544e5fe890d6b728b7859e9444fee14e204127079a1e137ec1"),
    "RES_00BA:08B7 near call": (0x1457, 0x145A,
        "e72bebba602f7eb4edd3783632f297e7b0bf432aa0b0438a34908168cce490a4"),
    "RES_00BA:0973 body": (0x1513, 0x1569,
        "a5093db76117998bf62e1d6a3b0ca1816f6109dd7d3bca570070a09c0f73ebd0"),
    "RES_00BA:09BB near format call": (0x155B, 0x155E,
        "0924e790e7d93bf418508fcbf8f4043c33b243a114d0dd47f665962626764f53"),
    "RES_00BA:09BE far printer call": (0x155E, 0x1563,
        "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"),
    "RES_0709:0626 printer body": (0x76B6, 0x7737, PRINTER_IMAGE_SHA),
}

RUNTIME_SPANS = {
    "08DE:08A0..08BA caller context": (0x08DE, 0x08A0, 0x08BA,
        ORIGINAL_SPANS["RES_00BA:08A0..08BA caller context"][2]),
    "08DE:08B7 near call": (0x08DE, 0x08B7, 0x08BA,
        ORIGINAL_SPANS["RES_00BA:08B7 near call"][2]),
    "08DE:0973 body": (0x08DE, 0x0973, 0x09C9,
        "c7577505151147a243592d02b42335233680a27273dc5df3f427d4e778a3c3d3"),
    "08DE:09BB near format call": (0x08DE, 0x09BB, 0x09BE,
        ORIGINAL_SPANS["RES_00BA:09BB near format call"][2]),
    "08DE:09BE far printer call": (0x08DE, 0x09BE, 0x09C3,
        "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"),
    "0F2D:0626 printer body": (0x0F2D, 0x0626, 0x06A7, PRINTER_RUNTIME_SHA),
}

RAM_SHA = {
    "ram-row9-col34-return-private.bin":
        "ea51cd7c6aaaff1b88a694622048f15a41f92d2d95d65da776aa4ef673702b13",
    "ram-next-printer-entry-private.bin":
        "b579c605a17505397d391eb3a12053067375e955119f034a77583aa736562532",
    "ram-next-printer-return-private.bin":
        "212d4eed4fc238e0887f52c94a7c2fae32ce855c69d456f483915fcc04d95182",
    "ram-first-view-private.bin":
        "62f29f96b7261dea7b3b6690940b94490df5df42e87ecabd23a8a057da9f8aa2",
}

BEFORE_PLANES = (
    "7547aaae3e11da15f796c248cdfc4062660a1e26217d5ed0fb4343cec0db95eb",
    "2b2f0c82a3a1edf11fba84d21499c3b17ccdd86e3efe6661f2bf7473f1a313ec",
    "f71ecd9f2d3d99968ef9d9cb0d89ad7d7517ae87f1b2c085c66782273d8a4196",
    "77688b45ee62f4259100b47c62eeb9d6d432c85e85279a8c12fc6b8dcef04dd4",
)
AFTER_PLANES = (
    BEFORE_PLANES[0],
    "68e322254f2ddd3d4d2721eb1ba5057ea439b46dffbd54b387c8c85183531c08",
    BEFORE_PLANES[2],
    "979c2807936ad31d8143f134150ee10fc7f95e4d2bdf5e89aee08a641c363a37",
)
FINAL_PLANES = AFTER_PLANES


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
    for name, (start, end, digest) in ORIGINAL_SPANS.items():
        check_hash(image[start:end], digest, f"original {name}")
    for at, ret, target in ((0x1457, 0x08BA, 0x0973),
                            (0x155B, 0x09BE, 0x1037)):
        insn = image[at:at + 3]
        if insn[0] != 0xE8 or ret + struct.unpack_from("<h", insn, 1)[0] != target:
            raise ValueError("original near call target changed")
    far = image[0x155E:0x1563]
    if far[0] != 0x9A or struct.unpack_from("<HH", far, 1) != (0x0626, 0x0709):
        raise ValueError("original far printer target changed")

    ram = {}
    for name, digest in RAM_SHA.items():
        data = (run / name).read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{name} RAM size changed")
        check_hash(data, digest, name)
        ram[name] = data
    entry = ram["ram-next-printer-entry-private.bin"]
    for name, (seg, start, end, digest) in RUNTIME_SPANS.items():
        check_hash(entry[seg * 16 + start:seg * 16 + end], digest,
                   f"loaded {name}")
    loaded_far = entry[0x08DE0 + 0x09BE:0x08DE0 + 0x09C3]
    if loaded_far[0] != 0x9A or struct.unpack_from("<HH", loaded_far, 1) != (
            0x0626, 0x0F2D):
        raise ValueError("loaded printer target changed")
    if struct.unpack_from("<7H", ram["ram-row9-col34-return-private.bin"],
                          0x1D160 + 0x3EA6) != (
            0x3EC2, 0x0878, 0x08DE, 9, 34, 8, 0x4529):
        raise ValueError("row9 col34 anchor frame changed")
    if struct.unpack_from("<8H", entry, 0x1D160 + 0x3EA4) != (
            0x3EC2, 0x08BA, 0x08DE, 0, 9, 38, 8, 0x4529):
        raise ValueError("0973 parent frame changed")
    ret_ip, ret_cs, src_off, src_seg, color, row, col = struct.unpack_from(
        "<7H", entry, 0x1D160 + 0x3D94)
    if (ret_ip, ret_cs, src_off, src_seg, color, row, col) != (
            0x09C3, 0x08DE, 0x3DA2, 0x1D16, 10, 9, 38):
        raise ValueError("printer entry frame changed")
    node = 0x45290 + 8
    if (entry[node + 0x11B], entry[node + 0x32],
            word(entry, 0x1D160 + 0x3EAA)) != (6, 6, 0):
        raise ValueError("0973 color branch inputs changed")
    source_at = src_seg * 16 + src_off
    length = entry[source_at]
    if length != 1:
        raise ValueError("private Pascal source length changed")
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

    anchor = page(run, "row9-col34-return", BEFORE_PLANES, BEFORE_SHA)
    before = page(run, "next-entry", BEFORE_PLANES, BEFORE_SHA)
    after = page(run, "next-return", AFTER_PLANES, AFTER_SHA)
    final = page(run, "first-view", FINAL_PLANES, FINAL_SHA)
    if anchor != before:
        raise ValueError("page changed before next printer entry")
    changed = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    expected = {
        lane * 3520 + scanline * 40 + 38 - 0x3C0
        for lane in (1, 3) for scanline in range(72, 79)
    }
    if changed != expected or len(changed) != 14:
        raise ValueError("one-cell immediate page delta changed")
    later = {i for i, (a, b) in enumerate(zip(after, final)) if a != b}
    total = {i for i, (a, b) in enumerate(zip(before, final)) if a != b}
    if (len(later) != 0 or len(total) != 14 or
            total != changed | later):
        raise ValueError("first-view persistence or later delta changed")

    private_input = run / "row9-col38-probe-input-private.bin"
    private_output = run / "row9-col38-probe-output-private.bin"
    record = (font + bytes((length, col, row, color)) +
              entry[source_at + 1:source_at + 1 + length] +
              bytes(255 - length) + before)
    if len(record) != 1416 + 4 + 255 + 14080:
        raise ValueError("private C probe input size changed")
    private_input.write_bytes(record)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=1 stores=32 changed=14":
        raise ValueError("C probe report changed")
    if private_output.read_bytes() != after:
        raise ValueError("C probe differs from native immediate page")

    print("RES_00BA:08B7 -> 0973 -> 09BB -> 1037 -> 09BE -> RES_0709:0626")
    print("row 9 column 38; one cell; color 10; 32 stores; 14 changed bytes")
    print("entry page SHA-256", BEFORE_SHA)
    print("immediate page SHA-256", AFTER_SHA)
    print("same-run first-view SHA-256", FINAL_SHA)
    print("14/14 changed bytes persist; zero later net page bytes")
    print("C projection 14080/14080 native immediate page bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
