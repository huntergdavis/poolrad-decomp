#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU SYSTEM Str 0AF8:1366 (with its near core 1254 and power table 12A6) and the
resident byte-to-string routine 00BA:1037 that composes it with StrStore 0B25, plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so both run from their original
bytes, and compares the entire 1 MiB RAM and all 14 CPU words between the original under Unicorn and the C models, with
no mask. Str runs directly over longint, width and maximum-length edges; 00BA:1037 runs over every byte value. The
original decides each 1037 stop: the fetch after its RETF 2 (return), an LCALL whose live bytes differ when fetched
(CODE), Str's entry fetched while its power table is not the original (CALLEE), or a RET/RETF that does not come back
to its call (TRANSFER). Any fetch outside the pre-decoded spans is an error. Every case keeps its stack frame apart from
the result string. Media stays private.
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

SYSTEM_CS, RES_CS = 0x0AF8, 0x00BA
SPANS = {"str": (0x1366, 0x13B1, 39), "core": (0x1254, 0x12A6, 37), "strstore": (0x0B25, 0x0B49, 16), "res": (0x1037, 0x1073, 28)}
TABLE, POWERS = 0x12A6, tuple(10 ** k for k in range(9, -1, -1))
STR_RETF, RES_RETF, SITES = 0x13AE, 0x1070, {0x1054: b"\x9a\x66\x13", 0x1068: b"\x9a\x25\x0b"}
SS = 0x2800
RET_CS, RET_IP = 0x9001, 0x7000
OK, CODE, CALLEE, TRANSFER = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    check(len(image) <= physical(SS, 0), "the image reaches the stack segment")
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    spans = {}
    for name, (start, end, count) in SPANS.items():
        segment = RES_CS if name == "res" else SYSTEM_CS
        body = image[physical(segment, start):physical(segment, end)]
        insns = list(md.disasm(body, start))
        check(sum(i.size for i in insns) == len(body) and (count is None or len(insns) == count), f"{name} extent differs: {len(insns)}")
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        spans[name] = dict(start=start, bytes=len(body), instructions=len(insns), sha256=sha256(body).hexdigest(),
                           decoded={i.address: i for i in insns}, conditional=conditional, last=insns[-1])
        print(f"{'RES 00BA' if name == 'res' else 'SYSTEM'} {name} {start:04X}..{end - 1:04X}: {len(body)} B, {len(insns)} insns, "
              f"{len(conditional)} conditional jumps, sha {spans[name]['sha256']}")
    check(spans["str"]["last"].address == STR_RETF and spans["str"]["last"].op_str == "0xc" and spans["core"]["last"].mnemonic == "ret"
          and spans["strstore"]["last"].mnemonic == "retf" and spans["strstore"]["last"].op_str == "0xa"
          and spans["res"]["last"].address == RES_RETF and spans["res"]["last"].op_str == "2", "span ends differ")
    table = image[physical(SYSTEM_CS, TABLE):physical(SYSTEM_CS, TABLE + 40)]
    check(struct.unpack("<10I", table) == POWERS, "the power table differs")
    calls = [(i.address, i.op_str) for i in spans["res"]["decoded"].values() if "call" in i.mnemonic]
    check(calls == [(0x1054, "0xaf8, 0x1366"), (0x1068, "0xaf8, 0xb25")], f"1037 calls differ: {calls}")
    return image, spans


def library(directory):
    path = directory / "str1037.so"
    sources = ["system_str_1366_cpu.c", "resident_00ba_byte_string_1037_cpu.c", "system_strstore_fullstate_0b25.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    lib = ctypes.CDLL(str(path))
    lib.pr_system_str_1366.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_system_str_1366.restype = ctypes.c_int
    lib.pr_res_00ba_1037_cpu.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu),
                                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    lib.pr_res_00ba_1037_cpu.restype = ctypes.c_int
    return dict(str=lib.pr_system_str_1366, res=lib.pr_res_00ba_1037_cpu)


