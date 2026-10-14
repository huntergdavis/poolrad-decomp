#!/usr/bin/env python3
"""Audit the bounded U3BD +07E9 call transfer against private native RAM."""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16


ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
RUN = ROOT / "resources/oracle-runs/slot-a-u3bd-successor-07e9-p5-026"
BEFORE = RUN / "ram-07e9-before.bin"
AFTER = RUN / "ram-047b-after.bin"
SOURCE = ROOT / "src/recovered/overlay_u3bd_first_call_07e9.c"
FILE_OFFSET = 0x2F0E0
LENGTH = 16
LOADED_OFFSET = (0x2D14 << 4) + 0x07E9
ORIGINAL_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "741850baf96f032d50cb9805332c95b3d41c0a95337a3372e755f608cd145a38"
BEFORE_SHA = "58a58dae33cf4cd30340c17c997659b7c5eb5cb382a038af3d2dcd254b1f43fc"
AFTER_SHA = "6c0df4179b35cea782d1fe87db44e645f022e926a5306d44d630b9014ee05c80"
BEFORE_CPU = (0, 0, 4, 0x000D, 0x3E68, 4, 0x3EB6, 0x3E9E,
              0x14A0, 0x3DA1, 0x1D16, 0x2D14, 0x7297)
AFTER_CPU = (6, 0, 4, 0x000D, 0x3E68, 4, 0x3EB6, 0x3E94,
             0x14A0, 0x3DA1, 0x1D16, 0x2D14, 0x7297)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Frame(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
                 "ds", "es", "ss", "cs", "flags")]


def main() -> None:
    original = ORIGINAL.read_bytes()
    before = BEFORE.read_bytes()
    after = AFTER.read_bytes()
    assert sha(original) == ORIGINAL_SHA
    assert len(before) == len(after) == 0xA0000
    assert sha(before) == BEFORE_SHA and sha(after) == AFTER_SHA
    span = original[FILE_OFFSET:FILE_OFFSET + LENGTH]
    assert len(span) == LENGTH and sha(span) == SPAN_SHA
    assert before[LOADED_OFFSET:LOADED_OFFSET + LENGTH] == span

    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x07E9))
    assert len(insns) == 8 and sum(ins.size for ins in insns) == LENGTH
    assert [ins.mnemonic for ins in insns] == [
        "mov", "push", "mov", "push", "mov", "push", "push", "call"
    ]
    assert insns[-1].address == 0x07F6 and insns[-1].op_str == "0x47b"

    changed = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    assert changed == list(range(0x20FF4, 0x20FFB)), changed
    ss_base = 0x1D16 << 4
    assert [before[ss_base + off] for off in (0x3EA9, 0x3EA7, 0x3EBC)] == [
        0x0D, 0x04, 0x06
    ]

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-07e9-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(SOURCE), "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library))
        model.pr_u3bd_first_call_07e9.argtypes = [
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
            ctypes.POINTER(Frame), ctypes.POINTER(ctypes.c_uint16),
        ]
        model.pr_u3bd_first_call_07e9.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * 0x100000)()
        ctypes.memmove(ram, before, len(before))
        frame = Frame(*BEFORE_CPU)
        ip = ctypes.c_uint16(0x07E9)
        assert model.pr_u3bd_first_call_07e9(
            ram, len(ram), ctypes.byref(frame), ctypes.byref(ip)) == 1
        assert bytes(ram)[:len(after)] == after
        assert tuple(getattr(frame, name) for name, _ in Frame._fields_) == AFTER_CPU
        assert ip.value == 0x047B

    print("U3BD +07E9..+07F9: original 16-byte/8-instruction hash locked")
    print("same-run native/C: 640 KiB RAM, 13 CPU fields and IP exact")
    print("changed RAM addresses: 0x20FF4..0x20FFA; reached +047B")


if __name__ == "__main__":
    main()
