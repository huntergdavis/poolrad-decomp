#!/usr/bin/env python3
"""Audit the three reached slot-A row-nine printer calls in one DOS replay.

All media, RAM, source text, font, and page payloads stay under resources/.
The public output contains only addresses, hashes, and counts.
"""

import argparse
from pathlib import Path
import struct
import subprocess

import slot_a_row9_seven_cell_printer_audit as seven
import slot_a_row9_col34_printer_audit as col34
import slot_a_row9_col38_printer_audit as col38
from slot_a_printer_page_audit import (
    FONT_SHA, START_SHA, check_hash, unpack, word,
)

RAM_SHA = {
    "call1-entry": "9a13e5bdfa0c59d690c86ba5fb35f3ac6021d065d90e716998f5cce7a57167f0",
    "call1-return": "72777a363c006b37dcba49b3d670fd4689779f96b21c72e2189ab864e479fe4d",
    "call2-entry": "a9e04872681c522cf0784efffd7547efb9ffadae297c767e8d3e29cc3abe7746",
    "call2-return": "8e921be4a14f3d4e8a22fc3bbdf1abc452339d29e79ea5a7230dd66a77902ef2",
    "call3-entry": "af1964039c300ecdabc1e43ee7a4699da0ef6dcc3544cdd85b450037b1dde3c5",
    "call3-return": "af71278590d071d2e3dcfcbaca6dd66ae233ee0ab8a62fdb7087e1afb35070af",
    "first-view": "45a00d0d4c315687170e639815498d57df6f97caa2b9878eb4c38b8d32d1fe66",
}

PAGE = {
    "call1-entry": (seven.BEFORE_PLANES, seven.BEFORE_SHA),
    "call1-return": (seven.AFTER_PLANES, seven.AFTER_SHA),
    "call2-entry": (col34.BEFORE_PLANES, col34.BEFORE_SHA),
    "call2-return": (col34.AFTER_PLANES, col34.AFTER_SHA),
    "call3-entry": (col38.BEFORE_PLANES, col38.BEFORE_SHA),
    "call3-return": (col38.AFTER_PLANES, col38.AFTER_SHA),
    "first-view": (col38.FINAL_PLANES, col38.FINAL_SHA),
}

CALLS = (
    ("call1", seven, 0x3E92, (0x1858, 0x08DE, 8, 0x4529, 11, 9, 17),
     "cells=7 stores=224 changed=147"),
    ("call2", col34, 0x3C96, (0x096D, 0x08DE, 0x3DA6, 0x1D16, 10, 9, 34),
     "cells=1 stores=32 changed=14"),
    ("call3", col38, 0x3D94, (0x09C3, 0x08DE, 0x3DA2, 0x1D16, 10, 9, 38),
     "cells=1 stores=32 changed=14"),
)


def read_page(run: Path, phase: str) -> bytes:
    plane_hashes, page_hash = PAGE[phase]
    planes = []
    for lane, digest in enumerate(plane_hashes):
        data = (run / f"page-{phase}-plane{lane}.bin").read_bytes()
        if len(data) != 3520:
            raise ValueError(f"{phase} plane {lane} size changed")
        check_hash(data, digest, f"{phase} plane {lane}")
        planes.append(data)
    page = b"".join(planes)
    check_hash(page, page_hash, f"{phase} whole page")
    return page


def check_routes(image: bytes, entries: list[bytes]) -> None:
    for name, start, end, original_sha, seg, a, b, loaded_sha in seven.SPANS:
        check_hash(image[start:end], original_sha, f"original {name}")
        check_hash(entries[0][seg * 16 + a:seg * 16 + b], loaded_sha,
                   f"loaded {name}")
    for module, entry in ((col34, entries[1]), (col38, entries[2])):
        for name, (a, b, digest) in module.ORIGINAL_SPANS.items():
            check_hash(image[a:b], digest, f"original {name}")
        for name, (seg, a, b, digest) in module.RUNTIME_SPANS.items():
            check_hash(entry[seg * 16 + a:seg * 16 + b], digest,
                       f"loaded {name}")
    for at, ret, target in ((0x13C8, 0x082B, 0x1816),
                            (0x1415, 0x0878, 0x08EF),
                            (0x1457, 0x08BA, 0x0973),
                            (0x155B, 0x09BE, 0x1037)):
        insn = image[at:at + 3]
        if insn[0] != 0xE8 or ret + struct.unpack_from("<h", insn, 1)[0] != target:
            raise ValueError(f"original near call at {at:04X} changed")
    for at, loaded_at, entry in ((0x23F3, 0x1853, entries[0]),
                                 (0x1508, 0x0968, entries[1]),
                                 (0x155E, 0x09BE, entries[2])):
        original = image[at:at + 5]
        loaded = entry[0x08DE0 + loaded_at:0x08DE0 + loaded_at + 5]
        if (original[0] != 0x9A or
            struct.unpack_from("<HH", original, 1) != (0x0626, 0x0709) or
            loaded[0] != 0x9A or
            struct.unpack_from("<HH", loaded, 1) != (0x0626, 0x0F2D)):
            raise ValueError(f"far printer call at {loaded_at:04X} changed")


