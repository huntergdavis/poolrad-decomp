#!/usr/bin/env python3
"""Audit one native post-Up printer call against the existing C page model.

All media, RAM, font, source text, and EGA data stay under ignored resources/.
Public output contains only addresses, hashes, and counts.
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
EMULATOR_SHA = "c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457"
ENTRY_RAM_SHA = "5c3aa481884391318a14a9cfe4e76b1446027b72b368065df606c2a37c5c0c23"
RETURN_RAM_SHA = "24b7c6335e01ec918c823935a52342278ddb81c3150e06d67d07b10dcc63d1e8"
ENTRY_PAGE_SHA = "b0e709378656a19cf2d35886ebe6193808ab9fc8747b2f813cae43a9a1bb0f44"
RETURN_PAGE_SHA = "a8f4e1857f4f9db3af55fa0581a7c22601ad2d04c04a82007788d1d14df845ac"
ENTRY_PLANES = (
    "15271c33fcee318327fd238aa423b9317410f61c217bac55af2bbf21cd16e1c4",
    "e85f6e37afae7a09aa580c274642222aeb071415f7e14e98af5cace31b7ee64f",
    "f96d5f3a995943f4e58f70f03d72ab44714ad8b2cd0987f01eb5f1f4fe28dcce",
    "d34da7cd6a7af447acdd93f48a289c4646b1926fe24ac41d0dc175ac709d3c84",
)
RETURN_PLANES = (
    ENTRY_PLANES[0],
    "f64094c6eae3e506c8c0217b55d092013aed86040a6903a23e644bc5c3f3979e",
    ENTRY_PLANES[2],
    "0f52c2fc90ea411ec434d06e18360d14b671aa0dbf1b3820126c2dbd3c10e746",
)
FRAME = (0x2B5A, 0x08DE, 0x3E7D, 0x1D16, 0xA00A, 0xA00F, 0xA011)
SOURCE_SHA = "e767d5c8ef552436c5e411af72d8eca9581f3feacbb988c67c16bf2b701bc2fe"
CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
LOADED_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
PROBE_INPUT_SHA = "53fd15433e954797e4b2ef165d7eac15c0accea3db040d956bfe9dedf2042318"


def validate_frame(ram: bytes) -> tuple[int, int, int, int, int]:
    if len(ram) != 0xA0000:
        raise ValueError("entry RAM size changed")
    if struct.unpack_from("<7H", ram, 0x1D160 + 0x3C6E) != FRAME:
        raise ValueError("post-Up printer frame changed")
    _, _, src_off, src_seg, color, row, col = FRAME
    source_at = src_seg * 16 + src_off
    length = ram[source_at]
    if length != 19:
        raise ValueError("post-Up Pascal source length changed")
    check_hash(ram[source_at:source_at + length + 1], SOURCE_SHA,
               "private post-Up source")
    ds = 0x14A00
    font_at = word(ram, ds + 0x5E20) + 16 * word(ram, ds + 0x5E22)
    if (word(ram, ds + 0x5E20), word(ram, ds + 0x5E22)) != (0, 0x3CB6):
        raise ValueError("post-Up font far pointer changed")
    check_hash(ram[font_at:font_at + 1416], FONT_SHA, "live post-Up font")
    if (word(ram, ds + 0x4A16), word(ram, ds + 0x4A18),
            ram[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
        raise ValueError("post-Up EGA page mode changed")
    return source_at, font_at, length, col & 255, row & 255


def validate_delta(before: bytes, after: bytes) -> int:
    if len(before) != 32000 or len(after) != 32000:
        raise ValueError("four-plane EGA page size changed")
    changed = {index for index, (left, right) in enumerate(zip(before, after))
               if left != right}
    possible = {plane * 8000 + line * 40 + column
                for plane in (1, 3) for line in range(120, 128)
                for column in range(17, 36)}
    if len(changed) != 210 or not changed <= possible:
        raise ValueError("post-Up row-15 page delta changed")
    if sum(8000 <= index < 16000 for index in changed) != 105 or \
            sum(24000 <= index < 32000 for index in changed) != 105:
        raise ValueError("post-Up plane delta distribution changed")
    return len(changed)


def page(run: Path, phase: str) -> bytes:
    hashes = ENTRY_PLANES if phase == "entry" else RETURN_PLANES
    planes = []
    for plane, digest in enumerate(hashes):
        data = (run / f"first-printer-{phase}-plane{plane}-private.bin").read_bytes()
        if len(data) != 8000:
            raise ValueError(f"{phase} plane {plane} size changed")
        check_hash(data, digest, f"{phase} plane {plane}")
        planes.append(data)
    result = b"".join(planes)
    check_hash(result, ENTRY_PAGE_SHA if phase == "entry" else RETURN_PAGE_SHA,
               f"{phase} full page")
    return result


def audit(exe_path: Path, emulator_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), SAVE_SHA,
               "slot-A save")
    check_hash(emulator_path.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")
    entry = (run / "first-printer-entry-private.ram").read_bytes()
    returned = (run / "first-printer-return-private.ram").read_bytes()
    check_hash(entry, ENTRY_RAM_SHA, "entry RAM")
    check_hash(returned, RETURN_RAM_SHA, "return RAM")
    if len(returned) != 0xA0000:
        raise ValueError("return RAM size changed")
    image, *_ = unpack(exe)
    original_call = image[0x36F5:0x36FA]
    loaded_call = entry[0x08DE0 + 0x2B55:0x08DE0 + 0x2B5A]
    check_hash(original_call, CALL_SHA, "original 00BA:2B55 call")
    check_hash(loaded_call, LOADED_CALL_SHA, "loaded 08DE:2B55 call")
    if original_call[0] != 0x9A or \
            struct.unpack_from("<HH", original_call, 1) != (0x0626, 0x0709) or \
            loaded_call[0] != 0x9A or \
            struct.unpack_from("<HH", loaded_call, 1) != (0x0626, 0x0F2D):
        raise ValueError("post-Up far-call target changed")
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA,
               "original RES_0709:0626 body")
    check_hash(entry[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
               PRINTER_RUNTIME_SHA, "loaded printer body")
    source_at, font_at, length, col, row = validate_frame(entry)
    before = page(run, "entry")
    after = page(run, "return")
    validate_delta(before, after)
    private_input = run / "first-printer-audit-input-private.bin"
    private_output = run / "first-printer-audit-output-private.bin"
    private_input.write_bytes(
        entry[font_at:font_at + 1416] + bytes((length, col, row, FRAME[4] & 255)) +
        entry[source_at + 1:source_at + length + 1] + bytes(255 - length) + before)
    check_hash(private_input.read_bytes(), PROBE_INPUT_SHA, "private C probe input")
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != "cells=19 stores=608 changed=210":
        raise ValueError(f"post-Up C report changed: {result.stdout!r}")
    if private_output.read_bytes() != after:
        raise ValueError("post-Up C page differs from immediate native return")
    print("first of three armed UI entries after one Up: 0F2D:0626")
    print("caller 08DE:2B55; return 08DE:2B5A; frame 1D16:3C6E")
    print("row/column/length/color: 15/17/19/10")
    print("entry page SHA-256", ENTRY_PAGE_SHA)
    print("return page SHA-256", RETURN_PAGE_SHA)
    print("210 changed values, planes 1/3, global scanlines 120-127")
    print("C immediate page: 32000/32000 native return bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
