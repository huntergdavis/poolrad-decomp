#!/usr/bin/env python3
"""Synthetic DOSBox-X U33 caller frames, not a reached game route.

Reuses the U25 COM-oracle method. Copied bodies and generated products are
written only under ignored resources/. Run with Capstone/Unicorn available.
"""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u33_byte_flags_08a4_audit import (
    Cpu, ROOT, SIZE, SPAN_SHA, library, physical, put16, source,
)

COUNT, FRAME, FRAME_SIZE = 512, 0xEFD0, 48
OFFSETS = list(range(FRAME, FRAME + FRAME_SIZE, 2)) + [0x465A, 0x49D2, 0x49D4, 0x8366]
RECORD_SIZE = 28 + len(OFFSETS) * 2


def main():
    directory = ROOT / "resources/u33-08a4-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body, _ = source(ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x7a4", '.incbin "' + str(body_path) + '"', ".org 0x2000",
           "main:", "mov ax,cs", "mov ds,ax", "mov ss,ax", "mov sp,0xf000",
           "mov word ptr [call_segment],ax", "mov word ptr [header],ax",
           "mov word ptr [header+2],offset returned", f"mov word ptr [header+4],{COUNT}",
           "mov word ptr [0x49d2],0x8000", "mov word ptr [0x49d4],cs",
           "mov byte ptr [0x465b],0xa5", "mov dx,offset filename", "xor cx,cx",
           "mov ah,0x3c", "int 0x21", "jc failure", "mov word ptr [handle],ax",
           "mov bx,ax", "mov dx,offset header", "mov cx,6", "mov ah,0x40", "int 0x21",
           "again:", "cli", "mov sp,0xf000", "push cs", "pop es", "cld",
           f"mov di,{FRAME}", f"mov cx,{FRAME_SIZE}", "mov al,0xa5", "rep stosb",
           "mov ax,word ptr [index]", "shr ax,1", "mov byte ptr [0x465a],al",
           "mov ax,word ptr [index]", "and ax,1", "add ax,0xfe",
           "mov word ptr [0x8366],ax", "mov ax,cs", "inc ax", "mov es,ax",
           "push 0x0002", "popf", "mov ax,0xabcd", "mov bx,0x3456",
           "mov cx,0x5678", "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
           "mov bp,0x7777", ".byte 0x9a", ".word 0x08a4", "call_segment: .word 0",
           "returned:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    for index, off in enumerate(OFFSETS):
        asm += [f"mov ax,word ptr [{off}]", f"mov word ptr [output+{28+2*index}],ax"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax", "sti", "mov bx,word ptr [handle]",
            "mov dx,offset output", f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
            f"cmp word ptr [index],{COUNT}", "jb again", "mov bx,word ptr [handle]",
            "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0", f"output: .space {RECORD_SIZE}"]
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
    result = result_path.read_bytes()
    if len(result) != 6 + COUNT * RECORD_SIZE:
        raise ValueError("native output incomplete")
    segment, return_ip, count = struct.unpack_from("<3H", result)
    if count != COUNT:
        raise ValueError("native count differs")
    lib, code = library(directory), com.read_bytes()
    for index in range(COUNT):
        value, window = index // 2, 0xFE + index % 2
        observed = result[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
        memory = bytearray([0xA5]) * SIZE
        at = physical(segment, 0x100)
        memory[at:at + len(code)] = code
        memory[physical(segment, 0x465A)] = value
        put16(memory, segment, 0x49D2, 0x8000)
        put16(memory, segment, 0x49D4, segment)
        put16(memory, segment, 0x8366, window)
        put16(memory, segment, 0xEFFC, return_ip)
        put16(memory, segment, 0xEFFE, segment)
        cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFFC, segment, 0x08A4, segment, (segment+1) & 0xFFFF,
                  segment, 0x0002)
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if lib.pr_u33_byte_flags_08a4(ram, SIZE, ctypes.byref(cpu)):
            raise ValueError(f"C rejected native case {index}")
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            # DI changes only on the high-bit path, whose last flags are CMP
            # when window != FF, and OR otherwise. Never mask defined CMP AF.
            cmp_flags = cpu.di == 0x8000 and window != 0xFF
            mask = 0xFFEF if name == "flags" and not cmp_flags else 0xFFFF
            if (getattr(cpu, name) ^ expected) & mask:
                raise ValueError(f"native {index} {name}: {getattr(cpu, name):04X}/{expected:04X}")
        actual = b"".join(bytes(memory[physical(segment, off):physical(segment, off)+2])
                          for off in OFFSETS)
        if actual != observed[28:]:
            raise ValueError(f"native {index} frame/input/pointer/window differs")
    report = dict(target="OVL_0033_08A4", source_sha256=SPAN_SHA, native_cases=count,
                  actual_far_call=True, reached_game_caller=False, interrupts_isolated=True,
                  compared_cpu_words=14,
                  compared_frame_bytes=FRAME_SIZE, result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
