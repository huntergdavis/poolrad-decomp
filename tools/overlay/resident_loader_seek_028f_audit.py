#!/usr/bin/env python3
"""Hash-locked native/C audit for saved resident DOS seek +028F.

Reads only ignored files. Prints addresses, hashes, and counts, never bytes.
Run with PYTHONPATH=/tmp/poolrad-capstone:tools/overlay.
"""

from __future__ import annotations

import ctypes
from hashlib import sha256
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-loader-seek-028f-008"
START = RUN / "POOLRAD/START.EXE"
SAVE = RUN / "POOLRAD/SAVGAMA.DAT"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
INSTRUCTION_SHA = "855d16faaa027a1c2bf57ebdc2ca53925d942f65bf436cbaf69672c4c8227a8c"
PRE_SHA = "bd0f46df996192228db2bed77c7e060823e6f459b4893d02c318500a9c5f8fc3"
POST_SHA = "52fc48d9846dce42cb9481d803345d214c1eea6544d791fc36965992b36ff902"
EVENT_SHA = "2828bfa0b49198e6c79f8fa8b5b50a5067a02eec7bb8d91b9ea78ce8f79c2f2c"
CAPTURE_SIZE = 0xA0000
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class CPU(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in FIELDS]


class Write(ctypes.Structure):
    _fields_ = [("linear", ctypes.c_uint32), ("old_byte", ctypes.c_uint8),
                ("new_byte", ctypes.c_uint8)]


CALLBACK = ctypes.CFUNCTYPE(
    ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(CPU),
    ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
    ctypes.POINTER(CPU), ctypes.POINTER(Write), ctypes.c_size_t,
    ctypes.POINTER(ctypes.c_size_t))


def checked(path: Path, length: int, digest: str) -> bytes:
    data = path.read_bytes()
    assert (len(data), sha256(data).hexdigest()) == (length, digest), path
    return data


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + offset) & 0xFFFFF


def word(ram: bytes, segment: int, offset: int) -> int:
    p = physical(segment, offset)
    return ram[p] | (ram[p + 1] << 8)


def rows(path: Path) -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(path, 226, EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert len(lines) == 3 and all(len(line) == 17 for line in lines[1:])
    return {line[0]: {field.lower(): int(value, 16)
                      for field, value in zip(lines[0][1:], line[1:])}
            for line in lines[1:]}


def main() -> None:
    start = checked(START, 65231, START_SHA)
    checked(SAVE, 13137, SAVE_SHA)
    image, *_ = unpack(start)
    instruction = image[physical(0x0AA2, 0x028F):physical(0x0AA2, 0x0291)]
    assert (len(instruction), sha256(instruction).hexdigest()) == (2, INSTRUCTION_SHA)
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(instruction, 0x028F))
    assert [(i.address, i.mnemonic, i.op_str) for i in decoded] == [
        (0x028F, "int", "0x21")]

    pre = checked(RUN / "ram-028f-pre.bin", CAPTURE_SIZE, PRE_SHA)
    post = checked(RUN / "ram-0291-post.bin", CAPTURE_SIZE, POST_SHA)
    assert pre[physical(0x12C6, 0x028F):physical(0x12C6, 0x0291)] == instruction
    native = rows(RUN / "ev-numeric.tsv")
    before, after = native["pre"], native["post"]
    assert (before["cs"], before["ip"], after["cs"], after["ip"]) == (
        0x12C6, 0x028F, 0x12C6, 0x0291)
    assert (before["ax"], before["bx"], before["cx"], before["dx"]) == (
        0x4200, 5, 2, 0xAA87)
    assert before["fs"] == before["gs"] == after["fs"] == after["gs"] == 0
    assert (word(pre, before["ds"], 0x4426),
            word(post, after["ds"], 0x4426)) == (0, 0)

    differences = [(i, a, b) for i, (a, b) in enumerate(zip(pre, post))
                   if a != b]
    assert len(differences) == 26
    assert [i for i, _, _ in differences] == [
        *range(0x0816E, 0x08170), *range(0x208B8, 0x208D0)]
    journal_digest = sha256(b"".join(struct.pack("<IBB", *entry)
                                     for entry in differences)).hexdigest()

    with tempfile.TemporaryDirectory(prefix="poolrad-seek-028f-") as directory:
        library = Path(directory) / "seek.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_seek_028f.c"),
                        "-o", str(library)], check=True)
        step = ctypes.CDLL(str(library)).pr_resident_loader_seek_028f
        step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(CPU), CALLBACK, ctypes.c_void_p,
                         ctypes.POINTER(Write), ctypes.c_size_t]
        step.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * RAM_SIZE)()
        ctypes.memmove(ram, pre, CAPTURE_SIZE)
        cpu = CPU(*(before[field] for field in FIELDS))
        writes = (Write * len(differences))()
        calls = 0

        with tempfile.TemporaryFile() as host_file:
            @CALLBACK
            def service(_context, input_cpu, _ram, ram_size, output_cpu,
                        output_writes, capacity, count):
                nonlocal calls
                calls += 1
                c = input_cpu.contents
                assert ram_size == RAM_SIZE and capacity >= len(differences)
                assert c.ax == 0x4200 and c.bx == before["bx"]
                requested = (c.cx << 16) | c.dx
                position = os.lseek(host_file.fileno(), requested, os.SEEK_SET)
                ctypes.memmove(output_cpu, input_cpu, ctypes.sizeof(CPU))
                output_cpu.contents.ax = position & 0xFFFF
                output_cpu.contents.dx = (position >> 16) & 0xFFFF
                output_cpu.contents.flags &= 0xFFFE
                output_cpu.contents.ip = 0x0291
                # DOS's guest-memory writes are an external service event.
                # Replay the independently captured ordered journal; this
                # audit does not claim to synthesize DOS kernel internals.
                for index, (linear, old, new) in enumerate(differences):
                    output_writes[index] = Write(linear, old, new)
                count[0] = len(differences)
                return 0

            assert step(ram, RAM_SIZE, ctypes.byref(cpu), service, None,
                        writes, len(differences)) == 0

        assert calls == 1
        assert bytes(ram)[:CAPTURE_SIZE] == post
        assert all(getattr(cpu, field) == after[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("0AA2/12C6:+028F..0290: 2 bytes, 1 instruction, SHA-256", INSTRUCTION_SHA)
    print("native +028F -> +0291: 26 guest-RAM changes in 2 regions")
    print("native service journal SHA-256", journal_digest)
    print("host seek-derived DX:AX, captured event, 14 CPU fields and 640 KiB RAM parity")
    print("IOResult word DS:4426 unchanged; later reads/returns outside boundary")


if __name__ == "__main__":
    main()
