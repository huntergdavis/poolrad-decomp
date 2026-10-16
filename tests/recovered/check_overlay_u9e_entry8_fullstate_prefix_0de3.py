#!/usr/bin/env python3
"""Static original/loaded-byte and C stop audit for U9E entry8 prefix.

Manual gate: --exe and --ovr point to ignored owner media. No emulator runs.
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
PREFIX_SHA = "d60bca0e93dde6af6abab280714fe496fa6fe7f9f2c0e5deb3dba528b6d5665f"
RAM_SIZE = 1 << 20
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class Cpu(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & (RAM_SIZE - 1)] = (value >> 8) & 255


def source(exe, ovr):
    assert sha256(exe).hexdigest() == EXE_SHA
    assert sha256(ovr).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x9E)
    assert unit["ovr_offset"] == 0x2AEEA
    assert unit["entries"][8:10] == [0x0DE3, 0x0E2E]
    assert unit["memory_segment"] == 0xB0
    entry = bytes(ovr[0x2BCCD:0x2BD18])
    prefix = entry[:0x0E0E - 0x0DE3]
    assert len(entry) == 75 and sha256(entry).hexdigest() == ENTRY_SHA
    assert len(prefix) == 43 and sha256(prefix).hexdigest() == PREFIX_SHA
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    all_ins = list(decoder.disasm(entry, 0x0DE3))
    pre_ins = list(decoder.disasm(prefix, 0x0DE3))
    assert len(all_ins) == 29 and sum(i.size for i in all_ins) == 75
    assert len(pre_ins) == 17 and sum(i.size for i in pre_ins) == 43
    assert pre_ins[-1].address == 0x0E0D and pre_ins[-1].mnemonic == "push"
    by_addr = {i.address: i for i in all_ins}
    assert by_addr[0x0DF1].mnemonic == "jbe" and by_addr[0x0DF1].op_str == "0xe1f"
    assert by_addr[0x0E0E].mnemonic == "lcall" and by_addr[0x0E0E].op_str == "0xaf8, 0x11c0"
    assert by_addr[0x0E2B].mnemonic == "retf" and by_addr[0x0E2B].op_str == "4"
    return entry


def check_case(func, entry, count):
    ram = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(0xAB55, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
              0x7777, 0x8000, 0x3000, 0x0DE3, 0x1000, 0x4000,
              0x2000, 0x7212)
    code_at = physical(cpu.cs, 0x0DE3)
    ram[code_at:code_at + len(entry)] = entry
    assert bytes(ram[code_at:code_at + 43]) == entry[:43]
    put16(ram, cpu.ss, cpu.sp, 0x0FE3)
    put16(ram, cpu.ss, cpu.sp + 2, 0xBEEF)
    put16(ram, cpu.ss, cpu.sp + 4, 0x9913)
    put16(ram, cpu.ss, cpu.sp + 6, count)
    live = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert bytes(live[code_at:code_at + 75]) == entry
    status = func(live, RAM_SIZE, ctypes.byref(cpu))
    assert status == (1 if count == 0 else 0), (count, status)
    assert cpu.ip == (0x0E1F if count == 0 else 0x0E0E)
    assert bytes(live[code_at:code_at + 75]) == entry
    changed = [i for i, (a, b) in enumerate(zip(ram, live)) if a != b]
    expected = (3 if count == 0 else 7)
    assert len(changed) <= expected, (count, changed)
    print(f"PASS count={count}: pinned loaded 75B unchanged; C stop {cpu.ip:04X}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    args = parser.parse_args()
    entry = source(args.exe.read_bytes(), args.ovr.read_bytes())
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-de3-static-") as tmp:
        library = Path(tmp) / "prefix.so"
        subprocess.run(("cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u9e_entry8_fullstate_prefix_0de3.c"),
                        "-o", str(library)), check=True)
        func = ctypes.CDLL(str(library)).pr_u9e_entry8_fullstate_prefix_0de3
        func.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu))
        func.restype = ctypes.c_int
        check_case(func, entry, 0)
        check_case(func, entry, 1)
        check_case(func, entry, 0xEE02)
    print("PASS original/loaded byte audit; no emulator or native parity claim")


if __name__ == "__main__":
    main()
