#!/usr/bin/env python3
"""Original-byte full RAM and CPU parity for GAME.OVR U3C2 entry 3."""

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
LEAF_SHA = "818132a151e78bff884848a874203a09a50b6d459e0ad9f5e7278de692d28337"
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


def put_far(memory, seg, off, ptr_off, ptr_seg):
    at = physical(seg, off)
    for i, value in enumerate((ptr_off & 255, ptr_off >> 8,
                               ptr_seg & 255, ptr_seg >> 8)):
        memory[(at + i) & MASK] = value


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    leaf = ovr[0x2FE1E:0x2FF74]
    assert len(leaf) == 342 and sha256(leaf).hexdigest() == LEAF_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(leaf, 0x02CC))
    assert len(ins) == 111 and sum(x.size for x in ins) == 342
    assert ins[0].address == 0x02CC and ins[-1].address == 0x041F
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "4"
    assert not any(x.mnemonic.startswith("call") or x.mnemonic == "lcall"
                   for x in ins)
    return leaf


def make_case(leaf, direction, changed, negative, a_byte, b_byte,
              ss, sp, flags, layout):
    memory = bytearray(RAM_SIZE)
    memory[physical(0xF000, 0x02CC):
           physical(0xF000, 0x02CC) + len(leaf)] = leaf
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x02CC, 0x6000, 0x8000, ss, flags)
    for i in range(-14, 12):
        memory[physical(ss, sp + i)] = (i * 31 + a_byte) & 255
    put16(memory, ss, sp, 0x1234)
    put16(memory, ss, sp + 2, 0xD000)
    rec = (0x2200 if layout == 0 else
           0xFFF0 if layout == 1 else
           (sp - 0x20) & 0xFFFF if layout == 2 else 0x25C0)
    rec_seg = ss if layout == 2 else 0x5000
    if layout == 3:
        cpu.ds = rec_seg
    for i in range(1024):
        memory[physical(cpu.ds, 0x25C0 + i)] = (i * 17 + 0x53) & 255
    put_far(memory, ss, sp + 4, rec, rec_seg)
    field = lambda disp: (rec + disp) & 0xFFFF
    put16(memory, rec_seg, field(0x0A), 5 if direction else 2)
    put16(memory, rec_seg, field(0x0C), 3)
    put16(memory, rec_seg, field(0x14), 2 if direction and not changed else 1)
    put16(memory, rec_seg, field(4), 2)
    put16(memory, rec_seg, field(0x16), 4 if not direction and not changed else 3)
    put16(memory, rec_seg, field(6), 4)
    put16(memory, rec_seg, field(8), 0xFFFF if negative else 0)
    put16(memory, rec_seg, field(0x0E), 7)
    put16(memory, rec_seg, field(0x10), 9)
    memory[physical(rec_seg, field(0x12))] = a_byte
    memory[physical(rec_seg, field(0x13))] = b_byte
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
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE, count=250)
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
        shared = Path(temp) / "u3c2_entry3.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3c2_entry3_02cc.c"),
            "-o", str(shared),
        ], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3c2_entry3_02cc
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(direction, changed, negative, a_byte, b_byte,
                  ss, sp, flags, layout)
                 for direction in (0, 1)
                 for changed in (0, 1)
                 for negative in (0, 1)
                 for a_byte, b_byte in ((0, 0), (2, 3), (0x7F, 0x80),
                                        (0x80, 0xFF), (0xFF, 0x7F))
                 for ss, sp, flags, layout in
                 ((0x7000, 0x8000, 0x0202, 0),
                  (0xE000, 0xFFFB, 0x0AD7, 1))]
        cases += [(direction, changed, negative, a_byte, b_byte,
                   0x7000, 0x8000, 0x0202, layout)
                  for layout in (2, 3)
                  for direction, changed, negative, a_byte, b_byte in
                  ((0, 1, 0, 2, 3), (1, 1, 0, 0x7F, 0x80),
                   (0, 1, 1, 0xFF, 0x7F), (1, 1, 1, 0x80, 0xFF))]
        for case in cases:
            compare_case(leaf, model, case)
    print(f"U3C2 +02CC original-byte full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
