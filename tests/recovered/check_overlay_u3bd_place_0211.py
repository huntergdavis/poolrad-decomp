#!/usr/bin/env python3
"""Original-byte differential for the whole U3BD +0211 caller body.

The sole external far call uses the same explicit no-effect callee contract
on both sides. This checks the helper, not the real tile callee or game route.
Original media and generated products stay in ignored resources/.
"""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"
RAM_SIZE = 1 << 20
RAM_MASK = RAM_SIZE - 1
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + off) & RAM_MASK


def word(memory, seg, off):
    at = physical(seg, off)
    return memory[at] | (memory[(at + 1) & RAM_MASK] << 8)


def put_word(memory, seg, off, value):
    at = physical(seg, off)
    memory[at] = value & 255
    memory[(at + 1) & RAM_MASK] = value >> 8


def load_source(path):
    data = path.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == GAME_SHA
    source = data[0x2EB08:0x2EC16]
    assert len(source) == 270 and sha256(source).hexdigest() == SPAN_SHA
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x0211))
    assert len(instructions) == 108
    assert instructions[-1].address == 0x031C and instructions[-1].mnemonic == "retf"
    assert instructions[-1].address + instructions[-1].size == 0x031F
    assert [(i.address, i.op_str) for i in instructions if i.mnemonic == "lcall"] == [
        (0x02FB, "0x428, 0x2a")]
    branches = {i.address: (int(i.op_str, 16), i.address + i.size)
                for i in instructions if i.mnemonic.startswith("j") and
                i.mnemonic != "jmp"}
    assert len(branches) == 9
    return source, {i.address for i in instructions}, branches


def build_library(directory):
    output = directory / "u3bd_0211.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u3bd_place_0211.c"),
                    "-o", str(output)], check=True)
    library = ctypes.CDLL(str(output))
    callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_uint8),
                                     ctypes.c_size_t, ctypes.POINTER(Cpu),
                                     ctypes.c_void_p)
    function = library.pr_u3bd_place_0211
    function.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p)
    function.restype = ctypes.c_int
    return function, callback_type


def make_case(source, *, view=0, x=2, y=1, columns=2, rows=2,
              cursor=0, slice_index=1, pieces=(7, 0, 9, 3),
              sp=0x8000, seed=0):
    memory = bytearray(RAM_SIZE)
    rng = random.Random(seed)
    for i in range(0x2000):
        memory[physical(0x1000, (sp - 0x400 + i) & 0xFFFF)] = rng.randrange(256)
    cs, ds, ss = 0x2000, 0x3000, 0x1000
    loaded = bytearray(source)
    loaded[0x02FE - 0x0211:0x0300 - 0x0211] = (0x0C4C).to_bytes(2, "little")
    code_at = physical(cs, 0x0211)
    memory[code_at:code_at + len(loaded)] = loaded
    for off, value in ((0, 0xCAFE), (4, view),
                       (6, slice_index), (8, y & 255), (10, x & 255)):
        put_word(memory, ss, (sp + off) & 0xFFFF, value)
    return_cs_at = (physical(ss, sp) + 2) & RAM_MASK
    memory[return_cs_at] = 0xEF
    memory[(return_cs_at + 1) & RAM_MASK] = 0xBE
    for base, value in ((0x0C8C, cursor), (0x0C96, columns), (0x0CA0, rows)):
        memory[physical(ds, base + view)] = value
    put_word(memory, ds, 0x6A58, 0x0200)
    put_word(memory, ds, 0x6A5A, 0x5000)
    for i, item in enumerate(pieces):
        memory[physical(0x5000, 0x0200 + cursor + i)] = item
    cpu = Cpu(0xABCD, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              0x7777, sp, cs, 0x0211, ds, 0x4000, ss, 0x7202)
    return memory, cpu, bytes(loaded)


