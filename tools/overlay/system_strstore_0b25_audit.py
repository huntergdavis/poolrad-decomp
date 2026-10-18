#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU SYSTEM 0AF8:0B25 string store, plus a DOSBox-X oracle.

The routine's 36 bytes come from the hash-checked expanded START.EXE image. The
Unicorn differential places the whole image at load segment 0 and runs the original
at 0AF8:0B25 against the C model, comparing the entire 1 MiB RAM and all 14 CPU words
for every source length and cap, overlapping copies in both directions, SI/DI
offset wrap, A20 and segment-end stacks. The DOSBox-X oracle runs the original bytes
at its own paragraph. Media stays private.
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

SYSTEM_CS, ENTRY, END = 0x0AF8, 0x0B25, 0x0B49
SS = 0x2800


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, "START.EXE provenance differs")
    image = bytes(unpack(exe)[0])
    body = image[SYSTEM_CS * 16 + ENTRY:SYSTEM_CS * 16 + END]
    decoded = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 36 and len(decoded) == 16 and decoded[0].mnemonic == "cld" and decoded[-1].mnemonic == "retf"
          and decoded[-1].op_str == "0xa" and sum(i.size for i in decoded) == 36, "SYSTEM 0B25 extent differs")
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in decoded if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    check(len(conditional) == 1, "SYSTEM 0B25 conditional count differs")
    target = dict(body=body, decoded=decoded, conditional=conditional, sha=sha256(body).hexdigest())
    print(f"SYSTEM 0AF8:0B25 string store ({len(body)}/{len(decoded)}, RETF 0Ah) sha {target['sha'][:16]}...")
    return target, image


def library(directory):
    path = directory / "s0b25.so"
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared",
                    "-I", str(ROOT / "include"), str(ROOT / "src/recovered/system_strstore_fullstate_0b25.c"),
                    "-o", str(path)], check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_system_strstore_0b25
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    f.restype = ctypes.c_int
    return f


def build(image, rng, *, length=None, cap=None, src=None, dst=None, sp=0x8000, ss=SS, flags=0x7202, regs=None, ds=0x1111,
          background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(image)] = image
    src = src or (0x5000 + rng.randrange(0x100), rng.randrange(0x10000))
    dst = dst or (0x6000 + rng.randrange(0x100), rng.randrange(0x10000))
    n = rng.randrange(256) if length is None else length
    memory[physical(src[0], src[1])] = n
    for i in range(1, 256):
        memory[physical(src[0], (src[1] + i) & 0xFFFF)] = rng.randrange(256)
    memory[physical(src[0], src[1])] = n                          # rewrite in case the fill wrapped onto it
    m = rng.randrange(0x10000) if cap is None else cap
    words = [0x7000, 0x7001, m, dst[1], dst[0], src[1], src[0]]
    frame = physical(ss, sp)
    for i, value in enumerate(words):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
        if i < 2:
            memory[(frame + 2 * i) & (SIZE - 1)] = value & 0xFF
            memory[(frame + 2 * i + 1) & (SIZE - 1)] = value >> 8
    reg = regs or tuple(rng.randrange(0x10000) for _ in range(7))
    high = any(s >= 0xF000 for s in (src[0], dst[0], ss))
    return memory, Cpu(*reg, sp, SYSTEM_CS, ENTRY, ds, rng.randrange(0x10000), ss, flags), high


