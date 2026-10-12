#!/usr/bin/env python3
"""Hash-locked private native parity audit for loaded resident +0268.

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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-0268-007"
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "1aa7e6650b22df729a0529f2723bc860c14f54ef5e79b24d79d7feecd4ae9e0b"
CALL_SHA = "685bf9d927e06f3383fb8feb01a79bdf24d6432cae1ee834f78ed811658a8a99"
CALLER_SHA = "6e73b2f90006ed8a773dc591e3bae592ece500019868b16da40fe255c74ba76b"
ENTRY_SHA = "c835697cf11c721678c3568524bbb3f2c24b5ecaf6c1475d18f21a52cc3d408c"
EXIT_SHA = "bf84dde80f3e5b29829d177968ec6298e6231075b3636c8ed86d32cbe5a79576"
EVENT_SHA = "5031f354a224a11fe3556b7b869f2afb8b7d7ea06c190410537cff383cf34bcc"
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
    lines = [line.split() for line in checked(path, 318, EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert len(lines) == 4 and all(len(line) == 17 for line in lines[1:])
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
    original = image[physical(0x0AA2, 0x0268):physical(0x0AA2, 0x028F)]
    call = image[physical(0x0AA2, 0x03AA):physical(0x0AA2, 0x03AE)]
    assert (len(original), sha(original)) == (39, BODY_SHA)
    assert (len(call), sha(call)) == (4, CALL_SHA)
    caller = checked(args.run / "ram-03aa-precall.bin", CAPTURE_SIZE, CALLER_SHA)
    entry = checked(args.run / "ram-0268-entry.bin", CAPTURE_SIZE, ENTRY_SHA)
    expected = checked(args.run / "ram-028f-preint.bin", CAPTURE_SIZE, EXIT_SHA)
    assert entry[physical(0x12C6, 0x0268):physical(0x12C6, 0x028F)] == original
    assert caller[physical(0x12C6, 0x03AA):physical(0x12C6, 0x03AE)] == call
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x0268))
    assert len(insns) == 11 and sum(i.size for i in insns) == len(original)
    assert [(i.address, i.mnemonic, i.op_str) for i in insns] == [
        (0x0268, "mov", "bx, sp"),
        (0x026A, "mov", "es, word ptr ss:[bx + 4]"),
        (0x026E, "push", "ds"),
        (0x026F, "mov", "bx, word ptr [0x43fc]"),
        (0x0273, "mov", "di, word ptr [0x4422]"),
        (0x0277, "add", "di, 0x10"),
        (0x027A, "mov", "dx, word ptr es:[4]"),
        (0x027F, "mov", "cx, word ptr es:[6]"),
        (0x0284, "add", "dx, word ptr [0x43e8]"),
        (0x0288, "adc", "cx, word ptr [0x43ea]"),
        (0x028C, "mov", "ax, 0x4200"),
    ]
    assert [(i.address, i.mnemonic, i.op_str) for i in
            Cs(CS_ARCH_X86, CS_MODE_16).disasm(call, 0x03AA)] == [
                (0x03AA, "lcall", "[0x8504]")]

    rows = native_rows(args.run / "ev-numeric.tsv")
    pre_call, initial, final = rows["caller"], rows["entry"], rows["pre_int"]
    assert (pre_call["cs"], pre_call["ip"], initial["cs"], initial["ip"],
            final["cs"], final["ip"]) == (
                0x12C6, 0x03AA, 0x12C6, 0x0268, 0x12C6, 0x028F)
    assert pre_call["fs"] == initial["fs"] == final["fs"] == 0
    assert pre_call["gs"] == initial["gs"] == final["gs"] == 0
    assert (word(caller, pre_call["ds"], 0x8504),
            word(caller, pre_call["ds"], 0x8506)) == (0x0268, 0x12C6)
    assert initial["sp"] == (pre_call["sp"] - 4) & 0xFFFF
    assert (word(entry, initial["ss"], initial["sp"]),
            word(entry, initial["ss"], (initial["sp"] + 2) & 0xFFFF),
            word(entry, initial["ss"], (initial["sp"] + 4) & 0xFFFF)) == (
                0x03AE, 0x12C6, 0x08D0)
    assert (word(entry, initial["ds"], 0x43FC),
            word(entry, initial["ds"], 0x4422),
            word(entry, initial["ds"], 0x43E8),
            word(entry, initial["ds"], 0x43EA),
            word(entry, 0x08D0, 0x0004),
            word(entry, 0x08D0, 0x0006)) == (
                0x0005, 0x0814, 0, 0, 0xAA87, 0x0002)
    changed = {i for i, (before, after) in enumerate(zip(entry, expected))
               if before != after}
    assert changed == {physical(0x1D16, 0x3770), physical(0x1D16, 0x3771)}

    with tempfile.TemporaryDirectory(prefix="poolrad-loader-0268-") as directory:
        library = Path(directory) / "loader.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_0268.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_resident_loader_0268
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
    print("original/loaded 0AA2:0268..028E / 12C6:0268..028E:",
          "39 bytes, 11 instructions, SHA-256", BODY_SHA)
    print("caller 12C6:03AA CALL FAR [8504]: 4 bytes, SHA-256", CALL_SHA)
    print("native 12C6:0268 -> 12C6:028F: 2 changed captured RAM bytes")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity before INT 21h")


if __name__ == "__main__":
    main()
