#!/usr/bin/env python3
"""DOSBox-X full-chain U3C6 map-rebuild captures; synthetic fixtures."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u3c6_rebuild_03a0_audit import ROOT,SIZE,Cpu,NAMES,source,library,fixture,install,physical,put16

OUT=ROOT/'resources/u3c6-03a0/native'
WINDOWS=((0xEFC0,0x80),(0,0x40),(0xFFC0,0x40))
STACK_BYTES=sum(n for _,n in WINDOWS)
HEADER=40+STACK_BYTES
RECORD=HEADER+65536


def options(case,cs,system,ss,ds):
    kw={k:v for k,v in case.items() if k!='stack_alias'}
    kw.update(cs=cs,system=system,ss=ds if case.get('stack_alias') else ss,ds=ds)
    return kw


def assembly(paths,case):
    m,c,_=fixture(**options(case,0x1000,0x2000,0x3000,0x4000));sp=c.sp
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main',
       '.org 0x200','caller:',f'.incbin "{paths[0].resolve()}"',
       '.org 0x1800','system:',f'.incbin "{paths[1].resolve()}"',
       '.org 0x3000','main:','mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
       'mov bx,0x5100','mov ah,0x4a','int 0x21','jc failure',
       'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld']
    for name,delta,n,meta in (('caller',0x1000,0x4BE,0),('system',0x2000,0x16BC,2)):
        a+=['mov ax,cs',f'add ax,{delta}',f'mov word ptr cs:[output+{meta}],ax','mov es,ax',f'mov si,offset {name}','xor di,di',f'mov cx,{n}','rep movsb']
    a += ['mov ax,word ptr cs:[output]','mov word ptr cs:[dispatch+2],ax','mov es,ax','mov ax,word ptr cs:[output+2]']
    # Relocate the original SYSTEM call segment word.
    bodies,decoded=source()
    for (kind,ip),i in decoded.items():
        if kind=='u3c6' and i.mnemonic=='lcall':a += [f'mov word ptr es:[{ip+3}],ax']
    a += ['mov ax,cs','add ax,0x3000','mov ss,ax','mov es,ax','mov word ptr cs:[output+4],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
          'mov ax,cs','add ax,0x4000','mov ds,ax','mov es,ax','mov word ptr cs:[output+6],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb']
    def initialize(segment,prefix):
        data=m[physical(segment,0):physical(segment,0)+65536];i=0
        while i<len(data):
            if data[i]==0xA5:i+=1;continue
            end=i+1
            while end<len(data) and data[end]==data[i]:end+=1
            # Scalar writes keep DS and ES available for the actual record links.
            a.extend(f'mov byte ptr {prefix}:[{j}],{data[j]}' for j in range(i,end))
            i=end
    initialize(0x4000,'ds');initialize(0x3000,'ss')
    # Rebind the synthetic map pointer to the actual DOS allocation.
    a += ['mov ax,ds','mov word ptr ds:[0x6718],ax']
    if case.get('stack_alias'):
        a += ['mov ax,ds','mov ss,ax','mov word ptr cs:[output+4],ax']
    a += [f'mov word ptr ss:[{sp}],offset returned','mov ax,cs',f'mov word ptr ss:[{sp+2}],ax','mov word ptr cs:[output+10],ax','mov word ptr cs:[output+8],offset returned',
          'mov ax,cs','mov ss,ax','mov sp,0xf000',f'push {c.flags & ~0x200}','popf','mov ax,word ptr cs:[output+4]','mov ss,ax',f'mov ax,{c.es}','mov es,ax']
    for n in ('ax','bx','cx','dx','si','di','bp'):a += [f'mov {n},{getattr(c,n)}']
    a += [f'mov sp,{sp}','jmp far ptr cs:[dispatch]','returned:']
    for i,n in enumerate(NAMES):
        if n in ('ip','flags'):continue
        a += [f'mov word ptr cs:[output+{12+2*i}],{n}']
    a += ['mov word ptr cs:[output+30],offset returned','mov ax,cs','mov ss,ax','mov sp,0xf000','pushf','pop word ptr cs:[output+38]']
    a += ['mov ax,cs','mov es,ax','mov ax,word ptr cs:[output+4]','mov ds,ax','cld','mov di,offset output+40']
    for off,n in WINDOWS:a += [f'mov si,{off}',f'mov cx,{n}','rep movsb']
    a += ['mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti',
          'mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{HEADER}','mov ah,0x40','int 0x21','jc failure',f'cmp ax,{HEADER}','jne failure',
          'mov ax,word ptr cs:[output+6]','mov ds,ax','xor dx,dx','mov cx,0x8000','mov bx,word ptr cs:[handle]','mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure',
          'mov dx,0x8000','mov cx,0x8000','mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure',
          'mov ah,0x3e','int 0x21','mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21',
          'filename: .asciz "RESULT.BIN"','handle: .word 0','dispatch: .word 0x3a0,0',f'output: .space {HEADER}']
    return '\n'.join(a)+'\n'


def main():
    OUT.mkdir(parents=True,exist_ok=True);bodies,decoded=source();blobs={'u3c6':bytearray(0x4BE),'sys':bytearray(0x16BC)}
    for (kind,start),data in bodies.items():blobs[kind][start:start+len(data)]=data
    paths=[]
    for kind in ('u3c6','sys'):
        p=OUT/(kind+'.bin');p.write_bytes(blobs[kind]);paths.append(p)
    f=library(OUT)
    cases=[dict(),dict(party=()),dict(party=((10,10,0),)),dict(shape=((-1,0),)*4),
           dict(shape=((0,0),(1,0),(0,1),(1,1))),dict(party=((10,10,1),(10,10,1)))]
    cases += [dict(party=((v,v,1),)) for v in (0,49,50,127,128,255)]
    cases += [dict(shape=((x,y),)*4) for x,y in ((127,127),(0,128),(0,255),(128,0),(255,255))]
    cases += [dict(origin=(v,v)) for v in (0,127,128,255)]
    cases += [dict(sp=v) for v in (0x1C,0x1D,0xFFFB,0xFFFC)]
    cases += [dict(map_offset=v) for v in (0xFFFD,0xFFFE,0xFFFF,0x60DB,0x6048,0x6090)]
    cases += [dict(party=tuple((i%50,i%25,1) for i in range(n))) for n in (72,109,255)]
    cases += [dict(stack_alias=True)]
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
        ram=(ctypes.c_uint8*SIZE).from_buffer(m);status=f(ram,SIZE,ctypes.byref(c),system,512,4096)
        assert status==0,(index,'status',status)
        for n,v in zip(NAMES,regs):assert getattr(c,n)==v,(index,n,hex(getattr(c,n)),hex(v))
        offset=40
        for start,n in WINDOWS:
            actual=result[offset:offset+n];expected=bytes(m[physical(ss,start):physical(ss,start)+n]);assert actual==expected,(index,'stack',hex(start),[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10]);offset+=n
        actual=result[HEADER:];expected=bytes(m[physical(ds,0):physical(ds,0)+65536]);assert actual==expected,(index,'DS',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
        hashes.append(sha256(result).hexdigest());statuses[str(status)]=statuses.get(str(status),0)+1;print('PASS native',index,flush=True)
    report=dict(target='OVL_03C6_03A0',cases=len(cases),full_returns=statuses.get('0',0),partial_stops=statuses.get('2',0),cpu_words=14,stack_bytes=STACK_BYTES,data_bytes=65536,masked_flags=0,status_counts=statuses,capture_sha256=hashes,native_route_measured=False,wrapped_frames_measured=False)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()
