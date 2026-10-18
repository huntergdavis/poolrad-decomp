#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U7D +0000 (party list timer) up to its unmodeled U9E +0026 call, composing
SYSTEM FillChar 16A8, plus a DOSBox-X oracle on returning paths.

The differential places the hash-checked expanded START.EXE image at load segment 0 (FillChar runs from its original
bytes), loads unit 0x7D, builds synthetic party lists, per-member flags, thresholds and member node lists, and compares
the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask. The original
decides each stop: the fetch after its RETF 4 (return), the +01FE LCALL to U9E +0026 fetched with its original bytes
(PENDING), a reached LCALL whose live bytes differ (CODE), or a fetch after more taken backward jumps than the host cap
(LIMIT). The 17 instructions after +01FE run only after U9E +0026 returns and are outside this model. Every case keeps
its stack frame apart from the data. Media stays private.
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

SYSTEM_CS, CALLER_CS = 0x0AF8, 0x8800
ENTRY, END, BODY_RETF, FILL_SITE, U9E_SITE = 0x0000, 0x02F2, 0x02EF, 0x0019, 0x01FE
BACKWARD = {0x0051: 0x0028, 0x008C: 0x006A, 0x0155: 0x0137, 0x0289: 0x024B, 0x028B: 0x015B, 0x02BD: 0x028E, 0x02D5: 0x00BA,
            0x02E9: 0x008E}
AFTER_CALL = range(0x0203, 0x0239)
FILL_SPAN = (0x16A8, 0x16BC, 7)
DS, SS = 0x1800, 0x2800
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER, PENDING, LIMIT = 0, 2, 3, 4, 5, 6
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", PENDING: "PENDING", LIMIT: "LIMIT"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "media provenance differs")
    image = bytes(unpack(exe)[0])
    u = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x7D)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[ENTRY:END]
    insns = list(md.disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    check(len(insns) == 248 and sum(i.size for i in insns) == len(body) and insns[-1].address == BODY_RETF and insns[-1].op_str == "4"
          and calls == [(FILL_SITE, "0xaf8, 0x16a8"), (U9E_SITE, "0xb0, 0x2a")]
          and all(insns_at.mnemonic.startswith("j") for insns_at in insns if insns_at.address in BACKWARD), f"U7D +0000 differs: {calls}")
    fill = list(md.disasm(image[physical(SYSTEM_CS, FILL_SPAN[0]):physical(SYSTEM_CS, FILL_SPAN[1])], FILL_SPAN[0]))
    check(len(fill) == FILL_SPAN[2], "FillChar differs")
    decoded = {("u7d", i.address): i for i in insns}
    decoded.update({("sys", i.address): i for i in fill})
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns
                   if i.mnemonic.startswith("j") and i.mnemonic != "jmp" and i.address not in AFTER_CALL}
    starts = [i.address for i in insns if i.address not in AFTER_CALL]
    sha = sha256(body).hexdigest()
    print(f"U7D +0000 (754/248, LCALL 0AF8:16A8 at +0019, LCALL 00B0:002A at +01FE, RETF 4) GAME file {u['ovr_offset']:#x} sha {sha}")
    return dict(conditional=conditional, starts=starts, sha=sha), code, image, decoded


def library(directory):
    path = directory / "u7d0000.so"
    sources = ["overlay_u7d_list_timer_0000_cpu.c", "system_runtime_leaves_cpu.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_u7d_list_timer_0000_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint]
    function.restype = ctypes.c_int
    return function


def world(memory, rng, *, ds, mode, flags, count, thresholds, members, cycle_members=False, node_cycle=None):
    """members: [(segment, offset, [(nseg, noff, type, duration), ...]), ...]; links are written in order."""
    memory[physical(ds, 0x49F3)] = mode
    record = (0x6000, rng.randrange(0x0000, 0x8000))
    put16(memory, ds, 0x49D6, record[1]); put16(memory, ds, 0x49D8, record[0])
    put16(memory, record[0], (record[1] + 0x67C) & 0xFFFF, count)
    for i, value in enumerate(thresholds):
        put16(memory, ds, 0x363A + 2 * i, value)
    head = (members[0][0], members[0][1]) if members else (0, 0)
    put16(memory, ds, 0x5D96, head[1]); put16(memory, ds, 0x5D98, head[0])
    for m, (seg, off, nodes) in enumerate(members):
        nxt = members[m + 1][:2] if m + 1 < len(members) else (members[0][:2] if cycle_members else (0, 0))
        put16(memory, seg, (off + 0x104) & 0xFFFF, nxt[1]); put16(memory, seg, (off + 0x106) & 0xFFFF, nxt[0])
        first = nodes[0][:2] if nodes else (0, 0)
        put16(memory, seg, (off + 0x7F) & 0xFFFF, first[1]); put16(memory, seg, (off + 0x81) & 0xFFFF, first[0])
        for k, (nseg, noff, kind, duration) in enumerate(nodes):
            link = nodes[k + 1][:2] if k + 1 < len(nodes) else (nodes[node_cycle][:2] if node_cycle is not None else (0, 0))
            memory[physical(nseg, noff)] = kind
            put16(memory, nseg, (noff + 1) & 0xFFFF, duration)
            put16(memory, nseg, (noff + 5) & 0xFFFF, link[1]); put16(memory, nseg, (noff + 7) & 0xFFFF, link[0])
    for i, value in enumerate(flags):
        memory[physical(ds, 0x471D + i)] = value


def build(code, image, rng, *, u, n, sp=0x4000, ss=SS, flags_word=0x0602, patch=(), background=None, **state):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    world(memory, rng, ds=DS, **state)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, (rng.randrange(256) << 8) | u, (rng.randrange(256) << 8) | n)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags_word), ss >= 0xF000


