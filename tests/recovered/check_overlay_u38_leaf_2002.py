#!/usr/bin/env python3
"""Original-byte full RAM/CPU parity for DOS GAME.OVR U38 +2002."""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile
import sys

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
from overlay_inventory import find_units, parse_mz

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
BODY_SHA = "cdf3d522f6de4b8f37ccc47e3267e244f26cab4866883306b9dcfe1525dd2af4"
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


def put16(ram, seg, off, value):
    at = physical(seg, off)
    ram[at] = value & 255
    ram[(at + 1) & MASK] = (value >> 8) & 255


def source(ovr_path=None, exe_path=None):
    ovr = (ovr_path or ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    exe = (exe_path or ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x38)
    assert unit["ovr_offset"] == 0xEC5B and unit["memory_segment"] == 0x41
    base = unit["ovr_offset"]
    parent = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
        ovr[base + 0x2122:base + 0x23A4], 0x2122))
    calls = []
    for i, ins in enumerate(parent):
        if ins.mnemonic == "call" and ins.op_str == "0x2002":
            preceding = parent[i - 1]
            assert preceding.mnemonic == "push" and preceding.op_str == "cs"
            assert preceding.address + preceding.size == ins.address
            calls.append(ins.address)
    assert calls == [0x22E7, 0x2300, 0x2313, 0x2360]
    print("PASS: U38 descriptor and four aligned PUSH CS / CALL +2002 sites")
    body = ovr[0x10C5D:0x10D3A]
    assert len(body) == 221 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2002))
    assert len(ins) == 75 and sum(i.size for i in ins) == len(body)
    assert ins[0].address == 0x2002 and ins[-1].address == 0x20DC
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "2"
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    addresses = {i.address for i in ins}
    assert all(int(i.op_str, 16) in addresses
               for i in ins if i.mnemonic.startswith("j"))
    print(f"Body: {len(body)} bytes / {len(ins)} instructions; SHA256 {BODY_SHA}")
    return body


def make_case(body, mode, sp, seed):
    ram = bytearray(RAM_SIZE)
    ram[physical(0xF000, 0x2002):physical(0xF000, 0x2002) + len(body)] = body
    ss, base, cells = 0x7000, 0x9000, 0x2000
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x2002, 0x6000, 0x8000, ss,
              0x0202 if seed == 0 else 0x0AD7)
    for i in range(-20, 20):
        ram[physical(ss, sp + i)] = (i * 19 + seed * 23 + mode * 7) & 255
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    put16(ram, ss, sp + 4, base)
    put16(ram, ss, sp + 6, 0xE55E)
    put16(ram, ss, base + 8, cells)
    put16(ram, ss, base + 10, 0x5000)
    patterns = (
        [2, 3, 4, 5, 6, 7, 8, 9],
        [2] * 8,
        [2] * 8,
        [2, 0xFF, 1, 2, 0x80, 3, 1, 2],
        [0] * 8,
        [2, 3, 2, 3, 2, 3, 2, 3],
        [2] * 8,
        [0x80, 2, 1, 0, 2, 2, 3, 1],
    )
    for i, value in enumerate(patterns[mode]):
        ram[physical(0x5000, cells + 0x96 + i)] = value
        score = (i + 1 if mode == 0 else
                 8 - i if mode == 1 else
                 0xFFFF if mode == 2 and i == 5 else
                 (i * 11 + seed * 5) & 0xFFFF)
        high = (0xFFFF if mode == 2 and i == 5 else
                0x8000 if mode == 5 and i % 2 else
                0 if mode != 6 else i & 1)
        put16(ram, cpu.ds, 0x409B + i * 0x38 + value * 4, score)
        put16(ram, cpu.ds, 0x409D + i * 0x38 + value * 4, high)
    return ram, cpu


def extended_cases():
    # Deterministic coverage of the byte domain and signed/unsigned score edges.
    scores = (0, 1, 0xFFFF, 0x10000, 0x7FFFFFFF, 0x80000000,
              0xFFFFFFFE, 0xFFFFFFFF)
    for slot in range(8):
        for value in range(256):
            cells = [2] * 8
            cells[slot] = value
            yield f"cell-{slot}-{value:02x}", dict(cells=cells,
                scores=[scores[(i + value) % len(scores)] for i in range(8)])
    for stale in range(256):
        yield f"stale-{stale:02x}", dict(cells=[0x80] * 8, stale=stale)
    for sp in (0, 1, 2, 3, 4, 5, 6, 0x7FFF, 0xFFF8, 0xFFFB, 0xFFFC, 0xFFFD):
        for base in (0, 1, 0x29, 0x2B, 0x2D, 0x2F, 0x31, 0xFFF4, 0xFFF5, 0xFFF6, 0xFFF7, 0xFFF8):
            yield f"frame-{sp:04x}-{base:04x}", dict(sp=sp, base=base)
    for offset in (0, 1, 2, 0xFFF8, 0xFFFF):
        for seg in (0, 0x7000, 0xFFFF):
            yield f"cells-{seg:04x}-{offset:04x}", dict(cell_seg=seg, cell_offset=offset)
    for delta in range(-12, 20):
        yield f"cell-local-alias-{delta}", dict(cell_seg=0x7000,
            cell_offset=(0x8000 + delta - 0x96) & 0xFFFF)
    for base in (0x40CC, 0x40D0, 0x40D4, 0x40D8):
        yield f"score-object-alias-{base:04x}", dict(ds=0x7000, base=base)
    rng = random.Random(0x382002)
    for i in range(128):
        yield f"random-{i}", dict(cells=[rng.randrange(256) for _ in range(8)],
            scores=[rng.getrandbits(32) for _ in range(8)],
            cs=rng.choice((0x1000, 0xF000, 0xFFFF)),
            ss=rng.choice((0, 0x7000, 0xFFFF)),
            ds=rng.choice((0x2000, 0x6000, 0xFFFF)),
            flags=rng.choice((0x202, 0xAD7, 0x602)), seed=i)


