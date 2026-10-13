#!/usr/bin/env python3
"""Hash-locked same-run native/C audit for saved loader +028F to caller.

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
RUN = ROOT / "resources/oracle-runs/slot-a-loader-composed-return-012"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EPILOGUE_SHA = "215e5153fb4987a2421c32058133392d054b5a365b2c7b6ba7eb7cc51618a992"
EVENT_SHA = "d1d232a9e94bb1b14ceadc27ca26157956b548228d94ee371b995eab55009365"
BUFFER_SHA = "c3badf3541d1395ac104a7441595b92f62628fd910081756f7b8973cc1185933"
RAM_SHAS = {
    "028f": "1583efcf9647a21a0cffde8886abd4c4799e3958f65da2bd043d9a8636719282",
    "0291": "d804f2081e95b2086bceab73cd0ba520fc754aba8027271da3ac782d1e777be9",
    "029f": "d804f2081e95b2086bceab73cd0ba520fc754aba8027271da3ac782d1e777be9",
    "02a1": "991396a012b20c4494063e7ef940832b7dabce071ef95e681ee0804eb993003d",
    "02e1": "991396a012b20c4494063e7ef940832b7dabce071ef95e681ee0804eb993003d",
    "03ae": "991396a012b20c4494063e7ef940832b7dabce071ef95e681ee0804eb993003d",
}
CAPTURE_SIZE = 0xA0000
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")
LABELS = ("seek_pre", "seek_post", "read_pre", "read_post",
          "epilogue_pre", "caller")
STOPS = ("028f", "0291", "029f", "02a1", "02e1", "03ae")


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


class Services(ctypes.Structure):
    _fields_ = [("seek", CALLBACK), ("seek_context", ctypes.c_void_p),
                ("seek_journal", ctypes.POINTER(Write)),
                ("seek_capacity", ctypes.c_size_t),
                ("read", CALLBACK), ("read_context", ctypes.c_void_p),
                ("read_journal", ctypes.POINTER(Write)),
                ("read_capacity", ctypes.c_size_t)]


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
    lines = [line.split() for line in checked(RUN / "ev-numeric.tsv", 487,
                                               EVENT_SHA).decode().splitlines()]
    assert lines[0] == "label CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS".split()
    assert len(lines) == 7 and all(len(line) == 17 for line in lines[1:])
    assert tuple(line[0] for line in lines[1:]) == LABELS
    return {line[0]: {key.lower(): int(value, 16)
                      for key, value in zip(lines[0][1:], line[1:])}
            for line in lines[1:]}


def changes(before: bytes, after: bytes) -> list[tuple[int, int, int]]:
    return [(i, a, b) for i, (a, b) in enumerate(zip(before, after)) if a != b]


def main() -> None:
    start = checked(RUN / "POOLRAD/START.EXE", 65231, START_SHA)
    game = checked(RUN / "POOLRAD/GAME.OVR", 206105, GAME_SHA)
    checked(RUN / "POOLRAD/SAVGAMA.DAT", 13137, SAVE_SHA)
    image, *_ = unpack(start)
    epilogue = image[physical(0x0AA2, 0x02E1):physical(0x0AA2, 0x02E5)]
    assert (len(epilogue), sha256(epilogue).hexdigest()) == (4, EPILOGUE_SHA)
    assert [(i.address, i.mnemonic, i.op_str) for i in
            Cs(CS_ARCH_X86, CS_MODE_16).disasm(epilogue, 0x02E1)] == [
                (0x02E1, "pop", "ds"), (0x02E2, "retf", "2")]

    ram_images = {stop: checked(RUN / f"ram-{stop}.bin", CAPTURE_SIZE,
                                RAM_SHAS[stop]) for stop in STOPS}
    assert ram_images["028f"][physical(0x12C6, 0x02E1):
                                    physical(0x12C6, 0x02E5)] == epilogue
    native = rows()
    for label, stop in zip(LABELS, STOPS):
        assert (native[label]["cs"], native[label]["ip"]) == (
            0x12C6, int(stop, 16))
        assert native[label]["fs"] == native[label]["gs"] == 0

    assert ram_images["0291"] == ram_images["029f"]
    assert ram_images["02a1"] == ram_images["02e1"] == ram_images["03ae"]
    seek_changes = changes(ram_images["028f"], ram_images["0291"])
    read_changes = changes(ram_images["029f"], ram_images["02a1"])
    assert len(seek_changes) == 26 and len(read_changes) == 1122
    assert all(0x0816E <= i <= 0x0816F or 0x208B8 <= i <= 0x208CF
               for i, _, _ in seek_changes)

    seek_in, seek_out = native["seek_pre"], native["seek_post"]
    assert (seek_in["ax"], seek_in["bx"], seek_in["cx"], seek_in["dx"]) == (
        0x4200, 5, 2, 0xAA87)
    cursor = (seek_in["cx"] << 16) | seek_in["dx"]
    assert cursor == 0x2AA87
    assert (seek_out["dx"] << 16) | seek_out["ax"] == cursor
    read_in, read_out = native["read_pre"], native["read_post"]
    assert (read_in["ax"] >> 8, read_in["bx"], read_in["cx"],
            read_in["ds"], read_in["dx"]) == (0x3F, 5, 0x463, 0x2CCD, 0)
    base = physical(read_in["ds"], read_in["dx"])
    payload = game[cursor:cursor + read_in["cx"]]
    assert (len(payload), sha256(payload).hexdigest()) == (1123, BUFFER_SHA)
    assert ram_images["02a1"][base:base + len(payload)] == payload
    outside = [entry for entry in read_changes
               if not base <= entry[0] < base + len(payload)]
    assert len(outside) == 10 and all(0x208B8 <= i <= 0x208CE
                                      for i, _, _ in outside)
    assert len(read_changes) - len(outside) == 1112
    read_event = [(base + i, ram_images["029f"][base + i], value)
                  for i, value in enumerate(payload)] + outside
    event_sha = sha256(b"".join(struct.pack("<IBB", *event)
                                for event in seek_changes + read_event)).hexdigest()

    epilogue_in, caller = native["epilogue_pre"], native["caller"]
    stack = ram_images["02e1"]
    assert (word(stack, epilogue_in["ss"], epilogue_in["sp"]),
            word(stack, epilogue_in["ss"], (epilogue_in["sp"] + 2) & 0xFFFF),
            word(stack, epilogue_in["ss"], (epilogue_in["sp"] + 4) & 0xFFFF)) == (
                0x14A0, 0x03AE, 0x12C6)
    assert (caller["ds"], caller["sp"], caller["ax"], caller["flags"]) == (
        0x14A0, 0x3778, 0, 0x7246)

    with tempfile.TemporaryDirectory(prefix="poolrad-composed-028f-") as d:
        library = Path(d) / "composed.so"
        sources = ("resident_loader_seek_028f.c",
                   "resident_loader_read_gate_0291.c",
                   "resident_loader_dos_read_029f.c",
                   "resident_loader_read_branch_02a1.c",
                   "resident_loader_composed_028f.c")
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        *(str(ROOT / "src/recovered" / source) for source in sources),
                        "-o", str(library)], check=True)
        run = ctypes.CDLL(str(library)).pr_resident_loader_composed_028f
        run.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                        ctypes.POINTER(CPU), ctypes.POINTER(Services)]
        run.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * RAM_SIZE)()
        ctypes.memmove(ram, ram_images["028f"], CAPTURE_SIZE)
        cpu = CPU(*(seek_in[field] for field in FIELDS))
        seek_journal = (Write * len(seek_changes))()
        read_journal = (Write * len(read_event))()
        calls = {"seek": 0, "read": 0}

        @CALLBACK
        def seek_service(_context, input_cpu, memory, ram_size,
                         output_cpu, writes, capacity, count):
            calls["seek"] += 1
            assert ram_size == RAM_SIZE and capacity >= len(seek_changes)
            assert bytes(memory[:CAPTURE_SIZE]) == ram_images["028f"]
            assert all(getattr(input_cpu.contents, field) == seek_in[field]
                       for field in FIELDS)
            ctypes.memmove(output_cpu, input_cpu, ctypes.sizeof(CPU))
            output_cpu.contents.ax = cursor & 0xFFFF
            output_cpu.contents.dx = cursor >> 16
            output_cpu.contents.flags &= 0xFFFE
            output_cpu.contents.ip = 0x0291
            for i, (linear, old, new) in enumerate(seek_changes):
                writes[i] = Write(linear, old, new)
            count[0] = len(seek_changes)
            return 0

        @CALLBACK
        def read_service(_context, input_cpu, memory, ram_size,
                         output_cpu, writes, capacity, count):
            calls["read"] += 1
            assert ram_size == RAM_SIZE and capacity >= len(read_event)
            assert bytes(memory[:CAPTURE_SIZE]) == ram_images["029f"]
            assert all(getattr(input_cpu.contents, field) == read_in[field]
                       for field in FIELDS)
            ctypes.memmove(output_cpu, input_cpu, ctypes.sizeof(CPU))
            output_cpu.contents.ax = len(payload)
            output_cpu.contents.flags &= 0xFFFE
            output_cpu.contents.ip = 0x02A1
            for i, (linear, old, new) in enumerate(read_event):
                writes[i] = Write(linear, old, new)
            count[0] = len(read_event)
            return 0

        services = Services(seek_service, None, seek_journal,
                            len(seek_changes), read_service, None,
                            read_journal, len(read_event))
        assert run(ram, RAM_SIZE, ctypes.byref(cpu), ctypes.byref(services)) == 0
        assert calls == {"seek": 1, "read": 1}
        assert bytes(ram)[:CAPTURE_SIZE] == ram_images["03ae"]
        assert all(getattr(cpu, field) == caller[field] for field in FIELDS)

    print("START SHA-256", START_SHA)
    print("GAME.OVR SHA-256", GAME_SHA)
    print("original/loaded +02E1..+02E4: 4 bytes, 2 instructions, SHA-256",
          EPILOGUE_SHA)
    print("same-run six stops: +028F, +0291, +029F, +02A1, +02E1, +03AE")
    print("native RAM net changes: seek 26; read 1122; setup/branch/return 0")
    print("host event journal: 26 seek + 1133 read writes, SHA-256", event_sha)
    print("caller 12C6:03AE, DS=14A0, SP=3778; C 14 CPU fields and 640 KiB RAM match")
    print("DOS internals are opaque; optional second read and caller body outside path")


if __name__ == "__main__":
    main()