def compare(label, memory, cpu, high, target, decoded, function, cap):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, calls=[], unexpected=None, loopbacks=0)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["returning"]:
            state["returning"] = False
            if (cs, ip) != state["calls"].pop():
                state["stop"] = TRANSFER
                return emulator.emu_stop()
        kind = "u7d" if cs == CALLER_CS else "sys" if cs == SYSTEM_CS else None
        key = (kind, ip)
        if key not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if kind == "u7d":
            if state["previous"] in BACKWARD and ip == BACKWARD[state["previous"]]:
                state["loopbacks"] += 1
                if state["loopbacks"] > cap:
                    state["stop"] = LIMIT
                    return emulator.emu_stop()
            if ip == FILL_SITE and bytes(emulator.mem_read(address, 3)) != b"\x9a\xa8\x16":
                state["stop"] = CODE
                return emulator.emu_stop()
            if ip == U9E_SITE:
                state["stop"] = PENDING if bytes(emulator.mem_read(address, 3)) == b"\x9a\x2a\x00" else CODE
                return emulator.emu_stop()
            visited.add(ip)
            if state["previous"] in target["conditional"] and state["previous"] != ip:
                edges.add((state["previous"], ip))
            state["previous"] = ip
        else:
            state["previous"] = None
        ins = decoded[key]
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf"):
            if key == ("u7d", BODY_RETF):
                state["returned"] = True
            else:
                state["returning"] = True
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(CALLER_CS, ENTRY), 2 * SIZE, count=3000000)
    check(state["unexpected"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), cap)
    check(status == state["stop"], f"C status {NAMES.get(status, status)}, original {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges


def random_state(rng, *, expire=None, members=None):
    same = rng.random() < 0.5
    count = rng.choice((1, 2, 3, 6, 10, 0x47))
    mode = rng.choice((2, 2, 2, 0, 1, 7))
    flags = [rng.choice((0, 0, 1, 2)) for _ in range(0x60)]
    thresholds = [rng.choice((1, 2, 6, 10, 60, rng.randrange(0x100))) for _ in range(7)]
    if members is None:
        members = []
        for m in range(rng.randrange(0, 7)):
            seg, off = 0x3000 + 0x400 * m, rng.randrange(0x0000, 0x2000, 2)
            nodes = []
            for k in range(rng.randrange(0, 6)):
                duration = rng.choice((0, 0x7FFF, 0xFFFF, rng.randrange(0x10000))) if expire is False else rng.choice((0, 1, 5, 10, 50, rng.randrange(0x10000)))
                nseg, noff = (0x5000 + 0x200 * m, 0x40 * k + rng.randrange(0x10)) if same else (0x5000 + 0x200 * m + 0x40 * k, rng.randrange(0x10))
                nodes.append((nseg, noff, rng.randrange(256), duration))
            members.append((seg, off, nodes))
    return dict(mode=mode, flags=flags, count=count, thresholds=thresholds, members=members)


def cases(rng):
    spec = []
    for i in range(200):
        spec.append((f"random-{i}", dict(u=rng.choice((0, 1, 1, 2, 3, 0x80, 0xFF)), n=rng.choice((0, 1, 5, 10, 25, 200, rng.randrange(256))),
                                         flags_word=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0x7000) & ~1,
                                         **random_state(rng, expire=rng.choice((False, None))))))
    two = [(0x3000, 0x0100, [(0x5000, 0x0, 1, 100), (0x5040, 0x0, 2, 0), (0x5080, 0x0, 3, 7)]), (0x3400, 0x0200, [(0x5200, 0x0, 4, 30)])]
    base = dict(mode=2, flags=[0] + [1] * 8 + [0] * 0x57, count=2, thresholds=[1] * 7)
    spec.append(("expire-pending", dict(u=1, n=10, members=two, **{k: v for k, v in base.items()})))
    spec.append(("decrement-return", dict(u=1, n=5, members=[(0x3000, 0x0100, [(0x5000, 0, 1, 100), (0x5040, 0, 2, 0)])],
                                           **{k: v for k, v in base.items()})))
    spec.append(("nothing-flagged", dict(u=1, n=5, members=two, **dict(base, flags=[0] * 0x60))))
    spec.append(("fillchar-path", dict(u=1, n=3, members=[(0x3000, 0x0100, [(0x5000, 0, 1, 500)])], **dict(base, mode=0))))
    spec.append(("amount-zero", dict(u=1, n=0, members=two, **base)))
    spec.append(("units-multiply", dict(u=4, n=3, members=[(0x3000, 0x0100, [(0x5000, 0, 1, 60000)])], **dict(base, thresholds=[60, 24, 7, 30, 12, 9, 2]))))
    spec.append(("units-signed", dict(u=0xF0, n=3, members=two, **base)))
    spec.append(("cyclic-members", dict(u=1, n=20, members=[(0x3000, 0x0100, [(0x5000, 0, 1, 60000)]), (0x3400, 0x0200, [])], cap=40,
                                         **dict(base, cycle_members=True))))
    spec.append(("cyclic-nodes", dict(u=1, n=20, members=[(0x3000, 0x0100, [(0x5000, 0, 1, 60000), (0x5040, 0, 2, 60000)])], cap=30,
                                       **dict(base, node_cycle=0))))
    alias = [(0x3000, 0x0100, [(0x0000, 0x0001, 1, 100), (0x5000, 0x0000, 2, 0), (DS, 0x4719, 3, 500)])]
    spec.append(("flag-link-alias", dict(u=1, n=5, members=alias, cap=200, **dict(base, flags=[0, 1, 0, 0, 0] + [0] * 0x5B, count=1))))
    spec.append(("cap-zero", dict(u=1, n=20, members=two, cap=0, **base)))
    spec.append(("code-fillchar", dict(u=1, n=3, members=two, patch=((FILL_SITE + 1, 0x00),), **dict(base, mode=0))))
    spec.append(("code-u9e", dict(u=1, n=10, members=two, patch=((U9E_SITE + 1, 0x2B),), **base)))
    spec.append(("ss-a20", dict(u=1, n=5, members=two, sp=0x0800, ss=0xFFFF, **base)))
    spec.append(("df-set", dict(u=1, n=5, members=two, flags_word=0x0603, **base)))
    spec.append(("random-background", dict(u=2, n=9, members=two, background=bytes(random.Random(0x7D0000).randbytes(SIZE)), **base)))
    return spec


