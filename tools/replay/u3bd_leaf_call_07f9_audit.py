#!/usr/bin/env python3
"""Audit the reached U3BD entry-5 local leaf call against private native RAM."""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16


ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
RUN = ROOT / "resources/oracle-runs/slot-a-u3bd-entry5-call-chain-p5-027"
BEFORE = RUN / "ram-07f9-before.bin"
LEAF_ENTRY = RUN / "ram-031f-entry.bin"
AFTER = RUN / "ram-080a-before-branch.bin"
SOURCE = ROOT / "src/recovered/overlay_u3bd_leaf_call_07f9.c"
UNIT_FILE_BASE = 0x2E8F7
ORIGINAL_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
CALLER_SHA = "f432c36103980b020edc3c7322183327367001f5b1c399631e3fe2d6b08b95c4"
LEAF_SHA = "ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b"
BEFORE_SHA = "0bf854b0b7f7324779cb6905f7ef70ce39d22e2efa39a111f0c748637bd74ad5"
LEAF_ENTRY_SHA = "e76369ece75277656911157e71f8789566f5ed29f0d598c50fe3bc3efb236ef8"
AFTER_SHA = "94656b315a93486dff325d530db006586b3e89b1c2fbc3b2d0903dc2c7d76e9e"
BEFORE_CPU = (0x000E, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E9E,
              0x14A0, 0x3DA1, 0x1D16, 0x2D14, 0x7202)
LEAF_ENTRY_CPU = (0x0004, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E96,
                  0x14A0, 0x3DA1, 0x1D16, 0x2D14, 0x7202)
AFTER_CPU = (0x0001, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E9E,
             0x14A0, 0x3DA1, 0x1D16, 0x2D14, 0x7202)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + (offset & 0xFFFF)) & 0xFFFFF


def word(ram: bytes, segment: int, offset: int) -> int:
    return ram[physical(segment, offset)] | (ram[physical(segment, offset + 1)] << 8)


class Frame(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
                 "ds", "es", "ss", "cs", "flags")]


def main() -> None:
    original = ORIGINAL.read_bytes()
    before = BEFORE.read_bytes()
    mid = LEAF_ENTRY.read_bytes()
    after = AFTER.read_bytes()
    assert sha(original) == ORIGINAL_SHA
    assert len(before) == len(mid) == len(after) == 0xA0000
    assert (sha(before), sha(mid), sha(after)) == (
        BEFORE_SHA, LEAF_ENTRY_SHA, AFTER_SHA
    )

    for start, end, expected_sha, expected_count in (
        (0x07F9, 0x080A, CALLER_SHA, 8),
        (0x031F, 0x0350, LEAF_SHA, 18),
    ):
        span = original[UNIT_FILE_BASE + start:UNIT_FILE_BASE + end]
        assert len(span) == end - start and sha(span) == expected_sha
        loaded = (0x2D14 << 4) + start
        assert before[loaded:loaded + len(span)] == span
        instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, start))
        assert len(instructions) == expected_count
        assert sum(ins.size for ins in instructions) == len(span)
    caller = original[UNIT_FILE_BASE + 0x07F9:UNIT_FILE_BASE + 0x080A]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(caller, 0x07F9))
    assert insns[-2].address == 0x0805 and insns[-2].op_str == "0x31f"
    assert insns[-1].address == 0x0808 and insns[-1].mnemonic == "or"

    ss = 0x1D16
    assert mid[physical(ss, 0x3EA2)] == 0x0E
    assert (word(mid, ss, 0x3E9C), word(mid, ss, 0x3E9A),
            word(mid, ss, 0x3E98), word(mid, ss, 0x3E96)) == (
        0x000D, 0x0004, 0x2D14, 0x0808
    )
    assert [i for i, (a, b) in enumerate(zip(before, mid)) if a != b] == [
        0x20FF6, 0x20FF7, 0x20FF8, 0x20FF9, 0x21002
    ]
    assert [i for i, (a, b) in enumerate(zip(mid, after)) if a != b] == [
        0x20FF3, 0x20FF4, 0x20FF5
    ]

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-07f9-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(SOURCE), "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library))
        model.pr_u3bd_leaf_call_07f9.argtypes = [
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
            ctypes.POINTER(Frame), ctypes.POINTER(ctypes.c_uint16),
        ]
        model.pr_u3bd_leaf_call_07f9.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * 0x100000)()
        ctypes.memmove(ram, before, len(before))
        frame = Frame(*BEFORE_CPU)
        ip = ctypes.c_uint16(0x07F9)
        assert model.pr_u3bd_leaf_call_07f9(
            ram, len(ram), ctypes.byref(frame), ctypes.byref(ip)) == 1
        assert bytes(ram)[:len(after)] == after
        assert tuple(getattr(frame, name) for name, _ in Frame._fields_) == AFTER_CPU
        assert ip.value == 0x080A

    print("U3BD +07F9..+080A: caller 17 bytes/8 instructions hash locked")
    print("U3BD +031F..+0350: local leaf 49 bytes/18 instructions hash locked")
    print("same-run native/C: 640 KiB RAM exact")
    print("CPU tuple/IP match manually observed EV stops; no saved EV transcript")
    print("saved pre-branch ZF clear; +080A JNE not executed by this model")


if __name__ == "__main__":
    main()
