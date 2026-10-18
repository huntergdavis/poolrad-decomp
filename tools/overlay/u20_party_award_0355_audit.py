#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U20 +0355 with the SYSTEM LongInt divide composed, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the original divide runs live at
0AF8:0A76 from +0355's five unrelocated, fixup-backed far calls. It loads unit 0x20 and compares the entire 1 MiB RAM and
all 14 CPU words between the original under Unicorn and the C model, with no mask. The original decides each stop: its
return (wherever RETF 4 goes), a rewritten call site (CODE) or the party-walk cap (LIMIT). Media stays private.
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

ENTRY, END = 0x0355, 0x04C9
SITES = (0x03BB, 0x03E8, 0x0415, 0x0441, 0x046F, 0x048F)
LOOP_HEAD, LOOP_JMP, BODY_RETF = 0x0368, 0x04C0, 0x04C6
DIVIDE = (0x0AF8, 0x0A76, 0x0AE4, "131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5")
CALLER_CS, DS, SS, EDGE_SS = 0x8800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, LIMIT = 0, 2, 5
NAMES = {OK: "return", CODE: "CODE", LIMIT: "LIMIT"}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def conditionals(decoded):
    return {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    seg, start, end, sha = DIVIDE
    divide = image[seg * 16 + start:seg * 16 + end]
    check(sha256(divide).hexdigest() == sha, "SYSTEM 0AF8:0A76 differs from the pinned LongInt divide")
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x20]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(END == next(s for s in starts if s > ENTRY), "U20 +0355 bounds differ")
    body = code[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    check(len(body) == 372 and len(decoded) == 126 and sum(i.size for i in decoded) == 372 and decoded[-1].op_str == "4"
          and calls == [(s, "0xaf8, 0xa76") for s in SITES] and all(s + 3 in set(unit["fixups"]) for s in SITES),
          f"U20 +0355 extent or calls differ: {calls}")
    target = dict(body=body, decoded=decoded, conditional=conditionals(decoded), sha=sha256(body).hexdigest(),
                  file_offset=unit["ovr_offset"] + ENTRY, divide=divide)
    print(f"U20 +0355 (372/126, 6 x LCALL 0AF8:0A76, RETF 4) GAME file {target['file_offset']:#x} sha {target['sha']}; "
          f"{len(target['conditional'])} conditionals")
    return target, code, image


def library(directory):
    path = directory / "u20.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u20_party_award_0355_cpu.c"), str(ROOT / "src/recovered/system_longdiv_0a76_cpu.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u20_party_award_0355_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def member(rng):
    return dict(active=rng.choice((0, 1, 1, 1, 2)), state=rng.choice((0, 1, 2, 1, 0)),
                cls=rng.choice((0, 2, 5, 6, 8, 9, 10, 12, 14, 15, 16, 1, 3, 4, 7, 11, 13, 17, 0x80, 0xFE, rng.randrange(256))),
                stats=[rng.choice((15, 16, rng.randrange(256))) for _ in range(4)], xp=rng.randrange(1 << 32))


def build(code, image, rng, members, x, *, sp=0x8000, ss=SS, flags=0x7202, cyclic=False, party_seg=0x5000, background=None, patch=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    base = physical(CALLER_CS, 0)
    memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    offsets = [0x0100 + 0x200 * k for k in range(len(members))]
    for k, (off, m) in enumerate(zip(offsets, members)):
        memory[physical(party_seg, off + 0x10D)] = m["active"]
        memory[physical(party_seg, off + 0x10C)] = m["state"]
        memory[physical(party_seg, off + 0x2F)] = m["cls"]
        for j, field in enumerate((0x12, 0x10, 0x11, 0x13)):
            memory[physical(party_seg, off + field)] = m["stats"][j]
        put16(memory, party_seg, off + 0xAC, m["xp"] & 0xFFFF)
        put16(memory, party_seg, off + 0xAE, m["xp"] >> 16)
        nxt = offsets[k + 1] if k + 1 < len(offsets) else (offsets[0] if cyclic else 0)
        put16(memory, party_seg, off + 0x104, nxt)
        put16(memory, party_seg, off + 0x106, party_seg if nxt else 0)
    put16(memory, DS, 0x5D96, offsets[0] if offsets else 0)
    put16(memory, DS, 0x5D98, party_seg if offsets else 0)
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS, x & 0xFFFF, x >> 16]):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = ss >= 0xF000 or party_seg >= 0xF000
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, cap, budget=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body = (base + ENTRY, base + END)
    divide = (physical(DIVIDE[0], DIVIDE[1]), physical(DIVIDE[0], DIVIDE[2]))
    sites = {base + s: bytes(memory[base + s:base + s + 3]) == bytes([0x9A, 0x76, 0x0A]) for s in SITES}
    state = dict(stop=None, returning=False, previous=None, loopbacks=0, last=None)
    visited, edges, divide_seen = set(), set(), set()

    def hook(emulator, address, size, user):
        del size, user
        last, state["last"] = state["last"], address
        if state["returning"]:
            state["stop"] = OK
            emulator.emu_stop()
            return
        if address in sites and not sites[address]:
            state["stop"] = CODE
            emulator.emu_stop()
            return
        if address == base + LOOP_HEAD and last == base + LOOP_JMP:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                state["stop"] = LIMIT
                emulator.emu_stop()
                return
        if body[0] <= address < body[1]:
            here = address - base
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
            if here == BODY_RETF:
                state["returning"] = True
        elif divide[0] <= address < divide[1]:
            divide_seen.add(address - divide[0] + DIVIDE[1])
            state["previous"] = None
        else:
            raise ValueError(f"unexpected fetch at {address:#x}: {label}")

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(body[0], 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee = ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), cap, ctypes.byref(callee))
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], visited, edges, divide_seen