def differential(target, code, image, decoded, function):
    rng = random.Random(0x7D0000)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 4000)
        memory, cpu, high = build(code, image, rng, **options)
        stop, v, e = compare(f"+0000 {label}", memory, cpu, high, target, decoded, function, cap)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]}")
        stops[stop] += 1
        visited |= v
        edges |= e
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs} - {(0x0251, 0x028B)}
    missing = sorted(hex(a) for a in target["starts"] if a not in visited and a != U9E_SITE)
    print(f"U7D +0000: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM "
          f"and all 14 CPU words equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts before the call, "
          f"{len(edges & reachable)}/{len(reachable)} conditional outcomes")
    check(not missing and reachable <= edges, f"coverage incomplete: {missing} {sorted(reachable - edges)}")
    check(stops[OK] and stops[PENDING] and stops[CODE] and stops[LIMIT], f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges & reachable))


# ---- DOSBox-X synthetic oracle (returning paths) -------------------------------------------------------
SYS_N, CALLER_N, DATA_N = 0x7000, 0x8800, 0x5000
MAIN_AT, CASES_AT, CODE_AT, SPAN_AT = 0x0300, 0x0C00, 0x7000, 0x8000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x60, 0x60
NATIVE_CASES, CASE_SIZE = 48, 0x200
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_world(rng):
    """One data segment (DS = DATA_N) holding every structure; durations of 0 or at least 256 so nothing expires."""
    data = bytearray([0xA5]) * 0x10000
    def w16(offset, value):
        data[offset & 0xFFFF] = value & 0xFF; data[(offset + 1) & 0xFFFF] = value >> 8
    data[0x49F3] = rng.choice((2, 2, 0, 1))
    for i in range(0x60):
        data[0x471D + i] = rng.choice((0, 1, 1, 2))
    w16(0x49D6, 0x8000); w16(0x49D8, DATA_N); w16(0x8000 + 0x67C, rng.choice((1, 3, 6)))
    for i in range(7):
        w16(0x363A + 2 * i, rng.choice((1, 2, 3)))
    members = [0x9000 + 0x300 * m for m in range(rng.randrange(0, 5))]
    w16(0x5D96, members[0] if members else 0); w16(0x5D98, DATA_N if members else 0)
    for m, base in enumerate(members):
        nxt = members[m + 1] if m + 1 < len(members) else 0
        w16(base + 0x104, nxt); w16(base + 0x106, DATA_N if nxt else 0)
        # at least one node: +0118 reads head->link before the nil test, and a nil head would read the live DOS vector
        # table at 0000:0005 (the Unicorn half covers empty lists)
        nodes = [0xD000 + 0x200 * m + 0x20 * k for k in range(rng.randrange(1, 4))]
        w16(base + 0x7F, nodes[0] if nodes else 0); w16(base + 0x81, DATA_N if nodes else 0)
        for k, node in enumerate(nodes):
            data[node] = rng.randrange(256)
            w16(node + 1, rng.choice((0, rng.randrange(256, 0x10000))))
            link = nodes[k + 1] if k + 1 < len(nodes) else 0
            w16(node + 5, link); w16(node + 7, DATA_N if link else 0)
    return data


