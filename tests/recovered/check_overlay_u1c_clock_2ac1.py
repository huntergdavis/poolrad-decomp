#!/usr/bin/env python3
"""Full-state original-byte audit of U1C:+2AC1 with three bounded calls."""

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
SPAN_SHA = "94a44f80f35c4b673eee1a45fe0869e383400087e6659a952b223c3df6d43420"
RAM_SIZE = 1 << 20
CALLS = {0x2ACA: (0, 2, 0x2ACF), 0x2AD3: (1, 2, 0x2AD8),
         0x2AE2: (2, 4, 0x2AE7)}
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
    body = data[0x3807:0x3831]
    assert len(body) == 42 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x2AC1))
    assert len(ins) == 18 and sum(x.size for x in ins) == 42
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    assert ins[-1].address == 0x2AEA and ins[-1].mnemonic == "retf"
    assert ins[-1].address + ins[-1].size == 0x2AEB
    return body


def make_case(body, local, ah, slot_result, loader_writes):
    memory = bytearray(RAM_SIZE)
    cs, ds, ss = 0xF000, 0x3000, 0xE000
    start = physical(cs, 0x2AC1)
    memory[start:start + len(body)] = body
    memory[physical(ss, 0x7FFD)] = local
    memory[physical(ss, 0x7FFC)] = 0x77
    put16(memory, ss, 0x8000, 0x0712)
    put16(memory, ss, 0x8002, 0xD000)
    cpu = Cpu((ah << 8) | 0xCD, 0x3456, 0x5678, 0x789A, 0x9ABC,
              0xBCDE, 0x7777, 0x8000, cs, 0x2AC1, ds, 0x1111,
              ss, 0x72D3)
    return memory, cpu, slot_result, loader_writes


def state_tuple(state):
    return tuple(getattr(state, name) for name, _ in Cpu._fields_)


def args(memory, state, event):
    return tuple(get16(memory, state.ss, state.sp + i * 2)
                 for i in range(1 if event < 2 else 2))


def compare_case(label, memory, cpu, body, model, callback_type,
                 slot_result, loader_writes):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    start = physical(cpu.cs, 0x2AC1)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - 0xF0000
        if off in CALLS:
            event, cleanup, next_ip = CALLS[off]
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            expected_calls.append((event, state_tuple(state), args(snapshot, state, event)))
            if event == 0 and loader_writes:
                emu.mem_write(physical(state.ss, state.bp - 1), b"\xA7")
                emu.reg_write(UC_X86_REG_AX, 0xB100)
            elif event == 1:
                emu.reg_write(UC_X86_REG_AX, slot_result)
                emu.reg_write(UC_X86_REG_EFLAGS, 0xA53)
            elif event == 2:
                emu.mem_write(physical(state.ds, 0x8460), b"\x73")
                emu.reg_write(UC_X86_REG_DX, 0x5A5A)
            emu.reg_write(UC_X86_REG_SP, (state.sp + cleanup) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, next_ip)
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
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        actual_calls.append((event, state_tuple(state), args(ram_pointer, state, event)))
        if event == 0 and loader_writes:
            ram_pointer[physical(state.ss, state.bp - 1)] = 0xA7
            state.ax = 0xB100
        elif event == 1:
            state.ax = slot_result
            state.flags = 0xA53
        elif event == 2:
            ram_pointer[physical(state.ds, 0x8460)] = 0x73
            state.dx = 0x5A5A
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
    with tempfile.TemporaryDirectory(prefix="poolrad-u1c-2ac1-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u1c_clock_2ac1.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u1c_clock_2ac1
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(local, ah, slot_result, writes)
                 for local in (0, 1, 0x80, 0xFF)
                 for ah in (0, 0xA5)
                 for slot_result, writes in ((0x0023, False), (0xB1C7, True))]
        for label in cases:
            memory, cpu, result, writes = make_case(body, *label)
            compare_case(label, memory, cpu, body, model, callback_type,
                         result, writes)
    print(f"u1c:+2AC1 full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
