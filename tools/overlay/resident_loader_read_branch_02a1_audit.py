#!/usr/bin/env python3
"""Hash-locked native/C audit for saved resident loader +02A1 branch.

Reads ignored DOS/RAM inputs; prints only addresses, hashes, and counts.
Run with PYTHONPATH=/tmp/poolrad-capstone:tools/overlay.
"""

from __future__ import annotations

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-loader-read-branch-02a1-011"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
SPAN_SHA = "d3acce3539cd3ca08b7378afe5d39747c6bfa2d4cb2b075375b7f49640991555"
RAM_SHA = "a4d69ce70aacf545a56e82742baf9b547b099643ca138f34c0ae198d8ce9d97f"
EVENT_SHA = "2ad3bc7d26531df2e675d5f1d4de5ae2a2563d4eee8cc88c02bf4e766ddf7b7a"
CAPTURE_SIZE = 0xA0000
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")
ROUTE = {
    0x02A1: ("jb", "0x2e1"),
    0x02A3: ("cmp", "ax, cx"),
    0x02A5: ("jb", "0x2de"),
    0x02A7: ("add", "ax, 0xf"),
    0x02AA: ("mov", "cl, 4"),
    0x02AC: ("shr", "ax, cl"),
    0x02AE: ("add", "ax, word ptr es:[0x10]"),
    0x02B3: ("mov", "ds, ax"),
    0x02B5: ("xor", "dx, dx"),
    0x02B7: ("mov", "cx, word ptr es:[0xa]"),
    0x02BC: ("jcxz", "0x2da"),
    0x02DA: ("xor", "ax, ax"),
    0x02DC: ("jmp", "0x2e1"),
}


class CPU(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def checked(path: Path, length: int, digest: str) -> bytes:
    data = path.read_bytes()
    assert (len(data), sha256(data).hexdigest()) == (length, digest), path
    return data


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + offset) & 0xFFFFF


def word(ram: bytes, segment: int, offset: int) -> int:
    low = physical(segment, offset)
    high = physical(segment, (offset + 1) & 0xFFFF)
    return ram[low] | (ram[high] << 8)


def rows() -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(RUN / "ev-numeric.tsv", 188,
                                               EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert len(lines) == 3 and all(len(line) == 17 for line in lines[1:])
    return {line[0]: {key.lower(): int(value, 16)
                      for key, value in zip(lines[0][1:], line[1:])}
            for line in lines[1:]}


def main() -> None:
    start = checked(RUN / "POOLRAD/START.EXE", 65231, START_SHA)
    checked(RUN / "POOLRAD/SAVGAMA.DAT", 13137, SAVE_SHA)
    image, *_ = unpack(start)
    original = image[physical(0x0AA2, 0x02A1):physical(0x0AA2, 0x02E1)]
    assert (len(original), sha256(original).hexdigest()) == (64, SPAN_SHA)
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(original, 0x02A1))
    assert len(decoded) == 27 and decoded[-1].address + decoded[-1].size == 0x02E1
    assert {i.address: (i.mnemonic, i.op_str) for i in decoded
            if i.address in ROUTE} == ROUTE
    assert not any(i.mnemonic.startswith("call") for i in decoded)

    pre = checked(RUN / "ram-02a1-pre.bin", CAPTURE_SIZE, RAM_SHA)
    post = checked(RUN / "ram-02e1-post.bin", CAPTURE_SIZE, RAM_SHA)
    assert pre == post
    assert pre[physical(0x12C6, 0x02A1):physical(0x12C6, 0x02E1)] == original
    native = rows()
    before, after = native["pre"], native["post"]
    assert (before["cs"], before["ip"], after["cs"], after["ip"]) == (
        0x12C6, 0x02A1, 0x12C6, 0x02E1)
    assert (before["ax"], before["cx"], before["flags"] & 1) == (0x463, 0x463, 0)
    assert (word(pre, before["es"], 0x000A),
            word(pre, before["es"], 0x0010)) == (0, 0x2CCD)
    assert (after["ax"], after["cx"], after["ds"], after["flags"]) == (
        0, 0, 0x2D14, 0x7246)
    assert all(before[k] == after[k] for k in
               ("bx", "si", "di", "bp", "sp", "cs", "es", "ss", "dx"))
    assert before["fs"] == before["gs"] == after["fs"] == after["gs"] == 0

    with tempfile.TemporaryDirectory(prefix="poolrad-branch-02a1-") as d:
        library = Path(d) / "branch.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_read_branch_02a1.c"),
                        "-o", str(library)], check=True)
        step = ctypes.CDLL(str(library)).pr_resident_loader_read_branch_02a1
        step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(CPU)]
        step.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * RAM_SIZE)()
        ctypes.memmove(ram, pre, CAPTURE_SIZE)
        cpu = CPU(*(before[field] for field in FIELDS))
        assert step(ram, RAM_SIZE, ctypes.byref(cpu)) == 0
        assert bytes(ram)[:CAPTURE_SIZE] == post
        assert all(getattr(cpu, field) == after[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("original/loaded +02A1..+02E0: 64 bytes, 27 instructions, SHA-256",
          SPAN_SHA)
    print("native +02A1 -> +02E1: CF clear; AX=CX=0x0463; ES:[000A]=0")
    print("native route: two JB fall through, JCXZ takes +02DA, then +02E1")
    print("native 640 KiB RAM unchanged, SHA-256", RAM_SHA)
    print("C 14 CPU fields and captured RAM match; second DOS read remains a boundary")


if __name__ == "__main__":
    main()
