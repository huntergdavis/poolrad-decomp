#!/usr/bin/env python3
"""Original-byte full-state parity for the three callers of the u9E dice entries.

u2F +0131, u38 +2D38 and u33 +0000 reach unit 0x9E public entries 8 (+0DE3)
and 9 (+0E2E) through the overlay stubs 00B0:0048 and 00B0:004D; entry 8
calls SYSTEM 0AF8:11C0, whose seed step lives at 0AF8:120F. The differential
loads the original caller spans, the loaded-state stubs, the original unit
0x9E span and the original SYSTEM bytes into one 1 MiB image at base 0, runs
the original under Unicorn and the composed C (the merged entry-8/entry-9
chain on raw RAM) on a copy, and compares the entire RAM and all 14 CPU
words. Media stays private.
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

BASES = {0x2F: 0x9807, 0x38: 0xEC5B, 0x33: 0xC5D1, 0x9E: 0x2AEEA}
U9E_SPAN = (0x0DE3, 0x0E52)
U9E_SHA = "9a2681e8b24a9a95838a11f2d7cbddea0c253d29e631db37d25fae017a14693b"
SYSTEM_FILE = (0xBE36, 0xBEBD)     # START.EXE bytes of 0AF8:11C0..1246 (wrapper, seed step, multiplier)
SYSTEM_CS, SYSTEM_OFFSET = 0x0AF8, 0x11C0
STUB_PARAGRAPH = 0x00B0
STUBS = {0x48: 0x0DE3, 0x4D: 0x0E2E}
CALLER_CS, DICE_CS, DS, SS = 0x3000, 0x4000, 0x1000, 0x2000
TARGETS = {
    "0131": dict(unit=0x2F, entry=0x0131, end=0x01F6, insns=68, retf=0x01F3, pops=4, conditionals=5,
                 sha="", symbol="pr_u2f_party_pick_0131", sites=[0x015D], stub=0x48,
                 callers={(0x007A, 0x00FE), (0x007A, 0x0119)}, infeasible=set()),
    "2d38": dict(unit=0x38, entry=0x2D38, end=0x2D88, insns=29, retf=0x2D85, pops=10, conditionals=3,
                 sha="", symbol="pr_u38_record_roll_2d38", sites=[0x2D6A], stub=0x4D,
                 callers=None, infeasible=set()),
    "0000": dict(unit=0x33, entry=0x0000, end=0x012B, insns=123, retf=0x012A, pops=0, conditionals=14,
                 sha="", symbol="pr_u33_grid_roll_0000", sites=[0x004B, 0x00B7, 0x00C6], stub=0x48,
                 callers={(0x0810, 0x089D), (0x11B4, 0x1225)},
                 # a (1, 64h) roll is 1..100 and a (1, 0Ah) roll is 1..10, so after 1..61h, 62h and 63h are
                 # excluded only 64h remains, and a d10 outside 1..6 is 7..10: these five outcomes never occur
                 infeasible={(0x0058, 0x0061), (0x00A8, 0x0115), (0x00CD, 0x00F1), (0x00F3, 0x0115), (0x00F7, 0x0115)}),
}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    for unit, base in BASES.items():
        check(units[unit]["ovr_offset"] == base, f"unit {unit:#x} placement differs")
    check(units[0x9E]["memory_segment"] == STUB_PARAGRAPH and units[0x9E]["entries"][8] == 0x0DE3 and
          units[0x9E]["entries"][9] == 0x0E2E, "unit 0x9E entries 8/9 differ")
    check(units[0x38]["entries"][106] == 0x2D38, "unit 0x38 entry 106 differs")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    dice = ovr[BASES[0x9E] + U9E_SPAN[0]:BASES[0x9E] + U9E_SPAN[1]]
    check(sha256(dice).hexdigest() == U9E_SHA, "unit 0x9E +0DE3..+0E51 differs")
    system = exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]]
    check(sha256(system[:22]).hexdigest() == "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9" and
          sha256(system[0x4F:0x4F + 54]).hexdigest() == "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242",
          "SYSTEM +11C0/+120F bytes differ from the merged evidence")
    check(struct.unpack_from("<H", system, 0x85)[0] == 0x8405, "SYSTEM multiplier word differs")
    bodies = {}
    for key, target in TARGETS.items():
        unit = target["unit"]
        code = ovr[BASES[unit]:BASES[unit] + units[unit]["code_size"]]
        body = code[target["entry"]:target["end"]]
        decoded = list(decoder.disasm(body, target["entry"]))
        target["sha"] = sha256(body).hexdigest()
        check(len(decoded) == target["insns"] and decoded[-1].address == target["retf"] and
              decoded[-1].mnemonic == "retf" and (int(decoded[-1].op_str, 16) if decoded[-1].op_str else 0) == target["pops"] and
              decoded[-1].address + decoded[-1].size == target["end"], f"+{key} extent differs")
        sites = {i.address for i in decoded}
        calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        check(calls == [(s, "lcall", f"0xb0, {target['stub']:#x}") for s in target["sites"]], f"+{key} calls differ: {calls}")
        fixups = set(units[unit]["fixups"])
        for s in target["sites"]:
            check((s + 3) in fixups, f"+{key} far call segment word at +{s:04X} is not fixup-backed")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                       if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == target["conditionals"], f"+{key} conditional count differs: {len(conditional)}")
        for i in decoded:
            if i.mnemonic.startswith("j"):
                check(int(i.op_str, 16) in sites, f"+{key} branch leaves the body")
        if target["callers"] is not None:
            starts = sorted(f["rel"] for f in units[unit]["functions"])
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
        else:
            # entry 106: every fixup-backed far call to stub 0041:0272 (0x20 + 5*106)
            far = []
            for u in units.values():
                ucode = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
                for fixup in u["fixups"]:
                    site = fixup - 3
                    if site >= 0 and ucode[site:site + 5] == struct.pack("<BHH", 0x9A, 0x20 + 5 * 106, 0x41):
                        far.append((u["stub_para"], site))
            target["far_callers"] = sorted(far)
        bodies[key] = dict(body=body, decoded=decoded, conditional=conditional, **target)
    print("u2F +0131 (197/68), u38 +2D38 (80/29), u33 +0000 (299/123): fixup-backed LCALL 00B0:0048/004D; "
          f"u9E +0DE3..+0E51 {U9E_SHA[:16]}... and SYSTEM 0AF8:11C0..1246 ({len(system)} bytes) original")
    print("aligned callers:", {k: (sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"]) if v["callers"] else
                                   [f"0x{u:x}:@+{s:04X}" for u, s in v["far_callers"]]) for k, v in bodies.items()})
    return bodies, dict(dice=dice, system=system)


def library(directory):
    path = directory / "dice_callers.so"
    sources = ["overlay_u2f_party_pick_0131.c", "overlay_u38_record_roll_2d38.c", "overlay_u33_grid_roll_0000.c",
               "overlay_u9e_entry9_roll_latch_0e2e.c", "overlay_u9e_entry8_fullstate_compose_0de3.c",
               "overlay_u9e_entry8_fullstate_prefix_0de3.c", "overlay_u9e_entry8_fullstate_loop_0e05.c",
               "overlay_u9e_entry8_fullstate_postcall_0e13.c", "overlay_u9e_entry8_fullstate_tail_0e1f.c",
               "overlay_u9e_entry8_system_call_0e0e.c", "system_random_fullstate_11c0.c", "system_seed_fullstate_120f.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fPIC", "-shared",
                    "-I", str(ROOT / "include")] + [str(ROOT / "src/recovered" / s) for s in sources] + ["-o", str(path)],
                   check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for key, target in TARGETS.items():
        f = getattr(lib, target["symbol"])
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
        f.restype = ctypes.c_int
        functions[key] = f
    return functions


def fixture(bodies, code, key, *, seed=0x12345678, nodes=(), head=True, record=None, record_byte=0x56,
            latch=(0x11, 0, 0x22), map_id=5, grid=None, table=None, result_at=(0x5000, 0x0100),
            ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202, background=None, return_frame=(0x7000, 0x7001),
            node_base=(0x6000, 0x0000), sub_base=(0x7000, 0x0000), grid_at=(0x8000, 0x0000), frame_wrapped=False):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    target = TARGETS[key]
    for k, b in bodies.items():
        at = physical(CALLER_CS, b["entry"]); memory[at:at + len(b["body"])] = b["body"]
    at = physical(DICE_CS, U9E_SPAN[0]); memory[at:at + len(code["dice"])] = code["dice"]
    at = physical(SYSTEM_CS, SYSTEM_OFFSET); memory[at:at + len(code["system"])] = code["system"]
    for offset, entry in STUBS.items():
        at = physical(STUB_PARAGRAPH, offset); memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, DICE_CS)
    put16(memory, ds, 0x4428, seed & 0xFFFF); put16(memory, ds, 0x442A, seed >> 16)
    # +0131: party list head, nodes (+104/+106 link, +108/+10A sub pointer), sub bytes +3
    nseg, noff = node_base; sseg, soff = sub_base
    put16(memory, ds, 0x5D96, (noff if (nodes and head) else 0) & 0xFFFF)
    put16(memory, ds, 0x5D98, nseg if (nodes and head) else 0)
    for k, (sub_byte, nxt) in enumerate(nodes):
        at = (noff + k * 0x120) & 0xFFFF
        put16(memory, nseg, (at + 0x104) & 0xFFFF, ((noff + nxt * 0x120) & 0xFFFF) if nxt is not None else 0)
        put16(memory, nseg, (at + 0x106) & 0xFFFF, nseg if nxt is not None else 0)
        put16(memory, nseg, (at + 0x108) & 0xFFFF, (soff + k * 0x10) & 0xFFFF)
        put16(memory, nseg, (at + 0x10A) & 0xFFFF, sseg)
        memory[physical(sseg, (soff + k * 0x10 + 3) & 0xFFFF)] = sub_byte & 0xFF
    # +2D38: active record pointer, its +CC/+CE far pointer, that record's +2E byte, the latch bytes
    rec = record if record is not None else (0x6000, 0x0800)
    put16(memory, ds, 0x5D92, rec[1]); put16(memory, ds, 0x5D94, rec[0])
    if record_byte is not None:
        put16(memory, rec[0], (rec[1] + 0xCC) & 0xFFFF, 0x0900); put16(memory, rec[0], (rec[1] + 0xCE) & 0xFFFF, 0x7000)
        memory[physical(0x7000, 0x0900 + 0x2E)] = record_byte & 0xFF
    else:
        put16(memory, rec[0], (rec[1] + 0xCC) & 0xFFFF, 0); put16(memory, rec[0], (rec[1] + 0xCE) & 0xFFFF, 0)
    memory[physical(ds, 0x6818)] = latch[0] & 0xFF
    memory[physical(ds, 0x6819)] = latch[1] & 0xFF
    memory[physical(ds, 0x681C)] = latch[2] & 0xFF
    # +0000: grid far pointer, grid bytes, the four-stride DS table, map id
    gseg, goff = grid_at
    put16(memory, ds, 0x6716, goff); put16(memory, ds, 0x6718, gseg)
    for i in range(0x4E9):
        memory[physical(gseg, (goff + i) & 0xFFFF)] = (grid[i] if grid else (i * 7 + 3) & 0xFF)
    for i in range(0x400):
        memory[physical(ds, 0x2747 + i)] = (table[i] if table else (0x16 if i % 4 == 0 else 0x33))
    memory[physical(ds, 0x84DC)] = map_id & 0xFF
    words = [return_frame[0], return_frame[1]]
    if key == "0131":
        words += [result_at[1], result_at[0]]
    elif key == "2d38":
        words += [0x1111, 0x2222, 0x3333, 0x4444, 0x5555]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if not frame_wrapped:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CALLER_CS, target["entry"], ds, es, ss, flags)
    return memory, cpu


def compare(label, memory, cpu, bodies, key, function, budget=4000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    legal = [(physical(DICE_CS, U9E_SPAN[0]), physical(DICE_CS, U9E_SPAN[1])),
             (physical(SYSTEM_CS, SYSTEM_OFFSET), physical(SYSTEM_CS, SYSTEM_OFFSET + 0x85)),
             (physical(STUB_PARAGRAPH, 0x48), physical(STUB_PARAGRAPH, 0x52))]
    returned, previous, visited, edges = [False], [None], set(), set()
    conditional = target["conditional"]

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
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
        check(any(lo <= linear < hi for lo, hi in legal), f"left the legal code: {label} at {linear:#x}")

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction); original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return within {budget} instructions: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    chain = ctypes.c_int(-1)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(chain))
    check(status == 0, f"C status {status} (chain {chain.value}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases(key):
    rng = random.Random({"0131": 0x2F0131, "2d38": 0x382D38, "0000": 0x330000}[key])
    spec = []
    if key == "0131":
        bytes_sets = [(0,), (1,), (0x7F,), (0x80,), (0xFF,), (5, 5), (5, 6), (6, 5), (0x7F, 0x7F, 0x7F), (0, 0, 0),
                      (3, 0x80, 3, 0xFF), (1, 2, 3, 4), (4, 3, 2, 1), (0x40, 0x40, 0x40, 0x40)]
        for b, bs in enumerate(bytes_sets):
            for s in range(8):
                nodes = tuple((v, (i + 1) if i + 1 < len(bs) else None) for i, v in enumerate(bs))
                spec.append((f"list-{b}-{s}", dict(nodes=nodes, seed=rng.randrange(1 << 32))))
        spec.append(("empty", dict(nodes=())))
        spec.append(("head-null-with-nodes", dict(nodes=((5, None),), head=False)))
        for i in range(160):
            count = rng.randrange(5)
            nodes = tuple((rng.randrange(256), (n + 1) if n + 1 < count else None) for n in range(count))
            spec.append((f"random-{i}", dict(nodes=nodes, seed=rng.randrange(1 << 32), es=rng.randrange(0x10000),
                                            flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647, 0x0A93)))))
        three = ((9, 1), (9, 2), (0x7F, None))
        spec.append(("result-in-frame", dict(nodes=three, result_at=(SS, 0x8000 - 0x20))))
        spec.append(("result-on-argument", dict(nodes=three, result_at=(SS, 0x8004))))
        spec.append(("result-on-return-ip", dict(nodes=three, result_at=(SS, 0x8000))))
        spec.append(("result-on-last-node-sub", dict(nodes=three, result_at=(0x7000, 0x0020))))
        spec.append(("result-on-seed", dict(nodes=three, result_at=(DS, 0x4428))))
        spec.append(("nodes-under-stack", dict(nodes=three, node_base=(SS, 0x8000 - 0x400))))
        spec.append(("subs-a20", dict(nodes=three, sub_base=(0xFFFF, 0x0010))))
        spec.append(("nodes-offset-wrap", dict(nodes=three, node_base=(0x6000, 0xFF00))))
        spec.append(("cyclic-return-budget", dict(nodes=((5, 0),), expect_nonreturn=True)))
    elif key == "2d38":
        for present in (True, False):
            for rb in (0x56, 0x55, 0x57, 0, 0xFF):
                for lf in (0, 1, 8, 9, 0xF6, 0xFF):
                    for s in range(2):
                        spec.append((f"rec-{present}-{rb:02x}-{lf:02x}-{s}",
                                     dict(record_byte=rb if present else None, latch=(rng.randrange(256), lf, rng.randrange(256)),
                                          seed=rng.randrange(1 << 32))))
        for i in range(120):
            spec.append((f"random-{i}", dict(record_byte=rng.choice((0x56, 0x56, rng.randrange(256))),
                                            latch=(rng.randrange(256), rng.randrange(256), rng.randrange(256)),
                                            seed=rng.randrange(1 << 32), es=rng.randrange(0x10000),
                                            flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647, 0x0A93)))))
        spec.append(("record-null-pointer", dict(record=(0, 0), record_byte=None)))
        spec.append(("record-on-latch", dict(record=(DS, 0x6818 - 0xCC), record_byte=0x56)))
        spec.append(("record-under-stack", dict(record=(SS, 0x8000 - 0x100), record_byte=0x56)))
        spec.append(("latch-wrap", dict(latch=(0xF0, 9, 0x20))))
    else:
        for i in range(40):
            grid = [rng.randrange(256) for _ in range(0x4E9)]
            spec.append((f"random-{i}", dict(grid=grid, seed=rng.randrange(1 << 32), map_id=rng.choice((5, 10, 0)),
                                            table=[(0x16 if (j % 4 == 0 and rng.random() < 0.5) else rng.randrange(256)) for j in range(0x400)],
                                            flags=rng.choice((0x7202, 0x0202, 0x0ED7)))))
        for i in range(60):
            spec.append((f"all-eligible-{i}", dict(grid=[rng.randrange(256) for _ in range(0x4E9)], seed=rng.randrange(1 << 32),
                                                  map_id=rng.choice((5, 5, 10)))))
        spec.append(("none-eligible", dict(table=[0x17] * 0x400)))
        spec.append(("map-ten", dict(map_id=10, seed=0xDEADBEEF)))
        spec.append(("grid-a20", dict(grid_at=(0xFFFF, 0x0010))))
        spec.append(("grid-offset-wrap", dict(grid_at=(0x8000, 0xFE00))))
        spec.append(("grid-under-stack", dict(grid_at=(SS, 0x8000 - 0x600))))
        spec.append(("grid-over-seed", dict(grid_at=(DS, 0x4428 - 0x200))))
    base = dict()
    for sp in (0, 2, 4, 6, 8, 0xFFF0, 0xFFF4, 0xFFF8, 0xFFFC, 0xFFFE, 0xFFFF):
        spec.append((f"sp-{sp:04x}", dict(base, sp=sp, ss=0x1800, zero=True, nodes=((5, None),))))
    spec.append(("ss-a20", dict(base, ss=0xFFFF, sp=0x0040, nodes=((5, None),))))
    spec.append(("ds-a20", dict(base, ds=0xFFFF, nodes=((5, None),))))
    spec.append(("random-background", dict(base, seed_bg=3, nodes=((5, None),))))
    spec.append(("preserved-df", dict(base, flags=0x0603, nodes=((5, None),))))
    return spec


def differential(bodies, code, key, functions, quick=False):
    visited, edges, count, nonreturns = set(), set(), 0, 0
    target = bodies[key]
    for label, options in cases(key):
        if quick and ("random" in label or "eligible" in label) and int(label.split("-")[-1]) >= 8:
            continue
        options = dict(options)
        bg = options.pop("seed_bg", None); zero = options.pop("zero", False)
        expect_nonreturn = options.pop("expect_nonreturn", False)
        background = bytes(random.Random(bg).randbytes(SIZE)) if bg is not None else (bytes(SIZE) if zero else None)
        memory, cpu = fixture(bodies, code, key, background=background, **options)
        if expect_nonreturn:
            try:
                compare(f"+{key} {label}", memory, cpu, bodies, key, functions[key], budget=200000)
            except ValueError as error:
                check("did not return" in str(error), f"+{key} {label}: {error}")
                nonreturns += 1
                continue
            raise ValueError(f"+{key} {label}: the cyclic list returned")
        v, e = compare(f"+{key} {label}", memory, cpu, bodies, key, functions[key])
        visited |= v; edges |= e; count += 1
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited)
    missing_edges = {(s, o) for s, outs in target["conditional"].items() for o in outs} - edges
    check(not missing_sites, f"+{key} unvisited instruction starts: {[hex(x) for x in missing_sites]}")
    check(missing_edges == target["infeasible"], f"+{key} untaken conditional outcomes: {sorted((hex(a), hex(b)) for a, b in missing_edges)}")
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional "
          f"outcomes ({len(target['infeasible'])} infeasible with 1..range rolls); {nonreturns} directed cyclic fixture excluded")
    return dict(returning=count, nonreturning=nonreturns, infeasible_edges=sorted(f"{a:04X}->{b:04X}" for a, b in target["infeasible"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    bodies, code = source()
    report = dict(targets={k: dict(sha256=v["sha"], bytes=len(v["body"]), instructions=v["insns"]) for k, v in bodies.items()},
                  dice_span=U9E_SHA)
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-dice-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = {k: differential(bodies, code, k, functions, args.quick) for k in bodies}
            if args.native:
                from u33_grid_roll_0000_native import native_audit
                report["native"] = native_audit(ROOT, bodies, code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
