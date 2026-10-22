#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U3C6 +10A7, composing entry 15 (+0C5F, folded), the merged entry 16
(+0CB9, WRAPPED_SS) and entry 5 (+03A0) models and SYSTEM FillChar, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so SYSTEM FillChar runs from its
original bytes, loads unit 0x3C6, and compares the entire 1 MiB RAM and all 14 CPU words between the original under
Unicorn and the C model. The original decides each stop: the fetch after its own RETF 0Ah (return), the FillChar site
whose live bytes differ when fetched (CODE), loop 1's taken JNE +117F past the host cap (LIMIT), an +03A0 budget point
reached with that budget spent, counted as its merged audit counts it, or a nested RETF that does not come back to its
call (CALLEE), or a RETF of a direct callee that does not come back to its call (TRANSFER). Any fetch outside the
pre-decoded original functions is an error. Only one FLAGS bit is ever excepted: AF at returns whose final flags come from
the slot-clearing path's SHL DI,1, where Unicorn leaves 0 and the C model, like the merged +0CB9 model, follows the pinned
DOSBox-X core's 1; the native oracle compares it unmasked. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY, END, BODY_RETF = 0x10A7, 0x1298, 0x1295
BODY_SHA = None
FUNCTIONS = ((0x0000, 0x0068), (0x03A0, 0x04BE), (0x04BE, 0x052B), (0x0C5F, 0x0CB9), (0x0CB9, 0x0E11), (0x10A7, 0x1298))
FILL_SITE, LOOP_HEAD, LOOP_JNE, FAIL_SHL = 0x11DF, 0x117F, 0x11EA, 0x1144
SYSTEM_CS = 0x0AF8
U3C6_CS, DS, SS, REC, MAP_SEG, MAP_OFF = 0x3000, 0x5000, 0x2000, 0x6000, 0x7000, 0x0100
RET_CS, RET_IP, R_OFF = 0x9001, 0x7000, 0x3000
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)
BUDGETS = (512, 4096)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u = units[0x3C6]
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    decoded = {}
    for start, end in FUNCTIONS:
        for i in md.disasm(code[start:end], start):
            decoded[("u3c6", i.address)] = i
    fill = image[physical(SYSTEM_CS, 0x16A8):physical(SYSTEM_CS, 0x16BC)]
    check(sha256(fill).hexdigest() == "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7", "SYSTEM FillChar differs")
    for i in md.disasm(fill, 0x16A8):
        decoded[("sys", i.address)] = i
    body = code[ENTRY:END]
    insns = [i for (kind, ip), i in sorted(decoded.items()) if kind == "u3c6" and ENTRY <= ip < END]
    calls = [(i.address, i.mnemonic, i.op_str) for i in insns if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(body) == 497 and len(insns) == 191 and insns[-1].address == BODY_RETF and insns[-1].op_str == "0xa"
          and calls == [(0x10C2, "call", "0xc5f"), (0x111A, "call", "0xcb9"), (0x11DF, "lcall", "0xaf8, 0x16a8"), (0x1286, "call", "0x3a0")]
          and FILL_SITE + 3 in set(u["fixups"]) and u["entries"][18] == ENTRY, f"U3C6 +10A7 extent or calls differ: {calls}")
    for start, end in FUNCTIONS[:-1]:
        last = [i for (kind, ip), i in sorted(decoded.items()) if kind == "u3c6" and start <= ip < end][-1]
        check(last.mnemonic == "retf", f"U3C6 +{start:04X} does not end in RETF")
    target = dict(conditional=conditional, starts=[i.address for i in insns], sha=sha256(body).hexdigest(), file_offset=u["ovr_offset"] + ENTRY)
    print(f"U3C6 +10A7 (497/191, near calls +0C5F/+0CB9/+03A0, LCALL 0AF8:16A8, RETF 0Ah) GAME file {target['file_offset']:#x} "
          f"sha {target['sha']}; {len(conditional)} conditional branches")
    return target, code, image, decoded


CHECKER_ROWS = ('OVL_03C6_10A7',)


