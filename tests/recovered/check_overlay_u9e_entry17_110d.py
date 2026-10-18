#!/usr/bin/env python3
"""Full original-byte RAM/CPU parity for GAME.OVR U9E entry 17."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEAF_SHA = "63aaf31a1bb13ac3a652db1b1fef79afa6ddf2aee869fe426bc4bda7e5395830"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(memory, seg, off, value):
    at = physical(seg, off)
    memory[at] = value & 255
    memory[(at + 1) & (RAM_SIZE - 1)] = (value >> 8) & 255


def put_far_contiguous(memory, seg, off, ptr_off, ptr_seg):
    at = physical(seg, off)
    for i, value in enumerate((ptr_off & 255, ptr_off >> 8,
                               ptr_seg & 255, ptr_seg >> 8)):
        memory[(at + i) & (RAM_SIZE - 1)] = value


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    leaf = ovr[0x2BFF7:0x2C046]
    assert len(leaf) == 79 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0x110D))
    assert len(ins) == 29 and sum(x.size for x in ins) == 79
    assert ins[0].address == 0x110D and ins[-1].address == 0x1159
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "0xc"
    return leaf


def make_case(leaf, mode, source_byte, ss, sp, flags):
    memory = bytearray(RAM_SIZE)
    code_at = physical(0xF000, 0x110D)
    memory[code_at:code_at + len(leaf)] = leaf
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x110D, 0x7000, 0x8000, ss, flags)
    for i in range(-4, 26):
        memory[physical(ss, (sp + i) & 0xFFFF)] = (i * 31 + source_byte) & 255
    put16(memory, ss, sp, 0x1234)
    put16(memory, ss, (sp + 2) & 0xFFFF, 0xD000)
    bp = (sp - 2) & 0xFFFF
    a = (0x2200, 0x5000)
    b = (0x2300, 0x5000)
    c = (0x2400, 0x5000)
    if mode == 1:  # A and B share the same target byte.
        b = a
    elif mode == 2:  # A clears the source byte before it is read.
        a = (c[0] + 3, c[1])
    elif mode == 3:  # B is also the source byte.
        b = (c[0] + 3, c[1])
    elif mode == 4:  # B writes its own far-pointer offset, then LES rereads.
        b = ((bp + 10) & 0xFFFF, ss)
    elif mode == 5:  # A clears B's far-pointer offset before the first B LES.
        a = ((bp + 10) & 0xFFFF, ss)
    elif mode == 6:  # Different segment:offset pairs alias one physical byte.
        b = (a[0] + 0x10, a[1] - 1)
    elif mode in (7, 8, 9):
        # Put one of the three LES operands at offset FFFF. This tests the
        # contiguous physical four-byte read against a wrapped second word.
        assert ((bp + (6, 10, 14)[mode - 7]) & 0xFFFF) == 0xFFFF
    elif mode != 0:
        raise ValueError(mode)
    for displacement, pointer in ((6, a), (10, b), (14, c)):
        put_far_contiguous(memory, ss, (bp + displacement) & 0xFFFF,
                           pointer[0], pointer[1])
    memory[physical(c[1], c[0] + 3)] = source_byte
    return memory, cpu


def compare_case(leaf, model, case):
    memory, cpu = make_case(leaf, *case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = []

    def on_code(emu, address, size, user):
        del size, user
        if emu.reg_read(UC_X86_REG_CS) != 0xF000:
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=65)
    assert len(reached) == 1, (case, reached)
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    assert model(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, case
    actual_ram = bytes(ram)
    if actual_ram != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((case, "RAM", [(hex(x), hex(actual_ram[x]),
                                             hex(expected_ram[x]))
                                            for x in diff[:12]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))


def main():
    leaf = source()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u9e_entry17.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u9e_entry17_110d.c"),
            "-o", str(shared),
        ], check=True)
        model = ctypes.CDLL(str(shared)).pr_u9e_entry17_110d
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, source_byte, ss, sp, flags)
                 for mode in range(7)
                 for source_byte in (0, 0x64, 0x65, 0x66, 0x7F, 0xE7, 0xFF)
                 for ss, sp, flags in ((0x7000, 0x8000, 0x0202),
                                      (0xE000, 0xFFFC, 0x0AD7))]
        cases += [(mode, source_byte, 0xE000, sp, 0x0002)
                  for mode, sp in ((7, 0xFFFB), (8, 0xFFF7), (9, 0xFFF3))
                  for source_byte in (0, 0x65, 0x66, 0xE7)]
        for case in cases:
            compare_case(leaf, model, case)
    print(f"U9E +110D original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