X_EDGES = (0, 1, 9, 10, 11, 29, 30, 1000, 0xFFFF, 0x10000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF, 0xFFFFFFF6, 0xFFFF0000)


def cases(rng):
    spec = []
    for cls in list(range(0, 21)) + [0x7F, 0x80, 0xF0, 0xFF]:
        for stat in (15, 16):
            m = dict(active=1, state=0, cls=cls, stats=[stat] * 4, xp=rng.randrange(1 << 32))
            spec.append((f"class-{cls:02x}-stat{stat}", dict(members=[m], x=rng.choice(X_EDGES[:10]))))
    for x in X_EDGES:
        spec.append((f"x-{x:08x}", dict(members=[member(rng) for _ in range(4)], x=x)))
    for i in range(150):
        spec.append((f"random-{i}", dict(members=[member(rng) for _ in range(rng.randrange(0, 7))], x=rng.randrange(1 << 32),
                                          flags=(0x7202, 0x0602, 0x0ED7, 0x3A93)[i % 4], sp=rng.randrange(0x0200, 0xFE00) & ~1)))
    spec.append(("empty", dict(members=[], x=1234)))
    spec.append(("cyclic", dict(members=[member(rng) for _ in range(3)], x=500, cyclic=True, cap=20)))
    spec.append(("cap-0", dict(members=[member(rng) for _ in range(3)], x=500, cap=0)))
    for sp in (0x0040, 0xFFF4, 0xFFF8, 0xFFFC):
        spec.append((f"sp-{sp:04x}", dict(members=[member(rng) for _ in range(2)], x=777, sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(members=[member(rng) for _ in range(2)], x=777, sp=0x0400, ss=0xFFFF)))
    spec.append(("party-a20", dict(members=[member(rng) for _ in range(2)], x=777, party_seg=0xFFF8)))
    spec.append(("random-background", dict(members=[member(rng) for _ in range(3)], x=999, background=bytes(random.Random(0x200355).randbytes(SIZE)))))
    m = dict(active=1, state=0, cls=0, stats=[20] * 4, xp=0)
    spec.append(("code-0", dict(members=[m], x=100, patch=((SITES[0], 0x90),))))
    m2 = dict(active=1, state=0, cls=9, stats=[1] * 4, xp=0)
    spec.append(("code-5", dict(members=[m2], x=100, patch=((SITES[5] + 1, 0x77),))))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x200355)
    visited, edges, divide_seen, stops = set(), set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        cap = options.pop("cap", 1000)
        memory, cpu, high = build(code, image, rng, options.pop("members"), options.pop("x"), **options)
        stop, v, e, d = compare(f"+0355 {label}", memory, cpu, high, target, function, cap)
        stops[stop] += 1
        visited |= v
        edges |= e
        divide_seen |= d
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing_edges = sorted((hex(s), hex(o)) for s, o in reachable - edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    check(stops[CODE] and stops[LIMIT], f"a stop kind was not exercised: {stops}")
    print(f"U20 +0355: {sum(stops.values())} original-byte cases passed ({stops[OK]} returns, {stops[CODE]} CODE, {stops[LIMIT]} LIMIT); full 1 MiB "
          f"RAM and all 14 CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{len(reachable)} "
          f"conditional outcomes; divide {len(divide_seen)} starts reached")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges), divide_starts=len(divide_seen))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, DIV_N, DATA_DS, PARTY_SEG = 0x8800, 0x9000, 0x1900, 0x2000
