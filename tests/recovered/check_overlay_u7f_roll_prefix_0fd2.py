#!/usr/bin/env python3
"""Original-byte full-state parity for U7F +0FD2 before its far call."""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn import x86_const as regs

ROOT = Path(__file__).resolve().parents[2]
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27"
PREFIX = {0x0FD2, 0x0FD3, 0x0FD5, 0x0FD8, 0x0FDA, 0x0FDB, 0x0FDD}


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


REGISTERS = [getattr(regs, "UC_X86_REG_" + ("EFLAGS" if name == "flags" else name.upper()))
             for name, _ in Cpu._fields_]


def physical(seg, off):
    return ((seg << 4) + off) & MASK


def source(path):
    game = path.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    body = game[154715:154777]
    assert len(body) == 62 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0FD2))
    assert len(ins) == 27 and sum(i.size for i in ins) == 62
    assert {i.address for i in ins if i.address < 0x0FDE} == PREFIX
    assert next(i for i in ins if i.address == 0x0FDE).bytes.hex() == "9a4800b000"
    return body


def build(directory):
    output = directory / "u7f_0fd2.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u7f_roll_prefix_0fd2.c"),
                    "-o", str(output)], check=True)
    function = ctypes.CDLL(str(output)).pr_u7f_roll_prefix_0fd2
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def case(label, body, function, *, sp=0x8000, ss=0x2000, ax=0xA5CD,
         flags=0x7202, ds=0x1000):
    memory = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    at = physical(cs, 0x0FD2)
    memory[at:at + len(body)] = body
    cpu = Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0FD2, ds, 0x1111, ss, flags)
    expected_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    visited = set()
    stopped = []

    def instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(regs.UC_X86_REG_IP)
        cs_now = uc.reg_read(regs.UC_X86_REG_CS)
        assert cs_now == cs, (label, hex(cs_now), hex(ip))
        if ip == 0x0FDE:
            stopped.append(ip)
            uc.emu_stop()
            return
        assert ip in PREFIX, (label, hex(ip), hex(address))
        visited.add(ip)

    original.hook_add(UC_HOOK_CODE, instruction)
    original.emu_start(physical(cs, 0x0FD2), 2 * RAM_SIZE, count=12)
    assert stopped == [0x0FDE] and visited == PREFIX, (label, visited, stopped)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
    assert function(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    if memory != bytes(expected_ram):
        at = next(i for i, (a, b) in enumerate(zip(memory, expected_ram)) if a != b)
        raise AssertionError((label, "RAM", hex(at), memory[at], expected_ram[at]))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(register) & 0xFFFF
        assert getattr(cpu, name) == expected, (
            label, name, hex(getattr(cpu, name)), hex(expected))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    args = parser.parse_args()
    body = source(args.original)
    specs = [
        ("ordinary", {}),
        ("stack-0000", {"sp": 0x0000}),
        ("stack-0001", {"sp": 0x0001}),
        ("stack-0002", {"sp": 0x0002}),
        ("stack-fffe", {"sp": 0xFFFE}),
        ("stack-ffff", {"sp": 0xFFFF}),
        ("a20-stack", {"ss": 0xFFFF, "sp": 0x0100}),
        ("ds-ss", {"ss": 0x1000}),
        ("flags-df", {"flags": 0x0687}),
    ]
    rng = random.Random(0x7F0FD2)
    for i in range(40):
        specs.append((f"seed-{i}", {"sp": rng.randrange(0x10000),
                                     "ax": rng.randrange(0x10000),
                                     "flags": 0x7202 ^ (rng.randrange(0x1000) & 0x08D5)}))
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-0fd2-") as temp:
        function = build(Path(temp))
        for label, options in specs:
            case(label, body, function, **options)
    print(f"U7F +0FD2..+0FDE prefix: {len(specs)} original-byte/C full-1MiB "
          "RAM+CPU14 cases PASS; 7/7 instruction starts; stop before far call")


if __name__ == "__main__":
    main()