def audit(exe_path: Path, emulator_path: Path, run: Path, probe: Path) -> None:
    exe = exe_path.read_bytes()
    check_hash(exe, START_SHA, "START.EXE")
    check_hash(emulator_path.read_bytes(), seven.EMULATOR_SHA, "heavy DOSBox-X")
    check_hash((run / "POOLRAD/GAME.OVR").read_bytes(), seven.GAME_SHA, "GAME.OVR")
    check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(), seven.SAVE_SHA,
               "slot-A save")
    image, *_ = unpack(exe)
    ram = {}
    pages = {}
    for phase, digest in RAM_SHA.items():
        data = (run / f"ram-{phase}-private.bin").read_bytes()
        if len(data) != 0xA0000:
            raise ValueError(f"{phase} RAM size changed")
        check_hash(data, digest, f"{phase} RAM")
        ram[phase] = data
        pages[phase] = read_page(run, phase)
    entries = [ram[f"call{i}-entry"] for i in range(1, 4)]
    check_routes(image, entries)
    for before, after in (("call1-return", "call2-entry"),
                          ("call2-return", "call3-entry"),
                          ("call3-return", "first-view")):
        if pages[before] != pages[after]:
            raise ValueError(f"page changed between {before} and {after}")
    parents = (
        ("call1-entry", 0x3EA4, (0x3EC2, 0x082B, 0x08DE, 0, 9, 17, 8, 0x4529)),
        ("call2-entry", 0x3EA6, (0x3EC2, 0x0878, 0x08DE, 9, 34, 8, 0x4529)),
        ("call3-entry", 0x3EA4, (0x3EC2, 0x08BA, 0x08DE, 0, 9, 38, 8, 0x4529)),
    )
    for phase, sp, expected in parents:
        if struct.unpack_from("<" + "H" * len(expected), ram[phase],
                              0x1D160 + sp) != expected:
            raise ValueError(f"{phase} parent frame changed")

    working = pages["call1-entry"]
    all_changed = set()
    for name, module, sp, frame, report in CALLS:
        entry = ram[f"{name}-entry"]
        if struct.unpack_from("<7H", entry, 0x1D160 + sp) != frame:
            raise ValueError(f"{name} printer frame changed")
        _, _, src_off, src_seg, color, row, col = frame
        source_at = src_seg * 16 + src_off
        length = entry[source_at]
        if length != (7 if name == "call1" else 1):
            raise ValueError(f"{name} source length changed")
        check_hash(entry[source_at:source_at + length + 1], module.SOURCE_SHA,
                   f"{name} private source")
        if name == "call1":
            if (entry[source_at + 0x10D], entry[source_at + 0x10E]) != (1, 0):
                raise ValueError("call1 color branch inputs changed")
        elif name == "call3":
            node = 0x45290 + 8
            if (entry[node + 0x11B], entry[node + 0x32],
                    word(entry, 0x1D160 + 0x3EAA)) != (6, 6, 0):
                raise ValueError("call3 color branch inputs changed")
        ds = 0x14A00
        font_at = word(entry, ds + 0x5E20) + 16 * word(entry, ds + 0x5E22)
        font = entry[font_at:font_at + 1416]
        check_hash(font, FONT_SHA, f"{name} live font")
        if (word(entry, ds + 0x4A16), word(entry, ds + 0x4A18),
                entry[ds + 0x4A1E]) != (0xA000, 0xA000, 1):
            raise ValueError(f"{name} EGA page mode changed")
        if working != pages[f"{name}-entry"]:
            raise ValueError(f"C input does not match {name} native entry")
        native_after = pages[f"{name}-return"]
        changed = {i for i, (a, b) in enumerate(zip(working, native_after))
                   if a != b}
        expected = {
            lane * 3520 + scanline * 40 + column - 0x3C0
            for lane in ((0, 1, 3) if name == "call1" else (1, 3))
            for scanline in range(72, 79)
            for column in range(col, col + length)
        }
        if changed != expected or changed & all_changed:
            raise ValueError(f"{name} native page delta changed")
        all_changed |= changed
        private_input = run / f"{name}-composition-probe-input-private.bin"
        private_output = run / f"{name}-composition-probe-output-private.bin"
        private_input.write_bytes(font + bytes((length, col, row, color)) +
                                  entry[source_at + 1:source_at + 1 + length] +
                                  bytes(255 - length) + working)
        result = subprocess.run([str(probe.resolve()), str(private_input),
                                 str(private_output)], check=True,
                                capture_output=True, text=True)
        if result.stdout.strip() != report:
            raise ValueError(f"{name} C probe report changed: {result.stdout!r}")
        working = private_output.read_bytes()
        if working != native_after:
            raise ValueError(f"{name} C page differs from native return")
    if len(all_changed) != 175 or working != pages["first-view"]:
        raise ValueError("three-call net page or first view changed")
    print("row-nine calls: 08DE:1858 -> 096D -> 09C3")
    print("row 9 columns 17-23, 34, 38; colors 11, 10, 10")
    print("three native calls; 9 cells; 288 plane stores; 175 net changed bytes")
    for phase in ("call1-entry", "call1-return", "call2-return", "call3-return"):
        print(phase, "page SHA-256", PAGE[phase][1])
    print("same-run first-view page SHA-256", PAGE["first-view"][1])
    print("C sequence: 3/3 return pages each 14080/14080 native bytes")
    print("C sequence final: 14080/14080 same-run first-view bytes")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.emulator, args.run, args.probe)
