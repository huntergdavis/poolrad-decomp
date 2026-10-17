#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u33 +0D13 and +0E9E.

The merged entry-8 chain expects unit 0x9E entry 8 at offset 0DE3 of its
segment and the SYSTEM generator at 0AF8:11C0, so the COM keeps the original
unit 0x9E span at CS:0DE3 and copies the original SYSTEM bytes to the absolute
paragraph 0AF8 at start-up. The original unit 0x33 pieces (+08A4..+0B96 and
+0D13..+11B3) are placed at a common base so their PUSH CS/CALL displacements
stay valid, and the eleven far-call segment words are relocated to one stub
paragraph by the loader's arithmetic. Each case primes the frame, seed, class
byte, window word, grid and table, enters one body through PUSH CS/CALL and
captures the 14 CPU words, the frame window, the seed and the grid. Media and
images stay under resources/.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16

MAIN_AT, STUB_AT, DICE_AT, SYSTEM_COPY_AT, U33_BASE, FIXTURES_AT = 0x0300, 0x0B48, 0x0DE3, 0x0F00, 0x1200, 0x4A00
STUB_DELTA = (STUB_AT - 0x48) // 16
FLAGS_AT, BODIES_AT = U33_BASE + 0x08A4, U33_BASE + 0x0D13      # 1AA4 and 1F13 (+0E9E copy at 209E)
SYSTEM_CS, SYSTEM_OFFSET, SYSTEM_SIZE = 0x0AF8, 0x11C0, 0x87
WINDOW, GRID, GRID_SIZE, OUTPUT, HEADER = 0x8000, 0xA000, 0x4E9, 0xB000, 0xB800
FRAME_LOW, FRAME_SIZE, STACK_TOP = 0xEF80, 0x80, 0xF000
FIXTURE_SIZE = 12
RECORD_BYTES = 28 + FRAME_SIZE + 4 + GRID_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
BODY_AT = {0: BODIES_AT, 1: BODIES_AT + 0x18B}
KEYS = {0: "0d13", 1: "0e9e"}
SITES_0D13 = (0x0DD6, 0x0DE9, 0x0DFD, 0x0E27)
SITES_0E9E = (0x0F45, 0x0F5E, 0x1026, 0x107B, 0x10D9, 0x1150, 0x116E)
RELOCATED_WORDS = tuple(U33_BASE + s + 3 for s in SITES_0D13 + SITES_0E9E)
TABLE_MODS = (1, 2, 3, 0)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def fixtures():
    rng = random.Random(0x330D13)
    cases = []
    for which in range(2):
        for i in range(70):
            cases.append(dict(which=which, seed=rng.randrange(1 << 32), class_byte=rng.randrange(256),
                              window=rng.choice((0x00FF, 0x00FF, 0x0000, rng.randrange(0x10000))),
                              grid_seed=rng.randrange(256), grid_step=rng.choice((1, 7, 13, 0)),
                              table_sel=rng.choice((0, 0, 1, 2, 3)), flags=i % len(FLAGS)))
    return cases


def grid_byte(seed, step, i):
    return (seed + step * i) & 0xFF


def table_bytes(sel):
    mod = TABLE_MODS[sel]
    out = bytearray([0x33]) * 0x400
    for b in range(256):
        out[4 * b] = 0x16 if (mod and b % mod == 0) else 0x17
    return bytes(out)


