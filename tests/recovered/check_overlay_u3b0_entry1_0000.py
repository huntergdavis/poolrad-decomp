#!/usr/bin/env python3
"""Full original-byte RAM/CPU parity for GAME.OVR U3B0 entry 1."""

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
LEAF_SHA = "a7d25485edf131a638e320475f22544c82fe81967478369e17556db2509b2a3c"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & MASK


def put16(memory, seg, off, value):
    at = physical(seg, off)
    memory[at] = value & 255
    memory[(at + 1) & MASK] = (value >> 8) & 255


def get16(memory, seg, off):
    at = physical(seg, off)
    return memory[at] | memory[(at + 1) & MASK] << 8


def put_next(memory, seg, off, next_off, next_seg):
    put16(memory, seg, off + 0x2A, next_off)
    put16(memory, seg, off + 0x2C, next_seg)


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    leaf = ovr[0x2C8CF:0x2C936]
    assert len(leaf) == 103 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0))
    assert len(ins) == 38 and sum(x.size for x in ins) == 103
    assert ins[0].address == 0 and ins[-1].address == 0x64
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "6"
    assert not any(x.mnemonic in ("call", "lcall") for x in ins)
    return leaf


def make_case(leaf, mode, target, ss, sp, flags):
    memory = bytearray(RAM_SIZE)
    code_at = physical(0xF000, 0)
    memory[code_at:code_at + len(leaf)] = leaf
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0, 0x6000, 0x8000, ss, flags)
    for i in range(-18, 18):
        memory[physical(ss, sp + i)] = (i * 19 + target) & 255
    put16(memory, ss, sp, 0x1234)
    put16(memory, ss, sp + 2, 0xD000)
    a = (0x2200, 0x5000)
    b = (0x2300, 0x5000)
    c = (0x2400, 0x5000)
    if mode == 0:  # Null head.
        a = (0, 0)
    elif mode == 3:  # First node lives beside local stack words.
        a = ((sp - 0x34) & 0xFFFF, ss)
    elif mode == 4:  # Node next-pointer offsets wrap within ES.
        a = (0xFFD8, 0x5000)
    elif mode == 5:  # Two far names alias the same physical node.
        b = (0x2210, 0x4FFF)
    elif mode not in (1, 2):
        raise ValueError(mode)
    if mode != 0:
        put_next(memory, *a, *b)
        put_next(memory, *b, *c)
        put_next(memory, *c, *(b if mode == 2 else (0, 0)))
    put16(memory, ss, sp + 4, a[0])
    put16(memory, ss, sp + 6, a[1])
    put16(memory, ss, sp + 8, target)
    assert get16(memory, ss, sp + 8) == target
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
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=1000)
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
        shared = Path(temp) / "u3b0_entry1.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry1_0000.c"),
            "-o", str(shared),
        ], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry1_0000
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, target, ss, sp, flags)
                 for mode in range(6)
                 for target in (0, 1, 2, 3, 4)
                 for ss, sp, flags in ((0x7000, 0x8000, 0x0202),
                                      (0x7000, 0xFFFB, 0x0AD7))]
        cases += [(mode, target, ss, sp, flags)
                  for mode in (0, 1, 4)
                  for target in (0x8000, 0xFFFF)
                  for ss, sp, flags in ((0x7000, 0x8000, 0x0202),
                                       (0x7000, 0xFFFB, 0x0AD7))]
        for case in cases:
            compare_case(leaf, model, case)
    print(f"U3B0 +0000 original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
