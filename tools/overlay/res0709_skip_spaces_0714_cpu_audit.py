#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU resident 0709:0714 nested space skip (RES 00BA:16EC closure).

The differential loads the original START.EXE span 0709:0714..074D in a 1 MiB image, builds a parent frame P in SS
(index byte P-103h, limit byte P-101h, text from P-100h) and compares the entire RAM and all 14 CPU words between the
original under Unicorn and the C model, with no mask; the original decides LIMIT by counting taken JMP 0717. One case
lets the INC hit the stack word holding P. The DOSBox-X synthetic oracle repeats random frames natively with the parent
area inside the captured stack window. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ("RES_0709_0714",)
R709, ENTRY, END, BODY_RETF, LOOP_JMP, LOOP_HEAD = 0x0709, 0x0714, 0x074E, 0x074B, 0x0748, 0x0717
SS = 0x4000
RET_CS, RET_IP = 0x9001, 0x7000
OK, LIMIT = 0, 2
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    span = bytes(unpack(exe)[0])[physical(R709, ENTRY):physical(R709, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(len(insns) == 19 and sum(i.size for i in insns) == len(span) and (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "2")
          and not any(i.mnemonic in ("call", "lcall", "in", "out", "int") for i in insns), "0709:0714 differs")
    check([i.address for i in insns if i.mnemonic in ("retf", "ret", "iret")] == [BODY_RETF], "media span must end at the first RETF")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(span).hexdigest()
    print(f"0709:0714..074D (58/19, call-free, RETF 2) sha {sha}")
    return dict(starts=[i.address for i in insns], conditional=conditional, sha=sha,
                image_span=[physical(R709, ENTRY), physical(R709, END)]), span


def library(directory):
    path = directory / "skip.so"
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            str(ROOT / "src/recovered/resident_0709_skip_spaces_0714_cpu.c"), "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_r0709_skip_spaces_0714_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint]
    function.restype = ctypes.c_int
    return function


def parent_bytes(rng, index, limit):
    text = bytes(rng.choice((0x20, 0x20, 0x20, 0x41, 0x00, 0x21, rng.randrange(256))) for _ in range(256))
    return index, limit, text


def build(span, rng, *, index, limit, text, p, ss=SS, sp=0x2000, flags=0x7202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(R709, ENTRY); memory[at:at + len(span)] = span
    for i, byte in enumerate(text):
        memory[physical(ss, (p - 0x100 + i) & 0xFFFF)] = byte
    memory[physical(ss, (p - 0x103) & 0xFFFF)] = index
    memory[physical(ss, (p - 0x101) & 0xFFFF)] = limit
    # The call frame lives at SS:(SP+2i) mod 64K only. A second, physically contiguous copy
    # would make a wrapped and a contiguous frame read indistinguishable at SP FFFC..FFFF.
    for i, value in enumerate((RET_IP, RET_CS, p)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, R709, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, memory, cpu, high, target, function, cap, split=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, end = physical(R709, ENTRY), physical(R709, END)
    state = dict(stop=None, returned=False, previous=None, loopbacks=0, illegal=None)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        linear = address & (2 * SIZE - 1)
        if not start <= linear < end:
            state["illegal"] = linear
            return emulator.emu_stop()
        here = linear - start + ENTRY
        if here == LOOP_HEAD and state["previous"] == LOOP_JMP:
            state["loopbacks"] += 1
            if state["loopbacks"] > cap:
                state["stop"] = LIMIT
                return emulator.emu_stop()
        visited.add(here)
        if state["previous"] in target["conditional"] and state["previous"] != here:
            edges.add((state["previous"], here))
        state["previous"] = here
        state["returned"] = here == BODY_RETF
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=200000)
    check(state["illegal"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['illegal']}): {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), cap)
    check(status == state["stop"], f"C status {status}, original {state['stop']}: {label}")
    expected, actual = bytes(original.mem_read(0, SIZE)), bytes(ram)
    if actual != expected:
        differences = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        theirs = original.reg_read(register) & 0xFFFF
        if name == "cs" and split is not None:
            # Declared oracle split: Unicorn reads the RETF CS word physically contiguous, the
            # model wraps it within SS as DOSBox-X does. CS is the ONLY permitted difference.
            check(cpu.cs == RET_CS and theirs == split != RET_CS, f"split {label}: C={cpu.cs:04x}, original={theirs:04x}, contiguous={split:04x}")
            continue
        check(getattr(cpu, name) == theirs, f"{name} mismatch {label}: C={getattr(cpu, name):04x}, original={theirs:04x}")
    return state["stop"], visited, edges, state["loopbacks"]


def differential(target, span, function):
    rng = random.Random(0x07090714)
    spec = []
    for i in range(150):
        limit = rng.choice((0, 1, 5, 40, 0xFF, rng.randrange(256)))
        index = rng.choice((0, 0, 1, limit, max(limit - 3, 0), rng.randrange(256)))
        _, _, text = parent_bytes(rng, index, limit)
        spec.append((f"random-{i}", dict(index=index, limit=limit, text=text, p=rng.choice((0x8000, 0x0102, rng.randrange(0x4000, 0xF000))),
                                         sp=rng.randrange(0x0800, 0x3000) & ~1, flags=FLAGS[i % len(FLAGS)]), 400))
    spaces = b" " * 256
    spec += [("all-spaces-limit", dict(index=0, limit=0xFF, text=spaces, p=0x8000), 400),
             ("cap-two", dict(index=0, limit=0x40, text=spaces, p=0x8000), 2),
             ("index-wraps-p", dict(index=0, limit=0xFF, text=spaces, p=0x2107, sp=0x2000), 50),
             ("ss-a20", dict(index=0, limit=9, text=b"   X" + spaces, p=0x8000, ss=0xFFFF, sp=0x0800), 400),
             ("df-set", dict(index=1, limit=9, text=b"   X" + spaces, p=0x8000, flags=0x0603), 400),
             ("random-background", dict(index=0, limit=9, text=b"  Y" + spaces, p=0x8000, background=bytes(random.Random(0x07090714).randbytes(SIZE))), 400)]
    # Stack-offset edges where Unicorn and DOSBox-X agree (measured): word offsets wrap mod 64K,
    # a word straddling offset FFFF is linear-contiguous. SP FFFE/FFFF are NOT here: there the
    # RETF CS word lies wholly past the wrap and the two oracles disagree (unresolved).
    spec += [(f"sp-edge-{sp:04x}", dict(index=0, limit=3, text=b"  X" + spaces, p=0x8000, sp=sp), 400)
             for sp in (0x0000, 0x0001, 0x0002, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD)]
    # SP FFFE/FFFF: the RETF CS word lies wholly past the wrap. DOSBox-X (measured) and the model
    # read SS:0000/0001; Unicorn reads SS*16+10000h/10001h. Declared split, CS only.
    spec += [(f"sp-split-{sp:04x}", dict(index=0, limit=3, text=b"  X" + spaces, p=0x8000, sp=sp, split=True), 400)
             for sp in (0xFFFE, 0xFFFF)]
    visited, edges, stops, most, splits = set(), set(), {OK: 0, LIMIT: 0}, 0, []
    for label, options, cap in spec:
        options = dict(options); split = options.pop("split", False)
        memory, cpu, high = build(span, rng, **options)
        contiguous = None
        if split:
            at = (physical(cpu.ss, cpu.sp) + 2) & (SIZE - 1)
            contiguous = memory[at] | memory[(at + 1) & (SIZE - 1)] << 8
            splits.append(dict(case=label, sp=f"{cpu.sp:04X}", model_cs=f"{RET_CS:04X}", unicorn_cs=f"{contiguous:04X}", differing="CS"))
        stop, v, e, n = compare(f"+0714 {label}", memory, cpu, high, target, function, cap, contiguous)
        stops[stop] += 1; visited |= v; edges |= e; most = max(most, n)
        if not label.startswith("random"):
            print(f"  {label}: {'return' if stop == OK else 'LIMIT'} after {n} loop-backs")
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    print(f"0709:0714: {len(spec) - len(splits)} original-byte cases equal ({stops[OK] - len(splits)} return, {stops[LIMIT]} LIMIT) on full 1 MiB RAM and all 14 CPU words, "
          f"no mask, plus {len(splits)} declared CS-only oracle splits (all else equal); {len(visited)}/{len(target['starts'])} instruction starts, {len(edges & reachable)}/{len(reachable)} conditional outcomes; "
          f"up to {most} loop-backs")
    check(len(visited) == len(target["starts"]) and reachable <= edges and stops[LIMIT], "coverage or stop kinds incomplete")
    return dict(cases=len(spec) - len(splits), oracle_splits=splits, stops=dict(RETURN=stops[OK] - len(splits), LIMIT=stops[LIMIT], SPLIT_RETURN=len(splits)), starts=len(visited), starts_total=len(target["starts"]),
                outcomes=len(edges & reachable), outcomes_total=len(reachable), successor_based=True, cpu_words=14, ram_bytes=SIZE, flag_masks=0)


# ---- DOSBox-X synthetic oracle: the parent frame sits inside the captured stack window ------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE, P_NATIVE = STACK_TOP - 0x200, 0x200, 0xEF10
NATIVE_CASES, CASE_SIZE = 32, 0x240
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_audit(span, function):
    import os
    directory = ROOT / "resources/oracle-runs/res0709-skip-spaces-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x07090714D)
    cases = []
    for n in range(NATIVE_CASES):
        window = bytearray([0xA5]) * WINDOW_SIZE
        limit = rng.choice((0, 3, 9, 40, rng.randrange(256)))
        index = rng.choice((0, 0, 1, limit, rng.randrange(256)))
        text = bytes(rng.choice((0x20, 0x20, 0x41, rng.randrange(256))) for _ in range(256))
        base = P_NATIVE - 0x100 - WINDOW_LOW
        window[base:base + 256] = text
        window[P_NATIVE - 0x103 - WINDOW_LOW] = index; window[P_NATIVE - 0x101 - WINDOW_LOW] = limit
        writes = [(WINDOW_LOW + o, window[o] | (window[o + 1] << 8)) for o in range(0, WINDOW_SIZE, 2) if window[o] != 0xA5 or window[o + 1] != 0xA5]
        check(4 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=n % len(NATIVE_FLAGS), window=window, writes=writes))
    table = b"".join((struct.pack("<BBH", c["flags"], 0, len(c["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"]))
                     .ljust(CASE_SIZE, b"\0") for c in cases)
    check(CASES_AT + len(table) <= OUTPUT and P_NATIVE - 0x103 >= WINDOW_LOW and P_NATIVE + 0x10 < STACK_TOP - 0x20, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw", "    push cs", "    pop es",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+4]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
         f"    mov ax,{P_NATIVE:#x}", "    push ax",
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
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
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{NATIVE_CASES}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("wrap", span, WRAP_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
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
    check(len(result) == 8 + NATIVE_CASES * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == NATIVE_CASES and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    skipped = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = b"\xa5" * 0x10000
        at = physical(segment, WINDOW_LOW); memory[at:at + WINDOW_SIZE] = case["window"]
        sp = STACK_TOP
        for value in (P_NATIVE, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), 1000) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        skipped += ram[physical(segment, P_NATIVE - 0x103)] != case["window"][P_NATIVE - 0x103 - WINDOW_LOW]
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES} ({skipped} skipped at least one space); all 14 CPU words, the "
          f"512-byte stack window holding the parent frame, and the whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; routine at {CALLER_N:04X}:{ENTRY:04X}")
    return dict(cases=NATIVE_CASES, skipped=skipped, result_sha256=digest)


# ---- DOSBox-X stack-offset edges: wrapped vs contiguous frame words, decided natively ---------------------
EDGE_SPS = (0x0000, 0x0001, 0x0002, 0xFFFA, 0xFFFB, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF)
EDGE_T, EDGE_REC = 0x4000, 32
EDGE_PADS = {1: (0, 0x1100), 2: (0, 0x1200), 3: (0x100, 0x1100), 4: (0x100, 0x1200)}


def edge_frame(sp, com):
    """Bytes of the call frame: segment-wrapped (IP 1100h, CS=COM, P 8000h) at SS:(SP+j) mod 64K, and for
    every byte whose linear address passes SS*16+FFFF a different contiguous value (IP 1200h, CS=COM+100h,
    P 9000h), so the landing and the parent read show which copy the CPU used."""
    wrapped, contiguous = struct.pack("<3H", 0x1100, com, 0x8000), struct.pack("<3H", 0x1200, com + 0x100, 0x9000)
    return [((sp + j) & 0xFFFF, wrapped[j], contiguous[j] if sp + j > 0xFFFF else None) for j in range(6)]


def edge_model(span, function, sp, com):
    memory = bytearray(SIZE)
    at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
    for off in list(range(0xFFF8, 0x10000)) + list(range(8)):
        memory[physical(EDGE_T, off)] = 0xA5
    for off in range(8):
        memory[physical(EDGE_T + 0x1000, off)] = 0xA5
    for base, limit, text in ((0x8000, 3, b"  X"), (0x9000, 4, b"   X")):
        memory[physical(EDGE_T, base - 0x103)] = 0; memory[physical(EDGE_T, base - 0x101)] = limit
        for i, byte in enumerate(text):
            memory[physical(EDGE_T, base - 0x100 + i)] = byte
    for off, value, contiguous in edge_frame(sp, com):
        memory[physical(EDGE_T, off)] = value
        if contiguous is not None:
            memory[physical(EDGE_T + 0x1000, off)] = contiguous
    cpu = Cpu(0, 0, 0, 0, 0, 0, 0x7777, sp, CALLER_N, ENTRY, com, 0, EDGE_T, 0x0046)
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu), 1000) == OK, f"edge model status at SP {sp:04x}")
    word = lambda seg, off: ram[physical(seg, off)] | ram[physical(seg, off + 1)] << 8
    return cpu, (word(EDGE_T, 0), word(EDGE_T, 2), word(EDGE_T, 0xFFFE), word(EDGE_T + 0x1000, 0), word(EDGE_T + 0x1000, 2))


