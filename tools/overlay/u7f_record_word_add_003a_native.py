#!/usr/bin/env python3
"""Native original U7F caller prefix and complete +003A leaf; synthetic data."""
import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess
from u7f_record_word_add_003a_audit import Cpu,ROOT,SIZE,BODY_SHA,library,physical,put16,source
COUNT,FRAME,FRAME_SIZE,DATA,DATA_SIZE=544,0xEFC0,64,0x9000,288
RECORD_SIZE=28+FRAME_SIZE+DATA_SIZE
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
BOUNDARIES=(0,1,15,16,127,128,255,256,0x7FFE,0x7FFF,0x8000,0x8001,0xFFFE,0xFFFF)


def case_data(case):
    ro=DATA
    if case<256:value,increment=case*257,1
    elif case<512:value,increment=0xFFFF,(case-256)*257
    elif case<540:value,increment=BOUNDARIES[(case-512)%14],1 if case<526 else 0x8000
    else:ro,value,increment=(0xEEF2,0xEEF8,0xEEFA,0xEEFC)[case-540],0xA5A5,0x1357
    return ro,value,increment


def main():
    directory=ROOT/'resources/u7f-003a-parity/native';directory.mkdir(parents=True,exist_ok=True)
    body,_,meta=source();code=bytearray(0x300);code[0x3A:0x54]=body;code[0x241:0x250]=meta['caller_prefix']
    code[0x250:0x255]=bytes([0xEA,0,0,0,0]) # Transparent capture jump after original return.
    (directory/'CODE.BIN').write_bytes(code)
    (directory/'CASES.BIN').write_bytes(b''.join(struct.pack('<3H',*case_data(i)) for i in range(COUNT)))
    asm=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main','.org 0x1000','main:',
        'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000','mov word ptr [header],ax',
        'mov bx,0x1000','mov ah,0x4a','int 0x21','jc failure','mov bx,0x30','mov ah,0x48','int 0x21','jc failure',
        'mov word ptr [header+2],ax','mov word ptr [far_dispatch+2],ax',f'mov word ptr [header+4],{COUNT}',
        'mov dx,offset code_name','mov ax,0x3d00','int 0x21','jc failure','mov bx,ax','push ds',
        'mov ax,word ptr [header+2]','mov ds,ax','xor dx,dx','mov cx,0x300','mov ah,0x3f','int 0x21','pop ds',
        'jc failure','cmp ax,0x300','jne failure','mov ah,0x3e','int 0x21','mov ax,word ptr [header+2]',
        'mov es,ax','mov word ptr es:[0x251],offset capture','mov ax,cs','mov word ptr es:[0x253],ax',
        'mov dx,offset cases_name','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [cases_handle],ax',
        'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [handle],ax',
        'mov bx,ax','mov dx,offset header','mov cx,6','mov ah,0x40','int 0x21','again:',
        'mov bx,word ptr [cases_handle]','mov dx,offset fixture','mov cx,6','mov ah,0x3f','int 0x21','jc failure',
        'cmp ax,6','jne failure','cli','mov sp,0xf000','push cs','pop es','cld',f'mov di,{FRAME}',
        f'mov cx,{FRAME_SIZE}','mov al,0xa5','rep stosb',f'mov di,{DATA}',f'mov cx,{DATA_SIZE}','rep stosb',
        'mov bx,word ptr [fixture]','mov word ptr [0x5d92],bx','mov ax,cs','mov word ptr [0x5d94],ax',
        'mov ax,word ptr [fixture+2]','mov word ptr [bx+0x102],ax','mov ax,word ptr [fixture+4]',
        'mov word ptr [0x7775],ax','mov ax,word ptr [index]','xor dx,dx','mov bx,6','div bx',
        'mov si,dx','shl si,1','mov ax,cs','inc ax','mov es,ax','push word ptr [flags_table+si]','popf',
        'mov ax,0xabcd','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde',
        'mov bp,0x7777','.byte 0xff,0x2e','.word far_dispatch','capture:','mov word ptr [output],ax',
        'mov ax,word ptr [header+2]','mov word ptr [output+16],ax','mov word ptr [output+18],0x250']
    for i,reg in enumerate('ax bx cx dx si di bp sp'.split()):
        if i:asm.append(f'mov word ptr [output+{2*i}],{reg}')
    asm+=['mov word ptr [output+20],ds','mov word ptr [output+22],es','mov word ptr [output+24],ss',
        'mov sp,0xf800','pushf','pop ax','mov word ptr [output+26],ax','push cs','pop es','cld',
        f'mov si,{FRAME}','mov di,offset output+28',f'mov cx,{FRAME_SIZE}','rep movsb',f'mov si,{DATA}',
        f'mov cx,{DATA_SIZE}','rep movsb','sti','mov bx,word ptr [handle]','mov dx,offset output',
        f'mov cx,{RECORD_SIZE}','mov ah,0x40','int 0x21','jc failure',f'cmp ax,{RECORD_SIZE}','jne failure',
        'inc word ptr [index]',f'cmp word ptr [index],{COUNT}','jb again','mov bx,word ptr [handle]',
        'mov ah,0x3e','int 0x21','mov bx,word ptr [cases_handle]','mov ah,0x3e','int 0x21',
        'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"',
        'code_name: .asciz "CODE.BIN"','cases_name: .asciz "CASES.BIN"','header: .space 6','handle: .word 0',
        'cases_handle: .word 0','index: .word 0','fixture: .space 6','far_dispatch: .word 0x241,0',
        f'output: .space {RECORD_SIZE}','flags_table: .word '+','.join(str(x) for x in FLAGS)]
    assembly,obj,com=directory/'oracle.S',directory/'oracle.o',directory/'ORACLE.COM'
    assembly.write_text('\n'.join(asm)+'\n');subprocess.run(['as','--32',str(assembly),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    if len(com.read_bytes())+0x100>=0x5D92:raise ValueError('synthetic program overlaps active-record globals')
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
        ro,value,increment=case_data(case);memory=bytearray([0xA5])*SIZE
        at=physical(segment,0x100);memory[at:at+len(program)]=program;at=physical(code_cs,0);memory[at:at+len(code)]=code
        put16(memory,segment,(ro+0x102)&0xFFFF,value)
        for off,value in ((0xEFF6,0x250),(0xEFF8,code_cs),(0xEFFA,increment),(0xEFFC,ro),(0xEFFE,segment)):put16(memory,segment,off,value)
        cpu=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0xEFF6,code_cs,0x3A,segment,(segment+1)&0xFFFF,segment,FLAGS[case%len(FLAGS)])
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        if lib.pr_u7f_record_word_add_003a(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected native case')
        observed=result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from('<14H',observed)):
            if getattr(cpu,name)!=expected:raise ValueError(f'native {case} {name}: {getattr(cpu,name):04X}/{expected:04X}')
        for offset,length,cursor in ((FRAME,FRAME_SIZE,28),(DATA,DATA_SIZE,28+FRAME_SIZE)):
            at=physical(segment,offset)
            if memory[at:at+length]!=observed[cursor:cursor+length]:raise ValueError(f'native {case} RAM region {offset:04X}')
    report=dict(target='OVL_007F_003A',source_sha256=BODY_SHA,native_cases=count,original_caller_prefix='U7F:+0241..024F',
        original_return_ip='0250',safe_frame_alias_cases=4,compared_cpu_words=14,compared_frame_bytes=FRAME_SIZE,
        compared_record_bytes=DATA_SIZE,masked_flags=0,separate_loaded_code_segment=True,natural_game_route_measured=False,
        loader_dispatch_measured=False,result_sha256=sha256(result).hexdigest())
    (directory/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