def compare(label, memory, cpu, high, target, function, budget=10000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    start = physical(cpu.cs, ENTRY); body_end = start + len(target["body"]); ret = physical(0x7001, 0x7000)
    previous, visited, edges, returned = [None], set(), set(), [False]

    def instruction(emulator, address, size, user):
        del emulator, size, user
        here = address - start + ENTRY
        visited.add(here)
        if previous[0] in target["conditional"] and previous[0] != here:
            edges.add((previous[0], here))
        previous[0] = here

    def outside(emulator, address, size, user):
        del size, user
        previous[0] = None
        if address == ret:
            returned[0] = True; emulator.emu_stop()

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, instruction, begin=start, end=body_end - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=0, end=start - 1)
    original.hook_add(UC_HOOK_CODE, outside, begin=body_end, end=2 * SIZE - 1)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(start, 2 * SIZE, count=budget)
    check(returned[0], f"original did not return: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        got, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(got == want, f"{name} mismatch {label}: C={got:04x}, original={want:04x}")
    return visited, edges


def cases():
    rng = random.Random(0x0B25)
    spec = []
    for n in range(256):
        spec.append((f"len-{n:02x}", dict(length=n, cap=(n + rng.choice((-3, 0, 5, 200))) & 0xFF | (rng.randrange(256) << 8))))
    for m in (0, 1, 0x28, 0xFF, 0x100, 0x1FF, 0xFFFF):
        spec.append((f"cap-{m:04x}", dict(length=0x80, cap=m)))
    for i in range(80):
        spec.append((f"random-{i}", dict(flags=(0x7202, 0x0602, 0x0ED7, 0x0647)[i % 4])))
    for delta in (1, 2, 5, 0x10, 0xFF, -1, -2, -5, -0x10):            # overlapping destination
        spec.append((f"overlap-{delta}", dict(length=0xC0, cap=0xFF, src=(0x5000, 0x0400), dst=(0x5000, (0x0400 + delta) & 0xFFFF))))
    spec.append(("dst-is-src", dict(length=0x40, cap=0x20, src=(0x5000, 0x0400), dst=(0x5000, 0x0400))))
    for off in (0xFFF0, 0xFFFF, 0xFF80):                               # SI/DI wrap at 16 bits
        spec.append((f"src-wrap-{off:04x}", dict(length=0x60, cap=0xFF, src=(0x5000, off))))
        spec.append((f"dst-wrap-{off:04x}", dict(length=0x60, cap=0xFF, dst=(0x6000, off))))
    spec.append(("a20-src", dict(length=0x30, cap=0xFF, src=(0xFFFF, 0x0100))))
    spec.append(("a20-dst", dict(length=0x30, cap=0xFF, dst=(0xFFFF, 0x0100))))
    for sp in (0x0002, 0x0004, 0xFFF0, 0xFFF2, 0xFFF4):
        spec.append((f"sp-{sp:04x}", dict(length=0x10, cap=0x20, sp=sp, ss=0x1C00, background=bytes(SIZE))))
    spec.append(("ss-a20", dict(length=0x10, cap=0x20, ss=0xFFFF, sp=0x0400)))
    spec.append(("random-background", dict(background=bytes(random.Random(9).randbytes(SIZE)))))
    return spec


def differential(target, image, function):
    rng = random.Random(0x0B25C0DE)
    visited, edges, count = set(), set(), 0
    for label, options in cases():
        memory, cpu, high = build(image, rng, **options)
        v, e = compare(f"0B25 {label}", memory, cpu, high, target, function)
        visited |= v; edges |= e; count += 1
    missing_sites = sorted(hex(s) for s in set(i.address for i in target["decoded"]) - visited)
    missing_edges = sorted((hex(s), hex(o)) for s, outs in target["conditional"].items() for o in outs if (s, o) not in edges)
    check(not missing_sites and not missing_edges, f"0B25 coverage incomplete: {missing_sites} {missing_edges}")
    print(f"0B25: {count} original-byte cases passed, full 1 MiB RAM and all 14 CPU words equal; "
          f"{len(visited)}/{len(target['decoded'])} instruction starts, {len(edges)}/{2 * len(target['conditional'])} conditional outcomes")
    return dict(returning=count, starts=len(visited), outcomes=len(edges))


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
MAIN_AT, CASES_AT, BODY_PARA = 0x0300, 0x0900, 0x1000
OUTPUT, HEADER, STACK_TOP = 0x9000, 0x9800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
SRC_SEG, DST_SEG = 0x2000, 0x2800
REGION = 0x0300
CASE_SIZE = 12
RECORD_BYTES = 28 + FRAME_SIZE + 2 * REGION
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0603)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_cases():
    rng = random.Random(0x0B25D0)
    cases = []
    for i in range(96):
        n = (0, 1, 0x28, 0xFF)[i % 4] if i < 8 else rng.randrange(256)
        cap = (0, 0x28, 0xFF, 0x1FF)[i % 4] if i < 8 else rng.choice((rng.randrange(256), 0x28, 0xFF, rng.randrange(0x10000)))
        src_off = rng.randrange(0x40, 0x80)
        dst_off = rng.randrange(0x40, 0x80) if i % 3 else src_off + rng.choice((1, 2, 7, -1, -3))   # some overlap (same segment)
        same = i % 3 == 0
        cases.append(dict(n=n, cap=cap & 0xFFFF, src_off=src_off, dst_off=dst_off & 0xFFFF, same=same, fill=rng.randrange(256),
                          flags=i % len(FLAGS)))
    return cases


