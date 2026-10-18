#!/usr/bin/env python3
"""Original-byte full-state parity for two pure callees of 0709:0768 in the RES 00BA:16EC closure: the raw-CPU SYSTEM
set expansion 0AF8:0D12 and the raw-CPU resident 0899:0104.

The differential loads each original START.EXE span at its own address in a 1 MiB image and compares the entire RAM and
all 14 CPU words between the original under Unicorn and the C model, with no mask. The DOSBox-X synthetic oracle repeats
each routine natively (--routine). Media stays private.
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

ROUTINES = {"0d12": dict(seg=0x0AF8, entry=0x0D12, end=0x0D3C, retf=0x0D39, insns=18, symbol="pr_system_set_expand_0d12",
                         source="system_set_expand_0d12_cpu.c", reach=None),
            "0104": dict(seg=0x0899, entry=0x0104, end=0x0122, retf=0x0121, insns=7, symbol="pr_r0899_idle_0104_cpu",
                         source="resident_0899_idle_0104_cpu.c", reach=(0x0104, 0x0105, 0x0107, 0x010A, 0x011E, 0x0120, 0x0121))}
DS, SS, SRC, DST = 0x3000, 0x4000, 0x5000, 0x6000
RET_CS, RET_IP = 0x9001, 0x7000
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source(name):
    r = ROUTINES[name]
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    span = bytes(unpack(exe)[0])[physical(r["seg"], r["entry"]):physical(r["seg"], r["end"])]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    if r["reach"] is None:
        insns = list(md.disasm(span, r["entry"]))
        check(len(insns) == r["insns"] and sum(i.size for i in insns) == len(span) and (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "6"),
              "SYSTEM 0D12 differs")
        starts = [i.address for i in insns]
    else:
        by = {i.address: i for i in md.disasm(span, r["entry"])}
        check(all(a in by for a in r["reach"]) and (by[0x010A].mnemonic, int(by[0x010A].op_str, 16)) == ("jmp", 0x011E)
              and by[0x0121].mnemonic == "retf" and by[0x0107].mnemonic == "sub", "0899:0104 differs")
        starts = list(r["reach"])
    sha = sha256(span).hexdigest()
    print(f"{r['seg']:04X}:{r['entry']:04X}..{r['end'] - 1:04X} ({len(span)} bytes, {len(starts)} reachable instructions) sha {sha}")
    return dict(starts=starts, sha=sha, bytes=len(span)), span


def library(directory, name):
    r = ROUTINES[name]
    path = directory / f"{name}.so"
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            str(ROOT / "src/recovered" / r["source"]), "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = getattr(ctypes.CDLL(str(path)), r["symbol"])
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def build(name, span, rng, *, count=0, src=(SRC, 0x0100), dst=(DST, 0x0200), data=b"", ss=SS, sp=0x8000, flags=0x7202, background=None):
    r = ROUTINES[name]
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    at = physical(r["seg"], r["entry"]); memory[at:at + len(span)] = span
    for i, byte in enumerate(data):
        memory[physical(src[0], (src[1] + i) & 0xFFFF)] = byte
    words = (RET_IP, RET_CS, count, src[1], src[0], dst[1], dst[0]) if name == "0d12" else (RET_IP, RET_CS)
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    cpu = Cpu(*(rng.randrange(0x10000) for _ in range(7)), sp, r["seg"], r["entry"], DS, rng.randrange(0x10000), ss, flags)
    return memory, cpu, ss >= 0xF000


def compare(name, label, memory, cpu, high, function):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    r = ROUTINES[name]
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (reg, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, reg))
    start, end = physical(r["seg"], r["entry"]), physical(r["seg"], r["end"])
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
        here = linear - start + r["entry"]
        visited.add(here)
        state["returned"] = here == r["retf"]
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=10000)
    check(state["illegal"] is None and state["stopped"], f"original did not stop cleanly ({state['illegal']}): {label}")
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    expected, actual = bytes(original.mem_read(0, SIZE)), bytes(ram)
    if actual != expected:
        differences = [i for i, (x, y) in enumerate(zip(actual, expected)) if x != y]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (reg, _), register in zip(Cpu._fields_, REGISTERS):
        theirs = original.reg_read(register) & 0xFFFF
        check(getattr(cpu, reg) == theirs, f"{reg} mismatch {label}: C={getattr(cpu, reg):04x}, original={theirs:04x}")
    return visited


def differential(name, target, span, function):
    rng = random.Random(int("0768" + name, 16))
    spec = []
    if name == "0d12":
        for i in range(140):
            bl = rng.choice((0, 1, 4, 16, 32, rng.randrange(33), rng.randrange(256)))
            bh = rng.choice((0, 1, 8, 32 - min(bl, 32), rng.randrange(33), rng.randrange(256)))
            spec.append((f"random-{i}", dict(count=(bh << 8) | bl, data=bytes(rng.randrange(256) for _ in range(bl)),
                                             src=(SRC, rng.choice((0x0100, 0xFFF8, rng.randrange(0x10000)))),
                                             dst=(DST, rng.choice((0x0200, 0xFFF0, rng.randrange(0x10000)))),
                                             sp=rng.randrange(0x0800, 0xF000) & ~1, flags=FLAGS[i % len(FLAGS)])))
        spec += [("wrap-255", dict(count=0x2111, data=bytes(range(0x11)))),
                 ("overlap", dict(count=0x0410, data=bytes(range(0x10)), src=(SRC, 0x0100), dst=(SRC, 0x00F8))),
                 ("all-zero", dict(count=0x2000)), ("df-set", dict(count=0x0204, data=b"\x01\x02\x03\x04", flags=0x0603)),
                 ("ss-a20", dict(count=0x0302, data=b"\x55\xAA", ss=0xFFFF, sp=0x0800)),
                 ("random-background", dict(count=0x0108, data=bytes(range(8)), background=bytes(random.Random(0x07680D12).randbytes(SIZE))))]
    else:
        for i in range(80):
            spec.append((f"random-{i}", dict(sp=rng.randrange(0x0800, 0xF000) & ~1, ss=rng.choice((SS, 0x2000, 0x7000)), flags=FLAGS[i % len(FLAGS)])))
        spec += [("sp-two", dict(sp=0x0002)), ("sp-zero", dict(sp=0x0000)), ("ss-a20", dict(ss=0xFFFF, sp=0x0800)),
                 ("random-background", dict(background=bytes(random.Random(0x08990104).randbytes(SIZE))))]
    visited = set()
    for label, options in spec:
        memory, cpu, high = build(name, span, rng, **options)
        visited |= compare(name, f"{name} {label}", memory, cpu, high, function)
    print(f"{name}: {len(spec)} original-byte cases (all return); full 1 MiB RAM and all 14 CPU words equal, no mask; "
          f"{len(visited)}/{len(target['starts'])} instruction starts")
    check(set(target["starts"]) <= visited, "coverage incomplete")
    return dict(cases=len(spec), starts=len(visited))


# ---- DOSBox-X synthetic oracle ---------------------------------------------------------------------------
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, WRAP_AT, CASES_AT = 0x0300, 0x2600, 0x2700
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 32, 0x100
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_audit(name, span, function):
    import os
    r = ROUTINES[name]
    directory = ROOT / f"resources/oracle-runs/res0768-{name}-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(int("0768" + name + "d", 16))
    cases = []
    for n in range(NATIVE_CASES):
        data = bytearray([0xA5]) * 0x10000
        bl = rng.choice((0, 2, 8, 16, 32, rng.randrange(33))) if name == "0d12" else 0
        bh = rng.choice((0, 1, 4, 32 - bl, rng.randrange(33 - bl))) if name == "0d12" else 0
        src, dst = 0x1000 + 2 * rng.randrange(0x100), 0x4000 + 2 * rng.randrange(0x100)
        data[src:src + bl] = bytes(rng.randrange(256) for _ in range(bl))
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(10 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=n % len(NATIVE_FLAGS), data=data, writes=writes, count=(bh << 8) | bl, src=src, dst=dst))
    table = b"".join((struct.pack("<BBHHHH", c["flags"], 0, len(c["writes"]), c["count"], c["src"], c["dst"])
                      + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"])).ljust(CASE_SIZE, b"\0") for c in cases)
    check(CASES_AT + len(table) <= OUTPUT, "COM layout overlaps")
    push_args = [f"    mov ax,{DATA_N:#x}", "    push ax", "    push word ptr [bx+8]", "    push ax", "    push word ptr [bx+6]", "    push word ptr [bx+4]"] \
        if name == "0d12" else []
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset wrap", f"    mov di,{r['entry']:#x}", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{NATIVE_CASES}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+10]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:"] + push_args + [
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
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
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {r['entry']:#x}, {CALLER_N:#x}",
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
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, r["entry"]); memory[at:at + len(span)] = span
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = case["data"]
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        args = (DATA_N, case["dst"], DATA_N, case["src"], case["count"]) if name == "0d12" else ()
        for value in (*args, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, r["entry"], DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
    digest = sha256(result).hexdigest()
    print(f"{name} DOSBox-X normal-core/386 synthetic cases passed: {NATIVE_CASES}; all 14 CPU words, the stack window and the whole 64 KiB "
          f"data segment; no mask; result SHA-256 {digest}")
    return dict(cases=NATIVE_CASES, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--routine", choices=sorted(ROUTINES), required=True)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, span = source(args.routine)
    report = dict(routine=args.routine, target=dict(sha256=target["sha"], bytes=target["bytes"], instructions=len(target["starts"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-0768-") as temp:
            function = library(Path(temp), args.routine)
            if args.unicorn:
                report["unicorn"] = differential(args.routine, target, span, function)
            if args.native:
                report["native"] = native_audit(args.routine, span, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
