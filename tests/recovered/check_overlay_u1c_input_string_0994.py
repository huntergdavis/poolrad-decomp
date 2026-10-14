#!/usr/bin/env python3
"""Original-byte/full-state parity for U1C:+0994 under explicit callees."""

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
SPAN_SHA = "cf8bd60da6a03799c2143ea2d71eb99dcf3a9ad994b31c2a5db01a771fb42ac9"
RAM_SIZE = 1 << 20
CALLS = {0x099E: (0, 2, 0x09A3, 1), 0x09AF: (1, 4, 0x09B4, 2),
         0x09C8: (2, 8, 0x09CD, 6), 0x09D7: (3, 10, 0x09DC, 5),
         0x09F2: (3, 10, 0x09F7, 5), 0x0A00: (4, 6, 0x0A05, 3)}
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
    body = data[0x16DA:0x174F]
    assert len(body) == 117 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x0994))
    assert len(ins) == 49 and sum(x.size for x in ins) == 117
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x0A08 and ins[-1].mnemonic == "retf"
    assert ins[-1].address + ins[-1].size == 0x0A09
    assert data[0x16D8:0x16DA] == bytes.fromhex("0120")
    assert data[0x174F:0x1752] == bytes.fromhex("5589e5")
    return body


def make_case(body, length, ds, ss, sp, touch):
    memory = bytearray(RAM_SIZE)
    cs = 0xF000
    start = physical(cs, 0x0994)
    memory[start - 2:start] = b"\x01 "
    memory[start:start + len(body)] = body
    put16(memory, ss, sp, 0x0712)
    put16(memory, ss, sp + 2, 0xD000)
    cpu = Cpu(0xA5CD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, sp, cs, 0x0994, ds, 0x1111,
              ss, 0x72D3)
    return memory, cpu, length, touch


def snap(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state):
    count = CALLS[state.ip][3]
    return tuple(get16(memory, state.ss, state.sp + i * 2)
                 for i in range(count))


def apply_call(memory, state, event, length, touch):
    words = args(memory, state)
    if event == 0:
        assert words == (0xA502,)
        memory[physical(state.ds, 0x7087)] = 0x12
        memory[physical(state.ds, 0x7047)] = 0x34
        state.ax = 0xB100
        state.flags = 0x0A53
    elif event == 1:
        assert words[0] == 0xB134 and words[1] in (0xB112, 0xB100)
        state.ax = 0x3412
        state.flags = 0x0457
    elif event == 2:
        limit, count, choice_off, choice_seg, out_off, out_seg = words
        assert (limit & 255, count & 255) == (40, 10)
        assert (choice_off, choice_seg) == ((state.bp - 2) & 0xFFFF, state.ss)
        assert (out_off, out_seg) == ((state.bp - 0x204) & 0xFFFF, state.ss)
        memory[physical(choice_seg, choice_off)] = 0x7E
        memory[physical(out_seg, out_off)] = length
        for i in range(length):
            memory[physical(out_seg, out_off + 1 + i)] = (65 + i % 26)
        state.ax = 0xCDEF
        state.flags = 0x0A53
    elif event == 3:
        cap, dest_off, dest_seg, src_off, src_seg = words
        assert cap == 0xFF
        assert (dest_off, dest_seg) == ((state.bp - 0x104) & 0xFFFF, state.ss)
        if state.ip == 0x09F2:
            assert (src_off, src_seg) == (0x0992, state.cs)
        else:
            assert (src_off, src_seg) == ((state.bp - 0x204) & 0xFFFF, state.ss)
        n = min(memory[physical(src_seg, src_off)], cap)
        memory[physical(dest_seg, dest_off)] = n
        for i in range(n):
            memory[physical(dest_seg, dest_off + 1 + i)] = memory[
                physical(src_seg, src_off + 1 + i)]
        state.dx = 0x5A5A
        state.flags = 0x0457
    else:
        string_off, string_seg, address = words
        assert (string_off, string_seg, address) == (
            (state.bp - 0x104) & 0xFFFF, state.ss, 0x3412)
        put16(memory, state.ds, 0x8460,
              memory[physical(string_seg, string_off)] ^ address)
        state.dx = 0x7575
        state.flags = 0x0A53
        if touch:
            put16(memory, state.ss, state.sp + 0x20A, 0x3344)
            put16(memory, state.ss, state.sp + 0x20C, 0x7788)
            put16(memory, state.ss, state.sp + 0x20E, 0x99AA)


def compare_case(label, memory, cpu, body, model, callback_type, length, touch):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x0994)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            event, cleanup, next_ip, _ = CALLS[off]
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, snap(state), args(snapshot, state)))
            apply_call(snapshot, state, event, length, touch)
            emu.mem_write(0, bytes(snapshot))
            for (name, _), reg in zip(Cpu._fields_, REGISTERS):
                emu.reg_write(reg, getattr(state, name))
            emu.reg_write(UC_X86_REG_SP, (state.sp + cleanup) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, next_ip)
        elif not start <= address < start + len(body):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(start, RAM_SIZE, count=100)
    assert returned[0] and len(expected_calls) == (6 if length == 0 else 5), label
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((event, snap(state), args(ram_pointer, state)))
        apply_call(ram_pointer, state, event, length, touch)
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-0994-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_input_string_0994.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_input_string_0994
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(length, ds, ss, sp, touch)
                 for length in (0, 1, 40)
                 for ds, ss, sp in ((0x3000, 0xE000, 0x8000),
                                    (0x3000, 0x3000, 0x708B),
                                    (0xF123, 0xE000, 0x0002))
                 for touch in (False, True)]
        for label in cases:
            memory, cpu, n, touch = make_case(body, *label)
            compare_case(label, memory, cpu, body, model, callback_type,
                         n, touch)
    print(f"u1c:+0994 full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