def native_audit(target, function):
    directory = ROOT / "resources/oracle-runs/system-0b25-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{BODY_PARA // 16:#x}", "    mov word ptr [target_far+2],ax",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds",
            "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
            # fill both regions: byte i = fill + 7i
            f"    mov ax,{SRC_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov al,byte ptr [si+10]", f"    mov cx,{REGION:#x}",
            "fill_src:", "    stosb", "    add al,7", "    loop fill_src",
            f"    mov ax,{DST_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov al,byte ptr [si+10]", "    xor al,0x5a", f"    mov cx,{REGION:#x}",
            "fill_dst:", "    stosb", "    add al,3", "    loop fill_dst",
            # source length byte
            f"    mov ax,{SRC_SEG:#x}", "    mov es,ax", "    mov di,word ptr [si+4]", "    mov al,byte ptr [si]", "    stosb",
            "    push cs", "    pop es", "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            # arguments: source far, destination far, cap
            f"    mov ax,{SRC_SEG:#x}", "    push ax", "    push word ptr [si+4]",
            "    cmp byte ptr [si+8],0", "    je other_segment", f"    mov ax,{SRC_SEG:#x}", "    jmp have_segment",
            "other_segment:", f"    mov ax,{DST_SEG:#x}", "have_segment:", "    push ax", "    push word ptr [si+6]",
            "    push word ptr [si+2]",
            "    mov bl,byte ptr [si+9]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop es", "    push cs", "    pop ds", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{SRC_SEG:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{REGION:#x}", "    rep movsb",
             f"    mov ax,{DST_SEG:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{REGION:#x}", "    rep movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        text.append("    .word " + ",".join(f"{v:#x}" for v in (case["n"], case["cap"], case["src_off"], case["dst_off"])) +
                    f"\n    .byte {1 if case['same'] else 0},{case['flags']},{case['fill']},0")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(text) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) <= BODY_PARA + ENTRY and CASES_AT + CASE_SIZE * len(cases) <= BODY_PARA + ENTRY, "COM layout overlaps")
    image = bytearray(BODY_PARA + ENTRY + len(target["body"]) - 0x100)
    image[:len(assembled)] = assembled
    image[BODY_PARA + ENTRY - 0x100:] = target["body"]
    (directory / "ORACLE.COM").write_bytes(bytes(image))
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
        for i in range(REGION):
            memory[physical(SRC_SEG, i)] = (case["fill"] + 7 * i) & 0xFF
            memory[physical(DST_SEG, i)] = ((case["fill"] ^ 0x5A) + 3 * i) & 0xFF
        memory[physical(SRC_SEG, case["src_off"])] = case["n"]
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        dst_seg = SRC_SEG if case["same"] else DST_SEG
        sp = STACK_TOP
        for value in (SRC_SEG, case["src_off"], dst_seg, case["dst_off"], case["cap"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, (segment + BODY_PARA // 16) & 0xFFFF, ENTRY, segment, segment, segment, FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(SRC_SEG, 0):physical(SRC_SEG, 0) + REGION]) + bytes(ram[physical(DST_SEG, 0):physical(DST_SEG, 0) + REGION])
        check(got == record[28:], f"native case {n} bytes differ")
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (a third with same-segment overlap); all 14 CPU words, "
          f"{FRAME_SIZE} stack bytes and both {REGION:#x}-byte string regions; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), result_sha256=digest)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    target, image = source()
    report = dict(target=dict(sha256=target["sha"], bytes=len(target["body"]), instructions=len(target["decoded"])))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-s0b25-") as temp:
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