def library(directory):
    path = directory / "u3c610a7.so"
    sources = ["overlay_u3c6_caller_10a7_cpu.c", "overlay_u3c6_0cb9_cpu.c", "overlay_u3c6_rebuild_03a0.c", "overlay_u3c6_table_pair_0000.c",
               "system_runtime_leaves_cpu.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u3c6_caller_10a7_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.c_size_t, ctypes.c_size_t,
                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def world(memory, rng, *, flag49, slots, q_slot, q_type, r13, shape, bias, ranks, cells, occupancy, entries, origin):
    """The DS world: slot table and slots, shape rows, bias, ranks, the 50 x 25 map and occupancy grid, 7-byte entries."""
    def byte(segment, offset, value):
        memory[physical(segment, offset & 0xFFFF)] = value & 255
    byte(DS, 0x49F3, flag49)
    for i in range(256):
        put16(memory, DS, 0x65B9 + 4 * i, 0x9999); put16(memory, DS, 0x65BB + 4 * i, 0x8888)
        byte(DS, 0x2744 + 4 * i, 0)
    byte(DS, 0x5F2A, len(slots))
    for i, (x, y, t) in enumerate(slots, 1):
        put16(memory, DS, 0x65B9 + 4 * i, 0x0200 * i); put16(memory, DS, 0x65BB + 4 * i, REC)
        byte(DS, 0x5F27 + 4 * i, x); byte(DS, 0x5F28 + 4 * i, y); byte(DS, 0x5F2A + 4 * i, t)
    for t in range(256):
        for i, (dx, dy) in enumerate(shape):
            byte(DS, 0x2844 + 8 * t + 2 * i, dx); byte(DS, 0x2845 + 8 * t + 2 * i, dy)
    byte(DS, 0x2736 + 8, bias[0]); byte(DS, 0x273F + 8, bias[1])
    for value, rank in ranks:
        byte(DS, 0x2744 + 4 * value, rank)
    put16(memory, DS, 0x6716, MAP_OFF); put16(memory, DS, 0x6718, MAP_SEG)
    byte(MAP_SEG, MAP_OFF + 2, origin[0]); byte(MAP_SEG, MAP_OFF + 3, origin[1])
    for i in range(1250):
        byte(MAP_SEG, MAP_OFF + 7 + i, 2); byte(DS, 0x60DB + i, 0)
    for (col, row), value in cells:
        byte(MAP_SEG, MAP_OFF + 7 + 50 * row + col, value)
    for (col, row), value in occupancy:
        byte(DS, 0x60DB + 50 * row + col, value)
    byte(DS, 0x6715, len(entries))
    for k, (offset, segment, x, y, b6) in enumerate(entries, 1):
        at = 0x66D6 + 7 * k
        put16(memory, DS, at, offset); put16(memory, DS, at + 2, segment); byte(DS, at + 4, x); byte(DS, at + 5, y); byte(DS, at + 6, b6)
    q = (0x0200 * q_slot, REC) if q_slot else (0x0F00, REC)
    byte(REC, q[0] + 0x6C, q_type)
    put16(memory, REC, q[0] + 0x108, R_OFF); put16(memory, REC, q[0] + 0x10A, REC)
    byte(REC, R_OFF + 0x13, r13)
    return q


def build(code, image, rng, *, flag49=5, slots=((10, 10, 1), (20, 12, 1)), q_slot=1, q_type=0x81, r13=0, F=1, X=10, Y=10,
          shape=((-1, 0),) * 4, bias=(0, 0), ranks=(), cells=(), occupancy=(), entries=(), origin=(3, 4), sp=0xF000, ss=SS,
          flags=0x0602, background=None, patch=(), writes=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(U3C6_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(U3C6_CS, offset)] = value
    q = world(memory, rng, flag49=flag49, slots=slots, q_slot=q_slot, q_type=q_type, r13=r13, shape=shape, bias=bias, ranks=ranks,
              cells=cells, occupancy=occupancy, entries=entries, origin=origin)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, (rng.randrange(256) << 8) | F, (rng.randrange(256) << 8) | Y, (rng.randrange(256) << 8) | X,
                               q[0], q[1])):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    for segment, offset, data in writes:
        for j, value in enumerate(data):
            memory[physical(segment, (offset + j) & 0xFFFF)] = value
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, U3C6_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), q


