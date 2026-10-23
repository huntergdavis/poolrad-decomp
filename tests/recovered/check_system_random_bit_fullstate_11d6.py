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
CS_SPLITS = {"wrap-fffe": 0xFFFE, "wrap-ffff": 0xFFFF}


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


def native(initial_ram, initial_cpu):
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(initial_ram))
    for field, reg in zip(FIELDS, REGS):
        emu.reg_write(reg, getattr(initial_cpu, field))
    reached = []
    pending_return = None

    def stop_after_return(uc, address, size, user):
        nonlocal pending_return
        del size, user
        cs = uc.reg_read(x.UC_X86_REG_CS)
        ip = uc.reg_read(x.UC_X86_REG_IP)
        # Observe the original exit independently of the C candidate. The
        # seed RET may transfer away instead of returning to +11D9.
        if pending_return == "far" or (pending_return == "near" and
                                       (cs, ip) != (initial_cpu.cs, 0x11D9)):
            reached.append(address)
            uc.emu_stop()
            return
        pending_return = None
        if cs == initial_cpu.cs:
            if ip == 0x11F2:
                pending_return = "far"
            elif ip == 0x1244:
                pending_return = "near"

    emu.hook_add(UC_HOOK_CODE, stop_after_return)
    emu.emu_start(physical(initial_cpu.cs, initial_cpu.ip), 0, count=500)
    assert len(reached) == 1, "original did not reach a return boundary"
    vals = [emu.reg_read(reg) & 0xFFFF for reg in REGS]
    return bytes(emu.mem_read(0, RAM_SIZE)), vals


def word(ram, at):
    return ram[at & (RAM_SIZE - 1)] | (ram[(at + 1) & (RAM_SIZE - 1)] << 8)


def compare(name, start, c_ram, c_cpu, native_ram, native_cpu):
    assert bytes(c_ram) == native_ram, (name, "RAM diff")
    split = name in CS_SPLITS
    if split:
        assert start.sp == CS_SPLITS[name], (name, "entry SP")
        wrapped = word(native_ram, physical(start.ss, start.sp + 2))
        contiguous = word(native_ram, physical(start.ss, start.sp) + 2)
        assert wrapped != contiguous, (name, "indistinguishable CS fixtures")
        assert c_cpu.cs == wrapped, (name, "C wrapped CS", hex(c_cpu.cs), hex(wrapped))
        assert native_cpu[FIELDS.index("cs")] == contiguous, (name, "Unicorn contiguous CS")
    for index, field in enumerate(FIELDS):
        if split and field == "cs":
            continue
        actual = getattr(c_cpu, field)
        expected = native_cpu[index]
        assert actual == expected, (name, field, hex(actual), hex(expected))


def build(directory, contiguous_mutant=False):
    source = ROOT / "src/recovered/system_random_bit_fullstate_11d6.c"
    if contiguous_mutant:
        text = source.read_text()
        old = "cpu->cs = read_word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));"
        new = """cpu->cs = (uint16_t)(
        ram[(physical(cpu->ss, cpu->sp) + 2u) & MASK] |
        ((uint16_t)ram[(physical(cpu->ss, cpu->sp) + 3u) & MASK] << 8));"""
        assert text.count(old) == 1, "contiguous-CS mutation site changed"
        source = directory / "contiguous_mutant.c"
        source.write_text(text.replace(old, new))
    library = directory / ("mutant.so" if contiguous_mutant else "candidate.so")
    subprocess.run(("cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/system_seed_fullstate_120f.c"),
                    str(source), "-o", str(library)), check=True)
    func = ctypes.CDLL(str(library)).pr_system_random_bit_fullstate_11d6
    func.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                     ctypes.POINTER(Cpu))
    func.restype = ctypes.c_int
    return func


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
        func = build(Path(tmp))
        split_oracles = []
        for name, low, high, sp, alias in cases:
            memory, start = make_case(bit_code, seed_code, low, high, sp, alias)
            c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
            c_cpu = Cpu(*(getattr(start, field) for field in FIELDS))
            status = func(c_ram, RAM_SIZE, ctypes.byref(c_cpu))
            assert status == (3 if name == "near-transfer" else 0), (name, status)
            native_ram, native_cpu = native(memory, start)
            compare(name, start, c_ram, c_cpu, native_ram, native_cpu)
            if name in CS_SPLITS:
                split_oracles.append((name, memory, start, native_ram, native_cpu))
            detail = "declared CS-only split; CPU13 including FLAGS exact" if name in CS_SPLITS else "CPU14 exact"
            print(f"PASS {name}: 1 MiB RAM + {detail}")
        assert len(split_oracles) == 2
        mutant = build(Path(tmp), contiguous_mutant=True)
        for name, memory, start, native_ram, native_cpu in split_oracles:
            c_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
            c_cpu = Cpu(*(getattr(start, field) for field in FIELDS))
            assert mutant(c_ram, RAM_SIZE, ctypes.byref(c_cpu)) == 0
            try:
                compare(name, start, c_ram, c_cpu, native_ram, native_cpu)
            except AssertionError as error:
                assert error.args[0][:2] == (name, "C wrapped CS"), error
            else:
                raise AssertionError((name, "contiguous-CS mutant accepted"))
            print(f"PASS negative control {name}: contiguous-CS C mutant rejected")
    print(f"PASS SYSTEM +11D6: {len(cases)} bounded cases, 2 CS splits, 2 mutant rejections; no FLAGS masks")


if __name__ == "__main__":
    main()
