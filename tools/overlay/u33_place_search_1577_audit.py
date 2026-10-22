#!/usr/bin/env python3
"""Original-byte full-state parity for u33 +1434 and +1577 with their callees composed.

+1577 calls +1403 and +01AD (which calls +012B and the unit 0x3BD lookups
through stub paragraph 03D5) and +1434, which far-calls unit 0x3C6 public
entry 16 (+0CB9, with its folded entries 15, 1 and 6) through the stub at
03E0:0070. The differential loads the original unit 0x33 code, the unit
0x3BD span, the three unit 0x3C6 spans and the loaded-state stubs into one
1 MiB image, runs the original under Unicorn and the composed C on a copy,
and compares the entire RAM and all 14 CPU words. Media stays private.
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

U33_BASE, U3BD_BASE, U3C6_BASE = 0xC5D1, 0x2E8F7, 0x3073A
U3BD_SPAN = (0x031F, 0x057A)
U3C6_SPANS = ((0x0000, 0x0068), (0x04BE, 0x052B), (0x0C5F, 0x0E11))
U33_SPANS = ((0x012B, 0x0221), (0x1403, 0x19F5))
STUB_3BD, STUB_3C6 = 0x03D5, 0x03E0
STUBS_3BD = {0x2F: 0x0350, 0x2A: 0x047B}
CALLER_CS, LOOKUP_CS, SCAN_CS, DS, SS = 0x3000, 0x4000, 0x6000, 0x1000, 0x2000
TARGETS = {
    "1434": dict(entry=0x1434, end=0x1577, insns=138, retf=0x1574, pops=0x0C, conditionals=8, args=6,
                 symbol="pr_u33_cell_claim_1434", calls=[(0x150E, "lcall", "0x3e0, 0x70")], callers={(0x1577, 0x19C7)},
                 infeasible=set(), unreachable=set()),
    "1577": dict(entry=0x1577, end=0x19F5, insns=470, retf=0x19F2, pops=0x02, conditionals=35, args=1,
                 symbol="pr_u33_place_search_1577",
                 calls=[(0x17C8, "call", "0x1403"), (0x1894, "call", "0x1ad"), (0x18C8, "call", "0x1403"),
                        (0x1950, "call", "0x1ad"), (0x19C7, "call", "0x1434")], callers={(0x19F5, 0x1C75)},
                 # [BP-7] is 1, 2 or 3 at the dispatch and 2 or 3 at +17B0: it starts at 1, the three states write 2/3,
                 # +18AE writes 1, and +18D6's 0 either becomes 1 at +1997 or ends the search through [BP-4]. Only a
                 # DS/SS-aliased table write from a callee could change it, so these are declared, not reached.
                 infeasible={(0x1712, 0x178E), (0x17B4, 0x17B6)}, unreachable={0x17B6}),
}
LIMIT = 200000


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u33, u3bd, u3c6 = units[0x33], units[0x3BD], units[0x3C6]
    check(u33["ovr_offset"] == U33_BASE and u33["memory_segment"] == 0x3B, "unit 0x33 placement differs")
    check(u3bd["ovr_offset"] == U3BD_BASE and u3bd["memory_segment"] == STUB_3BD and
          u3bd["entries"][3] == 0x0350 and u3bd["entries"][2] == 0x047B, "unit 0x3BD entries 2/3 differ")
    check(u3c6["ovr_offset"] == U3C6_BASE and u3c6["memory_segment"] == STUB_3C6 and u3c6["entries"][16] == 0x0CB9 and
          u3c6["entries"][15] == 0x0C5F and u3c6["entries"][1] == 0x0000 and u3c6["entries"][6] == 0x04BE,
          "unit 0x3C6 entries 1/6/15/16 differ")
    code = {0x33: ovr[U33_BASE:U33_BASE + u33["code_size"]], 0x3BD: ovr[U3BD_BASE:U3BD_BASE + u3bd["code_size"]],
            0x3C6: ovr[U3C6_BASE:U3C6_BASE + u3c6["code_size"]]}
    fixups, starts = set(u33["fixups"]), sorted(f["rel"] for f in u33["functions"])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    bodies = {}
    for key, target in TARGETS.items():
        body = code[0x33][target["entry"]:target["end"]]
        decoded = list(decoder.disasm(body, target["entry"]))
        check(len(decoded) == target["insns"] and decoded[-1].address == target["retf"] and decoded[-1].mnemonic == "retf" and
              int(decoded[-1].op_str, 16) == target["pops"] and decoded[-1].address + decoded[-1].size == target["end"],
              f"+{key} extent differs")
        check(target["end"] in starts and target["entry"] in starts, f"+{key} is not bounded by prologues")
        calls = [(i.address, i.mnemonic, i.op_str) for i in decoded if "call" in i.mnemonic or i.mnemonic.startswith("int")]
        check(calls == target["calls"], f"+{key} calls differ: {calls}")
        for address, mnemonic, _ in calls:
            if mnemonic == "lcall":
                check((address + 3) in fixups, f"+{key} far call segment word is not fixup-backed")
            else:
                check(decoded[[i.address for i in decoded].index(address) - 1].op_str == "cs", f"+{key} near call lacks PUSH CS")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded
                       if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        check(len(conditional) == target["conditionals"], f"+{key} conditional count differs")
        sites = {i.address for i in decoded}
        for i in decoded:
            if i.mnemonic.startswith("j"):
                check(int(i.op_str, 16) in sites, f"+{key} branch leaves the body")
        near = set()
        for function in starts:
            end = next((x for x in starts if x > function), len(code[0x33]))
            aligned = {i.address: i for i in decoder.disasm(code[0x33][function:end], function)}
            for site, ins in aligned.items():
                if ins.mnemonic == "call" and ins.op_str.startswith("0x") and int(ins.op_str, 16) == target["entry"]:
                    check(aligned[site - 1].op_str == "cs", f"+{key} caller lacks PUSH CS")
                    near.add((function, site))
        check(near == target["callers"], f"+{key} aligned near callers differ: {sorted(near)}")
        bodies[key] = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), **target)
    print(f"u33 +1434 ({len(bodies['1434']['body'])}/{bodies['1434']['insns']}, LCALL 03E0:0070 to u3C6 entry 16 +0CB9) "
          f"sha {bodies['1434']['sha'][:16]}...; +1577 ({len(bodies['1577']['body'])}/{bodies['1577']['insns']}, "
          f"+1403 x2, +01AD x2, +1434) sha {bodies['1577']['sha'][:16]}...")
    print("aligned PUSH CS/CALL callers:", {k: sorted(f"+{f:04X}@+{s:04X}" for f, s in v["callers"]) for k, v in bodies.items()})
    return bodies, code


CHECKER_ROWS = ('OVL_0033_1577',)


def library(directory):
    import subprocess
    path = directory / "u33.so"
    sources = ["overlay_u33_place_search_1577.c", "overlay_u33_cell_claim_1434.c", "overlay_u3c6_0cb9_cpu.c",
               "overlay_u33_both_outside_1403.c", "overlay_u33_cell_pair_01ad.c", "overlay_u33_cell_lookup_012b.c",
               "overlay_u3bd_lookup_fullstate.c", "overlay_u3bd_in_grid_031f.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources],
                    "-o", str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    claim = lib.pr_u33_cell_claim_1434
    claim.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
    claim.restype = ctypes.c_int
    search = lib.pr_u33_place_search_1577
    search.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint,
                       ctypes.POINTER(ctypes.c_int)]
    search.restype = ctypes.c_int

    def run_1434(ram, cpu, chain):
        return claim(ram, SIZE, ctypes.byref(cpu), ctypes.byref(chain))

    def run_1577(ram, cpu, chain, cap=LIMIT):
        return search(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(chain))
    return {"1434": run_1434, "1577": run_1577}


DIRECTIONS = ((0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1))


def world(rng, *, realistic=True):
    """Every DS byte the chain reads, keyed by DS offset."""
    data = {}
    put = lambda offset, value: data.__setitem__(offset & 0xFFFF, value & 0xFF)
    for i in range(0x10A):                                     # +012B / u3BD tables and the direction tables
        put(0x2736 + i, rng.randrange(256))
    for j in range(1, 0x40):                                   # class weights (class 0 is never weighed)
        put(0x2744 + 4 * j, rng.choice((0, 1, 2, 3, 0xFE, 0xFF)))
    for d, (dx, dy) in enumerate(DIRECTIONS):
        put(0x2736 + d, dx); put(0x273F + d, dy)
    put(0x2736 + 8, 0); put(0x273F + 8, 0)                     # k = 8: no offset
    if realistic:
        rotation = rng.randrange(4); odd = rng.randrange(2)
        for q in range(4):
            put(0x02F2 + q, 2 * ((q + rotation) % 4) + odd)
    else:
        for q in range(4):
            put(0x02F2 + q, rng.randrange(8))
    for i in range(0x30):                                      # 2D2/2E2 rows, 2F6/2FE base tables
        put(0x02D2 + i, rng.randrange(8) if i < 0x10 else rng.randrange(256))
    for i in range(0x10):
        put(0x02E2 + i, rng.randrange(8) * 2 + rng.randrange(2))
    for i in range(8):
        put(0x02F6 + i, rng.randrange(-2, 13)); put(0x02FE + i, rng.randrange(-2, 8))
    for m in range(2):
        put(0x4650 + m, rng.randrange(0, 11)); put(0x4652 + m, rng.randrange(0, 6))
        put(0x4654 + m, rng.randrange(1, 14)); put(0x4656 + m, rng.randrange(0, 4))
    put(0x4658, rng.choice((0, 1, 0, 1, rng.randrange(256))) if not realistic else rng.choice((0, 1)))
    put(0x49FA, rng.choice((0, 1, 2, 7)))
    put(0x6AAD, rng.randrange(-3, 13)); put(0x6AAE, rng.randrange(-3, 8))
    put(0x84DC, rng.choice((0, 5, 10)))
    density = rng.choice((0.02, 0.1, 0.4, 0.9))
    for i in range(0x210):                                     # availability bytes
        put(0x4440 + i, 1 if rng.random() < density else 0)
    count = rng.randrange(1, 9)
    put(0x5F2A, count)
    for s in range(1, 0x40):
        put(0x5F27 + 4 * s, rng.randrange(0x32)); put(0x5F28 + 4 * s, rng.randrange(0x19))
        put(0x5F29 + 4 * s, rng.randrange(256)); put(0x5F2A + 4 * s, rng.randrange(4))
    for s in range(0x40):
        put(0x65B9 + 4 * s, 0x1100 + s); put(0x65BA + 4 * s, (0x1100 + s) >> 8)
        put(0x65BB + 4 * s, 0x22); put(0x65BC + 4 * s, 0x22)
    for j in range(0x40):
        put(0x2844 + j, rng.choice((0x00, 0x01, 0x02, 0xFF, 0x80, 0x7F, 0xFE)))
    for j in range(50 * 25):
        put(0x60DB + j, rng.choice((0, 0, 0, 1, 2, 3)))
    return data, count


def fixture(code, key, *, seed=1, realistic=True, q=None, args=None, ss=SS, sp=0x8000, ds=DS, es=0x1111, flags=0x7202,
            background=None, grid_at=(0x8000, 0x0000), buffer_at=(0x5000, 0x0100), return_frame=(0x7000, 0x7001),
            frame_wrapped=False, stale=None, high_bytes=0x00, overrides=None, grid_fill=None):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[physical(CALLER_CS, 0):physical(CALLER_CS, 0) + len(code[0x33])] = code[0x33]
    at = physical(LOOKUP_CS, U3BD_SPAN[0]); memory[at:at + U3BD_SPAN[1] - U3BD_SPAN[0]] = code[0x3BD][U3BD_SPAN[0]:U3BD_SPAN[1]]
    for lo, hi in U3C6_SPANS:
        at = physical(SCAN_CS, lo); memory[at:at + hi - lo] = code[0x3C6][lo:hi]
    for offset, entry in STUBS_3BD.items():
        at = physical(STUB_3BD, offset); memory[at:at + 5] = struct.pack("<BHH", 0xEA, entry, LOOKUP_CS)
    at = physical(STUB_3C6, 0x70); memory[at:at + 5] = struct.pack("<BHH", 0xEA, 0x0CB9, SCAN_CS)
    data, count = world(rng, realistic=realistic)
    for offset, value in (overrides or {}).items():
        data[offset & 0xFFFF] = value & 0xFF
    count = data.get(0x5F2A, count) or 1
    for offset, value in data.items():
        memory[physical(ds, offset)] = value
    put16(memory, ds, 0x6716, grid_at[1]); put16(memory, ds, 0x6718, grid_at[0])
    for i in range(0x4E9):
        memory[physical(grid_at[0], (grid_at[1] + i) & 0xFFFF)] = (grid_fill if grid_fill is not None else
                                                                     rng.choice((0x00, 0x1E, 0x02, 0x05, 0x07, 0x80, 0x01, 0x03)))
    put16(memory, ds, 0x6A5C, buffer_at[1]); put16(memory, ds, 0x6A5E, buffer_at[0])
    for i in range(0x400):
        memory[physical(buffer_at[0], (buffer_at[1] + i) & 0xFFFF)] = rng.randrange(256)
    slot = q if q is not None else rng.randrange(1, count + 1)
    target = TARGETS[key]
    if key == "1577":
        words = [return_frame[0], return_frame[1], (high_bytes << 8) | slot]
    else:
        values = args if args is not None else (rng.randrange(4), rng.randrange(-1, 12), rng.randrange(-1, 12),
                                                rng.randrange(-1, 7), rng.randrange(-1, 12), slot)
        words = [return_frame[0], return_frame[1]] + [(rng.randrange(256) << 8) | (v & 0xFF) for v in values]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if not frame_wrapped:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    if stale is not None:                                       # bytes the callee frames will find below SP
        for i in range(0x80):
            memory[physical(ss, (sp - 0x80 + i) & 0xFFFF)] = stale[i % len(stale)]
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, CALLER_CS, target["entry"], ds, es, ss, flags)
    high = any(seg >= 0xF000 for seg in (ss, ds, grid_at[0], buffer_at[0]))
    return memory, cpu, high


def compare(label, memory, cpu, high, bodies, key, function, budget=6000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    target = bodies[key]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, target["entry"]); body_end = start + len(target["body"]); last = physical(cpu.cs, target["retf"])
    returned, previous, visited, edges = [False], [None], set(), set()
    conditional = target["conditional"]

    def instruction(emulator, address, size, user):
        del size, user
        if returned[0]:
            emulator.emu_stop(); return
        here = address - start + target["entry"]
        visited.add(here)
        if previous[0] in conditional:
            edges.add((previous[0], here))
        previous[0] = here
        if address == last:
            returned[0] = True

    def leave(emulator, address, size, user):
        del address, size, user
        previous[0] = None
        if returned[0]:
            emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, leave, begin=0, end=start - 1)
    original.hook_add(UC_HOOK_CODE, leave, begin=body_end, end=2 * SIZE - 1)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    chain = ctypes.c_int(-1)
    if not returned[0]:
        status = function(ram, cpu, chain, 2000) if key == "1577" else None
        check(status == 5, f"original did not return within {budget} instructions and the C cap gave {status}: {label}")
        return set(), set(), False                          # coverage counts only compared, returning runs
    expected = bytes(original.mem_read(0, SIZE))
    status = function(ram, cpu, chain)
    check(status == 0, f"C status {status} (callee {chain.value}): {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges, True


def cases(key):
    rng = random.Random({"1434": 0x331434, "1577": 0x331577}[key])
    spec = []
    for i in range(160 if key == "1434" else 120):
        spec.append((f"random-{i}", dict(seed=rng.randrange(1 << 30), flags=rng.choice((0x7202, 0x0202, 0x0ED7, 0x0647)),
                                        es=rng.randrange(0x10000), high_bytes=rng.randrange(256),
                                        stale=bytes(rng.randrange(256) for _ in range(16)))))
    for i in range(24):
        spec.append((f"unrealistic-{i}", dict(seed=rng.randrange(1 << 30), realistic=False)))
    near = {0x4440 + i: 1 for i in range(0x210)}
    near.update({0x5F2A: 4, 0x5F2E: 1, 0x5F32: 1, 0x5F36: 1, 0x5F3A: 1})
    near.update({0x2844 + j: 0 for j in range(0x40)})
    occupied = {**near, **{0x60DB + j: 2 for j in range(50 * 25)}}
    heavy = {**near, **{0x60DB + j: 0 for j in range(50 * 25)}, 0x274C: 0xFF}
    if key == "1434":
        for i in range(6):
            spec.append((f"occupied-{i}", dict(seed=900 + i, q=1, args=(i % 4, 0, 0, i % 6, 2 * i, 1), overrides=occupied, grid_fill=2)))
            spec.append((f"heavy-{i}", dict(seed=910 + i, q=1, args=(i % 4, 0, 0, i % 6, 2 * i, 1), overrides=heavy, grid_fill=2)))
            spec.append((f"claimable-{i}", dict(seed=920 + i, q=1, args=(i % 4, 0, 0, i % 6, 2 * i, 1),
                                                overrides={**heavy, 0x274C: 3}, grid_fill=2)))
    else:
        closed = {0x4440 + i: 0 for i in range(0x210)}
        bases = {**{0x02F6 + d: 5 for d in range(8)}, **{0x02FE + d: 2 for d in range(8)}}
        # Map 0, odd row byte, pass 0, ring 1: three +01AD probes from an origin 20 columns off the grid all return 1;
        # the ring moves diagonally (quadrant 2 -> SE) so a later ring leaves both axes and the search ends.
        probes = {**closed, **bases, 0x4658: 0, 0x4656: 1, 0x4657: 1, 0x49FA: 0, 0x6AAD: 0xEC, 0x6AAE: 0,
                  0x4650: 0, 0x4651: 0, 0x4652: 1, 0x4653: 1, 0x4654: 2, 0x4655: 2,
                  0x02F2: 0, 0x02F3: 2, 0x02F4: 3, 0x02F5: 6, **{0x02E2 + i: 0 for i in range(0x10)}}
        # Zero perpendicular directions keep every position on the ring base, so a later ring passes 0Bh steps.
        stuck = {**closed, **bases, 0x4658: 1, 0x4656: 0, 0x4657: 0, 0x49FA: 2, 0x4654: 1, 0x4655: 1,
                 0x02F2: 3, 0x02F3: 8, 0x02F4: 3, 0x02F5: 8, **{0x02E2 + i: 0 for i in range(0x10)}}
        for i in range(3):
            spec.append((f"probes-pass-{i}", dict(seed=940 + i, q=1, overrides=probes)))
            spec.append((f"stuck-ring-{i}", dict(seed=950 + i, q=1, overrides=stuck)))
        for i in range(6):
            spec.append((f"occupied-{i}", dict(seed=900 + i, q=1, overrides=occupied, grid_fill=2)))
            spec.append((f"claimable-{i}", dict(seed=920 + i, q=1, overrides={**heavy, 0x274C: 3}, grid_fill=2)))
    base = dict(seed=77)
    # The deepest nested frame (+1577, +1434, +0CB9 and its folded entry 1) sits about 70h bytes below the entry SP.
    # Lower entry SPs wrap a callee RETF frame across SS:0000, where the contiguous frame reading shared by this
    # model and the emulator sends the original out of the code and the model reports TRANSFER; those layouts are
    # the declared frame seam, not parity cases.
    for sp in (0x0100, 0x0180, 0xFFF0, 0xFFF8, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(base, sp=sp, ss=0x1800, zero=True)))
    spec.append(("ss-a20", dict(base, ss=0xFFFF, sp=0x0400)))
    spec.append(("ds-a20", dict(base, ds=0xFFFF)))
    spec.append(("grid-a20", dict(base, grid_at=(0xFFFF, 0x0010))))
    spec.append(("grid-offset-wrap", dict(base, grid_at=(0x8000, 0xFE00))))
    spec.append(("random-background", dict(base, bg=11)))
    spec.append(("preserved-df", dict(base, flags=0x0603)))
    return spec


def differential(bodies, code, key, functions, quick=False):
    visited, edges, count, nonreturning = set(), set(), 0, 0
    target = bodies[key]
    for label, options in cases(key):
        if quick and label.split("-")[0] == "random" and label.split("-")[1].isdigit() and int(label.split("-")[1]) >= 24:
            continue
        options = dict(options)
        bg = options.pop("bg", None); zero = options.pop("zero", False)
        background = bytes(random.Random(bg).randbytes(SIZE)) if bg is not None else (bytes(SIZE) if zero else None)
        memory, cpu, high = fixture(code, key, background=background, **options)
        v, e, returned = compare(f"+{key} {label}", memory, cpu, high, bodies, key, functions[key])
        visited |= v; edges |= e
        count += returned; nonreturning += not returned
    missing_sites = sorted(set(i.address for i in target["decoded"]) - visited - target["unreachable"])
    missing_edges = sorted({(s, o) for s, outs in target["conditional"].items() for o in outs} - edges - target["infeasible"])
    check(not (target["infeasible"] & edges) and not (target["unreachable"] & visited), f"+{key} reached a declared-infeasible outcome")
    print(f"+{key}: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; {nonreturning} non-returning "
          f"originals stopped by the C cap; {len(visited & set(i.address for i in target['decoded']))}/{len(target['decoded'])} "
          f"instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    if missing_sites or missing_edges:
        print(f"  missing starts {[hex(x) for x in missing_sites]}; missing outcomes {[(hex(a), hex(b)) for a, b in missing_edges]}")
    check(not missing_sites and not missing_edges, f"+{key} coverage incomplete")
    return dict(returning=count, nonreturning=nonreturning, starts=len(visited), outcomes=len(edges),
                infeasible=sorted(f"+{a:04X}->+{b:04X}" for a, b in target["infeasible"]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--only", choices=sorted(TARGETS))
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies, code = source()
    report = dict(targets={k: dict(sha256=v["sha"], bytes=len(v["body"]), instructions=v["insns"]) for k, v in bodies.items()})
    keys = [args.only] if args.only else list(bodies)
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u33-1577-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn_cases"] = {k: differential(bodies, code, k, functions, args.quick) for k in keys}
            if args.native:
                from u33_place_search_1577_native import native_audit
                report["native"] = native_audit(ROOT, bodies, code, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
