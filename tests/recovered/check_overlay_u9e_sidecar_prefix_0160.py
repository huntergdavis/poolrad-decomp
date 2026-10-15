#!/usr/bin/env python3
"""Original-byte/full-state parity before U9E:+018B resident sidecar call."""

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
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
FULL_SHA = "89a45482f4f42e1f4805cc4aa2448ebb685b777dbc7b91be985da4473bb208ba"
PREFIX_SHA = "55f45b6b7f6389eeb943ff7cd049e4168dd6f0232b59801c581bfe00c9fc9517"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(memory, seg, off, value):
    at = physical(seg, off)
    memory[at] = value & 255
    memory[(at + 1) & (RAM_SIZE - 1)] = (value >> 8) & 255


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    full = data[0x2B04A:0x2B1E4]
    prefix = data[0x2B04A:0x2B075]
    assert len(full) == 410 and sha256(full).hexdigest() == FULL_SHA
    assert len(prefix) == 43 and sha256(prefix).hexdigest() == PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(prefix, 0x0160))
    assert len(ins) == 15 and sum(x.size for x in ins) == 43
    assert ins[-1].address == 0x018A and ins[-1].mnemonic == "push"
    assert data[0x2B075:0x2B07A] == bytes.fromhex("9a9921ba00")
    return prefix + data[0x2B075:0x2B07A]


def make_case(code, ds, ss, sp, ax, flags, type_word):
    memory = bytearray(bytes(range(256)) * (RAM_SIZE // 256))
    cs = 0xF000
    at = physical(cs, 0x0160)
    memory[at:at + len(code)] = code
    cpu = Cpu(ax, 0x2345, 0x3456, 0x4567, 0x5678, 0x6789,
              0x789A, sp, cs, 0x0160, ds, 0x9876, ss, flags)
    put16(memory, ds, 0x5D96, 0xA432 ^ ax)
    put16(memory, ds, 0x5D98, 0x6B21 ^ sp)
    put16(memory, ss, (sp + 4) & 0xFFFF, type_word)
    put16(memory, ss, (sp + 6) & 0xFFFF, 0x1357 ^ ax)
    put16(memory, ss, (sp + 8) & 0xFFFF, 0x2468 ^ sp)
    put16(memory, ss, sp, 0x55AA)
    put16(memory, ss, (sp + 2) & 0xFFFF, 0xBEEF)
    return memory, cpu


def compare_case(label, code, model):
    memory, cpu = make_case(code, *label)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x0160)
    stop = physical(cpu.cs, 0x018B)
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == stop:
            reached[0] = True
            emu.emu_stop()
        else:
            assert start <= address < stop, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, stop + 5, count=24)
    assert reached[0], label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = model(ram, RAM_SIZE, ctypes.byref(cpu))
    assert status == 0, (label, status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:12]))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == expected, (label, name, hex(actual), hex(expected))


def main():
    code = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u9e-0160-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O0",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u9e_sidecar_prefix_0160.c"),
            "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library)).pr_u9e_sidecar_prefix_0160
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu)]
        model.restype = ctypes.c_int
        cases = [(ds, ss, sp, ax, flags, type_word)
                 for ds, ss in ((0x3000, 0xE000), (0x3000, 0x3000),
                                (0xF123, 0x0000), (0x0000, 0xF000))
                 for sp in (0x8000, 0x0100, 0x0000, 0xFFFF,
                            0x00E6, 0x5E80)
                 for ax, flags, type_word in ((0xA5CD, 0x72D3, 0x0031),
                                              (0x0042, 0x0803, 0xFF7D),
                                              (0xFF00, 0x0246, 0x1200))]
        for label in cases:
            compare_case(label, code, model)
    print(f"u9e:+0160..+018a original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
