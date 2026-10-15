#!/usr/bin/env python3
"""IRQ-isolated original U6A caller suffix/leaf parity, not natural game reach.

Hash-locked original bodies, assembly, COM and output stay in resources/.
"""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u6a_record_clear_high_0159_audit import Cpu, ROOT, SIZE, BODY_SHA, library, physical, put16, source

COUNT, AUX, AUX_SIZE = 260, 0xEF00, 0x140
DATA, DATA_SIZE = 0x9000, 64
RECORD_SIZE = 28 + AUX_SIZE + DATA_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)


def main():
    directory = ROOT / "resources/u6a-0159-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body, _, metadata = source()
    body_path, caller_path = directory / "body.bin", directory / "caller.bin"
    body_path.write_bytes(body)
    caller_path.write_bytes(metadata["caller_suffix"])
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x59", '.incbin "' + str(body_path) + '"',
           # Case 259's final clear changes original return IP 0976 to 0900.
           ".org 0x800", "aliased_return: mov word ptr [return_ip],offset aliased_return",
           "jmp capture", ".org 0x86a",
           "original_caller:", '.incbin "' + str(caller_path) + '"',
           "returned: mov word ptr [return_ip],offset returned", "jmp capture", ".org 0x1000", "main:",
           "mov ax,cs", "mov ds,ax", "mov ss,ax", "mov sp,0xf000",
           "mov word ptr [header],ax", "mov word ptr [header+2],offset returned",
           f"mov word ptr [header+4],{COUNT}", "mov dx,offset filename", "xor cx,cx",
           "mov ah,0x3c", "int 0x21", "jc failure", "mov word ptr [handle],ax",
           "mov bx,ax", "mov dx,offset header", "mov cx,6", "mov ah,0x40", "int 0x21",
           "again:", "cli", "mov sp,0xf000", "push cs", "pop es", "cld",
           f"mov di,{AUX}", f"mov cx,{AUX_SIZE}", "mov al,0xa5", "rep stosb",
           f"mov di,{DATA}", f"mov cx,{DATA_SIZE}", "rep stosb",
           "mov word ptr [0x5d92],0x9000", "mov ax,cs", "mov word ptr [0x5d94],ax",
           "cmp word ptr [index],257", "jb ordinary",
           "jne not_argument_alias", "mov word ptr [0x5d92],0xefe5", "jmp record_ready",
           "not_argument_alias:", "cmp word ptr [index],258", "jne return_alias",
           "mov word ptr [0x5d92],0xefca", "jmp record_ready", "return_alias:",
           "mov word ptr [0x5d92],0xefcc",
           "jmp record_ready", "ordinary:", "mov di,0x9017", "mov cx,21",
           "mov ax,word ptr [index]", "cmp ax,256", "jne fill_uniform", "xor si,si",
           "fill_boundary:", "mov al,byte ptr [boundary+si]", "stosb", "inc si",
           "cmp si,7", "jb next_boundary", "xor si,si", "next_boundary:",
           "loop fill_boundary", "jmp record_ready", "fill_uniform: rep stosb", "record_ready:",
           "mov ax,word ptr [index]", "xor dx,dx", "mov bx,6", "div bx",
           "mov si,dx", "shl si,1", "mov ax,cs", "inc ax", "mov es,ax",
           "push word ptr [flags_table+si]", "popf", "mov ax,0xabcd", "mov bx,0x3456",
           "mov cx,0x5678", "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
           "mov bp,0x7777", "jmp original_caller", "capture:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov ax,word ptr [return_ip]", "mov word ptr [output+18],ax",
            "mov word ptr [output+20],ds", "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
    # Capture stack before PUSHF overwrites EFFE. All these MOVs preserve flags.
    for offset in range(0, AUX_SIZE, 2):
        asm += [f"mov ax,word ptr [{AUX+offset}]", f"mov word ptr [output+{28+offset}],ax"]
    asm += ["pushf", "pop ax", "mov word ptr [output+26],ax", "push cs", "pop es", "cld",
            f"mov si,{DATA}", f"mov di,offset output+{28+AUX_SIZE}", f"mov cx,{DATA_SIZE}",
            "rep movsb", "sti", "mov bx,word ptr [handle]", "mov dx,offset output",
            f"mov cx,{RECORD_SIZE}", "mov ah,0x40", "int 0x21", "jc failure",
            f"cmp ax,{RECORD_SIZE}", "jne failure", "inc word ptr [index]",
            f"cmp word ptr [index],{COUNT}", "jb again", "mov bx,word ptr [handle]",
            "mov ah,0x3e", "int 0x21", "mov ax,0x4c00", "int 0x21", "failure:",
            "mov ax,0x4c01", "int 0x21", 'filename: .asciz "RESULT.BIN"',
            "header: .space 6", "handle: .word 0", "index: .word 0", "return_ip: .word 0",
            f"output: .space {RECORD_SIZE}", "boundary: .byte 0,1,0x7e,0x7f,0x80,0x81,0xff",
            "flags_table: .word " + ",".join(str(x) for x in FLAGS)]
    assembly, obj, com = directory / "oracle.S", directory / "oracle.o", directory / "ORACLE.COM"
    assembly.write_text("\n".join(asm)+"\n")
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
    if len(result) != 6 + COUNT*RECORD_SIZE:
        raise ValueError("native output incomplete")
    segment, return_ip, count = struct.unpack_from("<3H", result)
    if count != COUNT or return_ip != 0x0976:
        raise ValueError("native caller count/return differs")
    lib = library(directory)
    for case in range(COUNT):
        offset = 0x9000 if case < 257 else (0xEFE5, 0xEFCA, 0xEFCC)[case-257]
        memory = bytearray([0xA5])*SIZE
        at = physical(segment, 0x100)
        memory[at:at+len(code)] = code
        if case < 257:
            values = [case]*21 if case < 256 else [0, 1, 0x7E, 0x7F, 0x80, 0x81, 0xFF]*3
            for i, value in enumerate(values):
                memory[physical(segment, 0x9017+i)] = value
        put16(memory, segment, 0x5D92, offset)
        put16(memory, segment, 0x5D94, segment)
        # Exact original suffix pushes the far argument, then CS and near CALL.
        for off, value in ((0xEFF8, return_ip), (0xEFFA, segment), (0xEFFC, offset), (0xEFFE, segment)):
            put16(memory, segment, off, value)
        cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFF8, segment, 0x0159, segment, (segment+1) & 0xFFFF,
                  segment, FLAGS[case % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if lib.pr_u6a_record_clear_high_0159(ram, SIZE, ctypes.byref(cpu)):
            raise ValueError(f"C rejected native case {case}")
        observed = result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            if getattr(cpu, name) != expected:
                raise ValueError(f"native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}")
        at = physical(segment, AUX)
        if memory[at:at+AUX_SIZE] != observed[28:28+AUX_SIZE]:
            raise ValueError(f"native {case} stack/alias region differs")
        at = physical(segment, DATA)
        if memory[at:at+DATA_SIZE] != observed[28+AUX_SIZE:]:
            raise ValueError(f"native {case} record region differs")
    report = dict(target="OVL_006A_0159", source_sha256=BODY_SHA, native_cases=count,
                  original_caller_suffix="U6A:+096A..0975", caller_return_ip="0976",
                  actual_push_cs_near_call=True, reached_game_route=False, interrupts_isolated=True,
                  compared_cpu_words=14, compared_stack_alias_bytes=AUX_SIZE,
                  compared_record_bytes=DATA_SIZE, masked_flags=0,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2)+"\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
