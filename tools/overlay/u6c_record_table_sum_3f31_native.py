#!/usr/bin/env python3
"""IRQ-isolated original U6C caller/leaf parity; natural reach is unmeasured."""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u6c_record_table_sum_3f31_audit import Cpu, ROOT, SIZE, BODY_SHA, library, physical, put16, source

COUNT, FRAME, FRAME_SIZE = 1282, 0xEFD0, 64
TABLE, TABLE_SIZE, RECORD, RECORD_BYTES = 0x408E, 256, 0x9000, 0x120
RECORD_SIZE = 28 + FRAME_SIZE + TABLE_SIZE + RECORD_BYTES
FLAGS = (0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)


def main():
    directory = ROOT / "resources/u6c-3f31-parity/native"
    directory.mkdir(parents=True,exist_ok=True)
    body,_,metadata = source()
    body_path,caller_path = directory / "body.bin",directory / "caller.bin"
    body_path.write_bytes(body)
    caller_path.write_bytes(metadata["caller_suffix"])
    asm = [".intel_syntax noprefix",".code16",".global _start","_start: jmp main",
           ".org 0x2edf","original_caller:",'.incbin "'+str(caller_path)+'"',
           "returned: jmp capture",".org 0x3e31",'.incbin "'+str(body_path)+'"',
           ".org 0x6000","main:","mov ax,cs","mov ds,ax","mov ss,ax","mov sp,0xf000",
           "mov word ptr [header],ax","mov word ptr [header+2],offset returned",
           f"mov word ptr [header+4],{COUNT}","mov dx,offset filename","xor cx,cx",
           "mov ah,0x3c","int 0x21","jc failure","mov word ptr [handle],ax",
           "mov bx,ax","mov dx,offset header","mov cx,6","mov ah,0x40","int 0x21",
           "again:","cli","mov sp,0xf000","push cs","pop es","cld",
           f"mov di,{FRAME}",f"mov cx,{FRAME_SIZE}","mov al,0xa5","rep stosb",
           f"mov di,{RECORD}",f"mov cx,{RECORD_BYTES}","rep stosb",
           f"mov di,{TABLE}",f"mov cx,{TABLE_SIZE}","mov al,3","rep stosb",
           "mov word ptr [0x5d92],0x9000","mov ax,cs","mov word ptr [0x5d94],ax",
           "mov word ptr [before_sp],0xf000","mov byte ptr [0x9014],18","mov byte ptr [0x902f],2",
           "mov di,0x9096","mov cx,8","mov al,1","rep stosb","mov ax,word ptr [index]",
           "cmp ax,512","jae not_type","mov byte ptr [0x9014],al","cmp ax,256","jb ready",
           "mov byte ptr [0x902f],3","jmp ready","not_type:","cmp ax,768","jae not_table",
           "mov byte ptr [0x40a0],al","jmp ready","not_table:","cmp ax,1024","jae not_uniform_gate",
           "mov di,0x9096","mov cx,8","rep stosb","jmp ready","not_uniform_gate:",
           "cmp ax,1280","jae alias_cases","mov bl,al","mov di,0x9096","mov cx,8",
           "fill_enabled:","mov al,bl","and al,1","stosb","shr bl,1","loop fill_enabled",
           "jmp ready","alias_cases:","jne record_alias","mov word ptr [before_sp],0x40ac",
           "jmp ready","record_alias:","mov word ptr [0x5d92],0xefe0","mov byte ptr [0xf00f],2",
           "mov di,0xf076","mov cx,8","mov al,1","rep stosb","ready:",
           "mov ax,word ptr [index]","xor dx,dx","mov bx,6","div bx","mov si,dx","shl si,1",
           "mov ax,cs","inc ax","mov es,ax","mov sp,word ptr [before_sp]",
           "push word ptr [flags_table+si]","popf","mov ax,0xabcd","mov bx,0x3456",
           "mov cx,0x5678","mov dx,0x789a","mov si,0x9abc","mov di,0xbcde",
           "mov bp,0x7777","jmp original_caller","capture:"]
    for index,reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned","mov word ptr [output+20],ds",
            "mov word ptr [output+22],es","mov word ptr [output+24],ss"]
    for offset in range(0,FRAME_SIZE,2):
        asm += [f"mov ax,word ptr [{FRAME+offset}]",f"mov word ptr [output+{28+offset}],ax"]
    # PUSHF at 40AA would overwrite the table-alias return frame. Move SP
    # to F000 after recording CPU.SP, then capture flags without arithmetic.
    asm += ["mov sp,0xf000","pushf","pop ax","mov word ptr [output+26],ax","push cs","pop es","cld",
            f"mov si,{TABLE}",f"mov di,offset output+{28+FRAME_SIZE}",f"mov cx,{TABLE_SIZE}","rep movsb",
            f"mov si,{RECORD}",f"mov cx,{RECORD_BYTES}","rep movsb","sti",
            "mov bx,word ptr [handle]","mov dx,offset output",f"mov cx,{RECORD_SIZE}",
            "mov ah,0x40","int 0x21","jc failure",f"cmp ax,{RECORD_SIZE}","jne failure",
            "inc word ptr [index]",f"cmp word ptr [index],{COUNT}","jb again",
            "mov bx,word ptr [handle]","mov ah,0x3e","int 0x21","mov ax,0x4c00","int 0x21",
            "failure:","mov ax,0x4c01","int 0x21",'filename: .asciz "RESULT.BIN"',
            "header: .space 6","handle: .word 0","index: .word 0","before_sp: .word 0",
            f"output: .space {RECORD_SIZE}","flags_table: .word "+",".join(str(x) for x in FLAGS)]
    assembly,obj,com = directory / "oracle.S",directory / "oracle.o",directory / "ORACLE.COM"
    assembly.write_text("\n".join(asm)+"\n")
    subprocess.run(["as","--32",str(assembly),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\n"
        "cputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
        "[autoexec]\nmount c "+str(directory)+"\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
            env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy"),
            stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
    check_native(directory,result_path.read_bytes(),com.read_bytes())


def check_native(directory,result,code):
    if len(result) != 6+COUNT*RECORD_SIZE:
        raise ValueError("native output incomplete")
    segment,return_ip,count = struct.unpack_from("<3H",result)
    if count != COUNT or return_ip != 0x2FEB:
        raise ValueError("native caller count/return differs")
    lib = library(directory)
    for case in range(COUNT):
        memory = bytearray([0xA5])*SIZE
        at = physical(segment,0x100)
        memory[at:at+len(code)] = code
        memory[physical(segment,TABLE):physical(segment,TABLE)+TABLE_SIZE] = bytes([3])*TABLE_SIZE
        memory[physical(segment,0x9096):physical(segment,0x9096)+8] = bytes([1])*8
        memory[physical(segment,0x9014)] = case & 255 if case < 512 else 18
        memory[physical(segment,0x902F)] = 3 if 256 <= case < 512 else 2
        if 512 <= case < 768:
            memory[physical(segment,0x40A0)] = case & 255
        if 768 <= case < 1024:
            memory[physical(segment,0x9096):physical(segment,0x9096)+8] = bytes([case & 255])*8
        if 1024 <= case < 1280:
            for i in range(8):
                memory[physical(segment,0x9096+i)] = 1 if (case & 255) & (1 << i) else 0
        ro = 0xEFE0 if case == 1281 else 0x9000
        if case == 1281:
            memory[physical(segment,0xF00F)] = 2
            memory[physical(segment,0xF076):physical(segment,0xF076)+8] = bytes([1])*8
        put16(memory,segment,0x5D92,ro)
        put16(memory,segment,0x5D94,segment)
        before_sp = 0x40AC if case == 1280 else 0xF000
        sp = before_sp-8
        for off,value in ((sp,return_ip),(sp+2,segment),(sp+4,ro),(sp+6,segment)):
            put16(memory,segment,off,value)
        cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                  0x7777,sp,segment,0x3F31,segment,(segment+1) & 0xFFFF,segment,FLAGS[case % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        if lib.pr_u6c_record_table_sum_3f31(ram,SIZE,ctypes.byref(cpu)):
            raise ValueError(f"C rejected native case {case}")
        if case in (1280,1281) and cpu.ax != (254 if case == 1280 else 30):
            raise ValueError("alias fixture did not exercise intended repeated reloads")
        observed = result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",observed)):
            if getattr(cpu,name) != expected:
                raise ValueError(f"native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}")
        at = physical(segment,FRAME)
        if memory[at:at+FRAME_SIZE] != observed[28:28+FRAME_SIZE]:
            raise ValueError(f"native {case} frame differs")
        at = physical(segment,TABLE)
        if memory[at:at+TABLE_SIZE] != observed[28+FRAME_SIZE:28+FRAME_SIZE+TABLE_SIZE]:
            raise ValueError(f"native {case} table/alias region differs")
        at = physical(segment,RECORD)
        if memory[at:at+RECORD_BYTES] != observed[28+FRAME_SIZE+TABLE_SIZE:]:
            raise ValueError(f"native {case} record differs")
    report = dict(target="OVL_006C_3F31",source_sha256=BODY_SHA,native_cases=count,
                  original_caller_suffix="U6C:+2FDF..2FEA",caller_return_ip="2FEB",
                  actual_push_cs_near_call=True,reached_game_route=False,interrupts_isolated=True,
                  compared_cpu_words=14,compared_frame_bytes=FRAME_SIZE,compared_table_bytes=TABLE_SIZE,
                  compared_record_bytes=RECORD_BYTES,masked_flags=0,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    main()
