#!/usr/bin/env python3
"""Hash-locked private native parity audit for loaded resident +0538.

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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-0538-005"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "42bb665c1e5b081cbc73aea256d7973ee33e38aa8d181e465807adcad450791d"
RAM_SHA = "228032a610412cbde1922f1dc817584d2c36599ddf6ba47f71cccd1770708669"
EVENT_SHA = "a705ceb58dcb250b58237bb4dd4bb76c4414d3509ca529c5ee2f80398767d50d"
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
    lines = [line.split() for line in checked(path, 242, EVENT_SHA).decode().splitlines()]
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
    original = image[physical(0x0AA2, 0x0538):physical(0x0AA2, 0x0553)]
    assert (len(original), sha(original)) == (27, BODY_SHA)
    entry = checked(args.run / "ram-0538-entry.bin", CAPTURE_SIZE, RAM_SHA)
    expected = checked(args.run / "ram-0366-return.bin", CAPTURE_SIZE, RAM_SHA)
    assert entry == expected
    loaded = entry[physical(0x12C6, 0x0538):physical(0x12C6, 0x0553)]
    assert loaded == original
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x0538))
    assert sum(i.size for i in instructions) == len(original)
    assert [(i.address, i.mnemonic, i.op_str) for i in instructions] == [
        (0x0538, "mov", "ax, word ptr [0x43fa]"),
        (0x053B, "or", "ax, ax"),
        (0x053D, "je", "0x54b"),
        (0x053F, "mov", "es, ax"),
        (0x0541, "mov", "ax, word ptr es:[0x10]"),
        (0x0545, "sub", "ax, word ptr [0x43f6]"),
        (0x0549, "jae", "0x552"),
        (0x054B, "mov", "ax, word ptr [0x43f8]"),
        (0x054E, "sub", "ax, word ptr [0x43f6]"),
        (0x0552, "ret", ""),
    ]

    rows = native_rows(args.run / "ev-numeric.tsv")
    initial, final = rows["callee_entry"], rows["post_return"]
    assert (initial["cs"], initial["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x0538, 0x12C6, 0x0366)
    assert initial["fs"] == initial["gs"] == final["fs"] == final["gs"] == 0
    assert (word(entry, initial["ds"], 0x43F6),
            word(entry, initial["ds"], 0x43F8),
            word(entry, initial["ds"], 0x43FA),
            word(entry, 0x085C, 0x0010),
            word(entry, initial["ss"], initial["sp"])) == (
                0x2CCD, 0x3996, 0x085C, 0x2DCF, 0x0366)

    with tempfile.TemporaryDirectory(prefix="poolrad-loader-0538-") as directory:
        library = Path(directory) / "loader.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_0538.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_loader_0538
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
    print("original/loaded 0AA2:0538..0552 / 12C6:0538..0552:",
          "27 bytes, 10 instructions, 2 branches, 0 nested calls, SHA-256", BODY_SHA)
    print("native 12C6:0538 -> 12C6:0366: 0 changed captured RAM bytes")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity after near RET")


if __name__ == "__main__":
    main()
