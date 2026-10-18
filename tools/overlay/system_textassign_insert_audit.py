#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU SYSTEM Text Assign 0AF8:04F7 and Insert 0AF8:0C68 (which composes the merged
Copy 0B57, Concat 0B98 and StrStore 0B25 models), plus a DOSBox-X oracle.

The differential places the hash-checked expanded START.EXE image at load segment 0, so each routine and every leaf it
calls run from their original bytes, and compares the entire 1 MiB RAM and all 14 CPU words between the original under
Unicorn and the C models, with no mask, after the routine's RETF. Strings sit apart, overlapping and across segment ends;
every case keeps its stack frame apart from the data. Media stays private.
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

SYSTEM_CS = 0x0AF8
TARGETS = {"textassign": (0x04F7, 0x053C, 37, 0x0539, "8"), "insert": (0x0C68, 0x0CBC, 43, 0x0CB9, "0xc")}
LEAVES = {"strstore": (0x0B25, 0x0B49, 16), "copy": (0x0B57, 0x0B98, 29), "strcat": (0x0B98, 0x0BC4, 19)}
SYMBOLS = {"textassign": "pr_system_text_assign_04f7", "insert": "pr_system_insert_0c68"}
SS = 0x2800
RET_CS, RET_IP = 0x9001, 0x7000
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
    spans, decoded = {}, {}
    for name, (start, end, count, *rest) in {**TARGETS, **LEAVES}.items():
        span = image[physical(SYSTEM_CS, start):physical(SYSTEM_CS, end)]
        insns = list(md.disasm(span, start))
        check(len(insns) == count and sum(i.size for i in insns) == len(span), f"SYSTEM {name} differs")
        if rest:
            check(insns[-1].address == rest[0] and insns[-1].mnemonic == "retf" and insns[-1].op_str == rest[1], f"SYSTEM {name} end differs")
        decoded.update({i.address: i for i in insns})
        conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
        spans[name] = dict(bytes=len(span), instructions=count, sha256=sha256(span).hexdigest(), starts={i.address for i in insns},
                           conditional=conditional)
        if name in TARGETS:
            print(f"SYSTEM {name} {start:04X}..{end - 1:04X}: {len(span)} B, {count} insns, sha {spans[name]['sha256']}")
    calls = [i.op_str for i in md.disasm(image[physical(SYSTEM_CS, 0x0C68):physical(SYSTEM_CS, 0x0CBC)], 0x0C68) if i.mnemonic == "call"]
    check(calls == ["0xb57", "0xb98", "0xb57", "0xb98", "0xb25"], f"Insert calls differ: {calls}")
    return image, spans, decoded


