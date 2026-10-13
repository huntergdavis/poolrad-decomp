#!/usr/bin/env python3
"""Hash-locked native/C audit for saved resident loader +0291 read setup.

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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-read-gate-0291-009"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
SPAN_SHA = "e024fb0005bc417e084a74e2dd9650d5d5c52f80d43993f80a03dbde9788695c"
RAM_SHA = "9cd2d5fc44bff02bb14103265d8270c67558b55f27a291820c88866bf16d0ea8"
EVENT_SHA = "7b9a2e01beb538a367feccb309d40eb9c1f216ea6e17e1e7b7ac949bf75b5f90"
CAPTURE_SIZE = 0xA0000
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class CPU(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def checked(path: Path, size: int, digest: str) -> bytes:
    data = path.read_bytes()
    assert (len(data), sha256(data).hexdigest()) == (size, digest), path
    return data


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + offset) & 0xFFFFF


def word(ram: bytes, segment: int, offset: int) -> int:
    low = physical(segment, offset)
    high = physical(segment, (offset + 1) & 0xFFFF)
    return ram[low] | (ram[high] << 8)


def rows() -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(RUN / "ev-numeric.tsv", 232,
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
    span = image[physical(0x0AA2, 0x0291):physical(0x0AA2, 0x029F)]
    assert (len(span), sha256(span).hexdigest()) == (14, SPAN_SHA)
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x0291))
    assert [(i.address, i.mnemonic, i.op_str) for i in insns] == [
        (0x0291, "mov", "ds, word ptr es:[0x10]"),
        (0x0296, "xor", "dx, dx"),
        (0x0298, "mov", "cx, word ptr es:[8]"),
        (0x029D, "mov", "ah, 0x3f")]

    entry = checked(RUN / "ram-0291-entry.bin", CAPTURE_SIZE, RAM_SHA)
    expected = checked(RUN / "ram-029f-preint.bin", CAPTURE_SIZE, RAM_SHA)
    assert entry == expected
    assert entry[physical(0x12C6, 0x0291):physical(0x12C6, 0x029F)] == span
    native = rows()
    before, final = native["entry"], native["pre_read"]
    assert (before["cs"], before["ip"], final["cs"], final["ip"]) == (
        0x12C6, 0x0291, 0x12C6, 0x029F)
    assert before["fs"] == before["gs"] == final["fs"] == final["gs"] == 0
    assert (word(entry, before["es"], 0x0008),
            word(entry, before["es"], 0x0010)) == (0x0463, 0x2CCD)
    assert before["ax"] == 0xAA87 and final["ax"] == 0x3F87
    assert final["bx"] == before["bx"] == 5

    with tempfile.TemporaryDirectory(prefix="poolrad-read-gate-0291-") as d:
        library = Path(d) / "gate.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_read_gate_0291.c"),
                        "-o", str(library)], check=True)
        step = ctypes.CDLL(str(library)).pr_resident_loader_read_gate_0291
        step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(CPU)]
        step.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * RAM_SIZE)()
        ctypes.memmove(ram, entry, CAPTURE_SIZE)
        cpu = CPU(*(before[field] for field in FIELDS))
        assert step(ram, RAM_SIZE, ctypes.byref(cpu)) == 0
        assert bytes(ram)[:CAPTURE_SIZE] == expected
        assert all(getattr(cpu, field) == final[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("original/loaded +0291..029E: 14 bytes, 4 instructions, SHA-256", SPAN_SHA)
    print("native +0291 -> +029F: 0 changed captured RAM bytes")
    print("C exact captured 640 KiB RAM and 14 CPU-field parity before DOS read")
    print("+029F INT 21h effects remain external")


if __name__ == "__main__":
    main()
