#!/usr/bin/env python3
"""Check one native clean-page EGA byte against the bounded C font-plane model.

All binary inputs and the temporary font extraction remain under ignored
resources/. Public output is limited to addresses, hashes and match counts.
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
IMAGE_SHA = "8d71dd4c0162462f99aef0fbbd4cfee472702f330bca1e07b50f192ffd972aef"
RUNTIME_SHA = "05468fbf187f03220b52df161cea902636eb43d908da6ea9ae19543a2855b26e"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
ZERO_SHA = "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"
CHANGED_SHA = "8a8de823d5ed3e12746a62ef169bcf372be0ca44f0a1236abc35df05d96928e1"


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
    check_hash(image[0x726F:0x767A], IMAGE_SHA, "expanded glyph drawer")
    if image[0x74A2:0x74A5] != bytes.fromhex("268815"):
        raise ValueError("expanded source store changed")

    ram = (run / "ram-a000-0511-first-change.bin").read_bytes()
    if len(ram) != 655360:
        raise ValueError("watch RAM must be 640 KiB")
    base = 0x0F2D0
    check_hash(ram[base + 0x1DF:base + 0x5EA], RUNTIME_SHA, "runtime glyph drawer")
    if ram[base + 0x407:base + 0x415] != bytes.fromhex(
        "268a15a1164a508b7efc07268815"
    ):
        raise ValueError("glyph source/store instruction path changed")
    if ram[base + 0x68F:base + 0x694] != bytes.fromhex("500ee84bfb"):
        raise ValueError("printer callsite changed")

    ds = 0x14A00
    ss_bp = 0x1D160 + 0x3D8E
    args = tuple(word(ram, ss_bp + off) for off in (0x10, 0x0E, 0x0C, 0x0A, 0x08, 0x06))
    if args != (0x11, 0x04, 0x0F, 0x01, 0x54, 0x01):
        raise ValueError(f"watch frame changed: {args}")
    if word(ram, ss_bp + 2) != 0x0694 or word(ram, ss_bp + 4) != 0x0F2D:
        raise ValueError("printer return frame changed")
    if word(ram, ds + 0x4A16) != 0xA000 or ram[ds + 0x4A1E] != 1:
        raise ValueError("EGA output page/mode changed")
    font_off = word(ram, ds + 0x5E20) + 16 * word(ram, ds + 0x5E22)
    font = ram[font_off:font_off + 1416]
    check_hash(font, FONT_SHA, "resident font")
    if ram[ss_bp - 2] != 0x14 or ram[ss_bp - 0x0F] != 0:
        raise ValueError("effective glyph/scanline changed")
    if ram[font_off + 0x14 * 8] != 0x3F:
        raise ValueError("source font row changed")

    samples = {}
    for phase in ("before", "first-change", "view"):
        for plane in range(4):
            name = f"a000-0511-plane{plane}-{phase}.bin"
            data = (run / name).read_bytes()
            expected = CHANGED_SHA if phase == "view" or (
                phase == "first-change" and plane == 0
            ) else ZERO_SHA
            check_hash(data, expected, name)
            samples[phase, plane] = data[0]

    font_path = run / "font-buffer-private.bin"
    font_path.write_bytes(font)
    result = subprocess.run(
        [str(probe.resolve()), str(font_path), "0x54", "1", "0", "0x0F"],
        check=True, capture_output=True, text=True,
    )
    predicted = bytes(int(part, 16) for part in result.stdout.split())
    if len(predicted) != 4 or predicted != bytes(samples["view", p] for p in range(4)):
        raise ValueError("C model did not reproduce four native plane bytes")
    if samples["before", 0] == samples["first-change", 0]:
        raise ValueError("selected first breakpoint was not a value change")
    print("resident writer RES_0709:0412; printer call RES_0709:0691")
    print("watch A000:0511: first value change before later RES_0464:1B52")
    print("hash-locked original span: 1035 bytes; live font: 1416 bytes")
    print("C glyph-plane model: 4/4 first-view plane bytes")
    print("private captures and font remain under ignored resources/")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    args = parser.parse_args()
    audit(args.exe, args.run, args.probe)