def library(directory):
    path = directory / "tainsert.so"
    sources = ["system_textassign_insert_cpu.c", "system_string_leaves_cpu.c", "system_string_move_leaves_cpu.c", "system_strstore_fullstate_0b25.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    lib = ctypes.CDLL(str(path))
    functions = {}
    for name, symbol in SYMBOLS.items():
        f = getattr(lib, symbol)
        f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        f.restype = ctypes.c_int
        functions[name] = f
    return functions


def place(memory, segment, offset, data):
    for i, value in enumerate(data):
        memory[physical(segment, (offset + i) & 0xFFFF)] = value


def build(image, rng, *, target, words, data=(), sp=0x4000, ss=SS, flags=0x0602, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    for segment, offset, blob in data:
        place(memory, segment, offset, blob)
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, *words)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    reg = tuple(rng.randrange(0x10000) for _ in range(7))
    return memory, Cpu(*reg, sp, SYSTEM_CS, TARGETS[target][0], rng.randrange(0x10000), rng.randrange(0x10000), ss, flags), ss >= 0xF000


def compare(label, target, memory, cpu, high, decoded, functions):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    retf = TARGETS[target][3]
    state = dict(done=False, returned=False, unexpected=None, visited=set(), edges=set(), previous=None)

    def hook(emulator, address, size, user):
        del address, size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["done"] = True
            return emulator.emu_stop()
        if cs != SYSTEM_CS or ip not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        state["visited"].add(ip)
        if state["previous"] is not None and state["previous"] != ip:
            state["edges"].add((state["previous"], ip))
        state["previous"] = ip
        state["returned"] = ip == retf
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(SYSTEM_CS, TARGETS[target][0]), 2 * SIZE, count=300000)
    check(state["unexpected"] is None and state["done"], f"original did not return cleanly ({state['unexpected']}): {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(functions[target](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["visited"], state["edges"]


def pstring(rng, length):
    return bytes([length]) + bytes(rng.randrange(256) for _ in range(length))


def textassign_case(rng):
    mode = rng.choice(("apart", "apart", "name-in-record", "wrap"))
    length = rng.choice((0, 1, 8, 79, 80, 255, rng.randrange(256)))
    seg = rng.randrange(0x3000, 0x7000)
    if mode == "apart":
        t, s = (seg, rng.randrange(0, 0xF000, 2)), (rng.randrange(0x3000, 0x7000), rng.randrange(0xF000))
    elif mode == "name-in-record":
        off = rng.randrange(0, 0xE000, 2); t, s = (seg, off), (seg, off + rng.randrange(0, 128))
    else:
        t, s = (seg, rng.randrange(0xFF00, 0x10000, 2)), (rng.randrange(0x3000, 0x7000), rng.randrange(0xFF00, 0x10000))
    data = [(t[0], t[1], bytes(rng.randrange(256) for _ in range(260))), (s[0], s[1], pstring(rng, length))]
    return mode, dict(words=(s[1], s[0], t[1], t[0]), data=data)


def insert_case(rng):
    mode = rng.choice(("apart", "apart", "source-in-s", "wrap"))
    slen, srclen = rng.choice((0, 1, 5, 100, 200, 255, rng.randrange(256))), rng.choice((0, 1, 3, 60, 255, rng.randrange(256)))
    index = rng.choice((0xFFFB, 0, 1, 2, slen, slen + 1, slen + 2, 255, 0x7FFF, 0x8000, rng.randrange(1, slen + 2)))
    maximum = rng.choice((255, 255, 10, 1, 0, rng.randrange(256)))
    seg = rng.randrange(0x3000, 0x7000)
    if mode == "apart":
        s, src = (seg, rng.randrange(0xF000)), (rng.randrange(0x3000, 0x7000), rng.randrange(0xF000))
    elif mode == "source-in-s":
        off = rng.randrange(0xE000); s, src = (seg, off), (seg, off + rng.randrange(0, 200))
    else:
        s, src = (seg, rng.randrange(0xFF00, 0x10000)), (rng.randrange(0x3000, 0x7000), rng.randrange(0xFF00, 0x10000))
    data = [(s[0], s[1], pstring(rng, slen)), (src[0], src[1], pstring(rng, srclen))]
    return mode, dict(words=(index, maximum, s[1], s[0], src[1], src[0]), data=data)


def cases(rng):
    spec = []
    for i in range(300):
        target = ("textassign", "insert")[i % 2]
        mode, options = (textassign_case if target == "textassign" else insert_case)(rng)
        spec.append((f"random-{i}-{target}-{mode}", target, dict(options, flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0x7000) & ~1)))
    for target, maker in (("textassign", textassign_case), ("insert", insert_case)):
        _, options = maker(rng)
        spec.append((f"{target}-ss-a20", target, dict(options, sp=0x0800, ss=0xFFFF)))
        _, options = maker(rng)
        spec.append((f"{target}-df-set", target, dict(options, flags=0x0603)))
        _, options = maker(rng)
        spec.append((f"{target}-random-background", target, dict(options, background=bytes(random.Random(0x0C68 + len(target)).randbytes(SIZE)))))
    return spec


def differential(spans, decoded, functions):
    rng = random.Random(0x04F70C68)
    visited, edges, counts = set(), set(), {k: 0 for k in TARGETS}
    for label, target, options in cases(rng):
        memory, cpu, high = build(spans["image"], rng, target=target, **options)
        v, e = compare(label, target, memory, cpu, high, decoded, functions)
        visited |= v
        edges |= e
        counts[target] += 1
    for name in TARGETS:
        reachable = {(s, o) for s, outs in spans[name]["conditional"].items() for o in outs}
        check(spans[name]["starts"] <= visited and reachable <= edges, f"{name} coverage incomplete")
    print(f"SYSTEM Text Assign/Insert: {sum(counts.values())} original-byte cases ({counts}); full 1 MiB RAM and all 14 CPU words equal "
          f"after each RETF, no mask; every instruction of both and both outcomes of Text Assign's JBE executed")
    return dict(cases=counts)


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
SYS_N, DATA_N = 0x7000, 0x5000
SPAN_LOW, SPAN_HIGH = 0x0400, 0x0D00
MAIN_AT, SPAN_AT, BLOB_AT, CASES_AT = 0x0300, 0x0C00, 0x1600, 0x2600
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, BLOB_SIZE = STACK_TOP - 0x280, 0x280, 0x1000
CASE_SIZE, NATIVE_CASES = 32, 128
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        chunks = []

        def string(offset, length):
            chunks.append((offset & 0xFFFF, 1, length)); chunks.append(((offset + 1) & 0xFFFF, length, rng.randrange(256, BLOB_SIZE - 256)))

        if n % 2 == 0:
            length = rng.choice((0, 1, 9, 79, 80, 255, rng.randrange(256)))
            t = rng.choice((rng.randrange(0x0000, 0xF000, 2), rng.randrange(0xFF00, 0x10000, 2)))
            s = rng.choice((rng.randrange(0x0000, 0xF000), (t + rng.randrange(0, 128)) & 0xFFFF))
            string(s, length)
            cases.append(dict(target=0, flags=n % len(NATIVE_FLAGS), words=(DATA_N, t, DATA_N, s), chunks=chunks))
        else:
            slen, srclen = rng.choice((0, 1, 5, 100, 255, rng.randrange(256))), rng.choice((0, 3, 60, 255, rng.randrange(256)))
            s = rng.choice((rng.randrange(0x0000, 0xF000), rng.randrange(0xFF00, 0x10000)))
            src = rng.choice((rng.randrange(0x0000, 0xF000), (s + rng.randrange(0, 200)) & 0xFFFF))
            string(src, srclen)
            string(s, slen)
            index = rng.choice((0xFFFB, 0, 1, 2, slen, slen + 1, 0x8000, rng.randrange(1, slen + 2)))
            cases.append(dict(target=1, flags=n % len(NATIVE_FLAGS), words=(DATA_N, src, DATA_N, s, rng.choice((255, 10, 0)), index), chunks=chunks))
    return cases


