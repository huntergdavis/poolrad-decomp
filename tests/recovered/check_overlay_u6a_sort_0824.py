#!/usr/bin/env python3
"""Compare unit 0x6A:+0824 original bytes with its complete C translation.

Original media stays under ignored resources/; only hashes and results print.
"""

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import (
    UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
    UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
    UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
    UC_X86_REG_SS, UC_X86_REG_EFLAGS,
)

ROOT = Path(__file__).resolve().parents[2]
OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7"
RAM_SIZE = 1 << 20
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
             UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
             UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
             UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def physical(segment, offset):
    return ((segment << 4) + offset) & (RAM_SIZE - 1)


def put_word(memory, segment, offset, value):
    address = physical(segment, offset)
    memory[address] = value & 0xFF
    memory[(address + 1) & (RAM_SIZE - 1)] = value >> 8


def load_source():
    data = OVR.read_bytes()
    check(len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA,
          "GAME.OVR provenance mismatch")
    source = data[0x18055:0x180F0]
    check(len(source) == 155 and sha256(source).hexdigest() == SPAN_SHA,
          "unit 0x6A:+0824 source span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x0824))
    check(len(decoded) == 57 and decoded[-1].address == 0x08BE and
          decoded[-1].mnemonic == "retf" and
          decoded[-1].address + decoded[-1].size == 0x08BF,
          "unit 0x6A:+0824 instruction extent changed")
    check(not any("call" in ins.mnemonic for ins in decoded),
          "unit 0x6A:+0824 gained a call")
    starts = {ins.address for ins in decoded}
    for ins in decoded:
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in starts,
                  f"branch at {ins.address:#x} leaves the body")
    return source


def build_library(directory):
    library = directory / "u6a_0824.so"
    command = ["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
               "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
               str(ROOT / "src/recovered/overlay_u6a_sort_0824.c"),
               "-o", str(library)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u6a_sort_0824.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u6a_sort_0824.restype = ctypes.c_int
    return result


def compare_case(label, memory, cpu, source, library):
    code_start = physical(cpu.cs, 0x0824)
    code_end = code_start + len(source)
    check(memory[code_start:code_end] == source, f"code changed: {label}")
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    returned = [False]

    def on_instruction(emulator, address, size, user):
        del size, user
        if not code_start <= address < code_end:
            returned[0] = True
            emulator.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(code_start, RAM_SIZE, count=100000)
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = library.pr_u6a_sort_0824(ram, RAM_SIZE, ctypes.byref(cpu))
    check(status == 0, f"C rejected case: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected))
                       if a != b]
        raise ValueError(f"RAM mismatch {label}: first {differences[:8]}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(register) & 0xFFFF
        candidate = getattr(cpu, name)
        check(candidate == observed,
              f"{name} mismatch {label}: C={candidate:04x} DOS={observed:04x}")


def make_case(source, values, *, sp=0x8000, offset=0x0100,
              segment=0x4000, ds=0x1000, ss=0xE000):
    memory = bytearray(RAM_SIZE)
    cs = 0xF000
    start = physical(cs, 0x0824)
    memory[start:start + len(source)] = source
    put_word(memory, ds, 0x5D92, offset)
    put_word(memory, ds, 0x5D94, segment)
    for index, value in enumerate(values):
        memory[physical(segment, (offset + index + 0x17) & 0xFFFF)] = value
    put_word(memory, ss, sp, 0x0712)
    put_word(memory, ss, (sp + 2) & 0xFFFF, 0xF000)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0824, ds, 0x1111, ss, 0x7202)
    return memory, cpu


def main():
    source = load_source()
    patterns = [
        ("ascending", list(range(21))),
        ("descending", list(range(20, -1, -1))),
        ("alternating", [20 - i if i % 2 == 0 else i for i in range(21)]),
        ("all-equal", [0x77] * 21),
        ("high-bits", [0x80 | i for i in range(20, -1, -1)]),
        ("masked-ties", [0x80 | (i % 4) if i % 2 else i % 4
                          for i in range(21)]),
        ("zeros", [0] * 21),
        ("one-large", [0x7F] + [0] * 20),
        ("one-small", [0x7F] * 20 + [0]),
        ("mixed", [0xFF, 0x00, 0x80, 0x7F, 0x81, 0x01, 0x42] * 3),
    ]
    with tempfile.TemporaryDirectory(prefix="poolrad-u6a-0824-") as tmp:
        library = build_library(Path(tmp))
        cases = 0
        for label, values in patterns:
            memory, cpu = make_case(source, values)
            compare_case(label, memory, cpu, source, library)
            cases += 1
        for label, options in (
            ("stack-return-wrap", {"sp": 0xFFFF}),
            ("stack-local-wrap", {"sp": 0x0005}),
            ("array-offset-wrap", {"offset": 0xFFF0}),
            ("array-high-segment-wrap", {"segment": 0xE000,
                                          "offset": 0xFFF0}),
            ("ds-ss-same", {"ds": 0xE000}),
        ):
            memory, cpu = make_case(source, list(range(20, -1, -1)),
                                    **options)
            compare_case(label, memory, cpu, source, library)
            cases += 1
        # The first four list bytes are the live far pointer itself. Swaps
        # can change later LES results, so this checks source-order reads.
        values = [0x7B, 0x5D, 0x00, 0x10] + list(range(17))
        memory, cpu = make_case(source, values, offset=0x5D7B,
                                segment=0x1000)
        compare_case("list-overlaps-pointer-table", memory, cpu,
                     source, library)
        cases += 1
    print(f"unit 0x6A:+0824: 155 bytes, 57 instructions, {cases} "
          "original-byte/C cases; full RAM and returned CPU state equal")


if __name__ == "__main__":
    main()
