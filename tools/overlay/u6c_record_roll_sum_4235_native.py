#!/usr/bin/env python3
"""Native original caller prefix, raw dice and raw SYSTEM seed/frame evidence."""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u6c_record_roll_sum_4235_audit import Cpu, Layout, ROOT, SIZE, BODY_SHA, library, physical, put16, source, install

COUNT,FRAME,FRAME_SIZE = 620,0xEFC0,64
REGIONS = [(FRAME,FRAME_SIZE),(0x9000,256),(0x05DE,8),(0x08B6,16),(0x4428,4)]
RECORD_SIZE = 28+sum(n for _,n in REGIONS)
FLAGS = (0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)


def case_data(case):
    mask,ro,seed,gate,count,range_byte = 255,0x9000,0xBB36E7A1,1,1,6
    if case < 256:mask=case
    elif case < 512:range_byte=case-256;gate=1 if case & 1 else 2
    elif case < 576:count=(0,1,2,3,8,127,128,255)[(case-512)//8];range_byte=(0,1,6,255)[case % 4];gate=1 if case & 4 else 2
    elif case < 608:gate=(0,1,2,127,128,129,254,255)[(case-576)//4];seed=(0,1,0xFFFFFFFF,0x80000000)[case % 4]
    elif case < 616:ro=(0xEF3A,0xEF4C,0xEF50,0x4392)[(case-608)//2];gate=1 if case & 1 else 2
    else:seed=(0x00005733,0xFFFF5733,0x1234FFFF,0xFFFFFFFF)[case-616];count=3
    return mask,ro,seed,[gate]*8,[count]*8,[range_byte]*8,[1<<i for i in range(8)]


def main():
    directory = ROOT / "resources/u6c-4235-parity/native"
    directory.mkdir(parents=True,exist_ok=True)
    bodies,_,meta = source()
    code6,code9,codes = bytearray(0x5000),bytearray(0x1000),bytearray(0x1400)
    code6[0x4235:0x42ED]=bodies["caller"]
    code6[0x2FA4:0x2FB4]=meta["caller_prefix"]
    code6[0x2FB4:0x2FB9]=bytes([0xEA,0,0,0,0]) # Transparent capture jump after original return.
    code9[0xDE3:0xE2E]=bodies["dice"];code9[0x48:0x4B]=b"\xE9\x98\x0D"
    codes[0x11C0:0x11D6]=bodies["random"];codes[0x120F:0x1245]=bodies["seed"];codes[0x1245:0x1247]=meta["multiplier"]
    for name,data in (("CODE6.BIN",code6),("CODE9.BIN",code9),("CODES.BIN",codes)):(directory/name).write_bytes(data)
    fixtures = b"".join(struct.pack("<HHI32B",m,ro,sd,*g,*c,*r,*b) for m,ro,sd,g,c,r,b in map(case_data,range(COUNT)))
    (directory/"CASES.BIN").write_bytes(fixtures)
    asm = [".intel_syntax noprefix",".code16",".global _start","_start: jmp main",".org 0x1000","main:",
        "mov ax,cs","mov ds,ax","mov es,ax","mov ss,ax","mov sp,0xf000","mov word ptr [header],ax",
        "mov bx,0x1000","mov ah,0x4a","int 0x21","jc failure"]
    for i,(paras,size,name) in enumerate(((0x500,0x5000,"code6_name"),(0x100,0x1000,"code9_name"),(0x140,0x1400,"codes_name")),1):
        asm += [f"mov bx,{paras}","mov ah,0x48","int 0x21","jc failure",f"mov word ptr [header+{2*i}],ax",
            f"mov dx,offset {name}","mov ax,0x3d00","int 0x21","jc failure","mov bx,ax","push ds",
            f"mov ax,word ptr [header+{2*i}]","mov ds,ax","xor dx,dx",f"mov cx,{size}","mov ah,0x3f",
            "int 0x21","pop ds","jc failure",f"cmp ax,{size}","jne failure","mov ah,0x3e","int 0x21"]
    asm += ["mov ax,word ptr [header+2]","mov word ptr [far_dispatch+2],ax","mov es,ax",
        "mov word ptr es:[0x2fb5],offset capture","mov ax,cs","mov word ptr es:[0x2fb7],ax",
        "mov ax,word ptr [header+4]","mov word ptr es:[0x4283],ax","mov es,ax",
        "mov ax,word ptr [header+6]","mov word ptr es:[0xe11],ax",
        f"mov word ptr [header+8],{COUNT}","mov dx,offset cases_name","mov ax,0x3d00","int 0x21","jc failure",
        "mov word ptr [cases_handle],ax","mov dx,offset filename","xor cx,cx","mov ah,0x3c","int 0x21","jc failure",
        "mov word ptr [handle],ax","mov bx,ax","mov dx,offset header","mov cx,10","mov ah,0x40","int 0x21",
        "again:","mov bx,word ptr [cases_handle]","mov dx,offset fixture","mov cx,40","mov ah,0x3f","int 0x21",
        "jc failure","cmp ax,40","jne failure","cli","mov sp,0xf000","push cs","pop es","cld",
        f"mov di,{FRAME}",f"mov cx,{FRAME_SIZE}","mov al,0xa5","rep stosb","mov di,0x9000","mov cx,256","rep stosb",
        "mov si,offset fixture+8","mov bx,word ptr [fixture+2]","lea di,[bx+0x96]","mov cx,8","rep movsb",
        "mov di,0x8b6","mov cx,8","rep movsb","mov di,0x8be","mov cx,8","rep movsb",
        "mov di,0x5de","mov cx,8","rep movsb","mov ax,word ptr [fixture+4]","mov word ptr [0x4428],ax",
        "mov ax,word ptr [fixture+6]","mov word ptr [0x442a],ax","mov ax,word ptr [fixture+2]",
        "mov word ptr [0x5d92],ax","mov ax,cs","mov word ptr [0x5d94],ax","mov ax,word ptr [fixture]",
        "mov byte ptr [0x776b],al","mov ax,word ptr [index]","xor dx,dx","mov bx,6","div bx","mov si,dx",
        "shl si,1","mov ax,cs","inc ax","mov es,ax","push word ptr [flags_table+si]","popf",
        "mov ax,0xabcd","mov bx,0x3456","mov cx,0x5678","mov dx,0x789a","mov si,0x9abc","mov di,0xbcde",
        "mov bp,0x7777",".byte 0xff,0x2e",".word far_dispatch","capture:","mov word ptr [output],ax",
        "mov ax,word ptr [header+2]","mov word ptr [output+16],ax","mov word ptr [output+18],0x2fb4"]
    for index,reg in enumerate("ax bx cx dx si di bp sp".split()):
        if index:asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+20],ds","mov word ptr [output+22],es","mov word ptr [output+24],ss",
        "mov sp,0xf800","pushf","pop ax","mov word ptr [output+26],ax","push cs","pop es","cld","mov di,offset output+28"]
    for offset,length in REGIONS:asm += [f"mov si,{offset}",f"mov cx,{length}","rep movsb"]
    asm += ["sti","mov bx,word ptr [handle]","mov dx,offset output",f"mov cx,{RECORD_SIZE}","mov ah,0x40","int 0x21",
        "jc failure",f"cmp ax,{RECORD_SIZE}","jne failure","inc word ptr [index]",f"cmp word ptr [index],{COUNT}","jb again",
        "mov bx,word ptr [handle]","mov ah,0x3e","int 0x21","mov bx,word ptr [cases_handle]","mov ah,0x3e","int 0x21",
        "mov ax,0x4c00","int 0x21","failure:","mov ax,0x4c01","int 0x21",'filename: .asciz "RESULT.BIN"',
        'code6_name: .asciz "CODE6.BIN"','code9_name: .asciz "CODE9.BIN"','codes_name: .asciz "CODES.BIN"',
        'cases_name: .asciz "CASES.BIN"',"header: .space 10","handle: .word 0","cases_handle: .word 0","index: .word 0",
        "fixture: .space 40","far_dispatch: .word 0x2fa4,0",f"output: .space {RECORD_SIZE}",
        "flags_table: .word "+",".join(str(x) for x in FLAGS)]
    assembly,obj,com = directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    assembly.write_text("\n".join(asm)+"\n")
    subprocess.run(["as","--32",str(assembly),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    if len(com.read_bytes())+0x100 >= 0x4428:raise ValueError("synthetic program overlaps seed")
    config = directory/"dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
        "[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c "+str(directory)+"\nc:\nORACLE.COM\nexit\n")
    result_path = directory/"RESULT.BIN";result_path.unlink(missing_ok=True)
    with (directory/"dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
            env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy"),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    check_native(directory,result_path.read_bytes(),com.read_bytes(),bodies)


def check_native(directory,result,program,bodies):
    if len(result) != 10+COUNT*RECORD_SIZE:raise ValueError(f"native output incomplete: {len(result)}/{10+COUNT*RECORD_SIZE}")
    segment,code_cs,dice_cs,system_cs,count = struct.unpack_from("<5H",result)
    if count != COUNT or len({segment,code_cs,dice_cs,system_cs}) != 4:raise ValueError("native segment layout differs")
    layout,lib = Layout(dice_cs,system_cs),library(directory)
    for case in range(COUNT):
        mask,ro,seed,gates,counts,ranges,bits = case_data(case)
        memory=bytearray([0xA5])*SIZE
        at=physical(segment,0x100);memory[at:at+len(program)]=program
        for offset,length in REGIONS:memory[physical(segment,offset):physical(segment,offset)+length]=bytes([0xA5])*length
        for i,value in enumerate(gates):memory[physical(segment,(ro+0x96+i)&0xFFFF)]=value
        for base,values in ((0x8B6,counts),(0x8BE,ranges),(0x5DE,bits)):
            for i,value in enumerate(values):memory[physical(segment,base+i)]=value
        put16(memory,segment,0x4428,seed & 0xFFFF);put16(memory,segment,0x442A,seed>>16)
        for off,value in ((0xEFF6,0x2FB4),(0xEFF8,code_cs),(0xEFFA,0xAB00|mask),(0xEFFC,ro),(0xEFFE,segment)):
            put16(memory,segment,off,value)
        install(memory,bodies,code_cs,layout)
        cpu=Cpu(0xAB00|mask,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0xEFF6,
            code_cs,0x4235,segment,(segment+1)&0xFFFF,segment,FLAGS[case % len(FLAGS)])
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        status=lib.pr_u6c_record_roll_sum_4235(ram,SIZE,ctypes.byref(cpu),ctypes.byref(layout))
        if status:raise ValueError(f"native {case} C status {status}")
        observed=result[10+case*RECORD_SIZE:10+(case+1)*RECORD_SIZE]
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",observed)):
            if getattr(cpu,name) != expected:raise ValueError(f"native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}")
        cursor=28
        for offset,length in REGIONS:
            at=physical(segment,offset)
            if memory[at:at+length] != observed[cursor:cursor+length]:raise ValueError(f"native {case} RAM region {offset:04X}")
            cursor+=length
    report=dict(target="OVL_006C_4235",source_sha256=BODY_SHA,native_cases=count,returns=count,
        original_caller_prefix="U6C:+2FA4..2FB3",original_return_ip="2FB4",separate_loaded_code_segments=3,
        compared_cpu_words=14,compared_frame_bytes=FRAME_SIZE,compared_record_bytes=256,
        compared_mask_table_bytes=8,compared_dice_table_bytes=16,compared_seed_bytes=4,masked_flags=0,
        transparent_loaded_entry_stub=True,loader_dispatch_measured=False,natural_game_route_measured=False,
        result_sha256=sha256(result).hexdigest())
    (directory/"report.json").write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":main()
