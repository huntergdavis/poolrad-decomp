#!/usr/bin/env python3
"""Two original SYSTEM copies to U3B0 +0DC7 full-state C parity."""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
FIRST_SHA = "7ecaa950d8a79a4828f9f8106c7c57c890b34cc7c30d0cb39c404936d6cba427"
SECOND_SHA = "f25a0d186aa9a847bab7b9c5cc291c6b2498cd9f4c986c487c3f825d10b1ecd5"
GATE_SHA = "643d9d5fed1d2538537a21300fe535c8ebd4345bfd1a04290e982e9796ee56dc"
COPY_SHA = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
RAM_SIZE = 1 << 20
MASK = RAM_SIZE - 1
CS, SS, DS, SRC = 0xF000, 0x7000, 0x6000, 0x5000
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


def source():
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA
    assert len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA
    first = ovr[0x2D669:0x2D67E]
    second = ovr[0x2D683:0x2D691]
    gate = ovr[0x2D696:0x2D69E]
    copy = exe[0xB79B:0xB7BF]
    assert len(first) == 21 and sha256(first).hexdigest() == FIRST_SHA
    assert len(second) == 14 and sha256(second).hexdigest() == SECOND_SHA
    assert len(gate) == 8 and sha256(gate).hexdigest() == GATE_SHA
    assert len(copy) == 36 and sha256(copy).hexdigest() == COPY_SHA
    assert ovr[0x2D67E:0x2D683] == bytes.fromhex("9a250bf80a")
    assert ovr[0x2D691:0x2D696] == bytes.fromhex("9a250bf80a")
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(gate, 0x0DC7))
    assert [(i.address, i.mnemonic) for i in ins] == [
        (0x0DC7, "mov"), (0x0DCA, "or"), (0x0DCD, "jne")]
    assert ins[-1].op_str == "0xdea"
    return ovr[0x2D669:0x2D69E], copy


def cpu_from_emu(emu):
    return Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))


def load_cpu(emu, cpu):
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        emu.reg_write(reg, getattr(cpu, name))


def seed_string(ram, off, length, seed):
    ram[physical(SRC, off)] = length
    for i in range(length):
        ram[physical(SRC, off + 1 + i)] = (seed + i * 13) & 255


def two_copy_frame(code, copy, case):
    sp, lengths, head, seed = case
    ram = bytearray(RAM_SIZE)
    at = physical(CS, 0x0D9A)
    ram[at:at + len(code)] = code
    at = physical(0x0AF8, 0x0B25)
    ram[at:at + len(copy)] = copy
    cpu = Cpu(0xABCD, 0x1357, 0x2468, 0x3579, 0x468A, 0x579B,
              0x68AC, sp, CS, 0x0D9A, DS, 0x8000, SS,
              0x0202 if seed == 0 else 0x0AD7)
    bp = (sp - 2) & 0xFFFF
    for i in range(-0x1B0, 0x40):
        ram[physical(SS, sp + i)] = (i * 19 + seed * 23) & 255
    put16(ram, SS, sp, 0x1234)
    put16(ram, SS, sp + 2, 0xD000)
    put16(ram, SS, bp + 0x2A, 0x1000)
    put16(ram, SS, bp + 0x2C, SRC)
    put16(ram, SS, bp + 0x26, 0x1100)
    put16(ram, SS, bp + 0x28, SRC)
    put16(ram, SS, bp + 0x14, head[0])
    put16(ram, SS, bp + 0x16, head[1])
    seed_string(ram, 0x1000, lengths[0], 0x21)
    seed_string(ram, 0x1100, lengths[1], 0x62)
    emu = Uc(UC_ARCH_X86, UC_MODE_16)
    emu.mem_map(0, RAM_SIZE)
    emu.mem_write(0, bytes(ram))
    load_cpu(emu, cpu)
    trace = []

    def stop_after_copies(uc, address, size, user):
        del size, user
        cs = uc.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = uc.reg_read(UC_X86_REG_IP) & 0xFFFF
        assert address == physical(cs, ip)
        trace.append((cs, ip))
        if (cs, ip) == (CS, 0x0DC7):
            uc.emu_stop()

    emu.hook_add(UC_HOOK_CODE, stop_after_copies)
    emu.emu_start(physical(CS, 0x0D9A), RAM_SIZE, count=2000)
    assert trace[0] == (CS, 0x0D9A) and trace[-1] == (CS, 0x0DC7)
    assert trace.count((0x0AF8, 0x0B25)) == 2
    assert (CS, 0x0DB4) in trace and (CS, 0x0DC2) in trace
    after = bytes(emu.mem_read(0, RAM_SIZE))
    returned = cpu_from_emu(emu)
    assert returned.sp == (bp - 0x018E) & 0xFFFF
    assert returned.bp == bp and returned.ds == DS
    assert returned.ip == 0x0DC7 and returned.cs == CS
    for dest, length, seed_value in (
        (bp - 0x2A, lengths[0], 0x21),
        (bp - 0x53, lengths[1], 0x62),
    ):
        copied = min(length, 0x28)
        assert after[physical(SS, dest)] == copied
        assert bytes(after[physical(SS, dest + 1 + i)]
                     for i in range(copied)) == bytes(
                         (seed_value + i * 13) & 255 for i in range(copied))
    return after, returned


def compare_case(code, copy, model, case):
    frame_ram, frame_cpu = two_copy_frame(code, copy, case)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, frame_ram)
    load_cpu(original, frame_cpu)
    reached = []

    def stop_at_branch(emu, address, size, user):
        del size, user
        if emu.reg_read(UC_X86_REG_CS) == CS and (
                emu.reg_read(UC_X86_REG_IP) & 0xFFFF) in (0x0DCF, 0x0DEA):
            reached.append(address)
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, stop_at_branch)
    original.emu_start(physical(CS, 0x0DC7), RAM_SIZE, count=10)
    _, _, head, _ = case
    expected_ip = 0x0DEA if head != (0, 0) else 0x0DCF
    assert reached == [physical(CS, expected_ip)], (case, reached)
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    actual_ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(frame_ram)
    actual_cpu = Cpu.from_buffer_copy(frame_cpu)
    assert model(actual_ram, RAM_SIZE, ctypes.byref(actual_cpu)) == 0, case
    if bytes(actual_ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((case, "RAM", [(hex(i), hex(actual_ram[i]),
                                            hex(expected_ram[i]))
                                           for i in diff[:12]]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(actual_cpu, name)
        assert actual == expected, (case, name, hex(actual), hex(expected))
    assert actual_cpu.ip == expected_ip


def main():
    code, copy = source()
    with tempfile.TemporaryDirectory(dir=ROOT / "resources") as temp:
        shared = Path(temp) / "u3b0_0dc7.so"
        subprocess.run([
            "cc", "-shared", "-fPIC", "-std=c11", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3b0_entry4_node_gate_0dc7.c"),
            "-o", str(shared)], check=True)
        model = ctypes.CDLL(str(shared)).pr_u3b0_entry4_node_gate_0dc7
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu))
        model.restype = ctypes.c_int
        cases = [(sp, lengths, head, seed)
                 for sp in (0x8000, 0x9001)
                 for lengths in ((0, 0), (5, 9), (44, 60))
                 for head in ((0, 0), (0x2200, 0), (0, SRC),
                              (0x2200, SRC))
                 for seed in (0, 1)]
        for case in cases:
            compare_case(code, copy, model, case)
    print(f"U3B0 +0DC7 node gate, two original copies, full RAM/14-register parity: {len(cases)} cases PASS")


if __name__ == "__main__":
    main()
