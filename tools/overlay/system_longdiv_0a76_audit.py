#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU SYSTEM 0AF8:0A76 LongInt divide, plus a DOSBox-X oracle.

The routine's bytes come from the hash-checked expanded START.EXE image. The Unicorn differential places the
whole image at load segment 0, runs the original at 0AF8:0A76 and stops at the first fetch outside the routine:
its return, or the runtime-error target 0AF8:00E2 for a zero divisor. It compares the entire 1 MiB RAM and all
14 CPU words with the C model, with no mask. The DOSBox-X oracle runs the original bytes at 9000:0A76. Media
stays private.
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

SYSTEM_CS, ENTRY, END = 0x0AF8, 0x0A76, 0x0AE4
SS, EDGE_SS = 0x2800, 0x1C00
RET_CS, RET_IP = 0x9001, 0x7000
RETURNED, DIVIDE_ERROR = 0, 2
EDGES = (0, 1, 2, 3, 7, 10, 0xFFFF, 0x10000, 0x12345678, 0x7FFFFFFF, 0x80000000, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFFFFF9,
         0xFFFF0000, 0xEDCBA988)
FLAG_SET = (0x7202, 0x0202, 0x0ED7, 0x0002, 0x0646, 0x3A93)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    body = image[SYSTEM_CS * 16 + ENTRY:SYSTEM_CS * 16 + END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(sum(i.size for i in decoded) == len(body) == 110 and decoded[-1].mnemonic == "jmp" and decoded[-1].op_str == "0xe2"
          and any(i.mnemonic == "retf" for i in decoded) and len(conditional) == 8 and exe.count(body) == 1, "SYSTEM 0A76 extent differs")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest(), file_offset=exe.find(body))
    print(f"SYSTEM 0AF8:0A76 LongInt divide ({len(body)}/{len(decoded)}, RETF; zero divisor JMP 00E2) file {target['file_offset']:#x} "
          f"sha {target['sha']}")
    return target, image


def library(directory):
    path = directory / "longdiv.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                    str(ROOT / "src/recovered/system_longdiv_0a76_cpu.c"), "-o", str(path)], check=True, capture_output=True, text=True)
    function = ctypes.CDLL(str(path)).pr_system_longdiv_0a76
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    return function


def build(image, rng, dividend, divisor, *, sp=0x8000, ss=SS, flags=0x7202, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    frame = physical(ss, sp)
    for i, value in enumerate([RET_IP, RET_CS]):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        put16(memory, 0, (frame + 2 * i) & (SIZE - 1), value)
    regs = [dividend & 0xFFFF, divisor >> 16, divisor & 0xFFFF, dividend >> 16] + [rng.randrange(0x10000) for _ in range(3)]
    return memory, Cpu(*regs, sp, SYSTEM_CS, ENTRY, rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)


def compare(label, memory, cpu, target, function, high=False, budget=5000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start, end = physical(SYSTEM_CS, ENTRY), physical(SYSTEM_CS, END)
    state = dict(stop=None, previous=None)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        if start <= address < end:
            here = address - start + ENTRY
            visited.add(here)
            if state["previous"] in target["conditional"] and state["previous"] != here:
                edges.add((state["previous"], here))
            state["previous"] = here
            return
        state["stop"] = DIVIDE_ERROR if address == physical(SYSTEM_CS, 0x00E2) else RETURNED
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
    original.emu_start(start, 2 * SIZE, count=budget)
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu))
    check(status == state["stop"], f"C status {status}, original {state['stop']}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x}")
    return state["stop"], visited, edges


def cases(rng):
    spec = []
    for a in EDGES:
        for b in EDGES:
            spec.append((f"edge-{a:08x}-{b:08x}", dict(dividend=a, divisor=b, flags=FLAG_SET[(a ^ b) % 6])))
    for i in range(500):
        a = rng.choice((rng.randrange(1 << 32), rng.randrange(1 << 16), (1 << 32) - rng.randrange(1, 1 << 16)))
        b = rng.choice((rng.randrange(1, 1 << 32), rng.randrange(1, 1 << 16), (1 << 32) - rng.randrange(1, 1 << 16), rng.randrange(1, 64)))
        spec.append((f"random-{i}", dict(dividend=a, divisor=b, flags=FLAG_SET[i % 6], sp=rng.randrange(0x0100, 0xFF00) & ~1)))
    for sp in (0x0006, 0x0008, 0xFFFC, 0xFFFE):
        spec.append((f"sp-{sp:04x}", dict(dividend=0x87654321, divisor=0xFFFFFF00, sp=sp, ss=EDGE_SS)))
    spec.append(("ss-a20", dict(dividend=12345678, divisor=0xFFFFFF85, sp=0x0400, ss=0xFFFF, high=True)))
    spec.append(("random-background", dict(dividend=0xDEADBEEF, divisor=0x1234, background=bytes(random.Random(0x0A76).randbytes(SIZE)))))
    return spec


