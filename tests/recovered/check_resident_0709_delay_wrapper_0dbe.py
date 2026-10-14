#!/usr/bin/env python3
"""Original-byte full-state differential for RES_0709:+0DBE.

Uses a synthetic A40 callee that sets defined registers/flags and RETF 2.
Original game bytes and the private RAM snapshot stay under resources/.
"""

import ctypes as c
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
START = ROOT / "resources/DOS/POOLRAD/START.EXE"
RAM = (ROOT / "resources/oracle-runs/slot-a-u1c-print-rect-p9-096/"
       "preprint-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "107f1f0b4dbcecffc8fed3744b4d7f55b26e65186a2628401d586e354a19ab32"
ORIGINAL_SHA = "a7b0396a671f47e30f7123716ac21bce6271d5508b6f4acbe9e7fad20aae861d"
LOADED_SHA = "7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be"
RAM_SIZE = 1 << 20
WRAPPER_CS, WRAPPER_IP = 0x0F2D, 0x0DBE
A40_CS, A40_IP = 0x1264, 0x029C
REGS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
        UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
        UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
        UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)

# Newly written synthetic callee, never copied from game media. Its final
# PUSH/POPF fixes all flags so the returned 16-bit CPU state is comparable.
STUB = bytes.fromhex(
    "b8 89 67 8e c0 b8 5a 5a bb 34 12 b9 45 23 ba 56 34 "
    "be 67 45 bf 78 56 bd ef be 68 46 02 9d ca 02 00")


