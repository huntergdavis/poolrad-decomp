#!/usr/bin/env python3
"""Native full U38/U9E original chain with synthetic frames and list data."""
import ctypes
from hashlib import sha256
import json
import os
import struct
import subprocess
from u38_pair_04ba_audit import Cpu,ROOT,SIZE,physical,put16,fixture,source,library,install
OUT=ROOT/'resources/u38-04ba/native'
STACK_REGIONS=((0xEFC0,96),(0xFFC0,64),(0,64))
STACK_BYTES=sum(n for _,n in STACK_REGIONS)
DATA_BYTES=0x2000
RECORD_SIZE=8+28+STACK_BYTES+DATA_BYTES


def assembly(codepath,options):
    sp=options.get('sp',0xEFF8);attribute=options.get('attribute',5)
    nodes=options.get('nodes',((12,6),(38,106)));self_index=options.get('self_index')
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main','.org 0x200','code:',f'.incbin "{codepath.resolve()}"','.org 0x3400','main:',
       'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000','mov bx,0x4100','mov ah,0x4a','int 0x21','jc failure',
       'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld',
       'mov ax,cs','add ax,0x1000','mov word ptr cs:[dispatch+2],ax','mov word ptr cs:[output],ax','mov es,ax',
       'mov si,offset code','xor di,di','mov cx,0x3100','rep movsb',
       'mov ax,es','add ax,0x300','mov word ptr es:[0x4e0],ax','mov word ptr es:[0x555],ax','mov word ptr es:[0x5b3],ax',
       'mov ax,es','add ax,0x100','mov word ptr cs:[output+2],ax','mov word ptr es:[0x3073],ax','mov word ptr es:[0x3078],ax',
       'mov ax,cs','add ax,0x2000','mov ss,ax','mov es,ax','mov word ptr cs:[output+4],ax',
       'xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
       'mov ax,ss','add ax,0x1000','mov es,ax','mov word ptr cs:[output+6],ax',
       'xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb','mov ax,es','mov ds,ax',
       'mov byte ptr [0x100],12',f'mov byte ptr [0x103],{attribute}','mov word ptr [0x105],0','mov word ptr [0x107],0']
    pointers=[0x100 if i==self_index else 0x300+0x100*i for i in range(len(nodes))]
    for i,(kind,attr) in enumerate(nodes):
        at=pointers[i];n=pointers[i+1] if i+1<len(pointers) else 0
        a += [f'mov byte ptr [{at}],{kind}',f'mov byte ptr [{at+3}],{attr}',f'mov word ptr [{at+5}],{n}',f'mov word ptr [{at+7}],ax' if n else f'mov word ptr [{at+7}],0']
    a += [f'mov word ptr [0x187f],{pointers[0] if pointers else 0}','mov word ptr [0x1881],ax' if pointers else 'mov word ptr [0x1881],0']
    for off,val in ((0,'offset returned'),(4,'0xcafe'),(6,'0x100'),(10,'0x1800')):a += [f'mov word ptr ss:[{(sp+off)&65535}],{val}']
    a += [f'mov word ptr ss:[{(sp+8)&65535}],ax',f'mov word ptr ss:[{(sp+12)&65535}],ax','mov ax,cs',f'mov word ptr ss:[{(sp+2)&65535}],ax',
          'mov sp,0x8000','push 2','popf','mov ax,0x1111','mov es,ax',
          'mov ax,0xabcd','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',f'mov sp,{sp}',
          'jmp far ptr cs:[dispatch]','returned:']
    for i,reg in enumerate('ax bx cx dx si di bp sp cs'.split()):a += [f'mov word ptr cs:[output+{8+2*i}],{reg}']
    a += ['mov word ptr cs:[output+26],offset returned','mov word ptr cs:[output+28],ds','mov word ptr cs:[output+30],es','mov word ptr cs:[output+32],ss',
          'mov sp,0x8000','pushf','pop word ptr cs:[output+34]','mov ax,cs','mov es,ax','cld','mov ax,ss','mov ds,ax','mov di,offset output+36']
    for off,n in STACK_REGIONS:a += [f'mov si,{off}',f'mov cx,{n}','rep movsb']
    a += ['mov ax,ss','add ax,0x1000','mov ds,ax','xor si,si',f'mov cx,{DATA_BYTES}','rep movsb',
          'mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti','mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{RECORD_SIZE}',
          'mov ah,0x40','int 0x21','jc failure',f'cmp ax,{RECORD_SIZE}','jne failure','mov bx,word ptr [handle]','mov ah,0x3e','int 0x21',
          'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"','handle: .word 0','dispatch: .word 0x4ba,0',f'output: .space {RECORD_SIZE}']
    return '\n'.join(a)+'\n'


