#!/usr/bin/env python3
"""Hash-locked private native parity audit for loaded +0355..+0362.

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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-0355-004"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PREFIX_SHA = "19e093876566c075385306f87770bf4e21321599beec2a2459aa04c48859e30c"
RAM_SHA = "5c796d56b67719be33989782e2e26740ab68337cbb9fd70182dc32f6f5eacb7e"
EVENT_SHA = "5db88ac2de6f7227697f3553f901e879d8cd3ac1b73545aca7f55fefe82c70ee"
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
    lines = [line.split() for line in checked(path, 198, EVENT_SHA).decode().splitlines()]
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
    original = image[physical(0x0AA2, 0x0355):physical(0x0AA2, 0x0363)]
    assert (len(original), sha(original)) == (14, PREFIX_SHA)
    entry = checked(args.run / "ram-0355-entry.bin", CAPTURE_SIZE, RAM_SHA)
    expected = checked(args.run / "ram-0363-precall.bin", CAPTURE_SIZE, RAM_SHA)
    assert entry == expected
    loaded = entry[physical(0x12C6, 0x0355):physical(0x12C6, 0x0363)]
    assert loaded == original
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x0355))
    assert [(i.address, i.mnemonic, i.op_str) for i in instructions] == [
        (0x0355, "mov", "dx, word ptr es:[0xa]"),
        (0x035A, "add", "dx, 0xf"),
        (0x035D, "mov", "cl, 4"),
        (0x035F, "shr", "dx, cl"),
        (0x0361, "add", "dx, ax"),
    ]
    call = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
        image[physical(0x0AA2, 0x0363):physical(0x0AA2, 0x0366)], 0x0363))
    assert len(call) == 1 and (call[0].mnemonic, call[0].op_str) == ("call", "0x538")

    rows = native_rows(args.run / "ev-numeric.tsv")
    initial, final = rows["post_0553"], rows["pre_0538"]
    assert (initial["cs"], initial["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x0355, 0x12C6, 0x0363)
    assert initial["fs"] == initial["gs"] == final["fs"] == final["gs"] == 0
    assert int.from_bytes(entry[physical(initial["es"], 0x000A):
                                physical(initial["es"], 0x000A) + 2], "little") == 0

    with tempfile.TemporaryDirectory(prefix="poolrad-loader-0355-") as directory:
        library = Path(directory) / "loader.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_0355.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_loader_0355
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
    print("original/loaded 0AA2:0355..0362 / 12C6:0355..0362:",
          "14 bytes, 5 instructions, SHA-256", PREFIX_SHA)
    print("native 12C6:0355 -> 12C6:0363: 0 changed captured RAM bytes")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity before CALL +0538")


if __name__ == "__main__":
    main()
