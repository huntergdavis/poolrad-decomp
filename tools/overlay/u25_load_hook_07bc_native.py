#!/usr/bin/env python3
"""Synthetic DOSBox-X original/C oracle for unit 0x25 +07BC entered on its own; all products are ignored.

The +0E0C native's COM layout (the +06FB/+0756/+07BC prefix at CS:06FB, the same globals and records), but
the program calls +07BC directly with the hook's RETF 6 frame: the address word, then a far pointer to a
found-flag byte at DS:9F00, then PUSH CS; CALL. This does not establish natural saved-game reachability.
"""
# NOT_A_ROW_CHECKER: native helper imported by u25_load_hook_07bc_audit.py --native; the audit registers the row
import ctypes
from hashlib import sha256
import os
import struct
import subprocess

from u25_load_word_0e0c_audit import Cpu, SIZE, physical, pointer, put16, word

KEYS = (0x18, 0x9B, 0xA0, 0xA5, 0xAC, 0xB8, 0xBB, 0xBD, 0xBF, 0xC1, 0xC3, 0xD8, 0xF7, 0xF9, 0x100, 0x10C, 0x10D, 0x11B,
        0x2B1, 0x2B4, 0x312, 0x33E, 0x33F, 0x3FF)
CASES = [(0x6B00 + k, 26) for k in KEYS] + [(0x9900, 26), (0x0000, 26), (0xFFFF, 26)] + \
        [(0x6DCF, v) for v in (0, 3, 4, 7, 8, 12, 13, 16, 17, 18, 25, 26)]
FRAME_START, FRAME_SIZE, FLAG = 0xEFD0, 0x40, 0x9F00
RECORD_SIZE = 28 + FRAME_SIZE + 4
GLOBAL_POINTERS = ((0x49D2, 0x7000), (0x49D6, 0x7800), (0x49DA, 0x8000), (0x49DE, 0x8400), (0x5D92, 0x9000), (0x5D96, 0x9000))
GLOBAL_WORDS = ((0x6F6E, 0xFEDC), (0x6F6A, 0x1234), (0x6F6C, 0xABCD), (0x7830, 10), (0x9104, 0), (0x9106, 0), (0x7E7C, 0x8123))
GLOBAL_BYTES = ((0x5376, 3), (0x9014, 7), (0x906E, 255), (0x9073, 255), (0x9084, 0x81), (0x90BA, 0x7F), (0x920D, 1), (0x920E, 1), (0x920F, 0))


