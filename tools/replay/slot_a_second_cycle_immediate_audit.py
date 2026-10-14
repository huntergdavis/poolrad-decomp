#!/usr/bin/env python3
"""Audit saved-slot-A second-cycle printer returns and first row clear.

All RAM, font, text, and EGA page payloads remain under ignored resources/.
The C printer probe starts afresh from each measured native entry page. This
does not compose a C first-view frame across non-printer effects.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import re
import struct
import subprocess

from slot_a_printer_page_audit import (
    FONT_SHA, PRINTER_IMAGE_SHA, PRINTER_RUNTIME_SHA, START_SHA, unpack,
)

GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
RAM_SET_SHA = "893b7f88ce2f9983264e99c0008842b7095bd97e6538a6789c3e43164f09c0c5"
PAGE_SET_SHA = "650fde0e71c40848f90d50f48fe2738d1049a333a3b244f93fc174c1794a9748"
BOUNDARY_SET_SHA = "dd99ce02cf7024ecc30a514cbd197c410f4dc441515b8cfee44c784abe08e873"
FIRST_CYCLE_END_SHA = "3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738"
SECOND_CYCLE_START_SHA = "2aa65739f6bc280936e6203d6bb6cbaad26663475ee86e51468d48a4b7b4bfdf"
FIRST_CLEAR_ENTRY_SHA = "570ad46e976182f1227b76bdb367fa0303bd789c42bfa095770a74cb5f7aa52f"
WATCH_RAM_SHA = "be3ece5652d1746b146b42bb69d9bb882330be4ad69c4d8f3741d026ca4821f2"
FILL_RUNTIME_SHA = "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"
RECT_IMAGE_SHA = "f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5"
RECT_RUNTIME_SHA = "02ba954ed5bc0baa194fa7e74eeaceba27b09386107ac51df7df27a09c6aaa5f"
OUTER_IMAGE_SHA = "1258f9ea251527a82be56c60ecbd1f07e300c994bc0fdbac2a6537369080f5fc"
OUTER_RUNTIME_SHA = "f50a7fde3dcc5886f55beb81c5fcdc22efe4348b52d8df35c70a646d5a309d5a"
ORIGINAL_CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
LOADED_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
RECT_CALL_IMAGE_SHA = "2286767d9eccf2e3fe83669ef5367f5ef97da8c2145e12d54b0291510e19093c"
RECT_CALL_RUNTIME_SHA = "8d1b1dec8fbef9dea92ee3e851c55572850da6b639cd9fb3890c998e918cc8e2"
OUTER_CALL_IMAGE_SHA = "395783ec4ad75a0552ed076373c516213f522e5aacc77c12d8fbf9ead76dd23f"
OUTER_CALL_RUNTIME_SHA = "2f89ade8cd970b19f81e674324278b7e2f7845bd119dbf92374051d645fc1850"

# Return IP, Pascal string length, color, row, column, SS:SP.
CALLS = (
    (0x0814, 14, 15, 4, 17, 0x3EB0),
    (0x096D, 1, 10, 4, 34, 0x3CA0),
    (0x09C3, 2, 10, 4, 37, 0x3D9E),
    (0x1858, 6, 11, 5, 17, 0x3E9C),
    (0x096D, 1, 10, 5, 34, 0x3CA0),
    (0x09C3, 1, 10, 5, 38, 0x3D9E),
    (0x1858, 8, 11, 6, 17, 0x3E9C),
    (0x096D, 1, 10, 6, 34, 0x3CA0),
    (0x09C3, 1, 10, 6, 38, 0x3D9E),
    (0x1858, 12, 11, 7, 17, 0x3E9C),
    (0x096D, 1, 10, 7, 34, 0x3CA0),
    (0x09C3, 2, 10, 7, 37, 0x3D9E),
    (0x1858, 8, 11, 8, 17, 0x3E9C),
    (0x096D, 1, 10, 8, 34, 0x3CA0),
    (0x09C3, 1, 10, 8, 38, 0x3D9E),
    (0x1858, 7, 11, 9, 17, 0x3E9C),
    (0x096D, 1, 10, 9, 34, 0x3CA0),
    (0x09C3, 1, 10, 9, 38, 0x3D9E),
)
INTERCALL = {27: 154, 30: 196, 33: 273, 36: 196, 39: 175}
CHANGES = (368, 14, 28, 126, 14, 14, 168, 14, 14,
           231, 14, 28, 168, 14, 14, 147, 14, 14)


def check_hash(data: bytes, expected: str, label: str) -> None:
    actual = sha256(data).hexdigest()
    if actual != expected:
        raise ValueError(f"{label} SHA-256 changed: {actual}")


def check_set(paths, expected: str, count: int, size: int, label: str) -> None:
    digest = sha256()
    actual_count = actual_size = 0
    for path in paths:
        data = path.read_bytes()
        digest.update(data)
        actual_count += 1
        actual_size += len(data)
    if (actual_count, actual_size, digest.hexdigest()) != (count, size, expected):
        raise ValueError(f"{label} capture set changed")


def page(run: Path, tag: str) -> bytes:
    result = b"".join((run / f"page-{tag}-plane{lane}.bin").read_bytes()
                      for lane in range(4))
    if len(result) != 32000:
        raise ValueError(f"{tag} page length changed")
    return result


def call_input(ram: bytes, image: bytes, number: int, expected):
    return_ip, length, color, row, column, sp = expected
    words = struct.unpack_from("<7H", ram, 0x1D160 + sp)
    if (words[0], words[1], words[4] & 255, words[5] & 255,
            words[6] & 255) != (return_ip, 0x08DE, color, row, column):
        raise ValueError(f"call {number} frame changed")
    call_site = return_ip - 5
    check_hash(image[0xBA0 + call_site:0xBA5 + call_site],
               ORIGINAL_CALL_SHA, f"call {number} original far call")
    check_hash(ram[0x08DE0 + call_site:0x08DE5 + call_site],
               LOADED_CALL_SHA, f"call {number} loaded far call")
    check_hash(ram[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
               PRINTER_RUNTIME_SHA, f"call {number} loaded printer")
    source_at = words[3] * 16 + words[2]
    if source_at + length + 1 > len(ram) or ram[source_at] != length:
        raise ValueError(f"call {number} source length changed")
    ds = 0x14A00
    font_at = (struct.unpack_from("<H", ram, ds + 0x5E20)[0] +
               16 * struct.unpack_from("<H", ram, ds + 0x5E22)[0])
    font = ram[font_at:font_at + 1416]
    check_hash(font, FONT_SHA, f"call {number} live font")
    if (struct.unpack_from("<HH", ram, ds + 0x4A16) != (0xA000, 0xA000)
            or ram[ds + 0x4A1E] != 1):
        raise ValueError(f"call {number} page mode changed")
    return font, ram[source_at + 1:source_at + 1 + length]


def run_probe(probe: Path, run: Path, number: int, expected,
              font: bytes, source: bytes, entry: bytes):
    _, length, color, row, column, _ = expected
    record = font + bytes((length, column, row, color)) + source
    record += bytes(255 - length) + entry
    inp = run / f"call{number:03d}-independent-input-private.bin"
    out = run / f"call{number:03d}-independent-output-private.bin"
    inp.write_bytes(record)
    report = subprocess.run([str(probe.resolve()), str(inp), str(out)],
                            check=True, capture_output=True, text=True)
    match = re.fullmatch(r"cells=(\d+) stores=(\d+) changed=(\d+)\n?",
                         report.stdout)
    if match is None:
        raise ValueError(f"call {number} C report changed")
    return out.read_bytes(), tuple(map(int, match.groups()))


def check_watch(image: bytes, watch: Path) -> None:
    ram = (watch / "ram-row-clear-watch-private.bin").read_bytes()
    if len(ram) != 0xA0000:
        raise ValueError("watch RAM size changed")
    check_hash(ram, WATCH_RAM_SHA, "first row-clear watch RAM")
    spans = (
        (image, 0x7147, 0x7201, RECT_IMAGE_SHA, "original rectangle"),
        (ram, 0x0F2D0 + 0x00B7, 0x0F2D0 + 0x0171,
         RECT_RUNTIME_SHA, "loaded rectangle"),
        (image, 0x4047, 0x41F8, OUTER_IMAGE_SHA, "original outer caller"),
        (ram, 0x0C150 + 0x0137, 0x0C150 + 0x02E8,
         OUTER_RUNTIME_SHA, "loaded outer caller"),
        (ram, 0x131C0 + 0x16A8, 0x131C0 + 0x16BC,
         FILL_RUNTIME_SHA, "loaded zero fill"),
        (image, 0x4101, 0x4106, RECT_CALL_IMAGE_SHA, "original rectangle call"),
        (ram, 0x0C150 + 0x01F1, 0x0C150 + 0x01F6,
         RECT_CALL_RUNTIME_SHA, "loaded rectangle call"),
        (image, 0x138A, 0x138F, OUTER_CALL_IMAGE_SHA, "original outer call"),
        (ram, 0x08DE0 + 0x07EA, 0x08DE0 + 0x07EF,
         OUTER_CALL_RUNTIME_SHA, "loaded outer call"),
    )
    for data, start, end, expected, label in spans:
        check_hash(data[start:end], expected, label)
    ss = 0x1D160
    bp = 0x3E92
    if struct.unpack_from("<2H", ram, ss + bp + 2) != (0x01F6, 0x0C15):
        raise ValueError("rectangle return frame changed")
    if tuple(ram[ss + bp + i] for i in range(6, 18, 2)) != (
            0, 0, 5, 38, 5, 17):
        raise ValueError("rectangle low-byte arguments changed")
    outer_bp = 0x3EB0
    if struct.unpack_from("<2H", ram, ss + outer_bp + 2) != (
            0x07EF, 0x08DE):
        raise ValueError("outer return frame changed")
    if tuple(ram[ss + outer_bp + i] for i in range(6, 14, 2)) != (
            5, 38, 5, 17):
        raise ValueError("outer low-byte arguments changed")
    if (watch / "watch-pre-byte-private.bin").read_bytes() != bytes((124,)):
        raise ValueError("pre-watch page byte changed")
    if (watch / "watch-post-byte-private.bin").read_bytes() != bytes((0,)):
        raise ValueError("post-watch page byte changed")


def audit(exe: Path, ovr: Path, emulator: Path, run: Path,
          watch: Path, probe: Path) -> None:
    raw = exe.read_bytes()
    check_hash(raw, START_SHA, "START.EXE")
    check_hash(ovr.read_bytes(), GAME_SHA, "GAME.OVR")
    check_hash(emulator.read_bytes(), EMULATOR_SHA, "heavy debugger")
    image, *_ = unpack(raw)
    check_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA,
               "original printer body")
    for capture in (run, watch):
        check_hash((capture / "POOLRAD/SAVGAMA.DAT").read_bytes(),
                   SAVE_SHA, f"{capture.name} slot-A save")
    check_set((run / f"ram-call{n:03d}-entry-private.bin"
               for n in range(25, 43)), RAM_SET_SHA, 18, 18 * 0xA0000,
              "second-cycle RAM")
    check_set((run / f"page-call{n:03d}-{phase}-plane{lane}.bin"
               for n in range(25, 43) for phase in ("entry", "return")
               for lane in range(4)), PAGE_SET_SHA, 144, 144 * 8000,
              "second-cycle pages")
    check_set((run / f"page-{tag}-plane{lane}.bin"
               for tag in ("call024-return", "call025-entry", "first-view")
               for lane in range(4)), BOUNDARY_SET_SHA, 12, 12 * 8000,
              "cycle/terminal boundaries")
    last = page(run, "call024-return")
    check_hash(last, FIRST_CYCLE_END_SHA, "first-cycle final page")
    first = page(run, "call025-entry")
    check_hash(first, SECOND_CYCLE_START_SHA, "second-cycle entry page")
    if sum(a != b for a, b in zip(last, first)) != 410:
        raise ValueError("inter-cycle reset changed")

    reports = []
    changes = {}
    for number, expected in enumerate(CALLS, 25):
        ram = (run / f"ram-call{number:03d}-entry-private.bin").read_bytes()
        font, source = call_input(ram, image, number, expected)
        entry = page(run, f"call{number:03d}-entry")
        if number > 25:
            prior = page(run, f"call{number-1:03d}-return")
            changed = [(at, after) for at, (before, after) in
                       enumerate(zip(prior, entry)) if before != after]
            expected_count = INTERCALL.get(number - 1, 0)
            if len(changed) != expected_count or any(value != 0 for _, value in changed):
                raise ValueError(f"inter-call page effect before {number} changed")
            if changed:
                changes[number - 1] = len(changed)
        model, report = run_probe(probe, run, number, expected,
                                  font, source, entry)
        if report != (expected[1], expected[1] * 32,
                      CHANGES[number - 25]):
            raise ValueError(f"call {number} C report changed")
        if model != page(run, f"call{number:03d}-return"):
            raise ValueError(f"call {number} immediate C/native page mismatch")
        reports.append(report)
    if changes != INTERCALL:
        raise ValueError("inter-call clear sequence changed")
    if page(run, "call042-return") != page(run, "first-view"):
        raise ValueError("terminal boundary changed")

    check_watch(image, watch)
    clear_input = page(run, "call027-return")
    cleared = bytearray(clear_input)
    for lane in range(4):
        for scanline in range(5 * 8, 6 * 8):
            offset = lane * 8000 + scanline * 40 + 17
            cleared[offset:offset + 22] = bytes(22)
    check_hash(bytes(cleared), FIRST_CLEAR_ENTRY_SHA,
               "decoded row-five clear result")
    if bytes(cleared) != page(run, "call028-entry"):
        raise ValueError("first row-clear page differs from native entry")
    print("second cycle: 18/18 C immediate returns, each 32000/32000 bytes")
    print("printer C totals: %d cells, %d stores, %d per-call value changes" %
          tuple(sum(report[i] for report in reports) for i in range(3)))
    print("inter-call native zero deltas: 27/28=154, 30/31=196, "
          "33/34=273, 36/37=196, 39/40=175")
    print("first clear: RES_00BA:07EA -> RES_03F1:0137 -> "
          "RES_0709:00B7 -> 131C:16B7, A000:0651 124->0")
    print("first decoded clear: 32000/32000 native call28-entry bytes")
    print("first-cycle/terminal page SHA-256", FIRST_CYCLE_END_SHA)
    print("all game and capture payloads remain under ignored resources/")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--watch", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    options = parser.parse_args()
    audit(options.exe, options.ovr, options.emulator, options.run,
          options.watch, options.probe)
