"""DOSBox-X synthetic oracle for the composed U25 +0F53 (imported by u25_store_string_0f53_composed_audit.py).

At start-up the COM copies the unit 0x25 code to paragraph 8800, with +0F53's two fixup-backed
far-call segment words relocated loader-style to 9000, and the 36 original SYSTEM string-store
bytes to 9000:0B25. Each case fills a 288-byte stack window and a 1 KiB element window at
2000:0000, writes the four window far pointers and the record pointer in data segment 1900 so
element 0 (and the 6B00 name copy) lands at 2000:0100, writes the source string at 2800:0010,
pushes the address word and S and far-calls +0F53 with IRQs masked. The C replay rebuilds the same
memory and compares all 14 CPU words and both windows, with no mask.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess

from u25_store_string_0f53_composed_audit import (ENTRY, SITES, POINTERS, DISPLACEMENTS, RANGES, STORE_CS, STORE_IP, STORE_END,
                                                  check, class_of)
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CALLER_N, STORE_N, DATA_DS, WIN_SEG, SRC_SEG, SRC_OFF = 0x8800, 0x9000, 0x1900, 0x2000, 0x2800, 0x0010
MAIN_AT, STORE_BLOB, UNIT_BLOB, CASES_AT = 0x0300, 0x0F00, 0x1000, 0x3400
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE, WIN_SIZE, ELEMENT0 = 0xEEE0, 0x120, 0x400, 0x0100
CASE_SIZE = 2 + 8 + 2 + 256
RECORD_BYTES = 28 + FRAME_SIZE + WIN_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def windows_for(address):
    """Offsets of the four window pointers in WIN_SEG: the address's own class puts element 0 at ELEMENT0."""
    offsets = [0x0000] * 4
    c = class_of(address)
    if c < 4:
        width = 2 if c < 3 else 1
        offsets[c] = (ELEMENT0 - width * address - DISPLACEMENTS[c]) & 0xFFFF
    return offsets