def compare(label, memory, cpu, target, decoded, function, cap=64, budgets=BUDGETS, count=5000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, last=None, loops=0, remaining=list(budgets), calls=[],
                 unexpected=None, fill=0, fail_path=False)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["returning"]:
            state["returning"] = False
            want = state["calls"].pop()
            if (cs, ip) != want:
                state["stop"] = TRANSFER if not state["calls"] else CALLEE
                return emulator.emu_stop()
        kind = "u3c6" if cs == U3C6_CS else "sys" if cs == SYSTEM_CS else None
        key = (kind, ip)
        if key not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        ins = decoded[key]
        if kind == "u3c6" and ENTRY <= ip < END:
            if ip == FILL_SITE:
                if bytes(emulator.mem_read(address, 3)) != b"\x9a\xa8\x16":
                    state["stop"] = CODE
                    return emulator.emu_stop()
                state["fill"] += 1
            if ip == LOOP_HEAD and state["last"] == ("u3c6", LOOP_JNE):
                state["loops"] += 1
                if state["loops"] > cap:
                    state["stop"] = LIMIT
                    return emulator.emu_stop()
            if ip == FAIL_SHL:
                state["fail_path"] = True
            visited.add(ip)
            if state["previous"] in target["conditional"] and state["previous"] != ip:
                edges.add((state["previous"], ip))
            state["previous"] = ip
            state["returned"] = ip == BODY_RETF
        else:
            state["previous"] = None
            index = 0 if key == ("u3c6", 0x3CF) else 1 if key == ("u3c6", 0x3ED) else None
            if index is not None:
                if not state["remaining"][index]:
                    state["stop"] = CALLEE
                    return emulator.emu_stop()
                state["remaining"][index] -= 1
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf") and not (kind == "u3c6" and ip == BODY_RETF):
            state["returning"] = True
        state["last"] = key
        return None

    original.hook_add(UC_HOOK_CODE, hook)
    original.emu_start(physical(U3C6_CS, ENTRY), 2 * SIZE, count=count)
    check(state["unexpected"] is None, f"original left the decoded code at {state['unexpected']}: {label}")
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, *budgets, ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee.value}/{callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    masked = False
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        if name == "flags" and state["stop"] == OK and state["fail_path"] and mine == (want | 0x0010) and not want & 0x0010:
            masked = True                                                  # the declared SHL AF exception
            continue
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, (callee.value, callee_status.value), masked, cpu.ax & 0xFF, state["fill"]


def cases(rng):
    spec = []
    valid = ((0, 0), (1, 0), (0, 1), (-1, 0))
    for i in range(120):
        n = rng.randrange(1, 7)
        slots = tuple((rng.randrange(2, 46), rng.randrange(2, 22), rng.choice((1, 1, 0x81, 2, 0))) for _ in range(n))
        q_slot = rng.choice(range(1, n + 1)) if rng.random() < 0.9 else 0
        X, Y = rng.randrange(2, 46), rng.randrange(2, 22)
        shape = rng.choice((((-1, 0),) * 4, valid, ((0, 0), (-1, 0), (1, 1), (-2, 0))))
        cells = tuple(((X + dx, Y + dy), rng.choice((2, 2, 5, 9, 0, 0x30))) for dx, dy in valid if 0 <= X + dx <= 49 and 0 <= Y + dy <= 24)
        occupancy = tuple(((X + dx, Y + dy), rng.choice((0, 0, 0, 1, 3))) for dx, dy in valid if 0 <= X + dx <= 49 and 0 <= Y + dy <= 24)
        ranks = tuple((v, rng.choice((0, 1, 4, 0xFF))) for v in (2, 5, 9, 0x17, 0x30))
        entries = tuple((0x0200 * q_slot if rng.random() < 0.4 else rng.randrange(0x10000), REC if rng.random() < 0.7 else rng.randrange(0x10000),
                         rng.choice((X, X, rng.randrange(50))), rng.choice((Y, Y, rng.randrange(25))), rng.choice((0x1F, 3, 7, 0)))
                        for _ in range(rng.choice((0, 1, 2, 3, 5))))
        spec.append((f"random-{i}", dict(flag49=rng.choice((5,) * 9 + (4,)), slots=slots, q_slot=q_slot, q_type=rng.randrange(256),
                                         r13=rng.choice((0, 0, 0, 1)), F=rng.choice((1, 1, 1, 0)), X=X, Y=Y, shape=shape, cells=cells,
                                         occupancy=occupancy, ranks=ranks, entries=entries, origin=(rng.randrange(8), rng.randrange(8)),
                                         flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0xF000) & ~1)))
    q1 = (0x0200, REC)
    spec.append(("not-five", dict(flag49=4)))
    spec.append(("success-no-flag", dict(F=0)))
    spec.append(("success-r13-set", dict(r13=1)))
    spec.append(("success-loops-empty", dict()))
    spec.append(("success-entries", dict(entries=((q1[0], q1[1], 10, 10, 7), (0x1234, REC, 10, 10, 0x1F), (q1[0], q1[1], 5, 5, 0x1F), (0, 0, 10, 10, 1)))))
    spec.append(("success-seen", dict(entries=((0x1234, REC, 10, 10, 3),))))
    spec.append(("success-x-only", dict(entries=((0x1234, REC, 10, 11, 3),))))
    spec.append(("occupied-cells", dict(shape=valid, occupancy=(((10, 10), 1), ((11, 10), 1), ((10, 11), 1), ((9, 10), 1)))))
    spec.append(("fail-rank", dict(ranks=((0x17, 0xFF),))))
    spec.append(("fail-zero-cell", dict(shape=valid, cells=(((10, 10), 0), ((11, 10), 0), ((10, 11), 0), ((9, 10), 0)))))
    spec.append(("q-absent", dict(q_slot=0)))
    for sp in (0x0100, 0x0101, 0xFFE0, 0xFFF0):
        spec.append((f"sp-{sp:04x}", dict(sp=sp, ss=0x1000 if sp >= 0xFFE0 else SS)))
    spec.append(("random-background", dict(background=bytes(random.Random(0x3C610A7).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(flags=0x0603)))
    spec.append(("code-fill-site", dict(entries=((q1[0], q1[1], 10, 10, 7),), patch=((FILL_SITE + 1, 0x00),))))
    spec.append(("limit-loop-1", dict(entries=((q1[0], q1[1], 10, 10, 7),) * 3, cap=1)))
    spec.append(("cap-exact", dict(entries=((q1[0], q1[1], 10, 10, 7),) * 3, cap=2)))
    spec.append(("u3c6-slot-limit", dict(budgets=(1, 4096))))
    spec.append(("u3c6-cell-limit", dict(shape=((0, 0), (1, 0), (0, 1), (-1, 0)), budgets=(512, 1))))
    # SS = DS with +03A0's return frame at DS:60DBh: its own FillChar of the grid [60DBh, 65BDh) clears that frame (and
    # not FillChar's, 14h lower), so +03A0 returns to 0000:0000.
    spec.append(("u3c6-return-alias", dict(ss=DS, sp=0x60DB + 0x0E, F=0)))
    return spec


def differential(target, code, image, decoded, function):
    rng = random.Random(0x3C610A7)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    masked, results, fills = 0, {}, 0
    for label, options in cases(rng):
        cap = options.pop("cap", 64)
        budgets = options.pop("budgets", BUDGETS)
        memory, cpu, q = build(code, image, rng, **options)
        stop, v, e, callee, m, al, fill = compare(f"+10A7 {label}", memory, cpu, target, decoded, function, cap, budgets)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} callee {callee} AL {al}{' (AF excepted)' if m else ''} fills {fill}")
        stops[stop] += 1
        visited |= v
        edges |= e
        masked += m
        fills += fill
        if stop == OK:
            results[al] = results.get(al, 0) + 1
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(a) for a in target["starts"] if a not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    print(f"U3C6 +10A7: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); "
          f"full 1 MiB RAM and all 14 CPU words equal, AF excepted at {masked} slot-clearing returns; {len(visited)}/{len(target['starts'])} "
          f"instruction starts, {len(edges)}/{len(reachable)} conditional outcomes; FillChar fetches {fills}; results {dict(sorted(results.items()))}")
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(all(stops.values()), f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges), af_excepted=masked,
                fill_fetches=fills, results={str(k): v for k, v in sorted(results.items())})


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N = 0x7800
MAIN_AT, FILL_AT, CASES_AT, CODE_AT = 0x0300, 0x0E00, 0x0E20, 0x3000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, MAP_WINDOW, REC_BYTES = STACK_TOP - 0x200, 0x200, 0x0700, 0x4000
CASE_SIZE, NATIVE_CASES = 96, 48
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000 + MAP_WINDOW
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
NATIVE_VALID = ((0, 0), (1, 0), (0, 1), (-1, 0))


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        count = rng.randrange(1, 5)
        slots = [(rng.randrange(2, 46), rng.randrange(2, 22), rng.choice((1, 1, 0x81, 2, 0))) for _ in range(count)]
        q_slot = rng.randrange(1, count + 1) if n % 9 else 0
        X, Y = rng.randrange(2, 46), rng.randrange(2, 22)
        shape = rng.choice((((-1, 0),) * 4, NATIVE_VALID, ((0, 0), (-1, 0), (1, 1), (-2, 0))))
        q = (0x0200 * q_slot, REC) if q_slot else (0x0F00, REC)
        entries = [(q[0] if rng.random() < 0.5 else rng.randrange(0x10000), q[1] if rng.random() < 0.8 else rng.randrange(0x10000),
                    rng.choice((X, X, rng.randrange(50))), rng.choice((Y, Y, rng.randrange(25))), rng.choice((0x1F, 3, 7, 0)))
                   for _ in range(rng.choice((0, 1, 2, 3, 4)))]
        cases.append(dict(flags=n % len(NATIVE_FLAGS), flag49=5 if n % 11 else 4, slots=slots, q_slot=q_slot, q_type=rng.randrange(256),
                          r13=rng.choice((0, 0, 0, 1)), F=rng.choice((1, 1, 1, 0)), X=X, Y=Y, shape=shape, bias=(0, 0),
                          ranks=[(v, rng.choice((0, 1, 4, 0xFF))) for v in (2, 5, 9, 0x17, 0x30)],
                          cells=[((X + dx, Y + dy), rng.choice((2, 2, 5, 9, 0, 0x30))) for dx, dy in NATIVE_VALID],
                          occupancy=[((X + dx, Y + dy), rng.choice((0, 0, 0, 1, 3))) for dx, dy in NATIVE_VALID],
                          entries=entries, origin=(rng.randrange(8), rng.randrange(8))))
    return cases


