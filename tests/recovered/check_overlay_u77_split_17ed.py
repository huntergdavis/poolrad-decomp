#!/usr/bin/env python3
"""Full in-body state parity for GAME.OVR u77:+17ED with bounded calls."""

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
SPAN_SHA = "46c07cbe1120a95c2830f87199a1f5141a272b9dfba7df3f534e4168079e82e9"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)
CALLS = {0x1820: (0, 2), 0x1839: (1, 10),
         0x188D: (2, 8), 0x1892: (3, 0)}


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + (off & 0xFFFF)) & (RAM_SIZE - 1)


def put16(memory, seg, off, value):
    a = physical(seg, off)
    memory[a] = value & 255
    memory[(a + 1) & (RAM_SIZE - 1)] = value >> 8 & 255


def read16(memory, seg, off):
    a = physical(seg, off)
    return memory[a] | memory[(a + 1) & (RAM_SIZE - 1)] << 8


def source():
    data = OVR.read_bytes()
    assert len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA
    body = data[141351:141527]
    assert len(body) == 176 and sha256(body).hexdigest() == SPAN_SHA
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x17ED))
    assert len(ins) == 63 and ins[-1].address == 0x189A
    assert ins[-1].mnemonic == "retf" and ins[-1].op_str == "4"
    assert ins[-1].address + ins[-1].size == 0x189D
    assert {x.address for x in ins if x.mnemonic == "lcall"} == set(CALLS)
    starts = {x.address for x in ins}
    assert all(int(x.op_str, 16) in starts for x in ins
               if x.mnemonic.startswith("j"))
    return body


def make_case(body, count, *, source_off=0x0100,
              next_pointer=(0x0300, 0x6000), alloc=(0x0200, 0x5000)):
    memory = bytearray(RAM_SIZE)
    cs, ss = 0xF000, 0xE000
    source_ptr = (source_off, 0x4000)
    code_start = physical(cs, 0x17ED)
    memory[code_start:code_start + len(body)] = body
    for i in range(63):
        memory[physical(source_ptr[1], source_ptr[0] + i)] = (i * 19 + 7) & 255
    memory[physical(source_ptr[1], source_ptr[0] + 0x39)] = count
    put16(memory, source_ptr[1], source_ptr[0] + 0x2A, next_pointer[0])
    put16(memory, source_ptr[1], source_ptr[0] + 0x2C, next_pointer[1])
    put16(memory, ss, 0x8000, 0x0712)
    put16(memory, ss, 0x8002, cs)
    put16(memory, ss, 0x8004, source_ptr[0])
    put16(memory, ss, 0x8006, source_ptr[1])
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, 0x8000, cs, 0x17ED, 0x1000, 0x1111, ss, 0x7202)
    return memory, cpu, alloc


def call_args(memory, cpu, event):
    sp = cpu.sp
    words = [read16(memory, cpu.ss, sp + 2 * i)
             for i in range((1, 5, 4, 0)[event])]
    return (event, *words)


def apply_copy(memory, args):
    _, size, dest_off, dest_seg, src_off, src_seg = args
    block = [memory[physical(src_seg, src_off + i)] for i in range(size)]
    for i, value in enumerate(block):
        memory[physical(dest_seg, dest_off + i)] = value


def compare_case(label, memory, cpu, alloc, body, model, callback_type):
    cs = cpu.cs
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    code_start = physical(cs, 0x17ED)
    expected_calls = []
    returned = [False]

    def on_code(emu, address, size, user):
        del size, user
        off = address - cs * 16
        if off in CALLS:
            event, cleanup = CALLS[off]
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGISTERS))
            snapshot = bytearray(emu.mem_read(0, RAM_SIZE))
            args = call_args(snapshot, state, event)
            expected_calls.append(args)
            if event == 0:
                emu.reg_write(UC_X86_REG_AX, alloc[0])
                emu.reg_write(UC_X86_REG_DX, alloc[1])
            elif event == 1:
                apply_copy(snapshot, args)
                _, length, dst_off, dst_seg, _, _ = args
                for i in range(length):
                    a = physical(dst_seg, dst_off + i)
                    emu.mem_write(a, bytes((snapshot[a],)))
            emu.reg_write(UC_X86_REG_SP, (state.sp + cleanup) & 0xFFFF)
            emu.reg_write(UC_X86_REG_IP, off + 5)
        elif not code_start <= address < code_start + len(body):
            returned[0] = True
            emu.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(code_start, RAM_SIZE, count=1000)
    assert returned[0], f"original did not return: {label}"
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    actual_calls = []

    @callback_type
    def external(context, ram_pointer, ram_size, state_pointer, event):
        del context
        assert ram_size == RAM_SIZE
        state = state_pointer.contents
        args = call_args(ram_pointer, state, event)
        actual_calls.append(args)
        if event == 0:
            state.ax, state.dx = alloc
        elif event == 1:
            apply_copy(ram_pointer, args)
        return 1

    status = model(ram, RAM_SIZE, ctypes.byref(cpu), external, None)
    assert status == 0, (label, status)
    actual_ram = bytes(ram)
    if actual_ram != expected_ram:
        diff = [i for i, (a, b) in enumerate(zip(actual_ram, expected_ram))
                if a != b]
        raise AssertionError((label, "RAM", diff[:12],
                              [(i, actual_ram[i], expected_ram[i])
                               for i in diff[:12]]))
    assert actual_calls == expected_calls, (label, actual_calls, expected_calls)
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(reg) & 0xFFFF
        actual = getattr(cpu, name)
        assert actual == observed, (label, name, hex(actual), hex(observed))


def main():
    body = source()
    callback_type = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8),
        ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_int)
    with tempfile.TemporaryDirectory(prefix="poolrad-u77-17ed-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u77_split_17ed.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u77_split_17ed
        model.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p]
        model.restype = ctypes.c_int
        cases = [(f"count-{count}", dict(count=count))
                 for count in (0, 1, 2, 3, 4, 5, 127, 128, 254, 255)]
        cases += [
            ("source-offset-wrap", dict(count=5, source_off=0xFFE0)),
            ("one-word-zero-next", dict(count=4, next_pointer=(0, 0x6000))),
            ("zero-segment-allocation", dict(count=8, alloc=(0x0200, 0))),
        ]
        for label, options in cases:
            memory, cpu, alloc = make_case(body, **options)
            compare_case(label, memory, cpu, alloc, body, model, callback_type)
    print(f"u77:+17ED full-state original-byte parity: {len(cases)} cases")


if __name__ == "__main__":
    main()
