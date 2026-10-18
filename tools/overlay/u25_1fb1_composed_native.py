"""DOSBox-X synthetic oracle for the composed u25 +1FB1 (imported by u25_1fb1_composed_audit.py).

The COM carries the original unit 0x25 code and RES_00BA_2199's 111 bytes and copies
them to absolute paragraphs at start-up. The two fixup-backed far-call segment words
are relocated loader-style to the resident paragraph 8C10. For each case the harness
streams the audit fixture's DS head, character nodes, sidecar lists and outputs from
WORLDS.BIN into the same absolute segments, pushes the two far output pointers and
far-calls +1FB1 with IRQs masked. The record holds all 14 CPU words, 256 stack bytes
and every data region. Null heads (which read DOS-owned 0000:011C) and link-rewriting
worlds (which walk arbitrary memory) stay in the Unicorn differential.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CALLER_N, RES_N, DATA_DS, OUT_SEG = 0x8800, 0x8C10, 0x1900, 0x9400
SOURCE_DS = 0x1800
CHUNKS = ((DATA_DS, 0x5D90, 0x0010), (0x6000, 0x0000, 0x1200), (0x7000, 0x0000, 0x2000), (0x6800, 0x0000, 0x0040))
MAIN_AT, CASES_AT, UNIT_AT, RES_AT = 0x0300, 0x0C00, 0x1000, 0x3400
HEADER, STACK_TOP, FRAME_LOW, FRAME_SIZE = 0x0A00, 0xF000, 0xEF00, 0x100
RECORD_BYTES = 28 + FRAME_SIZE + sum(c[2] for c in CHUNKS)
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
CASE_SIZE = 10


def check(condition, message):
    if not condition:
        raise ValueError(message)


def blocks(code25, image):
    code = bytearray(code25)
    for site in (0x2004, 0x2027):
        struct.pack_into("<H", code, site + 3, RES_N)
    return [("unit25", CALLER_N, 0x0000, bytes(code), UNIT_AT), ("resident", RES_N, 0x2199, image[0x2D39:0x2DA8], RES_AT)]


def assembly(code_blocks, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for label, segment, offset, data, _ in code_blocks:
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}",
                 f"    mov cx,{len(data):#x}", "    rep movsb"]
    text += ["    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset worlds_name", "    mov ax,0x3d00", "    int 0x21", "    jc failure", "    mov word ptr [world_handle],ax",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:"]
    for segment, offset, length in CHUNKS:
        text += ["    mov bx,word ptr [world_handle]", f"    mov cx,{length:#x}", f"    mov dx,{offset:#x}",
                 f"    mov ax,{segment:#x}", "    push ds", "    mov ds,ax", "    mov ah,0x3f", "    int 0x21", "    pop ds",
                 "    jc failure", f"    cmp ax,{length:#x}", "    jne failure"]
    text += ["    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE:#x}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             "    push word ptr [si+6]", "    push word ptr [si+4]", "    push word ptr [si+2]", "    push word ptr [si]",
             "    mov bx,word ptr [si+8]", "    shl bx,1", "    push word ptr [flags_table+bx]",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[record+{2 * i}],{reg}")
    text += ["    mov word ptr cs:[record+18],offset returned", "    mov word ptr cs:[record+20],ds",
             "    mov word ptr cs:[record+22],es", "    mov word ptr cs:[record+24],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", "    mov word ptr cs:[record+26],ax",
             "    push cs", "    pop ds", "    cld",
             f"    mov ax,{OUT_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov si,offset record", "    mov cx,28", "    rep movsb",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE:#x}", "    rep movsb"]
    for segment, offset, length in CHUNKS:
        text += [f"    mov ax,{segment:#x}", "    mov ds,ax", f"    mov si,{offset:#x}", f"    mov cx,{length:#x}", "    rep movsb"]
    text += ["    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov ax,{OUT_SEG:#x}", "    push ds", "    mov ds,ax", "    xor dx,dx",
             f"    mov cx,{RECORD_BYTES}", "    mov ah,0x40", "    int 0x21", "    pop ds", "    jc failure",
             f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21",
             "    mov bx,word ptr [world_handle]", "    mov ah,0x3e", "    int 0x21",
             "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', 'worlds_name: .asciz "WORLDS.BIN"', "handle: .word 0", "world_handle: .word 0",
             "index: .word 0", f"target_far: .word 0x1fb1, {CALLER_N:#x}",
             "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS), "record: .fill 28, 1, 0",
             f".org {CASES_AT - 0x100:#x}", "cases:"]
    for n, case in enumerate(cases):
        (lo_seg, lo_off), (hi_seg, hi_off) = case["out"]
        text.append(f"    .word {lo_off:#x},{lo_seg:#x},{hi_off:#x},{hi_seg:#x},{n % len(FLAGS)}")
    for label, _, _, data, at in code_blocks:
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(data), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in data[i:i + 16]))
    return "\n".join(text) + "\n"


def native_cases():
    spec = [(f"random-{i}", dict(nodes=random.Random(100 + i).randrange(1, 9))) for i in range(30)]
    for n in (1, 3, 5):
        for pattern in ([0x27], [0x2A], [0x27, 0x2A], []):
            spec.append((f"keys-{n}-{len(pattern)}", dict(nodes=n, keysets=[pattern] * n)))
    spec += [(f"min-is-value-{i}", dict(nodes=1 + i, alias="min-is-value")) for i in range(4)]
    return spec


def native_audit(target, code25, image, function):
    from u25_1fb1_composed_audit import build
    del target
    directory = ROOT / "resources/oracle-runs/u25-1fb1-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x251FB10)
    worlds, cases = bytearray(), []
    for label, options in native_cases():
        memory, _, expect, _, out = build(code25, image, rng, **options)
        for k, (segment, offset, length) in enumerate(CHUNKS):
            at = physical(SOURCE_DS if k == 0 else segment, offset); worlds += memory[at:at + length]
        cases.append(dict(label=label, out=out, expect=expect))
    (directory / "WORLDS.BIN").write_bytes(bytes(worlds))
    code_blocks = blocks(code25, image)
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(code_blocks, cases))
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
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and 0x10000 + 16 * segment <= 16 * DATA_DS, "native header or COM placement differs")
    world_size = sum(c[2] for c in CHUNKS)
    semantic = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for _, seg, off, data, _ in code_blocks:
            at = physical(seg, off); memory[at:at + len(data)] = data
        offset = n * world_size
        for seg, off, length in CHUNKS:
            at = physical(seg, off); memory[at:at + length] = worlds[offset:offset + length]; offset += length
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        (lo_seg, lo_off), (hi_seg, hi_off) = case["out"]
        sp = STACK_TOP
        for value in (hi_seg, hi_off, lo_seg, lo_off, segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, 0x1FB1, DATA_DS, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(0)
        status = function(ram, SIZE, ctypes.byref(cpu), 300, 1000, ctypes.byref(callee))
        check(status == 0, f"C status {status} on native case {n} ({case['label']})")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} ({case['label']}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        offset = 28
        for seg, off, length in ((segment, FRAME_LOW, FRAME_SIZE),) + CHUNKS:
            at = physical(seg, off)
            if bytes(ram[at:at + length]) != record[offset:offset + length]:
                diff = [i for i in range(length) if ram[at + i] != record[offset + i]]
                raise ValueError(f"native case {n} ({case['label']}) bytes at {seg:04x}:{off + diff[0]:04x} differ ({len(diff)} bytes)")
            offset += length
        if case["expect"] is not None:
            lo, hi, _ = case["expect"]
            check((ram[physical(lo_seg, lo_off)], ram[physical(hi_seg, hi_off)]) == (lo, hi), f"native case {n} min/max")
            semantic += 1
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({semantic} with min/max checked); all 14 CPU words, "
          f"{FRAME_SIZE} stack bytes and {world_size} data bytes per case; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}")
    return dict(cases=len(cases), semantic=semantic, result_sha256=digest)