def main():
    bodies,_=source();OUT.mkdir(parents=True,exist_ok=True);code=bytearray(0x3100)
    for (unit,start),b in bodies.items():
        base=0 if unit==0x38 else 0x1000;code[base+start:base+start+len(b)]=b
    for off,entry in ((0x3070,0x10E4),(0x3075,0x110D)):code[off:off+5]=b'\xEA'+entry.to_bytes(2,'little')+bytes(2)
    cp=OUT/'code.bin';cp.write_bytes(code);lib=library(OUT)
    cases=[dict(attribute=128),dict(nodes=()),dict(nodes=((0,6),(255,127))),dict(nodes=((12,6),)),dict(attribute=106,nodes=((38,127),)),
           dict(nodes=((12,1),(38,1))),dict(nodes=((12,1),(38,2))),dict(nodes=((12,101),(38,0))),dict(nodes=((12,127),(38,106))),
           dict(nodes=((12,106),(38,127))),dict(nodes=((12,5),(38,6)),self_index=0),dict(nodes=((12,5),),self_index=0)]
    cases += [dict(sp=sp,**kw) for sp in (0xFFFE,0xFFFF) for kw in (dict(attribute=255),dict(nodes=()),dict(nodes=((12,6),)),dict(attribute=106,nodes=((38,127),)),dict(nodes=((12,101),(38,0))))]
    cases += [dict(sp=sp) for sp in (0x1C,0x1D)]
    hashes=[]
    for index,options in enumerate(cases):
        d=OUT/f'case-{index:02d}';d.mkdir(exist_ok=True);asm=d/'oracle.S';obj=d/'oracle.o';com=d/'ORACLE.COM'
        asm.write_text(assembly(cp,options));subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
        subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
        assert len(com.read_bytes())+0x100<0xE000,'native program exceeds private stack boundary'
        cfg=d/'dosbox.conf';cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(d)+'\nc:\nORACLE.COM\nexit\n')
        output=d/'RESULT.BIN';output.unlink(missing_ok=True)
        with (d/'dosbox.log').open('w') as log:
            subprocess.run(['dosbox-x','-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
        result=output.read_bytes();assert len(result)==RECORD_SIZE,(index,len(result))
        code_cs,callee,ss,ds=struct.unpack_from('<4H',result);regs=struct.unpack_from('<14H',result,8)
        memory,cpu,_,stub=fixture(ss=ss,cs=code_cs,callee_cs=callee,stub_cs=code_cs+0x300,flags=2,src=(ds,0x100),record=(ds,0x1800),node_seg=ds,**dict(dict(sp=0xEFF8),**options))
        cpu.ds=ds
        put16(memory,ss,cpu.sp,regs[9]);put16(memory,ss,(cpu.sp+2)&65535,regs[8]);install(memory,cpu,callee,stub,bodies)
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        assert lib.pr_u38_pair_04ba(ram,SIZE,ctypes.byref(cpu),callee,32)==0
        for (name,_),value in zip(Cpu._fields_,regs):assert getattr(cpu,name)==value,(index,name,hex(getattr(cpu,name)),hex(value))
        cursor=36
        for off,n in STACK_REGIONS:
            at=physical(ss,off)
            assert memory[at:at+n]==result[cursor:cursor+n],(index,'stack',hex(off),[hex(off+i) for i in range(n) if memory[at+i]!=result[cursor+i]])
            cursor+=n
        at=physical(ds,0)
        assert memory[at:at+DATA_BYTES]==result[cursor:],(index,'data',[hex(i) for i in range(DATA_BYTES) if memory[at+i]!=result[cursor+i]][:20])
        hashes.append(sha256(result).hexdigest());print(f'PASS native case {index}: {options}',flush=True)
    report=dict(target='OVL_0038_04BA',native_cases=len(cases),cpu_words=14,stack_bytes=STACK_BYTES,data_bytes=DATA_BYTES,masked_flags=0,full_original_chain=True,entry_return_wrap_cases=10,internal_return_wrap_cases=2,les_segment_seams_measured=False,natural_game_route_measured=False,result_sha256=hashes)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
