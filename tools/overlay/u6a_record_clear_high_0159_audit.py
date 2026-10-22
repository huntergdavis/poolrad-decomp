#!/usr/bin/env python3
"""Pinned U6A +0159 source, caller and complete raw-state differential audit."""

import argparse
from collections import deque
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_IP

from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x0159, 0x019C
BODY_SHA = "a130919171e4e4b45af9155e9cd7e16abc1027eb83e11fbdf801fca7a0d3c099"


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(exe) != 65231 or sha256(exe).hexdigest() != EXE_SHA:
        raise ValueError("START.EXE provenance differs")
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("GAME.OVR provenance differs")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    unit = next(u for u in units if u["stub_para"] == 0x6A)
    if unit["ovr_offset"] != 0x17831 or unit["memory_segment"] != 0x75 or unit["entries"][1] != 0x1E6F:
        raise ValueError("U6A placement/public entry differs")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    body = code[ENTRY:END]
    dec = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    if len(body) != 67 or sha256(body).hexdigest() != BODY_SHA or len(dec) != 24 or sum(i.size for i in dec) != 67:
        raise ValueError("U6A leaf extent differs")
    decoded = {i.address: i for i in dec}
    if dec[-1].address != 0x0199 or dec[-1].mnemonic != "retf" or dec[-1].op_str != "4":
        raise ValueError("leaf return differs")
    if any(ENTRY <= p < END for p in unit["fixups"]):
        raise ValueError("leaf gained a relocation")
    for i in dec:
        if "call" in i.mnemonic or i.mnemonic.startswith("int"):
            raise ValueError("leaf gained an external dependency")
        if i.mnemonic.startswith("j") and int(i.op_str, 16) not in decoded:
            raise ValueError("branch leaves leaf")
    for start, end, sites in ((0x0214, 0x025C, (0x0236,)), (0x090A, 0x0AB9, (0x0973, 0x0AA7))):
        caller = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[start:end], start)}
        for site in sites:
            if caller[site].mnemonic != "call" or caller[site].op_str != "0x159" or caller[site-1].mnemonic != "push" or caller[site-1].op_str != "cs":
                raise ValueError("aligned near caller differs")
    active = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[0x096A:0x0976], 0x096A)}
    if active[0x096A].op_str != "word ptr [0x5d94]" or active[0x096E].op_str != "word ptr [0x5d92]":
        raise ValueError("active-record argument source differs")
    pending, seen, reached = deque([(0x1E6F, [])]), set(), None
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    while pending:
        at, path = pending.popleft()
        if at in seen or not 0 <= at < len(code):
            continue
        seen.add(at)
        if at == ENTRY:
            reached = path
            break
        ins = next(decoder.disasm(code[at:at+15], at, count=1), None)
        if ins is None or ins.mnemonic.startswith("ret") or ins.mnemonic in ("int", "iret"):
            continue
        if ins.mnemonic == "call":
            pending.append((int(ins.op_str, 16), path + [(at, int(ins.op_str, 16))]))
        if ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"):
            try:
                pending.append((int(ins.op_str, 16), path))
            except ValueError:
                pass
            if ins.mnemonic == "jmp":
                continue
        pending.append((at+ins.size, path))
    if reached != [(0x1ECA, 0x0214), (0x0236, ENTRY)]:
        raise ValueError(f"public-entry structural call chain differs: {reached}")
    return body, decoded, dict(caller_suffix=code[0x096A:0x0976], public_call_chain=reached)


CHECKER_ROWS = ('OVL_006A_0159',)


def library(directory):
    path = directory / "u6a_0159.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u6a_record_clear_high_0159.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u6a_record_clear_high_0159.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u6a_record_clear_high_0159.restype = ctypes.c_int
    return lib


def make_case(body, values, *, record=(0x4000, 0x1000), ss=0x2000, sp=0x8000,
              flags=0x0202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0xF000, ENTRY, 0x1000, 0x1111, ss, flags)
    at = physical(cpu.cs, ENTRY)
    memory[at:at+len(body)] = body
    segment, offset = record
    for i, value in enumerate(values):
        memory[physical(segment, (offset + 0x17 + i) & 0xFFFF)] = value
    at = physical(ss, sp)
    for i, b in enumerate((0x12, 7, cpu.cs & 255, cpu.cs >> 8)):
        memory[(at+i) & (SIZE-1)] = b
    put16(memory, ss, (sp+4) & 0xFFFF, offset)
    put16(memory, ss, (sp+6) & 0xFFFF, segment)
    return memory, cpu


