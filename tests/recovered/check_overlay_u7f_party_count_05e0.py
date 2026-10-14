#!/usr/bin/env python3
"""Compare original GAME.OVR unit 0x7F:+05E0 with its C translation.

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
SPAN_SHA = "d4e73837529d5f4bb1ac788885f216ba39ccf5f67778a8278862f846c752a969"
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


def load_source():
    data = OVR.read_bytes()
    check(len(data) == 206105 and sha256(data).hexdigest() == OVR_SHA,
          "GAME.OVR provenance mismatch")
    source = data[0x25269:0x252C3]
    check(len(source) == 90 and sha256(source).hexdigest() == SPAN_SHA,
          "unit 0x7F:+05E0 source span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x05E0))
    check(len(decoded) == 30 and decoded[-1].address == 0x0639 and
          decoded[-1].mnemonic == "retf" and
          decoded[-1].address + decoded[-1].size == 0x063A,
          "unit 0x7F:+05E0 instruction extent changed")
    check(not any("call" in ins.mnemonic for ins in decoded),
          "unit 0x7F:+05E0 gained a call")
    sites = {ins.address for ins in decoded}
    for ins in decoded:
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str, 16) in sites,
                  f"branch at {ins.address:#x} leaves the span")
    return source


def build_library(directory):
    library = directory / "u7f_05e0.so"
    command = ["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
               "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
               str(ROOT / "src/recovered/overlay_u7f_party_count_05e0.c"),
               "-o", str(library)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u7f_party_count_05e0.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u7f_party_count_05e0.restype = ctypes.c_int
    return result


def compare_case(label, memory, cpu, source, library):
    code_start = physical(cpu.cs, 0x05E0)
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
    status = library.pr_u7f_party_count_05e0(ram, RAM_SIZE, ctypes.byref(cpu))
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


def make_case(source, statuses, sp=0x8000, offset=0x0100,
              head=None, captured=False):
    memory = bytearray(RAM_SIZE)
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
        if head is None:
            head = (0x4000, offset) if statuses else (0, 0)
        put_word(memory, ds, 0x5D96, head[1])
        put_word(memory, ds, 0x5D98, head[0])
        for i, value in enumerate(statuses):
            segment = 0x4000 + i * 0x20
            next_segment = segment + 0x20 if i + 1 < len(statuses) else 0
            memory[physical(segment, (offset + 0x84) & 0xFFFF)] = value
            put_word(memory, segment, (offset + 0x104) & 0xFFFF,
                     offset if next_segment else 0)
            put_word(memory, segment, (offset + 0x106) & 0xFFFF,
                     next_segment)
    cs, ss = 0xF000, 0xE000
    memory[physical(cs, 0x05E0):physical(cs, 0x05E0) + len(source)] = source
    put_word(memory, ss, sp, 0x0712)
    put_word(memory, ss, (sp + 2) & 0xFFFF, 0xF000)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x05E0, ds, 0x1111, ss, 0x7202)
    return memory, cpu


def main():
    source = load_source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-05e0-") as tmp:
        library = build_library(Path(tmp))
        cases = [
            ("empty", []), ("status-zero", [0]),
            ("status-b3", [0xB3]), ("status-other", [0x17]),
            ("mixed", [0x17, 0, 0xB3, 0xFF, 0]),
            ("count-wrap", [0] * 257),
        ]
        for label, statuses in cases:
            memory, cpu = make_case(source, statuses)
            compare_case(label, memory, cpu, source, library)
        for label, sp in (("stack-wrap-return", 0xFFFF),
                          ("stack-wrap-local", 0x0007),
                          ("stack-near-end", 0xFFFA)):
            memory, cpu = make_case(source, [], sp=sp)
            compare_case(label, memory, cpu, source, library)
        memory, cpu = make_case(source, [0xB3], offset=0xFF00)
        compare_case("node-offset-wrap", memory, cpu, source, library)
        memory, cpu = make_case(source, [0], sp=0x0006)
        compare_case("LES-local-offset-wrap", memory, cpu, source, library)
        memory, cpu = make_case(source, [])
        # The node's status byte is the live counter local at SS:[BP-6].
        put_word(memory, cpu.ds, 0x5D96, 0x7F74)
        put_word(memory, cpu.ds, 0x5D98, cpu.ss)
        put_word(memory, cpu.ss, 0x8078, 0)
        put_word(memory, cpu.ss, 0x807A, 0)
        compare_case("status-count-local-alias", memory, cpu, source,
                     library)
        for label, head in (("segment-only-pointer", (0x4000, 0)),
                            ("offset-only-pointer", (0, 0x0100))):
            memory, cpu = make_case(source, [], head=head)
            compare_case(label, memory, cpu, source, library)
        memory, cpu = make_case(source, [], captured=True)
        compare_case("captured-party-state", memory, cpu, source, library)
    print("unit 0x7F:+05E0: 90 bytes, 30 instructions, 15 original-byte/C "
          "cases; full RAM and defined returned CPU state equal")


if __name__ == "__main__":
    main()
