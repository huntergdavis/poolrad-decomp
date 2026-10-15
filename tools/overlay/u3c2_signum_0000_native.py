#!/usr/bin/env python3
"""Native original U3C2 signum prefix and complete +0000 leaf; synthetic data."""
import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess
from u3c2_signum_0000_audit import Cpu,ROOT,SIZE,BODY_SHA,library,physical,put16,source
COUNT,FRAME,FRAME_SIZE=776,0xEFC0,64
RECORD_SIZE=28+FRAME_SIZE
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)


def case_data(case):
    if case<256:return case*257
    if case<512:return case-256
    if case<768:return 0x8000+case-512
    return (0x7FFF,0x8000,0xFFFF,0xFFFE,0x7FFE,0x0100,0xFF00,0)[case-768]


def main():
    directory=ROOT/'resources/u3c2-0000-parity/native';directory.mkdir(parents=True,exist_ok=True)
    body,_,meta=source();code=bytearray(0x300);code[0:0x2E]=body;code[0x1F7:0x1FC]=meta['caller_prefix']
    code[0x1FC:0x201]=bytes([0xEA,0,0,0,0]) # Transparent capture jump after original return.
    (directory/'CODE.BIN').write_bytes(code)
    (directory/'CASES.BIN').write_bytes(b''.join(struct.pack('<H',case_data(i)) for i in range(COUNT)))
    asm=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main','.org 0x1000','main:',
        'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000','mov word ptr [header],ax',
        'mov bx,0x1000','mov ah,0x4a','int 0x21','jc failure','mov bx,0x30','mov ah,0x48','int 0x21','jc failure',
        'mov word ptr [header+2],ax','mov word ptr [far_dispatch+2],ax',f'mov word ptr [header+4],{COUNT}',
        'mov dx,offset code_name','mov ax,0x3d00','int 0x21','jc failure','mov bx,ax','push ds',
        'mov ax,word ptr [header+2]','mov ds,ax','xor dx,dx','mov cx,0x300','mov ah,0x3f','int 0x21','pop ds',
        'jc failure','cmp ax,0x300','jne failure','mov ah,0x3e','int 0x21','mov ax,word ptr [header+2]',
        'mov es,ax','mov word ptr es:[0x1fd],offset capture','mov ax,cs','mov word ptr es:[0x1ff],ax',
        'mov dx,offset cases_name','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [cases_handle],ax',
        'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [handle],ax',
        'mov bx,ax','mov dx,offset header','mov cx,6','mov ah,0x40','int 0x21','again:',
        'mov bx,word ptr [cases_handle]','mov dx,offset fixture','mov cx,2','mov ah,0x3f','int 0x21','jc failure',
        'cmp ax,2','jne failure','cli','mov sp,0xf000','push cs','pop es','cld',f'mov di,{FRAME}',
        f'mov cx,{FRAME_SIZE}','mov al,0xa5','rep stosb',
        'mov ax,word ptr [index]','xor dx,dx','mov bx,6','div bx',
        'mov si,dx','shl si,1','mov ax,cs','inc ax','mov es,ax','push word ptr [flags_table+si]','popf',
        'mov ax,word ptr [fixture]','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde',
        'mov bp,0x7777','.byte 0xff,0x2e','.word far_dispatch','capture:','mov word ptr [output],ax',
        'mov ax,word ptr [header+2]','mov word ptr [output+16],ax','mov word ptr [output+18],0x1fc']
    for i,reg in enumerate('ax bx cx dx si di bp sp'.split()):
        if i:asm.append(f'mov word ptr [output+{2*i}],{reg}')
    asm+=['mov word ptr [output+20],ds','mov word ptr [output+22],es','mov word ptr [output+24],ss',
        'mov sp,0xf800','pushf','pop ax','mov word ptr [output+26],ax','push cs','pop es','cld',
        f'mov si,{FRAME}','mov di,offset output+28',f'mov cx,{FRAME_SIZE}','rep movsb','sti','mov bx,word ptr [handle]','mov dx,offset output',
        f'mov cx,{RECORD_SIZE}','mov ah,0x40','int 0x21','jc failure',f'cmp ax,{RECORD_SIZE}','jne failure',
        'inc word ptr [index]',f'cmp word ptr [index],{COUNT}','jb again','mov bx,word ptr [handle]',
        'mov ah,0x3e','int 0x21','mov bx,word ptr [cases_handle]','mov ah,0x3e','int 0x21',
        'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"',
        'code_name: .asciz "CODE.BIN"','cases_name: .asciz "CASES.BIN"','header: .space 6','handle: .word 0',
        'cases_handle: .word 0','index: .word 0','fixture: .space 2','far_dispatch: .word 0x1f7,0',
        f'output: .space {RECORD_SIZE}','flags_table: .word '+','.join(str(x) for x in FLAGS)]
    assembly,obj,com=directory/'oracle.S',directory/'oracle.o',directory/'ORACLE.COM'
    assembly.write_text('\n'.join(asm)+'\n');subprocess.run(['as','--32',str(assembly),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    if len(com.read_bytes())+0x100>=FRAME:raise ValueError('synthetic program overlaps capture frame')
    config=directory/'dosbox.conf';config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory)+'\nc:\nORACLE.COM\nexit\n')
    result_path=directory/'RESULT.BIN';result_path.unlink(missing_ok=True)
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x','-conf',str(config),'-fastlaunch','-exit','-noconsole'],
            env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    check_native(directory,result_path.read_bytes(),com.read_bytes(),code)


def check_native(directory,result,program,code):
    if len(result)!=6+COUNT*RECORD_SIZE:raise ValueError(f'native output incomplete: {len(result)}/{6+COUNT*RECORD_SIZE}')
    segment,code_cs,count=struct.unpack_from('<3H',result)
    if count!=COUNT or code_cs==segment:raise ValueError('native loaded-code separation differs')
    lib=library(directory)
    for case in range(COUNT):
        value=case_data(case);memory=bytearray([0xA5])*SIZE
        at=physical(segment,0x100);memory[at:at+len(program)]=program;at=physical(code_cs,0);memory[at:at+len(code)]=code
        for off,word in ((0xEFFA,0x1FC),(0xEFFC,code_cs),(0xEFFE,value)):put16(memory,segment,off,word)
        cpu=Cpu(value,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0xEFFA,code_cs,0,segment,(segment+1)&0xFFFF,segment,FLAGS[case%len(FLAGS)])
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        if lib.pr_u3c2_signum_0000(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected native case')
        observed=result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from('<14H',observed)):
            mask=0xFFEF if name=='flags' and value==0 else 0xFFFF
            if (getattr(cpu,name)^expected)&mask:raise ValueError(f'native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}')
        for offset,length,cursor in ((FRAME,FRAME_SIZE,28),):
            at=physical(segment,offset)
            if memory[at:at+length]!=observed[cursor:cursor+length]:raise ValueError(f'native {case} RAM region {offset:04X}')
    report=dict(target='OVL_03C2_0000',source_sha256=BODY_SHA,native_cases=count,original_caller_prefix='U3C2:+01F7..01FB',
        original_return_ip='01FC',word_result=True,compared_cpu_words=14,compared_frame_bytes=FRAME_SIZE,
        nonzero_masked_flags=0,zero_path_undefined_AF_mask=0x10,zero_path_cases=sum(case_data(i)==0 for i in range(COUNT)),separate_loaded_code_segment=True,natural_game_route_measured=False,
        loader_dispatch_measured=False,result_sha256=sha256(result).hexdigest())
    (directory/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
