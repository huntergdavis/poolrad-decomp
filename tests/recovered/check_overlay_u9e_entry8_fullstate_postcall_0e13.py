#!/usr/bin/env python3
"""Static original/loaded-byte and C stop audit for U9E entry8 +0E13.

Manual gate using ignored --exe/--ovr media; no emulator or natural frame.
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
SLICE_SHA = "ca9171f30531cf4c50426daac59c488b59c46622436ee26859f0f63155a66ba0"
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class Cpu(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def source(exe, ovr):
    assert sha256(exe).hexdigest() == EXE_SHA
    assert sha256(ovr).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x9E)
    assert unit["ovr_offset"] == 0x2AEEA
    assert unit["entries"][8:10] == [0x0DE3, 0x0E2E]
    entry = bytes(ovr[0x2BCCD:0x2BD18])
    part = bytes(ovr[0x2BCFD:0x2BD09])
    assert len(entry) == 75 and sha256(entry).hexdigest() == ENTRY_SHA
    assert len(part) == 12 and sha256(part).hexdigest() == SLICE_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(part, 0x0E13))
    assert len(ins) == 5 and sum(i.size for i in ins) == 12
    assert [i.address for i in ins] == [0x0E13, 0x0E14, 0x0E17,
                                        0x0E1A, 0x0E1D]
    assert ins[-1].mnemonic == "jne" and ins[-1].op_str == "0xe05"
    return entry


def check_case(func, entry, index, count, stop):
    ram = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(0x12FE, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
              0x8002, 0x7FF8, 0x3000, 0x0E13, 0x1000, 0x4000,
              0x2000, 0x7212)
    code_at = physical(cpu.cs, 0x0DE3)
    ram[code_at:code_at + 75] = entry
    ram[physical(cpu.ss, cpu.bp - 4)] = count
    ram[physical(cpu.ss, cpu.bp - 3)] = 0x10
    ram[physical(cpu.ss, cpu.bp - 2)] = index
    live = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert bytes(live[code_at:code_at + 75]) == entry
    status = func(live, RAM_SIZE, ctypes.byref(cpu))
    assert status == (1 if stop == 0x0E1F else 0), status
    assert cpu.ip == stop and cpu.ax == 0x1201
    assert bytes(live[code_at:code_at + 75]) == entry
    changed = [i for i, (a, b) in enumerate(zip(ram, live)) if a != b]
    assert changed == [physical(cpu.ss, cpu.bp - 3)], changed
    print(f"PASS index={index} count={count}: loaded body unchanged; C stop {stop:04X}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    args = parser.parse_args()
    entry = source(args.exe.read_bytes(), args.ovr.read_bytes())
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-e13-static-") as tmp:
        library = Path(tmp) / "postcall.so"
        subprocess.run(("cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_postcall_0e13.c"),
                        "-o", str(library)), check=True)
        func = ctypes.CDLL(str(library)).pr_u9e_entry8_fullstate_postcall_0e13
        func.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu))
        func.restype = ctypes.c_int
        check_case(func, entry, 1, 1, 0x0E1F)
        check_case(func, entry, 1, 2, 0x0E05)
    print("PASS original/loaded post-call audit; no native parity claim")


if __name__ == "__main__":
    main()
