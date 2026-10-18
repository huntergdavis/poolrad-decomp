#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U25 +1951, composing SYSTEM GetMem (0AF8:023F) and U25 +06AF, plus a
DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so GetMem runs from its original
bytes, loads unit 0x25, builds synthetic Turbo Pascal heaps, list heads and tables in DS, and compares the entire 1 MiB
RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask. The original decides each
stop: the fetch after its own RETF 2 (return), a GetMem site whose live bytes differ when fetched (CODE), GetMem reaching
RunError 00E2, an indirect HeapError call through a non-default pointer, or its free-list walk past the host cap
(CALLEE), or a RET/RETF that does not come back to its call (TRANSFER). Any fetch outside the pre-decoded spans is an
error. Media stays private.
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
import system_getmem_023f_audit as g

ENTRY, END, BODY_RETF, WORD_SPAN = 0x1951, 0x19DB, 0x19D8, (0x06AF, 0x06D3)
SITES = (0x1961, 0x198F)
CALLER_CS, DS, SS = 0x8800, g.DS, g.SS
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}
FLAGS = g.FLAGS


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image, gdecoded, _, digests = g.source()
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u = units[0x25]
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[ENTRY:END]
    insns = list(md.disasm(body, ENTRY))
    word = list(md.disasm(code[WORD_SPAN[0]:WORD_SPAN[1]], WORD_SPAN[0]))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(body) == 138 and len(insns) == 47 and sum(i.size for i in insns) == 138 and insns[-1].address == BODY_RETF
          and insns[-1].op_str == "2" and calls == [(0x1961, "0xaf8, 0x23f"), (0x198F, "0xaf8, 0x23f"), (0x19D1, "0x6af")]
          and all(s + 3 in set(u["fixups"]) for s in SITES) and word[-1].mnemonic == "retf" and word[-1].op_str == "4",
          f"U25 +1951 extent or calls differ: {calls}")
    decoded = {("u25", i.address): i for i in insns + word}
    decoded.update({("sys", a): i for a, i in gdecoded.items()})
    sha = sha256(body).hexdigest()
    print(f"U25 +1951 (138/47, LCALL 0AF8:023F x2, PUSH CS; CALL +06AF, RETF 2) GAME file {u['ovr_offset'] + ENTRY:#x} sha {sha}")
    return dict(conditional=conditional, starts=[i.address for i in insns], sha=sha, file_offset=u["ovr_offset"] + ENTRY), code, image, decoded


def library(directory):
    path = directory / "u251951.so"
    sources = ["overlay_u25_alloc_1951_cpu.c", "system_getmem_023f_cpu.c", "overlay_u25_loader_0000.c", "overlay_u25_string_cores.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_u25_alloc_1951_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint,
                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code, image, rng, *, index, head=None, word=None, blocks=(), top=(g.HEAP + 0x400, 0), end=(g.HEAP + 0x1000, 0),
          heap_error=(0x00A9, g.SYSTEM_CS), sp=0x8000, ss=SS, flags=0x0602, background=None, patch=(), writes=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    g.heap(memory, blocks=blocks, top=top, end=end, heap_error=heap_error)
    head = head or (0, 0)
    put16(memory, DS, 0x6F70, head[1]); put16(memory, DS, 0x6F72, head[0])
    put16(memory, DS, 0x49ED, rng.randrange(0x10000) if word is None else word)
    for i in range(256):
        memory[physical(DS, 0x7045 + i)] = rng.randrange(256); memory[physical(DS, 0x7085 + i)] = rng.randrange(256)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, (rng.randrange(256) << 8) | index)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    for segment, offset, data in writes:
        for j, value in enumerate(data):
            memory[physical(segment, (offset + j) & 0xFFFF)] = value
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, decoded, function, cap=64, count=2000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, last=None, steps=0, calls=[], unexpected=None, sites=0)
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
                state["stop"] = CALLEE if state["calls"] else TRANSFER          # nested inside GetMem, or a direct callee
                return emulator.emu_stop()
        kind = "u25" if cs == CALLER_CS else "sys" if cs == g.SYSTEM_CS else None
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
            if state["steps"] > cap:
                state["stop"] = CALLEE
                return emulator.emu_stop()
        ins = decoded[key]
        if kind == "u25" and ENTRY <= ip < END:
            if ip in SITES:
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
            if kind == "u25" and ip == BODY_RETF:
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
    callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} (callee {callee.value}/{callee_status.value}), original stopped with "
          f"{NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, (callee.value, callee_status.value), state["sites"]


