#!/usr/bin/env python3
"""Compare unit 0x25:+0756 C with the original DOS bytes in Unicorn.

Original media and optional captured RAM are read from ignored resources.
They are never copied into a tracked output.
"""

import argparse
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
SPAN_SHA = "b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9"
CAPTURE_SHA = "0b50157ab03676d796b60f6ccda0b73fa17556a5775f49c91154f81faf0b8705"
NATIVE_ENTRY_SHA = "db63f4b0a0f45faa2aac4148ace9b7597b55dc9e436569436fc3bc7e833d818f"
NATIVE_RETURN_SHA = "a53e797d7d052a7a07e32025557c5aa28d1f958a1a3ec2dce1660a8259130bac"
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


def load_source():
    data = OVR.read_bytes()
    check(len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA,
          "GAME.OVR provenance mismatch")
    source = data[0x7C29:0x7C8F]
    check(len(source) == 102 and sha256(source).hexdigest() == SPAN_SHA,
          "unit 0x25:+0756 source span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x0756))
    check(len(decoded) == 35 and decoded[-1].address == 0x07B9 and
          decoded[-1].mnemonic == "retf" and
          decoded[-1].address + decoded[-1].size == 0x07BC,
          "unit 0x25:+0756 instruction extent changed")
    check(not any("call" in ins.mnemonic for ins in decoded),
          "unit 0x25:+0756 gained a call")
    sites = {ins.address for ins in decoded}
    for ins in decoded:
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in sites,
                  f"branch at {ins.address:#x} leaves the span")
    return source


def build_library(directory):
    library = directory / "u25_0756.so"
    command = ["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
               "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
               str(ROOT / "src/recovered/overlay_u25_party_index_0756.c"),
               "-o", str(library)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u25_party_index_0756.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u25_party_index_0756.restype = ctypes.c_int
    return result


def compare_case(label, memory, cpu, source, library):
    code_start = physical(cpu.cs, 0x0756)
    code_end = code_start + len(source)
    found_address = physical(cpu.ss, (cpu.sp - 9) & 0xFFFF)
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
    original.emu_start(code_start, RAM_SIZE, count=20000)
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = library.pr_u25_party_index_0756(ram, RAM_SIZE, ctypes.byref(cpu))
    check(status == 0, f"C rejected case: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected))
                       if a != b]
        raise ValueError(f"RAM mismatch {label}: first {differences[:8]}")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        observed = original.reg_read(register) & 0xFFFF
        candidate = getattr(cpu, name)
        if name == "flags":
            # Full FLAGS on both paths: the null path ends in OR, which clears
            # AF in pinned DOSBox-X and in Unicorn alike, and the model now does too.
            check(candidate == observed,
                  f"flags mismatch {label}: C={candidate:04x} DOS={observed:04x}")
        else:
            check(candidate == observed,
                  f"{name} mismatch {label}: C={candidate:04x} DOS={observed:04x}")


def make_case(source, length, target, captured=False, sp=0x8000):
    memory = bytearray(RAM_SIZE)
    ds = 0x1000
    if captured:
        data = CAPTURE.read_bytes()
        check(len(data) == 655360 and sha256(data).hexdigest() == CAPTURE_SHA,
              "native RAM capture provenance mismatch")
        memory[:len(data)] = data
        ds = 0x14A0
        head = (word(memory, ds, 0x5D98), word(memory, ds, 0x5D96))
        check(head == (0x449A, 0x0008), "captured party head changed")
        check((word(memory, 0x449A, 0x010E),
               word(memory, 0x449A, 0x010C)) == (0, 0),
              "captured party chain changed")
        argument = (0x449A, 0x0008) if target == 0 else (0x5000, 0x0123)
    else:
        head = (0x4000, 0x0100) if length else (0, 0)
        put_word(memory, ds, 0x5D96, head[1])
        put_word(memory, ds, 0x5D98, head[0])
        for i in range(length):
            segment = 0x4000 + i * 0x20
            next_segment = segment + 0x20 if i + 1 < length else 0
            put_word(memory, segment, 0x0204, 0x0100 if next_segment else 0)
            put_word(memory, segment, 0x0206, next_segment)
        argument = ((0x4000 + target * 0x20, 0x0100)
                    if target is not None else (0x7000, 0x0300))
    cs, ss = 0xF000, 0xE000
    memory[physical(cs, 0x0756):physical(cs, 0x0756) + len(source)] = source
    for displacement, value in ((0, 0x7000), (2, 0x1234),
                                (4, argument[1]), (6, argument[0])):
        put_word(memory, ss, (sp + displacement) & 0xFFFF, value)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0756, ds, 0x1111, ss, 0x7202)
    return memory, cpu


