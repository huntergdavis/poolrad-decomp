#!/usr/bin/env python3
"""Audit the bounded slot-A printer cycles using ignored native captures.

No original media, text, font, RAM, or EGA payload is emitted or tracked.
The first cycle has immediate-return C parity. The second has terminal-only
composition evidence because its intermediate native pages were not captured.
"""

import argparse
import hashlib
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
LOADED_CALL_SHA = "cb7ca4fe93a69e515be3015b899a9d5c242cb8d630b9a2dcb6aabb161bc8d585"
ORIGINAL_CALL_SHA = "04b2067ec093896f5354169062fbf161dc2a8c234fbf656598e83bbbaca7ba23"
FIRST_RAM_SET_SHA = "e2b1caa5dc4a0f9ab0422f3bc48c954b1a1d7a96460340d0cffb513f7b17188e"
FIRST_PAGE_SET_SHA = "e7214cd8e42b5df641c2c69ccef46bcc173efa8bcd4b663e555528e5cd4999f1"
FIRST_BOUNDARY_SET_SHA = "b35077cc45d06fb6596f08d0015dc47efc4ccc51b1ddd98d63c091810908ffa6"
SECOND_RAM_SET_SHA = "4606d4dc0fecec9da3d30515e8ef96ad8a39a36eb68844fe8965e1376701a216"
SECOND_BOUNDARY_SET_SHA = "dd99ce02cf7024ecc30a514cbd197c410f4dc441515b8cfee44c784abe08e873"
FIRST_CALL_RAM_SHA = "5a48787a046e44184ba1aa6195adc24cf0a638da040b50214bfa56fd36830c4e"
FIRST_CALL_SOURCE_SHA = "3268e5ee46c67c0bb66ab90e8f2c5b34da5aa23c6ab52d5b19fb80ceba9071af"
FIRST_TERMINAL_RAM_SHA = "69035390d72e1f21d7b72a2ef85c1e419c11280a7d9f09566906baaf171e3aa4"
SECOND_TERMINAL_RAM_SHA = "bf57d7ee989c0515a04311b5772bf7add537c2b81a0d3c0ce2648519c213ac8e"
FIRST_CALL_ENTRY_SHA = "eed1ea4a2494ae386259d34ad9dc6c90e1dc57be36a8ef5a6dacbfaacd3d7e76"
FIRST_CALL_RETURN_SHA = "1aeaaa19361512eb3937dae04daaef6bca7ff9723a6e1a2a01a95b203adafd6f"
FIRST_CYCLE_INPUT_SHA = "606d838123a21edb436339b6fbf4fb2583ae79b38964d89f24e763cdb34c7c77"
FIRST_CYCLE_OUTPUT_SHA = "3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738"
SECOND_CYCLE_INPUT_SHA = "2aa65739f6bc280936e6203d6bb6cbaad26663475ee86e51468d48a4b7b4bfdf"

# (return IP, length, color, row, column, SS:SP). All calls return to 08DE.
FIRST_CYCLE = (
    (0x07A1, 4, 15, 2, 17, 0x3EA6), (0x07BF, 6, 15, 2, 33, 0x3EA6),
    (0x0814, 14, 15, 4, 17, 0x3EA6), (0x096D, 1, 10, 4, 34, 0x3C96),
    (0x09C3, 2, 10, 4, 37, 0x3D94), (0x1858, 6, 11, 5, 17, 0x3E92),
    (0x096D, 1, 10, 5, 34, 0x3C96), (0x09C3, 1, 10, 5, 38, 0x3D94),
    (0x1858, 8, 11, 6, 17, 0x3E92), (0x096D, 1, 10, 6, 34, 0x3C96),
    (0x09C3, 1, 10, 6, 38, 0x3D94), (0x1858, 12, 11, 7, 17, 0x3E92),
    (0x096D, 1, 10, 7, 34, 0x3C96), (0x09C3, 2, 10, 7, 37, 0x3D94),
    (0x1858, 8, 11, 8, 17, 0x3E92), (0x096D, 1, 10, 8, 34, 0x3C96),
    (0x09C3, 1, 10, 8, 38, 0x3D94), (0x1858, 7, 11, 9, 17, 0x3E92),
    (0x096D, 1, 10, 9, 34, 0x3C96), (0x09C3, 1, 10, 9, 38, 0x3D94),
    (0x2B5A, 12, 10, 15, 17, 0x3C86),
    (0x07A1, 4, 15, 2, 17, 0x3EB0), (0x07BF, 6, 15, 2, 33, 0x3EB0),
)
# A second rendering of rows four through nine follows a 410-byte page reset.
SECOND_SPS = (
    0x3EB0, 0x3CA0, 0x3D9E, 0x3E9C, 0x3CA0, 0x3D9E,
    0x3E9C, 0x3CA0, 0x3D9E, 0x3E9C, 0x3CA0, 0x3D9E,
    0x3E9C, 0x3CA0, 0x3D9E, 0x3E9C, 0x3CA0, 0x3D9E,
)
SECOND_CYCLE = tuple((*call[:5], sp) for call, sp in
                     zip(FIRST_CYCLE[2:20], SECOND_SPS))