def case_bytes(case):
    data = struct.pack("<BBBB", case["target"], case["flags"], len(case["words"]), len(case["chunks"]))
    data += struct.pack(f"<{len(case['words'])}H", *case["words"]) + bytes(12 - 2 * len(case["words"]))
    data += b"".join(struct.pack("<HHH", *chunk) for chunk in case["chunks"])
    check(len(data) <= CASE_SIZE + 12, "case too large")
    return data.ljust(CASE_SIZE + 12, b"\0")


def native_memory(case, blob):
    data = bytearray([0xA5]) * 0x10000
    for offset, length, start in case["chunks"]:
        for i in range(length):
            data[(offset + i) & 0xFFFF] = blob[start + i]
    return data


def native_audit(spans, functions):
    directory = ROOT / "resources/oracle-runs/system-textassign-insert-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    image = spans["image"]
    span = image[physical(SYSTEM_CS, SPAN_LOW):physical(SYSTEM_CS, SPAN_HIGH)]
    rng = random.Random(0x04F70C68D)
    blob = bytes(range(256)) + bytes(rng.randrange(256) for _ in range(BLOB_SIZE - 256))
    cases = native_cases(rng)
    size = CASE_SIZE + 12
    table = b"".join(case_bytes(c) for c in cases)
    check(SPAN_AT + len(span) <= BLOB_AT and BLOB_AT + len(blob) <= CASES_AT and CASES_AT + len(table) <= WINDOW_LOW - 0x100, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{SYS_N:#x}", "    mov es,ax", "    mov si,offset span", f"    mov di,{SPAN_LOW:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{size}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    lea ax,[bx+16]", "    mov word ptr [chunk_ptr],ax", "    xor ah,ah", "    mov al,byte ptr [bx+3]", "    mov word ptr [chunk_count],ax",
         "chunk_loop:", "    cmp word ptr [chunk_count],0", "    je chunks_done", "    mov bp,word ptr [chunk_ptr]",
         "    mov di,word ptr ds:[bp]", "    mov cx,word ptr ds:[bp+2]", "    mov si,offset blob", "    add si,word ptr ds:[bp+4]", "    rep movsb",
         "    add word ptr [chunk_ptr],6", "    dec word ptr [chunk_count]", "    jmp chunk_loop", "chunks_done:",
         "    xor ch,ch", "    mov cl,byte ptr [bx+2]", "    lea si,[bx+4]", "push_loop:", "    push word ptr [si]", "    add si,2", "    loop push_loop",
         "    mov ax,0x04f7", "    cmp byte ptr [bx],0", "    je chosen", "    mov ax,0x0c68", "chosen:", "    mov word ptr [target_far],ax",
         "    xor ah,ah", "    mov al,byte ptr [bx+1]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", "    mov ax,0x4444", "    mov ds,ax"]
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "chunk_ptr: .word 0", "chunk_count: .word 0",
          f"target_far: .word 0x04f7, {SYS_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, data, where in (("span", span, SPAN_AT), ("blob", blob, BLOB_AT), ("cases", table, CASES_AT)):
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
        target = ("textassign", "insert")[case["target"]]
        memory = bytearray(SIZE)
        at = physical(SYS_N, SPAN_LOW); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = native_memory(case, blob)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (*case["words"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, SYS_N, TARGETS[target][0], 0x4444, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(functions[target](ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} ({target}) {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} ({target}) bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (Text Assign and Insert alternating); all 14 CPU words, the "
          f"{WINDOW_SIZE:#x}-byte stack window and the whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM span {SPAN_LOW:04X}..{SPAN_HIGH - 1:04X} at {SYS_N:04X}")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    image, spans, decoded = source()
    spans["image"] = image
    report = dict(target={k: dict(bytes=v["bytes"], instructions=v["instructions"], sha256=v["sha256"]) for k, v in spans.items() if k in TARGETS})
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-tainsert-") as temp:
            functions = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(spans, decoded, functions)
            if args.native:
                report["native"] = native_audit(spans, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
