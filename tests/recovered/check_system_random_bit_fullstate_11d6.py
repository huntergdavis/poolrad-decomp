#!/usr/bin/env python3
"""Bounded original-byte/strict-C RAM and CPU parity for SYSTEM +11D6.

Manual evidence gate: --exe points to ignored original START.EXE. This is not
part of CTest because it needs owner media and a scheduled native audit slot.
"""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn import x86_const as x

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from resident_loader_map import unpack  # noqa: E402

RAM_SIZE = 1 << 20
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "2fdaa8058f09e137bb95255831266138e17e594a39502eee7e515172cefd01ff"
SEED_SHA = "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242"
FIELDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp", "cs", "ip",
          "ds", "es", "ss", "flags")
REGS = (x.UC_X86_REG_AX, x.UC_X86_REG_BX, x.UC_X86_REG_CX,
        x.UC_X86_REG_DX, x.UC_X86_REG_SI, x.UC_X86_REG_DI,
        x.UC_X86_REG_BP, x.UC_X86_REG_SP, x.UC_X86_REG_CS,
        x.UC_X86_REG_IP, x.UC_X86_REG_DS, x.UC_X86_REG_ES,
        x.UC_X86_REG_SS, x.UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(field, ctypes.c_uint16) for field in FIELDS]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & (RAM_SIZE - 1)] = (value >> 8) & 255


def body(exe):
    assert sha256(exe).hexdigest() == EXE_SHA
    image, origin, *_ = unpack(exe)
    spans = ((0xC156, 0xC173, BODY_SHA, 13, 0xBE4C, 0xBE69),
             (0xC18F, 0xC1C5, SEED_SHA, 22, 0xBE85, 0xBEBB))
    result = []
    from capstone import CS_ARCH_X86, CS_MODE_16, Cs
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    for lo, hi, digest, count, file_lo, file_hi in spans:
        raw = bytes(image[lo:hi])
        ins = list(decoder.disasm(raw, lo - 0xAF80))
        assert sha256(raw).hexdigest() == digest
        assert len(ins) == count and sum(i.size for i in ins) == len(raw)
        assert origin[lo] + 0x200 == file_lo
        assert origin[hi - 1] + 0x201 == file_hi
        assert all(origin[p + 1] == origin[p] + 1 for p in range(lo, hi - 1))
        result.append(raw)
    return result


def make_case(bit_code, seed_code, low, high, sp=0x8000, alias=None):
    ram = bytearray([0xA5]) * RAM_SIZE
    cpu = Cpu(0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
              0x7777, sp, 0x3000, 0x11D6, 0x1000, 0x4000,
              0x1000 if alias else 0x2000, 0x7202)
    at = physical(cpu.cs, 0x11D6)
    ram[at:at + len(bit_code)] = bit_code
    at = physical(cpu.cs, 0x120F)
    ram[at:at + len(seed_code)] = seed_code
    put(ram, cpu.cs, 0x1245, 0x8405)
    put(ram, cpu.ds, 0x4428, low)
    put(ram, cpu.ds, 0x442A, high)
    put(ram, cpu.ss, cpu.sp, 0xBEEF)
    put(ram, cpu.ss, cpu.sp + 2, 0xCAFE)
    return ram, cpu


def native(initial_ram, initial_cpu, expected_cpu):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(initial_ram))
    for field, reg in zip(FIELDS, REGS):
        emu.reg_write(reg, getattr(initial_cpu, field))
    reached = []

    def stop_at_expected(uc, address, size, user):
        del size, user
        if (address == physical(expected_cpu.cs, expected_cpu.ip) and
            uc.reg_read(x.UC_X86_REG_CS) == expected_cpu.cs and
            uc.reg_read(x.UC_X86_REG_IP) == expected_cpu.ip):
            reached.append(address)
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, stop_at_expected)
    emu.emu_start(physical(initial_cpu.cs, initial_cpu.ip), 0, count=500)
    assert len(reached) == 1, "original did not reach C's transfer target"
    vals = [emu.reg_read(reg) & 0xFFFF for reg in REGS]
    return bytes(emu.mem_read(0, RAM_SIZE)), vals


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, type=Path)
    args = parser.parse_args()
    bit_code, seed_code = body(args.exe.read_bytes())
    cases = (("first-test", 0x5678, 0x1234, 0x8000, None),
             ("31-shifts", 0, 0, 0x8000, None),
             ("different-seed", 0x1357, 0x2468, 0x8000, None),
             ("wrap-fffe", 0x5678, 0x1234, 0xFFFE, None),
             ("wrap-ffff", 0x5678, 0x1234, 0xFFFF, None),
             ("far-frame-alias", 0x5678, 0x1234, 0x4428, "frame"),
             ("near-transfer", 0x5678, 0x1234, 0x442A, "near"))
    with tempfile.TemporaryDirectory(prefix="poolrad-11d6-") as tmp:
        library = Path(tmp) / "candidate.so"
        subprocess.run(("cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/system_seed_fullstate_120f.c"),
                        str(ROOT / "src/recovered/system_random_bit_fullstate_11d6.c"),
                        "-o", str(library)), check=True)
        func = ctypes.CDLL(str(library)).pr_system_random_bit_fullstate_11d6
        func.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu))
        func.restype = ctypes.c_int
        for name, low, high, sp, alias in cases:
            memory, start = make_case(bit_code, seed_code, low, high, sp, alias)
            c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
            c_cpu = Cpu(*(getattr(start, field) for field in FIELDS))
            status = func(c_ram, RAM_SIZE, ctypes.byref(c_cpu))
            assert status == (3 if name == "near-transfer" else 0), (name, status)
            native_ram, native_cpu = native(memory, start, c_cpu)
            assert bytes(c_ram) == native_ram, (name, "RAM diff")
            for index, field in enumerate(FIELDS):
                actual = getattr(c_cpu, field)
                expected = native_cpu[index]
                mask = 0xFFEF if field == "flags" else 0xFFFF
                assert ((actual ^ expected) & mask) == 0, (
                    name, field, hex(actual), hex(expected))
            print(f"PASS {name}: 1 MiB RAM + CPU14 (AF ignored)")
    print(f"PASS SYSTEM +11D6 original-byte/C {len(cases)} bounded cases")


if __name__ == "__main__":
    main()
