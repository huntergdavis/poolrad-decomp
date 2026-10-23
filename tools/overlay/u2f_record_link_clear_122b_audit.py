#!/usr/bin/env python3
"""Pinned U2F +122B caller/source and full-RAM/CPU differential audit."""

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
from unicorn.x86_const import UC_X86_REG_IP

from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END = 0x122B, 0x127A
BODY_SHA = "ef58c3eedf096f21b9b923c9c303dfc629e5f0935d2673dabf93e1bc9749de9a"


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    if len(exe) != 65231 or sha256(exe).hexdigest() != EXE_SHA:
        raise ValueError("START.EXE provenance differs")
    if len(ovr) != 206105 or sha256(ovr).hexdigest() != OVR_SHA:
        raise ValueError("GAME.OVR provenance differs")
    unit = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x2F)
    if unit["ovr_offset"] != 38919 or unit["memory_segment"] != 0x35:
        raise ValueError("U2F placement differs")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    body = code[ENTRY:END]
    dec = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    if len(body) != 79 or sha256(body).hexdigest() != BODY_SHA or len(dec) != 23 or sum(i.size for i in dec) != 79:
        raise ValueError("U2F leaf extent differs")
    decoded = {i.address: i for i in dec}
    if dec[-1].address != 0x1277 or dec[-1].mnemonic != "retf" or dec[-1].op_str != "4":
        raise ValueError("leaf return differs")
    if any(ENTRY <= p < END for p in unit["fixups"]):
        raise ValueError("leaf gained a relocation")
    for i in dec:
        if "call" in i.mnemonic or i.mnemonic.startswith("int"):
            raise ValueError("leaf gained an external dependency")
        if i.mnemonic.startswith("j") and int(i.op_str, 16) not in decoded:
            raise ValueError("branch leaves leaf")
    caller = {i.address: i for i in Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[0x031A:0x0666], 0x031A)}
    for site in (0x0465, 0x055A):
        if caller[site].mnemonic != "call" or caller[site].op_str != "0x122b" or caller[site-1].mnemonic != "push" or caller[site-1].op_str != "cs":
            raise ValueError("aligned near caller differs")
    expected = {0x053E:"ax, word ptr [0x5d96]", 0x0541:"dx, word ptr [0x5d98]",
                0x0553:"word ptr [bp - 4]", 0x0556:"word ptr [bp - 6]",
                0x055D:"di, ptr [bp - 6]", 0x0560:"ax, word ptr es:[di + 0x104]",
                0x0565:"dx, word ptr es:[di + 0x106]"}
    if any(caller[ip].op_str != operands for ip, operands in expected.items()):
        raise ValueError("party-record caller dataflow differs")
    return body, decoded, dict(caller_suffix=code[0x0553:0x055D])


CHECKER_ROWS = ('OVL_002F_122B',)


def library(directory):
    path = directory / "u2f_122b.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-fPIC", "-shared", "-I", str(ROOT / "include"),
        str(ROOT / "src/recovered/overlay_u2f_record_link_clear_122b.c"), "-o", str(path)],
        check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u2f_record_link_clear_122b.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u2f_record_link_clear_122b.restype = ctypes.c_int
    return lib


