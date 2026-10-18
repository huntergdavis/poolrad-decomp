#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U9E +0E52 (entry 10), composing SYSTEM GetMem (0AF8:023F), plus a
DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so GetMem runs from its original
bytes, loads unit 0x9E, builds synthetic Turbo Pascal heaps, records and linked lists, and compares the entire 1 MiB
RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask. The original decides each
stop: the fetch after its own RETF 0Ch (return), the GetMem site whose live bytes differ when fetched (CODE), GetMem
reaching RunError 00E2, an indirect HeapError call through a non-default pointer, or its free-list walk past the host
cap (CALLEE), a RET/RETF that does not come back to its call (TRANSFER), or the tail walk reaching +0EA8 once more
than the host link cap (LIMIT). Any fetch outside the pre-decoded spans is an error. Every case keeps the stack frame
apart from the heap arena, the record and the list. Media stays private.
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
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, OVR_SHA, REGISTERS, physical, put16
import system_getmem_023f_audit as g

ENTRY, END, BODY_RETF, SITE, FOLLOW = 0x0E52, 0x0F06, 0x0F03, 0x0E5C, 0x0EA8
CALLER_CS, DS, SS = 0x8800, g.DS, g.SS
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FLAGS = g.FLAGS
HEAP_CAP, LINK_CAP = 64, 64


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image, gdecoded, _, _ = g.source()
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u = units[0x9E]
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[ENTRY:END]
    insns = list(md.disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(u["entries"][10] == ENTRY and u["entries"][11] == END and len(body) == 180 and len(insns) == 57
          and sum(i.size for i in insns) == 180 and insns[-1].address == BODY_RETF and insns[-1].mnemonic == "retf"
          and insns[-1].op_str == "0xc" and calls == [(SITE, "0xaf8, 0x23f")] and SITE + 3 in set(u["fixups"])
          and FOLLOW in {i.address for i in insns}, f"U9E +0E52 extent or calls differ: {calls}")
    decoded = {("u9e", i.address): i for i in insns}
    decoded.update({("sys", a): i for a, i in gdecoded.items()})
    sha = sha256(body).hexdigest()
    print(f"U9E +0E52 entry 10 (180/57, LCALL 0AF8:023F, RETF 0Ch) GAME file {u['ovr_offset'] + ENTRY:#x} sha {sha}")
    return dict(conditional=conditional, starts=[i.address for i in insns], sha=sha, file_offset=u["ovr_offset"] + ENTRY), code, image, decoded


def library(directory):
    path = directory / "u9e0e52.so"
    sources = ["overlay_u9e_list_append_0e52_cpu.c", "system_getmem_023f_cpu.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_u9e_list_append_0e52_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.c_size_t,
                         ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code, image, rng, *, record=(0x4100, 0x0020), nodes=(), cycle=None, args=None, blocks=(), top=(g.HEAP + 0x400, 0),
          end=(g.HEAP + 0x1000, 0), heap_error=(0x00A9, g.SYSTEM_CS), sp=0x6000, ss=SS, flags=0x0602, background=None, patch=()):
    """A record whose far head (+7Fh/+81h) starts `nodes` linked through +5/+7; the last link is nil, or nodes[cycle]."""
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    g.heap(memory, blocks=blocks, top=top, end=end, heap_error=heap_error)
    head = nodes[0] if nodes else (0, 0)
    put16(memory, record[0], (record[1] + 0x7F) & 0xFFFF, head[1]); put16(memory, record[0], (record[1] + 0x81) & 0xFFFF, head[0])
    for i, (segment, offset) in enumerate(nodes):
        nxt = nodes[i + 1] if i + 1 < len(nodes) else nodes[cycle] if cycle is not None else (0, 0)
        for j in range(5):
            memory[physical(segment, (offset + j) & 0xFFFF)] = rng.randrange(256)
        put16(memory, segment, (offset + 5) & 0xFFFF, nxt[1]); put16(memory, segment, (offset + 7) & 0xFFFF, nxt[0])
    args = tuple(rng.randrange(0x10000) for _ in range(4)) if args is None else args
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, *args, record[1], record[0])):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, decoded, function, links=LINK_CAP, count=4000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, last=None, steps=0, follows=0, calls=[], unexpected=None,
                 sites=0)
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
                state["stop"] = CALLEE if state["calls"] else TRANSFER          # nested inside GetMem, or GetMem's own RETF 2
                return emulator.emu_stop()
        kind = "u9e" if cs == CALLER_CS else "sys" if cs == g.SYSTEM_CS else None
        if kind == "sys" and ip == g.RUNERROR_IP:
            state["stop"] = CALLEE
            return emulator.emu_stop()
        key = (kind, ip)
        if key not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if kind == "sys" and ip in g.HEAP_ERROR_SITES and bytes(emulator.mem_read(physical(cpu.ds, 0x4414), 4)) != struct.pack("<HH", 0x00A9, g.SYSTEM_CS):
            state["stop"] = CALLEE
            return emulator.emu_stop()
        if kind == "sys" and ip == g.WALK_HEAD and state["last"] in (("sys", a) for a in g.WALK_JAS):
            state["steps"] += 1
            if state["steps"] > HEAP_CAP:
                state["stop"] = CALLEE
                return emulator.emu_stop()
        if kind == "u9e" and ip == FOLLOW:
            state["follows"] += 1
            if state["follows"] > links:
                state["stop"] = LIMIT
                return emulator.emu_stop()
        ins = decoded[key]
        if kind == "u9e":
            if ip == SITE:
                if bytes(emulator.mem_read(address, 3)) != b"\x9a\x3f\x02":
                    state["stop"] = CODE
                    return emulator.emu_stop()
                state["sites"] += 1
            visited.add(ip)
            if state["previous"] in target["conditional"] and state["previous"] != ip:
                edges.add((state["previous"], ip))
            state["previous"] = ip
        else:
            state["previous"] = None
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf"):
            if kind == "u9e" and ip == BODY_RETF:
                state["returned"] = True
            else:
                state["returning"] = True
        state["last"] = key
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(CALLER_CS, ENTRY), 2 * SIZE, count=count)
    check(state["unexpected"] is None, f"original left the decoded code at {state['unexpected']}: {label}")
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    getmem_status = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), HEAP_CAP, links, ctypes.byref(getmem_status))
    check(status == state["stop"], f"C status {status} (GetMem {getmem_status.value}), original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, getmem_status.value, state["sites"]


def random_nodes(rng, count, low=0x5000, high=0x7000):
    return [(rng.randrange(low, high), rng.choice((rng.randrange(16), rng.randrange(0xFFF0)))) for _ in range(count)]


def cases(rng):
    spec = []
    for i in range(160):
        blocks, top, end = g.random_heap(rng)
        record = (rng.randrange(0x4000, 0x5000), rng.choice((rng.randrange(16), rng.randrange(0xFF70))))
        spec.append((f"random-{i}", dict(record=record, nodes=random_nodes(rng, rng.choice((0, 0, 1, 1, 2, 3, 5, 8))), blocks=blocks, top=top,
                                         end=end, flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0x7000) & ~1)))
    three = [(0x5100, 0x0000), (0x5200, 0x0008), (0x5300, 0x0004)]
    five = [(0x5100 + 0x10 * k, k) for k in range(5)]
    two = ((g.HEAP + 0x10, 0, 2, 0), (g.HEAP + 0x20, 8, 4, 8))
    spec.append(("empty-list", dict()))
    spec.append(("empty-list-fit", dict(blocks=two)))
    spec.append(("one-node", dict(nodes=[(0x5100, 0x0000)])))
    spec.append(("three-nodes", dict(nodes=three, blocks=two)))
    spec.append(("forty-nodes", dict(nodes=[(0x5000 + 0x20 * k, k % 16) for k in range(40)])))
    spec.append(("cap-exact", dict(nodes=five, links=4)))
    spec.append(("cap-limit", dict(nodes=five, links=3)))
    spec.append(("cyclic-list", dict(nodes=three, cycle=0)))
    spec.append(("self-loop", dict(nodes=[(0x5100, 0x0000)], cycle=0, links=5)))
    spec.append(("record-wrap", dict(record=(0x4100, 0xFFC0), nodes=three)))
    spec.append(("record-wrap-empty", dict(record=(0x4100, 0xFFC0))))
    spec.append(("node-wrap", dict(nodes=[(0x5100, 0xFFFC), (0x5200, 0xFFFB)])))
    spec.append(("heap-full-empty", dict(end=(g.HEAP + 0x400, 0))))
    spec.append(("heap-full-list", dict(nodes=three, end=(g.HEAP + 0x400, 0))))
    spec.append(("custom-heap-error", dict(nodes=three, end=(g.HEAP + 0x400, 0), heap_error=(0x1234, 0x5678))))
    spec.append(("code-opcode", dict(patch=((SITE, 0xEA),))))
    spec.append(("code-offset", dict(nodes=three, patch=((SITE + 1, 0x40),))))
    spec.append(("sp-0100", dict(nodes=three, sp=0x0100)))
    spec.append(("sp-high", dict(nodes=three, sp=0xFFE0, ss=0x1000)))
    spec.append(("frame-wrap", dict(nodes=three, sp=0xFFF4, ss=0x1000)))
    spec.append(("locals-wrap", dict(nodes=three, sp=0x0006, ss=0x1000)))
    spec.append(("code-locals-wrap", dict(nodes=three, sp=0x0006, ss=0x1000, patch=((SITE, 0xEA),))))
    spec.append(("ss-a20", dict(nodes=three, sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(nodes=three, background=bytes(random.Random(0x9E0E52).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(nodes=three, flags=0x0603)))
    return spec


def differential(target, code, image, decoded, function):
    rng = random.Random(0x9E0E52)
    visited, edges, stops, sites = set(), set(), {k: 0 for k in NAMES}, 0
    for label, options in cases(rng):
        links = options.pop("links", LINK_CAP)
        memory, cpu, high = build(code, image, rng, **options)
        stop, v, e, getmem_status, s = compare(f"+0E52 {label}", memory, cpu, high, target, decoded, function, links)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} GetMem {getmem_status}")
        stops[stop] += 1
        visited |= v
        edges |= e
        sites += s
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(a) for a in target["starts"] if a not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    print(f"U9E +0E52: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM and "
          f"all 14 CPU words equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts, {len(edges)}/{len(reachable)} conditional "
          f"outcomes; GetMem site fetches {sites}")
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(stops[OK] and stops[CODE] and stops[CALLEE] and stops[LIMIT], f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges), site_fetches=sites)


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, CALLER_N, DATA_DS, ARENA_N = 0x7000, 0x8800, 0x5000, 0x6000
MAIN_AT, SPAN_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x1200, 0x6000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, ARENA_BYTES, SPAN_BYTES = STACK_TOP - 0x100, 0x100, 0x4000, 0x0500
CASE_SIZE, NATIVE_CASES, MAX_WRITES = 112, 64, 16
RECORD_BYTES = 30 + WINDOW_SIZE + 0x10000 + ARENA_BYTES
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        blocks, segment = [], ARENA_N
        for _ in range(rng.choice((0, 1, 2, 3))):
            segment += rng.randrange(1, 30)
            paragraphs, granule = rng.randrange(0, 4), rng.choice((0, 8))
            if paragraphs == 0 and granule == 0:
                granule = 8
            blocks.append((segment, paragraphs, granule))
            segment += paragraphs + 1
        top = (segment + rng.randrange(1, 30), rng.choice((0, 8)))
        end = (top[0] + rng.choice((0, 0, 1, 30)), rng.choice((0, 8)))
        slots = rng.sample(range(0x10, 0x3F0), rng.choice((0, 0, 1, 2, 3, 5, 7)))
        nodes = [slot * 0x10 + rng.randrange(8) for slot in slots]
        record = rng.choice((rng.randrange(0x6000, 0xF000), rng.randrange(0x6000, 0xF000), 0xFFC0))
        writes = [((record + 0x7F) & 0xFFFF, nodes[0] if nodes else 0), ((record + 0x81) & 0xFFFF, DATA_DS if nodes else 0)]
        for i, node in enumerate(nodes):
            nxt = nodes[i + 1] if i + 1 < len(nodes) else None
            writes += [(node + 5, 0 if nxt is None else nxt), (node + 7, 0 if nxt is None else DATA_DS)]
        check(len(writes) <= MAX_WRITES, "too many native writes")
        cases.append(dict(flags=n % len(NATIVE_FLAGS), args=tuple(rng.randrange(0x10000) for _ in range(4)), record=record, writes=writes,
                          blocks=blocks, top=top, end=end))
    return cases


def case_bytes(case):
    data = struct.pack("<BB4H5HH", case["flags"], len(case["blocks"]), *case["args"], case["record"], case["top"][1], case["top"][0],
                       case["end"][1], case["end"][0], len(case["writes"]))
    blocks = b"".join(struct.pack("<HBB", seg, paragraphs, granule) for seg, paragraphs, granule in case["blocks"])
    data += blocks + bytes(12 - len(blocks))
    data += b"".join(struct.pack("<HH", offset, value) for offset, value in case["writes"])
    check(len(data) <= CASE_SIZE, "native case too large")
    return data + bytes(CASE_SIZE - len(data))


def native_state(memory, case):
    at = physical(DATA_DS, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
    at = physical(ARENA_N, 0); memory[at:at + ARENA_BYTES] = bytes([0xA5]) * ARENA_BYTES
    put16(memory, DATA_DS, 0x4400, 0); put16(memory, DATA_DS, 0x4402, ARENA_N)
    put16(memory, DATA_DS, 0x4404, case["top"][1]); put16(memory, DATA_DS, 0x4406, case["top"][0])
    put16(memory, DATA_DS, 0x4408, case["end"][1]); put16(memory, DATA_DS, 0x440A, case["end"][0])
    put16(memory, DATA_DS, 0x4414, 0x00A9); put16(memory, DATA_DS, 0x4416, SYS_N)
    first = (0, case["blocks"][0][0]) if case["blocks"] else (case["top"][1], case["top"][0])
    put16(memory, DATA_DS, 0x440C, first[0]); put16(memory, DATA_DS, 0x440E, first[1])
    for i, (seg, paragraphs, granule) in enumerate(case["blocks"]):
        nxt = (0, case["blocks"][i + 1][0]) if i + 1 < len(case["blocks"]) else (case["top"][1], case["top"][0])
        put16(memory, seg, 0, nxt[0]); put16(memory, seg, 2, nxt[1]); put16(memory, seg, 4, granule); put16(memory, seg, 6, paragraphs)
    for offset, value in case["writes"]:
        put16(memory, DATA_DS, offset, value)


def native_audit(code, image, function):
    directory = ROOT / "resources/oracle-runs/u9e-0e52-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = image[physical(g.SYSTEM_CS, 0):physical(g.SYSTEM_CS, SPAN_BYTES)]
    unit = bytearray(code)
    struct.pack_into("<H", unit, SITE + 3, SYS_N)
    rng = random.Random(0x9E0E52D)
    cases = native_cases(rng)
    check(SPAN_AT + len(span) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= CODE_AT and CODE_AT + len(unit) <= OUTPUT,
          "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, label, length in ((SYS_N, "span", len(span)), (CALLER_N, "unit", len(unit))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", "    xor di,di", f"    mov cx,{length:#x}", "    rep movsb"]
    t += [f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov byte ptr es:[0xe2],0xea", "    mov word ptr es:[0xe3],offset trap", "    mov word ptr es:[0xe5],cs",
          "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
          "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
          "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
          "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
          "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
          f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{ARENA_N:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{ARENA_BYTES // 2:#x}", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x4400],0", f"    mov word ptr es:[0x4402],{ARENA_N:#x}",
          "    mov ax,word ptr [si+12]", "    mov word ptr es:[0x4404],ax", "    mov ax,word ptr [si+14]", "    mov word ptr es:[0x4406],ax",
          "    mov ax,word ptr [si+16]", "    mov word ptr es:[0x4408],ax", "    mov ax,word ptr [si+18]", "    mov word ptr es:[0x440a],ax",
          "    mov word ptr es:[0x4414],0xa9", f"    mov word ptr es:[0x4416],{SYS_N:#x}",
          "    mov ax,word ptr [si+12]", "    mov dx,word ptr [si+14]", "    cmp byte ptr [si+1],0", "    je list_head",
          "    xor ax,ax", "    mov dx,word ptr [si+22]", "list_head:", "    mov word ptr es:[0x440c],ax", "    mov word ptr es:[0x440e],dx",
          "    xor ch,ch", "    mov cl,byte ptr [si+1]", "    jcxz blocks_done", "    lea bx,[si+22]", "block_loop:",
          "    mov es,word ptr [bx]", "    xor ax,ax", "    mov dx,word ptr [bx+4]", "    cmp cx,1", "    jne block_link",
          "    mov ax,word ptr [si+12]", "    mov dx,word ptr [si+14]", "block_link:",
          "    mov word ptr es:[0],ax", "    mov word ptr es:[2],dx", "    mov al,byte ptr [bx+3]", "    xor ah,ah", "    mov word ptr es:[4],ax",
          "    mov al,byte ptr [bx+2]", "    xor ah,ah", "    mov word ptr es:[6],ax", "    add bx,4", "    loop block_loop", "blocks_done:",
          f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov cx,word ptr [si+20]", "    jcxz writes_done", "    lea bx,[si+34]", "write_loop:",
          "    mov di,word ptr [bx]", "    mov ax,word ptr [bx+2]", "    mov word ptr es:[di],ax", "    add bx,4", "    loop write_loop", "writes_done:",
          "    push cs", "    pop es", f"    mov ax,{DATA_DS:#x}", "    push ax", "    push word ptr [si+10]", "    push word ptr [si+8]",
          "    push word ptr [si+6]", "    push word ptr [si+4]", "    push word ptr [si+2]",
          "    mov bl,byte ptr [si]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        t.append(f"    mov {name},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:", "    mov word ptr cs:[status],0", "    jmp record",
          "trap:", "    mov word ptr cs:[status],1", "record:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds", f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cmp word ptr cs:[status],0", "    jne trapped",
          f"    mov word ptr cs:[{OUTPUT + 16:#x}],cs", f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", "    jmp segments",
          "trapped:", f"    mov word ptr cs:[{OUTPUT + 16:#x}],{SYS_N:#x}", f"    mov word ptr cs:[{OUTPUT + 18:#x}],0xe2", "segments:",
          "    mov ax,word ptr cs:[status]", f"    mov word ptr cs:[{OUTPUT + 28:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 30:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{30 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{ARENA_N:#x}", "    mov ds,ax", "    xor dx,dx", f"    mov cx,{ARENA_BYTES:#x}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    push cs", "    pop ds", "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "status: .word 0",
          f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("span", span, SPAN_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT), ("unit", bytes(unit), CODE_AT)):
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
    statuses = {}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(SYS_N, 0); memory[at:at + len(span)] = span
        memory[physical(SYS_N, 0xE2):physical(SYS_N, 0xE2) + 5] = struct.pack("<BHH", 0xEA, 0, 0)
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        native_state(memory, case)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (DATA_DS, case["record"], case["args"][3], case["args"][2], case["args"][1], case["args"][0], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        getmem_status = ctypes.c_int(99)
        status = function(ram, SIZE, ctypes.byref(cpu), HEAP_CAP, LINK_CAP, ctypes.byref(getmem_status))
        trapped = struct.unpack_from("<H", record, 28)[0]
        check((status == OK and trapped == 0) or (status == CALLEE and getmem_status.value == g.RUNERROR and trapped == 1),
              f"native case {n}: C status {status}/{getmem_status.value}, DOS trapped {trapped}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0):physical(DATA_DS, 0) + 0x10000]) + bytes(ram[physical(ARENA_N, 0):physical(ARENA_N, 0) + ARENA_BYTES])
        if got != record[30:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[30:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        key = "return" if status == OK else "GetMem RunError"
        statuses[key] = statuses.get(key, 0) + 1
    lists = sum(1 for c in cases if c["writes"][0][1])
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({statuses}; {lists} with a non-empty list); all 14 CPU words, "
          f"the {WINDOW_SIZE}-byte stack window, the whole 64 KiB DS segment and the {ARENA_BYTES:#x}-byte heap arena; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U9E at {CALLER_N:04X}, SYSTEM at {SYS_N:04X} (RunError trapped at 00E2)")
    return dict(cases=len(cases), statuses=statuses, lists=lists, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image, decoded = source()
    report = dict(target=dict(sha256=target["sha"], bytes=180, instructions=57, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u9e0e52-") as temp:
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
