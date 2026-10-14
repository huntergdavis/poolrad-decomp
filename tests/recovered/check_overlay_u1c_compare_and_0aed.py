#!/usr/bin/env python3
"""Original-byte/full-state parity for U1C:+0AED under bounded callees."""

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
SPAN_SHA = "c45465b78fc299d0f7e52d7bab18c58568e7c5c87759ae9f231561c35c24161f"
RAM_SIZE = 1 << 20
CALLS = {0x0AFF: (0, 8, 0x0B04), 0x0B07: (1, 2, 0x0B0C),
         0x0B19: (2, 2, 0x0B1E)}
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
    body = data[0x1833:0x1898]
    assert len(body) == 101 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0AED))
    assert len(ins) == 40 and sum(x.size for x in ins) == 101
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x0B51 and ins[-1].mnemonic == "retf"
    assert ins[-1].address + ins[-1].size == 0x0B52
    return body


def make_case(body, values, ds, ss, sp, touch):
    memory = bytearray(RAM_SIZE)
    cs = 0xF000
    memory[physical(cs, 0x0AED):physical(cs, 0x0AED) + len(body)] = body
    for at in range(0x6F78, 0x6F7E):
        memory[physical(ds, at)] = 0xA5
    put16(memory, ss, sp, 0x0712)
    put16(memory, ss, sp + 2, 0xD000)
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, cs, 0x0AED, ds, 0x1111,
              ss, 0x72D3)
    return memory, cpu, values, touch


def snap(state):
    values = [getattr(state, name) for name, _ in Cpu._fields_]
    values[-1] &= 0xFFEF  # AF is undefined after XOR and SHL.
    return tuple(values)


def args(memory, state, event):
    return tuple(get16(memory, state.ss, state.sp + i * 2)
                 for i in range(4 if event == 0 else 1))


def apply_call(memory, state, event, values, touch, count):
    if event == 0:
        value, size, offset, segment = args(memory, state, event)
        for i in range(size):
            memory[physical(segment, offset + i)] = value & 255
        if touch:
            memory[physical(state.ds, 0x8460)] = 0x73
            state.dx = 0x5A5A
    elif event == 1:
        memory[physical(state.ds, 0x49ED)] = 0x9E
        state.ax = 0xB100
    else:
        state.ax = values[count]
        state.flags = 0x0A53


def compare_case(label, memory, cpu, body, model, callback_type, values, touch):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x0AED)
    expected_calls = []
    returned = [False]
    value_count = [0]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            event, cleanup, next_ip = CALLS[off]
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, snap(state), args(snapshot, state, event)))
            apply_call(snapshot, state, event, values, touch, value_count[0])
            if event == 2:
                value_count[0] += 1
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + cleanup) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, next_ip)
        elif not start <= address < start + len(body):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, RAM_SIZE, count=200)
    assert returned[0] and value_count[0] == 4, (label, value_count)
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []
    actual_count = [0]

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((event, snap(state), args(ram_pointer, state, event)))
        apply_call(ram_pointer, state, event, values, touch, actual_count[0])
        if event == 2:
            actual_count[0] += 1
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), external, None)
    assert status == 0, (label, status)
    if bytes(ram) != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(ram, expected_ram)) if a != b]
        raise AssertionError((label, "RAM", diff[:12]))
    assert actual_calls == expected_calls, (label, "calls", actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        if name == "flags":
            observed &= 0xFFEF
            actual &= 0xFFEF
        assert actual == observed, (label, name, hex(actual), hex(observed))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int)
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0aed-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_compare_and_0aed.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_compare_and_0aed
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        patterns = ((0, 0, 0, 0), (1, 2, 3, 3), (1, 1, 3, 4),
                    (0x7FFF, 0x7FFF, 0x8000, 0x8000),
                    (0xFFFF, 0, 0, 0))
        cases = [(pattern, ds, ss, sp, touch)
                 for pattern in patterns
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x6F86))
                 for touch in (False, True)]
        for label in cases:
            memory, cpu, vals, touch = make_case(body, *label)
            compare_case(label, memory, cpu, body, model, callback_type, vals, touch)
    print(f"u1c:+0AED full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
