"""DOSBox-X synthetic oracle for u86 +07D6 (imported by u86_roll_value_07d6_audit.py).

At start-up the COM copies the original SYSTEM generator bytes to absolute 0AF8:11C0
(COM offset 4000h at segment 0814, kept free), the unit 0x9E +0DE3..+0E51 span to
8000:0DE3, a loaded-form stub EA E3 0D 00 80 to 7F00:0048, RES_00BA_26C2's 103 bytes
to 9000:26C2, and the unit 0x86 code to 8800 with its six fixup-backed far-call
segment words relocated loader-style (dice sites to 7F00, +0880 to 9000). Each case
writes the k-th table entry, DS:49F3, the seed DS:4428, DS:6DBF and two record bytes
in data segment 1900, then far-calls +07D6 with IRQs masked. MUL's flags are compared
unmasked here.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CALLER_N, DICE_N, STUB_N, RES_N, DATA_DS = 0x8800, 0x8000, 0x7F00, 0x9000, 0x1900
MAIN_AT, RES_BLOB, U9E_BLOB, SYS_BLOB, CASES_AT, CODE_AT = 0x0300, 0x0C00, 0x0C80, 0x0D00, 0x0E00, 0x4100
SYSTEM_HOLE = (0x4000, 0x4090)
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFC0, 0x40
CASE_SIZE = 28
RECORD_BYTES = 28 + FRAME_SIZE + 16 + 1 + 4 + 1 + 2
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)
SITES = (0x07E9, 0x0809, 0x0820, 0x0846, 0x085D)
SYSTEM_FILE = (0xBE36, 0xBEBD)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def native_cases():
    rng = random.Random(0x8607D7)
    cases = []
    for i in range(120):
        k = (0x28, 0x39, 0x3B, 0x3D, 0x3F, 0x3F, 0x43, rng.randrange(256))[i % 8]
        entry = [rng.randrange(256) for _ in range(16)]
        entry[0] = rng.choice((0, 1, 2, 3))
        cases.append(dict(k=k, mode=(5, 0)[(i // 8) % 2], seed=rng.randrange(1 << 32), entry=entry, flag=rng.choice((0, 1)),
                          rec96=rng.randrange(256), rec9b=rng.randrange(256), high=rng.randrange(256)))
    return cases


def assembly(unit, res, span, system, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (RES_N, 0x26C2, "resident", len(res)),
                                           (DICE_N, 0x0DE3, "dice", len(span)), (0x0AF8, 0x11C0, "system", len(system))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}",
                 f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{STUB_N:#x}", "    mov es,ax", "    mov byte ptr es:[0x48],0xea", "    mov word ptr es:[0x49],0x0de3",
             f"    mov word ptr es:[0x4b],{DICE_N:#x}",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov word ptr es:[0x5d92],0x0100", "    mov word ptr es:[0x5d94],0x6000",
             "    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             "    mov al,byte ptr [si]", "    xor ah,ah", "    shl ax,4", "    add ax,0x31fa", "    mov word ptr [entry_off],ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax", "    mov di,word ptr [entry_off]", "    push si", "    add si,6", "    mov cx,16",
             "    rep movsb", "    pop si",
             "    mov al,byte ptr [si+1]", "    mov byte ptr es:[0x49f3],al",
             "    mov ax,word ptr [si+2]", "    mov word ptr es:[0x4428],ax", "    mov ax,word ptr [si+4]", "    mov word ptr es:[0x442a],ax",
             "    mov al,byte ptr [si+22]", "    mov byte ptr es:[0x6dbf],al",
             "    mov ax,0x6000", "    mov es,ax", "    mov al,byte ptr [si+23]", "    mov byte ptr es:[0x0196],al",
             "    mov al,byte ptr [si+24]", "    mov byte ptr es:[0x019b],al",
             "    push cs", "    pop es",
             "    mov al,byte ptr [si]", "    mov ah,byte ptr [si+25]", "    push ax",
             "    mov bl,byte ptr [si+26]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    popf"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax", "    mov si,word ptr cs:[entry_off]", "    mov cx,16", "    rep movsb",
             "    mov si,0x49f3", "    movsb", "    mov si,0x4428", "    mov cx,4", "    rep movsb", "    mov si,0x6dbf", "    movsb",
             "    mov ax,0x6000", "    mov ds,ax", "    mov si,0x0196", "    movsb", "    mov si,0x019b", "    movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", "entry_off: .word 0",
             f"target_far: .word 0x07d6, {CALLER_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS)]
    for label, blob, at in (("resident", res, RES_BLOB), ("dice", span, U9E_BLOB), ("system", system, SYS_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for n, case in enumerate(cases):
        row = [case["k"], case["mode"]] + list(struct.pack("<I", case["seed"])) + case["entry"] + \
              [case["flag"], case["rec96"], case["rec9b"], case["high"], n % len(FLAGS), 0]
        text.append("    .byte " + ",".join(f"{b:#x}" for b in row))
    text += [f".org {CODE_AT - 0x100:#x}", "unit:"]
    for i in range(0, len(unit), 16):
        text.append("    .byte " + ",".join(f"{b:#x}" for b in unit[i:i + 16]))
    return "\n".join(text) + "\n"


def native_audit(target, code86, code9e, image, function):
    del target
    directory = ROOT / "resources/oracle-runs/u86-07d6-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    system = exe[SYSTEM_FILE[0]:SYSTEM_FILE[1]]
    res = image[0x3262:0x32C9]
    span = code9e[0x0DE3:0x0E52]
    unit = bytearray(code86)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, STUB_N)
    struct.pack_into("<H", unit, 0x0880 + 3, RES_N)
    cases = native_cases()
    check(CASES_AT + CASE_SIZE * len(cases) <= SYSTEM_HOLE[0] and CODE_AT >= SYSTEM_HOLE[1] and CODE_AT + len(unit) <= OUTPUT
          and len(unit) <= 16 * (RES_N - CALLER_N) and SYS_BLOB + len(system) <= CASES_AT, "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(bytes(unit), res, span, system, cases))
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
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    mul_final = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (RES_N, 0x26C2, res), (DICE_N, 0x0DE3, span), (0x0AF8, 0x11C0, system)):
            at = physical(seg, off); memory[at:at + len(blob)] = blob
        memory[physical(STUB_N, 0x48):physical(STUB_N, 0x48) + 5] = struct.pack("<BHH", 0xEA, 0x0DE3, DICE_N)
        put16(memory, DATA_DS, 0x5D92, 0x0100); put16(memory, DATA_DS, 0x5D94, 0x6000)
        entry_off = 0x31FA + 16 * case["k"]
        for i, b in enumerate(case["entry"]):
            memory[physical(DATA_DS, entry_off + i)] = b
        memory[physical(DATA_DS, 0x49F3)] = case["mode"]
        put16(memory, DATA_DS, 0x4428, case["seed"] & 0xFFFF); put16(memory, DATA_DS, 0x442A, case["seed"] >> 16)
        memory[physical(DATA_DS, 0x6DBF)] = case["flag"]
        memory[physical(0x6000, 0x0196)] = case["rec96"]; memory[physical(0x6000, 0x019B)] = case["rec9b"]
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP
        for value in ((case["high"] << 8) | case["k"], segment, returned):
            sp -= 2; put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, 0x07D6, DATA_DS, segment, segment, FLAGS[n % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(0)
        check(function(ram, SIZE, ctypes.byref(cpu), 256, ctypes.byref(callee)) == 0, f"C status on native case {n}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"native case {n} (k={case['k']:02x}) {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(DATA_DS, entry_off):physical(DATA_DS, entry_off) + 16])
        got += bytes([ram[physical(DATA_DS, 0x49F3)]]) + bytes(ram[physical(DATA_DS, 0x4428):physical(DATA_DS, 0x4428) + 4])
        got += bytes([ram[physical(DATA_DS, 0x6DBF)], ram[physical(0x6000, 0x0196)], ram[physical(0x6000, 0x019B)]])
        check(got == record[28:], f"native case {n} (k={case['k']:02x}) bytes differ")
        mul_final += case["k"] in (0x28, 0x3F)
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({mul_final} MUL-final, flags unmasked); all 14 CPU words, "
          f"{FRAME_SIZE} stack bytes, the table entry, mode, seed, flag and record bytes; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM copied to absolute 0AF8:11C0")
    return dict(cases=len(cases), mul_final=mul_final, result_sha256=digest)
