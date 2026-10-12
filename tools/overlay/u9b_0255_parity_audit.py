#!/usr/bin/env python3
"""Hash-lock u9B +0255 and optionally compare private native/synthetic runs."""

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


SPAN_SHA = "f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326"
PRE_SHA = "0b50157ab03676d796b60f6ccda0b73fa17556a5775f49c91154f81faf0b8705"
POST_SHA = "e08be366b1571cfa234c305fb94b23394b264889f7b7f00f370b12baef73c29c"
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
    source = ovr.read_bytes()[0x2ACDC:0x2ADB0]
    check(len(source) == 212 and sha256(source).hexdigest() == SPAN_SHA,
          "u9B +0255 source span changed")
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(source, 0x0255))
    check(len(decoded) == 86 and decoded[-1].address == 0x0326 and
          decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "4" and
          decoded[-1].address + decoded[-1].size == 0x0329,
          "u9B +0255 instruction extent changed")
    check(not any(ins.mnemonic in ("call", "lcall") for ins in decoded) and
          sum(ins.mnemonic.startswith("j") for ins in decoded) == 9,
          "u9B +0255 control flow changed")
    instructions = {ins.address: ins for ins in decoded}
    for offset, mnemonic in ((0x026D, "mov"), (0x02BD, "mov"),
                             (0x0300, "mov"), (0x030D, "mov"),
                             (0x0326, "retf")):
        check(instructions[offset].mnemonic == mnemonic,
              f"u9B +0255 site {offset:#x} changed")
    print(f"u9B +0255..+0328: {len(source)} bytes, {len(decoded)} instructions, "
          f"9 branches, no calls, SHA-256 {SPAN_SHA}")
    return source


def build_library(root, directory):
    library = directory / "u9b_0255.so"
    command = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
               "-Wpedantic", "-Werror", "-fPIC", "-shared", "-I",
               str(root / "include"),
               str(root / "src/recovered/overlay_u9b_record_table_0255.c"),
               "-o", str(library)]
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u9b_record_table_0255.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u9b_record_table_0255.restype = ctypes.c_int
    return result


def run_c(library, memory, cpu):
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = library.pr_u9b_record_table_0255(ram, RAM_SIZE, ctypes.byref(cpu))
    check(status == 0, "portable C rejected valid RAM")
    return bytes(ram), cpu


