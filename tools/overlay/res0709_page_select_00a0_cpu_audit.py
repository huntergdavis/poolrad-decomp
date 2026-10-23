#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU resident 0709:00A0 EGA page select (RES 00BA:16EC closure).

The differential loads the original START.EXE span 0709:00A0..00B6 at its own address in a 1 MiB image and compares the
entire RAM and all 14 CPU words between the original under Unicorn and the C model, with no mask, including stores to
DS:[4A18h] that alias the return frame, the pushed BP or the argument. The DOSBox-X synthetic oracle repeats random pages
natively. Media stays private.
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

CHECKER_ROWS = ('RES_0709_00A0',)

R709, ENTRY, END, BODY_RETF = 0x0709, 0x00A0, 0x00B7, 0x00B4
DS, SS = 0x3000, 0x4000
RET_CS, RET_IP = 0x9001, 0x7000
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    span = bytes(unpack(exe)[0])[physical(R709, ENTRY):physical(R709, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    check(len(insns) == 10 and sum(i.size for i in insns) == len(span) and (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "2")
          and not any(i.mnemonic in ("call", "lcall", "in", "out", "int") or i.mnemonic.startswith("j") for i in insns), "0709:00A0 differs")
    sha = sha256(span).hexdigest()
    print(f"0709:00A0..00B6 (23/10, call-free, RETF 2) sha {sha}")
    return dict(starts=[i.address for i in insns], sha=sha), span


def library(directory):
    path = directory / "page.so"
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            str(ROOT / "src/recovered/resident_0709_page_select_00a0_cpu.c"), "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_r0709_page_select_00a0_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def build(span, rng, *, page, ds=DS, ss=SS, sp=0x8000, flags=0x7202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(R709, ENTRY); memory[at:at + len(span)] = span
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, page)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, R709, ENTRY, ds, rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, memory, cpu, high, function):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, end = physical(R709, ENTRY), physical(R709, END)
    state = dict(stopped=False, returned=False, illegal=None)
    visited = set()

    def hook(emulator, address, size, user):
        del size, user
        if state["returned"]:
            state["stopped"] = True
            return emulator.emu_stop()
        linear = address & (2 * SIZE - 1)
        if not start <= linear < end:
            state["illegal"] = linear
            return emulator.emu_stop()
        here = linear - start + ENTRY
        visited.add(here)
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
    original.emu_start(start, 2 * SIZE, count=1000)
    check(state["illegal"] is None and state["stopped"], f"original did not stop cleanly ({state['illegal']}): {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    expected, actual = bytes(original.mem_read(0, SIZE)), bytes(ram)
    if actual != expected:
        differences = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        theirs = original.reg_read(register) & 0xFFFF
        check(getattr(cpu, name) == theirs, f"{name} mismatch {label}: C={getattr(cpu, name):04x}, original={theirs:04x}")
    return visited


def differential(target, span, function):
    rng = random.Random(0x070900A0)
    pages = (0, 1, 2, 3, 0x7F, 0x80, 0xFF)
    spec = [(f"random-{i}", dict(page=(rng.randrange(256) << 8) | rng.choice((*pages, rng.randrange(256))), ds=rng.choice((DS, 0x1000, 0x6000)),
                                 ss=rng.choice((SS, 0x2000, 0x7000)), sp=rng.randrange(0x0800, 0xF000) & ~1, flags=FLAGS[i % len(FLAGS)]))
            for i in range(120)]
    spec += [("alias-return", dict(page=2, ds=0x4000, ss=0x4000, sp=0x4A18)), ("alias-bp", dict(page=3, ds=0x4000, ss=0x4000, sp=0x4A1A)),
             ("alias-arg", dict(page=1, ds=0x4000, ss=0x4000, sp=0x4A14)), ("ss-a20", dict(page=1, ss=0xFFFF, sp=0x0800)),
             ("df-set", dict(page=0x7F, flags=0x0603)), ("random-background", dict(page=5, background=bytes(random.Random(0x070900A0).randbytes(SIZE))))]
    visited = set()
    for label, options in spec:
        memory, cpu, high = build(span, rng, **options)
        visited |= compare(f"+00A0 {label}", memory, cpu, high, function)
    print(f"0709:00A0: {len(spec)} original-byte cases (all return); full 1 MiB RAM and all 14 CPU words equal, no mask; "
          f"{len(visited)}/{len(target['starts'])} instruction starts; return-frame, BP and argument aliases included")
    check(len(visited) == len(target["starts"]), "coverage incomplete")
    return dict(cases=len(spec), starts=len(visited))


# ---- DOSBox-X synthetic oracle ---------------------------------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES = 32
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_audit(span, function):
    import os
    directory = ROOT / "resources/oracle-runs/res0709-page-select-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x070900A0D)
    pages = [(rng.randrange(256) << 8) | rng.choice((0, 1, 2, 3, 0x7F, 0x80, 0xFF, rng.randrange(256))) for _ in range(NATIVE_CASES)]
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov bx,word ptr [index]", "    shl bx,1", "    push word ptr [pages+bx]",
         "    mov ax,word ptr [index]", f"    mov bx,{len(NATIVE_FLAGS)}", "    xor dx,dx", "    div bx", "    shl dx,1", "    mov si,dx",
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
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{NATIVE_CASES}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS), "pages: .word " + ",".join(f"{p:#x}" for p in pages)]
    t += [f".org {WRAP_AT - 0x100:#x}", "wrap:"]
    for i in range(0, len(span), 16):
        t.append("    .byte " + ",".join(f"{x:#x}" for x in span[i:i + 16]))
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
    for n, page in enumerate(pages):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = b"\xa5" * 0x10000
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (page, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[n % len(NATIVE_FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        check(got == record[28:], f"native case {n} stack window or data segment differs")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; all 14 CPU words, the stack window and the whole 64 KiB data "
          f"segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; routine at {CALLER_N:04X}:{ENTRY:04X}")
    return dict(cases=NATIVE_CASES, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true")
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.compile_only:
        with tempfile.TemporaryDirectory(prefix="poolrad-page-") as temp:
            library(Path(temp))
        print("CHECKER_BUILD_OK")
        return
    target, span = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(span), instructions=len(target["starts"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-page-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, span, function)
            if args.native:
                report["native"] = native_audit(span, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
