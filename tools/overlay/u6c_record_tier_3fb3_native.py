#!/usr/bin/env python3
"""Original U6C caller composition, including real INT 0 fault captures.

Fault captures restore the three known popped callee-frame words overwritten
by CPU exception entry, to compare the public pre-vector C boundary.
"""

import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess

from u6c_record_tier_3fb3_audit import Cpu, ROOT, SIZE, BODY_SHA, library, physical, put16, source

COUNT, FRAME, FRAME_SIZE = 1538, 0xEFD0, 64
TABLE, TABLE_SIZE, RECORD, RECORD_BYTES = 0x408E, 256, 0x9000, 0x120
RECORD_SIZE = 30+FRAME_SIZE+TABLE_SIZE+RECORD_BYTES
FLAGS = (0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)


def case_data(case):
    ro,type_byte,kind,table = 0x9000,18,2,3
    gates = [4]*8
    if case < 1024:
        gates,table = [(1,16,30,127)[case//256]]*8,case & 255
    elif case < 1280:
        type_byte = case & 255
    elif case < 1536:
        gates = [4 if (case & 255) & (1 << i) else 0 for i in range(8)]
    elif case == 1536:
        gates = [0x80]*8
    else:
        ro = 0xEF5E
    return ro,type_byte,kind,table,gates


def main():
    directory = ROOT / "resources/u6c-3fb3-parity/native"
    directory.mkdir(parents=True,exist_ok=True)
    body,_,meta = source()
    body_path,caller_path = directory / "body.bin",directory / "caller.bin"
    body_path.write_bytes(meta["leaf"]+body)
    caller_path.write_bytes(meta["caller_suffix"])
    fixtures = bytearray()
    for case in range(COUNT):
        ro,t,k,v,g = case_data(case)
        fixtures += struct.pack("<H12B",ro,t,k,v,*g,0)
    (directory / "CASES.BIN").write_bytes(fixtures)
    asm = [".intel_syntax noprefix",".code16",".global _start","_start: jmp main",
           ".org 0x2428","original_caller:",'.incbin "'+str(caller_path)+'"',
           "returned: jmp capture_normal",".org 0x3e31",'.incbin "'+str(body_path)+'"',
           ".org 0x6000","main:","mov ax,cs","mov ds,ax","mov ss,ax","mov sp,0xf000",
           "mov word ptr [header],ax","mov word ptr [header+2],offset returned",
           f"mov word ptr [header+4],{COUNT}","mov ax,0x3500","int 0x21",
           "mov word ptr [old_offset],bx","mov word ptr [old_segment],es",
           "mov dx,offset fault_handler","mov ax,0x2500","int 0x21",
           "mov dx,offset cases_name","mov ax,0x3d00","int 0x21","jc failure",
           "mov word ptr [cases_handle],ax","mov dx,offset filename","xor cx,cx",
           "mov ah,0x3c","int 0x21","jc failure","mov word ptr [handle],ax",
           "mov bx,ax","mov dx,offset header","mov cx,6","mov ah,0x40","int 0x21","again:",
           "mov bx,word ptr [cases_handle]","mov dx,offset fixture","mov cx,14","mov ah,0x3f",
           "int 0x21","jc failure","cmp ax,14","jne failure","cli","mov sp,0xf000",
           "push cs","pop es","cld",f"mov di,{FRAME}",f"mov cx,{FRAME_SIZE}","mov al,0xa5","rep stosb",
           f"mov di,{RECORD}",f"mov cx,{RECORD_BYTES}","rep stosb",f"mov di,{TABLE}",
           f"mov cx,{TABLE_SIZE}","mov al,3","rep stosb","mov bx,word ptr [fixture]",
           "mov word ptr [0x5d92],bx","mov ax,cs","mov word ptr [0x5d94],ax",
           "mov al,byte ptr [fixture+2]","mov byte ptr [bx+0x14],al",
           "mov al,byte ptr [fixture+3]","mov byte ptr [bx+0x2f],al","lea di,[bx+0x96]",
           "mov si,offset fixture+5","mov cx,8","rep movsb","xor bx,bx",
           "mov bl,byte ptr [fixture+2]","mov al,byte ptr [fixture+4]","mov byte ptr [bx+0x408e],al",
           "mov ax,word ptr [index]","xor dx,dx","mov bx,6","div bx","mov si,dx","shl si,1",
           "mov ax,cs","inc ax","mov es,ax","push word ptr [flags_table+si]","popf",
           "mov ax,0xabcd","mov bx,0x3456","mov cx,0x5678","mov dx,0x789a",
           "mov si,0x9abc","mov di,0xbcde","mov bp,0x7777","jmp original_caller","capture_normal:"]
    for index,reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+18],offset returned","mov word ptr [output+20],ds",
            "mov word ptr [output+22],es","mov word ptr [output+24],ss",
            "mov sp,0xf800","pushf","pop ax","mov word ptr [output+26],ax",
            "mov word ptr [output+28],0","jmp capture_regions","fault_handler:"]
    for index,reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        asm.append(f"mov word ptr [output+{2*index}],{reg}")
    asm += ["mov word ptr [output+20],ds","mov word ptr [output+22],es","mov word ptr [output+24],ss",
            "mov bx,sp","mov ax,word ptr ss:[bx]","mov word ptr [output+18],ax",
            "mov ax,word ptr ss:[bx+2]","mov word ptr [output+16],ax",
            "mov ax,word ptr ss:[bx+4]","mov word ptr [output+26],ax",
            "lea ax,[bx+6]","mov word ptr [output+14],ax","mov word ptr [output+28],2",
            # Restore the actual known pre-fault callee frame: CS, argument
            # offset, argument segment. These words were popped by RETF 4.
            "mov ax,cs","mov word ptr ss:[bx],ax","mov word ptr ss:[bx+4],ax",
            "mov ax,word ptr [0x5d92]","mov word ptr ss:[bx+2],ax","mov sp,0xf800","capture_regions:",
            "push cs","pop es","cld",f"mov si,{FRAME}","mov di,offset output+30",
            f"mov cx,{FRAME_SIZE}","rep movsb",f"mov si,{TABLE}",f"mov cx,{TABLE_SIZE}","rep movsb",
            f"mov si,{RECORD}",f"mov cx,{RECORD_BYTES}","rep movsb","sti",
            "mov bx,word ptr [handle]","mov dx,offset output",f"mov cx,{RECORD_SIZE}",
            "mov ah,0x40","int 0x21","jc failure",f"cmp ax,{RECORD_SIZE}","jne failure",
            "inc word ptr [index]",f"cmp word ptr [index],{COUNT}","jb again",
            "mov bx,word ptr [handle]","mov ah,0x3e","int 0x21","mov bx,word ptr [cases_handle]",
            "mov ah,0x3e","int 0x21","mov dx,word ptr [old_offset]","mov ax,word ptr [old_segment]",
            "push ds","mov ds,ax","mov ax,0x2500","int 0x21","pop ds",
            "mov ax,0x4c00","int 0x21","failure:","mov ax,0x4c01","int 0x21",
            'filename: .asciz "RESULT.BIN"','cases_name: .asciz "CASES.BIN"',"header: .space 6",
            "handle: .word 0","cases_handle: .word 0","old_offset: .word 0","old_segment: .word 0",
            "index: .word 0","fixture: .space 14",f"output: .space {RECORD_SIZE}",
            "flags_table: .word "+",".join(str(x) for x in FLAGS)]
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
            stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    check_native(directory,result_path.read_bytes(),com.read_bytes())