MAIN_AT, DIV_BLOB, UNIT_BLOB = 0x0300, 0x0F00, 0x1000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFB0, 0x50
MEMBERS = 4
CASE_SIZE = 4 + 2 + MEMBERS * 12
RECORD_BYTES = 28 + FRAME_SIZE + MEMBERS * 4
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x200355D)
    cases = []
    for i in range(120):
        count = rng.randrange(0, MEMBERS + 1)
        cases.append(dict(x=rng.choice((rng.randrange(1 << 32), rng.choice(X_EDGES))), count=count,
                          members=[member(rng) for _ in range(MEMBERS)], flags=i % len(NATIVE_FLAGS)))
    return cases


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u20-0355-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, DIV_N)
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (DIV_N, DIVIDE[1], "divide", len(target["divide"]))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{PARTY_SEG:#x}", "    mov es,ax"]
    for k in range(MEMBERS):
        off = 0x0100 + 0x200 * k
        b = 6 + 12 * k
        text += [f"    mov al,byte ptr [si+{b}]", f"    mov byte ptr es:[{off + 0x10D:#x}],al",
                 f"    mov al,byte ptr [si+{b + 1}]", f"    mov byte ptr es:[{off + 0x10C:#x}],al",
                 f"    mov al,byte ptr [si+{b + 2}]", f"    mov byte ptr es:[{off + 0x2F:#x}],al",
                 f"    mov al,byte ptr [si+{b + 3}]", f"    mov byte ptr es:[{off + 0x12:#x}],al",
                 f"    mov al,byte ptr [si+{b + 4}]", f"    mov byte ptr es:[{off + 0x10:#x}],al",
                 f"    mov al,byte ptr [si+{b + 5}]", f"    mov byte ptr es:[{off + 0x11:#x}],al",
                 f"    mov al,byte ptr [si+{b + 6}]", f"    mov byte ptr es:[{off + 0x13:#x}],al",
                 f"    mov ax,word ptr [si+{b + 8}]", f"    mov word ptr es:[{off + 0xAC:#x}],ax",
                 f"    mov ax,word ptr [si+{b + 10}]", f"    mov word ptr es:[{off + 0xAE:#x}],ax",
                 # link to the next member when it is within the case's count, else nil
                 f"    xor ax,ax", f"    cmp byte ptr [si+4],{k + 1}", f"    jbe nil_{k}", f"    mov ax,{off + 0x200:#x}", f"nil_{k}:",
                 f"    mov word ptr es:[{off + 0x104:#x}],ax", "    or ax,ax", f"    jz nilseg_{k}", f"    mov ax,{PARTY_SEG:#x}", f"nilseg_{k}:",
                 f"    mov word ptr es:[{off + 0x106:#x}],ax"]
    text += [f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    xor ax,ax", "    cmp byte ptr [si+4],0", "    je no_head", "    mov ax,0x0100", "no_head:",
             "    mov word ptr es:[0x5d96],ax", "    or ax,ax", "    jz no_head_seg", f"    mov ax,{PARTY_SEG:#x}", "no_head_seg:", "    mov word ptr es:[0x5d98],ax",
             "    mov ax,cs", "    mov es,ax",
             "    push word ptr [si+2]", "    push word ptr [si]",
             "    mov bl,byte ptr [si+5]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    mov ax,cs", "    mov es,ax", "    mov ds,ax", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb", f"    mov ax,{PARTY_SEG:#x}", "    mov ds,ax"]
    for k in range(MEMBERS):
        text += [f"    mov si,{0x0100 + 0x200 * k + 0xAC:#x}", "    movsw", "    movsw"]
    text += ["    mov ax,cs", "    mov ds,ax", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("divide", target["divide"], DIV_BLOB), ("unit", bytes(unit), UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    cases_at = (UNIT_BLOB + len(unit) + 0x1F) & ~0xF
    text += [f".org {cases_at - 0x100:#x}", "cases:"]
    for case in cases:
        row = struct.pack("<HHBB", case["x"] & 0xFFFF, case["x"] >> 16, case["count"], case["flags"])
        for m in case["members"]:
            row += bytes([m["active"], m["state"], m["cls"]] + m["stats"] + [0]) + struct.pack("<HH", m["xp"] & 0xFFFF, m["xp"] >> 16)
        check(len(row) == CASE_SIZE, f"case row size differs: {len(row)}")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(cases_at + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(text) + "\n")
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (DIV_N, DIVIDE[1], target["divide"])):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for k, m in enumerate(case["members"]):
            off = 0x0100 + 0x200 * k
            memory[physical(PARTY_SEG, off + 0x10D)] = m["active"]
            memory[physical(PARTY_SEG, off + 0x10C)] = m["state"]
            memory[physical(PARTY_SEG, off + 0x2F)] = m["cls"]
            for j, field in enumerate((0x12, 0x10, 0x11, 0x13)):
                memory[physical(PARTY_SEG, off + field)] = m["stats"][j]
            put16(memory, PARTY_SEG, off + 0xAC, m["xp"] & 0xFFFF)
            put16(memory, PARTY_SEG, off + 0xAE, m["xp"] >> 16)
            nxt = off + 0x200 if case["count"] > k + 1 else 0
            put16(memory, PARTY_SEG, off + 0x104, nxt)
            put16(memory, PARTY_SEG, off + 0x106, PARTY_SEG if nxt else 0)
        put16(memory, DATA_DS, 0x5D96, 0x0100 if case["count"] else 0)
        put16(memory, DATA_DS, 0x5D98, PARTY_SEG if case["count"] else 0)
        sp = STACK_TOP
        for value in (case["x"] >> 16, case["x"] & 0xFFFF, segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n}"
        check(function(ram, SIZE, ctypes.byref(cpu), 1000, ctypes.byref(callee)) == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        for k in range(MEMBERS):
            at = physical(PARTY_SEG, 0x0100 + 0x200 * k + 0xAC)
            got += bytes(ram[at:at + 4])
        check(got == record[28:], f"{label} bytes differ")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)}; all 14 CPU words, the {FRAME_SIZE}-byte stack window and four "
          f"members' LongInt fields; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; divide copied to {DIV_N:04X}:0A76")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=372, instructions=126, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u20-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, image, function)
            if args.native:
                report["native"] = native_audit(target, code, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
