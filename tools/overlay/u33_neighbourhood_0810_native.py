#!/usr/bin/env python3
"""IRQ-isolated DOSBox-X synthetic oracle for u33 +0810 with every callee.

The COM (DS = SS = its segment) bakes the DS tables, the +012B buffer and the
eligibility table at their real offsets. The original unit 0x33 code sits in
its own paragraph so +0810, +040F and +0615 run at their original offsets;
the unit 0x9E span sits in a paragraph that puts entry 8 at 0DE3; the unit
0x3BD span sits in the COM segment. At start-up the COM relocates the five
far-call segment words (+012B twice, +0000 three times) to one stub paragraph
by the loader's arithmetic, writes the three far-jump stubs and copies the
original SYSTEM generator to the absolute paragraph 0AF8. Per case it refills
the frame window, the grid and the seed, sets the party position and map,
far-calls +0810 with interrupts disabled and captures all 14 CPU words, the
frame window, the grid and the seed. Media and images stay under resources/.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, SIZE, physical, put16
import u33_neighbourhood_0810_audit as audit

MAIN_AT, FIXTURES_AT = 0x0300, 0x0A00
U3BD_AT = 0x0B00                           # unit 0x3BD offset 0 (span 031F..0579 lands at 0E1F)
STUB_AT = 0x1100                           # stub paragraph = CS + 110h: 002A/002F (u3BD), 0048/004D (u9E)
STUB_DELTA = STUB_AT // 16
U9E_AT = 0x1000                            # unit 0x9E offset 0: entry 8 at COM 1DE3
U9E_DELTA = U9E_AT // 16
GRID, BUFFER = 0x1E60, 0x2B50
U33_AT = 0x8600                            # unit 0x33 offset 0, its own paragraph
U33_DELTA = U33_AT // 16
OUTPUT, HEADER = 0xA800, 0xAF00
FRAME_LOW, FRAME_SIZE, STACK_TOP = 0xEF00, 0x100, 0xF000
SYSTEM_HOLE = (0x4000, 0x4090)             # 0AF8:11C0 = linear C140 = COM offset 4000 at segment 0814
GRID_BYTES = 0x4F0
FIXTURE_SIZE = 8
RECORD_BYTES = 28 + FRAME_SIZE + GRID_BYTES + 4
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SITES = (0x0155 + 3, 0x0170 + 3, 0x004B + 3, 0x00B7 + 3, 0x00C6 + 3)
EXPECTED_SEGMENT = 0x0814


def check(condition, message):
    if not condition:
        raise ValueError(message)


def world():
    rng = random.Random(0x330810A)
    data = {}
    for i in range(0x10A):
        data[0x2736 + i] = rng.randrange(256)
    for i in range(0x100):                     # cells holding 17h, 06h or 0Bh after a stamp are eligible
        data[0x2747 + 4 * i] = 0x16 if i in (0x17, 0x06, 0x0B, 0x05) or rng.random() < 0.05 else rng.choice((0x00, 0x15, 0x33))
    buffer = bytes(rng.randrange(256) for _ in range(0x400))
    return data, buffer


def cases():
    rng = random.Random(0x3308108)
    result = []
    for i in range(24):
        party = (rng.randrange(-3, 14), rng.randrange(-3, 9))
        result.append(dict(px=party[0] & 0xFF, py=party[1] & 0xFF, map=rng.choice((0, 5, 10)), grid_seed=rng.randrange(256),
                           seed=rng.randrange(1 << 32), flags=i % len(FLAGS)))
    return result


def assembly(cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}",
            "    mov ax,cs", f"    add ax,{U33_DELTA:#x}", "    mov word ptr [target_far+2],ax",
            "    mov es,ax", "    mov ax,cs", f"    add ax,{STUB_DELTA:#x}"]
    text += [f"    mov word ptr es:[{site:#x}],ax" for site in SITES]        # relocate inside the u33 paragraph
    text += ["    push cs", "    pop es"]
    for offset, entry, base in ((0x2A, 0x047B, U3BD_AT), (0x2F, 0x0350, U3BD_AT), (0x48, 0x0DE3, None), (0x4D, 0x0E2E, None)):
        text += [f"    mov byte ptr [{STUB_AT + offset:#x}],0xea"]
        if base is None:
            text += [f"    mov word ptr [{STUB_AT + offset + 1:#x}],{entry:#x}", "    mov ax,cs", f"    add ax,{U9E_DELTA:#x}"]
        else:
            text += [f"    mov word ptr [{STUB_AT + offset + 1:#x}],{base + entry:#x}", "    mov ax,cs"]
        text += [f"    mov word ptr [{STUB_AT + offset + 3:#x}],ax"]
    text += [f"    mov word ptr [0x6716],{GRID:#x}", "    mov word ptr [0x6718],cs",
             f"    mov word ptr [0x6a5c],{BUFFER:#x}", "    mov word ptr [0x6a5e],cs",
             # copy the SYSTEM generator to the absolute paragraph 0AF8
             "    mov ax,0x0af8", "    mov es,ax", "    mov si,offset system_bytes", "    mov di,0x11c0",
             f"    mov cx,{len(audit_system())}", "    cld", "    rep movsb", "    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{FIXTURE_SIZE}", "    mul bx", "    add ax,offset fixtures", "    mov si,ax",
             "    mov al,byte ptr [si]", "    mov byte ptr [0x6aad],al", "    mov al,byte ptr [si+1]", "    mov byte ptr [0x6aae],al",
             "    mov al,byte ptr [si+2]", "    mov byte ptr [0x84dc],al",
             "    mov al,byte ptr [si+3]", f"    mov di,{GRID:#x}", f"    mov cx,{GRID_BYTES:#x}", "grid_loop:",
             "    mov byte ptr [di],al", "    add al,13", "    inc di", "    loop grid_loop",
             "    mov ax,word ptr [si+4]", "    mov word ptr [0x4428],ax", "    mov ax,word ptr [si+6]", "    mov word ptr [0x442a],ax",
             "    mov ax,word ptr [index]", "    mov bl,6", "    div bl", "    mov bl,ah", "    xor bh,bh", "    shl bx,1",
             "    push word ptr [flags_table+bx]", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr [target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr [{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr [{OUTPUT + 18:#x}],offset returned", f"    mov word ptr [{OUTPUT + 20:#x}],ds",
             f"    mov word ptr [{OUTPUT + 22:#x}],es", f"    mov word ptr [{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr [{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             f"    mov si,{FRAME_LOW:#x}", f"    mov di,{OUTPUT + 28:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov si,{GRID:#x}", f"    mov cx,{GRID_BYTES:#x}", "    rep movsb",
             "    mov si,0x4428", "    mov cx,4", "    rep movsb",
             "    sti", "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0",
             "target_far: .word 0x0810, 0",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS),
             "system_bytes: .byte " + ",".join(str(b) for b in audit_system()),
             f".org {FIXTURES_AT - 0x100:#x}", "fixtures:"]
    for case in cases:
        row = [case["px"], case["py"], case["map"], case["grid_seed"], *struct.pack("<I", case["seed"])]
        check(len(row) == FIXTURE_SIZE, "fixture row size")
        text.append("    .byte " + ",".join(str(v) for v in row))
    return "\n".join(text) + "\n"


_SYSTEM = None


def audit_system():
    return _SYSTEM


def build_image(assembled, code, data, buffer):
    image = bytearray(U33_AT + len(code[0x33]) - 0x100)
    used = []
    def place(at, blob, name):
        check(at >= 0x100 and at + len(blob) <= 0x100 + len(image), f"{name} outside the image")
        image[at - 0x100:at - 0x100 + len(blob)] = blob; used.append((at, at + len(blob), name))
    place(0x100, assembled[:3], "jmp")
    place(MAIN_AT, assembled[MAIN_AT - 0x100:], "harness")
    place(U3BD_AT + audit.U3BD_SPAN[0], code[0x3BD][audit.U3BD_SPAN[0]:audit.U3BD_SPAN[1]], "u3BD span")
    place(U9E_AT + audit.U9E_SPAN[0], code[0x9E][audit.U9E_SPAN[0]:audit.U9E_SPAN[1]], "u9E span")
    place(BUFFER, buffer, "buffer")
    place(U33_AT, code[0x33], "u33 code")
    reserved = used + [(STUB_AT + 0x2A, STUB_AT + 0x52, "stubs"), (GRID, GRID + GRID_BYTES, "grid"),
                       (SYSTEM_HOLE[0], SYSTEM_HOLE[1], "SYSTEM hole"), (0x4428, 0x442C, "seed"),
                       (0x6716, 0x671A, "grid pointer"), (0x6A5C, 0x6A60, "buffer pointer"), (0x6AAD, 0x6AAF, "party"),
                       (0x84DC, 0x84DD, "map")]
    for i, (lo, hi, name) in enumerate(reserved):
        for lo2, hi2, name2 in reserved[i + 1:]:
            check(hi <= lo2 or hi2 <= lo, f"{name} overlaps {name2}")
    for offset, value in data.items():
        check(not any(lo <= offset < hi for lo, hi, _ in reserved), f"table byte DS:{offset:04X} collides")
        image[offset - 0x100] = value
    return bytes(image)


def expected(segment, image, case):
    memory = bytearray([0xA5]) * SIZE
    base = physical(segment, 0x100)
    memory[base:base + len(image)] = image
    u33 = (segment + U33_DELTA) & 0xFFFF
    for site in SITES:
        put16(memory, u33, site, (segment + STUB_DELTA) & 0xFFFF)
    for offset, entry, base_at in ((0x2A, 0x047B, U3BD_AT), (0x2F, 0x0350, U3BD_AT), (0x48, 0x0DE3, None), (0x4D, 0x0E2E, None)):
        memory[physical(segment, STUB_AT + offset)] = 0xEA
        put16(memory, segment, STUB_AT + offset + 1, entry if base_at is None else base_at + entry)
        put16(memory, segment, STUB_AT + offset + 3, (segment + U9E_DELTA) & 0xFFFF if base_at is None else segment)
    put16(memory, segment, 0x6716, GRID); put16(memory, segment, 0x6718, segment)
    put16(memory, segment, 0x6A5C, BUFFER); put16(memory, segment, 0x6A5E, segment)
    system = audit_system()
    memory[physical(0x0AF8, 0x11C0):physical(0x0AF8, 0x11C0) + len(system)] = system
    memory[physical(segment, 0x6AAD)] = case["px"]; memory[physical(segment, 0x6AAE)] = case["py"]
    memory[physical(segment, 0x84DC)] = case["map"]
    for i in range(GRID_BYTES):
        memory[physical(segment, GRID + i)] = (case["grid_seed"] + 13 * i) & 0xFF
    put16(memory, segment, 0x4428, case["seed"] & 0xFFFF); put16(memory, segment, 0x442A, case["seed"] >> 16)
    return memory


def native_audit(root, target, code, function):
    global _SYSTEM
    _SYSTEM = code["system"]
    directory = root / "resources/oracle-runs/u33-0810-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    data, buffer = world()
    all_cases = cases()
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(all_cases))
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "main.bin")], check=True)
    assembled = (directory / "main.bin").read_bytes()
    check(0x100 + len(assembled) <= U3BD_AT + audit.U3BD_SPAN[0], "harness reaches the unit 0x3BD span")
    image = build_image(assembled, code, data, buffer)
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=300)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(all_cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(all_cases) and segment == EXPECTED_SEGMENT, f"native header differs (segment {segment:04x})")
    passed = 0
    for n, case in enumerate(all_cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = expected(segment, image, case)
        sp = STACK_TOP
        for value in (segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(ENTRY_REGS["ax"], ENTRY_REGS["bx"], ENTRY_REGS["cx"], ENTRY_REGS["dx"], ENTRY_REGS["si"], ENTRY_REGS["di"],
                  ENTRY_REGS["bp"], sp, (segment + U33_DELTA) & 0xFFFF, 0x0810, segment, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = function.argtypes[3]._type_()               # the struct class bound to the loaded function
        status = function(ram, SIZE, ctypes.byref(cpu), ctypes.byref(callee))
        check(status == 0, f"C status {status} (site +{callee.site:04X}, {callee.status}) on native case {n}")
        actual = bytes(ram)
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for start, length in ((FRAME_LOW, FRAME_SIZE), (GRID, GRID_BYTES), (0x4428, 4)):
            at = physical(segment, start)
            if actual[at:at + length] != record[offset:offset + length]:
                diff = [i for i in range(length) if actual[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} bytes at {start:#x}+{diff[:6]} differ")
            offset += length
        passed += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {passed}; all 14 CPU words, {FRAME_SIZE} frame bytes, "
          f"{GRID_BYTES} grid bytes and the seed; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM copied to absolute 0AF8:11C0")
    return dict(cases=passed, result_sha256=digest, com_segment=f"{segment:04x}")
