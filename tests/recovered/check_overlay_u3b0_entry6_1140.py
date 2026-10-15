#!/usr/bin/env python3
"""Original-byte/full-state parity for overlay unit 0x3B0 entry 6."""

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
SPAN_SHA = "fdced3a4a7f0c069d55a9a4fef8a048eca70e649ec66559b84240259d2280c5c"
RAM_SIZE = 1 << 20
CALLS = {0x114A: 0x114F, 0x11A8: 0x11AD}
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
    body = data[0x2DA0F:0x2DAC2]
    assert len(body) == 179 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x1140))
    assert len(ins) == 58 and sum(x.size for x in ins) == 179
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x11F0 and ins[-1].mnemonic == "retf"
    assert ins[-1].op_str == "6" and ins[-1].address + ins[-1].size == 0x11F3
    assert data[0x2DAC2:0x2DAC5] == bytes.fromhex("5589e5")
    return body


def make_case(body, count, mode, ds, ss, sp):
    memory = bytearray(RAM_SIZE)
    cs = 0xF000
    start = physical(cs, 0x1140)
    memory[start:start + len(body)] = body
    put16(memory, ss, sp, 0x0712)
    put16(memory, ss, sp + 2, 0xD000)
    if mode == "return":
        output_seg, output_off = ss, sp
    elif mode == "wrap":
        output_seg, output_off = ss, 0xFFFF
    else:
        output_seg, output_off = ds, 0x7000
    put16(memory, ss, sp + 4, output_off)
    put16(memory, ss, sp + 6, output_seg)
    put16(memory, ss, sp + 8, count)
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, cs, 0x1140, ds, 0x1111,
              ss, 0x72D3)
    return memory, cpu


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def alloc_pointer(index):
    return 0x4000 + index * 0x40, 0x2200 + index * 0x100


def apply_alloc(memory, state, index):
    assert get16(memory, state.ss, state.sp) == 0x002E
    assert state.ip == (0x114A if index == 0 else 0x11A8)
    off, seg = alloc_pointer(index)
    for i in range(0x2E):
        memory[physical(seg, off + i)] = (0xB0 + index + i) & 255
    state.ax = off
    state.dx = seg
    state.bx = (0xB000 + index) & 0xFFFF
    state.flags = 0x0A53


def compare_case(label, memory, cpu, body, model, callback_type):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x1140)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((snap(state), get16(snapshot, state.ss, state.sp)))
            apply_alloc(snapshot, state, len(expected_calls) - 1)
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + 2) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, CALLS[off])
        elif not start <= address < start + len(body):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, RAM_SIZE, count=500)
    assert returned[0], label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def alloc(context, ram_pointer, ram_size, state_pointer):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((snap(state), get16(ram_pointer, state.ss, state.sp)))
        apply_alloc(ram_pointer, state, len(actual_calls) - 1)
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), alloc, None)
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u3b0-1140-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u3b0_entry6_1140.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u3b0_entry6_1140
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(count, mode, ds, ss, sp)
                 for count in (0, 1, 2, 3, 4, 0xFFFF, 0x8000)
                 for mode, ds, ss, sp in (("normal", 0x3000, 0xE000, 0x8000),
                                           ("return", 0x3000, 0x3000, 0x708B),
                                           ("wrap", 0xF123, 0xE000, 0x0002))]
        for label in cases:
            memory, cpu = make_case(body, *label)
            compare_case(label, memory, cpu, body, model, callback_type)
    print(f"u3b0:+1140 original-byte full-state parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
