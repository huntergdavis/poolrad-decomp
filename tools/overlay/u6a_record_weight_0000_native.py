#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u6A +0000 and +019C with the u86 +323D callee.

A synthetic COM holds the two original caller bodies, the original callee and a
loaded-state far-jump stub. The far call's segment word is relocated exactly as
the DOS loader's fixup would relocate it, so the call still reaches offset 003E
of a stub paragraph; that stub jumps to the copied callee. Each case primes the
frame, record, node and table RAM, enters through PUSH CS/CALL and captures all
14 CPU words plus the frame window, the record and the node area. Media and
generated images stay under ignored resources/.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16

# Absolute COM offsets; GAS .org is section-relative, so the emitted .org is the offset less 0x100.
CALLER0, CALLER1, CALLEE_AT, STUB_AT, MAIN_AT, FIXTURES_AT = 0x0300, 0x0500, 0x0700, 0x080E, 0x0900, 0x6700
STUB_DELTA = (STUB_AT - 0x3E) // 16          # the relocated stub paragraph is CS + STUB_DELTA
RECORD, NODES, NODE_STRIDE, OUTPUT, HEADER = 0xA400, 0xA600, 0x40, 0xA800, 0xAC00
FRAME_LOW, FRAME_SIZE, RECORD_SIZE, NODE_AREA = 0xEF80, 0x80, 0x120, 0x100
STACK_TOP = 0xF000
FIXTURE_SIZE = 36
RECORD_BYTES = 28 + FRAME_SIZE + RECORD_SIZE + NODE_AREA
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def fixtures():
    """Deterministic native cases: which body, node count, 21 item bytes, three nodes, flags selector."""
    rng = random.Random(0x6A86)
    cases = []
    high = lambda i: 0x80 | (i & 0x7F)
    for which in (0, 1):
        for i in range(200):
            kind = i % 8
            if kind == 0:
                items = [rng.randrange(0x80) for _ in range(21)]
            elif kind == 1:
                items = [high(rng.randrange(128)) for _ in range(21)]
            elif kind == 2:
                items = [high(37)] * 21
            elif kind == 3:
                items = [high(j) if j % 3 == 0 else 0x11 for j in range(21)]
            else:
                items = [rng.randrange(256) for _ in range(21)]
            count = (i // 2) % 4
            nodes = []
            for n in range(3):
                if kind in (5, 6):
                    index = (11, 12, 13, 10, 14, 0, 0xFF)[(i + n) % 7]
                else:
                    index = rng.randrange(256)
                bytes3 = [rng.randrange(256) for _ in range(3)] if kind != 7 else [0x80 | n, 0x7F, 0xFF]
                nodes.append((index, bytes3))
            cases.append(dict(which=which, count=count, items=items, nodes=nodes, flags=i % len(FLAGS)))
    return cases


def table1(i):
    return (i * 7) & 0xFF


def table2(i):
    return i & 0x0F


def assembly(directory, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {CALLER0 - 0x100:#x}", "caller0:", f'.incbin "{(directory / "caller0.bin").resolve()}"',
            f".org {CALLER1 - 0x100:#x}", "caller1:", f'.incbin "{(directory / "caller1.bin").resolve()}"',
            f".org {CALLEE_AT - 0x100:#x}", "callee:", f'.incbin "{(directory / "callee.bin").resolve()}"',
            f".org {STUB_AT - 0x100:#x}", "stub:", ".space 5",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            # loader-style relocation of both far-call segment words, then the loaded-state stub
            "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}",
            "    mov word ptr [caller0+0x8d],ax", "    mov word ptr [caller1+0x2a],ax",
            "    mov byte ptr [stub],0xea", "    mov word ptr [stub+1],offset callee",
            "    mov ax,cs", "    mov word ptr [stub+3],ax",
            # DS tables: 31FB + 16*i := (7*i) & FF for i < 128; 5582 + 16*i := i & F for i < 256
            "    xor cx,cx", "    mov di,0x31fb", "table1_loop:", "    mov ax,cx", "    mov bx,7", "    mul bx",
            "    mov byte ptr [di],al", "    add di,16", "    inc cx", "    cmp cx,128", "    jb table1_loop",
            "    xor cx,cx", "    mov di,0x5582", "table2_loop:", "    mov ax,cx", "    and al,0x0f",
            "    mov byte ptr [di],al", "    add di,16", "    inc cx", "    cmp cx,256", "    jb table2_loop",
            # header: segment, returned, caller offsets, callee, case count
            f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
            f"    mov word ptr [{HEADER + 4:#x}],offset caller0", f"    mov word ptr [{HEADER + 6:#x}],offset caller1",
            f"    mov word ptr [{HEADER + 8:#x}],offset callee", f"    mov word ptr [{HEADER + 10:#x}],{len(cases)}",
            "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
            "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,12",
            "    mov ah,0x40", "    int 0x21", "    jc failure",
            "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
            "    mov al,0xa5",
            f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
            f"    mov di,{RECORD:#x}", f"    mov cx,{RECORD_SIZE}", "    rep stosb",
            f"    mov di,{NODES:#x}", f"    mov cx,{NODE_AREA}", "    rep stosb",
            # fixture pointer: SI := fixtures + index * FIXTURE_SIZE
            "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures",
            "    mov si,ax", "    mov word ptr [case_ptr],si",
            # items at RECORD+17
            "    lea si,[si+2]", f"    mov di,{RECORD + 0x17:#x}", "    mov cx,21", "    rep movsb",
            # nodes: three entries of index,b0,b1,b2 placed at NODES + 40h*k (+2E, +3C..+3E)
            f"    mov di,{NODES:#x}", "    mov cx,3", "node_loop:", "    lodsb", "    mov byte ptr [di+0x2e],al",
            "    lodsb", "    mov byte ptr [di+0x3c],al", "    lodsb", "    mov byte ptr [di+0x3d],al",
            "    lodsb", "    mov byte ptr [di+0x3e],al", f"    add di,{NODE_STRIDE}", "    loop node_loop",
            # links: node k -> node k+1 while k+1 < count, else null; head -> node 0 when count > 0
            "    mov si,word ptr [case_ptr]", "    mov cl,byte ptr [si+1]", "    xor ch,ch",
            f"    mov word ptr [{RECORD + 0xC8:#x}],0", f"    mov word ptr [{RECORD + 0xCA:#x}],0",
            "    jcxz links_done", f"    mov word ptr [{RECORD + 0xC8:#x}],{NODES:#x}",
            f"    mov word ptr [{RECORD + 0xCA:#x}],cs",
            f"    mov di,{NODES:#x}", "    mov bx,1", "link_loop:", "    mov word ptr [di+0x2a],0",
            "    mov word ptr [di+0x2c],0", "    cmp bx,cx", "    jae links_done",
            f"    lea ax,[di+{NODE_STRIDE}]", "    mov word ptr [di+0x2a],ax", "    mov word ptr [di+0x2c],cs",
            f"    add di,{NODE_STRIDE}", "    inc bx", "    jmp link_loop", "links_done:",
            # choose the body before the flags are loaded, then registers and the far-call frame:
            # segment, offset, CS and a near CALL leave exactly what LCALL leaves
            "    mov si,word ptr [case_ptr]", "    mov ax,offset caller0", "    cmp byte ptr [si],0", "    je chosen",
            "    mov ax,offset caller1", "chosen:", "    mov word ptr [target],ax",
            "    mov bl,byte ptr [si+35]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
            "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    push cs", f"    push {RECORD:#x}", "    push cs", "    call word ptr [target]", "returned:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", "capture:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 20:#x}],ds", f"    mov word ptr [{OUTPUT + 22:#x}],es",
             f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             # SP is captured; park the stack above the frame window before the harness pushes anything
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{FRAME_LOW:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov si,{RECORD:#x}", f"    mov cx,{RECORD_SIZE}", "    rep movsb",
             f"    mov si,{NODES:#x}", f"    mov cx,{NODE_AREA}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0", "target: .word 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["which"], case["count"]] + case["items"]
        for index, bytes3 in case["nodes"]:
            row += [index] + bytes3
        row.append(case["flags"])
        check(len(row) == FIXTURE_SIZE, "fixture row size")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


def expected_ram(segment, case, returned, entry):
    """The RAM the C model starts from: the same primed regions as the native run."""
    memory = bytearray([0xA5]) * SIZE
    for i in range(128):
        memory[physical(segment, 0x31FB + 16 * i)] = table1(i)
    for i in range(256):
        memory[physical(segment, 0x5582 + 16 * i)] = table2(i)
    for i, value in enumerate(case["items"]):
        memory[physical(segment, RECORD + 0x17 + i)] = value
    for k, (index, bytes3) in enumerate(case["nodes"]):
        at = NODES + NODE_STRIDE * k
        memory[physical(segment, at + 0x2E)] = index
        for j, value in enumerate(bytes3):
            memory[physical(segment, at + 0x3C + j)] = value
        if k < case["count"]:  # the harness links only the nodes it walks; the rest keep the fill
            nxt = at + NODE_STRIDE if k + 1 < case["count"] else 0
            put16(memory, segment, at + 0x2A, nxt)
            put16(memory, segment, at + 0x2C, segment if nxt else 0)
    put16(memory, segment, RECORD + 0xC8, NODES if case["count"] else 0)
    put16(memory, segment, RECORD + 0xCA, segment if case["count"] else 0)
    sp = STACK_TOP - 8
    for delta, value in ((0, returned), (2, segment), (4, RECORD), (6, segment)):
        put16(memory, segment, sp + delta, value)
    cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"],
              ENTRY_REGS["di"], ENTRY_REGS["bp"], sp, segment, entry, segment, segment, segment, FLAGS[case["flags"]])
    return memory, cpu


def native_audit(root, bodies, callee, functions):
    directory = root / "resources/oracle-runs/u6a-0000-019c-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "caller0.bin").write_bytes(bodies["0000"]["body"])
    (directory / "caller1.bin").write_bytes(bodies["019c"]["body"])
    (directory / "callee.bin").write_bytes(callee)
    cases = fixtures()
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(assembly(directory, cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(com)], check=True)
    image = com.read_bytes()
    check(0x100 + len(image) < RECORD, "COM overlaps the primed data")
    for at, body in ((CALLER0, bodies["0000"]["body"]), (CALLER1, bodies["019c"]["body"]), (CALLEE_AT, callee)):
        check(image[at - 0x100:at - 0x100 + len(body)] == body, "copied original bytes differ in the image")
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
    check(len(result) == 12 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, caller0, caller1, callee_at, count = struct.unpack_from("<6H", result)
    check(count == len(cases) and caller0 == CALLER0 and caller1 == CALLER1 and callee_at == CALLEE_AT,
          "native header differs")
    # The image after relocation: both far-call segment words become CS + STUB_DELTA.
    for at in (CALLER0 + 0x8D, CALLER1 + 0x2A):
        check(struct.unpack_from("<H", image, at - 0x100)[0] == 0x0096, "unrelocated segment word differs")
    totals = {"0000": 0, "019c": 0}
    for index, case in enumerate(cases):
        record = result[12 + index * RECORD_BYTES:12 + (index + 1) * RECORD_BYTES]
        key = "0000" if case["which"] == 0 else "019c"
        entry = caller0 if case["which"] == 0 else caller1
        memory, cpu = expected_ram(segment, case, returned, entry)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        status = functions[key](ram, SIZE, ctypes.byref(cpu))
        check(status == 0, f"C status {status} on native case {index}")
        actual = bytes(ram)
        native_cpu = struct.unpack_from("<14H", record)
        for (name, _), want in zip(Cpu._fields_, native_cpu):
            got = getattr(cpu, name)
            check(got == want, f"native case {index} (+{key}) {name}: C={got:04x}, DOS={want:04x}")
        for offset, size, position in ((FRAME_LOW, FRAME_SIZE, 28), (RECORD, RECORD_SIZE, 28 + FRAME_SIZE),
                                       (NODES, NODE_AREA, 28 + FRAME_SIZE + RECORD_SIZE)):
            at = physical(segment, offset)
            if actual[at:at + size] != record[position:position + size]:
                diff = [i for i in range(size) if actual[at + i] != record[position + i]]
                raise ValueError(f"native case {index} (+{key}) RAM {offset:04x}+{diff[:8]} differs")
        totals[key] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: +0000 {totals['0000']}, +019C {totals['019c']}; "
          f"all 14 CPU words, {FRAME_SIZE} frame bytes, {RECORD_SIZE} record bytes and {NODE_AREA} node bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=totals, result_sha256=digest, relocated_stub_paragraph_delta=STUB_DELTA)