def native_sp_edges(span, function):
    import os
    directory = ROOT / "resources/res0709-skip/native-edges"; directory.mkdir(parents=True, exist_ok=True)
    (directory / "body.bin").write_bytes(span)
    T = EDGE_T
    a = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "cli", "mov ax,cs", "mov ds,ax", "cld",
         "mov word ptr [records-2],cs",
         f"mov ax,{CALLER_N}", "mov es,ax", f"mov di,{ENTRY}", "mov si,offset body", f"mov cx,{len(span)}", "rep movsb",
         "xor ax,ax", "mov es,ax", "mov ax,word ptr es:[0x34]", "mov word ptr [old0d],ax", "mov ax,word ptr es:[0x36]",
         "mov word ptr [old0d+2],ax", "mov word ptr es:[0x34],offset fault", "mov word ptr es:[0x36],cs", f"mov ax,{T}", "mov es,ax"]
    for base, limit, text in ((0x8000, 3, b"  X"), (0x9000, 4, b"   X")):
        a += [f"mov byte ptr es:[{base - 0x103}],0", f"mov byte ptr es:[{base - 0x101}],{limit}"]
        a += [f"mov byte ptr es:[{base - 0x100 + i}],{b}" for i, b in enumerate(text)]
    a += ["mov word ptr [back],offset back_here", "mov word ptr [back+2],cs"]
    for k, sp in enumerate(EDGE_SPS):
        a += [f"case{k}:", f"mov byte ptr [cur],{k}", f"mov ax,{T}", "mov es,ax"]
        a += [f"mov byte ptr es:[{off}],0xa5" for off in list(range(0xFFF8, 0x10000)) + list(range(8))]
        a += [f"mov ax,{T + 0x1000}", "mov es,ax"] + [f"mov byte ptr es:[{off}],0xa5" for off in range(8)]
        a += ["mov bx,cs", "mov dx,bx", "add dx,0x100"]
        for j, (off, _, contiguous) in enumerate(edge_frame(sp, 0)):
            src_w = {0: "0", 1: "0x11", 2: "bl", 3: "bh", 4: "0", 5: "0x80"}[j]
            src_c = {0: "0", 1: "0x12", 2: "dl", 3: "dh", 4: "0", 5: "0x90"}[j]
            a += [f"mov ax,{T}", "mov es,ax", f"mov byte ptr es:[{off}],{src_w}"]
            if contiguous is not None:
                a += [f"mov ax,{T + 0x1000}", "mov es,ax", f"mov byte ptr es:[{off}],{src_c}"]
        a += ["mov word ptr [save_sp],sp", "mov word ptr [save_ss],ss", f"mov ax,{T}", "mov ss,ax", f"mov sp,{sp}",
              "xor ax,ax", "xor di,di", "mov bp,0x7777", ".byte 0xea", f".word {ENTRY}", f".word {CALLER_N}"]
    a += ["back_here:", "mov ax,cs", "mov ds,ax", "mov ss,word ptr [save_ss]", "mov sp,word ptr [save_sp]",
          "xor bx,bx", "mov bl,byte ptr [cur]", "mov cl,5", "shl bx,cl", "add bx,offset records",
          "mov al,byte ptr [pad_id]", "mov byte ptr [bx],al", "mov byte ptr [pad_id],0", "mov al,byte ptr [pad_fl]", "mov byte ptr [bx+1],al",
          "mov ax,word ptr [pad_ax]", "mov word ptr [bx+2],ax", "mov ax,word ptr [pad_di]", "mov word ptr [bx+4],ax",
          "mov ax,word ptr [pad_bp]", "mov word ptr [bx+6],ax", "mov ax,word ptr [pad_sp]", "mov word ptr [bx+8],ax",
          "mov ax,word ptr [pad_cs]", "mov word ptr [bx+10],ax",
          f"mov ax,{T}", "mov es,ax", "mov ax,word ptr es:[0]", "mov word ptr [bx+14],ax", "mov ax,word ptr es:[2]", "mov word ptr [bx+16],ax",
          "mov ax,word ptr es:[0xfffe]", "mov word ptr [bx+18],ax",
          f"mov ax,{T + 0x1000}", "mov es,ax", "mov ax,word ptr es:[0]", "mov word ptr [bx+20],ax", "mov ax,word ptr es:[2]", "mov word ptr [bx+22],ax",
          "mov al,byte ptr [cur]", "inc al", "mov byte ptr [cur],al"]
    for k in range(len(EDGE_SPS)):
        a += [f"cmp al,{k}", f"jne skip{k}", f"jmp case{k}", f"skip{k}:"]
    a += ["xor ax,ax", "mov es,ax", "mov ax,word ptr [old0d]", "mov word ptr es:[0x34],ax", "mov ax,word ptr [old0d+2]",
          "mov word ptr es:[0x36],ax", "sti", "mov dx,offset outname", "xor cx,cx", "mov ah,0x3c", "int 0x21", "jc fail", "mov bx,ax",
          "mov dx,offset records-2", f"mov cx,{2 + EDGE_REC * len(EDGE_SPS)}", "mov ah,0x40", "int 0x21", "jc fail",
          "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21", "fail:", "mov ax,0x4c01", "int 0x21",
          "fault:", "mov ax,cs", "mov ds,ax", "mov word ptr [pad_sp],sp", "mov byte ptr [pad_id],0xff", "jmp back_here",
          'outname: .asciz "RESULT.BIN"', "save_sp: .word 0", "save_ss: .word 0", "cur: .byte 0", ".byte 0",
          "old0d: .word 0,0", "back: .word 0,0", "body:", f'.incbin "{directory / "body.bin"}"',
          ".word 0", f"records: .space {EDGE_REC * len(EDGE_SPS)}"]
    for org, pid in ((0x1000, 1), (0x1100, 2), (0x2000, 3), (0x2100, 4)):
        a += [f".org {org}", "mov word ptr ds:[pad_ax],ax", "lahf", "mov byte ptr ds:[pad_fl],ah", "mov word ptr ds:[pad_di],di",
              "mov word ptr ds:[pad_bp],bp", "mov word ptr ds:[pad_sp],sp", "mov word ptr ds:[pad_cs],cs",
              f"mov byte ptr ds:[pad_id],{pid}", ".byte 0xff,0x2e", ".word back"]
    a += [".org 0x2200", "pad_ax: .word 0", "pad_fl: .byte 0", "pad_id: .byte 0", "pad_di: .word 0", "pad_bp: .word 0",
          "pad_sp: .word 0", "pad_cs: .word 0"]
    (directory / "edges.S").write_text("\n".join(a) + "\n")
    subprocess.run(["as", "--32", str(directory / "edges.S"), "-o", str(directory / "edges.o")], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(directory / "edges.o"), "-o", str(directory / "EDGES.COM")], check=True)
    (directory / "dosbox.conf").write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                                           "[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nEDGES.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(directory / "dosbox.conf"), "-fastlaunch", "-exit", "-noconsole"],
                       env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 2 + EDGE_REC * len(EDGE_SPS), f"edge output length {len(result)}")
    com = struct.unpack_from("<H", result)[0]
    rows = []
    for k, sp in enumerate(EDGE_SPS):
        r = result[2 + EDGE_REC * k:2 + EDGE_REC * (k + 1)]
        pid, ah, ax, di, bp, sp_out, cs_out = r[0], r[1], *struct.unpack_from("<5H", r, 2)
        stack = struct.unpack_from("<5H", r, 14)
        check(pid in EDGE_PADS, f"native edge SP {sp:04x}: no landing (id {pid:#x}; FFh = INT 0Dh fault)")
        ip_out = EDGE_PADS[pid][1]
        cpu, model_stack = edge_model(span, function, sp, com)
        got = dict(cs=cs_out, ip=ip_out, ax=ax, di=di, bp=bp, sp=sp_out, flags=ah, stack=stack)
        want = dict(cs=cpu.cs, ip=cpu.ip, ax=cpu.ax, di=cpu.di, bp=cpu.bp, sp=cpu.sp, flags=(cpu.flags & 0xD5) | 2, stack=model_stack)
        check(got == want, f"native edge SP {sp:04x}: DOSBox {got} model {want}")
        rows.append(dict(sp=f"{sp:04X}", cs_wrapped=cs_out == com, ip_wrapped=ip_out == 0x1100, parent=f"{0x9000 if di >= 0x9000 else 0x8000:04X}"))
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X stack-offset edges: {len(EDGE_SPS)}/{len(EDGE_SPS)} equal to the model (landing CS:IP, AX, DI, BP, SP, LAHF flags, "
          f"5 stack words); no INT 0Dh; RESULT.BIN {digest}")
    return dict(cases=len(EDGE_SPS), rows=rows, faults=0, result_sha256=digest, com_segment=f"{com:04X}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix="poolrad-skip-") as temp:
            library(Path(temp))
        print("CHECKER_BUILD_OK")
        return
    target, span = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(span), instructions=len(target["starts"]), image_span=target["image_span"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-skip-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, span, function)
            if args.native:
                report["native"] = native_audit(span, function)
                report["native_sp_edges"] = native_sp_edges(span, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
