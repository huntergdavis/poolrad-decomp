#!/usr/bin/env python3
"""Static original/loaded-body and C stop audit for U9E entry8 +0E0E.

This uses controlled supplied frames, not a native reached frame or emulator.
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
CALL_SHA = "362d77932929304eb813cc915a525b9851526311ab34f6ad21a17b1533b9f402"
SYSTEM_SHA = "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9"
SEED_SHA = "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")


class Cpu(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 0xFF
    ram[(at + 1) & MASK] = value >> 8


def source(exe, ovr):
    assert sha256(exe).hexdigest() == EXE_SHA
    assert sha256(ovr).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x9E)
    assert unit["ovr_offset"] == 0x2AEEA
    assert unit["entries"][8:10] == [0x0DE3, 0x0E2E]
    entry = bytes(ovr[0x2BCCD:0x2BD18])
    call = bytes(ovr[0x2BCF8:0x2BCFD])
    system = bytes(exe[0xBE36:0xBE4C])
    seed = bytes(exe[0xBE85:0xBEBB])
    assert len(entry) == 75 and sha256(entry).hexdigest() == ENTRY_SHA
    assert len(call) == 5 and sha256(call).hexdigest() == CALL_SHA
    assert len(system) == 22 and sha256(system).hexdigest() == SYSTEM_SHA
    assert len(seed) == 54 and sha256(seed).hexdigest() == SEED_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(call, 0x0E0E))
    assert len(ins) == 1 and ins[0].address == 0x0E0E
    assert ins[0].mnemonic == "lcall" and ins[0].op_str == "0xaf8, 0x11c0"
    assert exe[0xBEBB:0xBEBD] == b"\x05\x84"
    return entry, system, seed


def check_case(func, bodies, sp, range_word):
    entry, system, seed_body = bodies
    ram = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(range_word, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
              (sp + 6) & 0xFFFF, sp, 0x3000, 0x0E0E, 0x1000, 0x4000,
              0x2000, 0x7256)
    u9e_at = physical(cpu.cs, 0x0DE3)
    system_at = physical(0x0AF8, 0x11C0)
    seed_at = physical(0x0AF8, 0x120F)
    ram[u9e_at:u9e_at + len(entry)] = entry
    ram[system_at:system_at + len(system)] = system
    ram[seed_at:seed_at + len(seed_body)] = seed_body
    put16(ram, 0x0AF8, 0x1245, 0x8405)
    put16(ram, cpu.ss, sp, range_word)
    put16(ram, cpu.ds, 0x4428, 0x2345)
    put16(ram, cpu.ds, 0x442A, 0x6789)
    if sp == 0x0002:
        linear_cs = (physical(cpu.ss, (sp - 4) & 0xFFFF) + 2) & MASK
        ram[linear_cs] = 0xAD
        ram[(linear_cs + 1) & MASK] = 0xDE
    expected = bytearray(ram)
    put16(expected, cpu.ss, (sp - 2) & 0xFFFF, cpu.cs)
    put16(expected, cpu.ss, (sp - 4) & 0xFFFF, 0x0E13)
    put16(expected, cpu.ss, (sp - 6) & 0xFFFF, 0x11C3)
    new_seed = (0x67892345 * 0x08088405 + 1) & 0xFFFFFFFF
    random_word = new_seed >> 16
    put16(expected, cpu.ds, 0x4428, new_seed & 0xFFFF)
    put16(expected, cpu.ds, 0x442A, random_word)
    live = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert bytes(live[u9e_at:u9e_at + len(entry)]) == entry
    assert bytes(live[system_at:system_at + len(system)]) == system
    assert bytes(live[seed_at:seed_at + len(seed_body)]) == seed_body
    status = func(live, RAM_SIZE, ctypes.byref(cpu))
    assert status == 0, status
    assert bytes(live) == expected
    assert cpu.ip == 0x0E13 and cpu.cs == 0x3000
    assert cpu.sp == (sp + 2) & 0xFFFF
    assert cpu.ax == (random_word % range_word if range_word else 0)
    assert cpu.dx == (random_word // range_word if range_word else random_word)
    assert cpu.bx == range_word and cpu.bp == ((sp + 6) & 0xFFFF)
    assert (cpu.si, cpu.di, cpu.ds, cpu.es, cpu.ss) == (
        0x5555, 0x6666, 0x1000, 0x4000, 0x2000)
    print(f"PASS SP={sp:04X} range={range_word}: loaded bodies and C stop 0E13")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--ovr", required=True, type=Path)
    args = parser.parse_args()
    bodies = source(args.exe.read_bytes(), args.ovr.read_bytes())
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-e0e-static-") as tmp:
        library = Path(tmp) / "call.so"
        sources = ("overlay_u9e_entry8_system_call_0e0e.c",
                   "system_random_fullstate_11c0.c",
                   "system_seed_fullstate_120f.c")
        subprocess.run(("cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        *(str(ROOT / "src/recovered" / name) for name in sources),
                        "-o", str(library)), check=True)
        func = ctypes.CDLL(str(library)).pr_u9e_entry8_system_call_0e0e
        func.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu))
        func.restype = ctypes.c_int
        check_case(func, bodies, 0x7FF8, 7)
        check_case(func, bodies, 0x0002, 5)
    print("PASS original/loaded call audit; no native parity claim")


if __name__ == "__main__":
    main()
