#!/usr/bin/env python3
"""Synthetic DOSBox-X U38 caller/suffix oracle, not natural game reachability.

All copied bodies, caller bytes, assembly, COM and outputs stay in resources/.
Transparent far-JMP stubs preserve the original relocated caller's frame.
"""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u38_effects_install_31a2_audit import (
    Cpu, ROOT, SIZE, BODY_SHA, SEGMENT_SITES, library, physical, put16, source,
)
from u86_procvars_3413_audit import (
    SEGMENT_SITES as U86_SITES, library as u86_library, source as u86_source,
)

COUNT, FRAME, FRAME_SIZE = 7, 0xEFD0, 48
DATA, DATA_SIZE = 0x6800, 0x600
RECORD_SIZE = 28 + FRAME_SIZE + DATA_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003, 0x0002)


def main():
    directory = ROOT / "resources/u38-31a2-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body, _, metadata = source()
    prior_body, _ = u86_source()
    body_path, prior_path, caller_path = directory / "u38.bin", directory / "u86.bin", directory / "caller.bin"
    body_path.write_bytes(body)
    prior_path.write_bytes(prior_body)
    caller_path.write_bytes(metadata["caller_suffix"])
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x25", ".byte 0xea", ".word 0x31a2", "stub38_segment: .word 0",
           ".org 0x5a2", ".byte 0xea", ".word 0x3413", "stub86_segment: .word 0",
           ".org 0x30a2", '.incbin "' + str(body_path) + '"', ".org 0x4000", "main:",
           "mov ax,cs", "mov ds,ax", "mov es,ax", "mov ss,ax", "mov sp,0xf000",
           "mov word ptr [header],ax", "mov word ptr [stub38_segment],ax", "add ax,0x400",
           "mov word ptr [stub86_segment],ax", "mov ax,cs", "add ax,0x10",
           "mov word ptr [call_leaf+3],ax", "mov word ptr [call_pair+8],ax", "add ax,0x55",
           "mov word ptr [call_pair+3],ax", "mov word ptr [header+2],offset leaf_returned",
           "mov word ptr [header+4],offset pair_second", "mov word ptr [header+6],offset pair_returned",
           f"mov word ptr [header+8],{COUNT}", "mov dx,offset filename", "xor cx,cx",
           "mov ah,0x3c", "int 0x21", "jc failure", "mov word ptr [handle],ax",
           "mov bx,ax", "mov dx,offset header", "mov cx,10", "mov ah,0x40", "int 0x21",
           "again:", "cli", "mov sp,0xf000", "push cs", "pop es", "cld",
           f"mov di,{FRAME}", f"mov cx,{FRAME_SIZE}", "mov al,0xa5", "rep stosb",
           f"mov di,{DATA}", f"mov cx,{DATA_SIZE}", "rep stosb", "mov ax,cs", "sub ax,0x31",
           "mov word ptr [load_base],ax", "mov dx,word ptr [index]", "test dx,dx", "jnz not_raw",
           "mov word ptr [load_base],0", "not_raw:", "cmp dx,2", "jne not_wrap",
           "mov word ptr [load_base],0xffff", "not_wrap:", "cmp dx,3", "jne not_zero",
           "mov word ptr [load_base],0xffbf", "not_zero:", "cmp dx,4", "jne set_relocations",
           "mov word ptr [load_base],0xf000", "set_relocations:", "xor si,si", "relocate38:",
           "mov bx,word ptr [relocations+si]", "mov ax,word ptr [original_segments+si]",
           "add ax,word ptr [load_base]", "cmp word ptr [index],5", "jne store38",
           "mov ax,si", "shr ax,1", "add ax,0x1000", "store38: mov word ptr [bx],ax",
           "add si,2", "cmp si,276", "jb relocate38", "mov ax,cs", "add ax,0x65"]
    for ip in U86_SITES:
        asm.append(f"mov word ptr [{0x4000+ip}],ax")
    asm += ["cmp word ptr [index],6", "jne single_entry",
            "mov word ptr [dispatch],offset call_pair", "mov word ptr [return_ip],offset pair_returned",
            "jmp dispatch_ready", "single_entry:", "mov word ptr [dispatch],offset call_leaf",
            "mov word ptr [return_ip],offset leaf_returned", "dispatch_ready:",
            "mov si,word ptr [index]", "shl si,1", "push word ptr [flags_table+si]", "popf",
            "mov ax,0xabcd", "mov bx,0x3456", "mov cx,0x5678", "mov dx,0x789a",
            "mov si,0x9abc", "mov di,0xbcde", "mov bp,0x7777",
            # JMP through a memory table dispatches without changing FLAGS.
            "jmp word ptr [dispatch]", "call_leaf:", '.incbin "' + str(caller_path) + '",5,5',
            "leaf_returned: jmp capture", "call_pair:", '.incbin "' + str(caller_path) + '",0,5',
            "pair_second:", '.incbin "' + str(caller_path) + '",5,5', "pair_returned: jmp capture",
            "capture:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov ax,word ptr [return_ip]", "mov word ptr [output+18],ax",
            "mov word ptr [output+20],ds", "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
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
            "header: .space 10", "handle: .word 0", "index: .word 0", "load_base: .word 0",
            "dispatch: .word 0", "return_ip: .word 0", f"output: .space {RECORD_SIZE}",
            "flags_table: .word " + ",".join(str(x) for x in FLAGS), "relocations:"]
    asm += [f".word {ip}" for ip in SEGMENT_SITES]
    asm += ["original_segments:"] + [f".word {seg}" for seg in metadata["segments"]]
    asm += [".org 0x7313", '.incbin "' + str(prior_path) + '"']
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
    check_native(directory, result_path.read_bytes(), com.read_bytes(), metadata)


