#!/usr/bin/env python3
"""Original-byte parity for the raw-CPU resident 0709:0000 EGA register reset, the first port-bound routine of the
RES 00BA:16EC closure, and a prototype of the guest-side port harness (docs/re/RES_00BA_16EC_PORT_HARNESS.md).

Unicorn: the original 0709:0000..0070 runs in a 1 MiB image with UC_HOOK_INSN on OUT; the entire RAM, all 14 CPU words
and the ordered OUT events (port, value, size, CS:IP) must equal the C model's, with its port environment recording the
OUTs, with no mask. PORT cases make the environment refuse one OUT; the original then stops before that OUT executes.
DOSBox-X: each case runs twice in the synthetic COM. The plain run gives the full-state record. The traced run sets TF
before the far call, and a guest INT 1 handler logs every OUT the routine reaches as a 32-byte spec event. The C model's
OUT sequence must equal that trace. The emulator stays an external process; no emulator code is involved. Media stays
private.
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

R709, ENTRY, END, BODY_RETF = 0x0709, 0x0000, 0x0071, 0x0070
DS, SS = 0x3000, 0x4000
RET_CS, RET_IP = 0x9001, 0x7000
OK, PORT = 0, 2
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)
OUT_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint16, ctypes.c_uint, ctypes.c_uint16, ctypes.c_uint16)
IN_FN = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint, ctypes.c_uint16, ctypes.c_uint16, ctypes.POINTER(ctypes.c_uint16))


class Env(ctypes.Structure):
    _fields_ = [("ctx", ctypes.c_void_p), ("out", OUT_FN), ("in_", IN_FN)]


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    span = bytes(unpack(exe)[0])[physical(R709, ENTRY):physical(R709, END)]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, ENTRY))
    outs = [i.address for i in insns if i.mnemonic == "out"]
    check(len(insns) == 58 and sum(i.size for i in insns) == len(span) and insns[-1].address == BODY_RETF and insns[-1].mnemonic == "retf"
          and len(outs) == 18 and not any(i.mnemonic in ("call", "lcall", "in", "int") or i.mnemonic.startswith("j") for i in insns),
          "0709:0000 differs")
    sha = sha256(span).hexdigest()
    print(f"0709:0000..0070 (113/58, 18 OUT DX,AL, call-free, RETF) sha {sha}")
    return dict(starts=[i.address for i in insns], outs=outs, sha=sha), span


def library(directory):
    path = directory / "ega.so"
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            str(ROOT / "src/recovered/resident_0709_ega_reset_0000_cpu.c"), "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_r0709_ega_reset_0000_cpu
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env)]
    function.restype = ctypes.c_int
    return function


def run_c(function, memory, cpu, refuse=None):
    """Runs the C model with a recording environment; `refuse` is the index of the OUT the environment refuses."""
    events = []

    def out(ctx, port, value, size, cs, ip):
        del ctx
        if refuse is not None and len(events) == refuse:
            return 1
        events.append((port, value, size, cs, ip))
        return 0

    env = Env(None, OUT_FN(out), IN_FN())
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(env))
    return status, bytes(ram), events


def build(span, rng, *, ss=SS, sp=0x8000, flags=0x7202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(R709, ENTRY); memory[at:at + len(span)] = span
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, R709, ENTRY, DS, rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, memory, cpu, high, target, function, refuse):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_INSN
    from unicorn.x86_const import UC_X86_INS_OUT
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, end = physical(R709, ENTRY), physical(R709, END)
    state = dict(stop=None, returned=False, here=None, illegal=None)
    events, visited = [], set()

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
        if refuse is not None and here in target["outs"] and len(events) == refuse:
            state["stop"] = PORT
            return emulator.emu_stop()
        visited.add(here)
        state["here"] = here
        state["returned"] = here == BODY_RETF
        return None

    def out(emulator, port, size, value, user):
        del emulator, user
        events.append((port, value & (0xFF if size == 1 else 0xFFFF), size, R709, state["here"]))

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    original.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=10000)
    check(state["illegal"] is None and state["stop"] is not None, f"original did not stop cleanly ({state['illegal']}): {label}")
    status, actual, mine = run_c(function, memory, cpu, refuse)
    check(status == state["stop"], f"C status {status}, original {state['stop']}: {label}")
    check(mine == events, f"OUT events differ {label}: C {mine[:3]}... original {events[:3]}...")
    expected = bytes(original.mem_read(0, SIZE))
    if actual != expected:
        differences = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        theirs = original.reg_read(register) & 0xFFFF
        check(getattr(cpu, name) == theirs, f"{name} mismatch {label}: C={getattr(cpu, name):04x}, original={theirs:04x}")
    return state["stop"], visited, len(events)


def differential(target, span, function):
    rng = random.Random(0x07090000)
    spec = [(f"random-{i}", dict(flags=FLAGS[i % len(FLAGS)], sp=rng.randrange(0x0800, 0xF000) & ~1, ss=rng.choice((SS, 0x2000, 0x6000))), None)
            for i in range(60)]
    spec += [("ss-a20", dict(ss=0xFFFF, sp=0x0800), None), ("df-set", dict(flags=0x0603), None),
             ("random-background", dict(background=bytes(random.Random(0x07090000).randbytes(SIZE))), None)]
    spec += [(f"port-refuse-{k}", dict(), k) for k in (0, 5, 17)]
    stops, visited, events = {OK: 0, PORT: 0}, set(), 0
    for label, options, refuse in spec:
        memory, cpu, high = build(span, rng, **options)
        stop, v, n = compare(f"+0000 {label}", memory, cpu, high, target, function, refuse)
        stops[stop] += 1
        visited |= v
        events += n
        if not label.startswith("random"):
            print(f"  {label}: {'return' if stop == OK else 'PORT'} after {n} OUTs")
    print(f"0709:0000: {sum(stops.values())} original-byte cases ({stops[OK]} return, {stops[PORT]} PORT); full 1 MiB RAM, all 14 CPU words "
          f"and {events} ordered OUT events equal, no mask; {len(visited)}/{len(target['starts'])} instruction starts")
    check(len(visited) == len(target["starts"]) and stops[OK] and stops[PORT], "coverage or stop kinds incomplete")
    return dict(cases=sum(stops.values()), returns=stops[OK], port_stops=stops[PORT], out_events=events, starts=len(visited))


# ---- DOSBox-X: plain run plus single-step traced run ------------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, TRACE_AT = 0x0300, 0x2600, 0x8000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, MAX_EVENTS, EVENT = 32, 32, 32
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
TRACE_BYTES = 2 + MAX_EVENTS * EVENT
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def case_setup(traced):
    """Guest code that resets the window and data, then calls the routine with the case's flags (TF added if traced)."""
    t = ["    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov ax,word ptr [index]", f"    mov bx,{len(NATIVE_FLAGS)}", "    xor dx,dx", "    div bx", "    shl dx,1", "    mov si,dx",
         "    mov ax,word ptr [flags_table+si]"]
    if traced:
        t += ["    or ax,0x100", "    push cs", "    pop es", f"    mov di,{TRACE_AT:#x}", f"    mov cx,{TRACE_BYTES // 2}", "    push ax",
              "    xor ax,ax", "    rep stosw", "    pop ax", f"    mov word ptr [trace_ptr],{TRACE_AT + 2:#x}"]
    t += ["    push ax", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]"]
    return t


