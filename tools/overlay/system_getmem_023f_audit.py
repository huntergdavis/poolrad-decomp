#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU SYSTEM GetMem (0AF8:023F with its core 0308, rounding 0491 and default
HeapError 00A9), plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so GetMem runs from its original
bytes at 0AF8, builds synthetic Turbo Pascal heaps in DS and a heap arena, and compares the entire 1 MiB RAM and all 14
CPU words between the original under Unicorn and the C model, with no mask. The original decides each stop: the fetch
after its own RETF 2 (return), the fetch of RunError at 00E2 (RUNERROR), the fetch of an indirect HeapError LCALL whose
live pointer is not the default 0AF8:00A9 (HEAP_ERROR), the free-list walk's taken JA +0319 past the host cap (LIMIT), or
a RET/RETF that does not come back to its call (TRANSFER). Any fetch outside the pre-decoded spans is an error.
Media stays private.
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
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

SYSTEM_CS, ENTRY = 0x0AF8, 0x023F
SPANS = ((0x023F, 0x0254), (0x0308, 0x03D3), (0x0491, 0x04A2), (0x00A9, 0x00AE))
ENTRY_RETF, RUNERROR_IP, HEAP_ERROR_SITES = 0x024B, 0x00E2, (0x03B7, 0x03C3)
WALK_HEAD, WALK_JAS = 0x0319, (0x032C, 0x0334)
DS, SS, HEAP = 0x1800, 0x2800, 0x3000
RET_CS, RET_IP = 0x9001, 0x7000
OK, RUNERROR, HEAP_ERROR, TRANSFER, LIMIT = 0, 2, 3, 4, 5
NAMES = {OK: "return", RUNERROR: "RUNERROR", HEAP_ERROR: "HEAP_ERROR", TRANSFER: "TRANSFER", LIMIT: "LIMIT"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    decoded, digests = {}, {}
    for start, end in SPANS:
        span = image[physical(SYSTEM_CS, start):physical(SYSTEM_CS, end)]
        insns = list(md.disasm(span, start))
        check(sum(i.size for i in insns) == len(span), f"SYSTEM {start:#x} does not decode cleanly")
        decoded.update({i.address: i for i in insns})
        digests[f"{start:04X}-{end - 1:04X}"] = (len(span), len(insns), sha256(span).hexdigest())
    check(decoded[ENTRY_RETF].mnemonic == "retf" and decoded[ENTRY_RETF].op_str == "2" and decoded[0x0245].op_str == "0x308"
          and decoded[0x030F].op_str == "0x491" and decoded[0x0251].mnemonic == "jmp" and decoded[0x0251].op_str == "0xe2"
          and all(decoded[s].mnemonic == "lcall" and decoded[s].op_str == "[0x4414]" for s in HEAP_ERROR_SITES)
          and image[physical(SYSTEM_CS, 0x0037):physical(SYSTEM_CS, 0x0037) + 6] == b"\xc7\x06\x14\x44\xa9\x00",
          "GetMem spans, calls or the default HeapError install differ")
    conditional = {a: (a + i.size, int(i.op_str, 16)) for a, i in decoded.items() if i.mnemonic.startswith("j") and i.mnemonic != "jmp"
                   and SPANS[0][0] <= a < SPANS[1][1]}
    print("GetMem spans:", {k: (n, c, d[:16]) for k, (n, c, d) in digests.items()}, f"{len(conditional)} conditional branches")
    return image, decoded, conditional, digests


def library(directory):
    path = directory / "getmem.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/system_getmem_023f_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_system_getmem_023f
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint]
    function.restype = ctypes.c_int
    return function


