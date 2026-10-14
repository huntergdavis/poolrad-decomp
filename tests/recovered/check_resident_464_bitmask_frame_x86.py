#!/usr/bin/env python3
"""Exercise every raw selector and mask outcome with original x86 versus C."""

import argparse
import ctypes as c
from pathlib import Path

from check_resident_464_reverse_1484_x86 import CPU, State
from resident_464_bitmask_audit import collect
from resident_loader_map import unpack


def check(exe, library):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16
    from unicorn import x86_const as x

    collect(exe)
    image, _, _, _ = unpack(exe)
    body = bytes(image[0x4640:0x46bb])
    model = c.CDLL(str(library.resolve()))
    model.pr_r464_bitmask_frame.argtypes = [c.POINTER(State)]
    model.pr_r464_bitmask_frame.restype = c.c_int
    registers = {name: getattr(x, "UC_X86_REG_" + name.upper())
                 for name, _ in CPU._fields_ if name != "flags"}
    registers["flags"] = x.UC_X86_REG_EFLAGS
    for case in range(512):
        selector = case if case < 256 else 13
        if case < 256:
            packed = bytes(((case * 0x9e37) & 0xffff).to_bytes(2, "little") +
                           ((case * 0x1234 + 0x1357) & 0xffff).to_bytes(2, "little"))
        else:
            nibbles = [selector if (case & (0x80 >> i)) else selector ^ 1
                       for i in range(8)]
            packed = bytes((nibbles[i] << 4) | nibbles[i + 1]
                           for i in range(0, 8, 2))
        initial = bytearray([0xa5]) * 0x100000
        initial[0xc880:0xc880 + len(body)] = body
        cpu = CPU(0x8765, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
                  0x789a, 0x37a0, 0x0c88, 0x1d16, 0x4529, 0x1d16,
                  0x1357, 0x2468, 0, 0x0ed7)
        stack = (cpu.ss << 4) + cpu.sp
        initial[stack:stack + 10] = bytes((0x7e, 6, 0x88, 0xc)) + packed + bytes((selector, 0xd3))
        native = Uc(UC_ARCH_X86, UC_MODE_16)
        native.mem_map(0, len(initial)); native.mem_write(0, bytes(initial))
        for name, reg in registers.items(): native.reg_write(reg, getattr(cpu, name))
        native.emu_start(0xc880, 0xc880 + 0x67e, count=1000)
        assert native.reg_read(x.UC_X86_REG_IP) == 0x67e
        ram = (c.c_uint8 * len(initial)).from_buffer_copy(initial)
        state = State(cpu, ram, len(initial))
        assert model.pr_r464_bitmask_frame(c.byref(state)) == 1
        for name, reg in registers.items():
            assert getattr(state.cpu, name) == native.reg_read(reg), (case, name)
        assert bytes(ram) == bytes(native.mem_read(0, len(initial))), case
    print("512 bitmask original-x86/C cases: 16 CPU fields and 1 MiB RAM match")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    args = parser.parse_args()
    check(args.exe.read_bytes(), args.library)
