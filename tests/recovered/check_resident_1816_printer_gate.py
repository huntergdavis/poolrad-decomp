#!/usr/bin/env python3
"""Private original-byte/C full-state differential through first printer entry.

The printer body is not run. Only addresses, hashes, and case counts print.
"""

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
sys.path.insert(0, str(ROOT / "tools/overlay"))
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PREFIX_SHA = "7b68709c7c03334a3b277bf7e2b100849407e2bf09c1f17b1e34f03e578c7d99"
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
    memory[(at + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def read16(memory, seg, off):
    at = physical(seg, off)
    return memory[at] | memory[(at + 1) & (RAM_SIZE - 1)] << 8


def cpu_values(cpu):
    return tuple(getattr(cpu, name) & 0xFFFF for name, _ in Cpu._fields_)


def uc_values(emu):
    return tuple(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS)


def source():
    raw = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(raw).hexdigest() == START_SHA
    image, *_ = unpack(raw)
    code = image[0x23B6:0x23F8]
    assert len(code) == 0x42 and sha256(code).hexdigest() == PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code, 0x1816))
    assert ins[0].address == 0x1816 and ins[-1].address == 0x1853
    assert ins[-1].mnemonic == "lcall" and ins[-1].op_str == "0x709, 0x626"
    assert [(x.address, x.op_str) for x in ins if x.mnemonic == "jne"] == [
        (0x1825, "0x182d"), (0x1836, "0x183e")]
    return code


def make_case(code, color_case, seed):
    memory = bytearray((i * 37 + 11) & 255 for i in range(RAM_SIZE))
    cs, ss, ds = 0x00BA, 0xE000, 0x1000
    sp = (0x8000, 0x0010, 0xFFFE)[seed]
    node_off = (0x2345, 0xFFFE, 0x0000)[seed]
    node_seg = (0x4567, 0x2000, 0x7FFF)[seed]
    memory[physical(cs, 0x1816):physical(cs, 0x1816) + len(code)] = code
    for offset, value in enumerate((0x3597, 0xBA56, 0xAB00 | (seed & 1),
                                    0xCD81 | seed, 0xEF39 | seed,
                                    node_off, node_seg)):
        put16(memory, ss, sp + 2 * offset, value)
    first, second = ((0, 0xA5), (7, 1), (0xFF, 2))[color_case]
    memory[physical(node_seg, node_off + 0x10D)] = first
    memory[physical(node_seg, node_off + 0x10E)] = second
    cpu = Cpu((0xABCD, 0x12EF, 0xFF00)[seed], 0x3456, 0x5678, 0x789A,
              0x9ABC, 0xBCDE, 0x3000, sp, cs, 0x1816, ds, 0x1111,
              ss, (0x3202, 0x3603, 0x3206)[seed])
    return memory, cpu, (node_off, node_seg,
                         (0x0C, 0x0E, 0x0B)[color_case])


def check_case(code, model, color_case, seed):
    label = f"color-{color_case}/seed-{seed}"
    memory, initial, (node_off, node_seg, color) = make_case(
        code, color_case, seed)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(initial, name))
    seen, snapshots = [], []

    def on_code(emu, address, size, user):
        del size, user
        seen.append((emu.reg_read(UC_X86_REG_CS), emu.reg_read(UC_X86_REG_IP)))
        if address == physical(0x0709, 0x0626):
            snapshots.append((bytes(emu.mem_read(0, RAM_SIZE)), uc_values(emu)))
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    try:
        original.emu_start(physical(0x00BA, 0x1816), 0, count=50)
    except Exception as exc:
        raise AssertionError((label, "emulator stopped", seen)) from exc
    assert len(snapshots) == 1, (label, seen)
    expected_ram, expected_cpu = snapshots[0]
    assert seen[0] == (0x00BA, 0x1816) and seen[-1] == (0x0709, 0x0626)
    sp = expected_cpu[7]
    assert tuple(read16(expected_ram, initial.ss, sp + 2 * i)
                 for i in range(7)) == (
                     0x1858, 0x00BA, node_off, node_seg,
                     (initial.ax & 0xFF00) | color,
                     (initial.ax & 0xFF00) | ((0x81 | seed) & 255),
                     (initial.ax & 0xFF00) | ((0x39 | seed) & 255))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    cpu = Cpu(*cpu_values(initial))
    assert model(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    assert bytes(ram) == expected_ram, (label, "RAM mismatch")
    for (name, _), actual, expected in zip(Cpu._fields_, cpu_values(cpu),
                                           expected_cpu):
        mask = 0x0FD5 if name == "flags" else 0xFFFF
        assert (actual ^ expected) & mask == 0, (
            label, name, hex(actual), hex(expected))


def main():
    code = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-1816-gate-",
                                     dir=ROOT / "resources") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC",
                        str(ROOT / "tools/replay/resident_1816_printer_gate.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_1816_first_printer_gate
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu)]
        model.restype = ctypes.c_int
        for color_case in range(3):
            for seed in range(3):
                check_case(code, model, color_case, seed)
    print("RES_00BA:1816 -> RES_0709:0626 first-entry gate: 9 "
          "original-byte/C full-state cases passed")
    print("Printer body and later optional suffix remain outside this gate")


if __name__ == "__main__":
    main()