def heap(memory, *, blocks, top, end, heap_error=(0x00A9, SYSTEM_CS), first=None):
    """Free blocks (segment, offset, paragraphs, granule) chained from FreeList to HeapPtr = top; HeapEnd = end."""
    put16(memory, DS, 0x4400, 0); put16(memory, DS, 0x4402, HEAP)
    put16(memory, DS, 0x4404, top[1]); put16(memory, DS, 0x4406, top[0])
    put16(memory, DS, 0x4408, end[1]); put16(memory, DS, 0x440A, end[0])
    put16(memory, DS, 0x4414, heap_error[0]); put16(memory, DS, 0x4416, heap_error[1])
    chain = list(blocks) + [(top[0], top[1], None, None)]
    head = chain[0] if first is None else first
    put16(memory, DS, 0x440C, head[1]); put16(memory, DS, 0x440E, head[0])
    for (seg, off, paragraphs, granule), nxt in zip(chain, chain[1:]):
        put16(memory, seg, off, nxt[1]); put16(memory, seg, (off + 2) & 0xFFFF, nxt[0])
        put16(memory, seg, (off + 4) & 0xFFFF, granule); put16(memory, seg, (off + 6) & 0xFFFF, paragraphs)


def build(image, rng, *, size, blocks=(), top=(HEAP + 0x400, 0), end=(HEAP + 0x1000, 0), heap_error=(0x00A9, SYSTEM_CS), first=None,
          sp=0x8000, ss=SS, flags=0x0602, bp=None, background=None, writes=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    heap(memory, blocks=blocks, top=top, end=end, heap_error=heap_error, first=first)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, size)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    for segment, offset, data in writes:
        for j, value in enumerate(data):
            memory[physical(segment, (offset + j) & 0xFFFF)] = value
    reg = [rng.randrange(0x10000) for _ in range(7)]
    if bp is not None:
        reg[6] = bp
    return memory, Cpu(*reg, sp, SYSTEM_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, decoded, conditional, function, cap=64, count=2000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, last=None, steps=0, calls=[], unexpected=None, path=set())
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
        if cs == SYSTEM_CS and ip == RUNERROR_IP:
            state["stop"] = RUNERROR
            return emulator.emu_stop()
        if cs != SYSTEM_CS or ip not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if ip in HEAP_ERROR_SITES:
            pointer = bytes(emulator.mem_read(physical(cpu.ds, 0x4414), 4))
            if pointer != struct.pack("<HH", 0x00A9, SYSTEM_CS):
                state["stop"] = HEAP_ERROR
                return emulator.emu_stop()
        if ip == WALK_HEAD and state["last"] in WALK_JAS:
            state["steps"] += 1
            if state["steps"] > cap:
                state["stop"] = LIMIT
                return emulator.emu_stop()
        ins = decoded[ip]
        visited.add(ip)
        state["path"].add(ip)
        if state["previous"] in conditional and state["previous"] != ip:
            edges.add((state["previous"], ip))
        state["previous"] = ip
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf"):
            if ip == ENTRY_RETF:
                state["returned"] = True
            else:
                state["returning"] = True
        state["last"] = ip
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(SYSTEM_CS, ENTRY), 2 * SIZE, count=count)
    check(state["unexpected"] is None, f"original left the decoded code at {state['unexpected']}: {label}")
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), cap)
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    kind = ("nil" if 0x0383 in state["path"] else "split" if 0x0343 in state["path"] else "exact" if 0x0376 in state["path"]
            and 0x0343 not in state["path"] else "top" if 0x03A0 in state["path"] else "fail" if 0x03BF in state["path"] else "other")
    return state["stop"], visited, edges, kind


def random_heap(rng):
    """A free list of normalized blocks in ascending order below HeapPtr, with gaps."""
    blocks, segment = [], HEAP
    for _ in range(rng.choice((0, 1, 2, 3, 4, 6))):
        segment += rng.randrange(1, 40)
        paragraphs, granule = rng.randrange(0, 24), rng.choice((0, 8))
        if paragraphs == 0 and granule == 0:
            granule = 8
        blocks.append((segment, 0, paragraphs, granule))
        segment += paragraphs + 1
    top = (segment + rng.randrange(1, 40), rng.choice((0, 8)))
    end = (top[0] + rng.choice((0, 2, 30, 300, 0x800)), rng.choice((0, 8)))
    return blocks, top, end


