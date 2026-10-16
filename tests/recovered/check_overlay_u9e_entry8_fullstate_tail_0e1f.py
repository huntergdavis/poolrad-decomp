#!/usr/bin/env python3
"""Original/loaded-byte static audit for U9E entry8 +0E1F shared RETF4 tail.

The caller supplies synthetic live RAM/CPU14; this does not run a native frame.
"""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
ENTRY_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"
TAIL_SHA = "480f3e495cbb8f3c29ac48422545fe45f39c92e58c01da4b5aa254d03e1a9c42"
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class Cpu(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def physical(segment, offset):
    return ((segment << 4) + (offset & 0xFFFF)) & (RAM_SIZE - 1)


def put16(ram, segment, offset, value):
    at = physical(segment, offset)
    ram[at] = value & 0xFF
    ram[(at + 1) & (RAM_SIZE - 1)] = value >> 8


def original(exe, ovr):
    assert sha256(exe).hexdigest() == EXE_SHA
    assert sha256(ovr).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x9E)
    assert unit["ovr_offset"] == 0x2AEEA
    assert unit["entries"][8:10] == [0x0DE3, 0x0E2E]
    entry = bytes(ovr[0x2BCCD:0x2BD18])
    tail = bytes(ovr[0x2BD09:0x2BD18])
    assert len(entry) == 75 and sha256(entry).hexdigest() == ENTRY_SHA
    assert len(tail) == 15 and sha256(tail).hexdigest() == TAIL_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(tail, 0x0E1F))
    assert len(ins) == 6 and sum(i.size for i in ins) == 15
    assert [i.address for i in ins] == [0x0E1F, 0x0E22, 0x0E25,
                                        0x0E28, 0x0E2A, 0x0E2B]
    assert [i.mnemonic for i in ins] == ["mov", "mov", "mov", "mov",
                                         "pop", "retf"]
    assert ins[-1].op_str == "4"
    return entry


def check_case(func, entry, bp):
    ram = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(0xABCD, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
              bp, 0x7FF8, 0x3000, 0x0E1F, 0x1000, 0x4000,
              0x2000, 0x7212)
    before = tuple(getattr(cpu, field) for field in FIELDS)
    code_at = physical(cpu.cs, 0x0DE3)
    ram[code_at:code_at + len(entry)] = entry
    ram[physical(cpu.ss, bp - 3)] = 0x12
    put16(ram, cpu.ss, bp, 0x7777)
    put16(ram, cpu.ss, (bp + 2) & 0xFFFF, 0x0FE3)
    put16(ram, cpu.ss, (bp + 4) & 0xFFFF, 0xBEEF)
    put16(ram, cpu.ss, (bp + 6) & 0xFFFF, 0x9913)
    put16(ram, cpu.ss, (bp + 8) & 0xFFFF, 1)
    if bp >= 0xFFFC:
        # False linear continuation of IP must not supply the wrapped CS.
        decoy = (physical(cpu.ss, (bp + 2) & 0xFFFF) + 2) & (RAM_SIZE - 1)
        ram[decoy] = 0xAD
        ram[(decoy + 1) & (RAM_SIZE - 1)] = 0xDE
    live = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert bytes(live[code_at:code_at + len(entry)]) == entry
    status = func(live, RAM_SIZE, ctypes.byref(cpu))
    assert status == 0, status
    assert bytes(live[code_at:code_at + len(entry)]) == entry
    expected = bytearray(ram)
    expected[physical(before[12], bp - 1)] = 0x12
    assert bytes(live) == expected
    actual_cpu = tuple(getattr(cpu, field) for field in FIELDS)
    expected_cpu = list(before)
    expected_cpu[0] = 0xAB12
    expected_cpu[6] = 0x7777
    expected_cpu[7] = (bp + 10) & 0xFFFF
    expected_cpu[8] = 0xBEEF
    expected_cpu[9] = 0x0FE3
    assert actual_cpu == tuple(expected_cpu), (actual_cpu, expected_cpu)
    print(f"PASS BP={bp:04X}: loaded body, full RAM/CPU14, wrapped RETF4")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    args = parser.parse_args()
    entry = original(args.exe.read_bytes(), args.ovr.read_bytes())
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-e1f-static-") as tmp:
        library = Path(tmp) / "tail.so"
        subprocess.run(("cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_tail_0e1f.c"),
                        "-o", str(library)), check=True)
        func = ctypes.CDLL(str(library)).pr_u9e_entry8_fullstate_tail_0e1f
        func.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu))
        func.restype = ctypes.c_int
        for bp in (0x8000, 0xFFFC, 0xFFFD):
            check_case(func, entry, bp)
    print("PASS original/loaded shared-tail audit; no native parity claim")


if __name__ == "__main__":
    main()
