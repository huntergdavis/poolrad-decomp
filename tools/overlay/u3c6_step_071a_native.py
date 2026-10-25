#!/usr/bin/env python3
"""DOSBox-X full original +071A chain, CPU14 and all touched stack bytes."""
# NOT_A_ROW_CHECKER: native companion of u3c6_step_071a_audit.py.
from native_emulator_provenance import describe_dosbox
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u3c6_step_071a_audit import Cpu, ROOT, SIZE, MASK, physical, put16, source, library, fixture
OUT=ROOT/'resources/u3c6-071a/native'
STACK_OFFSETS=tuple(range(0xEFC0,0xF020))+tuple(range(0xFFC0,0x10000))+tuple(range(0x40))
RECORD_SIZE=6+28+len(STACK_OFFSETS)+16


def assembly(codepath,options):
    sp=options.get('sp',0xEFF8);mode=options.get('mode',0);slot=options.get('slot',1);count=options.get('count',3)
    direction=options.get('direction',1);x=options.get('x',3);y=options.get('y',3)
    pairs=options.get('pairs',((0,0),(1,0),(0,1),(1,1)))
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main','.org 0x200','code:',f'.incbin "{codepath.resolve()}"','.org 0x1200','main:',
       'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000','mov bx,0x4100','mov ah,0x4a','int 0x21','jc failure',
       'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld',
       'mov ax,cs','add ax,0x1000','mov word ptr cs:[dispatch+2],ax','mov word ptr cs:[output],ax','mov es,ax',
       'mov si,offset code','xor di,di','mov cx,0xd00','rep movsb',
       'mov ax,cs','add ax,0x2000','mov ss,ax','mov es,ax','mov word ptr cs:[output+2],ax',
       'xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
       'mov ax,ss','add ax,0x1000','mov es,ax','mov word ptr cs:[output+4],ax',
       'xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb','mov ax,es','mov ds,ax']
    # Initialize exactly the same live table bytes as the portable fixture.
    a += [f'mov byte ptr [0x5f2a],{count}']
    for i in range(256):
        a += [f'mov word ptr [{0x65B9+4*i}],0x9999',f'mov word ptr [{0x65BB+4*i}],0x8888']
    if slot is not None:a += [f'mov word ptr [{0x65B9+4*slot}],0x1234',f'mov word ptr [{0x65BB+4*slot}],0x5678']
    slot=slot or 0
    a += [f'mov byte ptr [{0x5F2A+4*slot}],{direction}',f'mov byte ptr [{0x604A+slot}],{x}',f'mov byte ptr [{0x6092+slot}],{y}']
    for i,(p,q) in enumerate(pairs):a += [f'mov byte ptr [{0x2844+8*direction+2*i}],{p}',f'mov byte ptr [{0x2845+8*direction+2*i}],{q}']
    for off,val in ((0,'offset returned'),(4,str(mode)),(6,'0x1234'),(8,'0x5678')):a += [f'mov word ptr ss:[{(sp+off)&65535}],{val}']
    a += ['mov ax,cs',f'mov word ptr ss:[{(sp+2)&65535}],ax',
          'mov sp,0x8000','push 2','popf','mov ax,0x1111','mov es,ax',
          'mov ax,0xabcd','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',f'mov sp,{sp}',
          'jmp far ptr cs:[dispatch]','returned:']
    for i,reg in enumerate('ax bx cx dx si di bp sp cs'.split()):a += [f'mov word ptr cs:[output+{6+2*i}],{reg}']
    a += ['mov word ptr cs:[output+24],offset returned','mov word ptr cs:[output+26],ds','mov word ptr cs:[output+28],es','mov word ptr cs:[output+30],ss',
          'mov sp,0x8000','pushf','pop word ptr cs:[output+32]']
    for i,off in enumerate(STACK_OFFSETS):a += [f'mov al,byte ptr ss:[{off}]',f'mov byte ptr cs:[output+{34+i}],al']
    a += ['mov ax,ss','add ax,0x1000','mov es,ax']
    for i in range(16):a += [f'mov al,byte ptr es:[{i}]',f'mov byte ptr cs:[output+{34+len(STACK_OFFSETS)+i}],al']
    a += ['mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti','mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{RECORD_SIZE}',
          'mov ah,0x40','int 0x21','jc failure',f'cmp ax,{RECORD_SIZE}','jne failure','mov bx,word ptr [handle]','mov ah,0x3e','int 0x21',
          'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"','handle: .word 0','dispatch: .word 0x71a,0',f'output: .space {RECORD_SIZE}']
    return '\n'.join(a)+'\n'


