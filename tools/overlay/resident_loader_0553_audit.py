#!/usr/bin/env python3
"""Hash-locked private native parity audit for loaded resident +0553.

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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-0553-003"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "e7102f215250c40e24f1944f4f12412f821e5b86d84ad543487963f2cd1a2ab6"
RAM_SHA = "dca3585cff1400ba8fb5769a0fb8db92f57cdf774691a5392270fdd160121ff4"
EVENT_SHA = "6ba970b477060bbeb5cef267b7b71fd3a8028825ee686cd15b0de3eee5873b13"
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


def native_rows(path: Path) -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(path, 208, EVENT_SHA).decode().splitlines()]
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
    original = image[physical(0x0AA2, 0x0553):physical(0x0AA2, 0x055F)]
    assert (len(original), sha(original)) == (12, BODY_SHA)
    entry = checked(args.run / "ram-0553-entry.bin", CAPTURE_SIZE, RAM_SHA)
    expected = checked(args.run / "ram-0355-return.bin", CAPTURE_SIZE, RAM_SHA)
    assert entry == expected
    loaded = entry[physical(0x12C6, 0x0553):physical(0x12C6, 0x055F)]
    assert loaded == original
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x0553))
    assert [(i.address, i.mnemonic, i.op_str) for i in instructions] == [
        (0x0553, "mov", "ax, word ptr es:[8]"),
        (0x0557, "add", "ax, 0xf"),
        (0x055A, "mov", "cl, 4"),
        (0x055C, "shr", "ax, cl"),
        (0x055E, "ret", ""),
    ]

    rows = native_rows(args.run / "ev-numeric.tsv")
    initial, final = rows["callee_entry"], rows["post_return"]
    assert (initial["cs"], initial["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x0553, 0x12C6, 0x0355)
    assert initial["fs"] == initial["gs"] == final["fs"] == final["gs"] == 0
    assert int.from_bytes(entry[physical(initial["es"], 0x0008):
                                physical(initial["es"], 0x0008) + 2], "little") == 0x0463
    assert int.from_bytes(entry[physical(initial["ss"], initial["sp"]):
                                physical(initial["ss"], initial["sp"]) + 2], "little") == 0x0355

    with tempfile.TemporaryDirectory(prefix="poolrad-loader-0553-") as directory:
        library = Path(directory) / "loader.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_0553.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_loader_0553
        function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                             ctypes.POINTER(CPU)]
        function.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * SIZE)()
        ctypes.memmove(ram, entry, CAPTURE_SIZE)
        cpu = CPU(*(initial[field] for field in FIELDS))
        assert function(ram, SIZE, ctypes.byref(cpu)) == 0
        actual = bytes(ram)[:CAPTURE_SIZE]
        assert actual == expected
        assert all(getattr(cpu, field) == final[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("original/loaded 0AA2:0553..055E / 12C6:0553..055E:",
          "12 bytes, 5 instructions, 0 nested calls, SHA-256", BODY_SHA)
    print("native 12C6:0553 -> 12C6:0355: 0 changed captured RAM bytes")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity after near RET")


if __name__ == "__main__":
    main()
