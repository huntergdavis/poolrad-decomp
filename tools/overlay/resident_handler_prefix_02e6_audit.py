#!/usr/bin/env python3
"""Hash-locked private native parity audit for the loaded INT 3Fh prefix.

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
RUN = ROOT / "resources/oracle-runs/slot-a-native-first-insert-handler-038"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
ORIGINAL_SHA = "6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c"
LOADED_SHA = "5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef"
ENTRY_SHA = "9168e596446be367a2190b0956fb6fc157fd7691331790a8708ebbaba7fd3cf5"
PRE_SHA = "de9fe269a5d64ed092982ca95e99ed70257506dd9dedb73be24e763ab46072cc"
EVENT_SHA = "6dedfa81d1e1b78ba490d7abd64c88e44fa180087d42749d97aaf93ce22f46bb"
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


def changed_runs(before: bytes, after: bytes) -> tuple[int, int]:
    different = [a != b for a, b in zip(before, after)]
    return sum(different), sum(v and (i == 0 or not different[i - 1])
                               for i, v in enumerate(different))


def native_rows(path: Path) -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(path, 726, EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert all(len(line) == 17 for line in lines[1:])
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
    original = image[physical(0x0AA2, 0x02E6):physical(0x0AA2, 0x0338)]
    assert (len(original), sha(original)) == (82, ORIGINAL_SHA)
    entry = checked(args.run / "ram-handler-entry.bin", CAPTURE_SIZE, ENTRY_SHA)
    expected = checked(args.run / "ram-handler-preloader.bin", CAPTURE_SIZE, PRE_SHA)
    loaded = entry[physical(0x12C6, 0x02E6):physical(0x12C6, 0x0338)]
    assert (len(loaded), sha(loaded)) == (82, LOADED_SHA)
    assert [0x02E6 + i for i, (a, b) in enumerate(zip(original, loaded))
            if a != b] == [0x02F2, 0x02F3]
    assert int.from_bytes(loaded[0x02F2 - 0x02E6:0x02F4 - 0x02E6], "little") == 0x14A0
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x02E6))
    assert len(instructions) == 42 and instructions[-1].address == 0x0337
    assert [(i.address, i.mnemonic, i.op_str) for i in instructions
            if i.mnemonic == "call"] == [(0x0303, "call", "0x338"),
                                           (0x0314, "call", "0x338")]

    rows = native_rows(args.run / "ev-numeric.tsv")
    initial, final = rows["handler_entry"], rows["pre_loader"]
    assert (initial["cs"], initial["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x02E6, 0x12C6, 0x0314)
    assert initial["fs"] == initial["gs"] == final["fs"] == final["gs"] == 0

    with tempfile.TemporaryDirectory(prefix="poolrad-handler-") as directory:
        library = Path(directory) / "handler.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_handler_prefix_02e6.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_handler_prefix_02e6
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
        assert changed_runs(entry, actual) == (23, 2)

    print("START SHA-256", START_SHA)
    print("handler original 0AA2:02E6..0337, 82 bytes, 42 instructions, SHA-256", ORIGINAL_SHA)
    print("handler loaded 12C6:02E6..0337, 2 relocated bytes, SHA-256", LOADED_SHA)
    print("native 12C6:02E6 -> 12C6:0314: 23 changed bytes / 2 runs")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity at first loader call")
    print("+0303 alternate first-loader exit: synthetic CTest only")


if __name__ == "__main__":
    main()
