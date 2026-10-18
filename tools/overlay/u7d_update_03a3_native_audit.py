#!/usr/bin/env python3
"""DOSBox-X synthetic oracle for the existing complete raw-CPU U7D +03A3 model (pr_u7d_update_03a3, PR #606), which
composes the strict U7D +02F2 carry model.

docs/re/U7D_UPDATE_03A3.md records an original-byte Unicorn full-state differential but no DOSBox-X run. This oracle
loads the hash-checked unit 0x7D into a DOS COM, rebuilds each case's thresholds (DS:363A), seven-word array
(DS:6DC2) and non-cyclic party list (head DS:5D96, links +104/+106, counters +30) in one data segment, calls +03A3 from
a single call site with IRQs masked, and compares all 14 CPU words, the stack window and the whole data segment with
the C model, with no mask. Media stays private.
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

from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

BODY_SHA = "3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad"
CALLEE_SHA = "8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9"
ENTRY = 0x03A3
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x5800
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x80, 0x80
CASE_SIZE, NATIVE_CASES, MAX_WRITES = 144, 128, 34
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "media provenance differs")
    u = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x7D)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    check(sha256(code[0x03A3:0x03D5]).hexdigest() == BODY_SHA and sha256(code[0x02F2:0x03A3]).hexdigest() == CALLEE_SHA,
          "U7D +03A3 or +02F2 bytes differ")
    print(f"U7D +03A3 (50 B, SHA-256 {BODY_SHA}) and +02F2 (177 B, {CALLEE_SHA}) from GAME file {u['ovr_offset']:#x}")
    return code


def library(directory):
    path = directory / "u7d03a3.so"
    sources = ["overlay_u7d_update_03a3.c", "overlay_u7d_carry_02f2.c"]
    built = subprocess.run(["cc", "-std=c11", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = ctypes.CDLL(str(path)).pr_u7d_update_03a3
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def native_cases(rng):
    cases = []
    for n in range(NATIVE_CASES):
        if n < 64:
            limits = [i + 3 for i in range(7)]
            values = [limits[i] if n & (1 << (i % 6)) else rng.randrange(limits[i]) for i in range(7)]
            values[5] = rng.choice((0, 1, 2, values[5]))
        else:
            limits = [rng.choice((rng.randrange(1, 40), rng.randrange(65536))) for _ in range(7)]
            values = [rng.randrange(65536) for _ in range(7)]
        count = rng.randrange(7)
        nodes = [0x8000 + 0x200 * i + rng.randrange(0, 0x40, 2) for i in range(count)]
        writes = [(0x363A + 2 * i, v) for i, v in enumerate(limits)] + [(0x6DC2 + 2 * i, v) for i, v in enumerate(values)]
        writes += [(0x5D96, nodes[0] if nodes else 0), (0x5D98, DATA_N if nodes else 0)]
        for i, node in enumerate(nodes):
            nxt = nodes[i + 1] if i + 1 < len(nodes) else None
            writes += [(node + 0x30, 0xFFFF if i == 0 else 0x7FFF + i), (node + 0x104, nxt or 0), (node + 0x106, DATA_N if nxt else 0)]
        check(len(writes) <= MAX_WRITES, "too many writes")
        cases.append(dict(flags=n % len(NATIVE_FLAGS), writes=writes))
    return cases


def case_bytes(case):
    data = struct.pack("<BBH", case["flags"], 0, len(case["writes"])) + b"".join(struct.pack("<HH", o, v) for o, v in case["writes"])
    check(len(data) <= CASE_SIZE, "case too large")
    return data + bytes(CASE_SIZE - len(data))


def native_audit(code, function):
    directory = ROOT / "resources/oracle-runs/u7d-03a3-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x7D03A3)
    cases = native_cases(rng)
    table = b"".join(case_bytes(c) for c in cases)
    check(CASES_AT + len(table) <= CODE_AT and CODE_AT + len(code) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", "    xor di,di", f"    mov cx,{len(code):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    lea si,[bx+4]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop",
         "    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
         "    push word ptr [flags_table+si]", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
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
    for label, data, where in (("cases", table, CASES_AT), ("unit", code, CODE_AT)):
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
    capped = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(CALLER_N, 0); memory[at:at + len(code)] = code
        at = physical(DATA_N, 0); memory[at:at + 0x10000] = bytes([0xA5]) * 0x10000
        for offset, value in case["writes"]:
            put16(memory, DATA_N, offset, value)
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = function(ram, SIZE, ctypes.byref(cpu))
        check(status == 0, f"C status {status} on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (a, b) in enumerate(zip(got, record[28:])) if a != b]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        word4 = struct.unpack_from("<H", record, 28 + WINDOW_SIZE + 0x6DCA)[0]
        capped += word4 == 0x63
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (64 carry-mask, 64 random; party lists of 0..6 nodes; "
          f"{capped} ending with word 4 at the 63h cap); all 14 CPU words, the {WINDOW_SIZE:#x}-byte stack window and the whole "
          f"64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; U7D at {CALLER_N:04X}")
    return dict(cases=len(cases), capped=capped, result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    code = source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u7d03a3-") as temp:
        report = dict(native=native_audit(code, library(Path(temp))))
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