def cases(rng):
    spec = []
    for i in range(240):
        blocks, top, end = random_heap(rng)
        sizes = [0, 1, 7, 8, 9, 15, 16, 17, 100, 0xFFF9, rng.randrange(1, 400), rng.randrange(1, 0x10000)]
        sizes += [16 * b[2] + b[3] for b in blocks] + [16 * b[2] + b[3] - 3 for b in blocks if 16 * b[2] + b[3] > 3]
        spec.append((f"random-{i}", dict(size=rng.choice(sizes), blocks=blocks, top=top, end=end, flags=FLAGS[i % len(FLAGS)],
                                         sp=rng.randrange(0x0400, 0xF000) & ~1)))
    two = ((HEAP + 0x10, 0, 2, 0), (HEAP + 0x20, 8, 4, 8))
    spec.append(("zero", dict(size=0, blocks=two)))
    spec.append(("exact-first", dict(size=32, blocks=two)))
    spec.append(("exact-second", dict(size=72, blocks=two)))
    spec.append(("split-first", dict(size=24, blocks=two)))
    spec.append(("split-carry", dict(size=9, blocks=((HEAP + 0x10, 8, 2, 0),))))
    spec.append(("skip-to-top", dict(size=200, blocks=two)))
    spec.append(("empty-list-top", dict(size=64)))
    spec.append(("top-exact-end", dict(size=0x100, top=(HEAP + 0x400, 0), end=(HEAP + 0x410, 0))))
    spec.append(("top-granule-end", dict(size=0x108, top=(HEAP + 0x400, 0), end=(HEAP + 0x410, 8))))
    spec.append(("heap-full", dict(size=0x200, top=(HEAP + 0x400, 0), end=(HEAP + 0x410, 0))))
    spec.append(("max-size", dict(size=0xFFFF, top=(HEAP + 0x400, 0), end=(HEAP + 0x1500, 0))))
    spec.append(("custom-heap-error-top", dict(size=64, heap_error=(0x1234, 0x5678))))
    spec.append(("custom-heap-error-fail", dict(size=0x200, end=(HEAP + 0x410, 0), heap_error=(0x00A9, 0x0AF9))))
    spec.append(("cyclic", dict(size=0x400, blocks=((HEAP + 0x10, 0, 1, 0),), writes=((HEAP + 0x10, 0, struct.pack("<HH", 0, HEAP + 0x10)),), cap=5)))
    spec.append(("cap-exact", dict(size=72, blocks=two, cap=1)))
    spec.append(("cap-short", dict(size=72, blocks=two, cap=0)))
    for sp in (0x0010, 0x0100, 0xFFE0, 0xFFF8):
        spec.append((f"sp-{sp:04x}", dict(size=24, blocks=two, sp=sp, ss=0x2000 if sp < 0x100 else 0x1000)))
    spec.append(("ss-a20", dict(size=24, blocks=two, sp=0x0400, ss=0xFFFF)))
    spec.append(("random-background", dict(size=40, blocks=two, background=bytes(random.Random(0x0AF8023F).randbytes(SIZE)))))
    spec.append(("preserved-df-if", dict(size=24, blocks=two, flags=0x0603)))
    return spec


