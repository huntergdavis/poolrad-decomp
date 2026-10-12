#!/usr/bin/env python3
"""Hash-locked private native parity audit for the loaded +0338 loader prefix.

Reads ignored original and native RAM inputs; prints only addresses, hashes,
and counts. Run with PYTHONPATH=/tmp/poolrad-capstone:tools/overlay.
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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-firstcall-002"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "58427ef2f283771df95d170aced18285dc01498739e76c8e6df64326f70a4991"
ENTRY_SHA = "9d40ff74d9deb791bbfc9527474cd29aff7ba3fd215cd9c3eaedcdef21c06614"
EXIT_SHA = "3861865f8457dc946ab8760847a53c8ad78e47a7c9bcafb1c43e30a48d5b3bac"
EVENT_SHA = "034a1539e1ab2a00d273ff32aa03adf517cf98fdfd36b743864f209352a4c6a1"
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
    lines = [line.split() for line in checked(path, 207, EVENT_SHA).decode().splitlines()]
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
    original = image[physical(0x0AA2, 0x0338):physical(0x0AA2, 0x03E9)]
    assert (len(original), sha(original)) == (177, BODY_SHA)
    entry = checked(args.run / "ram-loader-entry.bin", CAPTURE_SIZE, ENTRY_SHA)
    expected = checked(args.run / "ram-pre-0553.bin", CAPTURE_SIZE, EXIT_SHA)
    loaded = entry[physical(0x12C6, 0x0338):physical(0x12C6, 0x03E9)]
    assert loaded == original

    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x0338))
    assert len(instructions) == 64 and instructions[-1].address == 0x03E6
    assert instructions[-1].address + instructions[-1].size == 0x03E9
    branches = [i for i in instructions if i.mnemonic.startswith("j")]
    calls = [i for i in instructions if i.mnemonic == "call"]
    assert len(branches) == 12 and len(calls) == 12
    assert [(i.address, i.mnemonic, i.op_str) for i in instructions[:8]] == [
        (0x0338, "inc", "word ptr [0x43e0]"),
        (0x033C, "cmp", "word ptr es:[0x10], 0"),
        (0x0342, "je", "0x34d"),
        (0x0344, "mov", "word ptr es:[0x12], 1"),
        (0x034B, "jmp", "0x3b6"),
        (0x034D, "inc", "word ptr [0x43e2]"),
        (0x0351, "push", "es"),
        (0x0352, "call", "0x553"),
    ]
    assert [(i.address, i.op_str) for i in calls if i.address in (0x0352, 0x03B6)] == [
        (0x0352, "0x553"), (0x03B6, "0x435")]

    rows = native_rows(args.run / "ev-numeric.tsv")
    initial, final = rows["loader_entry"], rows["pre_0553"]
    assert (initial["cs"], initial["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x0338, 0x12C6, 0x0352)
    assert initial["fs"] == initial["gs"] == final["fs"] == final["gs"] == 0
    assert int.from_bytes(entry[physical(initial["es"], 0x0010):
                                physical(initial["es"], 0x0010) + 2], "little") == 0

    with tempfile.TemporaryDirectory(prefix="poolrad-loader-0338-") as directory:
        library = Path(directory) / "loader.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_prefix_0338.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_loader_prefix_0338
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
        assert changed_runs(entry, actual) == (4, 3)

    print("START SHA-256", START_SHA)
    print("original/loaded 0AA2:0338..03E8 / 12C6:0338..03E8:",
          "177 bytes, 64 instructions, 12 branches, 12 calls, SHA-256", BODY_SHA)
    print("native 12C6:0338 -> 12C6:0352: 4 changed bytes / 3 runs")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity at first +0553 call")
    print("+03B6 alternate first-call exit: instruction-derived synthetic CTest only")


if __name__ == "__main__":
    main()
