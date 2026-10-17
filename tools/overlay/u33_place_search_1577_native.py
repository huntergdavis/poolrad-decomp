#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u33 +1434 and +1577 with their callees.

Each DOSBox run executes one synthetic COM whose image holds one generated
world at its real DS offsets (DS = CS = SS = the COM segment): the direction,
base, origin, availability, slot, class-weight and cell tables, the grid and
the +012B buffer. The image also holds the original unit 0x33 code (so every
PUSH CS/CALL displacement is the original), the unit 0x3BD span and the three
unit 0x3C6 spans, placed so that +0CB9 sits at offset 0CB9 of its own
paragraph. The three far-call segment words are relocated at start-up to one
stub paragraph by the loader's arithmetic. Per case the COM refills the frame
window, restores the availability and slot ranges from pristine copies, sets
the map, mode and arguments, enters +1434 or +1577 through PUSH CS/CALL with
interrupts disabled, and captures all 14 CPU words, the frame window and both
mutable DS ranges. Media and generated images stay under ignored resources/.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16
import u33_place_search_1577_audit as audit

MAIN_AT, FIXTURES_AT = 0x0320, 0x0E00
STUB_AT = 0x2600                    # stub paragraph = CS + 260h: 002A/002F (u3BD) and 0070 (u3C6) live here
STUB_DELTA = STUB_AT // 16
U3BD_AT, U33_AT, U3C6_AT = 0x2580, 0x8600, 0xA600   # COM offsets of each unit's code offset 0
U3C6_DELTA = U3C6_AT // 16
GRID, BUFFER = 0xB500, 0xBA00
RANGES = ((0x4440, 0x220), (0x5F20, 0x110))
PRISTINE = (0xBE00, 0xC020)
OUTPUT, HEADER = 0xC200, 0xC700
FRAME_LOW, FRAME_SIZE, STACK_TOP = 0xEF10, 0x100, 0xF000
FIXTURE_SIZE = 12
RECORD_BYTES = 28 + FRAME_SIZE + sum(n for _, n in RANGES)
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SITES = (0x0155 + 3, 0x0170 + 3, 0x150E + 3)       # far-call segment words in the unit 0x33 copy
IMAGE_END = 0xC800


def check(condition, message):
    if not condition:
        raise ValueError(message)


def worlds():
    rng = random.Random(0x331577A)
    near = {**{0x4440 + i: 1 for i in range(0x210)}, 0x5F2A: 4, 0x5F2E: 1, 0x5F32: 1, 0x5F36: 1, 0x5F3A: 1,
            **{0x2844 + j: 0 for j in range(0x40)}}
    closed = {0x4440 + i: 0 for i in range(0x210)}
    bases = {**{0x02F6 + d: 5 for d in range(8)}, **{0x02FE + d: 2 for d in range(8)}}
    probes = {**closed, **bases, 0x4656: 1, 0x4657: 1, 0x6AAD: 0xEC, 0x6AAE: 0, 0x4650: 0, 0x4651: 0,
              0x4652: 1, 0x4653: 1, 0x4654: 2, 0x4655: 2, 0x02F2: 0, 0x02F3: 2, 0x02F4: 3, 0x02F5: 6,
              **{0x02E2 + i: 0 for i in range(0x10)}}
    stuck = {**closed, **bases, 0x4656: 0, 0x4657: 0, 0x4654: 1, 0x4655: 1, 0x02F2: 3, 0x02F3: 8, 0x02F4: 3, 0x02F5: 8,
             **{0x02E2 + i: 0 for i in range(0x10)}}
    result = []
    for label, overrides, grid_fill in (("random-a", {}, None), ("random-b", {}, None), ("random-c", {}, None),
                                        ("claimable", {**near, **{0x60DB + j: 0 for j in range(50 * 25)}, 0x274C: 3}, 2),
                                        ("probes", probes, None), ("stuck", stuck, None)):
        world_rng = random.Random(rng.randrange(1 << 30))
        data, _ = audit.world(world_rng, realistic=True)
        data.update({k & 0xFFFF: v & 0xFF for k, v in overrides.items()})
        grid = bytes(grid_fill if grid_fill is not None else world_rng.choice((0x00, 0x1E, 0x02, 0x05, 0x07, 0x80, 0x01, 0x03))
                     for _ in range(0x4E9))
        buffer = bytes(world_rng.randrange(256) for _ in range(0x400))
        result.append(dict(label=label, data=data, grid=grid, buffer=buffer, count=max(1, data.get(0x5F2A, 1))))
    return result