def audit_native(run, source, library):
    entry = (run / "u25-0756-entry-ram.bin").read_bytes()
    returned = (run / "u25-0756-return-ram.bin").read_bytes()
    check(len(entry) == len(returned) == 655360 and
          sha256(entry).hexdigest() == NATIVE_ENTRY_SHA and
          sha256(returned).hexdigest() == NATIVE_RETURN_SHA,
          "reached native RAM captures changed")
    check(entry[physical(0x30CF, 0x0756):
                physical(0x30CF, 0x0756) + len(source)] == source,
          "reached native code differs from original source")
    check((word(entry, 0x1D16, 0x3CA6),
           word(entry, 0x1D16, 0x3CA8),
           word(entry, 0x1D16, 0x3CAA),
           word(entry, 0x1D16, 0x3CAC)) ==
          (0x0984, 0x30CF, 0x0008, 0x449A),
          "reached far-call frame changed")
    check((word(entry, 0x14A0, 0x5D96),
           word(entry, 0x14A0, 0x5D98)) == (0x0008, 0x449A),
          "reached party head changed")

    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(
        entry + bytes(RAM_SIZE - len(entry)))
    cpu = Cpu(0x02B1, 0x0082, 0x0008, 0x00B1, 0x3E47, 0x3CC0,
              0x3CB2, 0x3CA6, 0x30CF, 0x0756, 0x14A0, 0x1D16,
              0x1D16, 0x7246)
    status = library.pr_u25_party_index_0756(
        ram, RAM_SIZE, ctypes.byref(cpu))
    check(status == 0 and bytes(ram[:len(returned)]) == returned,
          "reached native/C RAM mismatch")
    expected = (0x4400, 0x0082, 0x0008, 0x449A, 0x3E47, 0x3CC0,
                0x3CB2, 0x3CAE, 0x30CF, 0x0984, 0x14A0, 0x1D16,
                0x1D16, 0x7202)
    actual = tuple(getattr(cpu, name) for name, _ in Cpu._fields_)
    check(actual == expected, f"reached native/C CPU mismatch: {actual!r}")
    print("reached native +0756: 640 KiB RAM and all 14 returned CPU "
          "fields equal")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native-run", type=Path,
                        help="ignored DOSBox-X run with +0756 entry/return RAM")
    args = parser.parse_args()
    source = load_source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u25-0756-") as tmp:
        library = build_library(Path(tmp))
        cases = [("empty", 0, None), ("head", 3, 0),
                 ("middle", 3, 1), ("tail", 3, 2),
                 ("missing", 3, None), ("count-wrap", 257, None)]
        for label, length, target in cases:
            memory, cpu = make_case(source, length, target)
            compare_case(label, memory, cpu, source, library)
        for label, sp in (("argument-offset-wrap", 0xFFFB),
                          ("return-offset-wrap", 0xFFFF)):
            memory, cpu = make_case(source, 0, None, sp=sp)
            compare_case(label, memory, cpu, source, library)
        memory, cpu = make_case(source, 1, 0, sp=0xFFFB)
        compare_case("argument-compare-offset-wrap", memory, cpu,
                     source, library)
        memory, cpu = make_case(source, 1, None, sp=0x0007)
        compare_case("LES-local-offset-wrap", memory, cpu,
                     source, library)
        # Entry AF=1 on the null (OR) path: pinned DOSBox-X and Unicorn both clear AF after OR,
        # so a model that keeps the incoming AF fails here (census #1464, PM i21821).
        for label, length in (("entry-AF-empty", 0), ("entry-AF-missing", 3)):
            memory, cpu = make_case(source, length, None)
            cpu.flags |= 0x0010
            compare_case(label, memory, cpu, source, library)
        memory, cpu = make_case(source, 0, None)
        # First link aliases the live far argument in the caller's frame.
        put_word(memory, cpu.ds, 0x5D96, 0x7F00)
        put_word(memory, cpu.ds, 0x5D98, cpu.ss)
        put_word(memory, cpu.ss, 0x8004, 0x0100)
        put_word(memory, cpu.ss, 0x8006, 0x5000)
        compare_case("link-argument-alias", memory, cpu, source, library)
        memory, cpu = make_case(source, 1, None)
        put_word(memory, cpu.ss, 0x8004, 0x0200)
        put_word(memory, cpu.ss, 0x8006, 0x4000)
        compare_case("same-segment-offset-miss", memory, cpu,
                     source, library)
        for label, segment, offset in (("segment-only-pointer", 0x4000, 0),
                                       ("offset-only-pointer", 0, 0x0100)):
            memory, cpu = make_case(source, 0, None)
            put_word(memory, cpu.ds, 0x5D96, offset)
            put_word(memory, cpu.ds, 0x5D98, segment)
            put_word(memory, cpu.ss, 0x8004, offset)
            put_word(memory, cpu.ss, 0x8006, segment)
            compare_case(label, memory, cpu, source, library)
        for target, label in ((0, "captured-head"),
                              (None, "captured-missing")):
            memory, cpu = make_case(source, 1, target, captured=True)
            compare_case(label, memory, cpu, source, library)
        if args.native_run is not None:
            audit_native(args.native_run, source, library)
    print("unit 0x25:+0756: 102 bytes, 35 instructions, 16 original-byte/C "
          "cases; full RAM and defined returned CPU state equal")


if __name__ == "__main__":
    main()
