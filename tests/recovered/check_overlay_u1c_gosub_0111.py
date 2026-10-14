#!/usr/bin/env python3
"""Original-byte/full-state parity for U1C:+0111 under bounded callees."""

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
SPAN_SHA = "7917b6be7b4ae2f3d53de4c2a9d80ec430fba207ca34e689e88ac2186c4b0edc"
RAM_SIZE = 1 << 20
CALLS = {0x0117: (0, 0x011C), 0x011F: (1, 0x0124)}
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
    body = data[0x0E57:0x0E6C]
    assert len(body) == 21 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0111))
    assert len(ins) == 10 and sum(x.size for x in ins) == 21
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x0125 and ins[-1].mnemonic == "retf"
    assert ins[-1].address + ins[-1].size == 0x0126
    return body


def make_case(body, ah, ds, ss, sp, touch):
    memory = bytearray(RAM_SIZE)
    cs = 0xF000
    start = physical(cs, 0x0111)
    memory[start:start + len(body)] = body
    put16(memory, ss, sp, 0x0712)
    put16(memory, ss, sp + 2, 0xD000)
    cpu = Cpu((ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, cs, 0x0111, ds, 0x1111,
              ss, 0x72D3)
    return memory, cpu, touch


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def apply_call(memory, state, event, touch):
    assert get16(memory, state.ss, state.sp) & 255 == 1
    if event == 0:
        put16(memory, state.ds, 0x49ED, 0x2357)
        state.ax = 0xB100
        state.flags = 0x0A53
    else:
        put16(memory, state.ds, 0x49ED, 0x4567)
        put16(memory, state.ds, 0x6F70, 0x2468)
        put16(memory, state.ds, 0x6F72, 0x1357)
        state.dx = 0x5A5A
        if touch:
            put16(memory, state.ss, state.sp + 2, 0x3344)
            put16(memory, state.ss, state.sp + 4, 0x7788)
            put16(memory, state.ss, state.sp + 6, 0x99AA)


def compare_case(label, memory, cpu, body, model, callback_type, touch):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x0111)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            event, next_ip = CALLS[off]
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, snap(state), get16(snapshot, state.ss, state.sp)))
            apply_call(snapshot, state, event, touch)
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + 2) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, next_ip)
        elif not start <= address < start + len(body):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, RAM_SIZE, count=35)
    assert returned[0] and len(expected_calls) == 2, label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((event, snap(state), get16(ram_pointer, state.ss, state.sp)))
        apply_call(ram_pointer, state, event, touch)
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
        assert actual == observed, (label, name, hex(actual), hex(observed))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int)
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0111-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_gosub_0111.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_gosub_0111
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(ah, ds, ss, sp, touch)
                 for ah in (0, 0xA5)
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x6F74),
                                    (0xF123, 0xE000, 0x0002))
                 for touch in (False, True)]
        for label in cases:
            memory, cpu, touch = make_case(body, *label)
            compare_case(label, memory, cpu, body, model, callback_type, touch)
    print(f"u1c:+0111 full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