def expect_hash(data: bytes, digest: str, label: str) -> None:
    if hashlib.sha256(data).hexdigest() != digest:
        raise ValueError(f"{label} SHA-256 changed")


def aggregate(paths, digest: str, count: int, size: int, label: str) -> None:
    state = hashlib.sha256()
    actual_count = actual_size = 0
    for path in paths:
        data = path.read_bytes()
        state.update(data)
        actual_count += 1
        actual_size += len(data)
    if (actual_count, actual_size, state.hexdigest()) != (count, size, digest):
        raise ValueError(f"{label} capture set changed")


def page(run: Path, tag: str) -> bytes:
    result = b"".join((run / f"page-{tag}-plane{plane}.bin").read_bytes()
                      for plane in range(4))
    if len(result) != 32000:
        raise ValueError(f"{tag} full-plane page size changed")
    return result


def call_frame(ram: bytes, image: bytes, number: int, expected):
    ret_ip, length, color, row, col, sp = expected
    words = struct.unpack_from("<7H", ram, 0x1D160 + sp)
    if (words[0], words[1], words[4] & 255, words[5] & 255,
            words[6] & 255) != (ret_ip, 0x08DE, color, row, col):
        raise ValueError(f"call {number} return or argument frame changed")
    site = ret_ip - 5
    original = image[site + 0xBA0:site + 0xBA5]
    loaded = ram[0x08DE0 + site:0x08DE0 + site + 5]
    expect_hash(original, ORIGINAL_CALL_SHA, f"call {number} original far call")
    expect_hash(loaded, LOADED_CALL_SHA, f"call {number} loaded far call")
    expect_hash(ram[0x0F2D0 + 0x0626:0x0F2D0 + 0x06A7],
                PRINTER_RUNTIME_SHA, f"call {number} loaded printer body")
    source_at = words[3] * 16 + words[2]
    if source_at + length + 1 > len(ram) or ram[source_at] != length:
        raise ValueError(f"call {number} source length changed")
    ds = 0x14A00
    font_at = (struct.unpack_from("<H", ram, ds + 0x5E20)[0] +
               16 * struct.unpack_from("<H", ram, ds + 0x5E22)[0])
    font = ram[font_at:font_at + 1416]
    expect_hash(font, FONT_SHA, f"call {number} live font")
    if (struct.unpack_from("<HH", ram, ds + 0x4A16) != (0xA000, 0xA000)
            or ram[ds + 0x4A1E] != 1):
        raise ValueError(f"call {number} EGA page mode changed")
    return font, ram[source_at + 1:source_at + length + 1]


def apply_probe(probe: Path, run: Path, number: int, expected,
                font: bytes, source: bytes, before: bytes):
    _, length, color, row, col, _ = expected
    payload = (font + bytes((length, col, row, color)) + source +
               bytes(255 - length) + before)
    private_input = run / f"call{number:03d}-whole-cycle-input-private.bin"
    private_output = run / f"call{number:03d}-whole-cycle-output-private.bin"
    private_input.write_bytes(payload)
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    match = re.fullmatch(r"cells=(\d+) stores=(\d+) changed=(\d+)\n?",
                         result.stdout)
    if match is None:
        raise ValueError(f"call {number} C probe report changed")
    return private_output.read_bytes(), tuple(map(int, match.groups()))


