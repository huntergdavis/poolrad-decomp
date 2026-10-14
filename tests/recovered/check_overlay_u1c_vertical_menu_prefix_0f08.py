#!/usr/bin/env python3
"""Original-byte/full-state parity for the bounded U1C:+0F08 prefix."""

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
FULL_SHA = "937fbfd3ae61653216beaff6084175a35be1b22c3f0c4158d5379463c7d12288"
PREFIX_SHA = "3d8da2c70bec5698c5cf0349d5355fd0d6dba50c008fa51e01e8aafc2a5623e5"
RAM_SIZE = 1 << 20
CALLS = {0x0F25: (0, 2, 0x0F2A, 1), 0x0F32: (1, 4, 0x0F37, 2),
         0x0F4A: (2, 10, 0x0F4F, 5), 0x0F52: (3, 2, 0x0F57, 1),
         0x0F62: (0, 2, 0x0F67, 1)}
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
    full = data[0x1C4E:0x1DCB]
    assert len(full) == 381 and sha256(full).hexdigest() == FULL_SHA
    body = data[0x1C4E:0x1CB9]
    assert len(body) == 107 and sha256(body).hexdigest() == PREFIX_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0F08))
    assert len(ins) == 39 and sum(x.size for x in ins) == 107
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x0F72 and ins[-1].mnemonic == "push"
    assert ins[-1].address + ins[-1].size == 0x0F73
    assert data[0x1CB9:0x1CBE] == bytes.fromhex("9a3e00c503")
    return body + data[0x1CB9:0x1CBE]


def make_case(code, length, ds, ss, sp, pc, n, touch):
    memory = bytearray(RAM_SIZE)
    cs = 0xF000
    start = physical(cs, 0x0F08)
    memory[start:start + len(code)] = code
    put16(memory, ss, sp, 0x0712)
    put16(memory, ss, sp + 2, 0xD000)
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, cs, 0x0F08, ds, 0x1111,
              ss, 0x72D3)
    memory[physical(ds, 0x70C8)] = length
    for i in range(length):
        memory[physical(ds, 0x70C9 + i)] = 65 + i % 26
    put16(memory, ds, 0x49ED, pc)
    return memory, cpu, length, n, touch


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state):
    count = CALLS[state.ip][3]
    return tuple(get16(memory, state.ss, state.sp + i * 2)
                 for i in range(count))


def apply_call(memory, state, event, length, n, touch):
    words = args(memory, state)
    if state.ip == 0x0F25:
        assert event == 0 and words == (0xA503,)
        memory[physical(state.ds, 0x7086)] = 0x12
        memory[physical(state.ds, 0x7046)] = 0x34
        state.ax = 0xB100
        state.flags = 0x0A53
    elif state.ip == 0x0F32:
        assert event == 1 and words == (0xB134, 0xB112)
        state.ax = 0x47B2
        state.flags = 0x0457
    elif state.ip == 0x0F4A:
        assert event == 2
        assert words == (0xFF, (state.bp - 0x102) & 0xFFFF,
                         state.ss, 0x70C8, state.ds)
        dst_off = words[1]
        memory[physical(state.ss, dst_off)] = length
        for i in range(length):
            memory[physical(state.ss, dst_off + 1 + i)] = memory[
                physical(state.ds, 0x70C9 + i)]
        state.dx = 0x5A5A
        state.flags = 0x0A53
    elif state.ip == 0x0F52:
        assert event == 3 and words == (0x0003,)
        state.ax = 0xA500 | n
        state.flags = 0x0457
    else:
        assert state.ip == 0x0F62 and event == 0
        assert words == (0xA500 | n,)
        if touch:
            memory[physical(state.ss, state.bp - 1)] = n ^ 0x55
        state.ax = 0xBD20
        state.flags = 0x0A53


def compare_case(label, memory, cpu, code, model, callback_type,
                 length, n, touch):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x0F08)
    stop = physical(cpu.cs, 0x0F73)
    expected_calls = []
    reached = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            event, cleanup, next_ip, _ = CALLS[off]
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, snap(state), args(snapshot, state)))
            apply_call(snapshot, state, event, length, n, touch)
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + cleanup) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, next_ip)
        elif address == stop:
            reached[0] = True
            emu.emu_stop()
        else:
            assert start <= address < stop, (label, hex(address))

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, RAM_SIZE, count=80)
    assert reached[0] and len(expected_calls) == 5, label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((event, snap(state), args(ram_pointer, state)))
        apply_call(ram_pointer, state, event, length, n, touch)
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
    code = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int)
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0f08-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_vertical_menu_prefix_0f08.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_vertical_menu_prefix_0f08
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(length, ds, ss, sp, pc, n, touch)
                 for length, pc, n in ((0, 0, 0), (1, 1, 1), (40, 0x8000, 5))
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x708A),
                                    (0xF123, 0xE000, 0x0002))
                 for touch in (False, True)]
        for label in cases:
            memory, cpu, length, n, touch = make_case(code, *label)
            compare_case(label, memory, cpu, code, model, callback_type,
                         length, n, touch)
    print(f"u1c:+0f08 bounded-prefix original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
