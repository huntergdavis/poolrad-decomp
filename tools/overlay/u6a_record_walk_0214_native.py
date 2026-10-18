#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u6A +0214 with +0159, +019C and u86 +323D.

Each DOSBox run executes one synthetic COM (DS = CS = SS = its segment) that bakes
one generated world at its real offsets: the u86 predicate tables at DS:31FB and
DS:5582, a chain of records with their item bytes, node-list heads and +104h
links, and the nodes. The COM holds the original unit 0x6A span +0159..+025B (so
both PUSH CS/CALL displacements are original), the original u86 +323D body and a
loaded-state stub whose paragraph the +019C far-call segment word is relocated to
by the loader's arithmetic. Per case it refills the frame window, restores the
record and node areas from pristine copies, sets the list head and enters +0214
with interrupts disabled, then captures all 14 CPU words, the frame window and
both areas. Media and images stay under ignored resources/.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16
import u6a_record_walk_0214_audit as audit

MAIN_AT, FIXTURES_AT = 0x0300, 0x0C00
U6A_AT = 0x1000                           # unit 0x6A offset 0; the span +0159..+025B lands at 1159
CALLEE_AT = 0x1400                        # u86 +323D copy
STUB_AT = 0x150E                          # stub paragraph CS + 14Dh holds offset 003E here
STUB_DELTA = (STUB_AT - 0x3E) // 16
RECORDS, RECORD_STRIDE, RECORD_COUNT = 0xA000, 0x200, 5
NODES, NODE_BYTES = 0xB000, 0x200
PRISTINE = (0xB400, 0xBE00)               # copies of the record area (0xA00) and the node area (0x200)
OUTPUT, HEADER = 0xC000, 0xCF00
FRAME_LOW, FRAME_SIZE, STACK_TOP = 0xEF80, 0x80, 0xF000
AREAS = ((FRAME_LOW, FRAME_SIZE), (RECORDS, RECORD_STRIDE * RECORD_COUNT), (NODES, NODE_BYTES))
FIXTURE_SIZE = 4
RECORD_BYTES = 28 + sum(n for _, n in AREAS)
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SITE_WORD = 0x01C3 + 3                    # +019C's LCALL 0096:003E segment word


def check(condition, message):
    if not condition:
        raise ValueError(message)


def world(seed):
    rng = random.Random(seed)
    data = {}
    for i in range(128):
        data[0x31FB + 16 * i] = rng.randrange(256)
    for i in range(256):
        data[0x5582 + 16 * i] = rng.randrange(256)
    area = bytearray(RECORD_STRIDE * RECORD_COUNT)
    nodes = bytearray(NODE_BYTES)
    node_slot = 0
    for k in range(RECORD_COUNT):
        base = k * RECORD_STRIDE
        for i in range(22):
            area[base + 0x17 + i] = rng.choice((rng.randrange(0x80), 0x80 | rng.randrange(0x80), 0xFF))
        count = rng.randrange(3) if node_slot + 2 <= NODE_BYTES // 0x40 else 0
        head = (0, 0)
        for j in range(count):
            off = (node_slot + j) * 0x40
            nodes[off + 0x2E] = rng.randrange(256)
            for b in range(3):
                nodes[off + 0x3C + b] = rng.randrange(256)
            nxt = (0, 0) if j + 1 == count else (None, NODES + (node_slot + j + 1) * 0x40)
            struct.pack_into("<HH", nodes, off + 0x2A, nxt[1], 0 if nxt[0] == 0 else 0xFFFF)   # segment patched at run time
            if j == 0:
                head = (None, NODES + node_slot * 0x40)
        node_slot += count
        struct.pack_into("<HH", area, base + 0xC8, head[1], 0 if head[0] == 0 else 0xFFFF)
        link = (0, 0) if k + 1 == RECORD_COUNT else (None, RECORDS + (k + 1) * RECORD_STRIDE)
        struct.pack_into("<HH", area, base + 0x104, link[1], 0 if link[0] == 0 else 0xFFFF)
    return data, area, nodes


def cases(index):
    rng = random.Random(0x6A0214 + index)
    return [dict(head=h, flags=rng.randrange(len(FLAGS))) for h in (0, 2, 0xFF, 0, 4)]   # FF: empty list


