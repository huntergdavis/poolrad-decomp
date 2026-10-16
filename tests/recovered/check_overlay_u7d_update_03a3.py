#!/usr/bin/env python3
"""Original-byte full-state differential for U7D +03A3 and its +02F2 callee."""

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
BODY_SHA = "3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad"
CALLEE_SHA = "8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9"
ENTRY = 0x03A3
END = 0x03D5


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


def put_linear_word(ram, at, value):
    ram[at & MASK] = value & 255
    ram[(at + 1) & MASK] = value >> 8


def source_audit(path):
    game = path.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    assert 0x23DCC + ENTRY == 147823
    callee = game[0x23DCC + 0x02F2:0x23DCC + ENTRY]
    body = game[147823:147873]
    assert len(callee) == 177 and sha256(callee).hexdigest() == CALLEE_SHA
    assert len(body) == 50 and sha256(body).hexdigest() == BODY_SHA
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    instructions = list(decoder.disasm(body, ENTRY))
    assert len(instructions) == 19 and sum(i.size for i in instructions) == 50
    assert instructions[-1].address == 0x03D4 and instructions[-1].mnemonic == "retf"
    assert [(i.address, i.op_str) for i in instructions if "call" in i.mnemonic] == [
        (0x03AC, "0x2f2")]
    branches = {i.address: (i.address + i.size, int(i.op_str, 16))
                for i in instructions if i.mnemonic == "jbe"}
    assert branches == {0x03B4: (0x03B6, 0x03D3), 0x03CB: (0x03CD, 0x03D3)}
    return callee + body, {i.address for i in instructions}, branches


def fixture(*, pending, base, multiplier, flags=0x7202, sp=0x8000,
            ss=0x2000, ds=0x1000, threshold0=0xFFFF, value0=0):
    memory = bytearray([0xA5]) * RAM_SIZE
    cs = 0x3000
    cpu = Cpu(0x1234, 0x5678, 0x9ABC, 0xDEF0, 0x1111, 0x2222,
              0x3333, sp, cs, ENTRY, ds, 0x4000, ss, flags)
    put_linear_word(memory, physical(ss, sp), 0xCAFE)
    put_linear_word(memory, physical(ss, sp) + 2, 0xBEEF)
    put_word(memory, ds, 0x5D96, 0)
    put_word(memory, ds, 0x5D98, 0)
    for i in range(7):
        put_word(memory, ds, 0x363A + 2 * i, 0xFFFF)
        put_word(memory, ds, 0x6DC2 + 2 * i, 0)
    put_word(memory, ds, 0x363A, threshold0)
    put_word(memory, ds, 0x6DC2, value0)
    put_word(memory, ds, 0x3642, multiplier)
    put_word(memory, ds, 0x6DCA, base)
    put_word(memory, ds, 0x6DCC, pending)
    return memory, cpu


def cases():
    result = [
        ("zero", dict(pending=0, base=0, multiplier=4)),
        ("uncapped", dict(pending=1, base=0, multiplier=4)),
        ("cap", dict(pending=3, base=90, multiplier=4)),
        ("mul-high", dict(pending=0x8000, base=0, multiplier=0xFFFE)),
        ("callee-carry", dict(pending=1, base=0, multiplier=4,
                               threshold0=2, value0=2)),
        ("offset-wrap", dict(pending=1, base=0, multiplier=4, sp=0xFFFE)),
        ("a20-stack", dict(pending=1, base=0, multiplier=4, ss=0xFFFF, sp=0x0100)),
        ("ds-ss-return-alias", dict(pending=1, base=0, multiplier=4,
                                     ss=0x1000, sp=0x6DCA)),
        ("flags-df", dict(pending=0, base=0, multiplier=4, flags=0x0603)),
    ]
    rng = random.Random(0x7D03A3)
    for n in range(40):
        result.append((f"seed-{n}", dict(pending=rng.randrange(0xFFFF),
                      base=rng.randrange(0x100), multiplier=rng.randrange(1, 0x100),
                      flags=0x7202 ^ (rng.randrange(0x1000) & 0x08D5))))
    return result


def build_c(directory):
    output = directory / "u7d_03a3.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                    "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u7d_carry_02f2.c"),
                    str(ROOT / "src/recovered/overlay_u7d_update_03a3.c"),
                    "-o", str(output)], check=True)
    function = ctypes.CDLL(str(output)).pr_u7d_update_03a3
    function.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                         ctypes.POINTER(Cpu))
    function.restype = ctypes.c_int
    return function


def differential(label, memory, cpu, code, function, starts, branches):
    original_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, RAM_SIZE, 7, ctypes.addressof(original_ram))
    original.mem_map_ptr(RAM_SIZE, RAM_SIZE, 7, ctypes.addressof(original_ram))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    ended = [False]
    visited = set()
    edges = set()
    previous = [None]

    def instruction(uc, address, size, user):
        del address, size, user
        if ended[0]:
            uc.emu_stop()
            return
        ip = uc.reg_read(regs.UC_X86_REG_IP)
        cs = uc.reg_read(regs.UC_X86_REG_CS)
        if cs != cpu.cs or not ENTRY <= ip < END:
            assert cs == cpu.cs and 0x02F2 <= ip < ENTRY, (label, hex(cs), hex(ip))
            return
        assert ip in starts, (label, hex(ip))
        if previous[0] in branches:
            assert ip in branches[previous[0]], (label, hex(previous[0]), hex(ip))
            edges.add((previous[0], ip))
        previous[0] = ip
        visited.add(ip)
        if ip == 0x03D4:
            ended[0] = True

    original.hook_add(UC_HOOK_CODE, instruction)
    start = physical(cpu.cs, ENTRY)
    try:
        original.emu_start(start, 2 * RAM_SIZE, count=20000)
    except Exception as exc:
        raise AssertionError(f"{label}: original emulator exception") from exc
    assert ended[0], (label, "did not reach outer RETF")
    expected_ram = bytes(original_ram)
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
    assert function(ram, RAM_SIZE, ctypes.byref(cpu)) == 0, label
    if memory != expected_ram:
        at = next(i for i, (a, b) in enumerate(zip(memory, expected_ram)) if a != b)
        raise AssertionError((label, "RAM", hex(at), memory[at], expected_ram[at]))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        observed = getattr(cpu, name)
        expected = original.reg_read(register) & 0xFFFF
        assert observed == expected, (label, name, hex(observed), hex(expected))
    return visited, edges


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, required=True)
    args = parser.parse_args()
    code, starts, branches = source_audit(args.original)
    visited, edges = set(), set()
    with tempfile.TemporaryDirectory(prefix="poolrad-u7d-03a3-") as temp:
        function = build_c(Path(temp))
        all_cases = cases()
        for label, options in all_cases:
            memory, cpu = fixture(**options)
            at = physical(cpu.cs, 0x02F2)
            memory[at:at + len(code)] = code
            reached, outcomes = differential(label, memory, cpu, code,
                                             function, starts, branches)
            visited.update(reached)
            edges.update(outcomes)
    assert visited == starts, ("missing instruction starts", sorted(starts - visited))
    expected_edges = {(site, target) for site, targets in branches.items() for target in targets}
    assert edges == expected_edges, ("missing branch outcomes", sorted(expected_edges - edges))
    print(f"U7D +03A3 with complete +02F2 callee: {len(all_cases)} original-byte/C "
          f"full-1MiB RAM+CPU14 cases PASS; {len(visited)}/19 starts, 4/4 branch outcomes")


if __name__ == "__main__":
    main()
