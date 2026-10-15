#!/usr/bin/env python3
"""Full-state original-byte parity at the first U3B0 entry4 SYSTEM call."""

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
PREFIX_SHA = "7ecaa950d8a79a4828f9f8106c7c57c890b34cc7c30d0cb39c404936d6cba427"
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
    ram[(at + 1) & MASK] = (value >> 8) & 255


def get16(ram, seg, off):
    at = physical(seg, off)
    return ram[at] | (ram[(at + 1) & MASK] << 8)


def original_prefix():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    body = ovr[0x2D669:0x2D67E]
    assert len(body) == 21 and sha256(body).hexdigest() == PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0D9A))
    assert len(ins) == 11 and sum(i.size for i in ins) == len(body)
    assert ins[0].address == 0x0D9A and ins[-1].address == 0x0DAE
    assert (ins[-1].mnemonic, ins[-1].op_str) == ("push", "ax")
    assert ovr[0x2D67E:0x2D683] == bytes.fromhex("9a250bf80a")
    return body


def make_case(body, sp, source, seed):
    ram = bytearray(RAM_SIZE)
    at = physical(0xF000, 0x0D9A)
    ram[at:at + len(body)] = body
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x0D9A, 0x6000, 0x8000, 0x7000,
              0x0202 if seed == 0 else 0x0AD7)
    for i in range(-0x1B0, 0x40):
        ram[physical(cpu.ss, sp + i)] = (i * 19 + seed * 23) & 255
    bp = (sp - 2) & 0xFFFF
    put16(ram, cpu.ss, sp, 0x1234)
    put16(ram, cpu.ss, sp + 2, 0xD000)
    put16(ram, cpu.ss, bp + 0x2A, source[0])
    put16(ram, cpu.ss, bp + 0x2C, source[1])
    if source == (0x1000, 0x5000):
        at = physical(0x5000, 0x1000)
        ram[at:at + 6] = b"\x05HELLO"
    return ram, cpu


def compare_case(body, model, case):
    ram, cpu = make_case(body, *case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(ram))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = []

    def stop_before_call(emu, address, size, user):
        del size, user
        if (emu.reg_read(UC_X86_REG_CS) & 0xFFFF,
                emu.reg_read(UC_X86_REG_IP) & 0xFFFF) == (0xF000, 0x0DAF):
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_before_call)
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=30)
    assert reached == [physical(0xF000, 0x0DAF)], (case, reached)
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert model(actual_ram, RAM_SIZE, ctypes.byref(cpu)) == 0, case
    if bytes(actual_ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((case, "RAM", [(hex(i), hex(actual_ram[i]),
                                            hex(expected_ram[i]))
                                           for i in diff[:12]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))
    sp, source, _ = case
    bp = (sp - 2) & 0xFFFF
    outgoing_sp = (bp - 0x018E - 10) & 0xFFFF
    assert cpu.sp == outgoing_sp and cpu.ip == 0x0DAF
    assert [get16(actual_ram, cpu.ss, outgoing_sp + i)
            for i in (0, 2, 4, 6, 8)] == [
                0x28, (bp - 0x2A) & 0xFFFF, cpu.ss,
                source[0], source[1]]


def main():
    body = original_prefix()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0d9a.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_prefix_0d9a.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_prefix_0d9a
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(sp, source, seed)
                 for sp in (0x8000, 0x0000, 0xFFFF, 0x01A0)
                 for source in ((0x1000, 0x5000), (0xFFFF, 0),
                                (0, 0x9000))
                 for seed in (0, 1)]
        for case in cases:
            compare_case(body, model, case)
    print(f"U3B0 +0D9A..+0DAE original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