class Cpu(c.Structure):
    _fields_ = [(name, c.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


class Report(c.Structure):
    _fields_ = [("argument", c.c_uint16),
                ("callee_frame_sp", c.c_uint16),
                ("callback_invoked", c.c_uint),
                ("callback_status", c.c_int)]


def physical(segment, offset):
    return ((segment << 4) + (offset & 0xFFFF)) & (RAM_SIZE - 1)


def word(memory, segment, offset):
    at = physical(segment, offset)
    return memory[at] | (memory[(at + 1) & (RAM_SIZE - 1)] << 8)


def set_word(memory, segment, offset, value):
    at = physical(segment, offset)
    memory[at] = value & 255
    memory[(at + 1) & (RAM_SIZE - 1)] = value >> 8


def pinned_memory():
    start = START.read_bytes()
    saved = RAM.read_bytes()
    assert sha256(start).hexdigest() == START_SHA
    assert len(saved) == 640 * 1024 and sha256(saved).hexdigest() == RAM_SHA
    import sys
    sys.path.insert(0, str(ROOT / "tools/overlay"))
    from resident_loader_map import unpack
    image, *_ = unpack(start)
    original = bytes(image[0x7E4E:0x7E63])
    assert len(original) == 21 and sha256(original).hexdigest() == ORIGINAL_SHA
    loaded = saved[physical(WRAPPER_CS, WRAPPER_IP):
                   physical(WRAPPER_CS, WRAPPER_IP) + 21]
    assert sha256(loaded).hexdigest() == LOADED_SHA
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(loaded, WRAPPER_IP))
    assert len(insns) == 10
    assert insns[7].address == 0x0DCC and insns[7].mnemonic == "lcall"
    assert insns[-1].address == 0x0DD2 and insns[-1].mnemonic == "retf"
    assert insns[-1].address + insns[-1].size == 0x0DD3
    return saved


def make_case(saved, factor, sp, flags):
    memory = bytearray(saved + bytes([0xA5]) * (RAM_SIZE - len(saved)))
    memory[physical(A40_CS, A40_IP):physical(A40_CS, A40_IP) + len(STUB)] = STUB
    memory[physical(0x14A0, 0x49E2)] = factor
    set_word(memory, 0xE000, sp, 0x7135)
    set_word(memory, 0xE000, (sp + 2) & 0xFFFF, 0x9000)
    cpu = Cpu(0xABCD, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666,
              0x7777, sp, WRAPPER_CS, WRAPPER_IP, 0x14A0,
              0x8888, 0xE000, flags)
    return memory, cpu


def compare_case(saved, model, callback_type, factor, sp, flags):
    label = (factor, sp, flags)
    memory, cpu = make_case(saved, factor, sp, flags)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGS):
        original.reg_write(register, getattr(cpu, name))
    entry = physical(WRAPPER_CS, WRAPPER_IP)
    exit_address = physical(0x9000, 0x7135)
    call_states = []

    def on_code(emu, address, size, user):
        del size, user
        if address == physical(A40_CS, A40_IP):
            state = Cpu(*(emu.reg_read(reg) & 0xFFFF for reg in REGS))
            frame = tuple(word(memory_view(emu), state.ss,
                               (state.sp + 2 * i) & 0xFFFF) for i in range(3))
            call_states.append((state, frame))

    def memory_view(emu):
        return emu.mem_read(0, RAM_SIZE)

    original.hook_add(UC_HOOK_CODE, on_code)
    original.emu_start(entry, exit_address, count=60)
    assert len(call_states) == 1, (label, "A40 calls", len(call_states))
    expected_ram = bytes(original.mem_read(0, RAM_SIZE))
    actual_ram = (c.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    observed_calls = []

    @callback_type
    def callback(ctx, ram_pointer, size, state_pointer, argument):
        del ctx
        assert size == RAM_SIZE
        state = state_pointer.contents
        observed_calls.append((Cpu(*(getattr(state, name)
                                     for name, _ in Cpu._fields_)), argument))
        at = physical(state.ss, (state.sp - 2) & 0xFFFF)
        ram_pointer[at] = 0x46
        ram_pointer[(at + 1) & (RAM_SIZE - 1)] = 0x02
        state.ax, state.bx, state.cx, state.dx = 0x5A5A, 0x1234, 0x2345, 0x3456
        state.si, state.di, state.bp, state.es = 0x4567, 0x5678, 0xBEEF, 0x6789
        state.flags = 0x0246
        return 0

    report = Report()
    status = model(actual_ram, RAM_SIZE, c.byref(cpu), A40_CS,
                   callback, None, c.byref(report))
    assert status == 0, (label, status)
    assert report.argument == factor * 225 and report.callback_invoked == 1
    assert len(observed_calls) == 1
    native_call, native_frame = call_states[0]
    c_call, c_argument = observed_calls[0]
    assert c_argument == factor * 225
    assert native_frame == (0x0DD1, WRAPPER_CS, c_argument), label
    for name, _ in Cpu._fields_:
        if name == "flags":
            assert (c_call.flags & 0x0801) == (native_call.flags & 0x0801), label
        else:
            assert getattr(c_call, name) == getattr(native_call, name), \
                   (label, "A40 entry", name)
    for (name, _), reg in zip(Cpu._fields_, REGS):
        assert getattr(cpu, name) == (original.reg_read(reg) & 0xFFFF), \
               (label, "return CPU", name, getattr(cpu, name),
                original.reg_read(reg) & 0xFFFF)
    if bytes(actual_ram) != expected_ram:
        differences = [i for i, (a, b) in
                       enumerate(zip(actual_ram, expected_ram)) if a != b]
        raise AssertionError((label, "full RAM", differences[:12]))


def main():
    saved = pinned_memory()
    callback_type = c.CFUNCTYPE(c.c_int, c.c_void_p,
                               c.POINTER(c.c_uint8), c.c_size_t,
                               c.POINTER(Cpu), c.c_uint16)
    with tempfile.TemporaryDirectory(prefix="poolrad-r0709-delay-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I",
                        str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_0709_delay_wrapper_0dbe.c"),
                        "-o", str(library)], check=True)
        model = c.CDLL(str(library)).pr_r0709_delay_wrapper_0dbe
        model.argtypes = [c.POINTER(c.c_uint8), c.c_size_t, c.POINTER(Cpu),
                          c.c_uint16, callback_type, c.c_void_p,
                          c.POINTER(Report)]
        model.restype = c.c_int
        cases = [(factor, sp, flags)
                 for factor in (0, 1, 4, 255)
                 for sp in (0x8000, 0x0002)
                 for flags in (0x7202, 0x7AD3)]
        for case in cases:
            compare_case(saved, model, callback_type, *case)
    print(f"RES_0709:+0DBE original-byte/C full-state parity: {len(cases)} "
          "cases, 14 returned CPU fields, 1 MiB RAM")


if __name__ == "__main__":
    main()
