#!/usr/bin/env python3
"""IRQ-isolated synthetic DOSBox-X U86 predicate caller frames.

Reuses the U33/U86 COM method. This does not establish a reached game route.
"""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u86_record_predicate_323d_audit import Cpu, ROOT, SIZE, BODY_SHA, library, physical, put16, source

INDICES = (0, 127, 128, 255)
COUNT, FRAME, FRAME_SIZE = 1025, 0xEFD0, 48
RECORD_SIZE = 28 + FRAME_SIZE + 4


def main():
    directory = ROOT / "resources/u86-323d-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body, _, _ = source()
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x313d", '.incbin "' + str(body_path) + '"', ".org 0x4000",
           "main:", "mov ax,cs", "mov ds,ax", "mov ss,ax", "mov sp,0xf000",
           "mov word ptr [call_segment],ax", "mov word ptr [header],ax",
           "mov word ptr [header+2],offset returned", f"mov word ptr [header+4],{COUNT}",
           "mov byte ptr [0x902f],0xa5", "mov dx,offset filename", "xor cx,cx",
           "mov ah,0x3c", "int 0x21", "jc failure", "mov word ptr [handle],ax",
           "mov bx,ax", "mov dx,offset header", "mov cx,6", "mov ah,0x40", "int 0x21",
           "again:", "cli", "mov sp,0xf000", "push cs", "pop es", "cld",
           f"mov di,{FRAME}", f"mov cx,{FRAME_SIZE}", "mov al,0xa5", "rep stosb",
           "cmp word ptr [index],1024", "je null_input", "mov si,word ptr [index]",
           "mov cl,8", "shr si,cl", "mov al,byte ptr [record_indices+si]",
           "mov byte ptr [0x902e],al", "xor ah,ah", "mov bx,ax", "mov cl,4",
           "shl bx,cl", "add bx,0x5582", "mov word ptr [table_offset],bx",
           "mov ax,word ptr [index]", "mov byte ptr [bx],al", "mov byte ptr [bx+1],0xa5",
           "push cs", "push 0x9000", "jmp call_setup", "null_input:",
           "mov byte ptr [0x902e],0", "mov word ptr [0x5582],0xa500",
           "mov word ptr [table_offset],0x5582", "push 0", "push 0", "call_setup:",
           "mov ax,cs", "inc ax", "mov es,ax", "push 0x0002", "popf",
           "mov ax,0xabcd", "mov bx,0x3456", "mov cx,0x5678", "mov dx,0x789a",
           "mov si,0x9abc", "mov di,0xbcde", "mov bp,0x7777",
           ".byte 0x9a", ".word 0x323d", "call_segment: .word 0", "returned:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    for offset in range(0, FRAME_SIZE, 2):
        asm += [f"mov ax,word ptr [{FRAME+offset}]", f"mov word ptr [output+{28+offset}],ax"]
    asm += ["mov ax,word ptr [0x902e]", f"mov word ptr [output+{28+FRAME_SIZE}],ax",
            "mov bx,word ptr [table_offset]", "mov ax,word ptr [bx]",
            f"mov word ptr [output+{30+FRAME_SIZE}],ax", "pushf", "pop ax",
            "mov word ptr [output+26],ax", "sti", "mov bx,word ptr [handle]",
            "mov dx,offset output", f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
            f"cmp word ptr [index],{COUNT}", "jb again", "mov bx,word ptr [handle]",
            "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0", "table_offset: .word 0",
            f"output: .space {RECORD_SIZE}", "record_indices: .byte 0,127,128,255"]
    assembly, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    assembly.write_text("\n".join(asm) + "\n")
    subprocess.run(["as", "--32", str(assembly), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary",
                    str(obj), "-o", str(com)], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\n"
        "cputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
        "[autoexec]\nmount c " + str(directory) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
            env=dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"),
            stdout=log, stderr=subprocess.STDOUT, check=True, timeout=45)
    check_native(directory, result_path.read_bytes(), com.read_bytes())


def check_native(directory, result, code):
    if len(result) != 6 + COUNT * RECORD_SIZE:
        raise ValueError("native output incomplete")
    segment, return_ip, count = struct.unpack_from("<3H", result)
    if count != COUNT:
        raise ValueError("native count differs")
    lib = library(directory)
    for case in range(COUNT):
        null = case == 1024
        index, value = (0, 0) if null else (INDICES[case // 256], case & 255)
        table_offset = 0x5582 + 16 * index
        memory = bytearray([0xA5]) * SIZE
        at = physical(segment, 0x100)
        memory[at:at + len(code)] = code
        memory[physical(segment, 0x902E)] = index
        put16(memory, segment, table_offset, 0xA500 | value)
        for off, data in ((0xEFF8, return_ip), (0xEFFA, segment),
                          (0xEFFC, 0 if null else 0x9000), (0xEFFE, 0 if null else segment)):
            put16(memory, segment, off, data)
        cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFF8, segment, 0x323D, segment, (segment + 1) & 0xFFFF,
                  segment, 0x0002)
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if lib.pr_u86_record_predicate_323d(ram, SIZE, ctypes.byref(cpu)):
            raise ValueError(f"C rejected native case {case}")
        observed = result[6 + case * RECORD_SIZE:6 + (case + 1) * RECORD_SIZE]
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            mask = 0xFFEF if name == "flags" and null else 0xFFFF
            if (getattr(cpu, name) ^ expected) & mask:
                raise ValueError(f"native {case} {name}: {getattr(cpu, name):04X}/{expected:04X}")
        at = physical(segment, FRAME)
        if memory[at:at + FRAME_SIZE] != observed[28:28 + FRAME_SIZE]:
            raise ValueError(f"native {case} frame differs")
        if struct.unpack_from("<2H", observed, 28 + FRAME_SIZE) != (0xA500 | index, 0xA500 | value):
            raise ValueError(f"native {case} input preservation differs")
    report = dict(target="OVL_0086_323D", source_sha256=BODY_SHA, native_cases=count,
                  actual_far_call=True, reached_game_caller=False, interrupts_isolated=True,
                  compared_cpu_words=14, compared_frame_bytes=FRAME_SIZE,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
