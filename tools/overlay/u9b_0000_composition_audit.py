#!/usr/bin/env python3
"""Hash-lock and compare the composed u9B +0000 C path to original bytes."""

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

RAM_SIZE = 1 << 20
IMAGE_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPANS = (
    (0x0000, 0x01AF, 159, "260fb152b8d892e56d4ee8dd3c22714c09d5d696490b0828a4653a9c9e5aee71"),
    (0x01AF, 0x0255, 49, "5dd90fb8796e7aa15a770e4868aaceeee0a0886e0251a9fc7c10f3f32935652c"),
    (0x0255, 0x0329, 86, "f19017361e6ac6197e38eeee7b70f438d29962a34675ff43903ac712444dd326"),
    (0x0329, 0x045E, 125, "f5ff12924988b708edcf3756213532a2c5ba1b4860a5f63e11ffcd229297f3d6"),
)
NATIVE_ENTRY_SHA = "241080db30401c3c6ca757e556e632443d6dfbff19657254cd82bff99a9767d3"
NATIVE_POST_SHA = "e08be366b1571cfa234c305fb94b23394b264889f7b7f00f370b12baef73c29c"


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
    image = ovr.read_bytes()
    check(sha256(image).hexdigest() == IMAGE_SHA, "GAME.OVR changed")
    source = image[0x2AA87:0x2AEE5]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    for start, end, count, digest in SPANS:
        span = source[start:end]
        decoded = list(md.disasm(span, start))
        check(sha256(span).hexdigest() == digest and len(decoded) == count and
              decoded[-1].address + decoded[-1].size == end,
              f"u9B {start:#x} span changed")
        if start == 0:
            calls = [(ins.address, ins.op_str) for ins in decoded
                     if "call" in ins.mnemonic]
            check(calls == [(0x00EC, "0x1af"), (0x0191, "0x255"),
                            (0x01A6, "0x329")],
                  "u9B +0000 call targets changed")
            sites = {ins.address for ins in decoded}
            for ins in decoded:
                if ins.mnemonic.startswith("j"):
                    check(int(ins.op_str, 16) in sites,
                          f"u9B +0000 branch exits body: {ins.address:#x}")
    print("u9B +0000..+045D: four hash-locked spans, "
          "419 instructions, three internal calls, no external calls")
    return source


def build_library(root, directory):
    library = directory / "u9b_0000.so"
    command = [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
               "-Wpedantic", "-Werror", "-fPIC", "-shared", "-I",
               str(root / "include")]
    command.extend(str(root / "src/recovered" / name) for name in (
        "overlay_u9b_entry_zero_0000.c",
        "overlay_u9b_record_table_0255.c",
        "overlay_u9b_record_fields_0329.c"))
    command.extend(("-o", str(library)))
    subprocess.run(command, check=True, capture_output=True, text=True)
    result = ctypes.CDLL(str(library))
    result.pr_u9b_entry_zero_0000.argtypes = [
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    result.pr_u9b_entry_zero_0000.restype = ctypes.c_int
    return result


def run_c(library, memory, cpu):
    ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer_copy(memory)
    status = library.pr_u9b_entry_zero_0000(ram, RAM_SIZE, ctypes.byref(cpu))
    return bytes(ram), cpu, status


def compare_unicorn(label, memory, cpu, source, library, native_post=None,
                    expected_status=0):
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
    code_start = physical(cpu.cs, 0)
    code_end = physical(cpu.cs, 0x045E)
    check(memory[code_start:code_end] == source,
          f"loaded u9B code differs: {label}")
    trace = {0x00EC: 0, 0x0191: 0, 0x01A6: 0}
    returned = [False]

    def on_instruction(emulator, address, size, user):
        del size, user
        if code_start <= address < code_end:
            offset = address - code_start
            if offset in trace:
                trace[offset] += 1
        else:
            returned[0] = True
            emulator.emu_stop()

    original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(code_start, RAM_SIZE, count=500000)
    check(returned[0], f"original case did not return: {label}")
    expected = bytes(original.mem_read(0, RAM_SIZE))
    actual, cpu, status = run_c(library, memory, cpu)
    check(status == expected_status,
          f"{label} portable C status {status}, expected {expected_status}")
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected))
                       if a != b]
        raise ValueError(f"{label} RAM differs at {differences[:12]}, "
                         f"calls {trace}")
    if native_post is not None:
        check(actual[:len(native_post)] == native_post,
              "composed C differs from pinned native +01AC RAM")
    for (name, _), register in zip(Cpu._fields_, registers):
        value = original.reg_read(register) & 0xFFFF
        check(getattr(cpu, name) == value,
              f"{label} register {name}: C={getattr(cpu, name):04x} "
              f"DOS={value:04x}, calls {trace}")
    return trace


