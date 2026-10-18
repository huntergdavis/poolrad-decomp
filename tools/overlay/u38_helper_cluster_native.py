"""DOSBox-X synthetic oracle for the raw-CPU U38 helper cluster (imported by u38_helper_cluster_audit.py).

The COM holds the original unit 0x38 code at offset 0 of its own paragraph, so every
entry runs at its original IP and every PUSH CS; CALL +0000 reaches the original
helper. Each case writes the six DS bytes the cluster reads, pushes the helper's key
word or an entry's five argument words, and far-calls the routine with IRQs masked.
The records hold all 14 CPU words, the DS bytes 6810..682F and 64 stack bytes.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

MAIN_AT, CASES_AT, UNIT_AT = 0x0300, 0x0A00, 0x1400
OUTPUT, HEADER, STACK_TOP = 0x9000, 0x9800, 0xF000
FRAME_LOW, FRAME_SIZE, DS_LOW, DS_SIZE = 0xEFC0, 0x40, 0x6810, 0x20
RECORD_BYTES = 28 + DS_SIZE + FRAME_SIZE
ORDER = (0x0000, 0x1490, 0x29B6, 0x29CB, 0x29D9, 0x29F0, 0x2A11)
DS_BYTES = (0x6816, 0x6817, 0x6818, 0x6819, 0x681C, 0x682A)
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x5A00, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
ARG_WORDS = (0x1111, 0x2222, 0x3333, 0x4444, 0x5555)
CASE_SIZE = 12


def check(condition, message):
    if not condition:
        raise ValueError(message)


def native_cases():
    rng = random.Random(0x38C0DF)
    cases = []
    for k in [0, 0x0B, 0x34, 0x35, 0x37, 0x80, 0xFF] + [rng.randrange(256) for _ in range(25)]:
        values = [rng.randrange(256) for _ in DS_BYTES]
        values[1] = k if rng.random() < 0.5 else rng.randrange(256)
        cases.append(dict(function=0, key=k, values=values))
    for function in range(1, len(ORDER)):
        for i in range(24):
            values = [rng.randrange(256) for _ in DS_BYTES]
            values[3] = (values[3] & 0xF0) | (i & 0x0F)
            values[1] = rng.choice((0, 0x0B, 0x34, 0x35, 0x37, values[1]))
            if ORDER[function] == 0x1490:
                values[4] = (0, 1, 2, 5, 0x40, 0xFF)[i % 6]
            if ORDER[function] == 0x29F0 and i % 2 == 0:
                values[5] = 0
            cases.append(dict(function=function, key=0, values=values))
    return cases


def assembly(code, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{UNIT_AT // 16:#x}", "    mov word ptr [target_far+2],ax",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax"]
    for n, offset in enumerate(DS_BYTES):
        text += [f"    mov al,byte ptr [si+{2 + n}]", f"    mov byte ptr [{offset:#x}],al"]
    text += ["    mov ax,word ptr [si]", "    mov word ptr [target_far],ax",
             "    cmp byte ptr [si+9],0", "    jne entry_frame",
             "    mov al,byte ptr [si+8]", "    mov ah,0x5a", "    push ax", "    jmp framed",
             "entry_frame:"]
    for word in reversed(ARG_WORDS):
        text += [f"    mov ax,{word:#x}", "    push ax"]
    text += ["framed:", "    mov bl,byte ptr [si+10]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr [target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{DS_LOW:#x}", f"    mov cx,{DS_SIZE}", "    rep movsb",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "target_far: .word 0, 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for n, case in enumerate(cases):
        v = case["values"]
        text.append(f"    .word {ORDER[case['function']]:#x}")
        text.append("    .byte " + ",".join(f"{b:#x}" for b in v) + f",{case['key']:#x},{1 if case['function'] else 0},{n % len(FLAGS)},0")
    text += [f".org {UNIT_AT - 0x100:#x}", "unit:"]
    for i in range(0, len(code), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in code[i:i + 16]))
    return "\n".join(text) + "\n"


def native_audit(targets, code, functions):
    from u38_helper_cluster_audit import TARGETS, call
    del targets
    directory = ROOT / "resources/oracle-runs/u38c-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    cases = native_cases()
    check(UNIT_AT + len(code) <= DS_LOW and CASES_AT + CASE_SIZE * len(cases) <= UNIT_AT, "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(code, cases))
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
    unit_cs = (segment + UNIT_AT // 16) & 0xFFFF
    per = [0] * len(ORDER)
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        entry = ORDER[case["function"]]
        memory = bytearray(SIZE)
        base = physical(unit_cs, 0); memory[base:base + len(code)] = code
        for offset, value in zip(DS_BYTES, case["values"]):
            memory[physical(segment, offset)] = value
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        words = list(ARG_WORDS) if case["function"] else [0x5A00 | case["key"]]
        for value in list(reversed(words)) + [segment, returned]:            # the harness pushes the last word first
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, unit_cs, entry, segment, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = call(functions, entry, ram, cpu, 400)
        check(status == 0, f"C status {status} on native case {n} (+{entry:04X})")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} (+{entry:04X}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for start, length in ((DS_LOW, DS_SIZE), (FRAME_LOW, FRAME_SIZE)):
            at = physical(segment, start)
            if bytes(ram[at:at + length]) != record[offset:offset + length]:
                diff = [i for i in range(length) if ram[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} (+{entry:04X}) bytes at {start + diff[0]:#06x} differ ({len(diff)} bytes)")
            offset += length
        per[case["function"]] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({', '.join(f'+{e:04X}: {k}' for e, k in zip(ORDER, per))}); "
          f"all 14 CPU words, {DS_SIZE} DS bytes and {FRAME_SIZE} stack bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), result_sha256=digest)