def native_cases():
    rng = random.Random(0x25F53D05)
    rows = []
    for low, high in RANGES:
        for address in (low, (low + high) // 2, high):
            for n in (0, 1, 3, 20, 255):
                rows.append((address, n))
    rows += [(address, n) for address in (0x0000, 0x4D00, 0x96FF, 0xFFFF) for n in (0, 5)]
    rows += [(0x6B00, n) for n in (0, 1, 14, 15, 16, 255)]
    for _ in range(26):
        c = rng.randrange(5)
        address = rng.randrange(*((RANGES[c][0], RANGES[c][1] + 1) if c < 4 else (0, 0x10000)))
        rows.append((address, rng.choice((0, rng.randrange(1, 40), rng.randrange(256)))))
    return [dict(address=address, n=n, chars=[rng.randrange(256) for _ in range(255)], flags=i % len(FLAGS),
                 windows=windows_for(address)) for i, (address, n) in enumerate(rows)]


def assembly(unit, store, cases):
    text = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main",
            f".org {MAIN_AT - 0x100:#x}", "main:",
            "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld"]
    for segment, offset, label, length in ((CALLER_N, 0, "unit", len(unit)), (STORE_N, STORE_IP, "store", len(store))):
        text += [f"    mov ax,{segment:#x}", "    mov es,ax", f"    mov si,offset {label}", f"    mov di,{offset:#x}",
                 f"    mov cx,{length:#x}", "    rep movsb"]
    text += [f"    mov ax,{DATA_DS:#x}", "    mov es,ax", f"    mov word ptr es:[0x5d92],{ELEMENT0:#x}", f"    mov word ptr es:[0x5d94],{WIN_SEG:#x}"]
    text += [f"    mov word ptr es:[{p + 2:#x}],{WIN_SEG:#x}" for p in POINTERS]
    text += ["    push cs", "    pop es",
             f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
             f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
             "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
             "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8",
             "    mov ah,0x40", "    int 0x21", "    jc failure",
             "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
             "    mov al,0xa5", f"    mov di,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep stosb",
             f"    mov ax,{WIN_SEG:#x}", "    mov es,ax", "    xor di,di", "    mov al,0x5a", f"    mov cx,{WIN_SIZE}", "    rep stosb",
             "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov si,ax",
             f"    mov ax,{DATA_DS:#x}", "    mov es,ax"]
    for k, pointer in enumerate(POINTERS):
        text += [f"    mov ax,word ptr [si+{2 + 2 * k}]", f"    mov word ptr es:[{pointer:#x}],ax"]
    text += [f"    mov ax,{SRC_SEG:#x}", "    mov es,ax", f"    mov di,{SRC_OFF:#x}", "    add si,12", "    mov cx,256", "    rep movsb",
             f"    sub si,{12 + 256}", "    push cs", "    pop es",
             "    push word ptr [si]", f"    mov ax,{SRC_SEG:#x}", "    push ax", f"    mov ax,{SRC_OFF:#x}", "    push ax",
             "    mov bl,byte ptr [si+10]", "    xor bh,bh", "    shl bx,1", "    push word ptr [flags_table+bx]", "    popf",
             f"    mov ax,{DATA_DS:#x}", "    mov ds,ax"]
    for name, value in ENTRY_REGS.items():
        text.append(f"    mov {name},{value:#x}")
    text += ["    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        text.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    text += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
             f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
             f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
             "    cld", "    push cs", "    pop es", "    push cs", "    pop ds", f"    mov di,{OUTPUT + 28:#x}",
             f"    mov si,{FRAME_LOW:#x}", f"    mov cx,{FRAME_SIZE}", "    rep movsb",
             f"    mov ax,{WIN_SEG:#x}", "    mov ds,ax", "    xor si,si", f"    mov cx,{WIN_SIZE}", "    rep movsb",
             "    push cs", "    pop ds", "    sti",
             "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{RECORD_BYTES}",
             "    mov ah,0x40", "    int 0x21", "    jc failure", f"    cmp ax,{RECORD_BYTES}", "    jne failure",
             "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
             "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
             "failure:", "    mov ax,0x4c01", "    int 0x21",
             'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0",
             f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}", "flags_table: .word " + ",".join(f"{f:#x}" for f in FLAGS)]
    for label, blob, at in (("store", store, STORE_BLOB), ("unit", unit, UNIT_BLOB)):
        text += [f".org {at - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in blob[i:i + 16]))
    text += [f".org {CASES_AT - 0x100:#x}", "cases:"]
    for case in cases:
        row = list(struct.pack("<H4HBB", case["address"], *case["windows"], case["flags"], 0)) + [case["n"]] + case["chars"]
        check(len(row) == CASE_SIZE, "case row size differs")
        for i in range(0, len(row), 32):
            text.append("    .byte " + ",".join(f"{b:#x}" for b in row[i:i + 32]))
    return "\n".join(text) + "\n"


def native_audit(target, code, image, function):
    del target
    directory = ROOT / "resources/oracle-runs/u25-0f53-composed-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    store = image[STORE_CS * 16 + STORE_IP:STORE_CS * 16 + STORE_END]
    unit = bytearray(code)
    for site in SITES:
        struct.pack_into("<H", unit, site + 3, STORE_N)
    cases = native_cases()
    check(STORE_BLOB + len(store) <= UNIT_BLOB and UNIT_BLOB + len(unit) <= CASES_AT and CASES_AT + CASE_SIZE * len(cases) <= OUTPUT
          and len(unit) <= 16 * (STORE_N - CALLER_N), "COM layout overlaps")
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text(assembly(bytes(unit), store, cases))
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
    classes = [0] * 5
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        for seg, off, blob in ((CALLER_N, 0, bytes(unit)), (STORE_N, STORE_IP, store)):
            at = physical(seg, off)
            memory[at:at + len(blob)] = blob
        put16(memory, DATA_DS, 0x5D92, ELEMENT0)
        put16(memory, DATA_DS, 0x5D94, WIN_SEG)
        for pointer, offset in zip(POINTERS, case["windows"]):
            put16(memory, DATA_DS, pointer, offset)
            put16(memory, DATA_DS, pointer + 2, WIN_SEG)
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        for i in range(WIN_SIZE):
            memory[physical(WIN_SEG, i)] = 0x5A
        for i, value in enumerate([case["n"]] + case["chars"]):
            memory[physical(SRC_SEG, SRC_OFF + i)] = value
        sp = STACK_TOP
        for value in (case["address"], SRC_SEG, SRC_OFF, segment, returned):
            sp -= 2
            put16(memory, segment, sp, value)
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_DS, segment, segment, FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        callee = ctypes.c_int(99)
        label = f"native case {n} (address {case['address']:04x}, n={case['n']})"
        check(function(ram, SIZE, ctypes.byref(cpu), 300, ctypes.byref(callee)) == 0 and callee.value == 0, f"C status on {label}")
        for (name, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, name) == want, f"{label} {name}: C={getattr(cpu, name):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        got += bytes(ram[physical(WIN_SEG, 0):physical(WIN_SEG, 0) + WIN_SIZE])
        check(got == record[28:], f"{label} bytes differ")
        classes[class_of(case["address"])] += 1
    digest = sha256(result).hexdigest()
    names = sum(case["address"] == 0x6B00 for case in cases)
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} (classes 0-4: {classes}, {names} of them 6B00 name copies); "
          f"all 14 CPU words, the {FRAME_SIZE}-byte stack window and the {WIN_SIZE}-byte element window; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; string store copied to {STORE_N:04X}:0B25")
    return dict(cases=len(cases), classes=classes, name_copies=names, result_sha256=digest)
