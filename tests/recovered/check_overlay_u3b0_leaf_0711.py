#!/usr/bin/env python3
"""Full original-byte RAM/CPU parity for GAME.OVR U3B0 +0711."""

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
BODY_SHA = "a3510458227e9373193717033475fa31fe5d1bf0ef01a33da6582726d6650092"
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
    ram[(at + 1) & MASK] = value >> 8


def put_node(ram, node, flag, next_node):
    off, seg = node
    ram[physical(seg, off + 0x29)] = flag
    put16(ram, seg, off + 0x2A, next_node[0])
    put16(ram, seg, off + 0x2C, next_node[1])


def source(ovr_path=None, exe_path=None):
    ovr = (ovr_path or ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    exe = (exe_path or ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
                if u["stub_para"] == 0x3B0)
    assert unit["ovr_offset"] == 0x2C8CF and unit["memory_segment"] == 0x3C5
    base = unit["ovr_offset"]
    parent = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(
        ovr[base + 0x7A4:base + 0x7FC], 0x7A4))
    assert [(i.address, i.mnemonic, i.op_str) for i in parent[-2:]] == [
        (0x7F8, "push", "cs"), (0x7F9, "call", "0x711")]
    print("PASS: U3B0 descriptor and aligned PUSH CS / CALL +0711 at +07F9")
    body = ovr[0x2CFE0:0x2D073]
    assert len(body) == 147 and sha256(body).hexdigest() == BODY_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0711))
    assert len(ins) == 52 and sum(i.size for i in ins) == 147
    assert ins[0].address == 0x0711 and ins[-1].address == 0x07A1
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "8"
    assert not any(i.mnemonic in ("call", "lcall") for i in ins)
    addresses = {i.address for i in ins}
    assert all(int(i.op_str, 16) in addresses for i in ins if i.mnemonic.startswith("j"))
    print(f"Body: {len(body)} bytes / {len(ins)} instructions; SHA256 {BODY_SHA}")
    return body


def make_case(body, mode, bound, sp):
    ram = bytearray(RAM_SIZE)
    ram[physical(0xF000, 0x0711):physical(0xF000, 0x0711) + len(body)] = body
    ss = 0x7000
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, 0xF000, 0x0711, 0x6000, 0x8000, ss,
              0x0202 if sp == 0x8000 else 0x0AD7)
    for i in range(-32, 24):
        ram[physical(ss, sp + i)] = (i * 19 + mode * 23 + bound) & 255
    put16(ram, ss, sp, 0x1234)
    put16(ram, ss, sp + 2, 0xD000)
    put16(ram, ss, sp + 4, 0xA5A5)
    a, b, c = (0x2200, 0x5000), (0x2300, 0x5000), (0x2400, 0x5000)
    if mode == 0:
        a = (0, 0)
    elif mode == 5:
        a = ((sp - 0x37) & 0xFFFF, ss)
    elif mode == 6:
        a = (0xFFD8, 0x5000)
    elif mode == 7:
        b = (0x2210, 0x4FFF)  # Physical alias of A.
    elif mode not in (1, 2, 3, 4):
        raise ValueError(mode)
    if mode != 0:
        flags = {
            1: (0, 1, 0), 2: (1, 0, 1), 3: (0, 0, 0),
            4: (1, 1, 1), 5: (1, 0, 1), 6: (1, 0, 1),
            7: (0, 1, 0),
        }[mode]
        put_node(ram, a, flags[0], b)
        put_node(ram, b, flags[1], c)
        put_node(ram, c, flags[2], a if mode in (3, 4) else (0, 0))
    put16(ram, ss, sp + 6, a[0])
    put16(ram, ss, sp + 8, a[1])
    put16(ram, ss, sp + 10, bound)
    return ram, cpu


def fixture(*, bound=3, node_flags=(0, 1, 0), head=(0x2200, 0x5000),
            sp=0x8000, ss=0x7000, cs=0xF000, flags=0x202, cycle=False,
            alias=False, seed=None):
    ram = bytearray([0xA5]) * RAM_SIZE if seed is None else bytearray(
        random.Random(seed).randbytes(RAM_SIZE))
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, cs, 0x0711, 0x6000, 0x8000, ss, flags)
    second = (((head[0] + 16) & 0xFFFF, (head[1] - 1) & 0xFFFF)
              if alias else (0x2300, 0x5000))
    third = (0x2400, 0x5000)
    put_node(ram, head, node_flags[0], second)
    put_node(ram, second, node_flags[1], third)
    put_node(ram, third, node_flags[2], head if cycle else (0, 0))
    for off, value in ((0, 0x1234), (2, 0xD000), (4, 0xA5A5),
                       (6, head[0]), (8, head[1]), (10, bound)):
        put16(ram, ss, sp + off, value)
    return ram, cpu


