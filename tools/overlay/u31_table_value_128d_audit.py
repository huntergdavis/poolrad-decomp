#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U31 +128D with SYSTEM Move composed, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the original
Move runs live at 0AF8:0A3F from +128D's unrelocated, fixup-backed far call. It loads unit 0x31 and
compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C model,
with no mask, stopping at the first fetch after RETF 8 wherever it goes, or at a rewritten call site
(CODE). The DOSBox-X oracle copies the unit to 8800 and Move to 9000:0A3F. Media stays private.
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

ENTRY, END, SITE = 0x128D, 0x13CA, 0x12B1
MOVE_CS, MOVE_IP, MOVE_END = 0x0AF8, 0x0A3F, 0x0A57
MOVE_SHA = "5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa"
CALLER_CS, DS, SS, EDGE_SS = 0x8800, 0x1800, 0x2800, 0x2000
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE = 0, 2


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
    move = image[MOVE_CS * 16 + MOVE_IP:MOVE_CS * 16 + MOVE_END]
    check(sha256(move).hexdigest() == MOVE_SHA, "SYSTEM 0AF8:0A3F differs from the pinned Move")
    unit = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}[0x31]
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    starts = sorted(f["rel"] for f in unit["functions"])
    check(END == next(s for s in starts if s > ENTRY), "U31 +128D bounds differ")
    body = code[ENTRY:END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    calls = [(i.address, i.op_str) for i in decoded if "call" in i.mnemonic]
    check(len(body) == 317 and len(decoded) == 115 and sum(i.size for i in decoded) == 317 and decoded[-1].op_str == "8"
          and calls == [(SITE, "0xaf8, 0xa3f")] and SITE + 3 in set(unit["fixups"]), f"U31 +128D extent or call differs: {calls}")
    conditional = conditionals(decoded)
    check(len(conditional) == 12, "U31 +128D conditional count differs")
    move_decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(move, MOVE_IP))
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=unit["ovr_offset"] + ENTRY,
                  move=move, move_decoded=move_decoded)
    print(f"U31 +128D (317/115, LCALL 0AF8:0A3F at +12B1, RETF 8) GAME file {target['file_offset']:#x} sha {target['sha']}")
    return target, code, image


