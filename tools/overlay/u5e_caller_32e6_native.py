#!/usr/bin/env python3
"""DOSBox-X original-chain CPU14/DS/stack captures for U5E +32E6."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u5e_caller_32e6_audit import ROOT,SIZE,NAMES,source,library,fixture,install,physical,put16,Layout,Limits,DEFAULT,call_target
OUT=ROOT/'resources/u5e-32e6/native'
WINDOWS=((0xEF00,0x140),(0,0x100),(0xFEE0,0x120),(0xDF00,0x400))
STACK_BYTES=sum(n for _,n in WINDOWS)
HEADER=44+STACK_BYTES

def assembly(paths,case,decoded):
    m,c,_=fixture(**case,cs=0x1000,slots=0x2000,chain=0x3000,system=0x4000,ss=0x5000,ds=0x6000)
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main',
       '.org 0x200','caller:',f'.incbin "{paths[0]}"',
       '.org 0x3600','slots:',f'.incbin "{paths[1]}"',
       '.org 0x4400','chain:',f'.incbin "{paths[2]}"',
       '.org 0x5000','system:',f'.incbin "{paths[3]}"',
       '.org 0x6000','main:','mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
       'mov bx,0x7100','mov ah,0x4a','int 0x21','jc failure','mov dx,offset filename','xor cx,cx',
       'mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld']
    for name,delta,length,meta in (('caller',0x1000,0x3389,0),('slots',0x2000,0xCB9,2),('chain',0x3000,0xBD9,4),('system',0x4000,0xA57,6)):
        a+=['mov ax,cs',f'add ax,{delta}',f'mov word ptr cs:[output+{meta}],ax','mov es,ax',f'mov si,offset {name}','xor di,di',f'mov cx,{length}','rep movsb']
    a+=['mov ax,word ptr cs:[output]','mov word ptr cs:[dispatch+2],ax']
    metadata={'caller':0,'slots':2,'chain':4,'sys':6}
    for (kind,ip),ins in decoded.items():
        if ins.mnemonic=='lcall':
            target,off=call_target(kind,ip,ins)
            a += [f'mov ax,word ptr cs:[output+{metadata[kind]}]','mov es,ax',
                  f'mov word ptr es:[{ip+1}],{off}',f'mov ax,word ptr cs:[output+{metadata[target]}]',f'mov word ptr es:[{ip+3}],ax']
    a+=['mov ax,cs','add ax,0x5000','mov ss,ax','mov es,ax','mov word ptr cs:[output+8],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
        'mov ax,cs','add ax,0x6000','mov ds,ax','mov es,ax','mov word ptr cs:[output+10],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb']
    for seg,prefix in ((0x6000,'ds'),(0x5000,'ss')):
        data=m[physical(seg,0):physical(seg,0)+65536];i=0
        while i<len(data):
            if data[i]==0xA5:i+=1;continue
            end=i+1
            while end<len(data) and data[end]==data[i]:end+=1
            if prefix=='ds' and end-i>=4:a += [f'mov di,{i}',f'mov cx,{end-i}',f'mov al,{data[i]}','rep stosb']
            else:a += [f'mov byte ptr {prefix}:[{j}],{data[j]}' for j in range(i,end)]
            i=end
    a+=['mov ax,ds','mov word ptr ds:[0x6718],ax',
        f'mov word ptr ss:[{c.sp}],offset returned','mov ax,cs',f'mov word ptr ss:[{c.sp+2}],ax',
        'mov word ptr cs:[output+14],ax','mov word ptr cs:[output+12],offset returned',
        'mov sp,0x8000',f'push {c.flags&~0x200}','popf',f'mov ax,{c.es}','mov es,ax']
    for n in ('ax','bx','cx','dx','si','di','bp'):a+=[f'mov {n},{getattr(c,n)}']
    a += [f'mov sp,{c.sp}','jmp far ptr cs:[dispatch]','returned:']
    for i,n in enumerate(NAMES):
        if n not in ('ip','flags'):a += [f'mov word ptr cs:[output+{16+2*i}],{n}']
    a+=['mov word ptr cs:[output+34],offset returned','mov sp,0x8000','pushf','pop word ptr cs:[output+42]',
        'mov ax,cs','mov es,ax','mov ax,ss','mov ds,ax','cld','mov di,offset output+44']
    for off,n in WINDOWS:a += [f'mov si,{off}',f'mov cx,{n}','rep movsb']
    a+=['mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti','mov bx,word ptr [handle]',
        'mov dx,offset output',f'mov cx,{HEADER}','mov ah,0x40','int 0x21','jc failure',f'cmp ax,{HEADER}','jne failure',
        'mov ax,word ptr cs:[output+10]','mov ds,ax','xor dx,dx','mov cx,0x8000','mov bx,word ptr cs:[handle]',
        'mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure','mov dx,0x8000','mov cx,0x8000',
        'mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure','mov ah,0x3e','int 0x21',
        'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"',
        'handle: .word 0','dispatch: .word 0x32e6,0',f'output: .space {HEADER}']
    return '\n'.join(a)+'\n'

def main():
    OUT.mkdir(parents=True,exist_ok=True);bodies,decoded=source();blobs={'caller':bytearray(0x3389),'slots':bytearray(0xCB9),'chain':bytearray(0xBD9),'sys':bytearray(0xA57)}
    for (kind,start),data in bodies.items():blobs[kind][start:start+len(data)]=data
    paths=[]
    for kind in ('caller','slots','chain','sys'):
        p=OUT/(kind+'.bin');p.write_bytes(blobs[kind]);paths.append(p.resolve())
    f=library(OUT)
    cases=[dict(),dict(party=()),dict(selected=256),dict(selected=0),dict(selected=2),
           dict(party=((10,10,0),(12,12,1))),dict(mode=0,heights=(0,255)),
           dict(party=((20,20,1),(11,11,1),(12,12,0))),dict(shape=((-1,0),)*4)]
    cases += [dict(party=((x,y,1),(12,12,1))) for x,y in ((0,0),(49,24),(50,25),(127,127),(128,128),(255,255))]
    cases += [dict(sp=v) for v in (0xA4,0xA5,0xFFF5,0xFFF6)]
    cases += [dict(parent=v) for v in (0xD9,0xDA,0xDB,0xDC,0xFFFF)]
    cases += [dict(party=tuple((20-i,20-i,1) for i in range(5)),selected=5)]
    cases += [dict(party=(),selected=256),dict(party=(),selected=0)]
    hashes=[]
    for index,case in enumerate(cases):
        case=dict(flags=(2,0xD7,0x8D7,0x402)[index%4],**case);d=OUT/f'case-{index:02d}';d.mkdir(exist_ok=True)
        asm=d/'oracle.S';obj=d/'oracle.o';com=d/'ORACLE.COM';asm.write_text(assembly(paths,case,decoded))
        subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True);subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
        assert com.stat().st_size<0xEF00
        cfg=d/'dosbox.conf';cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(d)+'\nc:\nORACLE.COM\nexit\n')
        output=d/'RESULT.BIN';output.unlink(missing_ok=True)
        with (d/'dosbox.log').open('w') as log:subprocess.run(['dosbox-x','-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
        result=output.read_bytes();assert len(result)==HEADER+65536,(index,len(result))
        cs,slots,chain,system,ss,ds,retip,retcs=struct.unpack_from('<8H',result);regs=struct.unpack_from('<14H',result,16)
        m,c,layout=fixture(**case,cs=cs,slots=slots,chain=chain,system=system,ss=ss,ds=ds);c.flags &= ~0x200
        put16(m,ss,c.sp,retip);put16(m,ss,c.sp+2,retcs);install(m,c,layout,bodies,decoded)
        ram=(ctypes.c_uint8*SIZE).from_buffer(m);limits=Limits(*DEFAULT);status=f(ram,SIZE,ctypes.byref(c),ctypes.byref(layout),ctypes.byref(limits))
        assert status==0,(index,status)
        for n,v in zip(NAMES,regs):assert getattr(c,n)==v,(index,n,hex(getattr(c,n)),hex(v))
        offset=44
        for start,n in WINDOWS:
            actual=result[offset:offset+n];expected=bytes(m[physical(ss,start):physical(ss,start)+n]);assert actual==expected,(index,'stack',hex(start),[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10]);offset+=n
        actual=result[HEADER:];expected=bytes(m[physical(ds,0):physical(ds,0)+65536]);assert actual==expected,(index,'DS',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
        hashes.append(sha256(result).hexdigest());print('PASS native',index,flush=True)
    report=dict(target='OVL_005E_32E6',cases=len(cases),full_returns=len(cases),cpu_words=14,stack_bytes=STACK_BYTES,data_bytes=65536,masked_flags=0,capture_sha256=hashes,native_route_measured=False,wrapped_frames_measured=False)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
