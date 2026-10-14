#!/usr/bin/env python3
"""Audit the bounded U3BD +07C7 reset against one private native run.

Original game inputs and RAM captures stay under ignored resources/.
"""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16


ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
RUN = ROOT / "resources/oracle-runs/slot-a-u3bd-loop-reset-07c7-p5-022"
BEFORE = RUN / "ram-07c7-before.bin"
AFTER = RUN / "ram-07e9-after.bin"
SOURCE = ROOT / "src/recovered/overlay_u3bd_loop_reset_07c7.c"
FILE_OFFSET = 0x2F0BE
LENGTH = 34
LOADED_OFFSET = (0x2D14 << 4) + 0x07C7
ORIGINAL_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "d9ba05c7b1926e3ec4fbd0be0e22de72e110c76470f325ab1fd8759993683c5f"
BEFORE_SHA = "68c6e52f51227c3cf75f347a076f84ddc81417ac91f0e8800fd7e1c687696bf7"
AFTER_SHA = "a577d7c11da8a602e9ba5cdf722f5c50a68bdde00b61ba6fb985d21c2c2ce104"
BEFORE_CPU = (0x0008, 0, 4, 0x000D, 0x3E68, 4, 0x3EB6, 0x3E9E,
              0x14A0, 0x3DA1, 0x1D16, 0x2D14, 0x7246)
AFTER_CPU = (0, 0, 4, 0x000D, 0x3E68, 4, 0x3EB6, 0x3E9E,
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

    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x07C7))
    assert len(insns) == 11
    assert insns[0].address == 0x07C7 and insns[-1].address == 0x07E6
    assert sum(ins.size for ins in insns) == LENGTH
    assert insns[-2].mnemonic == "jb" and insns[-2].op_str == "0x7e9"
    assert insns[-1].mnemonic == "jmp" and insns[-1].op_str == "0x8e8"

    changed = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    assert changed == [0x21004, 0x21005, 0x21006, 0x21007], changed

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-07c7-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(SOURCE), "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library))
        model.pr_u3bd_loop_reset_07c7.argtypes = [
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
            ctypes.POINTER(Frame), ctypes.POINTER(ctypes.c_uint16),
        ]
        model.pr_u3bd_loop_reset_07c7.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * 0x100000)()
        ctypes.memmove(ram, before, len(before))
        frame = Frame(*BEFORE_CPU)
        ip = ctypes.c_uint16(0x07C7)
        assert model.pr_u3bd_loop_reset_07c7(
            ram, len(ram), ctypes.byref(frame), ctypes.byref(ip)) == 1
        result = bytes(ram)[:len(after)]
        assert result == after
        assert tuple(getattr(frame, name) for name, _ in Frame._fields_) == AFTER_CPU
        assert ip.value == 0x07E9

    print("U3BD +07C7..+07E9: original 34-byte/11-instruction hash locked")
    print("same-run native/C: 640 KiB RAM, 13 CPU fields and IP exact")
    print("changed RAM addresses: 0x21004..0x21007; reached +07E9")


if __name__ == "__main__":
    main()
