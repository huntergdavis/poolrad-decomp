#!/usr/bin/env python3
"""Original-byte full RAM/CPU parity for DOS GAME.OVR U38 +2002."""

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
BODY_SHA = "cdf3d522f6de4b8f37ccc47e3267e244f26cab4866883306b9dcfe1525dd2af4"
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


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    body = ovr[0x10C5D:0x10D3A]
    assert len(body) == 221 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2002))
    assert len(ins) == 75 and sum(i.size for i in ins) == len(body)
    assert ins[0].address == 0x2002 and ins[-1].address == 0x20DC
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "2"
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    addresses = {i.address for i in ins}
    assert all(int(i.op_str, 16) in addresses
               for i in ins if i.mnemonic.startswith("j"))
    return body


def make_case(body, mode, sp, seed):
    ram = bytearray(RAM_SIZE)
    ram[physical(0xF000, 0x2002):physical(0xF000, 0x2002) + len(body)] = body
    ss, base, cells = 0x7000, 0x9000, 0x2000
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x2002, 0x6000, 0x8000, ss,
              0x0202 if seed == 0 else 0x0AD7)
    for i in range(-20, 20):
        ram[physical(ss, sp + i)] = (i * 19 + seed * 23 + mode * 7) & 255
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    put16(ram, ss, sp + 4, base)
    put16(ram, ss, sp + 6, 0xE55E)
    put16(ram, ss, base + 8, cells)
    put16(ram, ss, base + 10, 0x5000)
    patterns = (
        [2, 3, 4, 5, 6, 7, 8, 9],
        [2] * 8,
        [2] * 8,
        [2, 0xFF, 1, 2, 0x80, 3, 1, 2],
        [0] * 8,
        [2, 3, 2, 3, 2, 3, 2, 3],
        [2] * 8,
        [0x80, 2, 1, 0, 2, 2, 3, 1],
    )
    for i, value in enumerate(patterns[mode]):
        ram[physical(0x5000, cells + 0x96 + i)] = value
        score = (i + 1 if mode == 0 else
                 8 - i if mode == 1 else
                 0xFFFF if mode == 2 and i == 5 else
                 (i * 11 + seed * 5) & 0xFFFF)
        high = (0xFFFF if mode == 2 and i == 5 else
                0x8000 if mode == 5 and i % 2 else
                0 if mode != 6 else i & 1)
        put16(ram, cpu.ds, 0x409B + i * 0x38 + value * 4, score)
        put16(ram, cpu.ds, 0x409D + i * 0x38 + value * 4, high)
    return ram, cpu


def compare_case(body, model, case):
    ram, cpu = make_case(body, *case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(ram))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached = []

    def stop_on_return(emu, address, size, user):
        del size, user
        if emu.reg_read(UC_X86_REG_CS) != 0xF000:
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_on_return)
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=3000)
    assert len(reached) == 1, (case, reached)
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


def main():
    body = source()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u38_2002.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u38_leaf_2002.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u38_leaf_2002
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, sp, seed)
                 for mode in range(8)
                 for sp in (0x8000, 0xFFF8)
                 for seed in (0, 1)]
        for case in cases:
            compare_case(body, model, case)
    print(f"U38 +2002 original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
