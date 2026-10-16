#!/usr/bin/env python3
"""Compare U7F +0FDE original far-call transition with its C adapter."""

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


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


REGISTERS = [getattr(regs, "UC_X86_REG_" + ("EFLAGS" if name == "flags" else name.upper()))
             for name, _ in Cpu._fields_]
CALLBACK = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p,
                           ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                           ctypes.POINTER(Cpu))


def physical(seg, off):
    return ((seg << 4) + off) & MASK


def put_word(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = value >> 8


def audit_source(path):
    game = path.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    body = game[154715:154777]
    assert len(body) == 62 and sha256(body).hexdigest() == BODY_SHA
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0FD2))
    assert len(decoded) == 27 and sum(i.size for i in decoded) == 62
    call = next(i for i in decoded if i.address == 0x0FDE)
    assert call.bytes.hex() == "9a4800b000" and call.mnemonic == "lcall"
    return body


def build(directory):
    output = directory / "u7f_0fde.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u7f_roll_call_0fde.c"),
                    "-o", str(output)], check=True)
    function = ctypes.CDLL(str(output)).pr_u7f_roll_call_0fde
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu), CALLBACK, ctypes.c_void_p]
    function.restype = ctypes.c_int
    return function


def case(label, body, function, *, ss=0x2000, sp=0x8000, flags=0x7202,
         ds=0x1000, ax=0xA514):
    memory = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    cpu = Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0FDE, ds, 0x1111, ss, flags)
    at = physical(cs, 0x0FD2)
    memory[at:at + len(body)] = body
    put_word(memory, ss, sp, 20)
    put_word(memory, ss, (sp + 2) & 0xFFFF, 1)
    expected_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    seen = []

    def stop_at_stub(uc, address, size, user):
        del size, user
        if address == physical(cs, 0x0FDE):
            return
        assert address == physical(0x00B0, 0x0048), (label, hex(address))
        seen.append(address)
        uc.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_at_stub)
    original.emu_start(physical(cs, 0x0FDE), 2 * RAM_SIZE, count=5)
    assert len(seen) == 1, label
    called = []

    @CALLBACK
    def halt(context, ram, size, state):
        del context, ram
        assert size == RAM_SIZE
        called.append((state.contents.cs, state.contents.ip))
        return 0

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
    assert function(ram, RAM_SIZE, ctypes.byref(cpu), halt, None) == 2, label
    assert called == [(0x00B0, 0x0048)], label
    if memory != bytes(expected_ram):
        at = next(i for i, (a, b) in enumerate(zip(memory, expected_ram)) if a != b)
        raise AssertionError((label, "RAM", hex(at), memory[at], expected_ram[at]))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        assert getattr(cpu, name) == original.reg_read(register) & 0xFFFF, (
            label, name, hex(getattr(cpu, name)), hex(original.reg_read(register) & 0xFFFF))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    args = parser.parse_args()
    body = audit_source(args.original)
    specs = [
        ("ordinary", {}),
        ("stack-0002", {"sp": 0x0002}),
        ("stack-0000", {"sp": 0x0000}),
        ("stack-fffe", {"sp": 0xFFFE}),
        ("stack-ffff", {"sp": 0xFFFF}),
        ("a20", {"ss": 0xFFFF, "sp": 0x0100}),
        ("ds-ss", {"ss": 0x1000}),
        ("flags", {"flags": 0x0687}),
    ]
    rng = random.Random(0x7F0FDE)
    for i in range(32):
        specs.append((f"seed-{i}", {"sp": rng.randrange(0x10000),
                                     "ax": rng.randrange(0x10000),
                                     "flags": 0x7202 ^ (rng.randrange(0x1000) & 0x08D5)}))
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-0fde-") as temp:
        function = build(Path(temp))
        for label, options in specs:
            case(label, body, function, **options)
    print(f"U7F +0FDE far-call transition: {len(specs)} original-byte/C "
          "full-1MiB RAM+CPU14 cases PASS; stub boundary only")


if __name__ == "__main__":
    main()
