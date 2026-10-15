#!/usr/bin/env python3
"""Full original-byte RAM/CPU parity for GAME.OVR U3B0 +0711."""

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
BODY_SHA = "a3510458227e9373193717033475fa31fe5d1bf0ef01a33da6582726d6650092"
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


def put_node(ram, node, flag, next_node):
    off, seg = node
    ram[physical(seg, off + 0x29)] = flag
    put16(ram, seg, off + 0x2A, next_node[0])
    put16(ram, seg, off + 0x2C, next_node[1])


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    body = ovr[0x2CFE0:0x2D073]
    assert len(body) == 147 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0711))
    assert len(ins) == 52 and sum(i.size for i in ins) == 147
    assert ins[0].address == 0x0711 and ins[-1].address == 0x07A1
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "8"
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    return body


def make_case(body, mode, bound, sp):
    ram = bytearray(RAM_SIZE)
    ram[physical(0xF000, 0x0711):physical(0xF000, 0x0711) + len(body)] = body
    ss = 0x7000
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x0711, 0x6000, 0x8000, ss,
              0x0202 if sp == 0x8000 else 0x0AD7)
    for i in range(-32, 24):
        ram[physical(ss, sp + i)] = (i * 19 + mode * 23 + bound) & 255
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    put16(ram, ss, sp + 4, 0xA5A5)
    a, b, c = (0x2200, 0x5000), (0x2300, 0x5000), (0x2400, 0x5000)
    if mode == 0:
        a = (0, 0)
    elif mode == 5:
        a = ((sp - 0x37) & 0xFFFF, ss)
    elif mode == 6:
        a = (0xFFD8, 0x5000)
    elif mode == 7:
        b = (0x2210, 0x4FFF)  # Physical alias of A.
    elif mode not in (1, 2, 3, 4):
        raise ValueError(mode)
    if mode != 0:
        flags = {
            1: (0, 1, 0), 2: (1, 0, 1), 3: (0, 0, 0),
            4: (1, 1, 1), 5: (1, 0, 1), 6: (1, 0, 1),
            7: (0, 1, 0),
        }[mode]
        put_node(ram, a, flags[0], b)
        put_node(ram, b, flags[1], c)
        put_node(ram, c, flags[2], a if mode in (3, 4) else (0, 0))
    put16(ram, ss, sp + 6, a[0])
    put16(ram, ss, sp + 8, a[1])
    put16(ram, ss, sp + 10, bound)
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
        shared = Path(temp) / "u3b0_0711.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_leaf_0711.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_leaf_0711
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, bound, sp)
                 for mode in range(8)
                 for bound in (0, 1, 2, 3, 4, 0x8000, 0xFFFF)
                 for sp in (0x8000, 0xFFFB)]
        for case in cases:
            compare_case(body, model, case)
    print(f"U3B0 +0711 original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