def differential(image, decoded, conditional, function):
    rng = random.Random(0x0AF8023F)
    visited, edges, stops, kinds = set(), set(), {k: 0 for k in NAMES}, {}
    for label, options in cases(rng):
        cap = options.pop("cap", 64)
        memory, cpu, high = build(image, rng, **options)
        stop, v, e, kind = compare(f"GetMem {label}", memory, cpu, high, decoded, conditional, function, cap)
        if not label.startswith("random"):
            print(f"  {label}: {NAMES[stop]} ({kind})")
        stops[stop] += 1
        kinds[kind] = kinds.get(kind, 0) + 1
        visited |= v
        edges |= e
    starts = [a for a in decoded if SPANS[0][0] <= a < SPANS[0][1] or SPANS[1][0] <= a < SPANS[1][1] or SPANS[2][0] <= a < SPANS[2][1]
              or SPANS[3][0] <= a < SPANS[3][1]]
    reachable = {(s, o) for s, outs in conditional.items() for o in outs}
    missing = sorted(hex(a) for a in starts if a not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    unreachable = (["0x3cb", "0x3cd", "0x3d0"], [("0x3c9", "0x3cb"), ("0x3cb", "0x383"), ("0x3cb", "0x3cd")])
    check((missing, missing_edges) == unreachable, f"coverage differs from the default-HeapError gaps: {missing} {missing_edges}")
    check(stops[OK] and stops[RUNERROR] and stops[HEAP_ERROR] and stops[LIMIT], f"a reachable stop kind was not exercised: {stops}")
    print(f"GetMem: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM and all "
          f"14 CPU words equal, no mask; {len(visited)}/{len(starts)} instruction starts, {len(edges)}/{len(reachable)} conditional outcomes; "
          f"paths {dict(sorted(kinds.items()))}; unvisited {missing} {missing_edges}")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), total_starts=len(starts), outcomes=len(edges),
                total_outcomes=len(reachable), paths=kinds, unvisited=missing, unvisited_outcomes=missing_edges)


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, DATA_DS, ARENA_N = 0x7000, 0x5000, 0x6000
MAIN_AT, SPAN_AT, CASES_AT = 0x0300, 0x0C00, 0x1200
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, ARENA_BYTES, SPAN_BYTES = STACK_TOP - 0x100, 0x100, 0x4000, 0x0500
CASE_SIZE, NATIVE_CASES = 32, 64
RECORD_BYTES = 30 + WINDOW_SIZE + 0x10000 + ARENA_BYTES
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        blocks, segment = [], ARENA_N
        for _ in range(rng.choice((0, 1, 2, 3, 4))):
            segment += rng.randrange(1, 30)
            paragraphs, granule = rng.randrange(0, 20), rng.choice((0, 8))
            if paragraphs == 0 and granule == 0:
                granule = 8
            blocks.append((segment, 0, paragraphs, granule))
            segment += paragraphs + 1
        top = (segment + rng.randrange(1, 30), rng.choice((0, 8)))
        end = (min(ARENA_N + 0x3FF, top[0] + rng.choice((0, 2, 30, 300))), rng.choice((0, 8)))
        sizes = [0, 1, 8, 9, 16, 17, 100, rng.randrange(1, 600), rng.randrange(1, 0x3000)] + [16 * b[2] + b[3] for b in blocks]
        cases.append(dict(flags=n % len(NATIVE_FLAGS), size=rng.choice(sizes), blocks=blocks, top=top, end=end))
    return cases


def case_bytes(case):
    data = struct.pack("<HBB4H", case["size"], case["flags"], len(case["blocks"]), case["top"][1], case["top"][0], case["end"][1], case["end"][0])
    for seg, off, paragraphs, granule in case["blocks"]:
        data += struct.pack("<HBB", seg, paragraphs, granule)
    return data + bytes(CASE_SIZE - len(data))