def check_native(directory,result,code):
    if len(result) != 6+COUNT*RECORD_SIZE:
        raise ValueError(f"native output incomplete: {len(result)}/{6+COUNT*RECORD_SIZE}")
    segment,return_ip,count = struct.unpack_from("<3H",result)
    if count != COUNT or return_ip != 0x2534:
        raise ValueError("native caller count/return differs")
    lib = library(directory)
    totals = dict(returns=0,divide_errors=0,divisions=0,clamps=0)
    for case in range(COUNT):
        ro,t,k,v,gates = case_data(case)
        memory = bytearray([0xA5])*SIZE
        at = physical(segment,0x100)
        memory[at:at+len(code)] = code
        memory[physical(segment,TABLE):physical(segment,TABLE)+TABLE_SIZE] = bytes([3])*TABLE_SIZE
        memory[physical(segment,RECORD):physical(segment,RECORD)+RECORD_BYTES] = bytes([0xA5])*RECORD_BYTES
        memory[physical(segment,(ro+0x14) & 0xFFFF)] = t
        memory[physical(segment,(ro+0x2F) & 0xFFFF)] = k
        for i,value in enumerate(gates):
            memory[physical(segment,(ro+0x96+i) & 0xFFFF)] = value
        memory[physical(segment,TABLE+t)] = v
        put16(memory,segment,0x5D92,ro)
        put16(memory,segment,0x5D94,segment)
        for off,value in ((0xEFF8,return_ip),(0xEFFA,segment),(0xEFFC,ro),(0xEFFE,segment)):
            put16(memory,segment,off,value)
        cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                  0x7777,0xEFF8,segment,0x3FB3,segment,(segment+1) & 0xFFFF,segment,FLAGS[case % len(FLAGS)])
        ram = (ctypes.c_uint8 * SIZE).from_buffer(memory)
        status = lib.pr_u6c_record_tier_3fb3(ram,SIZE,ctypes.byref(cpu))
        observed = result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        actual_status = struct.unpack_from("<H",observed,28)[0]
        if status != actual_status or status not in (0,2):
            raise ValueError(f"native {case} status: {status}/{actual_status}")
        totals["returns" if status == 0 else "divide_errors"] += 1
        # Successful divisions have 0 <= remainder DX < positive divisor CX;
        # clamps retain DX = abs(adjustment)+CX, hence DX >= CX.
        divided = status == 0 and cpu.dx < cpu.cx
        if status == 0:
            totals["divisions" if divided else "clamps"] += 1
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",observed)):
            if (getattr(cpu,name) ^ expected) & 0xFFFF:  # FLAGS compared unmasked, including after IDIV
                raise ValueError(f"native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}")
        at = physical(segment,FRAME)
        if memory[at:at+FRAME_SIZE] != observed[30:30+FRAME_SIZE]:
            raise ValueError(f"native {case} frame differs")
        at = physical(segment,TABLE)
        if memory[at:at+TABLE_SIZE] != observed[30+FRAME_SIZE:30+FRAME_SIZE+TABLE_SIZE]:
            raise ValueError(f"native {case} table differs")
        at = physical(segment,RECORD)
        if memory[at:at+RECORD_BYTES] != observed[30+FRAME_SIZE+TABLE_SIZE:]:
            raise ValueError(f"native {case} record differs")
    if any(n == 0 for n in totals.values()):
        raise ValueError("native exit coverage incomplete")
    report = dict(target="OVL_006C_3FB3",source_sha256=BODY_SHA,native_cases=count,**totals,
                  original_caller_suffix="U6C:+2528..2533",caller_return_ip="2534",
                  actual_push_cs_near_call=True,reached_game_route=False,interrupts_isolated=True,
                  compared_cpu_words=14,compared_frame_bytes=FRAME_SIZE,compared_table_bytes=TABLE_SIZE,
                  compared_record_bytes=RECORD_BYTES,flags_masked=0,
                  fault_frame_normalized_words=3,result_sha256=sha256(result).hexdigest())
    (directory / "report.json").write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))


if __name__ == "__main__":
    main()
