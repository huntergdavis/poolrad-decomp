#!/usr/bin/env python3
"""Original-byte U9E:+018B and loaded RES_00BA_2199 full-state parity."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import sys
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
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
CALL_SHA = "e73d8f291398fc6975f6e2164133aebc49cf6ff33774e365c746c0ef58330646"
RESIDENT_SHA = "721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c"
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
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    call = ovr[0x2B075:0x2B086]
    assert len(call) == 17 and sha256(call).hexdigest() == CALL_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(call, 0x018B))
    assert len(ins) == 5 and sum(x.size for x in ins) == 17
    assert ins[0].address == 0x018B and ins[0].mnemonic == "lcall"
    assert ins[-1].address == 0x0199 and ins[-1].mnemonic == "jmp"
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from resident_loader_map import unpack
    expanded, *_ = unpack(exe)
    resident = bytes(expanded[0x2D39:0x2DA8])
    assert len(resident) == 111 and sha256(resident).hexdigest() == RESIDENT_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(resident, 0x2199))
    assert len(ins) == 37 and sum(x.size for x in ins) == 111
    assert ins[-1].address == 0x2205 and ins[-1].mnemonic == "retf"
    return call, resident


def make_case(call, resident, mode, ss, sp, flags):
    memory = bytearray(RAM_SIZE)
    memory[physical(0xF000, 0x018B):physical(0xF000, 0x018B) + len(call)] = call
    memory[physical(0x00BA, 0x2199):physical(0x00BA, 0x2199) + len(resident)] = resident
    ds = ss if mode & 1 else 0x3000
    bp = (sp + 0x00EE) & 0xFFFF
    cell_off = (bp - 0x00E0) & 0xFFFF
    key = 0x7D
    if mode == 4:
        char_seg, char_off = ss, (cell_off - 0x7F) & 0xFFFF
    elif mode == 5:
        char_seg, char_off = (ss - 1) & 0xFFFF, (cell_off - 0x6F) & 0xFFFF
    else:
        char_seg, char_off = 0x4000, 0x2300
    cpu = Cpu(0xA5CD, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              bp, sp, 0xF000, 0x018B, ds, 0x9876, ss, flags)
    for i in range(-10, 24):
        memory[physical(ss, (sp + i) & 0xFFFF)] = (i * 13 + mode) & 255
    for i, value in enumerate((cell_off, ss, 0xA500 | key,
                               char_off, char_seg)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    put16(memory, ss, cell_off, 0xAAAA)
    put16(memory, ss, (cell_off + 2) & 0xFFFF, 0xBBBB)
    head_off = 0 if mode == 0 else (0 if mode == 6 else 0x2200)
    head_seg = 0 if mode == 0 else 0x5000
    put16(memory, char_seg, (char_off + 0x7F) & 0xFFFF, head_off)
    put16(memory, char_seg, (char_off + 0x81) & 0xFFFF, head_seg)
    first_off = head_off
    memory[physical(0x5000, first_off)] = key if mode in (1, 4, 6) else 0x11
    put16(memory, 0x5000, (first_off + 5) & 0xFFFF,
          0x2300 if mode in (2, 5) else 0)
    put16(memory, 0x5000, (first_off + 7) & 0xFFFF,
          0x5000 if mode in (2, 5) else 0)
    memory[physical(0x5000, 0x2300)] = key
    return memory, cpu


def compare_case(label, call, resident, model):
    memory, cpu = make_case(call, resident, *label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    stops = {physical(cpu.cs, x) for x in (0x019C, 0x02D4)}
    reached = []

    def on_code(emu, address, size, user):
        del size, user
        if address in stops:
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(physical(cpu.cs, 0x018B), RAM_SIZE, count=500)
    assert len(reached) == 1, label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = model(ram, RAM_SIZE, ctypes.byref(cpu), 8)
    assert status == (0 if reached[0] == physical(cpu.cs, 0x019C) else 1), (label, status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [(hex(x), hex(ram[x]),
                                               hex(expected_ram[x]))
                                              for x in diff[:12]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    call, resident = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-018b-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O0",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u9e_sidecar_call_018b.c"),
            str(ROOT / "src/recovered/resident_sidecar_lookup.c"),
            "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library)).pr_u9e_sidecar_call_018b
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), ctypes.c_size_t]
        model.restype = ctypes.c_int
        cases = [(mode, ss, sp, flags)
                 for mode in range(7)
                 for ss in (0xE000, 0x3000)
                 for sp in (0x8000, 0x0100, 0x0000, 0xFFFF, 0xFFF1)
                 for flags in (0x72D3, 0x0E17)]
        for label in cases:
            compare_case(label, call, resident, model)
    print(f"u9e:+018b plus original resident full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