def library(directory):
    path = directory / "u31.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/overlay_u31_table_value_128d_cpu.c"), str(ROOT / "src/recovered/system_runtime_leaves_cpu.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_u31_table_value_128d_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def fields(rng, bias):
    """Branch-biased record fields: (row[16], c, p32, p36, p3d, p3e, q100, qa0, nil_r, t76)."""
    row = [rng.randrange(256) for _ in range(16)]
    f = dict(row=row, c=rng.randrange(256), p32=rng.randrange(256), p36=rng.choice((0, 0, rng.randrange(256))),
             p3d=rng.randrange(256), p3e=rng.randrange(256), q100=rng.randrange(256), qa0=rng.randrange(16), nil_r=rng.random() < 0.3,
             t76=rng.randrange(256))
    if bias & 1: f["c"] = 0x55
    if bias & 2: f["p3e"] = 0x84
    if bias & 4: f["p3d"] = 0x53 if rng.random() < 0.3 else f["p3d"]
    if bias & 8: f["q100"], row[1] = rng.randrange(3), rng.randrange(3)
    if bias & 16: f["p36"] = 0
    if bias & 32: row[14] |= 8
    return f


def build(image, code, rng, f, *, sp=0x8000, ss=SS, flags=0x7202, records=None, background=None, patch=()):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    base = physical(CALLER_CS, 0)
    memory[base:base + len(code)] = code
    for offset, value in patch:
        memory[physical(CALLER_CS, offset)] = value
    rand_far = lambda: (0x4000 + rng.randrange(0x3000), rng.randrange(0x10000))
    p, q, r, t = records or (rand_far(), rand_far(), rand_far(), rand_far())
    table = 0x5582 + 16 * f["c"]
    for i, value in enumerate(f["row"]):
        memory[physical(DS, (table + i) & 0xFFFF)] = value
    for off, value in ((0x2E, f["c"]), (0x32, f["p32"]), (0x36, f["p36"]), (0x3D, f["p3d"]), (0x3E, f["p3e"])):
        memory[physical(p[0], (p[1] + off) & 0xFFFF)] = value
    memory[physical(q[0], (q[1] + 0x100) & 0xFFFF)] = f["q100"]
    memory[physical(q[0], (q[1] + 0xA0) & 0xFFFF)] = f["qa0"]
    put16(memory, q[0], (q[1] + 0x108) & 0xFFFF, r[1])
    put16(memory, q[0], (q[1] + 0x10A) & 0xFFFF, r[0])
    tp = (0, 0) if f["nil_r"] else t
    put16(memory, r[0], (r[1] + 0x0A) & 0xFFFF, tp[1])
    put16(memory, r[0], (r[1] + 0x0C) & 0xFFFF, tp[0])
    memory[physical(tp[0], (tp[1] + 0x76) & 0xFFFF)] = f["t76"]
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS, p[1], p[0], q[1], q[0]]):   # both views agree at a segment end
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(seg >= 0xF000 for seg in (ss, p[0], q[0], r[0], tp[0]))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, DS, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, budget=20000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    base = physical(CALLER_CS, 0)
    body, move = (base + ENTRY, base + END), (physical(MOVE_CS, MOVE_IP), physical(MOVE_CS, MOVE_END))
    site = base + SITE
    site_ok = bytes(memory[site:site + 3]) == bytes([0x9A, 0x3F, 0x0A])
    state = dict(stop=None, previous=None, move_previous=None)
    visited, edges, move_visited = set(), set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if address == site and not site_ok:
            state["stop"] = CODE
            emulator.emu_stop()
            return
        if body[0] <= address < body[1]:
            here = address - base
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
        elif move[0] <= address < move[1]:
            move_visited.add(address - move[0] + MOVE_IP)
            state["previous"] = None
        else:
            state["stop"] = OK
            emulator.emu_stop()

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
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee))
    check(status == state["stop"] and callee.value == (0 if status == OK else -1), f"C status {status}/{callee.value} vs {state['stop']}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], visited, edges, move_visited


def cases(rng):
    spec = []
    for i in range(400):
        spec.append((f"random-{i}", dict(f=fields(rng, i % 64), flags=(0x7202, 0x0602, 0x0ED7, 0x0647, 0xF2D7)[i % 5],
                                          sp=rng.randrange(0x0200, 0xFE00) & ~1)))
    bp = 0x8000 - 2
    for name, delta in (("p-over-row", -0x12 - 0x2E), ("p-over-saved-bp", -0x2E), ("q-over-frame", -0x100), ("p-over-move-frame", -0x20 - 0x2E)):
        f = fields(rng, 0)
        p = (SS, (bp + delta) & 0xFFFF)
        q = (SS, (bp + delta) & 0xFFFF) if name.startswith("q") else (0x4800, 0x0100)
        spec.append((f"alias-{name}", dict(f=f, records=(p if not name.startswith("q") else (0x4800, 0x0200), q, (0x5000, 0x0300), (0x5800, 0x0400)))))
    for sp in (0x0030, 0x0050, 0xFFF4, 0xFFF6, 0xFFFA, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(f=fields(rng, 5), sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(f=fields(rng, 1), sp=0x0400, ss=0xFFFF)))
    spec.append(("records-a20", dict(f=fields(rng, 1), records=((0xFFFF, 0x0100), (0xFFF8, 0x0200), (0xFFF0, 0x0300), (0xFFFF, 0x0400)))))
    spec.append(("random-background", dict(f=fields(rng, 3), background=bytes(random.Random(0x128D).randbytes(SIZE)))))
    for k, (offset, value) in enumerate(((SITE, 0x90), (SITE + 1, 0x40), (SITE + 2, 0x0B))):
        spec.append((f"code-{k}", dict(f=fields(rng, 0), patch=((offset, value),))))
    return spec


def differential(target, code, image, function):
    rng = random.Random(0x31128D)
    visited, edges, move_visited, stops = set(), set(), set(), {OK: 0, CODE: 0}
    for label, options in cases(rng):
        f = options.pop("f")
        memory, cpu, high = build(image, code, rng, f, **options)
        stop, v, e, mv = compare(f"+128D {label}", memory, cpu, high, target, function)
        visited |= v
        edges |= e
        move_visited |= mv
        stops[stop] += 1
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
    check(not missing and not missing_edges and len(move_visited) == len(target["move_decoded"]),
          f"coverage incomplete: {missing} {missing_edges} move {len(move_visited)}")
    print(f"U31 +128D: {stops[OK]} returning and {stops[CODE]} CODE-stop original-byte cases passed, full 1 MiB RAM and all 14 CPU words "
          f"equal; {len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional "
          f"outcomes; Move {len(move_visited)}/{len(target['move_decoded'])}")
    return dict(returning=stops[OK], code_stops=stops[CODE], starts=len(visited), outcomes=len(edges), move_starts=len(move_visited))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
CALLER_N, MOVE_N, DATA_DS, WIN = 0x8800, 0x9000, 0x1900, 0x2000
MAIN_AT, MOVE_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0F00, 0x1000, 0x6000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFB0, 0x50
P_OFF, Q_OFF, R_OFF, T_OFF = 0x0000, 0x0100, 0x0300, 0x0400
CASE_SIZE = 32
RECORD_BYTES = 28 + FRAME_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x31128DD0)
    return [dict(fields(rng, i % 64), flags=i % len(NATIVE_FLAGS)) for i in range(160)]


