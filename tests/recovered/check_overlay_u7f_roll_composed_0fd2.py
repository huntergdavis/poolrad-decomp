#!/usr/bin/env python3
"""Eight controlled original-byte U7F caller/composed-C comparisons."""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
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
CONDITIONALS = {0x0FEB, 0x0FEF, 0x0FF9, 0x0FFD}


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


def word(ram, seg, off):
    at = physical(seg, off)
    return ram[at] | (ram[(at + 1) & MASK] << 8)


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
    assert decoded[0].address == 0x0FD2 and decoded[-1].address == 0x100F
    assert next(i for i in decoded if i.address == 0x0FDE).bytes.hex() == "9a4800b000"
    return body, {i.address for i in decoded}


def build(directory):
    output = directory / "u7f_0fd2_composed.so"
    sources = ["overlay_u7f_roll_prefix_0fd2.c",
               "overlay_u7f_roll_call_0fde.c",
               "overlay_u7f_roll_return_0fe3.c",
               "overlay_u7f_roll_composed_0fd2.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / source) for source in sources],
                    "-o", str(output)], check=True)
    function = ctypes.CDLL(str(output)).pr_u7f_roll_composed_0fd2
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu), CALLBACK, ctypes.c_void_p]
    function.restype = ctypes.c_int
    return function


def case(label, body, function, *, roll, ss=0x2000, sp=0x8000, ax=0xA500):
    cs, outer_ip, outer_cs = 0x3000, 0x2468, 0x1357
    memory = bytearray([0xA5]) * RAM_SIZE
    at = physical(cs, 0x0FD2)
    memory[at:at + len(body)] = body
    put_word(memory, ss, sp, outer_ip)
    put_word(memory, ss, (sp + 2) & 0xFFFF, outer_cs)
    cpu = Cpu(ax, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0FD2, 0x1000, 0x2222, ss, 0x7202)
    expected_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(expected_ram))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    trace, stub, returned = [], [], []

    def stop_at_boundaries(uc, address, size, user):
        del size, user
        if address == physical(0x00B0, 0x0048):
            stub.append(address)
            uc.emu_stop()
        elif address == physical(outer_cs, outer_ip):
            returned.append(address)
            uc.emu_stop()
        else:
            assert uc.reg_read(regs.UC_X86_REG_CS) == cs, (label, hex(address))
            trace.append(uc.reg_read(regs.UC_X86_REG_IP))

    original.hook_add(UC_HOOK_CODE, stop_at_boundaries)
    original.emu_start(physical(cs, 0x0FD2), 2 * RAM_SIZE, count=20)
    assert stub == [physical(0x00B0, 0x0048)], label
    assert original.reg_read(regs.UC_X86_REG_CS) == 0x00B0
    assert original.reg_read(regs.UC_X86_REG_IP) == 0x0048
    stub_sp = original.reg_read(regs.UC_X86_REG_SP)
    assert word(expected_ram, ss, (stub_sp + 4) & 0xFFFF) == (ax & 0xFF00) | 0x14
    assert word(expected_ram, ss, (stub_sp + 6) & 0xFFFF) == (ax & 0xFF00) | 1
    return_ip = word(expected_ram, ss, stub_sp)
    return_cs = word(expected_ram, ss, (stub_sp + 2) & 0xFFFF)
    assert (return_cs, return_ip) == (cs, 0x0FE3)
    original.reg_write(regs.UC_X86_REG_AX, 0xD200 | roll)
    original.reg_write(regs.UC_X86_REG_BX, 0xA432)
    original.reg_write(regs.UC_X86_REG_CX, 0x9988)
    original.reg_write(regs.UC_X86_REG_DX, 0x6142)
    original.reg_write(regs.UC_X86_REG_SI, 0xABCD)
    original.reg_write(regs.UC_X86_REG_DI, 0x1357)
    original.reg_write(regs.UC_X86_REG_DS, 0x4111)
    original.reg_write(regs.UC_X86_REG_ES, 0x6222)
    original.reg_write(regs.UC_X86_REG_EFLAGS, 0x7202)
    expected_ram[physical(0x4111, 0x1234)] = 0x5A
    original.reg_write(regs.UC_X86_REG_SP, (stub_sp + 8) & 0xFFFF)
    original.reg_write(regs.UC_X86_REG_CS, return_cs)
    original.reg_write(regs.UC_X86_REG_IP, return_ip)
    original.emu_start(physical(cs, 0x0FE3), 2 * RAM_SIZE, count=40)
    assert returned == [physical(outer_cs, outer_ip)], label

    called = []

    @CALLBACK
    def controlled_roll(context, ram, size, state):
        del context
        assert size == RAM_SIZE
        current = state.contents
        assert (current.cs, current.ip) == (0x00B0, 0x0048)
        assert word(ram, current.ss, (current.sp + 4) & 0xFFFF) == (ax & 0xFF00) | 0x14
        assert word(ram, current.ss, (current.sp + 6) & 0xFFFF) == (ax & 0xFF00) | 1
        called.append((current.cs, current.ip))
        target_ip = word(ram, current.ss, current.sp)
        target_cs = word(ram, current.ss, (current.sp + 2) & 0xFFFF)
        current.ax = 0xD200 | roll
        current.bx = 0xA432
        current.cx = 0x9988
        current.dx = 0x6142
        current.si = 0xABCD
        current.di = 0x1357
        current.ds = 0x4111
        current.es = 0x6222
        current.flags = 0x7202
        ram[physical(current.ds, 0x1234)] = 0x5A
        current.sp = (current.sp + 8) & 0xFFFF
        current.cs, current.ip = target_cs, target_ip
        return 1

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
    assert function(ram, RAM_SIZE, ctypes.byref(cpu), controlled_roll, None) == 0, label
    assert called == [(0x00B0, 0x0048)], label
    if memory != bytes(expected_ram):
        at = next(i for i, (a, b) in enumerate(zip(memory, expected_ram)) if a != b)
        raise AssertionError((label, "RAM", hex(at), memory[at], expected_ram[at]))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        assert getattr(cpu, name) == original.reg_read(register) & 0xFFFF, (
            label, name, hex(getattr(cpu, name)), hex(original.reg_read(register) & 0xFFFF))
    edges = {(a, b) for a, b in zip(trace, trace[1:]) if a in CONDITIONALS}
    return set(trace), edges


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    args = parser.parse_args()
    body, starts = audit_source(args.original)
    specs = [(f"roll-{r}", {"roll": r}) for r in (0, 1, 14, 15, 19, 20, 255)]
    specs.append(("a20", {"roll": 15, "ss": 0xFFFF, "sp": 0x0100,
                           "ax": 0x1200}))
    assert len(specs) == 8
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-0fd2-composed-") as temp:
        function = build(Path(temp))
        seen, edges = set(), set()
        for label, options in specs:
            reached, branches = case(label, body, function, **options)
            seen.update(reached)
            edges.update(branches)
    assert seen == starts, (sorted(seen ^ starts), len(seen), len(starts))
    for branch in CONDITIONALS:
        assert len({target for source, target in edges if source == branch}) == 2, branch
    print(f"U7F +0FD2 composed: {len(specs)} controlled original-byte/C "
          f"full-1MiB RAM+CPU14 cases PASS; {len(seen)}/{len(starts)} starts, "
          f"{len(edges)} conditional edges, 4/4 branch decisions both ways; "
          "synthetic callback only")


if __name__ == "__main__":
    main()
