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
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(span).hexdigest()
    print(f"0709:0714..074D (58/19, call-free, RETF 2) sha {sha}")
    return dict(starts=[i.address for i in insns], conditional=conditional, sha=sha), span


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
    frame = physical(ss, sp)
    for i, value in enumerate((RET_IP, RET_CS, p)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, R709, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(label, memory, cpu, high, target, function, cap):
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
    visited, edges, stops, most = set(), set(), {OK: 0, LIMIT: 0}, 0
    for label, options, cap in spec:
        memory, cpu, high = build(span, rng, **options)
        stop, v, e, n = compare(f"+0714 {label}", memory, cpu, high, target, function, cap)
        stops[stop] += 1; visited |= v; edges |= e; most = max(most, n)
        if not label.startswith("random"):
            print(f"  {label}: {'return' if stop == OK else 'LIMIT'} after {n} loop-backs")
    reachable = {(s, o) for s, outs in target["conditional"].items() for o in outs}
    print(f"0709:0714: {len(spec)} original-byte cases ({stops[OK]} return, {stops[LIMIT]} LIMIT); full 1 MiB RAM and all 14 CPU words equal, "
          f"no mask; {len(visited)}/{len(target['starts'])} instruction starts, {len(edges & reachable)}/{len(reachable)} conditional outcomes; "
          f"up to {most} loop-backs")
    check(len(visited) == len(target["starts"]) and reachable <= edges and stops[LIMIT], "coverage or stop kinds incomplete")
    return dict(cases=len(spec), returns=stops[OK], limits=stops[LIMIT], starts=len(visited), outcomes=len(edges & reachable))


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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, span = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(span), instructions=len(target["starts"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-skip-") as temp:
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
