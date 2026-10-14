#!/usr/bin/env python3
"""Original-byte full-state audit of unit 0x1C:+2AA2 with a bounded view call."""

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
SPAN_SHA = "4b1715dc38485f2880c28b16c1e3e0a7887086d488173d70f9e70dd863f2b25c"
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
    a = physical(seg, off)
    memory[a] = value & 255
    memory[(a + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def get16(memory, seg, off):
    a = physical(seg, off)
    return memory[a] | memory[(a + 1) & (RAM_SIZE - 1)] << 8


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    body = data[0x37E8:0x3807]
    assert len(body) == 31 and sha256(body).hexdigest() == SPAN_SHA
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2AA2))
    assert len(instructions) == 10
    assert instructions[-1].address == 0x2AC0
    assert instructions[-1].mnemonic == "retf"
    assert instructions[-1].address + instructions[-1].size == 0x2AC1
    assert instructions[5].address == 0x2AB0
    assert instructions[5].mnemonic == "lcall"
    assert instructions[4].op_str == "0x2abf"
    return body


def make_case(body, dirty, pc, flags):
    memory = bytearray(RAM_SIZE)
    cs, ds, ss = 0xF000, 0x3000, 0xE000
    start = physical(cs, 0x2AA2)
    memory[start:start + len(body)] = body
    put16(memory, ds, 0x49ED, pc)
    memory[physical(ds, 0x84E3)] = dirty
    memory[physical(ds, 0x84E0)] = 0xA5
    put16(memory, ss, 0x8000, 0x0712)
    put16(memory, ss, 0x8002, 0xD000)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, 0x8000, cs, 0x2AA2, ds, 0x1111, ss, flags)
    return memory, cpu


def state_tuple(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def compare_case(label, memory, cpu, body, model, callback_type, mutate):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x2AA2)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0xF000, 0x2AB0):
            before = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            expected_calls.append(state_tuple(before))
            if mutate:
                emu.reg_write(UC_X86_REG_AX, 0x5A5A)
                emu.mem_write(physical(before.ds, 0x8460), b"\x73")
            emu.reg_write(UC_X86_REG_IP, 0x2AB5)
        elif not start <= address < start + len(body):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, RAM_SIZE, count=100)
    assert returned[0], label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def view(context, ram_pointer, ram_size, state_pointer):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append(state_tuple(state))
        if mutate:
            state.ax = 0x5A5A
            ram_pointer[physical(state.ds, 0x8460)] = 0x73
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), view, None)
    assert status == 0, (label, status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:12]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == observed, (label, name, hex(actual), hex(observed))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-2aa2-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_sprite_off_2aa2.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_sprite_off_2aa2
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(dirty, pc, flags, mutate)
                 for dirty in (0, 1, 0x80, 0xFF)
                 for pc in (0, 0xFFFF)
                 for flags, mutate in ((0x7202, False), (0x7AD3, True))]
        for dirty, pc, flags, mutate in cases:
            label = (dirty, pc, flags, mutate)
            memory, cpu = make_case(body, dirty, pc, flags)
            compare_case(label, memory, cpu, body, model, callback_type, mutate)
    print(f"u1c:+2AA2 full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