def native_audit(image, decoded, function):
    del decoded
    directory = ROOT / "resources/oracle-runs/system-getmem-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = image[physical(SYSTEM_CS, 0):physical(SYSTEM_CS, SPAN_BYTES)]
    rng = random.Random(0x0AF8023D)
    cases = native_cases(rng)
    check(SPAN_AT + len(span) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov si,offset span", "    xor di,di", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    mov byte ptr es:[0xe2],0xea", "    mov word ptr es:[0xe3],offset trap", "    mov word ptr es:[0xe5],cs",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax", "    mov word ptr [case_ptr],ax",
         f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         f"    mov ax,{ARENA_N:#x}", "    mov es,ax", "    xor di,di", f"    mov cx,{ARENA_BYTES // 2:#x}", "    mov ax,0xa5a5", "    rep stosw",
         f"    mov ax,{DATA_DS:#x}", "    mov es,ax",
         "    mov word ptr es:[0x4400],0", f"    mov word ptr es:[0x4402],{ARENA_N:#x}",
         "    mov ax,word ptr [si+4]", "    mov word ptr es:[0x4404],ax", "    mov ax,word ptr [si+6]", "    mov word ptr es:[0x4406],ax",
         "    mov ax,word ptr [si+8]", "    mov word ptr es:[0x4408],ax", "    mov ax,word ptr [si+10]", "    mov word ptr es:[0x440a],ax",
         "    mov word ptr es:[0x4414],0xa9", f"    mov word ptr es:[0x4416],{SYS_N:#x}",
         # FreeList: the first block, or HeapPtr when there is none
         "    mov ax,word ptr [si+4]", "    mov dx,word ptr [si+6]", "    cmp byte ptr [si+3],0", "    je list_head",
         "    xor ax,ax", "    mov dx,word ptr [si+12]", "list_head:", "    mov word ptr es:[0x440c],ax", "    mov word ptr es:[0x440e],dx",
         # each block (segment, paragraphs, granule) at offset 0, linked to the next block or to HeapPtr
         "    xor ch,ch", "    mov cl,byte ptr [si+3]", "    jcxz blocks_done", "    lea bx,[si+12]", "block_loop:",
         "    mov es,word ptr [bx]", "    xor ax,ax", "    mov dx,word ptr [bx+4]", "    cmp cx,1", "    jne block_link",
         "    mov ax,word ptr [si+4]", "    mov dx,word ptr [si+6]", "block_link:",
         "    mov word ptr es:[0],ax", "    mov word ptr es:[2],dx", "    mov al,byte ptr [bx+3]", "    xor ah,ah", "    mov word ptr es:[4],ax",
         "    mov al,byte ptr [bx+2]", "    xor ah,ah", "    mov word ptr es:[6],ax", "    add bx,4", "    loop block_loop", "blocks_done:",
         "    push cs", "    pop es", "    push word ptr [si]", "    mov bl,byte ptr [si+2]", "    xor bh,bh", "    shl bx,1",
         "    push word ptr [flags_table+bx]", f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", "status: .word 0",
          f"target_far: .word {ENTRY:#x}, {SYS_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("span", span, SPAN_AT), ("cases", b"".join(case_bytes(c) for c in cases), CASES_AT)):
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
        at = physical(DATA_DS, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
        at = physical(ARENA_N, 0); memory[at:at + ARENA_BYTES] = bytes([0xA5]) * ARENA_BYTES
        put16(memory, DATA_DS, 0x4400, 0); put16(memory, DATA_DS, 0x4402, ARENA_N)
        put16(memory, DATA_DS, 0x4404, case["top"][1]); put16(memory, DATA_DS, 0x4406, case["top"][0])
        put16(memory, DATA_DS, 0x4408, case["end"][1]); put16(memory, DATA_DS, 0x440A, case["end"][0])
        put16(memory, DATA_DS, 0x4414, 0x00A9); put16(memory, DATA_DS, 0x4416, SYS_N)
        head = (case["blocks"][0][1], case["blocks"][0][0]) if case["blocks"] else (case["top"][1], case["top"][0])
        put16(memory, DATA_DS, 0x440C, head[0]); put16(memory, DATA_DS, 0x440E, head[1])
        for i, (seg, off, paragraphs, granule) in enumerate(case["blocks"]):
            nxt = (0, case["blocks"][i + 1][0]) if i + 1 < len(case["blocks"]) else (case["top"][1], case["top"][0])
            put16(memory, seg, 0, nxt[0]); put16(memory, seg, 2, nxt[1]); put16(memory, seg, 4, granule); put16(memory, seg, 6, paragraphs)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (case["size"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, SYS_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = function(ram, SIZE, ctypes.byref(cpu), 64)
        check(status in (OK, RUNERROR) and struct.unpack_from("<H", record, 28)[0] == (1 if status == RUNERROR else 0),
              f"native case {n}: C status {status}, DOS status {struct.unpack_from('<H', record, 28)[0]}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE])
        got += bytes(ram[physical(DATA_DS, 0):physical(DATA_DS, 0) + 0x10000]) + bytes(ram[physical(ARENA_N, 0):physical(ARENA_N, 0) + ARENA_BYTES])
        if got != record[30:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[30:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        statuses[NAMES[status]] = statuses.get(NAMES[status], 0) + 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({statuses}); all 14 CPU words, the {WINDOW_SIZE}-byte stack window, "
          f"the whole 64 KiB DS segment and the {ARENA_BYTES:#x}-byte heap arena; no mask; RunError captured at {SYS_N:04X}:00E2")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), statuses=statuses, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    image, decoded, conditional, digests = source()
    report = dict(target=dict(spans={k: dict(bytes=n, instructions=c, sha256=d) for k, (n, c, d) in digests.items()}))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-getmem-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(image, decoded, conditional, function)
            if args.native:
                report["native"] = native_audit(image, decoded, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