def assembly(directory, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}"]
    for at in RELOCATED_WORDS:
        text.append(f"    mov word ptr [{at:#x}],ax")
    text += [f"    mov byte ptr [{STUB_AT:#x}],0xea", f"    mov word ptr [{STUB_AT + 1:#x}],{DICE_AT:#x}",
             "    mov ax,cs", f"    mov word ptr [{STUB_AT + 3:#x}],ax",
             f"    mov ax,{SYSTEM_CS:#x}", "    mov es,ax", f"    mov di,{SYSTEM_OFFSET:#x}", "    mov si,offset system_copy",
             f"    mov cx,{SYSTEM_SIZE}", "    cld", "    rep movsb", "    push cs", "    pop es",
             f"    mov word ptr [0x6716],{GRID:#x}", "    mov word ptr [0x6718],cs",
             f"    mov word ptr [0x49d2],{WINDOW:#x}", "    mov word ptr [0x49d4],cs",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],offset main_end",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             f"    mov di,{WINDOW:#x}", "    mov cx,0x400", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures",
             "    mov si,ax", "    mov word ptr [case_ptr],si",
             "    mov ax,word ptr [si+1]", "    mov word ptr [0x4428],ax", "    mov ax,word ptr [si+3]", "    mov word ptr [0x442a],ax",
             "    mov al,byte ptr [si+5]", "    mov byte ptr [0x465a],al",
             "    mov ax,word ptr [si+6]", f"    mov word ptr [{WINDOW + 0x366:#x}],ax",
             "    mov al,byte ptr [si+8]", "    mov ah,byte ptr [si+9]", f"    mov di,{GRID:#x}", f"    mov cx,{GRID_SIZE}", "grid_loop:",
             "    mov byte ptr [di],al", "    add al,ah", "    inc di", "    loop grid_loop",
             "    mov bl,byte ptr [si+10]", "    xor bh,bh", "    mov bl,byte ptr [table_mods+bx]",
             "    mov di,0x2747", "    mov cx,0x400", "    mov al,0x33", "    rep stosb",
             "    xor cx,cx", "    mov di,0x2747", "table_loop:", "    mov byte ptr [di],0x17",
             "    test bl,bl", "    jz table_next", "    mov ax,cx", "    div bl", "    test ah,ah", "    jnz table_next",
             "    mov byte ptr [di],0x16", "table_next:", "    add di,4", "    inc cx", "    cmp cx,256", "    jb table_loop",
             "    mov si,word ptr [case_ptr]", "    mov al,byte ptr [si]", "    xor ah,ah", "    shl ax,1", "    mov bx,ax",
             "    mov ax,word ptr [body_table+bx]", "    mov word ptr [target],ax",
             "    mov bl,byte ptr [si+11]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
             "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    push cs", "    call word ptr [target]", "returned:", f"    mov word ptr [{OUTPUT + 18:#x}],offset returned"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 20:#x}],ds", f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{FRAME_LOW:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             "    mov si,0x4428", "    mov cx,4", "    rep movsb",
             f"    mov si,{GRID:#x}", f"    mov cx,{GRID_SIZE}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", "target: .word 0",
             "body_table: .word " + ",".join(f"{BODY_AT[i]:#x}" for i in range(2)),
             "table_mods: .byte " + ",".join(str(m) for m in TABLE_MODS),
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS), "main_end:",
             f".org {STUB_AT - 0x100:#x}", "stubs:", ".space 5",
             f".org {DICE_AT - 0x100:#x}", "dice:", f'.incbin "{(directory / "dice.bin").resolve()}"',
             f".org {SYSTEM_COPY_AT - 0x100:#x}", "system_copy:", f'.incbin "{(directory / "system.bin").resolve()}"',
             f".org {FLAGS_AT - 0x100:#x}", "flags_body:", f'.incbin "{(directory / "flags.bin").resolve()}"',
             f".org {BODIES_AT - 0x100:#x}", "bodies:", f'.incbin "{(directory / "bodies.bin").resolve()}"',
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["which"]] + list(struct.pack("<I", case["seed"])) + [case["class_byte"]] + list(struct.pack("<H", case["window"])) + \
              [case["grid_seed"], case["grid_step"], case["table_sel"], case["flags"]]
        check(len(row) == FIXTURE_SIZE, f"fixture row size {len(row)}")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


def expected_ram(segment, image, system, case, returned):
    memory = bytearray([0xA5]) * SIZE
    base = physical(segment, 0x100)
    memory[base:base + len(image)] = image
    for at in RELOCATED_WORDS:
        put16(memory, segment, at, (segment + STUB_DELTA) & 0xFFFF)
    memory[physical(segment, STUB_AT)] = 0xEA
    put16(memory, segment, STUB_AT + 1, DICE_AT); put16(memory, segment, STUB_AT + 3, segment)
    at = physical(SYSTEM_CS, SYSTEM_OFFSET); memory[at:at + SYSTEM_SIZE] = system
    put16(memory, segment, 0x6716, GRID); put16(memory, segment, 0x6718, segment)
    put16(memory, segment, 0x49D2, WINDOW); put16(memory, segment, 0x49D4, segment)
    at = physical(segment, WINDOW); memory[at:at + 0x400] = bytes([0xA5]) * 0x400
    put16(memory, segment, 0x4428, case["seed"] & 0xFFFF); put16(memory, segment, 0x442A, case["seed"] >> 16)
    memory[physical(segment, 0x465A)] = case["class_byte"]
    put16(memory, segment, WINDOW + 0x366, case["window"])
    for i in range(GRID_SIZE):
        memory[physical(segment, GRID + i)] = grid_byte(case["grid_seed"], case["grid_step"], i)
    at = physical(segment, 0x2747); memory[at:at + 0x400] = table_bytes(case["table_sel"])
    sp = STACK_TOP - 2; put16(memory, segment, sp, segment)
    sp -= 2; put16(memory, segment, sp, returned)
    cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"], ENTRY_REGS["di"],
              ENTRY_REGS["bp"], sp, segment, BODY_AT[case["which"]], segment, segment, segment, FLAGS[case["flags"]])
    return memory, cpu


