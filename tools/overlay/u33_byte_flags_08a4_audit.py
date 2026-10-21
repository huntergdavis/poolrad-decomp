#!/usr/bin/env python3
"""Compare U33 +08A4 original bytes with C; media stays under resources/."""

import argparse
import ctypes
from hashlib import sha256
import json
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
SIZE = 1 << 20
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "c721e542769e5b590a6ef00ee92ffa9b4a9b7a5a76a9c4563eda25b3a4b703bc"
CHECKER_ROWS = ("OVL_0033_08A4",)
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
             UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
             UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(segment, offset):
    return ((segment << 4) + offset) & (SIZE - 1)


def word(memory, segment, offset):
    at = physical(segment, offset)
    return memory[at] | (memory[(at + 1) & (SIZE - 1)] << 8)


def put16(memory, segment, offset, value):
    at = physical(segment, offset)
    memory[at] = value & 255
    memory[(at + 1) & (SIZE - 1)] = value >> 8


def source(path):
    data = path.read_bytes()
    if len(data) != 206105 or sha256(data).hexdigest() != OVR_SHA:
        raise ValueError("GAME.OVR provenance differs")
    body = data[52853:53608]
    if sha256(body).hexdigest() != SPAN_SHA:
        raise ValueError("U33 source span differs")
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x08A4))
    if len(instructions) != 338 or instructions[-1].address != 0x0B96 or instructions[-1].mnemonic != "retf":
        raise ValueError("U33 source extent differs")
    sites = {i.address for i in instructions}
    if sum(i.size for i in instructions) != 755 or any("call" in i.mnemonic for i in instructions):
        raise ValueError("U33 source has gaps or dependencies")
    for instruction in instructions:
        if instruction.mnemonic.startswith("j") and int(instruction.op_str, 16) not in sites:
            raise ValueError("U33 branch leaves source span")
    return body, {i.address: i for i in instructions}


def library(directory):
    path = directory / "u33_08a4.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u33_byte_flags_08a4.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u33_byte_flags_08a4.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                         ctypes.POINTER(Cpu)]
    lib.pr_u33_byte_flags_08a4.restype = ctypes.c_int
    return lib


def make_case(body, value, old=0xA5, window=0xFE, *, sp=0x8000, ss=0x2000,
              window_segment=0x4000, window_offset=0x1000):
    memory = bytearray([0xA5]) * SIZE
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0xF000, 0x08A4, 0x1000, 0x1111, ss, 0x0202)
    at = physical(cpu.cs, 0x08A4)
    memory[at:at + len(body)] = body
    put16(memory, cpu.ds, 0x49D2, window_offset)
    put16(memory, cpu.ds, 0x49D4, window_segment)
    put16(memory, window_segment, (window_offset + 0x366) & 0xFFFF, window)
    at = physical(ss, sp)
    memory[at] = 0x12
    memory[(at + 1) & (SIZE - 1)] = 0x07
    memory[(at + 2) & (SIZE - 1)] = cpu.cs & 255
    memory[(at + 3) & (SIZE - 1)] = cpu.cs >> 8
    memory[physical(ss, (sp - 4) & 0xFFFF)] = old
    memory[physical(cpu.ds, 0x465A)] = value
    return memory, cpu


def compare(label, memory, cpu, body, decoded, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    visited, final_flags, at_return = set(), [None], [False]
    entry = physical(cpu.cs, 0x08A4)

    def on_instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry + len(body) or ip not in decoded:
            raise ValueError(f"unexpected control transfer: {label}")
        visited.add(ip)
        if decoded[ip].mnemonic in ("sub", "and", "or", "cmp"):
            final_flags[0] = decoded[ip].mnemonic
        if ip == 0x0B96:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(entry, 2 * SIZE, count=1000)
    if not at_return[0]:
        raise ValueError(f"original did not return: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs, 0x0B96), 2 * SIZE, count=1)
    if bytes(backing[entry:entry + len(body)]) != body:
        raise ValueError(f"immutable source modified: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u33_byte_flags_08a4(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected state: {label}")
    expected, actual = bytes(backing), bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {differences[:8]}")
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        mask = 0xFFEF if name == "flags" and final_flags[0] in ("and", "or") else 0xFFFF
        if (getattr(cpu, name) ^ expected_word) & mask:
            raise ValueError(f"{name} differs {label}: {getattr(cpu, name):04X}/{expected_word:04X}")
    return visited


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path, default=ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--compile-only", action="store_true")
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-08a4-") as tmp:
            library(Path(tmp))
        print("CHECKER_BUILD_OK")
        return
    body, decoded = source(args.ovr)
    visited, count = set(), 0
    with tempfile.TemporaryDirectory(prefix="poolrad-u33-08a4-") as tmp:
        lib = library(Path(tmp))

        def run(label, value, old=0xA5, window=0xFE, **options):
            nonlocal visited, count
            memory, cpu = make_case(body, value, old, window, **options)
            visited |= compare(label, memory, cpu, body, decoded, lib)
            count += 1

        for value in range(256):
            for old in ((0xA5,) if args.quick else (0, 0x80, 0xA5, 0xFF)):
                for window in (0xFE, 0xFF):
                    run(f"input-{value:02x}-local-{old:02x}-window-{window:04x}", value, old, window)
        for old in (() if args.quick else range(256)):
            for window in (0xFE, 0xFF):
                run(f"unmatched-local-{old:02x}-{window:04x}", 0, old, window)
        for sp in (0, 1, 2, 3, 4, 5, 0x465A, 0x465C, 0x465E,
                   0x49D2, 0x49D4, 0x49D6, 0x49D8, 0xFFFD, 0xFFFE, 0xFFFF):
            for value in (0, 1, 0x57, 0xE1):
                run(f"stack-alias-{sp:04x}-{value:02x}", value, ss=0x1000, sp=sp)
        for window in (0, 1, 0x7F, 0x80, 0xFE, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF):
            run(f"window-cmp-{window:04x}", 0x57, window=window)
            run(f"a20-window-{window:04x}", 0xE1, window=window,
                window_segment=0xFFFF, window_offset=0xFCA9)
    report = dict(target="OVL_0033_08A4", source_sha256=SPAN_SHA, cases=count,
                  covered_sites=len(visited), source_sites=len(decoded),
                  unvisited_sites=[f"{ip:04X}" for ip in sorted(set(decoded) - visited)],
                  compared_ram_bytes=SIZE, compared_cpu_words=len(REGISTERS))
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