def compare(label, memory, cpu, decoded, lib):
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        original.reg_write(reg, getattr(cpu, name))
    entry = physical(cpu.cs, ENTRY)
    expected_code = memory[entry:entry+END-ENTRY]
    visited, edges, previous, at_return = set(), set(), [None], [False]

    def on_instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry+END-ENTRY or ip not in decoded:
            raise ValueError(f"unexpected control transfer: {label}")
        if previous[0] in (0x0177, 0x018C):
            edges.add((previous[0], ip))
        previous[0] = ip
        visited.add(ip)
        if ip == 0x0199:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(entry, 2*SIZE, count=2000)
    if not at_return[0]:
        raise ValueError(f"original did not return within bound: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs, 0x0199), 2*SIZE, count=1)
    if bytes(backing[entry:entry+END-ENTRY]) != expected_code:
        raise ValueError(f"immutable code changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u6a_record_clear_high_0159(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected input: {label}")
    actual, expected = bytes(ram), bytes(backing)
    if actual != expected:
        diffs = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        if getattr(cpu, name) != expected_word:
            raise ValueError(f"{name} differs {label}: {getattr(cpu,name):04X}/{expected_word:04X}")
    return visited, edges


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
    body, decoded, metadata = source()
    visited, edges, count = set(), set(), 0
    with tempfile.TemporaryDirectory(prefix="poolrad-u6a-0159-") as tmp:
        lib = library(Path(tmp))

        def run(label, values, **options):
            nonlocal visited, edges, count
            memory, cpu = make_case(body, values, **options)
            sites, branches = compare(label, memory, cpu, decoded, lib)
            visited |= sites
            edges |= branches
            count += 1

        for value in range(256):
            run(f"uniform-{value:02X}", [value]*21)
        for position in range(21):
            for value in (0, 1, 0x7E, 0x7F, 0x80, 0xFF):
                values = [0x81]*21
                values[position] = value
                run(f"position-{position}-{value:02X}", values)
        # The list/final clear may overlap local, saved BP, return or pointer
        # arguments. Later LES instructions must see actual changed RAM.
        for offset in range(0x7FC0, 0x800D, 4 if args.quick else 1):
            run(f"record-frame-{offset:04X}", [0xFF]*21, record=(0x2000, offset))
        for ss in (0, 0x2000, 0xFFFF):
            for sp in (0, 1, 2, 3, 4, 5, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):
                run(f"frame-wrap-{ss:04X}-{sp:04X}", [0x80]*21, ss=ss, sp=sp)
        for segment in (0, 0x4000, 0xFFFF):
            for offset in (0, 1, 0xFFCB, 0xFFD4, 0xFFE8, 0xFFFF):
                run(f"record-wrap-{segment:04X}-{offset:04X}", [0xFF]*21, record=(segment, offset))
        rng = random.Random(0x6A0159)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}", [rng.randrange(256) for _ in range(21)],
                flags=rng.choice((0x0202, 0x0ED7, 0x0047, 0x0A93)), background=rng.randbytes(SIZE))
    required_edges = {(0x0177, 0x0179), (0x0177, 0x0188), (0x018C, 0x0165), (0x018C, 0x018E)}
    if visited != set(decoded) or edges != required_edges:
        raise ValueError("instruction or conditional-edge coverage incomplete")
    report = dict(target="OVL_006A_0159", source_sha256=BODY_SHA, cases=count,
                  covered_sites=len(visited), source_sites=len(decoded), conditional_edges=len(edges),
                  compared_ram_bytes=SIZE, compared_cpu_words=14, masked_flags=0,
                  aligned_near_callers=["0214:+0236", "090A:+0973", "090A:+0AA7"],
                  public_call_chain=metadata["public_call_chain"], calls_assumed_to_return=True,
                  natural_game_route_measured=False)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2)+"\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