def fixtures(world, index):
    rng = random.Random(0x1577 + index)
    cases = []
    for i in range(12):
        which = 0 if i < 4 else 1
        q = rng.randrange(1, world["count"] + 1)
        if which == 0:
            args = (rng.randrange(4), rng.randrange(0, 2), rng.randrange(0, 2), rng.randrange(-1, 7), rng.randrange(-1, 12), q)
        else:
            args = (0, 0, 0, 0, 0, q)
        cases.append(dict(which=which, map=rng.choice((0, 1)) if world["label"] not in ("probes",) else 0,
                          mode=rng.choice((0, 1, 2)) if world["label"] not in ("probes", "stuck") else (0 if world["label"] == "probes" else 2),
                          args=[a & 0xFF for a in args], high=rng.randrange(256), flags=i % len(FLAGS)))
    return cases


def assembly(cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            # loader-style relocation of the three far-call segment words to the stub paragraph
            "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}"]
    text += [f"    mov word ptr [{U33_AT + site:#x}],ax" for site in SITES]
    for offset, entry in ((0x2A, 0x047B), (0x2F, 0x0350)):
        text += [f"    mov byte ptr [{STUB_AT + offset:#x}],0xea", f"    mov word ptr [{STUB_AT + offset + 1:#x}],{U3BD_AT + entry:#x}",
                 "    mov ax,cs", f"    mov word ptr [{STUB_AT + offset + 3:#x}],ax"]
    text += [f"    mov byte ptr [{STUB_AT + 0x70:#x}],0xea", f"    mov word ptr [{STUB_AT + 0x71:#x}],0x0cb9",
             "    mov ax,cs", f"    add ax,{U3C6_DELTA:#x}", f"    mov word ptr [{STUB_AT + 0x73:#x}],ax",
             f"    mov word ptr [0x6716],{GRID:#x}", "    mov word ptr [0x6718],cs",
             f"    mov word ptr [0x6a5c],{BUFFER:#x}", "    mov word ptr [0x6a5e],cs",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned1434",
             f"    mov word ptr [{HEADER + 4:#x}],offset returned1577", f"    mov word ptr [{HEADER + 6:#x}],{len(cases)}",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb"]
    for (start, length), pristine in zip(RANGES, PRISTINE):
        text += [f"    mov si,{pristine:#x}", f"    mov di,{start:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures",
             "    mov si,ax",
             "    mov al,byte ptr [si+1]", "    mov byte ptr [0x4658],al",
             "    mov al,byte ptr [si+2]", "    mov byte ptr [0x49fa],al",
             "    mov ah,byte ptr [si+9]"]
    for k, name in enumerate(("w_r", "w_b8", "w_b10", "w_y", "w_x", "w_q")):
        text += [f"    mov al,byte ptr [si+{3 + k}]", f"    mov word ptr [{name}],ax"]
    text += ["    mov ax,offset path1434", f"    mov word ptr [target],{U33_AT + 0x1434:#x}",
             "    cmp byte ptr [si],0", "    je path_chosen",
             "    mov ax,offset path1577", f"    mov word ptr [target],{U33_AT + 0x1577:#x}",
             "path_chosen:", "    mov word ptr [path],ax",
             "    mov bl,byte ptr [si+10]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    jmp word ptr [path]",
             "path1434:", "    push word ptr [w_q]", "    push word ptr [w_x]", "    push word ptr [w_y]",
             "    push word ptr [w_b10]", "    push word ptr [w_b8]", "    push word ptr [w_r]",
             "    push cs", "    call word ptr [target]", "returned1434:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned1434", "    jmp capture",
             "path1577:", "    push word ptr [w_q]", "    push cs", "    call word ptr [target]", "returned1577:",
             f"    mov word ptr [{OUTPUT + 18:#x}],offset returned1577",
             "capture:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 20:#x}],ds", f"    mov word ptr [{OUTPUT + 22:#x}],es",
             f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{FRAME_LOW:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb"]
    for start, length in RANGES:
        text += [f"    mov si,{start:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "target: .word 0", "path: .word 0",
             "w_r: .word 0", "w_b8: .word 0", "w_b10: .word 0", "w_y: .word 0", "w_x: .word 0", "w_q: .word 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["which"], case["map"], case["mode"], *case["args"], case["high"], case["flags"], 0]
        check(len(row) == FIXTURE_SIZE, "fixture row size")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


def build_image(assembled, code, world):
    image = bytearray(IMAGE_END - 0x100)
    def place(at, blob):
        check(at >= 0x100 and at + len(blob) <= IMAGE_END, f"placement {at:#x} outside the image")
        image[at - 0x100:at - 0x100 + len(blob)] = blob
    # The assembled harness is JMP MAIN at 0100, zero padding up to MAIN_AT (where DS 02D2..0305 tables live),
    # then the code, variables and fixture rows.
    used = [(0x100, 0x103), (MAIN_AT, 0x100 + len(assembled))] if len(assembled) > 3 else [(0x100, 0x100 + len(assembled))]
    place(0x100, assembled)
    for at, blob in ((U33_AT, code[0x33]), (U3BD_AT + audit.U3BD_SPAN[0], code[0x3BD][audit.U3BD_SPAN[0]:audit.U3BD_SPAN[1]])):
        place(at, blob); used.append((at, at + len(blob)))
    for lo, hi in audit.U3C6_SPANS:
        place(U3C6_AT + lo, code[0x3C6][lo:hi]); used.append((U3C6_AT + lo, U3C6_AT + hi))
    for at, blob in ((GRID, world["grid"]), (BUFFER, world["buffer"])):
        place(at, blob); used.append((at, at + len(blob)))
    used += [(STUB_AT + 0x2A, STUB_AT + 0x34), (STUB_AT + 0x70, STUB_AT + 0x75), (OUTPUT, OUTPUT + RECORD_BYTES),
             (HEADER, HEADER + 8), (PRISTINE[0], PRISTINE[0] + RANGES[0][1]), (PRISTINE[1], PRISTINE[1] + RANGES[1][1])]
    for offset in world["data"]:
        check(not any(lo <= offset < hi for lo, hi in used), f"world byte DS:{offset:04X} collides with the harness")
        check(0x100 <= offset < IMAGE_END, f"world byte DS:{offset:04X} outside the image")
    globals_touched = {0x4658, 0x49FA, 0x6716, 0x6717, 0x6718, 0x6719, 0x6A5C, 0x6A5D, 0x6A5E, 0x6A5F}
    for lo, hi in used:
        check(not any(lo <= g < hi for g in globals_touched), f"harness range {lo:#x} covers a primed DS global")
    for offset, value in world["data"].items():
        image[offset - 0x100] = value
    for (start, length), pristine in zip(RANGES, PRISTINE):
        image[pristine - 0x100:pristine - 0x100 + length] = image[start - 0x100:start - 0x100 + length]
    return bytes(image)


def expected(segment, image, case, returned):
    memory = bytearray([0xA5]) * SIZE
    base = physical(segment, 0x100)
    memory[base:base + len(image)] = image
    for site in SITES:
        put16(memory, segment, U33_AT + site, (segment + STUB_DELTA) & 0xFFFF)
    for offset, entry in ((0x2A, 0x047B), (0x2F, 0x0350)):
        memory[physical(segment, STUB_AT + offset)] = 0xEA
        put16(memory, segment, STUB_AT + offset + 1, U3BD_AT + entry)
        put16(memory, segment, STUB_AT + offset + 3, segment)
    memory[physical(segment, STUB_AT + 0x70)] = 0xEA
    put16(memory, segment, STUB_AT + 0x71, 0x0CB9)
    put16(memory, segment, STUB_AT + 0x73, (segment + U3C6_DELTA) & 0xFFFF)
    put16(memory, segment, 0x6716, GRID); put16(memory, segment, 0x6718, segment)
    put16(memory, segment, 0x6A5C, BUFFER); put16(memory, segment, 0x6A5E, segment)
    memory[physical(segment, 0x4658)] = case["map"]
    memory[physical(segment, 0x49FA)] = case["mode"]
    words = [(case["high"] << 8) | v for v in case["args"]]      # r, b8, b10, y, x, q
    pushes = list(reversed(words)) if case["which"] == 0 else [words[5]]
    sp = STACK_TOP
    for value in pushes:
        sp -= 2; put16(memory, segment, sp, value)
    sp -= 2; put16(memory, segment, sp, segment)
    sp -= 2; put16(memory, segment, sp, returned)
    entry = U33_AT + (0x1434 if case["which"] == 0 else 0x1577)
    cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"],
              ENTRY_REGS["di"], ENTRY_REGS["bp"], sp, segment, entry, segment, segment, segment, FLAGS[case["flags"]])
    return memory, cpu


def run_world(root, code, functions, world, index):
    directory = root / f"resources/oracle-runs/u33-1577-synthetic/{world['label']}"
    directory.mkdir(parents=True, exist_ok=True)
    cases, probe_image = [], build_image(b"\xeb", code, world)
    for case in fixtures(world, index):                     # keep only cases whose search ends (no DOSBox hang)
        memory, cpu = expected(0x0814, probe_image, case, 0x1234)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        chain = ctypes.c_int(-1)
        status = (functions["1434"](ram, cpu, chain) if case["which"] == 0 else functions["1577"](ram, cpu, chain, 20000))
        if status == 0:
            cases.append(case)
    check(len(cases) >= 6, f"too few returning native cases for {world['label']}")
    asm, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm.write_text(assembly(cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) == FIXTURES_AT + FIXTURE_SIZE * len(cases), "assembled harness layout differs")
    check(0x100 + len(assembled) <= U3BD_AT + audit.U3BD_SPAN[0], "assembled harness reaches the unit 0x3BD span")
    image = build_image(assembled, code, world)
    com.write_bytes(image)
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
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete for {world['label']}: {len(result)} bytes")
    segment, returned1434, returned1577, count = struct.unpack_from("<4H", result)
    check(count == len(cases), "native header differs")
    totals = {"1434": 0, "1577": 0}
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        key = "1434" if case["which"] == 0 else "1577"
        memory, cpu = expected(segment, image, case, returned1434 if case["which"] == 0 else returned1577)
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        chain = ctypes.c_int(-1)
        status = functions[key](ram, cpu, chain)
        check(status == 0, f"C status {status} (callee {chain.value}) on native {world['label']} case {n} (+{key})")
        actual = bytes(ram)
        native_cpu = struct.unpack_from("<14H", record)
        for (name, _), want in zip(Cpu._fields_, native_cpu):
            got = getattr(cpu, name)
            check(got == want, f"native {world['label']} case {n} (+{key}) {name}: C={got:04x}, DOS={want:04x}")
        offset = 28
        for start, length in ((FRAME_LOW, FRAME_SIZE), *RANGES):
            at = physical(segment, start)
            if actual[at:at + length] != record[offset:offset + length]:
                diff = [i for i in range(length) if actual[at + i] != record[offset + i]]
                raise ValueError(f"native {world['label']} case {n} (+{key}) bytes at {start:#x}+{diff[:6]} differ")
            offset += length
        totals[key] += 1
    return totals, sha256(result).hexdigest(), segment


def native_audit(root, bodies, code, functions):
    del bodies
    totals, digests, segments = {"1434": 0, "1577": 0}, {}, set()
    for index, world in enumerate(worlds()):
        counts, digest, segment = run_world(root, code, functions, world, index)
        for key in totals:
            totals[key] += counts[key]
        digests[world["label"]] = digest
        segments.add(segment)
    combined = sha256("".join(digests[k] for k in sorted(digests)).encode()).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {totals}; all 14 CPU words, {FRAME_SIZE} frame bytes and the "
          f"availability and slot ranges; no mask; {len(digests)} worlds")
    print(f"native combined result SHA-256 {combined}; COM segment(s) {sorted(f'{s:04x}' for s in segments)}")
    return dict(cases=totals, worlds=len(digests), result_sha256=combined, per_world=digests,
                relocated_stub_paragraph_delta=STUB_DELTA)
