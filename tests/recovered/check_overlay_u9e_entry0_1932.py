#!/usr/bin/env python3
"""Original-byte full-RAM and CPU parity for GAME.OVR U9E entry 0."""

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
BODY_SHA = "fc1e97b8fe4158fdf9a46de4d3a1b4a04602fb787f6793778bc6d2b7a758d780"
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


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = value >> 8


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    body = ovr[0x2C81C:0x2C821]
    assert len(body) == 5 and sha256(body).hexdigest() == BODY_SHA
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x1932))
    assert [(i.mnemonic, i.op_str) for i in instructions] == [
        ("push", "bp"), ("mov", "bp, sp"), ("pop", "bp"), ("retf", "")]
    return body


def compare_case(body, model, ss, sp, bp, flags):
    ram = bytearray(RAM_SIZE)
    cs = 0xF000
    ram[physical(cs, 0x1932):physical(cs, 0x1932) + len(body)] = body
    for offset in range(-12, 16):
        ram[physical(ss, sp + offset)] = (offset * 31 + bp) & 255
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    cpu = Cpu(0xA5CD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              bp, sp, cs, 0x1932, 0x6000, 0x8000, ss, flags)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(ram))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = []

    def stop_on_return(emu, address, size, user):
        del size, user
        if emu.reg_read(UC_X86_REG_CS) != cs:
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_on_return)
    original.emu_start(physical(cs, cpu.ip), RAM_SIZE, count=16)
    assert len(reached) == 1, (ss, sp, bp, flags, reached)
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert model(actual_ram, RAM_SIZE, ctypes.byref(cpu)) == 0
    if bytes(actual_ram) != expected_ram:
        different = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                     if a != b]
        raise AssertionError((ss, sp, bp, flags, "RAM", different[:12]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (ss, sp, bp, flags, name,
                                    hex(actual), hex(expected))


def main():
    body = source()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u9e_entry0.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u9e_entry0_1932.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u9e_entry0_1932
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(ss, sp, bp, flags)
                 for ss in (0x7000, 0xE000)
                 for sp in (0, 1, 2, 0x8000, 0xFFFB)
                 for bp in (0, 0x2468, 0xFFFF)
                 for flags in (0x0202, 0x0AD7)]
        for case in cases:
            compare_case(body, model, *case)
    print(f"U9E +1932 original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