def case_bytes(case):
    data = bytes((case["flags"], case["flag49"], len(case["slots"])))
    data += b"".join(bytes(slot) for slot in case["slots"]) + bytes(3 * (4 - len(case["slots"])))
    data += bytes((case["q_slot"], case["q_type"], case["r13"], case["F"], case["X"], case["Y"]))
    data += bytes(v & 255 for pair in case["shape"] for v in pair) + bytes(case["bias"])
    data += b"".join(bytes(pair) for pair in case["ranks"]) + bytes(case["origin"])
    data += b"".join(bytes((col, row, value)) for (col, row), value in case["cells"])
    data += b"".join(bytes((col, row, value)) for (col, row), value in case["occupancy"])
    data += bytes((len(case["entries"]),)) + b"".join(struct.pack("<HH3B", *e) for e in case["entries"]) + bytes(7 * (4 - len(case["entries"])))
    check(len(data) == 96, f"case layout {len(data)}")
    return data


def native_audit(target, code, image, decoded, function):
    del target, decoded
    directory = ROOT / "resources/oracle-runs/u3c6-10a7-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    fill = image[physical(SYSTEM_CS, 0x16A8):physical(SYSTEM_CS, 0x16BC)]
    unit = bytearray(code)
    for site in (0x03B2, FILL_SITE):
        struct.pack_into("<H", unit, site + 3, SYS_N)
    rng = random.Random(0x3C610A7D)
    cases = native_cases(rng)
    check(FILL_AT + len(fill) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= CODE_AT and CODE_AT + len(unit) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((U3C6_CS, 0, "unit", len(unit)), (SYS_N, 0x16A8, "fill", len(fill))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    t += ["    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
          "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
          "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
          "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
          "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov word ptr [case_ptr],ax",
          # the DS segment, the map window and the record segment start as A5h
          f"    mov ax,{DS:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{MAP_SEG:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{MAP_WINDOW // 2:#x}", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{REC:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{REC_BYTES // 2:#x}", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{DS:#x}", "    mov es,ax", "    mov si,word ptr [case_ptr]",
          "    mov al,byte ptr [si+1]", "    mov byte ptr es:[0x49f3],al",
          "    mov di,0x65b9", "    mov bx,0x2744", "    mov cx,256", "table_loop:", "    mov word ptr es:[di],0x9999", "    mov word ptr es:[di+2],0x8888",
          "    mov byte ptr es:[bx],0", "    add di,4", "    add bx,4", "    loop table_loop",
          "    mov al,byte ptr [si+2]", "    mov byte ptr es:[0x5f2a],al", "    xor ch,ch", "    mov cl,byte ptr [si+2]", "    jcxz slots_done",
          "    lea bx,[si+3]", "    mov di,0x65bd", "    mov dx,0x0200", "    mov bp,0x5f2b", "slot_loop:",
          "    mov word ptr es:[di],dx", f"    mov word ptr es:[di+2],{REC:#x}", "    mov al,byte ptr [bx]", "    mov byte ptr es:[bp],al",
          "    mov al,byte ptr [bx+1]", "    mov byte ptr es:[bp+1],al", "    mov al,byte ptr [bx+2]", "    mov byte ptr es:[bp+3],al",
          "    add bx,3", "    add di,4", "    add dx,0x0200", "    add bp,4", "    loop slot_loop", "slots_done:",
          "    mov di,0x2844", "    mov dx,256", "shape_loop:", "    lea si,[si+21]", "    mov cx,8", "    rep movsb", "    mov si,word ptr [case_ptr]",
          "    dec dx", "    jnz shape_loop",
          "    mov al,byte ptr [si+29]", "    mov byte ptr es:[0x273e],al", "    mov al,byte ptr [si+30]", "    mov byte ptr es:[0x2747],al",
          "    lea bx,[si+31]", "    mov cx,5", "rank_loop:", "    mov al,byte ptr [bx]", "    xor ah,ah", "    shl ax,1", "    shl ax,1", "    mov di,ax",
          "    mov al,byte ptr [bx+1]", "    mov byte ptr es:[di+0x2744],al", "    add bx,2", "    loop rank_loop",
          f"    mov word ptr es:[0x6716],{MAP_OFF:#x}", f"    mov word ptr es:[0x6718],{MAP_SEG:#x}",
          f"    mov ax,{MAP_SEG:#x}", "    mov es,ax", "    mov al,byte ptr [si+41]", f"    mov byte ptr es:[{MAP_OFF + 2:#x}],al",
          "    mov al,byte ptr [si+42]", f"    mov byte ptr es:[{MAP_OFF + 3:#x}],al",
          f"    mov di,{MAP_OFF + 7:#x}", "    mov cx,1250", "    mov al,2", "    rep stosb",
          f"    mov ax,{DS:#x}", "    mov es,ax", "    mov di,0x60db", "    mov cx,1250", "    xor al,al", "    rep stosb",
          # cells (map segment) then occupancy (DS), each 4 x (col, row, value): offset 50 * row + col
          "    lea bx,[si+43]", "    mov cx,4", "cell_loop:", "    mov al,byte ptr [bx+1]", "    mov dl,50", "    mul dl", "    xor dh,dh",
          "    mov dl,byte ptr [bx]", "    add ax,dx", "    mov di,ax", f"    mov ax,{MAP_SEG:#x}", "    mov es,ax", "    mov al,byte ptr [bx+2]",
          f"    mov byte ptr es:[di+{MAP_OFF + 7:#x}],al", "    add bx,3", "    loop cell_loop",
          "    lea bx,[si+55]", "    mov cx,4", "occupancy_loop:", "    mov al,byte ptr [bx+1]", "    mov dl,50", "    mul dl", "    xor dh,dh",
          "    mov dl,byte ptr [bx]", "    add ax,dx", "    mov di,ax", f"    mov ax,{DS:#x}", "    mov es,ax", "    mov al,byte ptr [bx+2]",
          "    mov byte ptr es:[di+0x60db],al", "    add bx,3", "    loop occupancy_loop",
          "    mov al,byte ptr [si+67]", "    mov byte ptr es:[0x6715],al", "    xor ch,ch", "    mov cl,byte ptr [si+67]", "    jcxz entries_done",
          "    lea bx,[si+68]", "    mov di,0x66dd", "entry_loop:", "    push cx", "    mov cx,7", "entry_byte:", "    mov al,byte ptr [bx]",
          "    mov byte ptr es:[di],al", "    inc bx", "    inc di", "    loop entry_byte", "    pop cx", "    loop entry_loop", "entries_done:",
          # records: Q at 200h * q_slot (0F00h when absent), R at 3000h
          f"    mov ax,{REC:#x}", "    mov es,ax", "    mov al,byte ptr [si+15]", "    xor ah,ah", "    mov bx,0x0200", "    mul bx", "    mov bx,ax",
          "    or bx,bx", "    jnz q_ready", "    mov bx,0x0f00", "q_ready:", "    mov word ptr [q_off],bx",
          "    mov al,byte ptr [si+16]", "    mov byte ptr es:[bx+0x6c],al", f"    mov word ptr es:[bx+0x108],{R_OFF:#x}",
          f"    mov word ptr es:[bx+0x10a],{REC:#x}", "    mov al,byte ptr [si+17]", f"    mov byte ptr es:[{R_OFF + 0x13:#x}],al",
          # the frame: Q, X, Y, F words, then flags, registers and the far call
          f"    mov ax,{REC:#x}", "    push ax", "    push word ptr [q_off]", "    mov al,byte ptr [si+19]", "    mov ah,0x5c", "    push ax",
          "    mov al,byte ptr [si+20]", "    mov ah,0x5b", "    push ax", "    mov al,byte ptr [si+18]", "    mov ah,0x5a", "    push ax",
          "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
          "    push cs", "    pop es", f"    mov ax,{DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        t.append(f"    mov {name},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DS:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{MAP_SEG:#x}", "    mov ds,ax", "    xor dx,dx", f"    mov cx,{MAP_WINDOW:#x}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", "q_off: .word 0",
          f"target_far: .word {ENTRY:#x}, {U3C6_CS:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("fill", fill, FILL_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT), ("unit", bytes(unit), CODE_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    results, fails = {}, 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(U3C6_CS, 0); memory[at:at + len(unit)] = unit
        at = physical(SYS_N, 0x16A8); memory[at:at + len(fill)] = fill
        at = physical(DS, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
        at = physical(MAP_SEG, 0); memory[at:at + MAP_WINDOW] = bytes([0xA5]) * MAP_WINDOW
        at = physical(REC, 0); memory[at:at + REC_BYTES] = bytes([0xA5]) * REC_BYTES
        q = world(memory, None, flag49=case["flag49"], slots=case["slots"], q_slot=case["q_slot"], q_type=case["q_type"], r13=case["r13"],
                  shape=case["shape"], bias=case["bias"], ranks=case["ranks"], cells=case["cells"], occupancy=case["occupancy"],
                  entries=case["entries"], origin=case["origin"])
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (q[1], q[0], 0x5C00 | case["X"], 0x5B00 | case["Y"], 0x5A00 | case["F"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, U3C6_CS, ENTRY, DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        status = function(ram, SIZE, ctypes.byref(cpu), 64, *BUDGETS, ctypes.byref(callee), ctypes.byref(callee_status))
        check(status == OK, f"C status {status} on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DS, 0):physical(DS, 0) + 0x10000]) + bytes(ram[physical(MAP_SEG, 0):physical(MAP_SEG, 0) + MAP_WINDOW])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        results[cpu.ax & 0xFF] = results.get(cpu.ax & 0xFF, 0) + 1
        fails += callee.value == 1 and (cpu.ax & 0xFF) == 0
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (results {dict(sorted(results.items()))}, {fails} slot-clearing returns "
          f"with AF measured 1 and unmasked); all 14 CPU words, the {WINDOW_SIZE}-byte stack window, the whole 64 KiB DS segment and the "
          f"{MAP_WINDOW:#x}-byte map window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U3C6 at {U3C6_CS:04X}, FillChar at {SYS_N:04X}:16A8")
    return dict(cases=len(cases), results={str(k): v for k, v in sorted(results.items())}, slot_clearing=fails, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true",
                        help="build and bind the candidate library only; no media")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    target, code, image, decoded = source()
    report = dict(target=dict(sha256=target["sha"], bytes=497, instructions=191, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u3c610a7-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, decoded, function)
            if args.native:
                report["native"] = native_audit(target, code, image, decoded, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