def native_audit(span, function):
    import os
    directory = ROOT / "resources/oracle-runs/res0709-ega-reset-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    save = ["returned{n}:"]
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    xor ax,ax", "    mov es,ax", "    mov ax,word ptr es:[4]", "    mov word ptr [old_int1],ax", "    mov ax,word ptr es:[6]",
         "    mov word ptr [old_int1+2],ax", "    cli", "    mov word ptr es:[4],offset int1", "    mov word ptr es:[6],cs", "    sti",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned_plain",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],{MAX_EVENTS}",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:"]
    t += case_setup(False) + ["returned_plain:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned_plain", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds"]
    t += case_setup(True) + ["returned_traced:", "    pushf", "    pop ax", "    and ax,0xfeff", "    push ax", "    popf", "    sti",
                             "    push cs", "    pop ds", "    mov ax,word ptr [trace_count]", f"    mov word ptr [{TRACE_AT:#x}],ax",
                             "    mov word ptr [trace_count],0",
                             "    mov bx,word ptr [handle]", f"    mov dx,{TRACE_AT:#x}", f"    mov cx,{TRACE_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
                             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{NATIVE_CASES}", "    jb again",
                             "    xor ax,ax", "    mov es,ax", "    cli", "    mov ax,word ptr [old_int1]", "    mov word ptr es:[4],ax",
                             "    mov ax,word ptr [old_int1+2]", "    mov word ptr es:[6],ax", "    sti",
                             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
                             "failure:", "    mov ax,0x4c01", "    int 0x21"]
    # INT 1: log OUT DX,AL / OUT DX,AX / OUT imm8,AL / OUT imm8,AX reached in the traced segment, as 32-byte spec events
    t += ["int1:", "    push bp", "    mov bp,sp", "    push ax", "    push bx", "    push si", "    push di", "    push ds", "    push es",
          "    mov ax,word ptr [bp+4]", f"    cmp ax,{CALLER_N:#x}", "    jne int1_done",
          "    mov si,word ptr [bp+2]", "    mov es,ax", "    mov bl,byte ptr es:[si]",
          "    cmp bl,0xee", "    je int1_dx8", "    cmp bl,0xef", "    je int1_dx16", "    cmp bl,0xe6", "    je int1_im8", "    cmp bl,0xe7", "    je int1_im16",
          "    jmp int1_done",
          "int1_dx8:", "    mov si,dx", "    mov bx,word ptr [bp-2]", "    xor bh,bh", "    mov ax,0x0101", "    jmp int1_record",
          "int1_dx16:", "    mov si,dx", "    mov bx,word ptr [bp-2]", "    mov ax,0x0202", "    jmp int1_record",
          "int1_im8:", "    mov al,byte ptr es:[si+1]", "    xor ah,ah", "    mov si,ax", "    mov bx,word ptr [bp-2]", "    xor bh,bh", "    mov ax,0x0101",
          "    jmp int1_record",
          "int1_im16:", "    mov al,byte ptr es:[si+1]", "    xor ah,ah", "    mov si,ax", "    mov bx,word ptr [bp-2]", "    mov ax,0x0202",
          "int1_record:", "    push cs", "    pop ds", f"    cmp word ptr [trace_count],{MAX_EVENTS}", "    jae int1_done",
          "    mov di,word ptr [trace_ptr]", "    push ax", "    mov ax,word ptr [trace_count]", "    mov word ptr [di],ax", "    pop ax",
          "    mov byte ptr [di+2],al", "    mov byte ptr [di+3],ah", "    mov ax,word ptr [bp+2]", "    mov word ptr [di+4],ax",
          "    mov ax,word ptr [bp+4]", "    mov word ptr [di+6],ax", "    mov word ptr [di+8],si", "    mov word ptr [di+10],bx",
          f"    add word ptr [trace_ptr],{EVENT}", "    inc word ptr [trace_count]",
          "int1_done:", "    pop es", "    pop ds", "    pop di", "    pop si", "    pop bx", "    pop ax", "    pop bp", "    iret",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "trace_ptr: .word 0", "trace_count: .word 0",
          "old_int1: .word 0, 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    t += [f".org {WRAP_AT - 0x100:#x}", "wrap:"]
    for i in range(0, len(span), 16):
        t.append("    .byte " + ",".join(f"{x:#x}" for x in span[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    check(len((directory / "ORACLE.COM").read_bytes()) + 0x100 <= TRACE_AT, "COM image reaches the trace buffer")
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
    check(len(result) == 8 + NATIVE_CASES * (RECORD_BYTES + TRACE_BYTES), f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == NATIVE_CASES and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    traced_events = 0
    for n in range(NATIVE_CASES):
        base = 8 + n * (RECORD_BYTES + TRACE_BYTES)
        record, trace = result[base:base + RECORD_BYTES], result[base + RECORD_BYTES:base + RECORD_BYTES + TRACE_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, ENTRY); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = b"\xa5" * 0x10000
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[n % len(NATIVE_FLAGS)])
        status, ram, mine = run_c(function, memory, cpu)
        check(status == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE] + ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000]
        check(got == record[28:], f"native case {n} stack window or data segment differs")
        events = struct.unpack_from("<H", trace)[0]
        logged = []
        for k in range(events):
            seq, kind, size, ip, cs, port, value = struct.unpack_from("<HBBHHHH", trace, 2 + k * EVENT)
            check(seq == k and kind == size and trace[2 + k * EVENT + 12:2 + (k + 1) * EVENT] == bytes(20), f"native case {n} event {k} malformed")
            logged.append((port, value, size, cs, ip))
        check(logged == mine, f"native case {n} OUT trace differs: DOS {logged[:3]}... C {mine[:3]}...")
        traced_events += events
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; plain run: all 14 CPU words, the stack window and the whole "
          f"64 KiB data segment, no mask; traced run: {traced_events} single-step OUT events equal the C model's port environment")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; routine at {CALLER_N:04X}:{ENTRY:04X}")
    return dict(cases=NATIVE_CASES, traced_out_events=traced_events, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, span = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(span), instructions=len(target["starts"]), outs=len(target["outs"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-ega-") as temp:
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
