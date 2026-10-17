#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u33 +012B/+01AD and the folded u3BD lookups.

A synthetic COM holds the original unit 0x33 span (+012B..+0220), the original
unit 0x3BD span (+031F..+0579) and two loaded-state far-jump stubs in one stub
paragraph. The +012B copy's two far-call segment words are relocated exactly as
the loader's fixups relocate them, so each call still reaches stub offset 002F
or 002A of that paragraph. Each case primes the frame, the map/party bytes, the
far buffer pointer, the 1,024-byte buffer and the step tables, enters one of the
four bodies through PUSH CS/CALL and captures all 14 CPU words plus the frame
window. Media and generated images stay under ignored resources/.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16

U33_AT, U3BD_AT, STUB_AT, MAIN_AT, FIXTURES_AT = 0x0300, 0x0500, 0x082A, 0x0900, 0x3000
STUB_DELTA = (STUB_AT - 0x2A) // 16             # the relocated stub paragraph is CS + STUB_DELTA
BUFFER, OUTPUT, HEADER = 0xB000, 0xC000, 0xC400
FRAME_LOW, FRAME_SIZE, STACK_TOP = 0xEF80, 0x80, 0xF000
FIXTURE_SIZE = 8
RECORD_BYTES = 28 + FRAME_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
BODY_AT = {0: U33_AT, 1: U33_AT + 0x82, 2: U3BD_AT + 0x31, 3: U3BD_AT + 0x15C}   # +012B, +01AD, +0350, +047B
KEYS = {0: "012b", 1: "01ad", 2: "0350", 3: "047b"}
LINK = 0x7777


def check(condition, message):
    if not condition:
        raise ValueError(message)


def fixtures():
    rng = random.Random(0x3312B)
    cases = []
    coords = (-1, 0, 1, 7, 15, 16, 3, 4)
    for which in range(4):
        for i in range(150):
            kind = i % 5
            if kind == 0:
                x, y, facing = rng.choice(coords), rng.choice(coords), rng.randrange(8)
            elif kind == 1:
                x, y, facing = rng.randrange(16), rng.randrange(16), rng.choice((0, 2, 4, 6))
            elif kind == 2:
                x, y, facing = rng.choice((-1, 16, 0x80)), rng.choice((4, 5)), rng.choice((2, 6, 1, 0xFF))
            else:
                x, y, facing = rng.randrange(256), rng.randrange(256), rng.randrange(256)
            cases.append(dict(which=which, x=x & 0xFF, y=y & 0xFF, facing=facing, map_id=rng.choice((0, 10, 5, 5, 5)),
                              party_y=rng.choice((4, 5, rng.randrange(256))), seed=rng.randrange(256), flags=i % len(FLAGS)))
    return cases


def buffer_byte(seed, i):
    return (seed + 13 * i) & 0xFF


def table_byte(i):
    return (3 + 5 * i) & 0xFF


