#!/usr/bin/env python3
"""Native original U3C6 range prefix and complete +06E9 leaf; synthetic data."""
# NOT_A_ROW_CHECKER: native companion of u3c6_in_range_06e9_audit.py.
from native_emulator_provenance import describe_dosbox
import argparse
import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess
from u3c6_in_range_06e9_audit import Cpu,ROOT,SIZE,BODY_SHA,library,physical,put16,source
WRAP=False
COUNT,FRAME,FRAME_SIZE=1024,0xEFC0,64
RECORD_SIZE=28+FRAME_SIZE
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)


def case_data(case):
    if WRAP:
        a,b=((0,0),(6,6),(7,6),(128,6),(6,7),(6,128),(255,255),(5,0))[case%8]
        return a,b,(((case*37)&255)<<8)|0xCD
    if case<256:a,b=case,6
    elif case<512:a,b=case-256,7
    elif case<768:a,b=6,case-512
    else:a,b=7,case-768
    return a,b,(((case*37)&255)<<8)|0xCD


def main():
    dosbox_x=describe_dosbox('dosbox-x')
    directory=ROOT/('resources/u3c6-06e9-parity/native-wrap' if WRAP else 'resources/u3c6-06e9-parity/native');directory.mkdir(parents=True,exist_ok=True)
    body,_,meta=source();code=bytearray(0x800);code[0x6E9:0x71A]=body;code[0x68D:0x699]=meta['caller_prefix']
    code[0x699:0x69E]=bytes([0xEA,0,0,0,0]) # Transparent capture jump after original return.
    (directory/'CODE.BIN').write_bytes(code)
    (directory/'CASES.BIN').write_bytes(b''.join(struct.pack('<4H',*case_data(i),((0xFFFE if i<8 else 0xFFFF)+8)&0xFFFF) if WRAP else struct.pack('<3H',*case_data(i)) for i in range(COUNT)))
    asm=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main','.org 0x1000','main:',
        'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000','mov word ptr [header],ax',
        'mov bx,0x1000','mov ah,0x4a','int 0x21','jc failure','mov bx,0x80','mov ah,0x48','int 0x21','jc failure',
        'mov word ptr [header+2],ax','mov word ptr [far_dispatch+2],ax',f'mov word ptr [header+4],{COUNT}',
        'mov dx,offset code_name','mov ax,0x3d00','int 0x21','jc failure','mov bx,ax','push ds',
        'mov ax,word ptr [header+2]','mov ds,ax','xor dx,dx','mov cx,0x800','mov ah,0x3f','int 0x21','pop ds',
        'jc failure','cmp ax,0x800','jne failure','mov ah,0x3e','int 0x21','mov ax,word ptr [header+2]',
        'mov es,ax','mov word ptr es:[0x69a],offset capture','mov ax,cs','mov word ptr es:[0x69c],ax',
        'mov dx,offset cases_name','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [cases_handle],ax',
        'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [handle],ax',
        'mov bx,ax','mov dx,offset header','mov cx,6','mov ah,0x40','int 0x21','again:',
        'mov bx,word ptr [cases_handle]','mov dx,offset fixture','mov cx,6','mov ah,0x3f','int 0x21','jc failure',
        'cmp ax,6','jne failure','cli','mov sp,0xf000','push cs','pop es','cld',f'mov di,{FRAME}',
        f'mov cx,{FRAME_SIZE}','mov al,0xa5','rep stosb',
        'mov ax,word ptr [fixture+2]','mov byte ptr [0x7781],al',
        'mov ax,word ptr [fixture]','mov byte ptr [0x777f],al',
        'mov ax,word ptr [index]','xor dx,dx','mov bx,6','div bx',
        'mov si,dx','shl si,1','mov ax,cs','inc ax','mov es,ax','push word ptr [flags_table+si]','popf',
        'mov ax,word ptr [fixture+4]','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde',
        'mov bp,0x7777','.byte 0xff,0x2e','.word far_dispatch','capture:','mov word ptr [output],ax',
        'mov ax,word ptr [header+2]','mov word ptr [output+16],ax','mov word ptr [output+18],0x699']
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
        'cases_handle: .word 0','index: .word 0','fixture: .space 6','far_dispatch: .word 0x68d,0',
        f'output: .space {RECORD_SIZE}','flags_table: .word '+','.join(str(x) for x in FLAGS)]
    if WRAP:
        asm=[x.replace('mov bx,0x1000','mov bx,0x2000').replace('fixture: .space 6','fixture: .space 8') for x in asm]
        at=asm.index('mov dx,offset fixture');asm[at+1]='mov cx,8'
        at=asm.index('cmp ax,6');asm[at]='cmp ax,8'
        at=asm.index('mov ax,word ptr [fixture+2]')
        asm[at:at]=['xor di,di','mov cx,8','rep stosb','mov ax,cs','add ax,0x1000','mov es,ax',
                    'xor di,di','mov cx,16','mov al,0xa5','rep stosb']
        at=asm.index('mov bp,0x7777')+1;asm[at:at]=['mov sp,word ptr [fixture+6]']
        at=asm.index('sti')
        asm[at:at]=['xor si,si','mov cx,8','rep movsb','mov ax,cs','add ax,0x1000','mov ds,ax',
                    'xor si,si','mov cx,16','rep movsb','push cs','pop ds']
    assembly,obj,com=directory/'oracle.S',directory/'oracle.o',directory/'ORACLE.COM'
    assembly.write_text('\n'.join(asm)+'\n');subprocess.run(['as','--32',str(assembly),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    if len(com.read_bytes())+0x100>=0x777F:raise ValueError('synthetic program overlaps caller locals')
    config=directory/'dosbox.conf';config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory)+'\nc:\nORACLE.COM\nexit\n')
    result_path=directory/'RESULT.BIN';result_path.unlink(missing_ok=True)
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run([dosbox_x['path'],'-conf',str(config),'-fastlaunch','-exit','-noconsole'],
            env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    check_native(directory,result_path.read_bytes(),com.read_bytes(),code,dosbox_x)


def check_native(directory,result,program,code,dosbox_x):
    if len(result)!=6+COUNT*RECORD_SIZE:raise ValueError(f'native output incomplete: {len(result)}/{6+COUNT*RECORD_SIZE}')
    segment,code_cs,count=struct.unpack_from('<3H',result)
    if count!=COUNT or code_cs==segment:raise ValueError('native loaded-code separation differs')
    lib=library(directory)
    failures=[]
    for case in range(COUNT):
        a,b,ax=case_data(case);memory=bytearray([0xA5])*SIZE
        at=physical(segment,0x100);memory[at:at+len(program)]=program;at=physical(code_cs,0);memory[at:at+len(code)]=code
        entry_sp=(0xFFFE if case<8 else 0xFFFF) if WRAP else 0xEFF8
        for delta,value in ((0,0x699),(2,code_cs),(4,(ax&0xFF00)|a),(6,(ax&0xFF00)|b)):put16(memory,segment,(entry_sp+delta)&0xFFFF,value)
        cpu=Cpu((ax&0xFF00)|a,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,entry_sp,code_cs,0x6E9,segment,(segment+1)&0xFFFF,segment,FLAGS[case%len(FLAGS)])
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        if lib.pr_u3c6_in_range_06e9(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected native case')
        observed=result[6+case*RECORD_SIZE:6+(case+1)*RECORD_SIZE]
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from('<14H',observed)):
            if getattr(cpu,name)!=expected:failures.append(dict(case=case,sp=f'{entry_sp:04X}',register=name,c=f'{getattr(cpu,name):04X}',native=f'{expected:04X}'))
        regions=[(FRAME,FRAME_SIZE,28)]
        if WRAP:regions.extend(((0,8,28+FRAME_SIZE),(0x10000,16,36+FRAME_SIZE)))
        for offset,length,cursor in regions:
            at=physical(segment,offset)
            if memory[at:at+length]!=observed[cursor:cursor+length]:failures.append(dict(case=case,region=f'{offset:05X}',different_offsets=[i for i in range(length) if memory[at+i]!=observed[cursor+i]]))
    report=dict(dosbox_x=dosbox_x,target='OVL_03C6_06E9',source_sha256=BODY_SHA,native_cases=count,original_caller_prefix='U3C6:+068D..0698',
        original_return_ip='0699',preserved_AH=True,compared_cpu_words=14,compared_frame_bytes=FRAME_SIZE+(24 if WRAP else 0),entry_sp_vectors=['FFFE','FFFF'] if WRAP else ['EFF8'],comparison_failures=failures,
        masked_flags=0,separate_loaded_code_segment=True,natural_game_route_measured=False,
        loader_dispatch_measured=False,result_sha256=sha256(result).hexdigest())
    (directory/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report,sort_keys=True))
    if failures:raise ValueError('native comparison failures; see report.json')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--wrap-returns',action='store_true');args=parser.parse_args()
    if args.wrap_returns:WRAP=True;COUNT=16;FRAME=0xFFB0;FRAME_SIZE=80;RECORD_SIZE=28+FRAME_SIZE+24
    main()