def audit(exe: Path, ovr: Path, emulator: Path, first: Path,
          second: Path, probe: Path) -> None:
    raw = exe.read_bytes()
    expect_hash(raw, START_SHA, "START.EXE")
    expect_hash(ovr.read_bytes(), GAME_SHA, "GAME.OVR")
    expect_hash(emulator.read_bytes(), EMULATOR_SHA, "heavy debugger")
    image, *_ = unpack(raw)
    expect_hash(image[0x76B6:0x7737], PRINTER_IMAGE_SHA,
                "original RES_0709:0626 body")
    for run in (first, second):
        expect_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(),
                    SAVE_SHA, f"{run.name} slot-A save")
    aggregate((first / f"ram-call{n:03d}-entry-private.bin"
               for n in range(2, 25)), FIRST_RAM_SET_SHA, 23, 23 * 0xA0000,
              "first-cycle RAM")
    aggregate((first / f"page-call{n:03d}-{phase}-plane{lane}.bin"
               for n in range(2, 25) for phase in ("entry", "return")
               for lane in range(4)), FIRST_PAGE_SET_SHA, 184, 184 * 8000,
              "first-cycle entry/return pages")
    aggregate((first / f"page-call{n:03d}-{phase}-plane{lane}.bin"
               for n, phase in ((1, "return"), (2, "entry"),
                                (24, "return"), (25, "entry"))
               for lane in range(4)), FIRST_BOUNDARY_SET_SHA, 16, 16 * 8000,
              "first-cycle boundary pages")
    aggregate((second / f"ram-call{n:03d}-entry-private.bin"
               for n in range(25, 43)), SECOND_RAM_SET_SHA, 18,
              18 * 0xA0000, "second-cycle RAM")
    aggregate((second / f"page-{tag}-plane{lane}.bin"
               for tag in ("call024-return", "call025-entry", "first-view")
               for lane in range(4)), SECOND_BOUNDARY_SET_SHA, 12,
              12 * 8000, "second-cycle boundaries")

    first_call = (first / "ram-call001-entry-private.bin").read_bytes()
    expect_hash(first_call, FIRST_CALL_RAM_SHA, "earliest printer RAM")
    overlay_frame = struct.unpack_from("<7H", first_call, 0x1D160 + 0x37AA)
    if (overlay_frame[0], overlay_frame[1], overlay_frame[4] & 255,
            overlay_frame[5] & 255, overlay_frame[6] & 255) != (
            0x17D5, 0x2A8E, 10, 24, 0):
        raise ValueError("earliest overlay printer frame changed")
    src = overlay_frame[3] * 16 + overlay_frame[2]
    if first_call[src] != 21:
        raise ValueError("earliest overlay source length changed")
    expect_hash(first_call[src:src + 22], FIRST_CALL_SOURCE_SHA,
                "earliest overlay source")
    expect_hash(first_call[0x2A8E0 + 0x17D0:0x2A8E0 + 0x17D5],
                LOADED_CALL_SHA, "earliest overlay loaded far call")
    expect_hash(page(first, "call001-entry"), FIRST_CALL_ENTRY_SHA,
                "earliest printer entry page")
    expect_hash(page(first, "call001-return"), FIRST_CALL_RETURN_SHA,
                "earliest printer return page")
    initial = page(first, "call002-entry")
    expect_hash(initial, FIRST_CYCLE_INPUT_SHA, "post-transfer page")
    transfer_delta = sum(a != b for a, b in
                         zip(page(first, "call001-return"), initial))
    if transfer_delta != 5119:
        raise ValueError("pre-cycle page transfer delta changed")

    working = initial
    first_totals = [0, 0, 0]
    for n, expected in enumerate(FIRST_CYCLE, 2):
        ram = (first / f"ram-call{n:03d}-entry-private.bin").read_bytes()
        font, source = call_frame(ram, image, n, expected)
        if working != page(first, f"call{n:03d}-entry"):
            raise ValueError(f"page changed between first-cycle calls before {n}")
        working, report = apply_probe(probe, first, n, expected,
                                      font, source, working)
        if working != page(first, f"call{n:03d}-return"):
            raise ValueError(f"call {n} C page differs from native return")
        first_totals = [a + b for a, b in zip(first_totals, report)]
    if first_totals != [101, 3232, 1754]:
        raise ValueError("first-cycle C counts changed")
    expect_hash(working, FIRST_CYCLE_OUTPUT_SHA, "first-cycle final page")
    expect_hash((first / "ram-first-view-private.bin").read_bytes(),
                FIRST_TERMINAL_RAM_SHA, "first-run terminal RAM")

    reset = page(first, "call025-entry")
    expect_hash(reset, SECOND_CYCLE_INPUT_SHA, "second-cycle reset page")
    if sum(a != b for a, b in zip(working, reset)) != 410:
        raise ValueError("inter-cycle page reset delta changed")
    if (page(second, "call024-return") != working or
            page(second, "call025-entry") != reset):
        raise ValueError("independent replay boundary pages differ")
    # Call 25 was also reached in the first run, independently of the
    # second run that captured all remaining caller frames.
    call_frame((first / "ram-call025-entry-private.bin").read_bytes(),
               image, 25, SECOND_CYCLE[0])
    second_working = reset
    second_totals = [0, 0, 0]
    for n, expected in enumerate(SECOND_CYCLE, 25):
        ram = (second / f"ram-call{n:03d}-entry-private.bin").read_bytes()
        font, source = call_frame(ram, image, n, expected)
        second_working, report = apply_probe(probe, second, n, expected,
                                             font, source, second_working)
        second_totals = [a + b for a, b in zip(second_totals, report)]
    if second_totals != [69, 2208, 410]:
        raise ValueError("second-cycle C counts changed")
    terminal = page(second, "first-view")
    expect_hash((second / "ram-first-view-private.bin").read_bytes(),
                SECOND_TERMINAL_RAM_SHA, "second-run terminal RAM")
    if terminal != working or second_working != terminal:
        raise ValueError("second-cycle C terminal page differs from native")
    print("native printer entries before 3582:3387: 42 (24 + 18)")
    print("pre-first-cycle transfer changed 5119/32000 page values")
    print("first-cycle C: 23/23 immediate returns, each 32000/32000 bytes")
    print("first-cycle C: 101 cells, 3232 stores, 1754 net changes")
    print("inter-cycle reset changed 410/32000 page values")
    print("second-cycle C: terminal-only 32000/32000 bytes; intermediate returns unmeasured")
    print("first-cycle and terminal page SHA-256", FIRST_CYCLE_OUTPUT_SHA)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--first-run", type=Path, required=True)
    parser.add_argument("--second-run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    options = parser.parse_args()
    audit(options.exe, options.ovr, options.emulator, options.first_run,
          options.second_run, options.probe)