def assembly(directory, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {U33_AT - 0x100:#x}", "u33span:", f'.incbin "{(directory / "u33.bin").resolve()}"',
            f".org {U3BD_AT - 0x100:#x}", "u3bdspan:", f'.incbin "{(directory / "u3bd.bin").resolve()}"',
            f".org {STUB_AT - 0x100:#x}", "stubs:", ".space 10",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            # loader-style relocation of the two far-call segment words in the +012B copy
            "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}",
            f"    mov word ptr [{U33_AT + 0x2D:#x}],ax", f"    mov word ptr [{U33_AT + 0x48:#x}],ax",
            # loaded-state stubs: 002A -> +047B copy, 002F -> +0350 copy
            f"    mov byte ptr [{STUB_AT:#x}],0xea", f"    mov word ptr [{STUB_AT + 1:#x}],{BODY_AT[3]:#x}",
            "    mov ax,cs", f"    mov word ptr [{STUB_AT + 3:#x}],ax",
            f"    mov byte ptr [{STUB_AT + 5:#x}],0xea", f"    mov word ptr [{STUB_AT + 6:#x}],{BODY_AT[2]:#x}",
            f"    mov word ptr [{STUB_AT + 8:#x}],ax",
            # far buffer pointer and step tables
            f"    mov word ptr [0x6a5c],{BUFFER:#x}", "    mov word ptr [0x6a5e],cs",
            "    mov di,0x2736", "    mov cx,0x10a", "    mov al,3", "table_loop:", "    mov byte ptr [di],al",
            "    add al,5", "    inc di", "    loop table_loop",
            # header: segment, returned4, returned3, count
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned4",
            f"    mov word ptr [{HEADER + 4:#x}],offset returned3", f"    mov word ptr [{HEADER + 6:#x}],{len(cases)}",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures",
            "    mov si,ax",
            "    mov al,byte ptr [si+1]", "    xor ah,ah", "    mov word ptr [arg_x],ax",
            "    mov al,byte ptr [si+2]", "    xor ah,ah", "    mov word ptr [arg_y],ax",
            "    mov al,byte ptr [si+3]", "    xor ah,ah", "    mov word ptr [arg_f],ax",
            "    mov al,byte ptr [si+4]", "    mov byte ptr [0x84dc],al",
            "    mov al,byte ptr [si+5]", "    mov byte ptr [0x6aae],al",
            "    mov al,byte ptr [si+6]", f"    mov di,{BUFFER:#x}", "    mov cx,0x400", "buffer_loop:",
            "    mov byte ptr [di],al", "    add al,13", "    inc di", "    loop buffer_loop",
            # body and path by which (0..3), chosen before the flags are loaded
            "    mov al,byte ptr [si]", "    xor ah,ah", "    shl ax,1", "    mov bx,ax",
            "    mov ax,word ptr [body_table+bx]", "    mov word ptr [target],ax",
            "    mov ax,offset path3", "    cmp bx,0", "    jne path_chosen", "    mov ax,offset path4",
            "path_chosen:", "    mov word ptr [path],ax",
            "    mov bl,byte ptr [si+7]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
            "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    jmp word ptr [path]",
             "path4:", "    push word ptr [arg_x]", "    push word ptr [arg_y]", "    push word ptr [arg_f]",
             f"    push {LINK:#x}", "    push cs", "    call word ptr [target]", "returned4:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned4", "    jmp capture",
             "path3:", "    push word ptr [arg_x]", "    push word ptr [arg_y]", "    push word ptr [arg_f]",
             "    push cs", "    call word ptr [target]", "returned3:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned3",
             "capture:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 20:#x}],ds", f"    mov word ptr [{OUTPUT + 22:#x}],es",
             f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             # SP is captured; park the stack above the frame window before the harness pushes anything
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{FRAME_LOW:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "target: .word 0", "path: .word 0",
             "arg_x: .word 0", "arg_y: .word 0", "arg_f: .word 0",
             "body_table: .word " + ",".join(f"{BODY_AT[i]:#x}" for i in range(4)),
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["which"], case["x"], case["y"], case["facing"], case["map_id"], case["party_y"], case["seed"], case["flags"]]
        check(len(row) == FIXTURE_SIZE, "fixture row size")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


def expected_ram(segment, image, case, returned):
    """The RAM the C model starts from: the loaded COM with its runtime patches and the primed data."""
    memory = bytearray([0xA5]) * SIZE
    base = physical(segment, 0x100)
    memory[base:base + len(image)] = image
    put16(memory, segment, U33_AT + 0x2D, (segment + STUB_DELTA) & 0xFFFF)
    put16(memory, segment, U33_AT + 0x48, (segment + STUB_DELTA) & 0xFFFF)
    for offset, body in ((STUB_AT, BODY_AT[3]), (STUB_AT + 5, BODY_AT[2])):
        memory[physical(segment, offset)] = 0xEA
        put16(memory, segment, offset + 1, body)
        put16(memory, segment, offset + 3, segment)
    put16(memory, segment, 0x6A5C, BUFFER)
    put16(memory, segment, 0x6A5E, segment)
    for i in range(0x10A):
        memory[physical(segment, 0x2736 + i)] = table_byte(i)
    memory[physical(segment, 0x84DC)] = case["map_id"]
    memory[physical(segment, 0x6AAE)] = case["party_y"]
    for i in range(0x400):
        memory[physical(segment, BUFFER + i)] = buffer_byte(case["seed"], i)
    words = [case["x"], case["y"], case["facing"]] + ([LINK] if case["which"] == 0 else [])
    sp = STACK_TOP
    for value in words:
        sp -= 2
        put16(memory, segment, sp, value)
    sp -= 2; put16(memory, segment, sp, segment)
    sp -= 2; put16(memory, segment, sp, returned)
    cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"],
              ENTRY_REGS["di"], ENTRY_REGS["bp"], sp, segment, BODY_AT[case["which"]], segment, segment, segment,
              FLAGS[case["flags"]])
    return memory, cpu


def native_audit(root, bodies, spans, functions):
    directory = root / "resources/oracle-runs/u33-012b-01ad-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "u33.bin").write_bytes(spans[0x33])
    (directory / "u3bd.bin").write_bytes(spans[0x3BD])
    cases = fixtures()
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(assembly(directory, cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(com)], check=True)
    image = com.read_bytes()
    check(0x100 + len(image) < 0x6A5C, "COM overlaps the primed data")
    check(image[U33_AT - 0x100:U33_AT - 0x100 + len(spans[0x33])] == spans[0x33], "copied unit 0x33 span differs")
    check(image[U3BD_AT - 0x100:U3BD_AT - 0x100 + len(spans[0x3BD])] == spans[0x3BD], "copied unit 0x3BD span differs")
    for at in (U33_AT + 0x2D, U33_AT + 0x48):
        check(struct.unpack_from("<H", image, at - 0x100)[0] == 0x03D5, "unrelocated segment word differs")
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=120)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned4, returned3, count = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    totals = {k: 0 for k in KEYS.values()}
    af_masked = 0
    for index, case in enumerate(cases):
        record = result[8 + index * RECORD_BYTES:8 + (index + 1) * RECORD_BYTES]
        key = KEYS[case["which"]]
        returned = returned4 if case["which"] == 0 else returned3
        memory, cpu = expected_ram(segment, image, case, returned)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = functions[key](ram, SIZE, ctypes.byref(cpu))
        check(status == 0, f"C status {status} on native case {index} (+{key})")
        actual = bytes(ram)
        native_cpu = struct.unpack_from("<14H", record)
        for (name, _), want in zip(Cpu._fields_, native_cpu):
            got = getattr(cpu, name)
            if got != want and name == "flags" and (got ^ want) == 0x0010 and case["which"] in (2, 3):
                af_masked += 1   # SHR leaves AF undefined; the model follows Unicorn's zero
                continue
            check(got == want, f"native case {index} (+{key}) {name}: C={got:04x}, DOS={want:04x}")
        at = physical(segment, FRAME_LOW)
        if actual[at:at + FRAME_SIZE] != record[28:28 + FRAME_SIZE]:
            diff = [i for i in range(FRAME_SIZE) if actual[at + i] != record[28 + i]]
            raise ValueError(f"native case {index} (+{key}) frame bytes {diff[:8]} differ")
        totals[key] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {totals}; all 14 CPU words and {FRAME_SIZE} frame bytes; "
          f"{af_masked} direct lookup cases differed only in undefined AF after SHR")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=totals, result_sha256=digest, af_only_differences=af_masked, relocated_stub_paragraph_delta=STUB_DELTA)