def audit_native(directory, source, library):
    pre = (directory / "u9b-pre-0255-ram.bin").read_bytes()
    post = (directory / "u9b-return-01ac-ram.bin").read_bytes()
    check(len(pre) == 0xA0000 and sha256(pre).hexdigest() == PRE_SHA,
          "private pre-call RAM changed")
    check(len(post) == 0xA0000 and sha256(post).hexdigest() == POST_SHA,
          "private post-call RAM changed")
    check(sha256(pre[physical(0x2CCD, 0x0255):
                     physical(0x2CCD, 0x0329)]).digest() == sha256(source).digest(),
          "native loaded leaf changed")
    memory = bytearray(pre + bytes(RAM_SIZE - len(pre)))
    # The capture stops at CALL +0191, after PUSH CS; a near CALL writes
    # return IP +0194 at SS:3784 and enters +0255 with SP=3784.
    word_at(memory, physical(0x1D16, 0x3784), 0x0194)
    cpu = Cpu(7, 0x001F, 0, 0, 0x44BA, 8, 0x3794, 0x3784,
              0x2CCD, 0x0255, 0x14A0, 0x449A, 0x1D16, 0x7246)
    result, cpu = run_c(library, memory, cpu)
    check(result[:len(post)] == post,
          "portable C differs from saved native post-leaf RAM")
    check((cpu.cs, cpu.ip, cpu.sp, cpu.bp, cpu.ax, cpu.bx, cpu.cx,
           cpu.dx, cpu.si, cpu.di, cpu.es, cpu.flags) ==
          (0x2CCD, 0x0194, 0x378C, 0x3794, 7, 4, 5, 0x11,
           1, 0x000F, 0x449A, 0x7246),
          "portable C post-leaf register state changed")
    print("saved +0191 near-call to +0255: full 640 KiB RAM equal at +01AC;")
    print("returned CS:IP 2CCD:0194, SP 378C, BP 3794, FLAGS 7246")


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
    reached_return = [False]
    code_start = physical(cpu.cs, 0x0255)
    code_end = physical(cpu.cs, 0x0329)
    check(memory[code_start:code_end] == source,
          f"synthetic loaded code changed: {label}")

    def on_instruction(emulator, address, size, user):
        del size, user
        if not code_start <= address < code_end:
            reached_return[0] = True
            emulator.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(code_start, 0, count=200000)
    check(reached_return[0], f"original synthetic case did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    actual, cpu = run_c(library, memory, cpu)
    check(actual == expected, f"synthetic full-RAM mismatch: {label}")
    for (name, _), register in zip(Cpu._fields_, registers):
        check(getattr(cpu, name) == (original.reg_read(register) & 0xFFFF),
              f"synthetic register {name} mismatch: {label}")


def synthetic_case(label, source, library, record_segment, record_offset,
                   ds, classes, fields, table_seed, sp=0x8000):
    memory = bytearray(RAM_SIZE)
    cs, ss = 0x3000, 0x2000
    start = physical(cs, 0x0255)
    memory[start:start + len(source)] = source
    for index in range(0x600):
        memory[physical(ds, 0x426E + index)] = (index * table_seed + 17) & 0xFF
    for index, value in enumerate(classes):
        memory[physical(record_segment, record_offset + 0x96 + index)] = value
    for index, value in enumerate(fields):
        memory[physical(record_segment, record_offset + 0x6D + index)] = value
    frame = physical(ss, sp)
    for displacement, value in ((0, 0x7000), (2, cs),
                                (4, record_offset), (6, record_segment)):
        word_at(memory, frame + displacement, value)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0x0255, ds, 0x1111, ss, 0x7202)
    compare_unicorn(label, memory, cpu, source, library)


def audit_synthetic(source, library):
    rng = random.Random(0x2505)
    for case in range(100):
        classes = [rng.choice((0, 0, 1, 2, 4, 8, 0x7F, 0x80, 0xFF))
                   for _ in range(8)]
        fields = [rng.randrange(256) for _ in range(5)]
        synthetic_case(f"disjoint-{case}", source, library, 0x4000, 0x0100,
                       0x1000, classes, fields, rng.randrange(1, 255))
    synthetic_case("argument alias", source, library, 0x2000, 0x7F97,
                   0x1000, [1, 2, 0, 0, 0, 0, 0, 0], [40, 41, 42, 43, 44], 7)
    synthetic_case("DS table alias", source, library, 0x1000, 0x4201,
                   0x1000, [1, 2, 3, 4, 5, 6, 7, 8], [40, 41, 42, 43, 44], 9)
    synthetic_case("return-frame alias", source, library, 0x2000, 0x7F93,
                   0x1000, [1, 2, 0, 0, 0, 0, 0, 0], [40, 41, 42, 43, 44], 11)
    for label, sp in (("LES offset crossing", 0xFFFB),
                      ("RETF offset crossing", 0xFFFF)):
        synthetic_case(label, source, library, 0x4000, 0x0100,
                       0x1000, [0] * 8, [0] * 5, 1, sp)
    print("105 synthetic original-byte/C cases: full 1 MiB RAM and all "
          "returned registers equal")


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--native-dir", type=Path,
                        help="ignored saved +0191 and +01AC RAM captures")
    parser.add_argument("--unicorn", action="store_true",
                        help="compare synthetic cases with original bytes in Unicorn")
    args = parser.parse_args()
    source = audit_span(args.exe, args.ovr)
    if args.native_dir is not None or args.unicorn:
        with tempfile.TemporaryDirectory(prefix="poolrad-u9b-0255-") as temporary:
            library = build_library(root, Path(temporary))
            if args.native_dir is not None:
                audit_native(args.native_dir, source, library)
            if args.unicorn:
                audit_synthetic(source, library)


if __name__ == "__main__":
    main()
