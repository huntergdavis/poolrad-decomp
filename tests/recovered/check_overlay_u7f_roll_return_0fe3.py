#!/usr/bin/env python3
"""Compare U7F +0FE3 continuation with original bytes on supplied frames."""

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
    continuation = [i for i in decoded if 0x0FE3 <= i.address <= 0x100F]
    assert continuation[0].address == 0x0FE3
    assert continuation[-1].address == 0x100F and continuation[-1].mnemonic == "retf"
    assert sum(i.size for i in continuation) == 0x1010 - 0x0FE3
    return body, {i.address for i in continuation}


def build(directory):
    output = directory / "u7f_0fe3.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u7f_roll_return_0fe3.c"),
                    "-o", str(output)], check=True)
    function = ctypes.CDLL(str(output)).pr_u7f_roll_return_0fe3
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def case(label, body, function, *, roll, bp=0x8000, ss=0x2000,
         flags=0x7202, ah=0xA5, sp=0x7000):
    memory = bytearray([0xA5]) * RAM_SIZE
    cs, return_cs, return_ip = 0x3000, 0x1357, 0x2468
    cpu = Cpu((ah << 8) | roll, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, bp, sp, cs, 0x0FE3, 0x1000, 0x2222, ss, flags)
    at = physical(cs, 0x0FD2)
    memory[at:at + len(body)] = body
    put_word(memory, ss, bp, 0x7777)
    put_word(memory, ss, (bp + 2) & 0xFFFF, return_ip)
    put_word(memory, ss, (bp + 4) & 0xFFFF, return_cs)
    expected_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    reached = []
    seen = set()

    def stop_at_return(uc, address, size, user):
        del size, user
        if address == physical(return_cs, return_ip):
            reached.append(address)
            uc.emu_stop()
        else:
            seen.add(address)

    original.hook_add(UC_HOOK_CODE, stop_at_return)
    original.emu_start(physical(cs, 0x0FE3), 2 * RAM_SIZE, count=40)
    assert reached == [physical(return_cs, return_ip)], label
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
    assert function(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    if memory != bytes(expected_ram):
        at = next(i for i, (a, b) in enumerate(zip(memory, expected_ram)) if a != b)
        raise AssertionError((label, "RAM", hex(at), memory[at], expected_ram[at]))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        assert getattr(cpu, name) == original.reg_read(register) & 0xFFFF, (
            label, name, hex(getattr(cpu, name)), hex(original.reg_read(register) & 0xFFFF))
    return seen


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    args = parser.parse_args()
    body, starts = audit_source(args.original)
    specs = [(f"roll-{r}", {"roll": r}) for r in (0, 1, 14, 15, 19, 20, 255)]
    specs.append(("a20", {"roll": 15, "ss": 0xFFFF, "bp": 0x0100}))
    rng = random.Random(0x7F0FE3)
    for i in range(32):
        specs.append((f"seed-{i}", {"roll": rng.randrange(256),
                                    "bp": rng.randrange(0x0100, 0xFF00),
                                    "sp": rng.randrange(0x10000),
                                    "ah": rng.randrange(256),
                                    "flags": 0x7202 ^ (rng.randrange(0x1000) & 0x08D5)}))
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-0fe3-") as temp:
        function = build(Path(temp))
        seen = set()
        for label, options in specs:
            seen.update(case(label, body, function, **options))
    expected = {physical(0x3000, address) for address in starts}
    assert seen == expected, (sorted(seen ^ expected), len(seen), len(expected))
    print(f"U7F +0FE3 continuation: {len(specs)} controlled original-byte/C "
          f"full-1MiB RAM+CPU14 cases PASS; {len(seen)}/{len(starts)} reached starts; "
          "no callee or natural reach")


if __name__ == "__main__":
    main()