def differential(target, image, function):
    rng = random.Random(0x0A760A76)
    visited, edges, stops = set(), set(), {RETURNED: 0, DIVIDE_ERROR: 0}
    for label, options in cases(rng):
        high = options.pop("high", False)
        memory, cpu = build(image, rng, options.pop("dividend"), options.pop("divisor"), **options)
        stop, v, e = compare(f"0A76 {label}", memory, cpu, target, function, high)
        stops[stop] += 1
        visited |= v
        edges |= e
    missing = sorted(hex(i.address) for i in target["decoded"] if i.address not in visited)
    missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
    check(not missing and not missing_edges, f"coverage incomplete: {missing} {missing_edges}")
    print(f"0A76: {stops[RETURNED]} returning and {stops[DIVIDE_ERROR]} divide-error original-byte cases passed, full 1 MiB RAM and all 14 "
          f"CPU words equal; {len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} outcomes")
    return dict(returning=stops[RETURNED], divide_errors=stops[DIVIDE_ERROR], starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
BODY_N = 0x9000
MAIN_AT, BLOB_AT, CASES_AT = 0x0300, 0x0F00, 0x1000
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFE0, 0x20
CASE_SIZE = 10
RECORD_BYTES = 28 + FRAME_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x0A76D05)
    rows = [(a, b) for a in EDGES for b in EDGES if b]
    rows += [(rng.randrange(1 << 32), rng.choice((rng.randrange(1, 1 << 32), rng.randrange(1, 1 << 16), (1 << 32) - rng.randrange(1, 1 << 16))))
             for _ in range(200)]
    return [dict(dividend=a, divisor=b, flags=n % len(NATIVE_FLAGS)) for n, (a, b) in enumerate(rows)]


def native_audit(target, function):
    directory = ROOT / "resources/oracle-runs/system-longdiv-0a76-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
            f"    mov ax,{BODY_N:#x}", "    mov es,ax", "    mov si,offset body", f"    mov di,{ENTRY:#x}", f"    mov cx,{len(target['body'])}", "    rep movsb",
            "    mov ax,cs", "    mov es,ax",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    mov ax,cs", "    mov ds,ax", "    mov es,ax",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
            "    mov bl,byte ptr [si+8]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
            "    mov ax,word ptr [si]", "    mov dx,word ptr [si+2]", "    mov cx,word ptr [si+4]", "    mov bx,word ptr [si+6]"]
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
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {BODY_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS),
             f".org {BLOB_AT - 0x100:#x}", "body:", "    .byte " + ",".join(f"{b:#x}" for b in target["body"]),
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = struct.pack("<HHHHBB", case["dividend"] & 0xFFFF, case["dividend"] >> 16, case["divisor"] & 0xFFFF, case["divisor"] >> 16,
                          case["flags"], 0)
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    check(CASES_AT + CASE_SIZE * len(cases) <= OUTPUT, "COM layout overlaps")
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
        at = physical(BODY_N, ENTRY)
        memory[at:at + len(target["body"])] = target["body"]
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        a, b = case["dividend"], case["divisor"]
        cpu = Cpu(a & 0xFFFF, b >> 16, b & 0xFFFF, a >> 16, *ENTRY_REGS.values(), sp, BODY_N, ENTRY, segment, segment, segment,
                  NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        label = f"native case {n} ({a:08x} / {b:08x})"
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[28:], f"{label} stack bytes differ")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)}; all 14 CPU words and the {FRAME_SIZE}-byte stack window "
          f"(both PUSHF words); no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"]), file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-longdiv-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, image, function)
            if args.native:
                report["native"] = native_audit(target, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