def cases(rng):
    spec = []
    for i in range(160):
        blocks, top, end = g.random_heap(rng)
        head = rng.choice((None, None, (g.HEAP + 0x900, rng.choice((0, 8))), (rng.randrange(0x4000, 0x8000), rng.randrange(16))))
        spec.append((f"random-{i}", dict(index=rng.randrange(256), head=head, blocks=blocks, top=top, end=end, flags=FLAGS[i % len(FLAGS)],
                                         sp=rng.randrange(0x0400, 0xF000) & ~1)))
    two = ((g.HEAP + 0x10, 0, 2, 0), (g.HEAP + 0x20, 8, 4, 8))
    spec.append(("empty-list", dict(index=7)))
    spec.append(("empty-list-fit", dict(index=200, blocks=two)))
    spec.append(("push-onto-head", dict(index=3, head=(g.HEAP + 0x900, 8), blocks=two)))
    spec.append(("heap-full-empty", dict(index=1, end=(g.HEAP + 0x400, 0))))
    spec.append(("heap-full-push", dict(index=1, head=(g.HEAP + 0x900, 0), end=(g.HEAP + 0x400, 0))))
    spec.append(("custom-heap-error", dict(index=1, heap_error=(0x1234, 0x5678))))
    spec.append(("cyclic-heap", dict(index=1, blocks=((g.HEAP + 0x10, 0, 0, 0),), writes=((g.HEAP + 0x10, 0, struct.pack("<HHHH", 0, g.HEAP + 0x10, 0, 0)),), cap=3)))
    spec.append(("code-site-empty", dict(index=1, patch=((0x1962, 0x40),))))
    spec.append(("code-site-push", dict(index=1, head=(g.HEAP + 0x900, 0), patch=((0x1990, 0x40),))))
    for sp in (0x0100, 0xFFE0):
        spec.append((f"sp-{sp:04x}", dict(index=9, head=(g.HEAP + 0x900, 0), sp=sp, ss=0x1000 if sp > 0x8000 else SS)))
    spec.append(("ss-a20", dict(index=9, sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(index=77, head=(g.HEAP + 0x900, 0), background=bytes(random.Random(0x251951).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(index=5, flags=0x0603)))
    return spec


def differential(target, code, image, decoded, function):
    rng = random.Random(0x251951)
    visited, edges, stops, sites = set(), set(), {k: 0 for k in NAMES}, 0
    for label, options in cases(rng):
        cap = options.pop("cap", 64)
        memory, cpu, high = build(code, image, rng, **options)
        stop, v, e, callee, s = compare(f"+1951 {label}", memory, cpu, high, target, decoded, function, cap)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} callee {callee}")
        stops[stop] += 1
        visited |= v
        edges |= e
        sites += s
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(a) for a in target["starts"] if a not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    print(f"U25 +1951: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM and "
          f"all 14 CPU words equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts, {len(edges)}/{len(reachable)} conditional "
          f"outcomes; GetMem site fetches {sites}")
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(stops[OK] and stops[CODE] and stops[CALLEE], f"a stop kind was not exercised: {stops}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges), site_fetches=sites)


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, CALLER_N, DATA_DS, ARENA_N = 0x7000, 0x8800, 0x5000, 0x6000
MAIN_AT, SPAN_AT, TABLE_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x1200, 0x1400, 0x2000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, ARENA_BYTES, SPAN_BYTES = STACK_TOP - 0x100, 0x100, 0x4000, 0x0500
CASE_SIZE, NATIVE_CASES = 40, 64
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
        head = rng.choice(((0, 0), (0, 0), (ARENA_N + 0x300, 8), (rng.randrange(0x10000), rng.randrange(16))))
        cases.append(dict(flags=n % len(NATIVE_FLAGS), index=rng.randrange(256), word=rng.randrange(0x10000), head=head, blocks=blocks,
                          top=top, end=end))
    return cases


def case_bytes(case):
    data = struct.pack("<BBH2H4HB", case["flags"], case["index"], case["word"], case["head"][1], case["head"][0], case["top"][1], case["top"][0],
                       case["end"][1], case["end"][0], len(case["blocks"]))
    for seg, paragraphs, granule in case["blocks"]:
        data += struct.pack("<HBB", seg, paragraphs, granule)
    return data + bytes(CASE_SIZE - len(data))


def native_state(memory, case, tables):
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
    put16(memory, DATA_DS, 0x6F70, case["head"][1]); put16(memory, DATA_DS, 0x6F72, case["head"][0])
    put16(memory, DATA_DS, 0x49ED, case["word"])
    at = physical(DATA_DS, 0x7045); memory[at:at + len(tables)] = tables


def native_audit(code, image, function):
    directory = ROOT / "resources/oracle-runs/u25-1951-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = image[physical(g.SYSTEM_CS, 0):physical(g.SYSTEM_CS, SPAN_BYTES)]
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, SYS_N)
    rng = random.Random(0x251951D)
    tables = bytes(rng.randrange(256) for _ in range(0x140))
    cases = native_cases(rng)
    check(SPAN_AT + len(span) <= TABLE_AT and TABLE_AT + len(tables) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= CODE_AT
          and CODE_AT + len(unit) <= OUTPUT, "COM layout overlaps")
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
          "    mov ax,word ptr [si+8]", "    mov word ptr es:[0x4404],ax", "    mov ax,word ptr [si+10]", "    mov word ptr es:[0x4406],ax",
          "    mov ax,word ptr [si+12]", "    mov word ptr es:[0x4408],ax", "    mov ax,word ptr [si+14]", "    mov word ptr es:[0x440a],ax",
          "    mov word ptr es:[0x4414],0xa9", f"    mov word ptr es:[0x4416],{SYS_N:#x}",
          "    mov ax,word ptr [si+8]", "    mov dx,word ptr [si+10]", "    cmp byte ptr [si+16],0", "    je list_head",
          "    xor ax,ax", "    mov dx,word ptr [si+17]", "list_head:", "    mov word ptr es:[0x440c],ax", "    mov word ptr es:[0x440e],dx",
          "    xor ch,ch", "    mov cl,byte ptr [si+16]", "    jcxz blocks_done", "    lea bx,[si+17]", "block_loop:",
          "    mov es,word ptr [bx]", "    xor ax,ax", "    mov dx,word ptr [bx+4]", "    cmp cx,1", "    jne block_link",
          "    mov ax,word ptr [si+8]", "    mov dx,word ptr [si+10]", "block_link:",
          "    mov word ptr es:[0],ax", "    mov word ptr es:[2],dx", "    mov al,byte ptr [bx+3]", "    xor ah,ah", "    mov word ptr es:[4],ax",
          "    mov al,byte ptr [bx+2]", "    xor ah,ah", "    mov word ptr es:[6],ax", "    add bx,4", "    loop block_loop", "blocks_done:",
          f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov ax,word ptr [si+4]", "    mov word ptr es:[0x6f70],ax", "    mov ax,word ptr [si+6]",
          "    mov word ptr es:[0x6f72],ax", "    mov ax,word ptr [si+2]", "    mov word ptr es:[0x49ed],ax",
          "    push si", "    mov si,offset tables", "    mov di,0x7045", f"    mov cx,{len(tables):#x}", "    rep movsb", "    pop si",
          "    push cs", "    pop es", "    mov al,byte ptr [si+1]", "    mov ah,0x5a", "    push ax",
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
    for label, blob, where in (("span", span, SPAN_AT), ("tables", tables, TABLE_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT),
                               ("unit", bytes(unit), CODE_AT)):
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
        native_state(memory, case, tables)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (0x5A00 | case["index"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        status = function(ram, SIZE, ctypes.byref(cpu), 64, ctypes.byref(callee), ctypes.byref(callee_status))
        trapped = struct.unpack_from("<H", record, 28)[0]
        check((status == OK and trapped == 0) or (status == CALLEE and callee_status.value == g.RUNERROR and trapped == 1),
              f"native case {n}: C status {status}/{callee_status.value}, DOS trapped {trapped}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0):physical(DATA_DS, 0) + 0x10000]) + bytes(ram[physical(ARENA_N, 0):physical(ARENA_N, 0) + ARENA_BYTES])
        if got != record[30:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[30:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        key = "return" if status == OK else "GetMem RunError"
        statuses[key] = statuses.get(key, 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({statuses}); all 14 CPU words, the {WINDOW_SIZE}-byte stack window, "
          f"the whole 64 KiB DS segment and the {ARENA_BYTES:#x}-byte heap arena; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U25 at {CALLER_N:04X}, SYSTEM at {SYS_N:04X} (RunError trapped at 00E2)")
    return dict(cases=len(cases), statuses=statuses, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image, decoded = source()
    report = dict(target=dict(sha256=target["sha"], bytes=138, instructions=47, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u251951-") as temp:
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
