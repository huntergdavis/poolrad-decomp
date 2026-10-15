#!/usr/bin/env python3
"""Original-byte full-state U86 record predicate audit; media stays private."""

import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_IP

from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x323D, 0x328A
BODY_SHA = "2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd"


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
    dec = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    if len(body) != 77 or sha256(body).hexdigest() != BODY_SHA or len(dec) != 29 or sum(i.size for i in dec) != 77:
        raise ValueError("U86 predicate extent differs")
    if dec[-1].address != 0x3287 or dec[-1].mnemonic != "retf" or dec[-1].op_str != "4":
        raise ValueError("U86 predicate return differs")
    if unit["entries"][6] != ENTRY or unit["memory_segment"] != 0x96:
        raise ValueError("U86 entry/stub differs")
    decoded = {i.address: i for i in dec}
    for i in dec:
        if "call" in i.mnemonic or i.mnemonic.startswith("int"):
            raise ValueError("predicate gained an external dependency")
        if i.mnemonic.startswith("j") and int(i.op_str, 16) not in decoded:
            raise ValueError("predicate branch leaves the span")
    callers = []
    for u in units:
        code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
        for fixup in u["fixups"]:
            site = fixup - 3
            if site < 0 or code[site:site+5] != struct.pack("<BHH", 0x9A, 0x3E, 0x96):
                continue
            starts = sorted(f["rel"] for f in u["functions"])
            start = max(x for x in starts if x <= site)
            end = next((x for x in starts if x > start), len(code))
            caller = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[start:end], start)}
            if site not in caller or caller[site].mnemonic != "lcall":
                raise ValueError("far caller is not aligned from its function start")
            callers.append(dict(unit=u["stub_para"], function=start, site=site))
    if len(callers) != 9:
        raise ValueError("far caller set differs")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    near = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[0x51C:0x608], 0x51C)}
    if near[0x572].mnemonic != "call" or near[0x572].op_str != "0x323d" or near[0x571].op_str != "cs":
        raise ValueError("near caller frame differs")
    return body, decoded, callers


def library(directory):
    path = directory / "u86_323d.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u86_record_predicate_323d.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u86_record_predicate_323d.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u86_record_predicate_323d.restype = ctypes.c_int
    return lib


def make_case(body, index=5, value=12, *, record=(0x4000, 0x1000), ss=0x2000,
              sp=0x8000, ds=0x1000, flags=0x0202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0xF000, ENTRY, ds, 0x1111, ss, flags)
    at = physical(cpu.cs, ENTRY)
    memory[at:at + len(body)] = body
    segment, offset = record
    memory[physical(segment, (offset + 0x2E) & 0xFFFF)] = index
    memory[physical(ds, 0x5582 + 16 * index)] = value
    at = physical(ss, sp)
    for i, b in enumerate((0x12, 7, cpu.cs & 255, cpu.cs >> 8)):
        memory[(at + i) & (SIZE - 1)] = b
    put16(memory, ss, (sp + 4) & 0xFFFF, offset)
    put16(memory, ss, (sp + 6) & 0xFFFF, segment)
    return memory, cpu


def compare(label, memory, cpu, decoded, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    entry = physical(cpu.cs, ENTRY)
    expected_code = memory[entry:entry + END - ENTRY]
    visited, final_flags, at_return = set(), [None], [False]

    def on_instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry + END - ENTRY or ip not in decoded:
            raise ValueError(f"unexpected control transfer: {label}")
        visited.add(ip)
        if decoded[ip].mnemonic in ("sub", "or", "xor", "shl", "cmp"):
            final_flags[0] = decoded[ip].mnemonic
        if ip == 0x3287:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(entry, 2 * SIZE, count=1000)
    if not at_return[0]:
        raise ValueError(f"original did not return: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs, 0x3287), 2 * SIZE, count=1)
    if bytes(backing[entry:entry + END - ENTRY]) != expected_code:
        raise ValueError(f"immutable code changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u86_record_predicate_323d(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected state: {label}")
    actual, expected = bytes(ram), bytes(backing)
    if actual != expected:
        diffs = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        mask = 0xFFEF if name == "flags" and final_flags[0] == "or" else 0xFFFF
        if (getattr(cpu, name) ^ expected_word) & mask:
            raise ValueError(f"{name} differs {label}: {getattr(cpu, name):04X}/{expected_word:04X}")
    return visited


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    body, decoded, callers = source()
    visited, count = set(), 0
    with tempfile.TemporaryDirectory(prefix="poolrad-u86-323d-") as tmp:
        lib = library(Path(tmp))

        def run(label, index=5, value=12, **options):
            nonlocal visited, count
            memory, cpu = make_case(body, index, value, **options)
            visited |= compare(label, memory, cpu, decoded, lib)
            count += 1

        for value in range(256):
            run(f"table-byte-{value:02X}", value=value)
        for index in ((0, 127, 128, 255) if args.quick else range(256)):
            for value in (0, 10, 11, 12, 13, 14, 255):
                run(f"record-index-{index:02X}-table-{value:02X}", index, value)
        for record in ((0, 0), (0, 0x1000), (0x4000, 0), (0xFFFF, 0x10),
                       (0xFFFF, 0xFFE1), (0x4000, 0xFFDE), (0x4000, 0xFFFF)):
            for value in (0, 12, 255):
                run(f"record-{record}-{value}", 255, value, record=record)
        for ss in (0x1000, 0x2000, 0xFFFF):
            for sp in (0, 1, 2, 3, 4, 5, 0x5580, 0x5581, 0x5582, 0x5583, 0x5584,
                       0x5585, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):
                run(f"frame-{ss:04X}-{sp:04X}", 0, ss=ss, sp=sp)
        rng = random.Random(0x86323D)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}", rng.randrange(256), rng.randrange(256),
                ds=rng.choice((0, 0x1000, 0xF000, 0xFFFF)),
                flags=rng.choice((0x0202, 0x0ED7, 0x0047, 0x0A93)), background=rng.randbytes(SIZE))
    report = dict(target="OVL_0086_323D", source_sha256=BODY_SHA, cases=count,
                  covered_sites=len(visited), source_sites=len(decoded),
                  unvisited_sites=[f"{ip:04X}" for ip in sorted(set(decoded) - visited)],
                  compared_ram_bytes=SIZE, compared_cpu_words=14, far_callers=callers,
                  near_caller="OVL_0086_051C:+0572")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
