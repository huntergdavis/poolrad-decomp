#!/usr/bin/env python3
"""Hash-locked native/C audit for saved resident DOS read +029F.

Reads ignored DOS/RAM inputs; prints only addresses, hashes, and counts.
Run with PYTHONPATH=/tmp/poolrad-capstone:tools/overlay.
"""

from __future__ import annotations

import ctypes
from hashlib import sha256
from pathlib import Path
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-loader-dos-read-029f-010"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
INSTRUCTION_SHA = "855d16faaa027a1c2bf57ebdc2ca53925d942f65bf436cbaf69672c4c8227a8c"
PRE_SHA = "18a41c5e0e6f818ff57dcb79b759516abcce1dceac29c962037b15bb428fb665"
POST_SHA = "bdeee778d6df28218adc7bd684db9a683e6fe6318923a7422056a72f379b5280"
EVENT_SHA = "0b890e9cd2f48622738739355618dfe9618c6998b23962aeb2cdb6ea2ab2659b"
BUFFER_SHA = "c3badf3541d1395ac104a7441595b92f62628fd910081756f7b8973cc1185933"
CAPTURE_SIZE = 0xA0000
RAM_SIZE = 1 << 20
FILE_CURSOR = 0x2AA87  # prior reached +028F seek result
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class CPU(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


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
    low = physical(segment, offset)
    high = physical(segment, (offset + 1) & 0xFFFF)
    return ram[low] | (ram[high] << 8)


def rows() -> dict[str, dict[str, int]]:
    lines = [line.split() for line in checked(RUN / "ev-numeric.tsv", 226,
                                               EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert len(lines) == 3 and all(len(line) == 17 for line in lines[1:])
    return {line[0]: {key.lower(): int(value, 16)
                      for key, value in zip(lines[0][1:], line[1:])}
            for line in lines[1:]}


def main() -> None:
    start = checked(RUN / "POOLRAD/START.EXE", 65231, START_SHA)
    game = checked(RUN / "POOLRAD/GAME.OVR", 206105, GAME_SHA)
    checked(RUN / "POOLRAD/SAVGAMA.DAT", 13137, SAVE_SHA)
    image, *_ = unpack(start)
    instruction = image[physical(0x0AA2, 0x029F):physical(0x0AA2, 0x02A1)]
    assert (len(instruction), sha256(instruction).hexdigest()) == (
        2, INSTRUCTION_SHA)
    assert [(i.address, i.mnemonic, i.op_str) for i in
            Cs(CS_ARCH_X86, CS_MODE_16).disasm(instruction, 0x029F)] == [
                (0x029F, "int", "0x21")]

    pre = checked(RUN / "ram-029f-pre.bin", CAPTURE_SIZE, PRE_SHA)
    post = checked(RUN / "ram-02a1-post.bin", CAPTURE_SIZE, POST_SHA)
    assert pre[physical(0x12C6, 0x029F):physical(0x12C6, 0x02A1)] == instruction
    native = rows()
    before, after = native["pre"], native["post"]
    assert (before["cs"], before["ip"], after["cs"], after["ip"]) == (
        0x12C6, 0x029F, 0x12C6, 0x02A1)
    assert before["ax"] >> 8 == 0x3F and before["bx"] == 5
    assert before["fs"] == before["gs"] == after["fs"] == after["gs"] == 0
    assert (word(pre, 0x14A0, 0x4426),
            word(post, 0x14A0, 0x4426)) == (0, 0)

    base = physical(before["ds"], before["dx"])
    count = before["cx"]
    assert (base, count) == (0x2CCD0, 0x0463)
    with (RUN / "POOLRAD/GAME.OVR").open("rb") as source:
        source.seek(FILE_CURSOR)
        payload = source.read(count)
    assert payload == game[FILE_CURSOR:FILE_CURSOR + count]
    assert (len(payload), sha256(payload).hexdigest()) == (1123, BUFFER_SHA)
    assert post[base:base + count] == payload

    changed = [(i, a, b) for i, (a, b) in enumerate(zip(pre, post)) if a != b]
    outside = [entry for entry in changed
               if not base <= entry[0] < base + count]
    assert len(changed) == 1122 and len(outside) == 10
    assert all(0x208B8 <= i <= 0x208CE for i, _, _ in outside)
    assert len(changed) - len(outside) == 1112

    # The buffer journal is derived from the independently pinned source
    # file, including same-value stores. Only ten DOS-internal changes use
    # captured opaque service data; no engine-level source is inferred.
    event = [(base + i, pre[base + i], value)
             for i, value in enumerate(payload)] + outside
    event_digest = sha256(b"".join(struct.pack("<IBB", *entry)
                                   for entry in event)).hexdigest()

    with tempfile.TemporaryDirectory(prefix="poolrad-read-029f-") as d:
        library = Path(d) / "read.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_loader_dos_read_029f.c"),
                        "-o", str(library)], check=True)
        step = ctypes.CDLL(str(library)).pr_resident_loader_dos_read_029f
        step.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(CPU), CALLBACK, ctypes.c_void_p,
                         ctypes.POINTER(Write), ctypes.c_size_t]
        step.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * RAM_SIZE)()
        ctypes.memmove(ram, pre, CAPTURE_SIZE)
        cpu = CPU(*(before[field] for field in FIELDS))
        journal = (Write * len(event))()
        calls = 0

        @CALLBACK
        def service(_context, input_cpu, _ram, ram_size, output_cpu,
                    output_writes, capacity, write_count):
            nonlocal calls
            calls += 1
            c = input_cpu.contents
            assert ram_size == RAM_SIZE and capacity >= len(event)
            assert c.ax >> 8 == 0x3F and c.bx == 5
            assert (c.ds, c.dx, c.cx) == (before["ds"], before["dx"], count)
            ctypes.memmove(output_cpu, input_cpu, ctypes.sizeof(CPU))
            output_cpu.contents.ax = len(payload)
            output_cpu.contents.flags &= 0xFFFE
            output_cpu.contents.ip = 0x02A1
            for i, (linear, old, new) in enumerate(event):
                output_writes[i] = Write(linear, old, new)
            write_count[0] = len(event)
            return 0

        assert step(ram, RAM_SIZE, ctypes.byref(cpu), service, None,
                    journal, len(event)) == 0
        assert calls == 1
        assert bytes(ram)[:CAPTURE_SIZE] == post
        assert all(getattr(cpu, field) == after[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("GAME.OVR SHA-256", GAME_SHA)
    print("original/loaded +029F..02A0: 2 bytes, 1 instruction, SHA-256",
          INSTRUCTION_SHA)
    print("native +029F -> +02A1: 1122 changed RAM bytes, 1112 in read buffer")
    print("GAME.OVR source slice: 1123 bytes, SHA-256", BUFFER_SHA)
    print("external service journal: 1133 writes, SHA-256", event_digest)
    print("C full captured 640 KiB RAM and 14 CPU-field event parity")
    print("IOResult word unchanged; following JB and later loader outside stage")


if __name__ == "__main__":
    main()
