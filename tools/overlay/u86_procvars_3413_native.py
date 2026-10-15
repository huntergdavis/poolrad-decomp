#!/usr/bin/env python3
"""Synthetic DOSBox-X U86 far-call oracle; all products stay in resources/.

Reuses the U25/U77 COM method, including IRQ isolation during frame capture.
This is not a reached game caller or an execution of installed callbacks.
"""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u86_procvars_3413_audit import (
    Cpu, ROOT, SIZE, BODY_SHA, SEGMENT_SITES, library, physical, put16, source,
)

FRAME, FRAME_SIZE = 0xEFD0, 48
DATA, DATA_SIZE = 0x6800, 0x600
RECORD_SIZE = 28 + FRAME_SIZE + DATA_SIZE
COUNT = 6


def main():
    directory = ROOT / "resources/u86-3413-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body, _ = source()
    body_path = directory / "body.bin"
    body_path.write_bytes(body)
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x3313", '.incbin "' + str(body_path) + '"', ".org 0x4000",
           "main:", "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax",
           "mov sp,0xf000", "mov word ptr [call_segment],ax", "mov word ptr [header],ax",
           "mov word ptr [header+2],offset returned", f"mov word ptr [header+4],{COUNT}",
           "mov dx,offset filename", "xor cx,cx", "mov ah,0x3c", "int 0x21", "jc failure",
           "mov word ptr [handle],ax", "mov bx,ax", "mov dx,offset header", "mov cx,6",
           "mov ah,0x40", "int 0x21", "again:", "cli", "mov sp,0xf000",
           "push cs", "pop es", "cld", f"mov di,{FRAME}", f"mov cx,{FRAME_SIZE}",
           "mov al,0xa5", "rep stosb", f"mov di,{DATA}", f"mov cx,{DATA_SIZE}", "rep stosb",
           "xor si,si", "relocate:", "mov bx,word ptr [relocations+si]",
           "mov ax,word ptr [index]", "cmp ax,1", "jb raw_segment", "je loaded_segment",
           "cmp ax,2", "je wrapped_segment", "cmp ax,3", "je zero_segment",
           "cmp ax,4", "je high_segment", "mov ax,si", "shr ax,1", "add ax,0x1000",
           "jmp put_segment", "raw_segment: mov ax,0x96", "jmp put_segment",
           "loaded_segment: mov ax,cs", "add ax,0x96", "jmp put_segment",
           "wrapped_segment: mov ax,0x95", "jmp put_segment",
           "zero_segment: xor ax,ax", "jmp put_segment",
           "high_segment: mov ax,0xf096", "put_segment: mov word ptr [bx],ax",
           "add si,2", f"cmp si,{len(SEGMENT_SITES)*2}", "jb relocate",
           "push 0x0002", "popf", "mov ax,0xabcd", "mov bx,0x3456", "mov cx,0x5678",
           "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde", "mov bp,0x7777",
           ".byte 0x9a", ".word 0x3413", "call_segment: .word 0", "returned:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    for offset in range(0, FRAME_SIZE, 2):
        asm += [f"mov ax,word ptr [{FRAME+offset}]", f"mov word ptr [output+{28+offset}],ax"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax", "cld",
            f"mov si,{DATA}", f"mov di,offset output+{28+FRAME_SIZE}",
            f"mov cx,{DATA_SIZE}", "rep movsb", "sti", "mov bx,word ptr [handle]",
            "mov dx,offset output", f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21",
            "jc failure", f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
            f"cmp word ptr [index],{COUNT}", "jb again", "mov bx,word ptr [handle]",
            "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0",
            f"output: .space {RECORD_SIZE}", "relocations:"]
    asm += [f".word {ip}" for ip in SEGMENT_SITES]
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
    for index in range(COUNT):
        values = ([0x96] * 69 if index == 0 else
                  [(segment + 0x96) & 0xFFFF] * 69 if index == 1 else
                  [0x95] * 69 if index == 2 else [0] * 69 if index == 3 else
                  [0xF096] * 69 if index == 4 else [0x1000 + i for i in range(69)])
        memory = bytearray([0xA5]) * SIZE
        at = physical(segment, 0x100)
        memory[at:at + len(code)] = code
        for ip, value in zip(SEGMENT_SITES, values):
            put16(memory, segment, ip, value)
        put16(memory, segment, 0xEFFC, return_ip)
        put16(memory, segment, 0xEFFE, segment)
        cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFFC, segment, 0x3413, segment, segment, segment, 0x0002)
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if lib.pr_u86_procvars_3413(ram, SIZE, ctypes.byref(cpu)):
            raise ValueError(f"C rejected native case {index}")
        observed = result[6 + index * RECORD_SIZE:6 + (index + 1) * RECORD_SIZE]
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            mask = 0xFFEF if name == "flags" else 0xFFFF
            if (getattr(cpu, name) ^ expected) & mask:
                raise ValueError(f"native {index} {name}: {getattr(cpu, name):04X}/{expected:04X}")
        at = physical(segment, FRAME)
        if memory[at:at + FRAME_SIZE] != observed[28:28 + FRAME_SIZE]:
            raise ValueError(f"native {index} frame differs")
        at = physical(segment, DATA)
        if memory[at:at + DATA_SIZE] != observed[28 + FRAME_SIZE:]:
            raise ValueError(f"native {index} data region differs")
    report = dict(target="OVL_0086_3413", source_sha256=BODY_SHA, native_cases=count,
                  actual_far_call=True, reached_game_caller=False, interrupts_isolated=True,
                  compared_cpu_words=14, compared_frame_bytes=FRAME_SIZE,
                  compared_data_bytes=DATA_SIZE, segment_fixups=len(SEGMENT_SITES),
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