def synthetic_case(label, source, library, rng, classes,
                   gate=0, age=0, sp=0x8000,
                   rec_seg=0x4000, rec_off=0x0100, ss=0x2000,
                   zero_tables=False, return_ip=0x7000,
                   expected_status=0):
    memory = bytearray(RAM_SIZE)
    cs, ds = 0x3000, 0x1000
    code_start = physical(cs, 0)
    memory[code_start:code_start + len(source)] = source
    for address in range(0x31FA, 0x4300):
        memory[physical(ds, address)] = (
            0 if zero_tables else rng.randrange(256))
    for index, value in enumerate(classes):
        memory[physical(rec_seg, rec_off + 0x96 + index)] = value
    for field, value in ((0x9C, gate), (0x12, age),
                         (0xB2, 1), (0xB3, 1), (0xB4, 1)):
        memory[physical(rec_seg, rec_off + field)] = value
    frame = physical(ss, sp)
    for displacement, value in ((0, return_ip), (2, cs),
                                (4, rec_off), (6, rec_seg)):
        word_at(memory, frame + displacement, value)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, cs, 0, ds, 0x1111, ss, 0x7202)
    return compare_unicorn(label, memory, cpu, source, library,
                           expected_status=expected_status)


def audit_synthetic(source, library):
    rng = random.Random(0x9B0000)
    totals = {0x00EC: 0, 0x0191: 0, 0x01A6: 0}
    cases = [
        ("all-zero", [0] * 8, 0, 0),
        ("increment", [2] + [0] * 7, 0, 0x11),
        ("fields", [0] * 8, 1, 0),
        ("both", [2] + [0] * 7, 1, 0x11),
        ("index-two", [0, 0, 7] + [0] * 5, 0, 0),
        ("index-five", [0] * 5 + [2, 0, 0], 0, 0),
    ]
    for label, classes, gate, age in cases:
        trace = synthetic_case(label, source, library, rng,
                               classes, gate, age)
        for address in totals:
            totals[address] += trace[address]
    for case in range(100):
        classes = [rng.choice((0, 0, 1, 2, 6, 7, 0x7F, 0x80, 0xFF))
                   for _ in range(8)]
        trace = synthetic_case(f"random-{case}", source, library, rng,
                               classes, rng.choice((0, 1, 2, 0x7F, 0x80)),
                               rng.randrange(256))
        for address in totals:
            totals[address] += trace[address]
    for label, sp in (("LES offset crossing", 0xFFFB),
                      ("RETF offset crossing", 0xFFFF)):
        trace = synthetic_case(label, source, library, rng,
                               [0] * 8, sp=sp, ss=0x5000)
        for address in totals:
            totals[address] += trace[address]
    for label, rec_seg, rec_off in (
        ("DS table alias", 0x1000, 0x3C4F),
        ("SS return-frame alias", 0x2000, 0x7FD3),
    ):
        trace = synthetic_case(label, source, library, rng, [0] * 8,
                               rec_seg=rec_seg, rec_off=rec_off)
        for address in totals:
            totals[address] += trace[address]
    trace = synthetic_case("top return overwritten", source, library, rng,
                           [0] * 8, rec_seg=0x2000, rec_off=0x7FD3,
                           zero_tables=True, return_ip=0x70AA)
    for address in totals:
        totals[address] += trace[address]
    trace = synthetic_case("near-return transfer", source, library, rng,
                           [0] * 8, rec_seg=0x2000, rec_off=0x7F81,
                           zero_tables=True, expected_status=2)
    for address in totals:
        totals[address] += trace[address]
    check(all(value > 0 for value in totals.values()),
          f"synthetic cases missed a call site: {totals}")
    print(f"112 synthetic original-byte/C cases: full 1 MiB RAM and all "
          f"returned registers equal; calls {totals}")


def audit_native(directory, source, library):
    entry = (directory / "u9b-entry-0000-ram.bin").read_bytes()
    post = (directory / "u9b-return-01ac-ram.bin").read_bytes()
    check(len(entry) == 0xA0000 and
          sha256(entry).hexdigest() == NATIVE_ENTRY_SHA,
          "pinned native +0000 entry changed")
    check(len(post) == 0xA0000 and sha256(post).hexdigest() == NATIVE_POST_SHA,
          "pinned native +01AC RAM changed")
    memory = bytearray(entry + bytes(RAM_SIZE - len(entry)))
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x37AE, 0x3796, 0x2CCD, 0, 0x14A0, 0x449A,
              0x1D16, 0x7202)
    trace = compare_unicorn("saved native", memory, cpu, source,
                            library, native_post=post)
    check(trace == {0x00EC: 0, 0x0191: 1, 0x01A6: 0},
          f"saved native call route changed: {trace}")
    print("saved native +0000..+01AC: 640 KiB RAM equal; "
          "only +0255 reached")


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--native-dir", type=Path,
                        help="ignored pinned +0000 and +01AC RAM captures")
    parser.add_argument("--unicorn", action="store_true")
    args = parser.parse_args()
    source = audit_span(args.exe, args.ovr)
    if args.unicorn or args.native_dir is not None:
        with tempfile.TemporaryDirectory(prefix="poolrad-u9b-0000-") as tmp:
            library = build_library(root, Path(tmp))
            if args.unicorn:
                audit_synthetic(source, library)
            if args.native_dir is not None:
                audit_native(args.native_dir, source, library)


if __name__ == "__main__":
    main()