def run_case(label, memory, cpu, loaded, branches, function, callback_type):
    source_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(source_ram))
    emulator.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(source_ram))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        emulator.reg_write(register, getattr(cpu, name))
    reached = []
    visited = set()
    edges = set()
    previous = [None]

    def instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        assert address == physical(uc.reg_read(UC_X86_REG_CS), ip), (label, hex(address))
        assert 0x0211 <= ip < 0x031F, (label, hex(ip))
        if previous[0] in branches:
            target, fallthrough = branches[previous[0]]
            assert ip in (target, fallthrough), (label, hex(previous[0]), hex(ip))
            edges.add((previous[0], "taken" if ip == target else "fallthrough"))
        previous[0] = ip
        visited.add(ip)
        if ip in (0x02FB, 0x031C):
            reached.append(ip)
            uc.emu_stop()

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    count = 0
    while True:
        start = physical(emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP))
        emulator.emu_start(start, 2 * RAM_SIZE, count=3000)
        assert reached, f"{label}: no boundary"
        stop = reached.pop()
        if stop == 0x031C:
            emulator.hook_del(hook)
            start = physical(emulator.reg_read(UC_X86_REG_CS), 0x031C)
            emulator.emu_start(start, 2 * RAM_SIZE, count=1)
            break
        assert stop == 0x02FB
        ss = emulator.reg_read(UC_X86_REG_SS)
        sp = emulator.reg_read(UC_X86_REG_SP)
        cs = emulator.reg_read(UC_X86_REG_CS)
        sp = (sp - 2) & 0xFFFF
        put_word(source_ram, ss, sp, cs)
        sp = (sp - 2) & 0xFFFF
        put_word(source_ram, ss, sp, 0x0300)
        assert word(source_ram, ss, sp) == 0x0300
        emulator.reg_write(UC_X86_REG_SP, (sp + 14) & 0xFFFF)
        emulator.reg_write(UC_X86_REG_IP, 0x0300)
        count += 1
        assert count < 512

    assert bytes(source_ram[physical(cpu.cs, 0x0211):
                            physical(cpu.cs, 0x0211) + len(loaded)]) == loaded
    candidate_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    callback_calls = [0]

    @callback_type
    def no_effect_tile(ram, ram_size, state, context):
        del context
        assert ram_size == RAM_SIZE
        current = state.contents
        assert (current.cs, current.ip) == (0x0C4C, 0x002A)
        assert word(ram, current.ss, current.sp) == 0x0300
        saved_ip = word(ram, current.ss, current.sp)
        current.sp = (current.sp + 2) & 0xFFFF
        saved_cs = word(ram, current.ss, current.sp)
        current.sp = (current.sp + 12) & 0xFFFF
        current.ip, current.cs = saved_ip, saved_cs
        callback_calls[0] += 1
        return 0

    assert function(candidate_ram, RAM_SIZE, ctypes.byref(cpu), no_effect_tile, None) == 0
    assert count == callback_calls[0], (label, count, callback_calls[0])
    if bytes(candidate_ram) != bytes(source_ram):
        diff = [i for i, (a, b) in enumerate(zip(candidate_ram, source_ram)) if a != b]
        raise AssertionError((label, "RAM", [hex(i) for i in diff[:16]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = emulator.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (label, name, hex(actual), hex(expected))
    return visited, edges, count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, default=ORIGINAL)
    args = parser.parse_args()
    source, all_starts, branches = load_source(args.original)
    cases = [dict(rows=0, columns=2), dict(rows=2, columns=0),
             dict(x=-1, y=0), dict(x=11, y=0), dict(x=0, y=-1),
             dict(x=0, y=11), dict(pieces=(0, 0, 0, 0)),
             dict(pieces=(7, 9, 3, 2)), dict(cursor=255, rows=2, columns=2),
             dict(x=-128, y=127, rows=1, columns=1),
             dict(sp=0xFFFE, rows=0), dict(sp=0, columns=0)]
    rng = random.Random(0x3BD0211)
    for _ in range(54):
        cases.append(dict(x=rng.choice((-2, -1, 0, 1, 8, 9, 10, 11)),
                          y=rng.choice((-2, -1, 0, 1, 8, 9, 10, 11)),
                          columns=rng.randrange(3), rows=rng.randrange(3),
                          cursor=rng.choice((0, 1, 255)),
                          pieces=tuple(rng.choice((0, 1, 7, 15)) for _ in range(4))))
    visited, edges, calls = set(), set(), 0
    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-0211-") as temp:
        function, callback_type = build_library(Path(temp))
        for index, args in enumerate(cases):
            memory, cpu, loaded = make_case(source, seed=index, **args)
            seen, branch_edges, count = run_case(
                index, memory, cpu, loaded, branches, function, callback_type)
            visited |= seen
            edges |= branch_edges
            calls += count
    assert visited == all_starts, f"unvisited {[hex(i) for i in sorted(all_starts - visited)]}"
    expected_edges = {(address, direction) for address in branches
                      for direction in ("taken", "fallthrough")}
    assert edges == expected_edges, f"missing branches {expected_edges - edges}"
    print(f"U3BD +0211: {len(cases)} original-byte/C full-1MiB RAM+CPU14 cases PASS; "
          f"{len(visited)}/108 instruction starts, {len(edges)}/18 branch outcomes, "
          f"{calls} no-effect tile calls")


if __name__ == "__main__":
    main()
