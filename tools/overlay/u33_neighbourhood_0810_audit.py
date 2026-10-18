#!/usr/bin/env python3
"""Original-byte full-state parity for u33 +0810 with every callee composed.

+0810 sweeps a 13 by 5 neighbourhood of the party position: per position it
calls +01AD for three facings (reaching +012B and the unit 0x3BD lookups through
stub paragraph 03D5) and the pattern bodies +02BA, +0377, +040F and +0615 (all
writing through +0221) with its frame as their static link; it ends with the
+0000 grid roll, which reaches unit 0x9E entry 8 through stub 00B0:0048 and the
SYSTEM generator at 0AF8:11C0. The differential loads the original unit 0x33
code, the unit 0x3BD and unit 0x9E spans, the SYSTEM generator and the
loaded-state stubs into one 1 MiB image, runs the original under Unicorn and the
composed C on a copy, and compares the entire RAM and all 14 CPU words.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

U33_BASE, U3BD_BASE, U9E_BASE = 0xC5D1, 0x2E8F7, 0x2AEEA
U3BD_SPAN, U9E_SPAN = (0x031F, 0x057A), (0x0DE3, 0x0E52)
SYSTEM_FILE, SYSTEM_CS, SYSTEM_OFFSET = (0xBE36, 0xBEBD), 0x0AF8, 0x11C0
STUB_3BD, STUB_9E = 0x03D5, 0x00B0
CALLER_CS, LOOKUP_CS, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
TARGET = dict(entry=0x0810, end=0x08A4, insns=71, retf=0x08A3, conditionals=2,
              calls=[(0x0852, "0x1ad"), (0x0864, "0x1ad"), (0x0876, "0x1ad"), (0x087E, "0x2ba"), (0x0883, "0x377"),
                     (0x0888, "0x40f"), (0x088D, "0x615"), (0x089D, "0")], callers={(0x1244, 0x12D4)})
SOURCES = ("overlay_u33_neighbourhood_0810", "overlay_u33_cell_pair_01ad", "overlay_u33_cell_lookup_012b",
           "overlay_u3bd_lookup_fullstate", "overlay_u3bd_in_grid_031f", "overlay_u33_stamp_block_02ba",
           "overlay_u33_stamp_square_0377", "overlay_u33_clipped_writer_0221", "overlay_u33_grid_pattern_040f",
           "overlay_u33_grid_pattern_0615", "overlay_u33_grid_roll_0000", "overlay_u9e_entry8_fullstate_compose_0de3",
           "overlay_u9e_entry8_fullstate_prefix_0de3", "overlay_u9e_entry8_fullstate_loop_0e05",
           "overlay_u9e_entry8_fullstate_postcall_0e13", "overlay_u9e_entry8_fullstate_tail_0e1f",
           "overlay_u9e_entry8_system_call_0e0e", "system_random_fullstate_11c0", "system_seed_fullstate_120f")


class Callee(ctypes.Structure):
    _fields_ = [("site", ctypes.c_uint16), ("status", ctypes.c_int)]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    check(units[0x33]["ovr_offset"] == U33_BASE and units[0x3BD]["ovr_offset"] == U3BD_BASE and
          units[0x9E]["ovr_offset"] == U9E_BASE, "unit placement differs")
    check(units[0x3BD]["memory_segment"] == STUB_3BD and units[0x9E]["memory_segment"] == STUB_9E and
          units[0x9E]["entries"][8] == 0x0DE3, "stub paragraphs differ")
    code = {0x33: ovr[U33_BASE:U33_BASE + units[0x33]["code_size"]],
            0x3BD: ovr[U3BD_BASE:U3BD_BASE + units[0x3BD]["code_size"]],
            0x9E: ovr[U9E_BASE:U9E_BASE + units[0x9E]["code_size"]],
            "system": exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]]}
    starts = sorted(f["rel"] for f in units[0x33]["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[0x33][TARGET["entry"]:TARGET["end"]]
    decoded = list(decoder.disasm(body, TARGET["entry"]))
    check(len(decoded) == TARGET["insns"] and decoded[-1].address == TARGET["retf"] and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "" and TARGET["end"] in starts, "+0810 extent differs")
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
    check(calls == TARGET["calls"], f"+0810 calls differ: {calls}")
    check(all(decoded[decoded.index(i) - 1].op_str == "cs" for i in decoded if i.mnemonic == "call"), "+0810 near call lacks PUSH CS")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == TARGET["conditionals"], "+0810 conditional count differs")
    near = set()
    for function in starts:
        end = next((x for x in starts if x > function), len(code[0x33]))
        aligned = {i.address: i for i in decoder.disasm(code[0x33][function:end], function)}
        for site, ins in aligned.items():
            if ins.mnemonic == "call" and ins.op_str.startswith("0x") and int(ins.op_str, 16) == TARGET["entry"]:
                check(aligned[site - 1].op_str == "cs", "+0810 caller lacks PUSH CS")
                near.add((function, site))
    check(near == TARGET["callers"], f"+0810 aligned near callers differ: {sorted(near)}")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), **TARGET)
    print(f"u33 +0810 ({len(body)}/{len(decoded)}, +01AD x3, +02BA, +0377, +040F, +0615, +0000) sha {target['sha'][:16]}...; "
          f"caller +1244@+12D4; SYSTEM {sha256(code['system']).hexdigest()[:16]}...")
    return target, code


def library(directory):
    import subprocess
    path = directory / "u33_0810.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / (s + ".c")) for s in SOURCES],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u33_neighbourhood_0810
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Callee)]
    f.restype = ctypes.c_int
    return f


def fixture(code, *, seed=1, ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202, grid_at=(0x6000, 0x0100),
            buffer_at=(0x5000, 0x0100), background=None, party=None, map_id=None, eligible=None, regs=None):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    def place(seg, off, blob):
        at = physical(seg, off); memory[at:at + len(blob)] = blob
    place(CALLER_CS, 0, code[0x33])
    place(LOOKUP_CS, U3BD_SPAN[0], code[0x3BD][U3BD_SPAN[0]:U3BD_SPAN[1]])
    place(LOOKUP_CS, U9E_SPAN[0], code[0x9E][U9E_SPAN[0]:U9E_SPAN[1]])
    place(SYSTEM_CS, SYSTEM_OFFSET, code["system"])
    for seg, off, entry in ((STUB_3BD, 0x2A, 0x047B), (STUB_3BD, 0x2F, 0x0350), (STUB_9E, 0x48, 0x0DE3), (STUB_9E, 0x4D, 0x0E2E)):
        place(seg, off, struct.pack("<BHH", 0xEA, entry, LOOKUP_CS))
    px, py = party if party is not None else (rng.randrange(-3, 14), rng.randrange(-3, 9))
    memory[physical(ds, 0x6AAD)] = px & 0xFF; memory[physical(ds, 0x6AAE)] = py & 0xFF
    memory[physical(ds, 0x84DC)] = (map_id if map_id is not None else rng.choice((0, 5, 10, rng.randrange(256)))) & 0xFF
    put16(memory, ds, 0x6A5C, buffer_at[1]); put16(memory, ds, 0x6A5E, buffer_at[0])
    put16(memory, ds, 0x6716, grid_at[1]); put16(memory, ds, 0x6718, grid_at[0])
    put16(memory, ds, 0x4428, rng.randrange(0x10000)); put16(memory, ds, 0x442A, rng.randrange(0x10000))
    for i in range(0x10A):
        memory[physical(ds, 0x2736 + i)] = rng.randrange(256)
    density = eligible if eligible is not None else rng.choice((0.0, 0.05, 0.3))
    for i in range(0x100):                                   # +0000 eligibility table, four-byte stride
        memory[physical(ds, 0x2747 + 4 * i)] = 0x16 if rng.random() < density else rng.choice((0x00, 0x15, 0x17, 0x33))
    for i in range(0x400):
        memory[physical(buffer_at[0], (buffer_at[1] + i) & 0xFFFF)] = rng.randrange(256)
    for i in range(0x4F0):
        memory[physical(grid_at[0], (grid_at[1] + i) & 0xFFFF)] = rng.randrange(256)
    for i, value in enumerate((0x7000, 0x7001)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        at = physical(ss, sp); memory[(at + 2 * i) & (SIZE - 1)] = value & 0xFF; memory[(at + 2 * i + 1) & (SIZE - 1)] = value >> 8
    r = regs or (0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777)
    cpu = Cpu(*r, sp, CALLER_CS, TARGET["entry"], ds, es, ss, flags)
    high = any(seg >= 0xF000 for seg in (ss, ds, grid_at[0], buffer_at[0]))
    return memory, cpu, high


def read_word(memory, segment, offset):
    at = physical(segment, offset & 0xFFFF)
    return memory[at] | (memory[(at + 1) & (SIZE - 1)] << 8)


def compare(label, memory, cpu, high, target, function, budget=40000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    returned, previous, visited, edges = [False], [None], set(), set()

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
        here = address - start + target["entry"]
        visited.add(here)
        if previous[0] in target["conditional"]:
            edges.add((previous[0], here))
        previous[0] = here
        if address == last:
            returned[0] = True

    def stop(emulator, address, size, user):
        del address, size, user
        if returned[0]:
            emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    # No conditional branch leaves the body and every call returns to the instruction after it, so edges
    # need no outside hook; one hook at the return CS:IP stops the run after the body's RETF.
    back = physical(read_word(memory, cpu.ss, cpu.sp + 2), read_word(memory, cpu.ss, cpu.sp))
    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, stop, begin=back, end=back)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return within {budget} instructions: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = Callee()
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee))
    check(status == 0, f"C status {status} (site +{callee.site:04X}, callee {callee.status}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(quick=False):
    rng = random.Random(0x330810)
    spec = []
    for i in range(12 if quick else 48):
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 30), flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647)),
                                        es=rng.randrange(0x10000), regs=tuple(rng.randrange(0x10000) for _ in range(7)))))
    for party in ((0, 0), (5, 3), (10, 5), (-6, -2), (16, 8), (0x7F, 0x7F), (-128, -128)):
        spec.append((f"party-{party[0]}-{party[1]}", dict(seed=900 + party[0] * 3 + party[1], party=party, eligible=0.2)))
    spec.append(("map-10", dict(seed=31, map_id=10, eligible=0.5)))
    spec.append(("all-eligible", dict(seed=32, eligible=1.0)))
    base = dict(seed=77, party=(5, 3))
    for sp in (0x0100, 0x0200, 0xFFE0, 0xFFF0):
        spec.append((f"sp-{sp:04x}", dict(base, sp=sp, ss=0x1800, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(base, ss=0xFFFF, sp=0x0400)))
    spec.append(("ds-a20", dict(base, ds=0xFFFF)))
    spec.append(("grid-a20", dict(base, grid_at=(0xFFFF, 0x0010))))
    spec.append(("grid-offset-wrap", dict(base, grid_at=(0x6000, 0xFE00))))
    # A grid in the stack segment below the deepest callee frame. A grid laid over +0810's own frame lets the
    # stamps rewrite its loop counters or return frames and the original then never returns; frame aliasing
    # that terminates is exercised in the +02BA/+0377 and +0615 audits.
    spec.append(("grid-in-stack-segment", dict(base, grid_at=(SS, 0x4000))))
    spec.append(("random-background", dict(base, background=bytes(random.Random(9).randbytes(SIZE)))))
    spec.append(("preserved-df", dict(base, flags=0x0603)))
    return spec


def differential(target, code, function, quick=False):
    visited, edges, count = set(), set(), 0
    for label, options in cases(quick):
        memory, cpu, high = fixture(code, **options)
        v, e = compare(f"+0810 {label}", memory, cpu, high, target, function)
        visited |= v; edges |= e; count += 1
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites and not missing_edges, f"+0810 coverage incomplete: {missing_sites} {missing_edges}")
    print(f"+0810: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return dict(returning=count, starts=len(visited), outcomes=len(edges))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=target["insns"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-0810-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = differential(target, code, function, args.quick)
            if args.native:
                from u33_neighbourhood_0810_native import native_audit
                report["native"] = native_audit(ROOT, target, code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
