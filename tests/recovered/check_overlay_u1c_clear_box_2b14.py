#!/usr/bin/env python3
"""Original-byte full-state audit of unit 0x1C:+2B14 with bounded display."""

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
SPAN_SHA = "69641d023ee5478d48a8dd54832eda981a2d9cb1c87aeff0d08435e27ae3c0bf"
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
    body = data[0x385A:0x3879]
    assert len(body) == 31 and sha256(body).hexdigest() == SPAN_SHA
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2B14))
    assert len(instructions) == 15
    assert instructions[11].address == 0x2B27
    assert instructions[11].mnemonic == "lcall"
    assert instructions[-1].address == 0x2B32
    assert instructions[-1].mnemonic == "retf"
    assert instructions[-1].address + instructions[-1].size == 0x2B33
    return body


def make_case(body, pc, ah, flags):
    memory = bytearray(RAM_SIZE)
    cs, ds, ss = 0xF000, 0x3000, 0xE000
    start = physical(cs, 0x2B14)
    memory[start:start + len(body)] = body
    put16(memory, ds, 0x49ED, pc)
    memory[physical(ds, 0x84DE)] = 0xA5
    put16(memory, ss, 0x8000, 0x0712)
    put16(memory, ss, 0x8002, 0xD000)
    cpu = Cpu((ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, 0x8000, cs, 0x2B14, ds, 0x1111,
              ss, flags)
    return memory, cpu


def state_tuple(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state):
    return tuple(get16(memory, state.ss, state.sp + i * 2) for i in range(4))


def compare_case(label, memory, cpu, body, model, callback_type, mutate):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x2B14)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0xF000, 0x2B27):
            before = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((state_tuple(before), args(snapshot, before)))
            if mutate:
                emu.reg_write(UC_X86_REG_AX, 0x5A5A)
                emu.mem_write(physical(before.ds, 0x8460), b"\x73")
            emu.reg_write(UC_X86_REG_SP, (before.sp + 8) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, 0x2B2C)
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
    def display(context, ram_pointer, ram_size, state_pointer):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((state_tuple(state), args(ram_pointer, state)))
        if mutate:
            state.ax = 0x5A5A
            ram_pointer[physical(state.ds, 0x8460)] = 0x73
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), display, None)
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-2b14-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_clear_box_2b14.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_clear_box_2b14
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(pc, ah, flags, mutate)
                 for pc in (0, 0x000F, 0x7FFF, 0xFFFF)
                 for ah in (0, 0xA5)
                 for flags, mutate in ((0x7202, False), (0x7AD3, True))]
        for pc, ah, flags, mutate in cases:
            label = (pc, ah, flags, mutate)
            memory, cpu = make_case(body, pc, ah, flags)
            compare_case(label, memory, cpu, body, model, callback_type, mutate)
    print(f"u1c:+2B14 full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
