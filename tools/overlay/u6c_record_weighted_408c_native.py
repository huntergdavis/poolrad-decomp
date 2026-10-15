#!/usr/bin/env python3
"""Native weighted caller with separate loaded code and real INT 0 capture."""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u6c_record_weighted_408c_audit import Cpu, ROOT, SIZE, BODY_SHA, library, physical, put16, source

COUNT, FRAME, FRAME_SIZE = 1029,0xEFD0,64
TABLE,TABLE_SIZE,DATA,DATA_SIZE = 0x408E,256,0x9000,0x600
RECORD_SIZE = 30+FRAME_SIZE+TABLE_SIZE+DATA_SIZE
FLAGS = (0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)


def case_data(case):
    mode,argument,active,table = 0,[4]*8,[4]*8,3
    if case < 768:
        argument = active = [(1,4,127)[case//256]]*8
        table = case & 255
    elif case < 1024:
        argument = active = [4 if (case & 255) & (1 << i) else 0 for i in range(8)]
    elif case == 1024:
        argument = active = [0]*8
    elif case == 1025:
        mode,argument,active,table = 1,[0]*8,[1]*8,14
    elif case == 1026:
        mode,argument,active = 1,[1]*8,[0]*8
    elif case == 1027:
        mode,argument,active = 1,[0]*8,[1]*8
    else:
        mode,argument,active,table = 1,[0,0,127,0,0,0,0,0],[1]*8,13
    return 0x9000,0x9400 if mode else 0x9000,mode,18,2,table,argument,active


def main():
    directory = ROOT / "resources/u6c-408c-parity/native"
    directory.mkdir(parents=True,exist_ok=True)
    body,_,meta = source()
    code = bytearray(0x5000)
    code[0x3F31:0x3F31+len(meta["leaf"])] = meta["leaf"]
    code[0x408C:0x4235] = body
    code[0x2469:0x2475] = meta["caller_suffix"]
    code[0x2475:0x247A] = bytes([0xEA,0,0,0,0]) # Transparent synthetic capture jump.
    (directory / "CODE.BIN").write_bytes(code)
    fixtures = bytearray()
    for case in range(COUNT):
        ro,ao,m,t,k,v,g,ag = case_data(case)
        fixtures += struct.pack("<HH20B",ro,ao,m,t,k,v,*g,*ag)
    (directory / "CASES.BIN").write_bytes(fixtures)
    asm = [".intel_syntax noprefix",".code16",".global _start","_start: jmp main",
           ".org 0x1000","main:","mov ax,cs","mov ds,ax","mov es,ax","mov ss,ax","mov sp,0xf000",
           "mov word ptr [header],ax","mov bx,0x1000","mov ah,0x4a","int 0x21","jc failure",
           "mov bx,0x500","mov ah,0x48","int 0x21","jc failure","mov word ptr [header+2],ax",
           "mov word ptr [far_dispatch+2],ax","mov word ptr [direct_segment],ax",
           "mov word ptr [header+4],offset normal_from_direct",f"mov word ptr [header+6],{COUNT}",
           "mov dx,offset code_name","mov ax,0x3d00","int 0x21","jc failure","mov bx,ax","push ds",
           "mov ax,word ptr [header+2]","mov ds,ax","xor dx,dx","mov cx,0x5000","mov ah,0x3f",
           "int 0x21","pop ds","jc failure","cmp ax,0x5000","jne failure","mov ah,0x3e","int 0x21",
           "mov ax,word ptr [header+2]","mov es,ax","mov word ptr es:[0x2476],offset normal_from_code",
           "mov ax,cs","mov word ptr es:[0x2478],ax","mov ax,0x3500","int 0x21",
           "mov word ptr [old_offset],bx","mov word ptr [old_segment],es",
           "mov dx,offset fault_handler","mov ax,0x2500","int 0x21",
           "mov dx,offset cases_name","mov ax,0x3d00","int 0x21","jc failure","mov word ptr [cases_handle],ax",
           "mov dx,offset filename","xor cx,cx","mov ah,0x3c","int 0x21","jc failure",
           "mov word ptr [handle],ax","mov bx,ax","mov dx,offset header","mov cx,8","mov ah,0x40","int 0x21",
           "again:","mov bx,word ptr [cases_handle]","mov dx,offset fixture","mov cx,24","mov ah,0x3f",
           "int 0x21","jc failure","cmp ax,24","jne failure","cli","mov sp,0xf000","push cs","pop es","cld",
           f"mov di,{FRAME}",f"mov cx,{FRAME_SIZE}","mov al,0xa5","rep stosb",
           f"mov di,{DATA}",f"mov cx,{DATA_SIZE}","rep stosb",f"mov di,{TABLE}",f"mov cx,{TABLE_SIZE}",
           "mov al,3","rep stosb","mov bx,word ptr [fixture]","lea di,[bx+0x96]",
           "mov si,offset fixture+8","mov cx,8","rep movsb","mov bx,word ptr [fixture+2]",
           "mov word ptr [0x5d92],bx","mov ax,cs","mov word ptr [0x5d94],ax",
           "mov al,byte ptr [fixture+5]","mov byte ptr [bx+0x14],al",
           "mov al,byte ptr [fixture+6]","mov byte ptr [bx+0x2f],al","lea di,[bx+0x96]",
           "mov si,offset fixture+16","mov cx,8","rep movsb","xor bx,bx","mov bl,byte ptr [fixture+5]",
           "mov al,byte ptr [fixture+7]","mov byte ptr [bx+0x408e],al",
           "mov word ptr [mode_dispatch],offset jump_original","cmp byte ptr [fixture+4],0","je mode_ready",
           "mov word ptr [mode_dispatch],offset direct_entry","mode_ready:","mov ax,word ptr [index]",
           "xor dx,dx","mov bx,6","div bx","mov si,dx","shl si,1","mov ax,cs","inc ax","mov es,ax",
           "push word ptr [flags_table+si]","popf","mov ax,0xabcd","mov bx,0x3456","mov cx,0x5678",
           "mov dx,0x789a","mov si,0x9abc","mov di,0xbcde","mov bp,0x7777","jmp word ptr [mode_dispatch]",
           "jump_original:",".byte 0xff,0x2e",".word far_dispatch","direct_entry:","push ds",
           "push word ptr [fixture]",".byte 0x9a",".word 0x408c","direct_segment: .word 0",
           "normal_from_direct:","mov word ptr [output],ax","mov word ptr [output+16],cs",
           "mov word ptr [output+18],offset normal_from_direct","jmp capture_rest",
           "normal_from_code:","mov word ptr [output],ax","mov ax,word ptr [header+2]",
           "mov word ptr [output+16],ax","mov word ptr [output+18],0x2475","capture_rest:"]
    for index,reg in enumerate("ax bx cx dx si di bp sp".split()):
        if index: asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+20],ds","mov word ptr [output+22],es","mov word ptr [output+24],ss",
            "mov sp,0xf800","pushf","pop ax","mov word ptr [output+26],ax","mov word ptr [output+28],0",
            "jmp capture_regions","fault_handler:"]
    for index,reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+20],ds","mov word ptr [output+22],es","mov word ptr [output+24],ss",
            "mov bx,sp","mov ax,word ptr ss:[bx]","mov word ptr [output+18],ax",
            "mov ax,word ptr ss:[bx+2]","mov word ptr [output+16],ax",
            "mov ax,word ptr ss:[bx+4]","mov word ptr [output+26],ax","lea ax,[bx+6]",
            "mov word ptr [output+14],ax","mov word ptr [output+28],2",
            "mov ax,word ptr [header+2]","mov word ptr ss:[bx],ax","mov ax,cs",
            "mov word ptr ss:[bx+4],ax","mov ax,word ptr [0x5d92]","mov word ptr ss:[bx+2],ax",
            "mov sp,0xf800","capture_regions:","push cs","pop es","cld",f"mov si,{FRAME}",
            "mov di,offset output+30",f"mov cx,{FRAME_SIZE}","rep movsb",f"mov si,{TABLE}",
            f"mov cx,{TABLE_SIZE}","rep movsb",f"mov si,{DATA}",f"mov cx,{DATA_SIZE}","rep movsb","sti",
            "mov bx,word ptr [handle]","mov dx,offset output",f"mov cx,{RECORD_SIZE}","mov ah,0x40",
            "int 0x21","jc failure",f"cmp ax,{RECORD_SIZE}","jne failure","inc word ptr [index]",
            f"cmp word ptr [index],{COUNT}","jb again","mov bx,word ptr [handle]","mov ah,0x3e",
            "int 0x21","mov bx,word ptr [cases_handle]","mov ah,0x3e","int 0x21",
            "mov dx,word ptr [old_offset]","mov ax,word ptr [old_segment]","push ds","mov ds,ax",
            "mov ax,0x2500","int 0x21","pop ds","mov ax,0x4c00","int 0x21","failure:",
            "mov ax,0x4c01","int 0x21",'filename: .asciz "RESULT.BIN"','code_name: .asciz "CODE.BIN"',
            'cases_name: .asciz "CASES.BIN"',"header: .space 8","handle: .word 0","cases_handle: .word 0",
            "old_offset: .word 0","old_segment: .word 0","index: .word 0","fixture: .space 24",
            "mode_dispatch: .word 0","far_dispatch: .word 0x2469,0",f"output: .space {RECORD_SIZE}",
            "flags_table: .word "+",".join(str(x) for x in FLAGS)]
    assembly,obj,com = directory / "oracle.S",directory / "oracle.o",directory / "ORACLE.COM"
    assembly.write_text("\n".join(asm)+"\n")
    subprocess.run(["as","--32",str(assembly),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    if len(com.read_bytes())+0x100 >= TABLE:
        raise ValueError("synthetic program overlaps DS lookup table")
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\n"
        "cputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
        "[autoexec]\nmount c "+str(directory)+"\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    result_path.unlink(missing_ok=True)
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
            env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy"),
            stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    check_native(directory,result_path.read_bytes(),com.read_bytes(),code)


def check_native(directory,result,program,code):
    if len(result) != 8+COUNT*RECORD_SIZE:
        raise ValueError(f"native output incomplete: {len(result)}/{8+COUNT*RECORD_SIZE}")
    segment,code_segment,direct_return,count = struct.unpack_from("<4H",result)
    if count != COUNT or code_segment == segment:
        raise ValueError("native count/code-data separation differs")
    lib = library(directory)
    totals = dict(returns=0,divide_errors=0,divisions=0,clamps=0)
    for case in range(COUNT):
        ro,ao,mode,t,k,v,gates,active_gates = case_data(case)
        memory = bytearray([0xA5])*SIZE
        at = physical(segment,0x100)
        memory[at:at+len(program)] = program
        at = physical(code_segment,0)
        memory[at:at+len(code)] = code
        memory[physical(segment,TABLE):physical(segment,TABLE)+TABLE_SIZE] = bytes([3])*TABLE_SIZE
        for i,value in enumerate(gates):
            memory[physical(segment,ro+0x96+i)] = value
        memory[physical(segment,ao+0x14)] = t
        memory[physical(segment,ao+0x2F)] = k
        for i,value in enumerate(active_gates):
            memory[physical(segment,ao+0x96+i)] = value
        memory[physical(segment,TABLE+t)] = v
        put16(memory,segment,0x5D92,ao)
        put16(memory,segment,0x5D94,segment)
        return_cs,return_ip = (segment,direct_return) if mode else (code_segment,0x2475)
        for off,value in ((0xEFF8,return_ip),(0xEFFA,return_cs),(0xEFFC,ro),(0xEFFE,segment)):
            put16(memory,segment,off,value)
        cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                  0x7777,0xEFF8,code_segment,0x408C,segment,(segment+1) & 0xFFFF,segment,FLAGS[case % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        status = lib.pr_u6c_record_weighted_408c(ram,SIZE,ctypes.byref(cpu))
        observed = result[8+case*RECORD_SIZE:8+(case+1)*RECORD_SIZE]
        actual_status = struct.unpack_from("<H",observed,28)[0]
        if status != actual_status or status not in (0,2):
            raise ValueError(f"native {case} status: {status}/{actual_status}")
        totals["returns" if status == 0 else "divide_errors"] += 1
        divided = status == 0 and cpu.dx < cpu.cx
        if status == 0: totals["divisions" if divided else "clamps"] += 1
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",observed)):
            mask = (0xFFFF ^ 0x08D5) if name == "flags" and divided else 0xFFFF
            if (getattr(cpu,name) ^ expected) & mask:
                raise ValueError(f"native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}")
        at = physical(segment,FRAME)
        if memory[at:at+FRAME_SIZE] != observed[30:30+FRAME_SIZE]:
            raise ValueError(f"native {case} frame differs")
        at = physical(segment,TABLE)
        if memory[at:at+TABLE_SIZE] != observed[30+FRAME_SIZE:30+FRAME_SIZE+TABLE_SIZE]:
            raise ValueError(f"native {case} table differs")
        at = physical(segment,DATA)
        if memory[at:at+DATA_SIZE] != observed[30+FRAME_SIZE+TABLE_SIZE:]:
            raise ValueError(f"native {case} record region differs")
    if any(n == 0 for n in totals.values()): raise ValueError("native exit coverage incomplete")
    report = dict(target="OVL_006C_408C",source_sha256=BODY_SHA,native_cases=count,**totals,
                  original_caller_cases=1025,distinct_record_direct_cases=4,separate_allocated_code_segment=True,
                  original_caller_suffix="U6C:+2469..2474",original_return_ip="2475",
                  reached_game_route=False,interrupts_isolated=True,compared_cpu_words=14,
                  compared_frame_bytes=FRAME_SIZE,compared_table_bytes=TABLE_SIZE,compared_record_bytes=DATA_SIZE,
                  undefined_arithmetic_flags_masked_only_after_idiv=True,fault_frame_normalized_words=3,
                  result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__": main()
