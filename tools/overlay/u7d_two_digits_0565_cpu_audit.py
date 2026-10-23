#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U7D +0565 (a byte as a two-character string), composing resident 00BA:1037
(which composes SYSTEM Str and StrStore) and the SYSTEM StrStore, StrLoad, StrConcat and Copy leaves, plus a DOSBox-X
oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so the resident routine and every
SYSTEM leaf run from their original bytes, loads unit 0x7D, and compares the entire 1 MiB RAM and all 14 CPU words
between the original under Unicorn and the C model, with no mask. The original decides each stop: the fetch after its
own RETF 2 (return), one of its seven LCALLs whose live bytes differ when fetched (CODE), a nested stop inside 00BA:1037
or Str (CALLEE), or a RET/RETF that does not come back to its call (TRANSFER). Any fetch outside the pre-decoded spans is
an error. Every case keeps its stack frame apart from the result string. Media stays private.
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

SYSTEM_CS, RES_CS, CALLER_CS = 0x0AF8, 0x00BA, 0x8800
ENTRY, END, BODY_RETF = 0x0565, 0x05E3, 0x05E0
SITES = {0x0576: (0x1037, "res"), 0x0584: (0x0B25, "sys"), 0x059A: (0x0B0B, "sys"), 0x05A4: (0x0B98, "sys"),
         0x05B2: (0x0B25, "sys"), 0x05CA: (0x0B57, "sys"), 0x05D8: (0x0B25, "sys")}
SYSTEM_SPANS = {"str": (0x1366, 0x13B1, 39), "core": (0x1254, 0x12A6, 37), "strstore": (0x0B25, 0x0B49, 16),
                "strload": (0x0B0B, 0x0B25, 12), "strcat": (0x0B98, 0x0BC4, 19), "copy": (0x0B57, 0x0B98, 29)}
RES_SPAN, RES_SITES = (0x1037, 0x1073, 28), {0x1054: b"\x9a\x66\x13", 0x1068: b"\x9a\x25\x0b"}
TABLE, POWERS = 0x12A6, tuple(10 ** k for k in range(9, -1, -1))
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
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR provenance differs")
    image = bytes(unpack(exe)[0])
    check(len(image) <= physical(SS, 0), "the image reaches the stack segment")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    u = units[0x7D]
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[ENTRY:END]
    insns = list(md.disasm(body, ENTRY))
    calls = {i.address: i.op_str for i in insns if "call" in i.mnemonic}
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(body) == 126 and len(insns) == 54 and sum(i.size for i in insns) == 126 and insns[-1].address == BODY_RETF
          and insns[-1].op_str == "2" and sorted(calls) == sorted(SITES) and all(site + 3 in set(u["fixups"]) for site in SITES)
          and code[0x0563:0x0565] == b"\x01\x30", f"U7D +0565 extent or calls differ: {calls}")
    decoded = {("u7d", i.address): i for i in insns}
    spans = dict(u7d=dict(bytes=len(body), instructions=len(insns), sha256=sha256(body).hexdigest(), file_offset=u["ovr_offset"] + ENTRY))
    for name, (start, end, count) in SYSTEM_SPANS.items():
        part = image[physical(SYSTEM_CS, start):physical(SYSTEM_CS, end)]
        found = list(md.disasm(part, start))
        check(len(found) == count and sum(i.size for i in found) == len(part), f"SYSTEM {name} differs")
        decoded.update({("sys", i.address): i for i in found})
    part = image[physical(RES_CS, RES_SPAN[0]):physical(RES_CS, RES_SPAN[1])]
    found = list(md.disasm(part, RES_SPAN[0]))
    check(len(found) == RES_SPAN[2] and sum(i.size for i in found) == len(part), "00BA:1037 differs")
    decoded.update({("res", i.address): i for i in found})
    check(struct.unpack("<10I", image[physical(SYSTEM_CS, TABLE):physical(SYSTEM_CS, TABLE + 40)]) == POWERS, "power table differs")
    print(f"U7D +0565 (126/54, seven LCALLs, RETF 2; '0' at +0563) GAME file {u['ovr_offset'] + ENTRY:#x} sha {spans['u7d']['sha256']}")
    return dict(conditional=conditional, starts=[i.address for i in insns]), code, image, decoded, spans


CHECKER_ROWS = ('OVL_007D_0565',)