def fixture(*, cells=(2, 3, 4, 5, 6, 7, 8, 9), scores=None,
            stale=0xA5, sp=0x8000, base=0x9000, cell_seg=0x5000,
            cell_offset=0x2000, cs=0xF000, ss=0x7000, ds=0x6000,
            flags=0x202, seed=None):
    ram = bytearray([0xA5]) * RAM_SIZE if seed is None else bytearray(
        random.Random(seed).randbytes(RAM_SIZE))
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, cs, 0x2002, ds, 0x8000, ss, flags)
    scores = scores or list(range(8))
    for i, (cell, score) in enumerate(zip(cells, scores)):
        put16(ram, ds, 0x409B + i * 0x38 + cell * 4, score)
        put16(ram, ds, 0x409D + i * 0x38 + cell * 4, score >> 16)
    for off, value in ((0, 0x1234), (2, 0xD000), (4, base)):
        put16(ram, ss, sp + off, value)
    put16(ram, ss, base + 8, cell_offset)
    put16(ram, ss, base + 10, cell_seg)
    ram[physical(ss, sp - 3)] = stale
    for i, value in enumerate(cells):
        ram[physical(cell_seg, cell_offset + 0x96 + i)] = value
    return ram, cpu


def compare_case(body, model, label, initial, coverage, edges):
    ram, cpu = initial
    instructions = {i.address:i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2002)}
    code = {}
    for i, value in enumerate(body):
        at = (physical(cpu.cs, 0x2002) + i) & MASK
        ram[at] = value
        code[at] = value
    backing = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached, previous = [], [None]
    local_coverage, local_edges = set(), set()

    def instruction(emu, address, size, user):
        del address, size, user
        ip = emu.reg_read(UC_X86_REG_IP)
        assert ip in instructions, (label, "unexpected PC", hex(ip))
        ins = instructions[ip]
        local_coverage.add(ip)
        if previous[0] is not None:
            local_edges.add((previous[0], ip))
        previous[0] = ip if ins.mnemonic.startswith("j") and ins.mnemonic != "jmp" else None
        if ins.mnemonic == "retf":
            reached.append(ip)
            emu.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, instruction)
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE * 2, count=20000)
    original.hook_del(hook)
    if not reached:
        # Do not invoke the unbounded C loop on an original nonreturning alias.
        return "budget-exhausted"
    assert all(backing[at] == value for at, value in code.items()), (label, "code modified")
    original.emu_start(physical(cpu.cs, reached[0]), RAM_SIZE * 2, count=1)
    expected_ram = bytes(backing)
    actual_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    assert model(actual_ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    if bytes(actual_ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", [(hex(i), hex(actual_ram[i]), hex(expected_ram[i]))
                                            for i in diff[:12]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (label, name, hex(actual), hex(expected))
    coverage.update(local_coverage)
    edges.update(local_edges)
    return "return"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path)
    parser.add_argument("--exe", type=Path)
    args = parser.parse_args()
    body = source(args.ovr, args.exe)
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u38_2002.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u38_leaf_2002.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u38_leaf_2002
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, sp, seed)
                 for mode in range(8)
                 for sp in (0x8000, 0xFFF8)
                 for seed in (0, 1)]
        coverage, edges, excluded = set(), set(), []
        returned = 0
        for case in cases:
            assert compare_case(body, model, str(case), make_case(body, *case), coverage, edges) == "return"
            returned += 1
        for label, options in extended_cases():
            if compare_case(body, model, label, fixture(**options), coverage, edges) == "return":
                returned += 1
            else:
                excluded.append(label)
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2002))
        expected = {i.address for i in decoded}
        outcomes = {(i.address, dest) for i in decoded
                    if i.mnemonic.startswith("j") and i.mnemonic != "jmp"
                    for dest in (i.address + i.size, int(i.op_str, 16))}
        print(f"U38 +2002 original-byte full RAM/14-register parity: {returned} returned cases PASS")
        print(f"Instruction coverage: {len(coverage & expected)}/{len(expected)}")
        print(f"Conditional outcomes: {len(edges & outcomes)}/{len(outcomes)}")
        print(f"Original instruction-budget exclusions ({len(excluded)}): {excluded}")
        assert excluded == ["frame-0001-0029", "frame-0002-0029", "frame-0003-0029",
                            "frame-0003-002b", "frame-0004-002b", "frame-0005-002b",
                            "frame-0005-002d", "frame-0006-002d"], excluded
        assert expected <= coverage, sorted(expected - coverage)
        assert outcomes <= edges, sorted(outcomes - edges)


if __name__ == "__main__":
    main()