def native(directory, bodies, lib):
    directory.mkdir(parents=True, exist_ok=True)
    prefix = directory / "prefix.bin"
    prefix.write_bytes(bodies[0x6FB] + bodies[0x756] + bodies[0x7BC])
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x5fb", '.incbin "' + str(prefix.resolve()) + '"',
           ".org 0x1f00", "main:", "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
           "mov sp,0xf000", "cld", "mov di,0x7000", "mov cx,0x3200", "mov al,0xa5", "rep stosb"]
    for off, target in GLOBAL_POINTERS:
        asm += [f"mov word ptr [{off}],{target}", f"mov word ptr [{off + 2}],cs"]
    asm += [f"mov word ptr [{off}],{value}" for off, value in GLOBAL_WORDS]
    asm += [f"mov byte ptr [{off}],{value}" for off, value in GLOBAL_BYTES]
    asm += ["mov word ptr [header],cs", "mov word ptr [header+2],offset returned", f"mov word ptr [header+4],{0x7BC}",
            f"mov word ptr [header+6],{len(CASES)}",
            "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c", "int 0x21", "jc failure",
            "mov word ptr [handle],ax", "mov bx,ax", "mov dx,offset header", "mov cx,8", "mov ah,0x40", "int 0x21",
            "again:", "mov sp,0xf000", "push cs", "pop es", "cld",
            f"mov di,{FRAME_START}", f"mov cx,{FRAME_SIZE}", "mov al,0xa5", "rep stosb", f"mov byte ptr [{FLAG}],0x77",
            "mov si,word ptr [index]", "shl si,1", "shl si,1",
            "mov ax,word ptr [case_table+si]", "mov word ptr [address],ax",
            "mov ax,word ptr [case_table+si+2]", "mov byte ptr [0x9015],al",
            "push word ptr [address]", "push cs", f"mov ax,{FLAG}", "push ax",
            "mov ax,0x0202", "push ax", "popf",
            "mov ax,0x9234", "mov bx,0x1357", "mov cx,0x2468", "mov dx,0xabcd",
            "mov si,0x5678", "mov di,0x6789", "mov bp,0x4321", "push cs", f"call {0x7BC}", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2 * i}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    for off in range(0, FRAME_SIZE, 2):
        asm += [f"mov ax,word ptr [{FRAME_START + off}]", f"mov word ptr [output+{28 + off}],ax"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax",
            f"mov al,byte ptr [{FLAG}]", "xor ah,ah", f"mov word ptr [output+{28 + FRAME_SIZE}],ax",
            "mov ax,word ptr [address]", f"mov word ptr [output+{30 + FRAME_SIZE}],ax",
            "mov bx,word ptr [handle]", "mov dx,offset output", f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
            f"cmp word ptr [index],{len(CASES)}", "jb again",
            "mov bx,word ptr [handle]", "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21",
            "failure:", "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 8", "handle: .word 0", "index: .word 0", "address: .word 0",
            f"output: .space {RECORD_SIZE}", "case_table:"]
    asm += [f".word {address},{value}" for address, value in CASES]
    asm_path, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    asm_path.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(asm_path), "-o", str(obj)], check=True, capture_output=True, text=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(com)],
                   check=True, capture_output=True, text=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c " + str(directory.resolve()) +
                      "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
                       stdout=log, stderr=subprocess.STDOUT, check=True, timeout=45)
    result = result_path.read_bytes()
    check_native(result, lib, com.read_bytes())
    return dict(native_cases=len(CASES), bytes_compared=len(CASES) * RECORD_SIZE, masked_flags=0,
                dosbox_binary_sha256=sha256(open(subprocess.run(["which", "dosbox-x"], capture_output=True, text=True).stdout.strip(), "rb").read()).hexdigest(),
                pinned_build_used=False, result_sha256=sha256(result).hexdigest())


def check_native(result, lib, code):
    if len(result) != 8 + len(CASES) * RECORD_SIZE:
        raise ValueError("synthetic native output incomplete")
    segment, return_ip, entry_ip, count = struct.unpack_from("<4H", result)
    if entry_ip != 0x7BC or count != len(CASES):
        raise ValueError("synthetic native header differs")
    for index, (address, value) in enumerate(CASES):
        observed = result[8 + index * RECORD_SIZE:8 + (index + 1) * RECORD_SIZE]
        ram = bytearray([0xA5]) * SIZE
        code_at = physical(segment, 0x100)
        ram[code_at:code_at + len(code)] = code
        for off, target in GLOBAL_POINTERS:
            pointer(ram, segment, off, segment, target)
        for off, data in GLOBAL_WORDS:
            put16(ram, segment, off, data)
        for off, data in GLOBAL_BYTES + ((0x9015, value), (FLAG, 0x77)):
            ram[physical(segment, off)] = data
        sp = 0xF000 - 10
        for k, data in enumerate((return_ip, segment, FLAG, segment, address)):
            put16(ram, segment, sp + 2 * k, data)
        cpu = Cpu(0x9234, 0x1357, 0x2468, 0xABCD, 0x5678, 0x6789, 0x4321, sp, segment, entry_ip, segment, segment, segment, 0x0202)
        memory = (ctypes.c_uint8 * SIZE).from_buffer(ram)
        if lib.pr_u25_load_hook_07bc(memory, SIZE, ctypes.byref(cpu)) != 0:
            raise ValueError(f"C rejected native case {index}")
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            if getattr(cpu, name) != expected:
                raise ValueError(f"native {name} mismatch case {index} ({address:04X}): {getattr(cpu, name):04X}/{expected:04X}")
        at = physical(segment, FRAME_START)
        if ram[at:at + FRAME_SIZE] != observed[28:28 + FRAME_SIZE]:
            raise ValueError(f"native stack-frame mismatch case {index} ({address:04X})")
        flag, kept = struct.unpack_from("<2H", observed, 28 + FRAME_SIZE)
        if (flag, kept) != (ram[physical(segment, FLAG)], address):
            raise ValueError(f"native flag/input mismatch case {index} ({address:04X}): {flag}/{ram[physical(segment, FLAG)]}")
