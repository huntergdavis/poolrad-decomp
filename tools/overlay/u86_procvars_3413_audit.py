#!/usr/bin/env python3
"""U86 +3413 original-byte/C full-state oracle; media stays in resources/."""

import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
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

from overlay_inventory import find_units, parse_mz

ROOT = Path(__file__).resolve().parents[2]
SIZE = 1 << 20
ENTRY, END = 0x3413, 0x37B0
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "66ffc8c06e80199ee68e064db44ccb47bcb2e51d661786e0bcdc4f466080e5d5"
SEGMENT_SITES = [0x341A] + [0x343E + 13 * i for i in range(67)] + [0x37A5]
REGISTERS = (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX,
             UC_X86_REG_SI, UC_X86_REG_DI, UC_X86_REG_BP, UC_X86_REG_SP,
             UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_DS, UC_X86_REG_ES,
             UC_X86_REG_SS, UC_X86_REG_EFLAGS)


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(segment, offset):
    return ((segment << 4) + offset) & (SIZE - 1)


def put16(memory, segment, offset, value):
    at = physical(segment, offset)
    memory[at] = value & 255
    memory[(at + 1) & (SIZE - 1)] = value >> 8


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(exe) != 65231 or sha256(exe).hexdigest() != EXE_SHA:
        raise ValueError("START.EXE provenance differs")
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("GAME.OVR provenance differs")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    unit = next(u for u in units if u["stub_para"] == 0x86)
    body = ovr[unit["ovr_offset"] + ENTRY:unit["ovr_offset"] + END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    if sha256(body).hexdigest() != BODY_SHA or len(decoded) != 286 or sum(i.size for i in decoded) != 925:
        raise ValueError("U86 source extent differs")
    if [i.mnemonic for i in decoded].count("retf") != 1 or decoded[-1].address != END - 1:
        raise ValueError("U86 return differs")
    if any(i.mnemonic not in ("push", "mov", "xor", "pop", "retf") for i in decoded):
        raise ValueError("U86 gained a dependency or branch")
    sites = [i.address + 1 for i in decoded if i.mnemonic == "mov" and i.op_str == "dx, 0x96"]
    if sites != SEGMENT_SITES or set(sites) != {p for p in unit["fixups"] if ENTRY <= p < END}:
        raise ValueError("U86 loaded segment operands differ from relocation table")
    if unit["memory_segment"] != 0x96 or unit["entries"][10] != ENTRY:
        raise ValueError("U86 public entry/stub differs")
    caller = next(u for u in units if u["stub_para"] == 0x36)
    caller_code = ovr[caller["ovr_offset"]:caller["ovr_offset"] + caller["code_size"]]
    caller_decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(caller_code[0x2D:0x60C], 0x2D))
    call = next((i for i in caller_decoded if i.address == 0x607), None)
    if call is None or call.mnemonic != "lcall" or call.op_str != "0x96, 0x52" or 0x60A not in caller["fixups"]:
        raise ValueError("aligned U36 caller or caller relocation differs")
    return body, {i.address: i for i in decoded}


CHECKER_ROWS = ('OVL_0086_3413',)


def library(directory):
    path = directory / "u86_3413.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u86_procvars_3413.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u86_procvars_3413.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                       ctypes.POINTER(Cpu)]
    lib.pr_u86_procvars_3413.restype = ctypes.c_int
    return lib


def make_case(body, *, ds=0x1000, ss=0x2000, sp=0x8000, base=0, operands=None,
              flags=0x0202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0xF000, ENTRY, ds, 0x1111, ss, flags)
    at = physical(cpu.cs, ENTRY)
    memory[at:at + len(body)] = body
    values = operands if operands is not None else [(base + 0x96) & 0xFFFF] * len(SEGMENT_SITES)
    for ip, value in zip(SEGMENT_SITES, values):
        put16(memory, cpu.cs, ip, value)
    at = physical(ss, sp)
    for i, value in enumerate((0x12, 7, cpu.cs & 255, cpu.cs >> 8)):
        memory[(at + i) & (SIZE - 1)] = value
    return memory, cpu


def compare(label, memory, cpu, decoded, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    entry = physical(cpu.cs, ENTRY)
    loaded_body = memory[entry:entry + END - ENTRY]
    visited, at_return = set(), [False]

    def on_instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry + END - ENTRY or ip not in decoded:
            raise ValueError(f"unexpected control transfer: {label}")
        visited.add(ip)
        if ip == END - 1:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(entry, 2 * SIZE, count=1000)
    if not at_return[0]:
        raise ValueError(f"original did not return: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs, END - 1), 2 * SIZE, count=1)
    if bytes(backing[entry:entry + END - ENTRY]) != loaded_body:
        raise ValueError(f"immutable code modified: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u86_procvars_3413(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected input: {label}")
    actual, expected = bytes(ram), bytes(backing)
    if actual != expected:
        diffs = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        mask = 0xFFEF if name == "flags" else 0xFFFF
        if (getattr(cpu, name) ^ expected_word) & mask:
            raise ValueError(f"{name} differs {label}: {getattr(cpu, name):04X}/{expected_word:04X}")
    return visited


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    body, decoded = source()
    visited, count = set(), 0
    with tempfile.TemporaryDirectory(prefix="poolrad-u86-3413-") as tmp:
        lib = library(Path(tmp))

        def run(label, **options):
            nonlocal visited, count
            memory, cpu = make_case(body, **options)
            visited |= compare(label, memory, cpu, decoded, lib)
            count += 1

        for base in (0, 0x824, 0x14A0, 0xF000, 0xFF6A, 0xFFFF):
            run(f"loader-base-{base:04X}", base=base)
        for sp in list(range(0x6B80, 0x6C99, 8 if args.quick else 1)) + [
            0x681F, 0x6820, 0x6821, 0x6822, 0x6823, 0x6988, 0x698A, 0x698C,
            0x6DB5, 0x6DB6, 0x6DB7, 0x6DB8, 0x6DB9, 0x6DBF, 0x6DC0]:
            run(f"frame-global-alias-{sp:04X}", ss=0x1000, sp=sp, base=0x824)
        for ds in (0, 0xF000, 0xFFFF):
            for ss in (0, 0xFFFF):
                for sp in (0, 1, 2, 3, 4, 0xFFFD, 0xFFFE, 0xFFFF):
                    run(f"wrap-{ds:04X}-{ss:04X}-{sp:04X}", ds=ds, ss=ss, sp=sp)
        rng = random.Random(0x863413)
        for index in range(8 if args.quick else 256):
            run(f"seeded-relocations-{index}", operands=[rng.randrange(0x10000) for _ in SEGMENT_SITES],
                flags=rng.choice((0x0202, 0x0ED7, 0x0047, 0x0A93)), background=rng.randbytes(SIZE))
    report = dict(target="OVL_0086_3413", source_sha256=BODY_SHA, cases=count,
                  covered_sites=len(visited), source_sites=len(decoded),
                  unvisited_sites=[f"{ip:04X}" for ip in sorted(set(decoded) - visited)],
                  compared_ram_bytes=SIZE, compared_cpu_words=14, segment_fixups=len(SEGMENT_SITES),
                  static_caller="OVL_0036_002D:+0607 -> STUB_0096_0052")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
