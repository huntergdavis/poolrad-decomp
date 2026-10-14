#!/usr/bin/env python3
"""Compare original 16-bit leaf to C on synthetic CPU/RAM, not a live DOS claim.

Build the C model as a shared library and pass it with the ignored START.EXE.
Dependencies: Capstone (audit) and Unicorn (independent x86 execution).
"""

import argparse
import ctypes as c
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/overlay"))
from resident_464_reverse_1484_audit import BASE, LO, HI, collect
from resident_loader_map import unpack


class CPU(c.Structure):
    _fields_ = [(n, c.c_uint16) for n in
                "ax bx cx dx si di bp sp cs ds es ss fs gs ip flags".split()]


class State(c.Structure):
    _fields_ = [("cpu", CPU), ("ram", c.POINTER(c.c_uint8)),
                ("ram_size", c.c_size_t)]


def check(exe, library):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
    from unicorn import x86_const as x

    evidence = collect(exe)
    image, _, _, _ = unpack(exe)
    body = bytes(image[BASE + LO:BASE + HI])
    model = c.CDLL(str(library.resolve()))
    model.pr_r464_reverse_1484_frame.argtypes = [c.POINTER(State)]
    model.pr_r464_reverse_1484_frame.restype = c.c_int
    registers = {name: getattr(x, "UC_X86_REG_" + name.upper())
                 for name, _ in CPU._fields_ if name != "flags"}
    registers["flags"] = x.UC_X86_REG_EFLAGS
    code = 0x0c880 + LO
    dest = 0x0c880 + 0x17ae
    for value in range(256):
        initial = bytearray([0xa5]) * 0x100000
        initial[code:code + len(body)] = body
        cpu = CPU(0x8765, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
                  0x789a, 0x37a0, 0x0c88, 0x1d16, 0x4529, 0x1d16,
                  0x1357, 0x2468, LO, 0x0ed7)
        stack = (cpu.ss << 4) + cpu.sp
        initial[stack:stack + 6] = bytes((0xae, 0x17, 0x88, 0x0c, value, 0xd3))
        native = Uc(UC_ARCH_X86, UC_MODE_16)
        native.mem_map(0, len(initial))
        native.mem_write(0, bytes(initial))
        for name, reg in registers.items():
            native.reg_write(reg, getattr(cpu, name))
        native.emu_start(code, dest, count=evidence["instructions"])
        ram = (c.c_uint8 * len(initial)).from_buffer_copy(initial)
        state = State(cpu, ram, len(initial))
        assert model.pr_r464_reverse_1484_frame(c.byref(state)) == 1
        for name, reg in registers.items():
            assert getattr(state.cpu, name) == native.reg_read(reg), (
                value, name, getattr(state.cpu, name), native.reg_read(reg))
        assert bytes(ram) == bytes(native.mem_read(0, len(initial))), value
    print("256 original-x86/C cases: all 16 CPU fields and 1 MiB RAM match")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    args = parser.parse_args()
    check(args.exe.read_bytes(), args.library)
