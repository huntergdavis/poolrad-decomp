#!/usr/bin/env python3
"""Hash-lock u9B +0329 and compare terminating C paths with original bytes."""

import argparse
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from u6f_u9b_transfer_audit import audit as audit_transfer

SPAN_SHA = "f5ff12924988b708edcf3756213532a2c5ba1b4860a5f63e11ffcd229297f3d6"
RAM_SIZE = 1 << 20


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def physical(segment, offset):
    return ((segment << 4) + offset) & (RAM_SIZE - 1)


def word_at(memory, address, value):
    memory[address & (RAM_SIZE - 1)] = value & 0xFF
    memory[(address + 1) & (RAM_SIZE - 1)] = value >> 8


def audit_span(exe, ovr):
    audit_transfer(exe, ovr)
    source = ovr.read_bytes()[0x2ADB0:0x2AEE5]
    check(len(source) == 309 and sha256(source).hexdigest() == SPAN_SHA,
          "u9B +0329 source span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x0329))
    check(len(decoded) == 125 and decoded[-1].address == 0x045B and
          decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "4" and
          decoded[-1].address + decoded[-1].size == 0x045E,
          "u9B +0329 instruction extent changed")
    check(not any("call" in ins.mnemonic for ins in decoded) and
          sum(ins.mnemonic.startswith("j") for ins in decoded) == 7,
          "u9B +0329 control flow changed")
    sites = {ins.address: ins for ins in decoded}
    for address, mnemonic in ((0x032F, "les"), (0x0358, "cmp"),
                              (0x039B, "cdq"), (0x03AE, "mov"),
                              (0x0400, "mov"), (0x0426, "mov"),
                              (0x044B, "mov"), (0x045B, "retf")):
        check(sites[address].mnemonic == mnemonic,
              f"u9B +0329 site {address:#x} changed")
    for ins in decoded:
        if ins.mnemonic.startswith("j"):
            target = int(ins.op_str, 16)
            check(target in sites, f"branch target {target:#x} out of span")
    print(f"u9B +0329..+045D: {len(source)} bytes, {len(decoded)} "
          f"instructions, 7 branches, no calls, SHA-256 {SPAN_SHA}")
    return source


def build_library(root, directory):
    library = directory / "u9b_0329.so"
    command = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
               "-Wpedantic", "-Werror", "-fPIC", "-shared", "-I",
               str(root / "include"),
               str(root / "src/recovered/overlay_u9b_record_fields_0329.c"),
               "-o", str(library)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u9b_record_fields_0329.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u9b_record_fields_0329.restype = ctypes.c_int
    return result


def run_c(library, memory, cpu):
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = library.pr_u9b_record_fields_0329(ram, RAM_SIZE, ctypes.byref(cpu))
    check(status == 0, "portable C rejected valid RAM")
    return bytes(ram), cpu


def compare_unicorn(label, memory, cpu, source, library):
    from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
    from unicorn.x86_const import (
        UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
        UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
        UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
        UC_X86_REG_SS, UC_X86_REG_EFLAGS)

    registers = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                 UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
                 UC_X86_REG_BP, UC_X86_REG_SP, UC_X86_REG_CS,
                 UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
                 UC_X86_REG_SS, UC_X86_REG_EFLAGS)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, RAM_SIZE)
    original.mem_write(0, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, registers):
        original.reg_write(register, getattr(cpu, name))
    returned = [False]
    code_start = physical(cpu.cs, 0x0329)
    code_end = physical(cpu.cs, 0x045E)
    check(memory[code_start:code_end] == source,
          f"synthetic loaded code changed: {label}")

    def on_instruction(emulator, address, size, user):
        del size, user
        if not code_start <= address < code_end:
            returned[0] = True
            emulator.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(code_start, RAM_SIZE, count=200000)
    check(returned[0], f"original synthetic case did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    actual, cpu = run_c(library, memory, cpu)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected))
                       if a != b]
        raise ValueError(f"synthetic RAM mismatch {label}: "
                         f"first {differences[:8]}")
    for (name, _), register in zip(Cpu._fields_, registers):
        expected_register = original.reg_read(register) & 0xFFFF
        check(getattr(cpu, name) == expected_register,
              f"synthetic register {name} mismatch {label}: "
              f"C={getattr(cpu, name):04x}, DOS={expected_register:04x}")


def synthetic_case(label, source, library, rng, record_segment=0x4000,
                   record_offset=0x0100, ds=0x1000, sp=0x8000,
                   table_mode="random"):
    memory = bytearray(RAM_SIZE)
    cs, ss = 0x3000, 0x2000
    code_start = physical(cs, 0x0329)
    memory[code_start:code_start + len(source)] = source
    for address in range(0x3CF3, 0x3E80):
        value = rng.randrange(256) if table_mode == "random" else 0
        memory[physical(ds, address)] = value
    for field in (0x13, 0x2E, 0x9C):
        memory[physical(record_segment, record_offset + field)] = (
            rng.choice((0, 1, 2, 5, 0x7F, 0x80, 0xFF)) if field != 0x13
            else rng.randrange(256))
    frame = physical(ss, sp)
    for displacement, value in ((0, 0x7000), (2, cs),
                                (4, record_offset), (6, record_segment)):
        word_at(memory, frame + displacement, value)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0329, ds, 0x1111, ss, 0x7202)
    compare_unicorn(label, memory, cpu, source, library)


def audit_synthetic(source, library):
    rng = random.Random(0x9329)
    for case in range(120):
        synthetic_case(f"disjoint-{case}", source, library, rng)
    for case in range(15):
        synthetic_case(f"zero-{case}", source, library, rng,
                       table_mode="zero")
    synthetic_case("return-frame alias", source, library, rng,
                   record_segment=0x2000, record_offset=0x7F89,
                   table_mode="zero")
    synthetic_case("DS table alias", source, library, rng,
                   record_segment=0x1000, record_offset=0x3C7D)
    synthetic_case("cached far-pointer alias", source, library, rng,
                   record_segment=0x2000, record_offset=0x7F82,
                   table_mode="zero")
    for label, sp in (("LES offset crossing", 0xFFFB),
                      ("RETF offset crossing", 0xFFFF)):
        synthetic_case(label, source, library, rng, sp=sp,
                       table_mode="zero")
    print("140 synthetic original-byte/C cases: full 1 MiB RAM and all "
          "returned registers equal")


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--unicorn", action="store_true",
                        help="compare original bytes with portable C")
    args = parser.parse_args()
    source = audit_span(args.exe, args.ovr)
    if args.unicorn:
        with tempfile.TemporaryDirectory(prefix="poolrad-u9b-0329-") as tmp:
            library = build_library(root, Path(tmp))
            audit_synthetic(source, library)


if __name__ == "__main__":
    main()