def main():
    dosbox_x=describe_dosbox('dosbox-x')
    bodies,_=source();OUT.mkdir(parents=True,exist_ok=True);code=bytearray(0xD00)
    for start,b in bodies.items():code[start:start+len(b)]=b
    cp=OUT/'code.bin';cp.write_bytes(code);lib=library(OUT)
    cases=[dict(direction=0),dict(mode=0,x=3,y=3),dict(mode=1,x=3,y=3),dict(mode=0,x=7,y=7),dict(mode=1,x=7,y=7),
           dict(mode=0,x=6,y=6,pairs=((127,0),(0,0),(128,0),(255,255))),dict(mode=1,x=6,y=6),dict(slot=None,count=0,direction=0),
           dict(slot=255,count=0,mode=1),dict(mode=0,x=128,y=255),dict(mode=1,pairs=((128,0),)*4),dict(mode=255,x=255,y=0)]
    cases += [dict(sp=sp,**kw) for sp in (0xFFFE,0xFFFF) for kw in (dict(direction=0),dict(mode=0),dict(mode=1),dict(mode=0,x=7),dict(mode=1,x=7))]
    cases += [dict(sp=sp,direction=0) for sp in (0x10,0x11)]
    cases += [dict(sp=sp,mode=1) for sp in (0x18,0x19)]
    hashes=[]
    for index,options in enumerate(cases):
        d=OUT/f'case-{index:02d}';d.mkdir(exist_ok=True);asm=d/'oracle.S';obj=d/'oracle.o';com=d/'ORACLE.COM'
        asm.write_text(assembly(cp,options));subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
        subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
        cfg=d/'dosbox.conf';cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(d)+'\nc:\nORACLE.COM\nexit\n')
        output=d/'RESULT.BIN';output.unlink(missing_ok=True)
        with (d/'dosbox.log').open('w') as log:
            subprocess.run([dosbox_x['path'],'-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
        result=output.read_bytes();assert len(result)==RECORD_SIZE,(index,len(result))
        code_cs,ss,ds=struct.unpack_from('<3H',result);regs=struct.unpack_from('<14H',result,6)
        memory,cpu=fixture(ss=ss,ds=ds,cs=code_cs,flags=2,**dict(dict(sp=0xEFF8),**options))
        put16(memory,ss,cpu.sp,regs[9]);put16(memory,ss,(cpu.sp+2)&65535,regs[8])
        for start,b in bodies.items():memory[physical(code_cs,start):physical(code_cs,start)+len(b)]=b
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        assert lib.pr_u3c6_step_071a(ram,SIZE,ctypes.byref(cpu),512)==0
        for (name,_),value in zip(Cpu._fields_,regs):
            assert getattr(cpu,name)==value,(index,name,hex(getattr(cpu,name)),hex(value))
        for i,off in enumerate(STACK_OFFSETS):assert memory[physical(ss,off)]==result[34+i],(index,'stack',hex(off),memory[physical(ss,off)],result[34+i])
        for i in range(16):assert memory[(physical(ss,0)+0x10000+i)&MASK]==result[34+len(STACK_OFFSETS)+i],(index,'extension',i)
        hashes.append(sha256(result).hexdigest())
        print(f'PASS native case {index}: {options}',flush=True)
    report=dict(dosbox_x=dosbox_x,target='OVL_03C6_071A',native_cases=len(cases),cpu_words=14,stack_bytes=len(STACK_OFFSETS)+16,masked_flags=0,full_original_chain=True,entry_return_wrap_cases=10,internal_return_wrap_cases=4,les_segment_seams_measured=False,natural_game_route_measured=False,result_sha256=hashes)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
