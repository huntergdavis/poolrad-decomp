#!/usr/bin/env python3
"""IRQ-isolated original U2F caller-suffix/leaf parity, not natural game reach."""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u2f_record_link_clear_122b_audit import Cpu, ROOT, SIZE, BODY_SHA, library, physical, put16, source

COUNT, AUX, AUX_SIZE = 518, 0xEF00, 0x140
DATA, DATA_SIZE = 0x9000, 0x600
RECORD_SIZE = 28 + AUX_SIZE + DATA_SIZE
FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)


def main():
    directory = ROOT / "resources/u2f-122b-parity/native"
    directory.mkdir(parents=True, exist_ok=True)
    body, _, metadata = source()
    body_path, caller_path = directory / "body.bin", directory / "caller.bin"
    body_path.write_bytes(body)
    caller_path.write_bytes(metadata["caller_suffix"])
    asm = [".intel_syntax noprefix", ".code16", ".global _start", "_start: jmp main",
           ".org 0x453", "original_caller:", '.incbin "' + str(caller_path) + '"',
           "returned: jmp capture", ".org 0x112b", '.incbin "' + str(body_path) + '"',
           ".org 0x2000", "main:", "mov ax,cs", "mov ds,ax", "mov ss,ax", "mov sp,0xf000",
           "mov word ptr [header],ax", "mov word ptr [header+2],offset returned",
           f"mov word ptr [header+4],{COUNT}", "mov dx,offset filename", "xor cx,cx",
           "mov ah,0x3c", "int 0x21", "jc failure", "mov word ptr [handle],ax",
           "mov bx,ax", "mov dx,offset header", "mov cx,6", "mov ah,0x40", "int 0x21",
           "again:", "cli", "mov sp,0xf000", "push cs", "pop es", "cld",
           f"mov di,{AUX}", f"mov cx,{AUX_SIZE}", "mov al,0xa5", "rep stosb",
           f"mov di,{DATA}", f"mov cx,{DATA_SIZE}", "rep stosb",
           "mov word ptr [record_off],0x9000", "mov word ptr [child_off],0x9200",
           "mov word ptr [target_off],0x9400", "mov ax,cs", "mov word ptr [target_seg],ax",
           "mov ax,word ptr [index]", "cmp ax,512", "jae special", "mov byte ptr [0x950e],al",
           "cmp ax,256", "jb ordinary_equal", "xor al,0x80", "ordinary_equal:",
           "mov byte ptr [0x910e],al", "jmp seed_chain", "special:",
           "mov byte ptr [0x910e],7", "mov byte ptr [0x950e],7", "cmp ax,512", "jne not_null",
           "mov word ptr [target_off],0", "mov word ptr [target_seg],0", "jmp seed_chain",
           "not_null:", "cmp ax,513", "jne not_zero_offset", "mov word ptr [target_off],0",
           "mov ax,cs", "add ax,0x900", "mov word ptr [target_seg],ax", "jmp seed_chain",
           "not_zero_offset:", "cmp ax,514", "jne not_equivalent_pointer",
           "mov word ptr [target_off],0x8400", "mov ax,cs", "add ax,0x100",
           "mov word ptr [target_seg],ax", "mov byte ptr [0x950e],8", "jmp seed_chain",
           "not_equivalent_pointer:", "cmp ax,515", "jne not_saved_bp_alias",
           "mov word ptr [record_off],0xeee7", "mov byte ptr [0xeff5],7", "jmp seed_chain",
           "not_saved_bp_alias:", "cmp ax,516", "jne argument_clear",
           "mov word ptr [record_off],0xeeed", "mov byte ptr [0xef0f],7",
           "mov word ptr [0xef09],0x9200", "mov ax,cs", "mov word ptr [0xef0b],ax",
           "jmp seed_child", "argument_clear:", "mov word ptr [child_off],0xeff2", "seed_chain:",
           "mov bx,word ptr [record_off]", "mov ax,word ptr [child_off]",
           "mov word ptr [bx+0x108],ax", "mov ax,cs", "mov word ptr [bx+0x10a],ax", "seed_child:",
           "mov bx,word ptr [child_off]", "mov ax,word ptr [target_off]",
           "mov word ptr [bx+0xa],ax", "mov ax,word ptr [target_seg]", "mov word ptr [bx+0xc],ax",
           # Exact +0553 suffix reads the original caller's BP-6/BP-4 locals.
           "mov ax,word ptr [record_off]", "mov word ptr [0x7771],ax", "mov ax,cs",
           "mov word ptr [0x7773],ax", "inc ax", "mov es,ax", "mov ax,word ptr [index]",
           "xor dx,dx", "mov bx,6", "div bx", "mov si,dx", "shl si,1",
           "push word ptr [flags_table+si]", "popf", "mov ax,0xabcd", "mov bx,0x3456",
           "mov cx,0x5678", "mov dx,0x789a", "mov si,0x9abc", "mov di,0xbcde",
           "mov bp,0x7777", "jmp original_caller", "capture:"]
    for index, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned", "mov word ptr [output+20],ds",
            "mov word ptr [output+22],es", "mov word ptr [output+24],ss"]
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
            "header: .space 6", "handle: .word 0", "index: .word 0", "record_off: .word 0",
            "child_off: .word 0", "target_off: .word 0", "target_seg: .word 0",
            f"output: .space {RECORD_SIZE}", "flags_table: .word " + ",".join(str(x) for x in FLAGS)]
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
    if count != COUNT or return_ip != 0x055D:
        raise ValueError("native caller count/return differs")
    lib = library(directory)
    for case in range(COUNT):
        ro, co, to, ts = 0x9000, 0x9200, 0x9400, segment
        left, right = case & 255, case & 255
        if 256 <= case < 512:
            right ^= 0x80
        elif case >= 512:
            left = right = 7
            if case == 512:
                to = ts = 0
            elif case == 513:
                to, ts = 0, (segment+0x900) & 0xFFFF
            elif case == 514:
                to, ts, left = 0x8400, (segment+0x100) & 0xFFFF, 8
            elif case == 515:
                ro = 0xEEE7
            elif case == 516:
                ro = 0xEEED
            else:
                co = 0xEFF2
        memory = bytearray([0xA5])*SIZE
        at = physical(segment,0x100)
        memory[at:at+len(code)] = code
        memory[physical(segment,0x950E)] = left
        memory[physical(segment,0x910E)] = right
        effective_record = 0xEE01 if case == 516 else ro
        put16(memory, segment, (effective_record+0x108) & 0xFFFF, co)
        put16(memory, segment, (effective_record+0x10A) & 0xFFFF, segment)
        if case in (515,516):
            memory[physical(segment,(effective_record+0x10E) & 0xFFFF)] = right
        put16(memory, segment, (co+0x0A) & 0xFFFF, to)
        put16(memory, segment, (co+0x0C) & 0xFFFF, ts)
        put16(memory, segment, 0x7771, ro)
        put16(memory, segment, 0x7773, segment)
        for off, value in ((0xEFF8,return_ip),(0xEFFA,segment),(0xEFFC,ro),(0xEFFE,segment)):
            put16(memory,segment,off,value)
        cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                  0x7777,0xEFF8,segment,0x122B,segment,(segment+1) & 0xFFFF,
                  segment,FLAGS[case % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if lib.pr_u2f_record_link_clear_122b(ram,SIZE,ctypes.byref(cpu)):
            raise ValueError(f"C rejected native case {case}")
        observed = result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        mismatch = 256 <= case < 512 or case == 514
        for (name,_), expected in zip(Cpu._fields_,struct.unpack_from("<14H",observed)):
            mask = 0xFFEF if name == "flags" and not mismatch else 0xFFFF
            if (getattr(cpu,name) ^ expected) & mask:
                raise ValueError(f"native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}")
        at = physical(segment,AUX)
        if memory[at:at+AUX_SIZE] != observed[28:28+AUX_SIZE]:
            raise ValueError(f"native {case} stack/alias region differs")
        at = physical(segment,DATA)
        if memory[at:at+DATA_SIZE] != observed[28+AUX_SIZE:]:
            raise ValueError(f"native {case} record region differs")
    report = dict(target="OVL_002F_122B",source_sha256=BODY_SHA,native_cases=count,
                  original_caller_suffix="U2F:+0553..055C",caller_return_ip="055D",
                  actual_push_cs_near_call=True,reached_game_route=False,interrupts_isolated=True,
                  compared_cpu_words=14,compared_stack_alias_bytes=AUX_SIZE,compared_record_bytes=DATA_SIZE,
                  undefined_af_masked_on_logic_exits=True,cmp_flags_masked=0,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    main()