def check_native(directory, result, code, metadata):
    if len(result) != 10 + COUNT * RECORD_SIZE:
        raise ValueError("native output incomplete")
    segment, leaf_return, middle_return, pair_return, count = struct.unpack_from("<5H", result)
    if count != COUNT:
        raise ValueError("native count differs")
    lib, prior = library(directory), u86_library(directory)
    for case in range(COUNT):
        base = (0 if case == 0 else 0xFFFF if case == 2 else 0xFFBF if case == 3 else
                0xF000 if case == 4 else (segment - 0x31) & 0xFFFF)
        values = [0x1000 + i for i in range(138)] if case == 5 else [
            (seg + base) & 0xFFFF for seg in metadata["segments"]]
        memory = bytearray([0xA5]) * SIZE
        at = physical(segment, 0x100)
        memory[at:at + len(code)] = code
        at = physical(segment, DATA)
        memory[at:at + DATA_SIZE] = bytes([0xA5]) * DATA_SIZE
        for ip, value in zip(SEGMENT_SITES, values):
            put16(memory, segment, ip, value)
        for ip in U86_SITES:
            put16(memory, (segment + 0x400) & 0xFFFF, ip, (segment + 0x65) & 0xFFFF)
        composed = case == 6
        put16(memory, segment, 0xEFFC, middle_return if composed else leaf_return)
        put16(memory, segment, 0xEFFE, segment)
        cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE,
                  0x7777, 0xEFFC, (segment + 0x400) & 0xFFFF if composed else segment,
                  0x3413 if composed else 0x31A2, segment, segment, segment, FLAGS[case])
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if composed:
            if prior.pr_u86_procvars_3413(ram, SIZE, ctypes.byref(cpu)):
                raise ValueError("C prior initializer rejected composition")
            if (cpu.cs, cpu.ip, cpu.sp) != (segment, middle_return, 0xF000):
                raise ValueError("C prior initializer return frame differs")
            put16(memory, segment, 0xEFFC, pair_return)
            put16(memory, segment, 0xEFFE, segment)
            cpu.cs, cpu.ip, cpu.sp = segment, 0x31A2, 0xEFFC
        if lib.pr_u38_effects_install_31a2(ram, SIZE, ctypes.byref(cpu)):
            raise ValueError(f"C installer rejected native case {case}")
        observed = result[10 + case * RECORD_SIZE:10 + (case + 1) * RECORD_SIZE]
        for (name, _), expected in zip(Cpu._fields_, struct.unpack_from("<14H", observed)):
            # U38 preserves every flag. Only the prior U86 XOR makes AF
            # undefined in the one composed caller-suffix case.
            mask = 0xFFEF if name == "flags" and composed else 0xFFFF
            if (getattr(cpu, name) ^ expected) & mask:
                raise ValueError(f"native {case} {name}: {getattr(cpu, name):04X}/{expected:04X}")
        at = physical(segment, FRAME)
        if memory[at:at + FRAME_SIZE] != observed[28:28 + FRAME_SIZE]:
            raise ValueError(f"native {case} frame differs")
        at = physical(segment, DATA)
        if memory[at:at + DATA_SIZE] != observed[28 + FRAME_SIZE:]:
            raise ValueError(f"native {case} data region differs")
        expected_hole = ((0x15B, (segment + 0x65) & 0xFFFF) if composed else (0xA5A5, 0xA5A5))
        if struct.unpack_from("<2H", memory, physical(segment, 0x6988)) != expected_hole:
            raise ValueError("installer overwrote the prior pointer in the skipped slot")
    report = dict(target="OVL_0038_31A2", source_sha256=BODY_SHA, native_cases=count,
                  direct_leaf_cases=6, original_caller_suffix_cases=1, actual_far_calls=True,
                  transparent_stubs=True, interrupts_isolated=True, natural_game_route_measured=False,
                  leaf_flags_masked=0, composed_undefined_af_masked=True,
                  compared_cpu_words=14, compared_frame_bytes=FRAME_SIZE, compared_data_bytes=DATA_SIZE,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