def native_audit(code, image, function):
    directory = ROOT / "resources/oracle-runs/u7d-0000-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    struct.pack_into("<H", unit, FILL_SITE + 3, SYS_N)
    span = image[physical(SYSTEM_CS, 0x1600):physical(SYSTEM_CS, 0x1700)]
    rng = random.Random(0x7D0000D)
    cases = []
    while len(cases) < NATIVE_CASES:
        data, u, n, flags = native_world(rng), rng.choice((0, 1, 1, 2, 3)), rng.randrange(256), len(cases) % len(NATIVE_FLAGS)
        memory = bytearray(SIZE)
        at = physical(SYS_N, 0x1600); memory[at:at + len(span)] = span
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
        cpu = Cpu(*ENTRY_REGS.values(), STACK_TOP - 8, CALLER_N, ENTRY, DATA_N, 0x0814, 0x0814, NATIVE_FLAGS[flags])
        put16(memory, 0x0814, STACK_TOP - 4, 0x5A00 | u); put16(memory, 0x0814, STACK_TOP - 2, 0x5A00 | n)
        put16(memory, 0x0814, STACK_TOP - 8, 0); put16(memory, 0x0814, STACK_TOP - 6, 0x0814)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        if function(ram, SIZE, ctypes.byref(cpu), 100000) != OK:
            continue
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        if 4 + 4 * len(writes) > CASE_SIZE:
            continue
        cases.append(dict(u=u, n=n, flags=flags, data=data, writes=writes))
    table = b"".join((struct.pack("<BBH", c["flags"], 0, len(c["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"])).ljust(CASE_SIZE, b"\0")
                     + struct.pack("<HH", 0x5A00 | c["u"], 0x5A00 | c["n"]) for c in cases)
    size = CASE_SIZE + 4
    check(CASES_AT + len(table) <= CODE_AT and CODE_AT + len(unit) <= SPAN_AT and SPAN_AT + len(span) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, label, offset, length in ((SYS_N, "span", 0x1600, len(span)), (CALLER_N, "unit", 0, len(unit))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    t += ["    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
          "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
          "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
          "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
          "    mov ax,word ptr [index]", f"    mov bx,{size}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
          f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
          "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+4]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
          "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
          f"    push word ptr [bx+{CASE_SIZE + 2}]", f"    push word ptr [bx+{CASE_SIZE}]",
          "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
          "    push word ptr [flags_table+si]", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("cases", table, CASES_AT), ("unit", bytes(unit), CODE_AT), ("span", span, SPAN_AT)):
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
    fill = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(SYS_N, 0x1600); memory[at:at + len(span)] = span
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = case["data"]
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (0x5A00 | case["n"], 0x5A00 | case["u"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 100000) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        fill += case["data"][0x49F3] != 2
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic returning cases passed: {len(cases)} ({fill} through FillChar); all 14 CPU words, the stack "
          f"window and the whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U7D at {CALLER_N:04X}, FillChar span at {SYS_N:04X}")
    return dict(cases=len(cases), fillchar=fill, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image, decoded = source()
    report = dict(target=dict(sha256=target["sha"], bytes=754, instructions=248))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u7d0000-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, decoded, function)
            if args.native:
                report["native"] = native_audit(code, image, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