def library(directory):
    path = directory / "u7d0565.so"
    sources = ["overlay_u7d_two_digits_0565_cpu.c", "resident_00ba_byte_string_1037_cpu.c", "system_str_1366_cpu.c",
               "system_strstore_fullstate_0b25.c", "system_string_leaves_cpu.c", "system_string_move_leaves_cpu.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_u7d_two_digits_0565_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int),
                         ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def build(code, image, rng, *, b, result, sp=0x4000, ss=SS, flags=0x0602, background=None, patch=(), table=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    at = physical(CALLER_CS, 0); memory[at:at + len(code)] = code
    for segment, offset, value in patch:
        memory[physical(segment, offset)] = value
    if table is not None:
        at = physical(SYSTEM_CS, TABLE); memory[at:at + 40] = table
    region = {physical(result[0], (result[1] + i) & 0xFFFF) for i in range(64)}
    check(not region & (set(range(len(image))) | set(range(physical(CALLER_CS, 0), physical(CALLER_CS, len(code))))),
          "the result string overlaps code")
    for i in range(64):
        memory[physical(result[0], (result[1] + i) & 0xFFFF)] = rng.randrange(256)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, (rng.randrange(256) << 8) | b, result[1], result[0])):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, CALLER_CS, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, memory, cpu, high, target, decoded, function):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, calls=[], unexpected=None)
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
                state["stop"] = CALLEE if state["calls"] else TRANSFER
                return emulator.emu_stop()
        kind = "u7d" if cs == CALLER_CS else "res" if cs == RES_CS else "sys" if cs == SYSTEM_CS else None
        key = (kind, ip)
        if key not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if kind == "u7d" and ip in SITES and bytes(emulator.mem_read(address, 3)) != b"\x9a" + struct.pack("<H", SITES[ip][0]):
            state["stop"] = CODE
            return emulator.emu_stop()
        if kind == "res" and ip in RES_SITES and bytes(emulator.mem_read(address, 3)) != RES_SITES[ip]:
            state["stop"] = CALLEE
            return emulator.emu_stop()
        if key == ("sys", 0x1366) and struct.unpack("<10I", bytes(emulator.mem_read(physical(cs, TABLE), 40))) != POWERS:
            state["stop"] = CALLEE
            return emulator.emu_stop()
        if kind == "u7d":
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
    original.emu_start(physical(CALLER_CS, ENTRY), 2 * SIZE, count=400000)
    check(state["unexpected"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    callee, callee_status = ctypes.c_int(99), ctypes.c_int(99)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee), ctypes.byref(callee_status))
    check(status == state["stop"], f"C status {status} ({callee.value}/{callee_status.value}), original {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges


def result_pointer(rng):
    return rng.randrange(0x3000, 0x7000), rng.choice((rng.randrange(0xF000), rng.randrange(0xFF00, 0x10000)))


def cases(rng):
    spec = []
    for b in range(256):
        spec.append((f"byte-{b}", dict(b=b, result=result_pointer(rng), flags=FLAGS[b % len(FLAGS)], sp=rng.randrange(0x0800, 0x7000) & ~1)))
    for site, (target, _) in SITES.items():
        spec.append((f"code-{site:04x}", dict(b=rng.choice((3, 42)) if site not in (0x059A, 0x05A4, 0x05B2) else 3, result=result_pointer(rng),
                                               patch=((CALLER_CS, site + 1, (target & 0xFF) ^ 0x40),))))
    spec.append(("callee-1037-site", dict(b=7, result=result_pointer(rng), patch=((RES_CS, 0x1055, 0x67),))))
    spec.append(("callee-str-table", dict(b=7, result=result_pointer(rng), table=struct.pack("<10I", *POWERS[:9], 2))))
    spec.append(("frame-wraps", dict(b=5, result=result_pointer(rng), sp=0x0200, ss=0x1000)))
    spec.append(("ss-a20", dict(b=99, result=result_pointer(rng), sp=0x0800, ss=0xFFFF)))
    spec.append(("df-set", dict(b=1, result=result_pointer(rng), flags=0x0603)))
    spec.append(("random-background", dict(b=200, result=result_pointer(rng), background=bytes(random.Random(0x7D0565).randbytes(SIZE)))))
    return spec


def differential(target, code, image, decoded, function):
    rng = random.Random(0x7D0565)
    visited, edges, stops = set(), set(), {k: 0 for k in NAMES}
    for label, options in cases(rng):
        memory, cpu, high = build(code, image, rng, **options)
        stop, v, e = compare(f"+0565 {label}", memory, cpu, high, target, decoded, function)
        if not label.startswith("byte"):
            print(f"  {label}: {NAMES[stop]}")
        stops[stop] += 1
        visited |= v
        edges |= e
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    missing = sorted(hex(a) for a in target["starts"] if a not in visited)
    check(not missing and reachable <= edges, f"coverage incomplete: {missing} {sorted(reachable - edges)}")
    check(stops[OK] and stops[CODE] and stops[CALLEE], f"a stop kind was not exercised: {stops}")
    print(f"U7D +0565: {sum(stops.values())} original-byte cases ({', '.join(f'{v} {NAMES[k]}' for k, v in stops.items())}); full 1 MiB RAM "
          f"and all 14 CPU words equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts, {len(edges)}/{len(reachable)} "
          f"conditional outcomes; every byte value")
    return dict(stops={NAMES[k]: v for k, v in stops.items()}, starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, RES_N, CALLER_N, DATA_N = 0x7000, 0x7800, 0x8800, 0x5000
SPAN_LOW, SPAN_HIGH = 0x0B00, 0x1400
MAIN_AT, SPAN_AT, RES_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x1600, 0x1700, 0x2000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x300, 0x300
CASE_SIZE, NATIVE_CASES = 8, 128
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_audit(code, image, function):
    directory = ROOT / "resources/oracle-runs/u7d-0565-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = image[physical(SYSTEM_CS, SPAN_LOW):physical(SYSTEM_CS, SPAN_HIGH)]
    res = bytearray(image[physical(RES_CS, RES_SPAN[0]):physical(RES_CS, RES_SPAN[1])])
    for site in RES_SITES:
        struct.pack_into("<H", res, site + 3 - RES_SPAN[0], SYS_N)
    unit = bytearray(code)
    for site, (_, kind) in SITES.items():
        struct.pack_into("<H", unit, site + 3, RES_N if kind == "res" else SYS_N)
    rng = random.Random(0x7D0565D)
    cases = [dict(b=n if n < 24 else rng.randrange(256), high=rng.randrange(256), result=rng.choice((rng.randrange(0xF000), rng.randrange(0xFF00, 0x10000))),
                  flags=n % len(NATIVE_FLAGS)) for n in range(NATIVE_CASES)]
    table = b"".join(struct.pack("<BBHH2x", c["flags"], c["b"], (c["high"] << 8) | c["b"], c["result"]) for c in cases)
    check(SPAN_AT + len(span) <= RES_AT and RES_AT + len(res) <= CASES_AT and CASES_AT + len(table) <= CODE_AT and CODE_AT + len(unit) <= OUTPUT,
          "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, label, offset, length in ((SYS_N, "span", SPAN_LOW, len(span)), (RES_N, "res", RES_SPAN[0], len(res)), (CALLER_N, "unit", 0, len(unit))):
        t += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    t += ["    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
          f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
          "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
          "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
          "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
          "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
          f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
          f"    mov ax,{DATA_N:#x}", "    push ax", "    push word ptr [bx+4]", "    push word ptr [bx+2]",
          "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, data, where in (("span", span, SPAN_AT), ("res", bytes(res), RES_AT), ("cases", table, CASES_AT), ("unit", bytes(unit), CODE_AT)):
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
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(SYS_N, SPAN_LOW); memory[at:at + len(span)] = span
        at = physical(RES_N, RES_SPAN[0]); memory[at:at + len(res)] = res
        at = physical(CALLER_N, 0); memory[at:at + len(unit)] = unit
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (DATA_N, case["result"], (case["high"] << 8) | case["b"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, 0x4444, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = function(ram, SIZE, ctypes.byref(cpu), None, None)
        check(status == OK, f"C status {status} on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (b = 0..23 and random); all 14 CPU words, the {WINDOW_SIZE:#x}-byte "
          f"stack window and the whole 64 KiB result segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U7D at {CALLER_N:04X}, 1037 at {RES_N:04X}, SYSTEM at {SYS_N:04X}")
    return dict(cases=len(cases), result_sha256=digest)


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
    target, code, image, decoded, spans = source()
    report = dict(target=spans["u7d"])
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u7d0565-") as temp:
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