def native_audit(root, bodies, code, functions):
    directory = root / "resources/oracle-runs/u33-0d13-0e9e-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    flags = code["code"][0x08A4:0x0B97]
    pieces = code["code"][0x0D13:0x11B4]
    (directory / "flags.bin").write_bytes(flags)
    (directory / "bodies.bin").write_bytes(pieces)
    (directory / "dice.bin").write_bytes(code["dice"])
    (directory / "system.bin").write_bytes(code["system"])
    cases = fixtures()
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(assembly(directory, cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(com)], check=True)
    image = com.read_bytes()
    check(0x100 + len(image) < WINDOW, "COM overlaps the primed data")
    for at, body in ((FLAGS_AT, flags), (BODIES_AT, pieces), (DICE_AT, code["dice"]), (SYSTEM_COPY_AT, code["system"])):
        check(image[at - 0x100:at - 0x100 + len(body)] == body, f"copied original bytes differ at {at:#x}")
    for at in RELOCATED_WORDS:
        check(struct.unpack_from("<H", image, at - 0x100)[0] == 0x00B0, "unrelocated segment word differs")
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
    segment, returned, count, main_end = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    hole = (SYSTEM_CS << 4) + SYSTEM_OFFSET - (segment << 4)
    check(main_end <= STUB_AT, f"harness code reaches the stub paragraph: {main_end:#x}")
    check(BODIES_AT + len(pieces) <= hole and hole + SYSTEM_SIZE <= FIXTURES_AT, f"absolute SYSTEM paragraph lands on used COM offset {hole:#x}")
    fixtures_end = FIXTURES_AT + len(cases) * FIXTURE_SIZE
    for low, high, name in ((0x2747, 0x2B47, "table"), (0x4428, 0x442C, "seed"), (0x465A, 0x465B, "class byte"),
                            (0x49D2, 0x49D6, "window pointer"), (0x6716, 0x671A, "grid pointer"), (WINDOW, STACK_TOP, "data")):
        check(fixtures_end <= low or FIXTURES_AT >= high, f"fixture table overlaps the {name} bytes")
        for lo, hi in ((MAIN_AT, main_end), (FLAGS_AT, FLAGS_AT + len(flags)), (BODIES_AT, BODIES_AT + len(pieces))):
            check(hi <= low or lo >= high, f"copied code overlaps the {name} bytes")
    totals = {k: 0 for k in KEYS.values()}
    for index, case in enumerate(cases):
        record = result[8 + index * RECORD_BYTES:8 + (index + 1) * RECORD_BYTES]
        key = KEYS[case["which"]]
        memory, cpu = expected_ram(segment, image, code["system"], case, returned)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        chain = ctypes.c_int(-1)
        status = functions[key](ram, SIZE, ctypes.byref(cpu), ctypes.byref(chain))
        check(status == 0, f"C status {status} (chain {chain.value}) on native case {index} (+{key})")
        actual = bytes(ram)
        native_cpu = struct.unpack_from("<14H", record)
        for (name, _), want in zip(Cpu._fields_, native_cpu):
            got = getattr(cpu, name)
            check(got == want, f"native case {index} (+{key}) {name}: C={got:04x}, DOS={want:04x}")
        position = 28
        for offset, size in ((FRAME_LOW, FRAME_SIZE), (0x4428, 4), (GRID, GRID_SIZE)):
            at = physical(segment, offset)
            if actual[at:at + size] != record[position:position + size]:
                diff = [i for i in range(size) if actual[at + i] != record[position + i]]
                raise ValueError(f"native case {index} (+{key}) RAM {offset:04x}+{diff[:8]} differs")
            position += size
        totals[key] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {totals}; all 14 CPU words, {FRAME_SIZE} frame bytes, the seed and "
          f"{GRID_SIZE} grid bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM at absolute 0AF8:11C0 (COM offset {hole:#x})")
    return dict(cases=totals, result_sha256=digest, relocated_stub_paragraph_delta=STUB_DELTA, system_com_offset=hole)
