#!/usr/bin/env python3
"""Full-state original-byte audit of U1C:+0A9F under bounded FreeMem."""

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
SPAN_SHA = "a0b08ebfbd9bab9e5a735600cbc6120d0e56d67f77f585d2d07705db74d1681c"
RAM_SIZE = 1 << 20
DEFINED_FLAGS = 0xFFEF  # AF is undefined after the body's OR instruction.
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
    body = data[0x17E5:0x1833]
    assert len(body) == 78 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0A9F))
    assert len(ins) == 25 and sum(x.size for x in ins) == 78
    assert {x.address for x in ins if x.mnemonic == "lcall"} == {0x0AD8}
    assert ins[-1].address == 0x0AEC and ins[-1].mnemonic == "retf"
    assert ins[-1].address + ins[-1].size == 0x0AED
    assert {x.address for x in ins if x.mnemonic == "je"} == {0x0AAD}
    assert ins[5].op_str == "0xaeb"
    return body


def make_case(body, head, next_ptr, pc, callback_mutates):
    memory = bytearray(RAM_SIZE)
    cs, ds, ss = 0xF000, 0x3000, 0xE000
    start = physical(cs, 0x0A9F)
    memory[start:start + len(body)] = body
    put16(memory, ds, 0x49ED, pc)
    put16(memory, ds, 0x6F70, head[0])
    put16(memory, ds, 0x6F72, head[1])
    put16(memory, ds, 0x6F74, 0xA1A1)
    put16(memory, ds, 0x6F76, 0xB2B2)
    if head != (0, 0):
        put16(memory, head[1], head[0], 0x9137)
        put16(memory, head[1], head[0] + 2, next_ptr[0])
        put16(memory, head[1], head[0] + 4, next_ptr[1])
    put16(memory, ss, 0x8000, 0x0712)
    put16(memory, ss, 0x8002, 0xD000)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, 0x8000, cs, 0x0A9F,
              ds, 0x1111, ss, 0x72D3)
    return memory, cpu, callback_mutates


def state_tuple(state):
    values = [getattr(state, name) for name, _ in Cpu._fields_]
    values[-1] &= DEFINED_FLAGS
    return tuple(values)


def args(memory, state):
    return tuple(get16(memory, state.ss, state.sp + i * 2) for i in range(3))


def compare_case(label, memory, cpu, body, model, callback_type, mutate):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x0A9F)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(0xF000, 0x0AD8):
            before = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((state_tuple(before), args(snapshot, before)))
            if mutate:
                emu.mem_write(physical(before.ds, 0x6F74), b"\x44\x33")
                emu.reg_write(UC_X86_REG_AX, 0x5A5A)
            emu.reg_write(UC_X86_REG_SP, (before.sp + 6) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, 0x0ADD)
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
    def free_node(context, ram_pointer, ram_size, state_pointer):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((state_tuple(state), args(ram_pointer, state)))
        if mutate:
            put16(ram_pointer, state.ds, 0x6F74, 0x3344)
            state.ax = 0x5A5A
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), free_node, None)
    assert status == 0, (label, status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:12]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        if name == "flags":
            observed &= DEFINED_FLAGS
            actual &= DEFINED_FLAGS
        assert actual == observed, (label, name, hex(actual), hex(observed))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu))
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0a9f-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_return_0a9f.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_return_0a9f
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(head, next_ptr, pc, mutate)
                 for head, next_ptr in (
                     ((0, 0), (0, 0)),
                     ((0x0100, 0), (0, 0)),
                     ((0, 0x4000), (0x0300, 0x6000)),
                     ((0xFFFE, 0x4000), (0, 0x6000)),
                     ((0x0100, 0x4000), (0x0300, 0x6000)))
                 for pc, mutate in ((0, False), (0xFFFF, True))]
        for label in cases:
            memory, cpu, changes = make_case(body, *label)
            compare_case(label, memory, cpu, body, model, callback_type, changes)
    print(f"u1c:+0A9F full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