def make_case(body, left=7, right=7, *, record=(0x4000, 0x1000), child=(0x5000, 0x2000),
              target=(0x6000, 0x3000), ss=0x2000, sp=0x8000, flags=0x0202, background=None):
    memory = bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
              0x7777, sp, 0xF000, ENTRY, 0x1000, 0x1111, ss, flags)
    at = physical(cpu.cs, ENTRY)
    memory[at:at+len(body)] = body
    rs, ro = record
    cs, co = child
    ts, to = target
    put16(memory, rs, (ro+0x108) & 0xFFFF, co)
    # LES reads the segment contiguous with the offset, including at FFFF.
    at = (physical(rs, (ro+0x108) & 0xFFFF)+2) & (SIZE-1)
    memory[at], memory[(at+1) & (SIZE-1)] = cs & 255, cs >> 8
    put16(memory, cs, (co+0x0A) & 0xFFFF, to)
    put16(memory, cs, (co+0x0C) & 0xFFFF, ts)
    memory[physical(rs, (ro+0x10E) & 0xFFFF)] = right
    memory[physical(ts, (to+0x10E) & 0xFFFF)] = left
    at = physical(ss, sp)
    for i, b in enumerate((0x12, 7, cpu.cs & 255, cpu.cs >> 8)):
        memory[(at+i) & (SIZE-1)] = b
    put16(memory, ss, (sp+4) & 0xFFFF, ro)
    at = (physical(ss, (sp+4) & 0xFFFF)+2) & (SIZE-1)
    memory[at], memory[(at+1) & (SIZE-1)] = rs & 255, rs >> 8
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
    visited, edges, previous, final, at_return = set(), set(), [None], [None], [False]

    def on_instruction(uc, address, size, user):
        del size, user
        ip = uc.reg_read(UC_X86_REG_IP)
        if not entry <= address < entry+END-ENTRY or ip not in decoded:
            raise ValueError(f"unexpected control transfer: {label}")
        if previous[0] in (0x1247, 0x1262):
            edges.add((previous[0], ip))
        previous[0] = ip
        visited.add(ip)
        if decoded[ip].mnemonic in ("or", "xor", "cmp"):
            final[0] = decoded[ip].mnemonic
        if ip == 0x1277:
            at_return[0] = True
            uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, on_instruction)
    original.emu_start(entry, 2*SIZE, count=100)
    if not at_return[0]:
        raise ValueError(f"original did not return within bound: {label}")
    original.hook_del(hook)
    original.emu_start(physical(cpu.cs, 0x1277), 2*SIZE, count=1)
    if bytes(backing[entry:entry+END-ENTRY]) != expected_code:
        raise ValueError(f"immutable code changed: {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if lib.pr_u2f_record_link_clear_122b(ram, SIZE, ctypes.byref(cpu)):
        raise ValueError(f"C rejected input: {label}")
    actual, expected = bytes(ram), bytes(backing)
    if actual != expected:
        diffs = [i for i, (a,b) in enumerate(zip(actual,expected)) if a != b]
        raise ValueError(f"RAM differs {label}: {diffs[:8]}")
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        expected_word = original.reg_read(reg) & 0xFFFF
        mask = 0xFFEF if name == "flags" and final[0] in ("or", "xor") else 0xFFFF
        if (getattr(cpu,name) ^ expected_word) & mask:
            raise ValueError(f"{name} differs {label}: {getattr(cpu,name):04X}/{expected_word:04X}")
    return visited, edges, final[0]


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
    body, decoded, _ = source()
    visited, edges, count, exits = set(), set(), 0, dict(or_=0, cmp=0, xor=0)
    with tempfile.TemporaryDirectory(prefix="poolrad-u2f-122b-") as tmp:
        lib = library(Path(tmp))

        def run(label, left=7, right=7, **options):
            nonlocal visited, edges, count
            memory, cpu = make_case(body, left, right, **options)
            sites, branches, final = compare(label, memory, cpu, decoded, lib)
            visited |= sites
            edges |= branches
            exits["or_" if final == "or" else final] += 1
            count += 1

        for value in range(256):
            run(f"equal-{value}", value, value)
            run(f"unequal-next-{value}", value, (value+1) & 255)
            run(f"unequal-sign-{value}", value, value ^ 0x80)
        for left in (0, 1, 15, 16, 127, 128, 255):
            for right in (0, 1, 15, 16, 127, 128, 255):
                run(f"boundary-{left}-{right}", left, right)
        for target in ((0,0), (0,1), (1,0), (0,0xFFFF), (0xFFFF,0), (0xFFFF,0xFFFF)):
            run(f"target-{target}", target=target)
        for offset in range(0x7ED0, 0x7F10, 4 if args.quick else 1):
            run(f"record-frame-{offset:04X}", record=(0x2000,offset))
        for offset in range(0x7FE0, 0x8009, 4 if args.quick else 1):
            run(f"child-frame-{offset:04X}", child=(0x2000,offset))
        for ss in (0, 0x2000, 0xFFFF):
            for sp in (0,1,2,3,4,5,0xFFF8,0xFFF9,0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
                run(f"frame-wrap-{ss:04X}-{sp:04X}", ss=ss, sp=sp)
        for segment in (0, 0x4000, 0xFFFF):
            for offset in (0,1,0xFEF0,0xFEF7,0xFFF3,0xFFF6,0xFFFF):
                run(f"record-wrap-{segment:04X}-{offset:04X}", record=(segment,offset))
                run(f"child-wrap-{segment:04X}-{offset:04X}", child=(segment,offset))
        for child, target in (((0x4000,0x1000),(0x4000,0x1000)),
                              ((0x4000,0x1104),(0x4000,0x1000)),
                              ((0x5000,0x2000),(0x5000,0x2000))):
            run(f"record-chain-alias-{child}-{target}", child=child, target=target)
        rng = random.Random(0x2F122B)
        for index in range(8 if args.quick else 128):
            run(f"seeded-{index}", rng.randrange(256), rng.randrange(256),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)), background=rng.randbytes(SIZE))
    required = {(0x1247,0x1276),(0x1247,0x1249),(0x1262,0x1276),(0x1262,0x1264)}
    if visited != set(decoded) or edges != required or any(n == 0 for n in exits.values()):
        raise ValueError("instruction/conditional-edge/exit coverage incomplete")
    report = dict(target="OVL_002F_122B", source_sha256=BODY_SHA, cases=count,
                  covered_sites=len(visited), source_sites=len(decoded), conditional_edges=len(edges),
                  compared_ram_bytes=SIZE, compared_cpu_words=14,
                  undefined_af_masked_on_logic_exits=True, cmp_flags_masked=0, exits=exits,
                  aligned_near_callers=["031A:+0465","031A:+055A"], natural_game_route_measured=False)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    main()
