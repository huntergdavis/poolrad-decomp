#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u2F +0131, u38 +2D38 and u33 +0000.

The merged entry-8 chain expects unit 0x9E entry 8 at offset 0DE3 of its
segment and the SYSTEM generator at 0AF8:11C0, which is how the game loads
them. The synthetic COM therefore keeps the original unit 0x9E span at
CS:0DE3, copies the original SYSTEM bytes to the absolute paragraph 0AF8 at
start-up, holds the three original caller spans elsewhere and relocates each
far call's segment word to one stub paragraph exactly as the loader's fixup
would, so every call still reaches stub offset 0048 or 004D. Each case primes
the frame, seed, list, record, latch bytes, grid and table, enters one body
through PUSH CS/CALL and captures the 14 CPU words plus the frame window, the
seed, the latch bytes and the grid. Media and images stay under resources/.
"""
import ctypes
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16

# Absolute COM offsets. The fixture table must stay clear of every DS global the bodies and the
# generator touch (the seed dword at 4428, the pointers at 5D92..5D98 and 6716, the latch bytes at
# 6818..681C, the map byte at 84DC) and of the absolute SYSTEM paragraph; see native_audit's checks.
U2F_AT, U38_AT, U33_AT, STUB_AT, DICE_AT, SYSTEM_COPY_AT, MAIN_AT, FIXTURES_AT = 0x0300, 0x0400, 0x0500, 0x0748, 0x0DE3, 0x0F00, 0x1000, 0x4500
STUB_DELTA = (STUB_AT - 0x48) // 16
SYSTEM_CS, SYSTEM_OFFSET, SYSTEM_SIZE = 0x0AF8, 0x11C0, 0x87
RECORD, SUB, NODES, NODE_STRIDE, SUBS, SUB_STRIDE, GRID, GRID_SIZE = 0x9000, 0x9200, 0x9400, 0x120, 0x9C00, 0x10, 0xA000, 0x4E9
OUTPUT, HEADER, RESULT_AT = 0xB000, 0xB800, 0xEFC0
FRAME_LOW, FRAME_SIZE, STACK_TOP = 0xEF80, 0x80, 0xF000
FIXTURE_SIZE = 20
RECORD_BYTES = 28 + FRAME_SIZE + 4 + 3 + GRID_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
BODY_AT = {0: U2F_AT, 1: U38_AT, 2: U33_AT}
KEYS = {0: "0131", 1: "2d38", 2: "0000"}
RELOCATED_WORDS = (U2F_AT + 0x2C + 3, U38_AT + 0x32 + 3, U33_AT + 0x4B + 3, U33_AT + 0xB7 + 3, U33_AT + 0xC6 + 3)
TABLE_MODS = (1, 3, 0)   # table selector: every cell eligible, one in three, none


def check(condition, message):
    if not condition:
        raise ValueError(message)


def fixtures():
    rng = random.Random(0x9E0DE3)
    cases = []
    for which in range(3):
        for i in range(80 if which < 2 else 60):
            count = rng.randrange(5)
            cases.append(dict(which=which, seed=rng.randrange(1 << 32), count=count,
                              subs=[rng.choice((0, 1, 5, 5, 0x7F, 0x80, 0xFF, rng.randrange(256))) for _ in range(4)],
                              present=rng.choice((1, 1, 0)), record_byte=rng.choice((0x56, 0x56, 0x55, rng.randrange(256))),
                              latch=[rng.randrange(256), rng.choice((0, 1, 8, 9, rng.randrange(256))), rng.randrange(256)],
                              map_id=rng.choice((5, 5, 10, 0)), grid_seed=rng.randrange(256),
                              table_sel=rng.choice((0, 0, 1, 2)) if which == 2 else 0, flags=i % len(FLAGS)))
    return cases


def grid_byte(seed, i):
    return (seed + 7 * i) & 0xFF


def table_bytes(sel):
    mod = TABLE_MODS[sel]
    out = bytearray([0x33]) * 0x400
    for b in range(256):
        out[4 * b] = 0x16 if (mod and b % mod == 0) else 0x17
    return bytes(out)


def assembly(directory, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {U2F_AT - 0x100:#x}", "u2f:", f'.incbin "{(directory / "u2f.bin").resolve()}"',
            f".org {U38_AT - 0x100:#x}", "u38:", f'.incbin "{(directory / "u38.bin").resolve()}"',
            f".org {U33_AT - 0x100:#x}", "u33:", f'.incbin "{(directory / "u33.bin").resolve()}"',
            f".org {STUB_AT - 0x100:#x}", "stubs:", ".space 10",
            f".org {DICE_AT - 0x100:#x}", "dice:", f'.incbin "{(directory / "dice.bin").resolve()}"',
            f".org {SYSTEM_COPY_AT - 0x100:#x}", "system_copy:", f'.incbin "{(directory / "system.bin").resolve()}"',
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            # loader-style relocation of the five far-call segment words
            "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}"]
    for at in RELOCATED_WORDS:
        text.append(f"    mov word ptr [{at:#x}],ax")
    text += [# loaded-state stubs for entries 8 and 9 at CS:0DE3 / CS:0E2E
             f"    mov byte ptr [{STUB_AT:#x}],0xea", f"    mov word ptr [{STUB_AT + 1:#x}],{DICE_AT:#x}",
             "    mov ax,cs", f"    mov word ptr [{STUB_AT + 3:#x}],ax",
             f"    mov byte ptr [{STUB_AT + 5:#x}],0xea", f"    mov word ptr [{STUB_AT + 6:#x}],{DICE_AT + 0x4B:#x}",
             f"    mov word ptr [{STUB_AT + 8:#x}],ax",
             # the SYSTEM generator at its absolute paragraph
             f"    mov ax,{SYSTEM_CS:#x}", "    mov es,ax", f"    mov di,{SYSTEM_OFFSET:#x}", "    mov si,offset system_copy",
             f"    mov cx,{SYSTEM_SIZE}", "    cld", "    rep movsb", "    push cs", "    pop es",
             # fixed far pointers
             f"    mov word ptr [0x5d92],{RECORD:#x}", "    mov word ptr [0x5d94],cs",
             f"    mov word ptr [0x6716],{GRID:#x}", "    mov word ptr [0x6718],cs",
             # header: segment, returned0..2, count
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned0",
             f"    mov word ptr [{HEADER + 4:#x}],offset returned1", f"    mov word ptr [{HEADER + 6:#x}],offset returned2",
             f"    mov word ptr [{HEADER + 8:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 10:#x}],offset main_end",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,12",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             f"    mov di,{RECORD:#x}", "    mov cx,0x1000", "    rep stosb",          # record, sub, nodes, subs area
             "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures",
             "    mov si,ax", "    mov word ptr [case_ptr],si",
             # seed dword, latch bytes, map
             "    mov ax,word ptr [si+1]", "    mov word ptr [0x4428],ax", "    mov ax,word ptr [si+3]", "    mov word ptr [0x442a],ax",
             "    mov al,byte ptr [si+12]", "    mov byte ptr [0x6818],al", "    mov al,byte ptr [si+13]", "    mov byte ptr [0x6819],al",
             "    mov al,byte ptr [si+14]", "    mov byte ptr [0x681c],al", "    mov al,byte ptr [si+15]", "    mov byte ptr [0x84dc],al",
             # record +CC/+CE and the sub record byte +2E
             f"    mov word ptr [{RECORD + 0xCC:#x}],0", f"    mov word ptr [{RECORD + 0xCE:#x}],0",
             "    cmp byte ptr [si+10],0", "    je record_done",
             f"    mov word ptr [{RECORD + 0xCC:#x}],{SUB:#x}", f"    mov word ptr [{RECORD + 0xCE:#x}],cs",
             "record_done:", "    mov al,byte ptr [si+11]", f"    mov byte ptr [{SUB + 0x2E:#x}],al",
             # party list: head, four nodes with links and sub pointers, sub bytes +3
             "    mov cl,byte ptr [si+5]", "    xor ch,ch", "    mov word ptr [0x5d96],0", "    mov word ptr [0x5d98],0",
             "    jcxz nodes_done", f"    mov word ptr [0x5d96],{NODES:#x}", "    mov word ptr [0x5d98],cs",
             f"    mov di,{NODES:#x}", f"    mov dx,{SUBS:#x}", "    mov bx,1", "    lea bp,[si+6]",
             "node_loop:", "    mov word ptr [di+0x104],0", "    mov word ptr [di+0x106],0",
             "    mov word ptr [di+0x108],dx", "    mov word ptr [di+0x10a],cs",
             "    push di", "    mov di,dx", "    mov al,byte ptr [bp]", "    mov byte ptr [di+3],al", "    pop di", "    inc bp",
             "    cmp bx,cx", "    jae nodes_done",
             f"    lea ax,[di+{NODE_STRIDE:#x}]", "    mov word ptr [di+0x104],ax", "    mov word ptr [di+0x106],cs",
             f"    add di,{NODE_STRIDE:#x}", f"    add dx,{SUB_STRIDE}", "    inc bx", "    jmp node_loop",
             "nodes_done:",
             # grid bytes and the four-stride table
             "    mov al,byte ptr [si+16]", f"    mov di,{GRID:#x}", f"    mov cx,{GRID_SIZE}", "grid_loop:",
             "    mov byte ptr [di],al", "    add al,7", "    inc di", "    loop grid_loop",
             "    mov bl,byte ptr [si+17]", "    xor bh,bh", "    mov bl,byte ptr [table_mods+bx]",   # bl := mod
             "    mov di,0x2747", "    mov cx,0x400", "    mov al,0x33", "    rep stosb",
             "    xor cx,cx", "    mov di,0x2747", "table_loop:", "    mov byte ptr [di],0x17",
             "    test bl,bl", "    jz table_next", "    mov ax,cx", "    div bl", "    test ah,ah", "    jnz table_next",
             "    mov byte ptr [di],0x16", "table_next:", "    add di,4", "    inc cx", "    cmp cx,256", "    jb table_loop",
             # body, path and flags
             "    mov si,word ptr [case_ptr]", "    mov al,byte ptr [si]", "    xor ah,ah", "    shl ax,1", "    mov bx,ax",
             "    mov ax,word ptr [body_table+bx]", "    mov word ptr [target],ax",
             "    mov ax,word ptr [path_table+bx]", "    mov word ptr [path],ax",
             "    mov bl,byte ptr [si+18]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
             "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    jmp word ptr [path]",
             "path0:", "    push cs", f"    push {RESULT_AT:#x}", "    push cs", "    call word ptr [target]", "returned0:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned0", "    jmp capture",
             "path1:", "    push 0x1111", "    push 0x2222", "    push 0x3333", "    push 0x4444", "    push 0x5555",
             "    push cs", "    call word ptr [target]", "returned1:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned1", "    jmp capture",
             "path2:", "    push cs", "    call word ptr [target]", "returned2:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned2",
             "capture:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 20:#x}],ds", f"    mov word ptr [{OUTPUT + 22:#x}],es",
             f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{FRAME_LOW:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             "    mov si,0x4428", "    mov cx,4", "    rep movsb",
             "    mov si,0x6818", "    movsb", "    movsb", "    mov si,0x681c", "    movsb",
             f"    mov si,{GRID:#x}", f"    mov cx,{GRID_SIZE}", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "case_ptr: .word 0",
             "target: .word 0", "path: .word 0",
             "body_table: .word " + ",".join(f"{BODY_AT[i]:#x}" for i in range(3)),
             "path_table: .word offset path0, offset path1, offset path2",
             "table_mods: .byte " + ",".join(str(m) for m in TABLE_MODS),
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS), "main_end:",
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["which"]] + list(struct.pack("<I", case["seed"])) + [case["count"]] + case["subs"] + \
              [case["present"], case["record_byte"]] + case["latch"] + [case["map_id"], case["grid_seed"], case["table_sel"], case["flags"], 0]
        check(len(row) == FIXTURE_SIZE, f"fixture row size {len(row)}")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


def expected_ram(segment, image, system, case, returned):
    memory = bytearray([0xA5]) * SIZE
    base = physical(segment, 0x100)
    memory[base:base + len(image)] = image
    for at in RELOCATED_WORDS:
        put16(memory, segment, at, (segment + STUB_DELTA) & 0xFFFF)
    for offset, body in ((STUB_AT, DICE_AT), (STUB_AT + 5, DICE_AT + 0x4B)):
        memory[physical(segment, offset)] = 0xEA
        put16(memory, segment, offset + 1, body)
        put16(memory, segment, offset + 3, segment)
    at = physical(SYSTEM_CS, SYSTEM_OFFSET)
    memory[at:at + SYSTEM_SIZE] = system
    put16(memory, segment, 0x5D92, RECORD); put16(memory, segment, 0x5D94, segment)
    put16(memory, segment, 0x6716, GRID); put16(memory, segment, 0x6718, segment)
    at = physical(segment, RECORD)
    memory[at:at + 0x1000] = bytes([0xA5]) * 0x1000
    put16(memory, segment, 0x4428, case["seed"] & 0xFFFF); put16(memory, segment, 0x442A, case["seed"] >> 16)
    memory[physical(segment, 0x6818)] = case["latch"][0]
    memory[physical(segment, 0x6819)] = case["latch"][1]
    memory[physical(segment, 0x681C)] = case["latch"][2]
    memory[physical(segment, 0x84DC)] = case["map_id"]
    if case["present"]:
        put16(memory, segment, RECORD + 0xCC, SUB); put16(memory, segment, RECORD + 0xCE, segment)
    else:
        put16(memory, segment, RECORD + 0xCC, 0); put16(memory, segment, RECORD + 0xCE, 0)
    memory[physical(segment, SUB + 0x2E)] = case["record_byte"]
    count = case["count"]
    put16(memory, segment, 0x5D96, NODES if count else 0); put16(memory, segment, 0x5D98, segment if count else 0)
    for k in range(count):
        node = NODES + k * NODE_STRIDE
        nxt = node + NODE_STRIDE if k + 1 < count else 0
        put16(memory, segment, node + 0x104, nxt); put16(memory, segment, node + 0x106, segment if nxt else 0)
        put16(memory, segment, node + 0x108, SUBS + k * SUB_STRIDE); put16(memory, segment, node + 0x10A, segment)
        memory[physical(segment, SUBS + k * SUB_STRIDE + 3)] = case["subs"][k]
    for i in range(GRID_SIZE):
        memory[physical(segment, GRID + i)] = grid_byte(case["grid_seed"], i)
    at = physical(segment, 0x2747)
    memory[at:at + 0x400] = table_bytes(case["table_sel"])
    words = {0: [segment, RESULT_AT], 1: [0x1111, 0x2222, 0x3333, 0x4444, 0x5555], 2: []}[case["which"]]
    sp = STACK_TOP
    for value in words:
        sp -= 2; put16(memory, segment, sp, value)
    sp -= 2; put16(memory, segment, sp, segment)
    sp -= 2; put16(memory, segment, sp, returned)
    cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"],
              ENTRY_REGS["di"], ENTRY_REGS["bp"], sp, segment, BODY_AT[case["which"]], segment, segment, segment,
              FLAGS[case["flags"]])
    return memory, cpu


def native_audit(root, bodies, code, functions):
    directory = root / "resources/oracle-runs/u33-0000-dice-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "u2f.bin").write_bytes(bodies["0131"]["body"])
    (directory / "u38.bin").write_bytes(bodies["2d38"]["body"])
    (directory / "u33.bin").write_bytes(bodies["0000"]["body"])
    (directory / "dice.bin").write_bytes(code["dice"])
    (directory / "system.bin").write_bytes(code["system"])
    check(len(code["system"]) == SYSTEM_SIZE, "SYSTEM span size")
    cases = fixtures()
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(assembly(directory, cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(com)], check=True)
    image = com.read_bytes()
    check(0x100 + len(image) < RECORD, "COM overlaps the primed data")
    for at, body in ((U2F_AT, bodies["0131"]["body"]), (U38_AT, bodies["2d38"]["body"]), (U33_AT, bodies["0000"]["body"]),
                     (DICE_AT, code["dice"]), (SYSTEM_COPY_AT, code["system"])):
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 12 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned0, returned1, returned2, count, main_end = struct.unpack_from("<6H", result)
    check(count == len(cases), "native header differs")
    hole = (SYSTEM_CS << 4) + SYSTEM_OFFSET - (segment << 4)
    check(main_end <= hole and hole + SYSTEM_SIZE <= FIXTURES_AT, f"absolute SYSTEM paragraph lands on used COM offset {hole:#x}")
    fixtures_end = FIXTURES_AT + len(cases) * FIXTURE_SIZE
    for low, high, name in ((0x2747, 0x2B47, "table"), (0x4428, 0x442C, "seed"), (0x5D92, 0x5D9A, "pointers"),
                            (0x6716, 0x671A, "grid pointer"), (0x6818, 0x681D, "latch"), (0x84DC, 0x84DD, "map"), (RECORD, STACK_TOP, "data")):
        check(fixtures_end <= low or FIXTURES_AT >= high, f"fixture table overlaps the {name} bytes")
        check(main_end <= low or MAIN_AT >= high, f"harness code overlaps the {name} bytes")
    returned = {0: returned0, 1: returned1, 2: returned2}
    totals = {k: 0 for k in KEYS.values()}
    for index, case in enumerate(cases):
        record = result[12 + index * RECORD_BYTES:12 + (index + 1) * RECORD_BYTES]
        key = KEYS[case["which"]]
        memory, cpu = expected_ram(segment, image, code["system"], case, returned[case["which"]])
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
        for offset, size in ((FRAME_LOW, FRAME_SIZE), (0x4428, 4), (0x6818, 2), (0x681C, 1), (GRID, GRID_SIZE)):
            at = physical(segment, offset)
            if actual[at:at + size] != record[position:position + size]:
                diff = [i for i in range(size) if actual[at + i] != record[position + i]]
                raise ValueError(f"native case {index} (+{key}) RAM {offset:04x}+{diff[:8]} differs")
            position += size
        totals[key] += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {totals}; all 14 CPU words, {FRAME_SIZE} frame bytes, "
          f"the seed, the three latch bytes and {GRID_SIZE} grid bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM at absolute 0AF8:11C0 (COM offset {hole:#x})")
    return dict(cases=totals, result_sha256=digest, relocated_stub_paragraph_delta=STUB_DELTA, system_com_offset=hole)
