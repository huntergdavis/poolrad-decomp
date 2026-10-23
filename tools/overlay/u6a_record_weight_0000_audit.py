#!/usr/bin/env python3
"""Original-byte full-state parity for u6A +0000 and +019C composed with u86 +323D.

Both callers reach unit 0x86 public entry 6 (+323D) through the overlay
stub 0096:003E. The differential loads the original caller bytes, the
loaded-state five-byte far-jump stub and the original callee bytes into
one 1 MiB image, runs the original under Unicorn and the composed C on a
copy, and compares the entire RAM and all 14 CPU words. Media stays private.
"""
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
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

UNIT_BASE = 0x17831
STUB_LINEAR = (0x0096 << 4) + 0x003E
CALLEE = dict(unit=0x86, entry=0x323D, end=0x328A, insns=29,
              sha="2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd")
TARGETS = {
    "0000": dict(entry=0x0000, end=0x0159, insns=121, retf=0x0156, call=0x008A,
                 sha="b361d881a02fc656fa6e5e7ad390b54724164d9f8eab5c751a3b57545da0760a",
                 symbol="pr_u6a_record_weight_0000", source="overlay_u6a_record_weight_0000.c",
                 callers={(0x077A, 0x07A1)}, conditionals=12),
    "019c": dict(entry=0x019C, end=0x0214, insns=42, retf=0x0211, call=0x01C3,
                 sha="d1c2b5b72d6abc696263f11879fa61be5a86c7ed7d4e0d040d9b2509cba34972",
                 symbol="pr_u6a_link_high_clear_019c", source="overlay_u6a_link_high_clear_019c.c",
                 callers={(0x0214, 0x0240), (0x0B5B, 0x0BC4), (0x0B5B, 0x0E25)}, conditionals=3),
}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    unit = next(u for u in units if u["stub_para"] == 0x6A)
    check(unit["ovr_offset"] == UNIT_BASE and unit["memory_segment"] == 0x75, "unit 0x6A placement differs")
    callee_unit = next(u for u in units if u["stub_para"] == 0x86)
    check(callee_unit["memory_segment"] == 0x96 and callee_unit["entries"][6] == CALLEE["entry"],
          "unit 0x86 entry 6 stub differs")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    callee = ovr[callee_unit["ovr_offset"] + CALLEE["entry"]:callee_unit["ovr_offset"] + CALLEE["end"]]
    callee_decoded = list(decoder.disasm(callee, CALLEE["entry"]))
    check(sha256(callee).hexdigest() == CALLEE["sha"] and len(callee_decoded) == CALLEE["insns"] and
          callee_decoded[-1].mnemonic == "retf" and callee_decoded[-1].op_str == "4", "callee differs")
    code = ovr[UNIT_BASE:UNIT_BASE + unit["code_size"]]
    fixups = set(unit["fixups"])
    starts = sorted(f["rel"] for f in unit["functions"])
    bodies = {}
    for key, target in TARGETS.items():
        body = code[target["entry"]:target["end"]]
        decoded = list(decoder.disasm(body, target["entry"]))
        check(sha256(body).hexdigest() == target["sha"], f"+{key} hash differs")
        check(len(decoded) == target["insns"] and decoded[-1].address == target["retf"] and
              decoded[-1].mnemonic == "retf" and decoded[-1].op_str == "4" and
              decoded[-1].address + decoded[-1].size == target["end"], f"+{key} extent differs")
        sites = {i.address for i in decoded}
        calls = [i for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        check(len(calls) == 1 and calls[0].address == target["call"] and calls[0].mnemonic == "lcall" and
              calls[0].op_str == "0x96, 0x3e" and (target["call"] + 3) in fixups, f"+{key} call differs")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                       if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == target["conditionals"], f"+{key} conditional count differs")
        for i in decoded:
            if i.mnemonic.startswith("j"):
                check(int(i.op_str, 16) in sites, f"+{key} branch leaves the body")
        near = set()
        for function in starts:
            end = next((x for x in starts if x > function), len(code))
            aligned = {i.address: i for i in decoder.disasm(code[function:end], function)}
            for site, ins in aligned.items():
                if ins.mnemonic != "call":
                    continue
                try:
                    destination = int(ins.op_str, 16)
                except ValueError:
                    continue
                if destination == target["entry"]:
                    check(aligned[site - 1].mnemonic == "push" and aligned[site - 1].op_str == "cs",
                          f"+{key} caller lacks PUSH CS")
                    near.add((function, site))
        check(near == target["callers"], f"+{key} aligned near callers differ: {sorted(near)}")
        bodies[key] = dict(body=body, decoded=decoded, conditional=conditional, **target)
    print(f"u6A +0000..+0158 ({len(bodies['0000']['body'])} bytes, 121 instructions) and "
          f"+019C..+0213 ({len(bodies['019c']['body'])} bytes, 42 instructions): one LCALL 0096:003E each, "
          f"fixup-backed, every branch internal; callee u86 +323D 77 bytes {CALLEE['sha'][:16]}...")
    print("aligned PUSH CS/CALL callers:",
          {k: sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"]) for k, v in bodies.items()})
    return bodies, callee


CHECKER_ROWS = ('OVL_006A_0000', 'OVL_006A_019C')


def library(directory):
    path = directory / "u6a.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u6a_record_weight_0000.c"),
                    str(ROOT / "src/recovered/overlay_u6a_link_high_clear_019c.c"),
                    str(ROOT / "src/recovered/overlay_u86_record_predicate_323d.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for key, target in TARGETS.items():
        f = getattr(lib, target["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[key] = f
    return functions


# Fixture: record item bytes, node list, two DS tables and placement.
def fixture(bodies, callee, key, *, items=(0,) * 21, nodes=(), head=0, table1=None, table2=None,
            cs=0x3000, cs2=0x4000, ds=0x1000, ss=0x2000, sp=0x8000, es=0x1111,
            record=(0x5000, 0x0100), node_base=(0x6000, 0x0000), node_stride=0x40,
            flags=0x7202, background=None, return_frame=(0x7000, 0x7001), seed=None,
            frame_wrapped=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    rng = random.Random(seed)
    # original code: both caller bodies at their unit offsets below CS, the stub, the callee
    for k, b in bodies.items():
        at = physical(cs, b["entry"])
        memory[at:at + len(b["body"])] = b["body"]
    at = physical(cs2, CALLEE["entry"])
    memory[at:at + len(callee)] = callee
    memory[STUB_LINEAR:STUB_LINEAR + 5] = struct.pack("<BHH", 0xEA, CALLEE["entry"], cs2)
    for i in range(128):
        value = table1[i] if table1 is not None else (i * 7) & 0xFF
        memory[physical(ds, 0x31FB + 16 * i)] = value
    for i in range(256):
        value = table2[i] if table2 is not None else i & 0x0F
        memory[physical(ds, 0x5582 + 16 * i)] = value
    rseg, roff = record
    for i, value in enumerate(items):
        memory[physical(rseg, (roff + 0x17 + i) & 0xFFFF)] = value
    nseg, noff = node_base
    def node_pointer(index):
        return (0, 0) if index is None else (nseg, (noff + index * node_stride) & 0xFFFF)
    hseg, hoff = node_pointer(head if nodes else None)
    put16(memory, rseg, (roff + 0xC8) & 0xFFFF, hoff)
    put16(memory, rseg, (roff + 0xCA) & 0xFFFF, hseg)
    for index, (table_index, bytes3, nxt) in enumerate(nodes):
        seg, off = node_pointer(index)
        memory[physical(seg, (off + 0x2E) & 0xFFFF)] = table_index
        for j, value in enumerate(bytes3):
            memory[physical(seg, (off + 0x3C + j) & 0xFFFF)] = value
        xs, xo = node_pointer(nxt)
        put16(memory, seg, (off + 0x2A) & 0xFFFF, xo)
        put16(memory, seg, (off + 0x2C) & 0xFFFF, xs)
    # Frame words sit where sixteen-bit pushes leave them. A word or LES operand that starts
    # at FFFE/FFFF is read from the physically contiguous bytes beyond the segment instead, so
    # unless frame_wrapped asks for the pushed layout alone, both placements hold the value.
    frame = physical(ss, sp)
    for delta, value in ((0, return_frame[0]), (2, return_frame[1]), (4, roff), (6, rseg)):
        put16(memory, ss, (sp + delta) & 0xFFFF, value)
        if not frame_wrapped:
            memory[(frame + delta) & (SIZE - 1)] = value & 0xFF
            memory[(frame + delta + 1) & (SIZE - 1)] = value >> 8
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, cs, bodies[key]["entry"],
              ds, es, ss, flags)
    del rng
    return memory, cpu


def compare(label, memory, cpu, bodies, key, function, budget=400000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"])
    body_end = start + len(target["body"])
    last = physical(cpu.cs, target["retf"])
    callee_start = STUB_LINEAR  # the stub and the callee are the only other legal code
    callee_body = (physical(0x4000, CALLEE["entry"]), physical(0x4000, CALLEE["end"]))
    returned, previous, visited, edges = [False], [None], set(), set()
    conditional = target["conditional"]

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop()
            return
        linear = address & (2 * SIZE - 1)
        if start <= linear < body_end:
            here = linear - start + target["entry"]
            visited.add(here)
            if previous[0] in conditional:
                edges.add((previous[0], here))
            previous[0] = here
            if linear == last:
                returned[0] = True
            return
        previous[0] = None
        check(linear == callee_start or callee_body[0] <= linear < callee_body[1],
              f"left the caller, stub and callee: {label} at {linear:#x}")

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction)
    original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return within {budget} instructions: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu))
    check(status == 0, f"C status {status}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(key):
    """Deterministic fixture specifications shared by the differential and the C side."""
    rng = random.Random(0x6A0000 if key == "0000" else 0x6A019C)
    spec = []
    high = lambda i: 0x80 | (i & 0x7F)
    low_items = tuple(rng.randrange(0x80) for _ in range(21))
    node = lambda t, b, n: (t, tuple(b), n)
    # empty list, no high bytes
    spec.append(("empty", dict(items=low_items)))
    # single positions
    for position in range(21):
        for index in (0, 1, 2, 3, 0x7F):
            items = list(low_items); items[position] = high(index)
            spec.append((f"item-{position}-{index}", dict(items=tuple(items))))
    # table value sweep through item 0 (table1[i] = i*7 & FF default)
    for index in range(128):
        items = list(low_items); items[0] = high(index); items[20] = high(index ^ 1)
        spec.append((f"table-{index}", dict(items=tuple(items))))
    # sum wrap and maxima
    spec.append(("all-high-wrap", dict(items=tuple(high(37) for _ in range(21)))))
    spec.append(("max-descending", dict(items=tuple(high(20 - i) for i in range(21)))))
    spec.append(("max-ascending", dict(items=tuple(high(i) for i in range(21)))))
    # lists: predicate index 11/12/13 true under the default table2 (i & 0xF)
    bytes_sets = [(0x80, 0x90, 0xA0), (0x7F, 0x00, 0xFF), (0x81, 0x7F, 0xFE), (0xFF, 0xFF, 0xFF), (0, 0, 0)]
    for count in (1, 2, 3):
        for predicate in ("all-true", "all-false", "mixed", "zero-index", "ff-index"):
            for b in range(len(bytes_sets)):
                nodes = []
                for n in range(count):
                    if predicate == "all-true": t = 11 + (n % 3)
                    elif predicate == "all-false": t = (10, 14, 0)[n % 3]
                    elif predicate == "mixed": t = (12, 3, 13)[n % 3]
                    elif predicate == "zero-index": t = 0
                    else: t = 0xFF
                    nodes.append(node(t, bytes_sets[(b + n) % len(bytes_sets)], n + 1 if n + 1 < count else None))
                spec.append((f"list-{count}-{predicate}-{b}", dict(items=low_items, nodes=tuple(nodes), head=0)))
    # random
    for i in range(160):
        items = tuple(rng.randrange(256) for _ in range(21))
        count = rng.randrange(4)
        nodes = tuple(node(rng.randrange(256), [rng.randrange(256) for _ in range(3)],
                           n + 1 if n + 1 < count else None) for n in range(count))
        table1 = [rng.randrange(256) for _ in range(128)]
        table2 = [rng.randrange(256) for _ in range(256)]
        spec.append((f"random-{i}", dict(items=items, nodes=nodes, head=0, table1=table1, table2=table2,
                                        flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647, 0x0A93)),
                                        es=rng.randrange(0x10000))))
    one = (node(12, (0x85, 0x7F, 0x90), None),)
    three = (node(12, (0x85, 0x7F, 0x90), 1), node(5, (0x80, 0x80, 0x80), 2), node(13, (0xFF, 0x01, 0x82), None))
    base_items = tuple(high(3) if i % 4 == 0 else 0x11 for i in range(21))
    # stack boundaries and A20
    for sp in (0, 2, 4, 6, 8, 0xFFF0, 0xFFF2, 0xFFF4, 0xFFF6, 0xFFF8, 0xFFFA, 0xFFFC, 0xFFFE, 0xFFFF, 0x0001, 0x0003):
        # A frame at either end of the segment spills into the neighbouring paragraphs, and a
        # pointer local that straddles FFFF is written with the offset wrap but read by LES from
        # the contiguous bytes: the original then walks whatever those bytes name. A zero
        # background and a stack segment clear of code keep that walk finite in both engines.
        spec.append((f"sp-{sp:04x}", dict(items=base_items, nodes=three, head=0, sp=sp, ss=0x1800, zero=True)))
    spec.append(("ss-a20", dict(items=base_items, nodes=three, head=0, ss=0xFFFF, sp=0x0020)))
    spec.append(("ds-a20", dict(items=base_items, nodes=three, head=0, ds=0xFFFF)))
    spec.append(("record-a20", dict(items=base_items, nodes=three, head=0, record=(0xFFFF, 0x0010))))
    spec.append(("record-offset-wrap", dict(items=base_items, nodes=three, head=0, record=(0x5000, 0xFFF0))))
    spec.append(("node-offset-wrap", dict(items=base_items, nodes=three, head=0, node_base=(0x6000, 0xFFE0))))
    spec.append(("node-a20", dict(items=base_items, nodes=three, head=0, node_base=(0xFFFF, 0x0100))))
    # aliases: record over the return frame and locals, nodes under the stack, DS table in the frame
    spec.append(("record-return-ip-alias", dict(items=base_items, nodes=one, head=0, record=(0x2000, 0x8000 - 0x2C))))
    spec.append(("record-locals-alias", dict(items=base_items, nodes=one, head=0, record=(0x2000, 0x8000 - 0x17 - 0x0E))))
    spec.append(("record-argument-alias", dict(items=base_items, nodes=one, head=0, record=(0x2000, 0x8000 + 4 - 0x2C))))
    spec.append(("node-under-stack", dict(items=base_items, nodes=three, head=0, node_base=(0x2000, 0x8000 - 0x60))))
    # the node's +2E index byte is the callee's own [BP-1] local: read by the predicate before the
    # callee writes its result there; the node's link words lie just below that frame and stay zero
    spec.append(("node-in-callee-frame", dict(items=base_items, nodes=one, head=0,
                                              node_base=(0x2000, (0x7FE5 if key == "0000" else 0x7FED) - 0x2E))))
    spec.append(("table1-in-frame", dict(items=base_items, nodes=one, head=0, ds=0x2000, ss=0x2000, sp=0x31FB + 16 * 3 + 2)))
    spec.append(("table2-in-frame", dict(items=base_items, nodes=one, head=0, ds=0x2000, ss=0x2000, sp=0x5582 + 16 * 12 + 1)))
    spec.append(("record-is-node", dict(items=base_items, nodes=(), head=0)))  # head patched below
    spec.append(("head-offset-only", dict(items=base_items, nodes=one, head=0, node_base=(0, 0x0040))))
    spec.append(("head-segment-only", dict(items=base_items, nodes=one, head=0, node_base=(0x6000, 0))))
    spec.append(("random-background", dict(items=base_items, nodes=three, head=0, seed=7)))
    # a frame placed by sixteen-bit pushes at the top of the segment: the LES reads the record
    # segment from the contiguous bytes beyond the segment, here a zero background (null head)
    spec.append(("wrapped-frame-zero-background", dict(items=base_items, nodes=one, head=0, sp=0xFFFA, ss=0x1800,
                                                        frame_wrapped=True, zero=True)))
    spec.append(("wrapped-frame-ffff", dict(items=base_items, nodes=one, head=0, sp=0xFFFF, ss=0x1800,
                                            frame_wrapped=True, zero=True)))
    # a node linked to itself never returns in DOS; the original fails the budget and C is not run
    spec.append(("cyclic-list-return-budget", dict(items=base_items, nodes=(node(12, (0x80, 0, 0), 0),), head=0,
                                                   expect_nonreturn=True)))
    return spec


def differential(bodies, callee, key, functions, quick=False):
    visited, edges, count, nonreturns = set(), set(), 0, 0
    target = bodies[key]
    for label, options in cases(key):
        if quick and label.startswith("random-") and int(label.split("-")[1]) >= 20:
            continue
        options = dict(options)
        seed = options.pop("seed", None)
        zero = options.pop("zero", False)
        expect_nonreturn = options.pop("expect_nonreturn", False)
        background = bytes(random.Random(seed).randbytes(SIZE)) if seed is not None else (bytes(SIZE) if zero else None)
        memory, cpu = fixture(bodies, callee, key, background=background, **options)
        if label == "record-is-node":
            rseg, roff = options.get("record", (0x5000, 0x0100))
            put16(memory, rseg, roff + 0xC8, roff)
            put16(memory, rseg, roff + 0xCA, rseg)
            memory[physical(rseg, roff + 0x2E)] = 12
            put16(memory, rseg, roff + 0x2A, 0)
            put16(memory, rseg, roff + 0x2C, 0)
        if label == "head-segment-only":
            put16(memory, 0x5000, 0x01C8, 0)
            put16(memory, 0x5000, 0x01CA, 0x6000)
        if expect_nonreturn:
            try:
                compare(f"+{key} {label}", memory, cpu, bodies, key, functions[key], budget=100000)
            except ValueError as error:
                check("did not return" in str(error), f"+{key} {label}: {error}")
                nonreturns += 1
                continue
            raise ValueError(f"+{key} {label}: the cyclic list returned")
        v, e = compare(f"+{key} {label}", memory, cpu, bodies, key, functions[key])
        visited |= v
        edges |= e
        count += 1
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges)
    check(not missing_sites, f"+{key} unvisited instruction starts: {[hex(x) for x in missing_sites]}")
    check(not missing_edges, f"+{key} untaken conditional outcomes: {[(hex(a), hex(b)) for a, b in missing_edges]}")
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, "
          f"{len(edges)}/{2 * len(target['conditional'])} conditional outcomes; "
          f"{nonreturns} directed cyclic fixture failed the original return budget, C not invoked")
    return dict(returning=count, nonreturning=nonreturns)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix="poolrad-checker-") as temp:
            library(Path(temp))
        print("CHECKER_BUILD_OK")
        return
    bodies, callee = source()
    report = dict(targets={k: dict(sha256=v["sha"], bytes=len(v["body"]), instructions=v["insns"]) for k, v in bodies.items()},
                  callee=CALLEE["sha"])
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u6a-") as temp:
            functions = library(Path(temp))
            from loaded_stub_checks import verify_refusals
            report["loaded_stub_guards"] = {}
            for key in bodies:
                memory, cpu = fixture(bodies, callee, key)
                report["loaded_stub_guards"][key] = verify_refusals(
                    memory, cpu, lambda ram, state: functions[key](ram, SIZE, ctypes.byref(state)),
                    [(cpu.cs, bodies[key]["call"])], check_target_cs=False)
            if args.unicorn:
                report["unicorn_cases"] = {k: differential(bodies, callee, k, functions, args.quick) for k in bodies}
            if args.native:
                from u6a_record_weight_0000_native import native_audit
                report["native"] = native_audit(ROOT, bodies, callee, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