def build(image, rng, *, target, words, result, sp=0x4000, ss=SS, flags=0x0602, background=None, patch=(), table=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    for segment, offset, value in patch:
        memory[physical(segment, offset)] = value
    if table is not None:
        at = physical(SYSTEM_CS, TABLE); memory[at:at + 40] = table
    for i in range(300):
        memory[physical(result[0], (result[1] + i) & 0xFFFF)] = rng.randrange(256)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, *words)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    cs, ip = (SYSTEM_CS, 0x1366) if target == "str" else (RES_CS, 0x1037)
    return memory, Cpu(*reg, sp, cs, ip, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, target, memory, cpu, high, spans, functions):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    decoded = {("res", a): i for a, i in spans["res"]["decoded"].items()}
    for name in ("str", "core", "strstore"):
        decoded.update({("sys", a): i for a, i in spans[name]["decoded"].items()})
    conditional = {("res", a): v for a, v in spans["res"]["conditional"].items()}
    for name in ("str", "core"):
        conditional.update({("sys", a): v for a, v in spans[name]["conditional"].items()})
    top = ("sys", STR_RETF) if target == "str" else ("res", RES_RETF)
    state = dict(stop=None, returning=False, returned=False, previous=None, calls=[], unexpected=None, visited=set(), edges=set())

    def hook(emulator, address, size, user):
        del size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["returning"]:
            state["returning"] = False
            if (cs, ip) != state["calls"].pop():
                state["stop"] = CALLEE if state["calls"] else TRANSFER
                return emulator.emu_stop()
        kind = "res" if cs == RES_CS and target == "res" else "sys" if cs == SYSTEM_CS else None
        key = (kind, ip)
        if key not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if key == ("sys", 0x1366) and target == "res" and struct.unpack("<10I", bytes(emulator.mem_read(physical(cs, TABLE), 40))) != POWERS:
            state["stop"] = CALLEE
            return emulator.emu_stop()
        if kind == "res" and ip in SITES and bytes(emulator.mem_read(address, 3)) != SITES[ip]:
            state["stop"] = CODE
            return emulator.emu_stop()
        state["visited"].add(key)
        if state["previous"] in conditional and state["previous"] != key:
            state["edges"].add((state["previous"], key))
        state["previous"] = key
        ins = decoded[key]
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf"):
            if key == top:
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
    original.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=200000)
    check(state["unexpected"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    if target == "str":
        status = functions["str"](ram, SIZE, ctypes.byref(cpu))
        check(status == 0 and state["stop"] == OK, f"Str status {status}, original {NAMES[state['stop']]}: {label}")
    else:
        callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
        status = functions["res"](ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee), ctypes.byref(callee_status))
        check(status == state["stop"], f"1037 status {status} ({callee.value}/{callee_status.value}), original {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], state["visited"], state["edges"]


EDGES = (0, 1, 9, 10, 99, 100, 65535, 65536, 999999999, 1000000000, 2147483647, -1, -9, -10, -2147483648, 4000000000 - 2 ** 32)


def str_case(rng):
    value = rng.choice(EDGES + (rng.randrange(-2 ** 31, 2 ** 31), rng.randrange(-1000, 1000), rng.randrange(2 ** 31)))
    width = rng.choice((0, 0, 1, 5, 10, 11, 12, 20, 255, 300, 0x8000, 0xFFFB, rng.randrange(0x10000)))
    maxlen = rng.choice((255, 255, 0, 1, 3, 8, 11, rng.randrange(0x100)))
    result = (rng.randrange(0x3000, 0x8000), rng.choice((rng.randrange(0xF000), rng.randrange(0xFF00, 0x10000))))
    x = value & 0xFFFFFFFF
    return dict(target="str", words=(maxlen, result[1], result[0], width & 0xFFFF, x & 0xFFFF, x >> 16), result=result)


def res_case(rng, byte=None):
    result = (rng.randrange(0x3000, 0x8000), rng.choice((rng.randrange(0xF000), rng.randrange(0xFF00, 0x10000))))
    b = rng.randrange(256) if byte is None else byte
    return dict(target="res", words=((rng.randrange(256) << 8) | b, result[1], result[0]), result=result)


def cases(rng):
    spec = []
    for i in range(300):
        options = str_case(rng) if i % 2 == 0 else res_case(rng)
        spec.append((f"random-{i}-{options['target']}", dict(options, flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0400, 0x7000) & ~1)))
    for value in EDGES:
        spec.append((f"str-edge-{value}", dict(str_case(rng), words=(255, 0x0100, 0x5000, 0, value & 0xFFFF, (value & 0xFFFFFFFF) >> 16), result=(0x5000, 0x0100))))
    for b in range(256):
        spec.append((f"res-byte-{b}", res_case(rng, b)))
    spec.append(("str-temporary-wraps", dict(str_case(rng), sp=0x0010, ss=0x1000)))
    spec.append(("str-pad-wraps", dict(str_case(rng), words=(255, 0xFFF0, 0x5000, 40, 7, 0), result=(0x5000, 0xFFF0))))
    spec.append(("res-temporary-wraps", dict(res_case(rng), sp=0x0080, ss=0x1000)))
    spec.append(("res-code-str", dict(res_case(rng), patch=((RES_CS, 0x1055, 0x67),))))
    spec.append(("res-code-strstore", dict(res_case(rng), patch=((RES_CS, 0x1068, 0xEA),))))
    spec.append(("res-table-altered", dict(res_case(rng), table=struct.pack("<10I", *POWERS[:9], 2))))
    for target, maker in (("str", str_case), ("res", res_case)):
        spec.append((f"{target}-ss-a20", dict(maker(rng), sp=0x0400, ss=0xFFFF)))
        spec.append((f"{target}-df-set", dict(maker(rng), flags=0x0603)))
        spec.append((f"{target}-random-background", dict(maker(rng), background=bytes(random.Random(0x1366 + len(target)).randbytes(SIZE)))))
    return spec


def differential(spans, functions):
    rng = random.Random(0x13661037)
    stops, counts = {k: 0 for k in NAMES}, {"str": 0, "res": 0}
    visited, edges = set(), set()
    for label, options in cases(rng):
        target = options["target"]
        memory, cpu, high = build(spans["image"], rng, **options)
        stop, v, e = compare(label, target, memory, cpu, high, spans, functions)
        stops[stop] += 1
        counts[target] += 1
        visited |= v
        edges |= e
    starts = {("res", a) for a in spans["res"]["decoded"]} | {("sys", a) for n in ("str", "core") for a in spans[n]["decoded"]}
    reachable = {(("res", s), ("res", o)) for s, outs in spans["res"]["conditional"].items() for o in outs}
    reachable |= {(("sys", s), ("sys", o)) for n in ("str", "core") for s, outs in spans[n]["conditional"].items() for o in outs}
    missing = sorted(starts - visited)
    check(not missing, f"instruction coverage incomplete: {missing[:8]}")
    check(reachable <= edges, f"conditional coverage incomplete: {sorted(reachable - edges)[:8]}")
    check(stops[OK] and stops[CODE] and stops[CALLEE], f"a stop kind was not exercised: {stops}")
    print(f"Str and 00BA:1037: {sum(counts.values())} original-byte cases ({counts}; {', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); "
          f"full 1 MiB RAM and all 14 CPU words equal, no mask; every instruction of Str, its core and 1037 and both outcomes of "
          f"each of their {len(reachable) // 2} conditional jumps")
    return dict(cases=counts, stops={NAMES[k]: v for k, v in stops.items()}, starts=len(starts), outcomes=len(reachable))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, RES_N, DATA_N = 0x7000, 0x7800, 0x5000
SPAN_LOW, SPAN_HIGH = 0x0B00, 0x1400
MAIN_AT, SPAN_AT, RES_AT, CASES_AT = 0x0300, 0x0C00, 0x1600, 0x1700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x180, 0x180
CASE_SIZE, NATIVE_CASES = 16, 128
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        result = rng.choice((rng.randrange(0x0000, 0xF000), rng.randrange(0xFF00, 0x10000)))
        if n % 2 == 0:
            value = rng.choice(EDGES + (rng.randrange(-2 ** 31, 2 ** 31), rng.randrange(-1000, 1000))) & 0xFFFFFFFF
            width, maxlen = rng.choice((0, 1, 5, 11, 12, 40, 255, 300, 0xFFFB)), rng.choice((255, 255, 0, 3, 8, 11))
            words = (value >> 16, value & 0xFFFF, width, DATA_N, result, maxlen)
        else:
            words = (DATA_N, result, (rng.randrange(256) << 8) | rng.randrange(256))
        cases.append(dict(target=n % 2, flags=n % len(NATIVE_FLAGS), words=words))
    return cases


def native_audit(spans, functions):
    directory = ROOT / "resources/oracle-runs/system-str-res1037-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    image = spans["image"]
    span = image[physical(SYSTEM_CS, SPAN_LOW):physical(SYSTEM_CS, SPAN_HIGH)]
    res = bytearray(image[physical(RES_CS, 0x1037):physical(RES_CS, 0x1073)])
    for site in SITES:
        struct.pack_into("<H", res, site + 3 - 0x1037, SYS_N)
    rng = random.Random(0x1366D)
    cases = native_cases(rng)
    table = b"".join(struct.pack(f"<BBB{len(c['words'])}H", c["target"], c["flags"], len(c["words"]), *c["words"]).ljust(CASE_SIZE, b"\0")
                     for c in cases)
    check(SPAN_AT + len(span) <= RES_AT and RES_AT + len(res) <= CASES_AT and CASES_AT + len(table) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov si,offset span", f"    mov di,{SPAN_LOW:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         f"    mov ax,{RES_N:#x}", "    mov es,ax", "    mov si,offset res", "    mov di,0x1037", f"    mov cx,{len(res):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    xor ch,ch", "    mov cl,byte ptr [bx+2]", "    lea si,[bx+3]", "push_loop:", "    push word ptr [si]", "    add si,2", "    loop push_loop",
         "    mov ax,0x1366", f"    mov dx,{SYS_N:#x}", "    cmp byte ptr [bx],0", "    je chosen", "    mov ax,0x1037", f"    mov dx,{RES_N:#x}", "chosen:",
         "    mov word ptr [target_far],ax", "    mov word ptr [target_far+2],dx",
         "    xor ah,ah", "    mov al,byte ptr [bx+1]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", "    mov ax,0x4444", "    mov ds,ax"]
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
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word 0x1366, {SYS_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, data, where in (("span", span, SPAN_AT), ("res", bytes(res), RES_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(data), 16):
            t.append("    .byte " + ",".join(f"{b:#x}" for b in data[i:i + 16]))
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
    tally = {"str": 0, "res": 0}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        target = ("str", "res")[case["target"]]
        memory = bytearray(SIZE)
        at = physical(SYS_N, SPAN_LOW); memory[at:at + len(span)] = span
        at = physical(RES_N, 0x1037); memory[at:at + len(res)] = res
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (*case["words"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cs, ip = (SYS_N, 0x1366) if target == "str" else (RES_N, 0x1037)
        cpu = Cpu(*ENTRY_REGS.values(), sp, cs, ip, 0x4444, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        if target == "str":
            status = functions["str"](ram, SIZE, ctypes.byref(cpu))
        else:
            status = functions["res"](ram, SIZE, ctypes.byref(cpu), None, None)
        check(status == 0, f"C status {status} on native case {n} ({target})")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} ({target}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} ({target}) bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        tally[target] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({tally}); all 14 CPU words, the {WINDOW_SIZE:#x}-byte stack window "
          f"and the whole 64 KiB result segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM {SPAN_LOW:04X}..{SPAN_HIGH - 1:04X} at {SYS_N:04X}, 1037 at {RES_N:04X}")
    return dict(cases=len(cases), targets=tally, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    image, spans = source()
    spans["image"] = image
    report = dict(target={k: dict(bytes=v["bytes"], instructions=v["instructions"], sha256=v["sha256"]) for k, v in spans.items() if k in SPANS})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-str1037-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(spans, functions)
            if args.native:
                report["native"] = native_audit(spans, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