def extended_cases():
    for slot in range(3):
        for flag in range(256):
            values = [0, 0, 0]
            values[slot] = flag
            yield f"flag-{slot}-{flag:02x}", dict(node_flags=values)
    for flag in range(256):
        yield f"fallback-{flag:02x}", dict(bound=0, node_flags=(flag, 0, 0))
    for bound in (0, 1, 2, 3, 4, 15, 255, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF):
        for values in ((0, 0, 0), (1, 0, 1), (0x80, 0xFF, 0)):
            yield f"bound-{bound:04x}-{values}", dict(bound=bound, node_flags=values)
    for bound in (0, 1, 2, 3, 4, 15, 255, 0x7FFF):
        yield f"cycle-{bound:04x}", dict(bound=bound, cycle=True)
    for sp in tuple(range(24)) + tuple(range(0xFFF0, 0xFFFE)):
        for bound in (0, 1, 3, 0x8000):
            yield f"stack-{sp:04x}-{bound:04x}", dict(sp=sp, bound=bound)
    for delta in range(-48, 24):
        yield f"local-alias-{delta}", dict(head=((0x8000 + delta - 0x29) & 0xFFFF, 0x7000))
    for offset in (0, 1, 0xFFCF, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3,
                   0xFFD4, 0xFFD5, 0xFFD6, 0xFFD7, 0xFFD8, 0xFFD9, 0xFFFF):
        for seg in (0, 0x5000, 0xFFFF):
            yield f"node-seam-{seg:04x}-{offset:04x}", dict(head=(offset, seg))
    for head in ((0, 0), (0, 0xFFFF), (0xFFFF, 0), (0x2200, 0x5000)):
        for bound in (0, 3, 0x8000):
            yield f"physical-alias-{head}-{bound}", dict(head=head, bound=bound, alias=True)
    for flag in (0, 1, 0x7F, 0x80, 0xFF):
        for bound in (0, 3):
            yield f"null-fallback-{flag}-{bound}", dict(head=(0, 0), bound=bound,
                                                       node_flags=(flag, 0, 0))
    rng = random.Random(0x3B00711)
    for i in range(128):
        yield f"random-{i}", dict(node_flags=[rng.randrange(256) for _ in range(3)],
            bound=rng.choice((0, 1, 2, 3, 4, 0x7FFF, 0x8000, 0xFFFF)),
            ss=rng.choice((0, 0x7000, 0xFFFF)), cs=rng.choice((0x1000, 0xF000, 0xFFFF)),
            flags=rng.choice((0x202, 0xAD7, 0x602)), seed=i)


# Shared A20 memory and actual outer RETF follow the U38 +2002 audit.
def compare_case(body, model, label, initial, coverage, edges):
    ram, cpu = initial
    instructions = {i.address:i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0711)}
    code = {}
    for i, value in enumerate(body):
        at = (physical(cpu.cs, 0x0711) + i) & MASK
        ram[at] = value
        code[at] = value
    backing = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(ram)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    reached, previous, visits = [], [None], [0]
    local_coverage, local_edges = set(), set()

    def instruction(emu, address, size, user):
        del address, size, user
        ip = emu.reg_read(UC_X86_REG_IP)
        assert ip in instructions, (label, "unexpected PC", hex(ip))
        ins = instructions[ip]
        if ip == 0x740:
            visits[0] += 1
        local_coverage.add(ip)
        if previous[0] is not None:
            local_edges.add((previous[0], ip))
        previous[0] = ip if ins.mnemonic.startswith("j") and ins.mnemonic != "jmp" else None
        if ins.mnemonic == "retf":
            reached.append(ip)
            emu.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, instruction)
    original.emu_start(physical(cpu.cs, cpu.ip), RAM_SIZE * 2, count=2000000)
    original.hook_del(hook)
    assert reached, (label, "original instruction budget exhausted")
    if label.startswith("cycle-"):
        assert visits[0] == int(label.split("-")[1], 16), (label, visits)
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
        shared = Path(temp) / "u3b0_0711.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_leaf_0711.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_leaf_0711
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(mode, bound, sp)
                 for mode in range(8)
                 for bound in (0, 1, 2, 3, 4, 0x8000, 0xFFFF)
                 for sp in (0x8000, 0xFFFB)]
        coverage, edges = set(), set()
        returned = 0
        for case in cases:
            assert compare_case(body, model, str(case), make_case(body, *case), coverage, edges) == "return"
            returned += 1
        for label, options in extended_cases():
            assert compare_case(body, model, label, fixture(**options), coverage, edges) == "return"
            returned += 1
        decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0711))
        expected = {i.address for i in decoded}
        outcomes = {(i.address, dest) for i in decoded
                    if i.mnemonic.startswith("j") and i.mnemonic != "jmp"
                    for dest in (i.address + i.size, int(i.op_str, 16))}
        print(f"U3B0 +0711 full 1 MiB RAM/14 CPU words: {returned} returning cases PASS; no exclusions")
        print(f"Instruction coverage: {len(coverage & expected)}/{len(expected)}")
        print(f"Conditional outcomes: {len(edges & outcomes)}/{len(outcomes)}")
        print("Cyclic maximum signed bound: 32767 original node visits asserted")
        assert expected <= coverage, sorted(expected - coverage)
        assert outcomes <= edges, sorted(outcomes - edges)


if __name__ == "__main__":
    main()
