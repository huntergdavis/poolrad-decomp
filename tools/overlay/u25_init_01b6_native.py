#!/usr/bin/env python3
"""DOSBox-X full-chain initializer captures; synthetic fixtures, original code."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u25_init_01b6_audit import ROOT,SIZE,Cpu,NAMES,source,library,fixture,install,physical,put16,operand

OUT=ROOT/'resources/u25-01b6/native'
WINDOWS=((0xEDC0,0x250),(0,0x240),(0xFD80,0x280))
STACK_BYTES=sum(n for _,n in WINDOWS)
HEADER=40+STACK_BYTES
RECORD=HEADER+65536


def options(options,cs,system,ss,ds):
    kw={k:v for k,v in options.items() if k not in ('alias','stop','budgets')}
    kw.update(cs=cs,system=system,ss=ss,ds=ds)
    if options.get('alias')=='restored':kw['window0']=(ds,0x49F8-0x1CC)
    if options.get('alias')=='operand':kw['window0']=(ds,0x7046-0x200)
    return kw


def assembly(paths,case):
    m,c,_=fixture(**options(case,0x1000,0x2000,0x3000,0x4000));sp=c.sp;stop=case.get('stop')
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main',
       '.org 0x200','caller:',f'.incbin "{paths[0].resolve()}"',
       '.org 0x1800','system:',f'.incbin "{paths[1].resolve()}"',
       '.org 0x3000','main:','mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
       'mov bx,0x5100','mov ah,0x4a','int 0x21','jc failure',
       'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld']
    for name,delta,n,meta in (('caller',0x1000,0x154D,0),('system',0x2000,0x16BC,2)):
        a+=['mov ax,cs',f'add ax,{delta}',f'mov word ptr cs:[output+{meta}],ax','mov es,ax',f'mov si,offset {name}','xor di,di',f'mov cx,{n}','rep movsb']
        if stop and stop[0]==name:
            a += [f'mov byte ptr es:[{stop[1]}],0xea',f'mov word ptr es:[{stop[1]+1}],offset returned','mov ax,cs',f'mov word ptr es:[{stop[1]+3}],ax']
    a += ['mov ax,word ptr cs:[output]','mov word ptr cs:[dispatch+2],ax','mov es,ax','mov ax,word ptr cs:[output+2]']
    # Relocate all original SYSTEM call segment words in the closed U25 chain.
    bodies,decoded=source()
    for (kind,ip),i in decoded.items():
        if kind=='u25' and i.mnemonic=='lcall':a += [f'mov word ptr es:[{ip+3}],ax']
    a += ['mov ax,cs','add ax,0x3000','mov ss,ax','mov es,ax','mov word ptr cs:[output+4],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
          'mov ax,cs','add ax,0x4000','mov ds,ax','mov es,ax','mov word ptr cs:[output+6],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb']
    data=m[physical(0x4000,0):physical(0x4000,0)+65536];i=0
    while i<len(data):
        if data[i]==0xA5:i+=1;continue
        end=i+1
        while end<len(data) and data[end]==data[i]:end+=1
        if end-i>=4:a += [f'mov di,{i}',f'mov cx,{end-i}',f'mov al,{data[i]}','rep stosb']
        else:a += [f'mov byte ptr [{j}],{data[j]}' for j in range(i,end)]
        i=end
    a += ['mov ax,ds']+[f'mov word ptr [{off}],ax' for off in (0x49D4,0x49D8,0x49DC,0x49E0,0x5D94)]
    a += [f'mov word ptr ss:[{sp}],offset returned','mov ax,cs',f'mov word ptr ss:[{sp+2}],ax','mov word ptr cs:[output+10],ax','mov word ptr cs:[output+8],offset returned',
          'mov sp,0x8000',f'push {c.flags & ~0x200}','popf',f'mov ax,{c.es}','mov es,ax']
    for n in ('ax','bx','cx','dx','si','di','bp'):a += [f'mov {n},{getattr(c,n)}']
    a += [f'mov sp,{sp}','jmp far ptr cs:[dispatch]','returned:']
    for i,n in enumerate(NAMES):
        if n in ('ip','flags'):continue
        a += [f'mov word ptr cs:[output+{12+2*i}],{n}']
    a += ['mov word ptr cs:[output+30],offset returned','mov sp,0x8000','pushf','pop word ptr cs:[output+38]']
    if stop:
        a += [f'mov ax,word ptr cs:[output+{0 if stop[0]=="caller" else 2}]','mov word ptr cs:[output+28],ax',f'mov word ptr cs:[output+30],{stop[1]}']
    a += ['mov ax,cs','mov es,ax','mov ax,ss','mov ds,ax','cld','mov di,offset output+40']
    for off,n in WINDOWS:a += [f'mov si,{off}',f'mov cx,{n}','rep movsb']
    a += ['mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti',
          'mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{HEADER}','mov ah,0x40','int 0x21','jc failure',f'cmp ax,{HEADER}','jne failure',
          'mov ax,word ptr cs:[output+6]','mov ds,ax','xor dx,dx','mov cx,0x8000','mov bx,word ptr cs:[handle]','mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure',
          'mov dx,0x8000','mov cx,0x8000','mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure',
          'mov ah,0x3e','int 0x21','mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21',
          'filename: .asciz "RESULT.BIN"','handle: .word 0','dispatch: .word 0x1b6,0',f'output: .space {HEADER}']
    return '\n'.join(a)+'\n'


def main():
    OUT.mkdir(parents=True,exist_ok=True);bodies,decoded=source();blobs={'u25':bytearray(0x154D),'sys':bytearray(0x16BC)}
    for (kind,start),data in bodies.items():blobs[kind][start:start+len(data)]=data
    paths=[]
    for kind in ('u25','sys'):
        p=OUT/(kind+'.bin');p.write_bytes(blobs[kind]);paths.append(p)
    f=library(OUT)
    cases=[dict(),dict(restored=1),dict(restored=255)]
    for tag in (0,1,2,3,0x55,0x80,0xFF):cases.append(dict(operands=[operand(tag)]*5))
    for addr in (0,0x4900,0x6B00,0x6B01,0x9700,0xA000,0xC000,0xFFFF):cases.append(dict(operands=[operand(0x81,addr&255,addr>>8)]*5))
    for n in (0,1,12,127,255):cases.append(dict(operands=[bytes((0x80,n))+bytes([255])*n]*5))
    for sp in (0x236,0x237,0xFFFB,0xFFFC):cases.append(dict(sp=sp,operands=[operand(0x80)]*5))
    cases += [dict(alias='restored'),dict(alias='operand'),dict(budgets=(32,32,512),stop=('caller',0x2FC))]
    hashes=[];statuses={}
    for index,case in enumerate(cases):
        case=dict(flags=(2,0xD7,0x8D7,0x402)[index%4],**case)
        d=OUT/f'case-{index:02d}';d.mkdir(exist_ok=True);asm=d/'oracle.S';obj=d/'oracle.o';com=d/'ORACLE.COM';asm.write_text(assembly(paths,case))
        subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True);subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
        assert com.stat().st_size<0xEF00
        cfg=d/'dosbox.conf';cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(d)+'\nc:\nORACLE.COM\nexit\n')
        output=d/'RESULT.BIN';output.unlink(missing_ok=True)
        with (d/'dosbox.log').open('w') as log:subprocess.run(['dosbox-x','-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
        result=output.read_bytes();assert len(result)==RECORD,(index,len(result),RECORD)
        cs,system,ss,ds,retip,retcs=struct.unpack_from('<6H',result);regs=struct.unpack_from('<14H',result,12)
        m,c,_=fixture(**options(case,cs,system,ss,ds));c.flags &= ~0x200
        put16(m,ss,c.sp,retip);put16(m,ss,c.sp+2,retcs);install(m,c,system,bodies,decoded)
        ram=(ctypes.c_uint8*SIZE).from_buffer(m);status=f(ram,SIZE,ctypes.byref(c),system,*case.get('budgets',(128,32,512)))
        assert status==(2 if case.get('stop') else 0),(index,'status',status)
        for n,v in zip(NAMES,regs):assert getattr(c,n)==v,(index,n,hex(getattr(c,n)),hex(v))
        offset=40
        for start,n in WINDOWS:
            actual=result[offset:offset+n];expected=bytes(m[physical(ss,start):physical(ss,start)+n]);assert actual==expected,(index,'stack',hex(start),[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10]);offset+=n
        actual=result[HEADER:];expected=bytes(m[physical(ds,0):physical(ds,0)+65536]);assert actual==expected,(index,'DS',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
        hashes.append(sha256(result).hexdigest());statuses[str(status)]=statuses.get(str(status),0)+1;print('PASS native',index,flush=True)
    report=dict(target='OVL_0025_01B6',cases=len(cases),full_returns=statuses.get('0',0),partial_stops=statuses.get('2',0),cpu_words=14,stack_bytes=STACK_BYTES,data_bytes=65536,masked_flags=0,status_counts=statuses,capture_sha256=hashes,native_route_measured=False,wrapped_frames_measured=False)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()
