#!/usr/bin/env python3
"""Compare original GAME.OVR unit 0x77:+27DF with its C translation.

Original media and optional captured RAM stay under ignored resources/.
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
CAPTURE = (ROOT / "resources/oracle-runs/u9b-entry-zero-runtime-002"
           / "u9b-pre-0255-ram.bin")
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "461b9a087f9daa616a7bb52320c801cb4e7608f279f0b1105dca3c08746a9b1c"
CAPTURE_SHA = "0b50157ab03676d796b60f6ccda0b73fa17556a5775f49c91154f81faf0b8705"
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


def word(memory, segment, offset):
    address = physical(segment, offset)
    return memory[address] | (memory[(address + 1) & (RAM_SIZE - 1)] << 8)


def put_word(memory, segment, offset, value):
    address = physical(segment, offset)
    memory[address] = value & 0xFF
    memory[(address + 1) & (RAM_SIZE - 1)] = value >> 8


def put_far(memory, segment, offset, target):
    target_segment, target_offset = target
    put_word(memory, segment, offset, target_offset)
    put_word(memory, segment, (offset + 2) & 0xFFFF, target_segment)


def load_source():
    data = OVR.read_bytes()
    check(len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA,
          "GAME.OVR provenance mismatch")
    source = data[145433:145656]
    check(len(source) == 223 and sha256(source).hexdigest() == SPAN_SHA,
          "unit 0x77:+27DF source span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x27DF))
    check(len(decoded) == 71 and decoded[-1].address == 0x28BB and
          decoded[-1].mnemonic == "retf" and
          decoded[-1].op_str == "2" and
          decoded[-1].address + decoded[-1].size == 0x28BE,
          "unit 0x77:+27DF instruction extent changed")
    check(not any("call" in ins.mnemonic for ins in decoded),
          "unit 0x77:+27DF gained a call")
    starts = {ins.address for ins in decoded}
    for ins in decoded:
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in starts,
                  f"branch at {ins.address:#x} leaves the span")
    return source


def build_library(directory):
    library = directory / "u77_27df.so"
    command = ["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
               "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
               str(ROOT / "src/recovered/overlay_u77_active_select_27df.c"),
               "-o", str(library)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u77_active_select_27df.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u77_active_select_27df.restype = ctypes.c_int
    return result


def compare_case(label, memory, cpu, source, library):
    code_start = physical(cpu.cs, 0x27DF)
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
    original.emu_start(code_start, RAM_SIZE, count=40000)
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = library.pr_u77_active_select_27df(ram, RAM_SIZE, ctypes.byref(cpu))
    check(status == 0, f"C rejected case: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected))
                       if a != b]
        raise ValueError(f"RAM mismatch {label}: first {differences[:8]}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(register) & 0xFFFF
        candidate = getattr(cpu, name)
        mask = 0xFFEF if name == "flags" else 0xFFFF
        check(((candidate ^ observed) & mask) == 0,
              f"{name} mismatch {label}: C={candidate:04x} DOS={observed:04x}")


def make_case(source, key, nodes, active=None, *, sp=0x8000,
              first_offset=0x0100, captured=False):
    memory = bytearray(RAM_SIZE)
    cs, ss = 0xF000, 0xE000
    ds = 0x1000
    if captured:
        data = CAPTURE.read_bytes()
        check(len(data) == 655360 and sha256(data).hexdigest() == CAPTURE_SHA,
              "native RAM capture provenance mismatch")
        memory[:len(data)] = data
        ds = 0x14A0
        check((word(memory, ds, 0x5D96),
               word(memory, ds, 0x5D98)) == (0x0008, 0x449A),
              "captured party head changed")
    else:
        targets = [(0x4000 + index * 0x20, first_offset)
                   for index in range(nodes)]
        put_far(memory, ds, 0x5D96, targets[0] if targets else (0, 0))
        for index, target in enumerate(targets):
            segment, offset = target
            put_far(memory, segment, (offset + 0x104) & 0xFFFF,
                    targets[index + 1] if index + 1 < nodes else (0, 0))
        if active is None:
            active = targets[0] if targets else (0, 0)
        elif isinstance(active, int):
            active = targets[active]
        put_far(memory, ds, 0x5D92, active)
    code_start = physical(cs, 0x27DF)
    memory[code_start:code_start + len(source)] = source
    put_word(memory, ss, sp, 0x0712)
    put_word(memory, ss, (sp + 2) & 0xFFFF, cs)
    put_word(memory, ss, (sp + 4) & 0xFFFF, key)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x27DF, ds, 0x1111, ss, 0x7202)
    return memory, cpu


def main():
    source = load_source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u77-27df-") as tmp:
        library = build_library(Path(tmp))
        cases = [
            ("empty-G", ord("G"), 0, None, {}),
            ("empty-O", ord("O"), 0, None, {}),
            ("empty-other", ord("X"), 0, (0x4000, 0x0100), {}),
            ("single-G", ord("G"), 1, 0, {}),
            ("single-O", ord("O"), 1, 0, {}),
            ("single-other", ord("X"), 1, 0, {}),
            ("head-G", ord("G"), 3, 0, {}),
            ("head-O", ord("O"), 3, 0, {}),
            ("middle-G", ord("G"), 3, 1, {}),
            ("middle-O", ord("O"), 3, 1, {}),
            ("tail-G", ord("G"), 3, 2, {}),
            ("tail-O", ord("O"), 3, 2, {}),
            ("tail-other", ord("Q"), 3, 2, {}),
            ("absent-G", ord("G"), 3, (0x5000, 0x0200), {}),
            ("absent-O", ord("O"), 3, (0x5000, 0x0200), {}),
            ("head-same-segment-other-offset-G", ord("G"), 3,
             (0x4000, 0x0200), {}),
            ("next-same-segment-other-offset-G", ord("G"), 3,
             (0x4020, 0x0200), {}),
            ("high-byte-G", 0x1247, 3, 1, {}),
            ("offset-wrap-G", ord("G"), 2, 0,
             {"first_offset": 0xFF00}),
            ("stack-return-wrap", ord("G"), 1, 0, {"sp": 0xFFFF}),
            ("stack-frame-wrap", ord("O"), 1, 0, {"sp": 0x0005}),
            ("captured-G", ord("G"), 0, None, {"captured": True}),
            ("captured-O", ord("O"), 0, None, {"captured": True}),
        ]
        case_count = 0
        for label, key, nodes, active, options in cases:
            memory, cpu = make_case(source, key, nodes, active, **options)
            compare_case(label, memory, cpu, source, library)
            case_count += 1
        for label, target in (("segment-only-head", (0x4000, 0)),
                              ("offset-only-head", (0, 0x0100))):
            memory, cpu = make_case(source, ord("G"), 0)
            put_far(memory, cpu.ds, 0x5D96, target)
            put_far(memory, cpu.ds, 0x5D92, target)
            put_far(memory, target[0], (target[1] + 0x104) & 0xFFFF,
                    (0, 0))
            compare_case(label, memory, cpu, source, library)
            case_count += 1
        # An active record whose next link is the DS active-pointer table.
        memory, cpu = make_case(source, ord("O"), 0)
        target = (cpu.ds, 0x5C8E)
        put_far(memory, cpu.ds, 0x5D96, target)
        put_far(memory, cpu.ds, 0x5D92, target)
        compare_case("link-overlaps-active-global", memory, cpu,
                     source, library)
        case_count += 1
    print(f"unit 0x77:+27DF: 223 bytes, 71 instructions, {case_count} "
          "original-byte/C cases; full RAM and defined returned CPU state equal")


if __name__ == "__main__":
    main()