def assembly(cases_):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}", f"    mov word ptr [{U6A_AT + SITE_WORD:#x}],ax",
            f"    mov byte ptr [{STUB_AT:#x}],0xea", f"    mov word ptr [{STUB_AT + 1:#x}],{CALLEE_AT:#x}",
            "    mov ax,cs", f"    mov word ptr [{STUB_AT + 3:#x}],ax",
            # patch every FFFF segment placeholder in the pristine areas to CS
            f"    mov si,{PRISTINE[0]:#x}", f"    mov cx,{(RECORD_STRIDE * RECORD_COUNT + NODE_BYTES) // 2:#x}", "fix_loop:",
            "    cmp word ptr [si],0xffff", "    jne fix_next", "    mov word ptr [si],ax", "fix_next:", "    add si,2", "    loop fix_loop",
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],{len(cases_)}", f"    mov word ptr [{HEADER + 6:#x}],0",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
            "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            f"    mov si,{PRISTINE[0]:#x}", f"    mov di,{RECORDS:#x}", f"    mov cx,{RECORD_STRIDE * RECORD_COUNT:#x}", "    rep movsb",
            f"    mov si,{PRISTINE[1]:#x}", f"    mov di,{NODES:#x}", f"    mov cx,{NODE_BYTES:#x}", "    rep movsb",
            "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures", "    mov si,ax",
            "    mov word ptr [0x5d96],0", "    mov word ptr [0x5d98],0",
            "    mov al,byte ptr [si]", "    cmp al,0xff", "    je head_set",
            "    xor ah,ah", f"    mov bx,{RECORD_STRIDE:#x}", "    mul bx", f"    add ax,{RECORDS:#x}",
            "    mov word ptr [0x5d96],ax", "    mov word ptr [0x5d98],cs",
            "head_set:", "    mov bl,byte ptr [si+1]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    push cs", f"    call {U6A_AT + 0x0214:#x}", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", f"    mov word ptr [{OUTPUT + 20:#x}],ds",
             f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}"]
    for start, length in AREAS:
        text += [f"    mov si,{start:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases_)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases_:
        text.append(f"    .byte {case['head']},{case['flags']},0,0")
    return "\n".join(text) + "\n"


def build_image(assembled, code, callee, data, area, nodes):
    image = bytearray(PRISTINE[1] + NODE_BYTES - 0x100)
    def place(at, blob):
        image[at - 0x100:at - 0x100 + len(blob)] = blob
    place(0x100, assembled)
    place(U6A_AT + 0x0159, code[0x0159:0x025C])
    place(CALLEE_AT, callee)
    place(PRISTINE[0], area)
    place(PRISTINE[1], nodes)
    used = [(0x100, 0x100 + len(assembled)), (U6A_AT + 0x159, U6A_AT + 0x25C), (CALLEE_AT, CALLEE_AT + len(callee)),
            (STUB_AT, STUB_AT + 5), (RECORDS, RECORDS + len(area)), (NODES, NODES + NODE_BYTES),
            (PRISTINE[0], PRISTINE[0] + len(area)), (PRISTINE[1], PRISTINE[1] + NODE_BYTES), (0x5D96, 0x5D9A)]
    for offset, value in data.items():
        check(not any(lo <= offset < hi for lo, hi in used), f"table byte DS:{offset:04X} collides")
        image[offset - 0x100] = value
    return bytes(image)


def expected(segment, image, case):
    memory = bytearray([0xA5]) * SIZE
    base = physical(segment, 0x100)
    memory[base:base + len(image)] = image
    put16(memory, segment, U6A_AT + SITE_WORD, (segment + STUB_DELTA) & 0xFFFF)
    memory[physical(segment, STUB_AT)] = 0xEA
    put16(memory, segment, STUB_AT + 1, CALLEE_AT); put16(memory, segment, STUB_AT + 3, segment)
    for start in (PRISTINE[0], PRISTINE[1]):
        length = RECORD_STRIDE * RECORD_COUNT if start == PRISTINE[0] else NODE_BYTES
        for i in range(0, length, 2):
            if memory[physical(segment, start + i)] == 0xFF and memory[physical(segment, start + i + 1)] == 0xFF:
                put16(memory, segment, start + i, segment)
    for src, dst, length in ((PRISTINE[0], RECORDS, RECORD_STRIDE * RECORD_COUNT), (PRISTINE[1], NODES, NODE_BYTES)):
        for i in range(length):
            memory[physical(segment, dst + i)] = memory[physical(segment, src + i)]
    if case["head"] == 0xFF:
        put16(memory, segment, 0x5D96, 0); put16(memory, segment, 0x5D98, 0)
    else:
        put16(memory, segment, 0x5D96, RECORDS + case["head"] * RECORD_STRIDE); put16(memory, segment, 0x5D98, segment)
    return memory


def native_audit(root, target, code, callee, function):
    del target
    totals, digests = 0, []
    for index in range(6):
        directory = root / f"resources/oracle-runs/u6a-0214-synthetic/world-{index}"
        directory.mkdir(parents=True, exist_ok=True)
        data, area, nodes = world(0x6A02140 + index)
        cases_ = cases(index)
        asm, obj = directory / "oracle.S", directory / "oracle.o"
        asm.write_text(assembly(cases_))
        subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
        subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
        assembled = (directory / "main.bin").read_bytes()
        check(0x100 + len(assembled) <= U6A_AT + 0x159, "harness reaches the unit 0x6A span")
        image = build_image(assembled, code, callee, data, area, nodes)
        (directory / "ORACLE.COM").write_bytes(image)
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
                           env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=180)
        result = result_path.read_bytes()
        check(len(result) == 8 + len(cases_) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
        segment, returned, count, _ = struct.unpack_from("<4H", result)
        check(count == len(cases_), "native header differs")
        for n, case in enumerate(cases_):
            record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
            memory = expected(segment, image, case)
            sp = STACK_TOP
            for value in (segment, returned):
                sp -= 2; put16(memory, segment, sp, value)
            cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"], ENTRY_REGS["di"],
                      ENTRY_REGS["bp"], sp, segment, U6A_AT + 0x0214, segment, segment, segment, FLAGS[case["flags"]])
            ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
            chain = ctypes.c_int(-1)
            check(function(ram, SIZE, ctypes.byref(cpu), 64, ctypes.byref(chain)) == 0, f"C status on world {index} case {n}")
            actual = bytes(ram)
            for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
                check(getattr(cpu, name) == want, f"native world {index} case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
            offset = 28
            for start, length in AREAS:
                at = physical(segment, start)
                if actual[at:at + length] != record[offset:offset + length]:
                    diff = [i for i in range(length) if actual[at + i] != record[offset + i]]
                    raise ValueError(f"native world {index} case {n} bytes at {start:#x}+{diff[:6]} differ")
                offset += length
            totals += 1
        digests.append(sha256(result).hexdigest())
    combined = sha256("".join(digests).encode()).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {totals} over 6 worlds; all 14 CPU words, {FRAME_SIZE} frame "
          f"bytes and the record and node areas; no mask")
    print(f"native combined result SHA-256 {combined}")
    return dict(cases=totals, worlds=6, result_sha256=combined)
