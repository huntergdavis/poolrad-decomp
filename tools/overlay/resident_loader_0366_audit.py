#!/usr/bin/env python3
"""Hash-locked private native parity audit for loaded resident +0366.

Reads ignored DOS/RAM inputs; prints only addresses, hashes, and counts.
Run with PYTHONPATH=/tmp/poolrad-capstone:tools/overlay.
"""

from __future__ import annotations

import argparse
import ctypes
import hashlib
from pathlib import Path
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-loader-0366-006"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "bc10e48da989984caa915f95651900f4ff21724debacd9448fae85877833803c"
ENTRY_SHA = "f5331f379ecfc6861781dc42b7128d85b48a262a439a50851a6620df4fe64245"
EXIT_SHA = "62aef24e08e15f99fe2627552a5e4ddc641ae493fbbbefa51f950d4c28c814e7"
EVENT_SHA = "3d2cc367f385d0dc991c6729f6ab697e5f28437eab22f8e6b7a8df5d9b96f338"
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class CPU(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in FIELDS]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked(path: Path, length: int, digest: str) -> bytes:
    data = path.read_bytes()
    assert (len(data), sha(data)) == (length, digest), path
    return data


def physical(seg: int, off: int) -> int:
    return ((seg << 4) + off) & (SIZE - 1)


def word(ram: bytes, seg: int, off: int) -> int:
    return ram[physical(seg, off)] | (ram[physical(seg, (off + 1) & 0xFFFF)] << 8)


def native_rows(path: Path) -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(path, 232, EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert len(lines) == 3 and all(len(line) == 17 for line in lines[1:])
    return {line[0]: {key.lower(): int(value, 16)
                      for key, value in zip(lines[0][1:], line[1:])}
            for line in lines[1:]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, default=RUN)
    parser.add_argument("--start", type=Path, default=START)
    args = parser.parse_args()

    start = checked(args.start, 65231, START_SHA)
    image, *_ = unpack(start)
    original = image[physical(0x0AA2, 0x0366):physical(0x0AA2, 0x03AA)]
    assert (len(original), sha(original)) == (68, BODY_SHA)
    entry = checked(args.run / "ram-0366-entry.bin", CAPTURE_SIZE, ENTRY_SHA)
    expected = checked(args.run / "ram-03aa-precall.bin", CAPTURE_SIZE, EXIT_SHA)
    loaded = entry[physical(0x12C6, 0x0366):physical(0x12C6, 0x03AA)]
    assert loaded == original
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x0366))
    assert sum(i.size for i in instructions) == len(original)
    decoded = {i.address: (i.mnemonic, i.op_str) for i in instructions}
    assert {addr: decoded[addr] for addr in (
        0x0366, 0x0368, 0x0369, 0x039C, 0x039E, 0x03A0, 0x03A1,
        0x03A4, 0x03A8, 0x03A9)} == {
        0x0366: ("jmp", "0x39c"),
        0x0368: ("push", "dx"),
        0x0369: ("call", "0x3e9"),
        0x039C: ("sub", "dx, ax"),
        0x039E: ("ja", "0x368"),
        0x03A0: ("pop", "es"),
        0x03A1: ("mov", "ax, word ptr [0x43f6]"),
        0x03A4: ("mov", "word ptr es:[0x10], ax"),
        0x03A8: ("push", "es"),
        0x03A9: ("push", "es"),
    }

    rows = native_rows(args.run / "ev-numeric.tsv")
    initial, final = rows["entry"], rows["pre_call"]
    assert (initial["cs"], initial["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x0366, 0x12C6, 0x03AA)
    assert initial["fs"] == initial["gs"] == final["fs"] == final["gs"] == 0
    assert (word(entry, initial["ds"], 0x43F6),
            word(entry, initial["ss"], initial["sp"]),
            word(entry, initial["ds"], 0x8504),
            word(entry, initial["ds"], 0x8506)) == (
                0x2CCD, 0x08D0, 0x0268, 0x12C6)
    changed = {i for i, (before, after) in enumerate(zip(entry, expected))
               if before != after}
    assert changed == {physical(0x08D0, 0x0010), physical(0x08D0, 0x0011),
                       physical(0x1D16, 0x3776), physical(0x1D16, 0x3777)}

    with tempfile.TemporaryDirectory(prefix="poolrad-loader-0366-") as directory:
        library = Path(directory) / "loader.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_0366.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_loader_0366
        function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                             ctypes.POINTER(CPU)]
        function.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * SIZE)()
        ctypes.memmove(ram, entry, CAPTURE_SIZE)
        cpu = CPU(*(initial[field] for field in FIELDS))
        assert function(ram, SIZE, ctypes.byref(cpu)) == 0
        assert bytes(ram)[:CAPTURE_SIZE] == expected
        assert all(getattr(cpu, field) == final[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("original/loaded 0AA2:0366..03A9 / 12C6:0366..03A9:",
          "68 bytes, source window SHA-256", BODY_SHA)
    print("saved route: 8 instructions, +039E JA false, before +03AA far call")
    print("native changed captured RAM bytes:", len(changed))
    print("C exact captured 640 KiB RAM and 14 CPU-field parity at call boundary")


if __name__ == "__main__":
    main()