def native_audit(target, code, function):
    directory = ROOT / "resources/oracle-runs/u31-128d-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    unit = bytearray(code)
    struct.pack_into("<H", unit, SITE + 3, MOVE_N)
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (MOVE_N, MOVE_IP, "move", len(target["move"]))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{WIN:#x}", "    mov es,ax",
             f"    mov word ptr es:[{Q_OFF + 0x108:#x}],{R_OFF:#x}", f"    mov word ptr es:[{Q_OFF + 0x10A:#x}],{WIN:#x}",
             "    mov ax,cs", "    mov es,ax",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             # table row: DATA_DS:5582 + 16c
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov al,byte ptr [si]", "    xor ah,ah", "    mov cl,4", "    shl ax,cl",
             "    add ax,0x5582", "    mov di,ax", "    push si", "    add si,16", "    mov cx,16", "    rep movsb", "    pop si",
             # record fields in WIN
             f"    mov ax,{WIN:#x}", "    mov es,ax",
             "    mov al,byte ptr [si]", f"    mov byte ptr es:[{P_OFF + 0x2E:#x}],al",
             "    mov al,byte ptr [si+1]", f"    mov byte ptr es:[{P_OFF + 0x32:#x}],al",
             "    mov al,byte ptr [si+2]", f"    mov byte ptr es:[{P_OFF + 0x36:#x}],al",
             "    mov al,byte ptr [si+3]", f"    mov byte ptr es:[{P_OFF + 0x3D:#x}],al",
             "    mov al,byte ptr [si+4]", f"    mov byte ptr es:[{P_OFF + 0x3E:#x}],al",
             "    mov al,byte ptr [si+5]", f"    mov byte ptr es:[{Q_OFF + 0x100:#x}],al",
             "    mov al,byte ptr [si+6]", f"    mov byte ptr es:[{Q_OFF + 0xA0:#x}],al",
             "    mov ax,word ptr [si+8]", f"    mov word ptr es:[{R_OFF + 0x0A:#x}],ax",
             "    mov ax,word ptr [si+10]", f"    mov word ptr es:[{R_OFF + 0x0C:#x}],ax",
             "    mov al,byte ptr [si+7]", f"    mov byte ptr es:[{T_OFF + 0x76:#x}],al",
             "    mov ax,cs", "    mov es,ax",
             f"    mov ax,{WIN:#x}", "    push ax", f"    mov ax,{Q_OFF:#x}", "    push ax", f"    mov ax,{WIN:#x}", "    push ax", f"    mov ax,{P_OFF:#x}", "    push ax",
             "    mov bl,byte ptr [si+12]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
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
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, at in (("move", target["move"], MOVE_BLOB), ("unit", bytes(unit), UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        tp = (0, 0) if case["nil_r"] else (WIN, T_OFF)
        row = bytes([case["c"], case["p32"], case["p36"], case["p3d"], case["p3e"], case["q100"], case["qa0"], case["t76"]])
        row += struct.pack("<HHBBBB", tp[1], tp[0], case["flags"], 0, 0, 0) + bytes(case["row"])
        check(len(row) == CASE_SIZE, "case row size differs")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(UNIT_BLOB + len(unit) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
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
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (MOVE_N, MOVE_IP, target["move"])):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        put16(memory, WIN, Q_OFF + 0x108, R_OFF)
        put16(memory, WIN, Q_OFF + 0x10A, WIN)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i, value in enumerate(case["row"]):
            memory[physical(DATA_DS, 0x5582 + 16 * case["c"] + i)] = value
        for off, key in ((P_OFF + 0x2E, "c"), (P_OFF + 0x32, "p32"), (P_OFF + 0x36, "p36"), (P_OFF + 0x3D, "p3d"), (P_OFF + 0x3E, "p3e"),
                         (Q_OFF + 0x100, "q100"), (Q_OFF + 0xA0, "qa0"), (T_OFF + 0x76, "t76")):
            memory[physical(WIN, off)] = case[key]
        tp = (0, 0) if case["nil_r"] else (WIN, T_OFF)
        put16(memory, WIN, R_OFF + 0x0A, tp[1])
        put16(memory, WIN, R_OFF + 0x0C, tp[0])
        sp = STACK_TOP
        for value in (WIN, Q_OFF, WIN, P_OFF, segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n}"
        check(function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee)) == 0 and callee.value == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[28:], f"{label} stack bytes differ")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)}; all 14 CPU words and the {FRAME_SIZE}-byte stack window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; Move copied to {MOVE_N:04X}:0A3F")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, code, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=317, instructions=115, file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u31-") as temp:
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
