#!/usr/bin/env python3
"""Full original-byte RAM/CPU parity for GAME.OVR U9E entry 16."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEAF_SHA = "12ce30806a561605b7f7be238444a28a0d22f7cc53bda4872d6e0800330f41db"
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


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    leaf = ovr[0x2BFCE:0x2BFF7]
    assert len(leaf) == 41 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0x10E4))
    assert len(ins) == 17 and sum(x.size for x in ins) == 41
    assert ins[0].address == 0x10E4 and ins[-1].address == 0x110A
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "4"
    return leaf


def make_case(leaf, first, second, ss, sp, flags):
    memory = bytearray(RAM_SIZE)
    code_at = physical(0xF000, 0x10E4)
    memory[code_at:code_at + len(leaf)] = leaf
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x10E4, 0x7000, 0x8000, ss, flags)
    for i in range(-6, 14):
        memory[physical(ss, (sp + i) & 0xFFFF)] = (i * 31 + first) & 255
    for i, value in enumerate((0x1234, 0xD000,
                               0xA500 | first, 0xB600 | second)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    return memory, cpu


def compare_case(leaf, model, case):
    first, second, ss, sp, flags = case
    memory, cpu = make_case(leaf, *case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    target = physical(0xD000, 0x1234)
    reached = []

    def on_code(emu, address, size, user):
        del size, user
        if address == target:
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=30)
    assert reached == [target], (case, reached)
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
        shared = Path(temp) / "u9e_entry16.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u9e_entry16_10e4.c"),
            "-o", str(shared),
        ], check=True)
        model = ctypes.CDLL(str(shared)).pr_u9e_entry16_10e4
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        pairs = [(first, second) for first in (0, 1, 0x7F, 0xFE, 0xFF)
                 for second in (0, 0x11, 0x12, 0x13, 0x7F, 0x80, 0xFF)]
        cases = [(first, second, ss, sp, flags)
                 for first, second in pairs
                 for ss, sp, flags in ((0x7000, 0x8000, 0x0202),
                                      (0xE000, 0xFFFC, 0x0AD7))]
        for case in cases:
            compare_case(leaf, model, case)
    print(f"U9E +10E4 original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
