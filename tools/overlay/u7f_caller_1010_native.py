#!/usr/bin/env python3
"""DOSBox-X original-chain CPU14/DS/stack captures for U7F +1010."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u7f_caller_1010_audit import ROOT,SIZE,NAMES,source,library,fixture,install,physical,put16,SYSTEM
OUT=ROOT/'resources/u7f-1010/native'
WINDOWS=((0xEF00,0x140),(0,0x100),(0xFEE0,0x120))
STACK_BYTES=sum(n for _,n in WINDOWS)
HEADER=42+STACK_BYTES

def assembly(paths,case,decoded):
    m,c,_=fixture(**case,cs=0x1000,dice=0x2000,ss=0x4000,ds=0x5000)
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main',
       '.org 0x200','caller:',f'.incbin "{paths[0]}"',
       '.org 0x1B00','dice:',f'.incbin "{paths[1]}"',
       '.org 0x5000','system:',f'.incbin "{paths[2]}"',
       '.org 0x7000','main:','mov ax,cs','cmp ax,0x814','jne failure','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
       'mov bx,0x6100','mov ah,0x4a','int 0x21','jc failure','mov dx,offset filename','xor cx,cx',
       'mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld']
    for name,delta,length,meta in (('caller',0x1000,0x18E0,0),('dice',0x2000,0xE2E,2),('system',None,0x16BC,4)):
        a+=(['mov ax,cs',f'add ax,{delta}'] if delta is not None else ['mov ax,0xAF8'])+[f'mov word ptr cs:[output+{meta}],ax','mov es,ax',f'mov si,offset {name}','xor di,di',f'mov cx,{length}','rep movsb']
    a+=['mov ax,word ptr cs:[output]','mov word ptr cs:[dispatch+2],ax','mov es,ax']
    for (kind,ip),ins in decoded.items():
        if kind=='u7f' and ins.mnemonic=='lcall' and ins.op_str=='0xb0, 0x48':
            a += [f'mov word ptr es:[{ip+1}],{0x48 if ip==0xFDE else 0xDE3}',
                  'mov ax,word ptr cs:[output+2]']
            if ip==0xFDE:a+=['add ax,0x100']
            a += [f'mov word ptr es:[{ip+3}],ax']
    a += ['mov ax,word ptr cs:[output+2]','add ax,0x100','mov es,ax',
          'mov byte ptr es:[0x48],0xea','mov word ptr es:[0x49],0xde3',
          'mov ax,word ptr cs:[output+2]','mov word ptr es:[0x4b],ax']
    a+=['mov ax,cs','add ax,0x4000','mov ss,ax','mov es,ax','mov word ptr cs:[output+6],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
        'mov ax,cs','add ax,0x5000','mov ds,ax','mov es,ax','mov word ptr cs:[output+8],ax','xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb']
    for seg,prefix in ((0x5000,'ds'),(0x4000,'ss')):
        data=m[physical(seg,0):physical(seg,0)+65536];i=0
        while i<len(data):
            if data[i]==0xA5:i+=1;continue
            end=i+1
            while end<len(data) and data[end]==data[i]:end+=1
            if prefix=='ds' and end-i>=4:a += [f'mov di,{i}',f'mov cx,{end-i}',f'mov al,{data[i]}','rep stosb']
            else:a += [f'mov byte ptr {prefix}:[{j}],{data[j]}' for j in range(i,end)]
            i=end
    a+=['mov ax,ds',f'mov word ptr ss:[{c.sp+6}],ax',
        f'mov word ptr ss:[{c.sp}],offset returned','mov ax,cs',f'mov word ptr ss:[{c.sp+2}],ax',
        'mov word ptr cs:[output+12],ax','mov word ptr cs:[output+10],offset returned',
        'mov sp,0x8000',f'push {c.flags&~0x200}','popf',f'mov ax,{c.es}','mov es,ax']
    for n in ('ax','bx','cx','dx','si','di','bp'):a+=[f'mov {n},{getattr(c,n)}']
    a += [f'mov sp,{c.sp}','jmp far ptr cs:[dispatch]','returned:']
    for i,n in enumerate(NAMES):
        if n not in ('ip','flags'):a += [f'mov word ptr cs:[output+{14+2*i}],{n}']
    a+=['mov word ptr cs:[output+32],offset returned','mov sp,0x8000','pushf','pop word ptr cs:[output+40]',
        'mov ax,cs','mov es,ax','mov ax,ss','mov ds,ax','cld','mov di,offset output+42']
    for off,n in WINDOWS:a += [f'mov si,{off}',f'mov cx,{n}','rep movsb']
    a+=['mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti','mov bx,word ptr [handle]',
        'mov dx,offset output',f'mov cx,{HEADER}','mov ah,0x40','int 0x21','jc failure',f'cmp ax,{HEADER}','jne failure',
        'mov ax,word ptr cs:[output+8]','mov ds,ax','xor dx,dx','mov cx,0x8000','mov bx,word ptr cs:[handle]',
        'mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure','mov dx,0x8000','mov cx,0x8000',
        'mov ah,0x40','int 0x21','jc failure','cmp ax,0x8000','jne failure','mov ah,0x3e','int 0x21',
        'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"',
        'handle: .word 0','dispatch: .word 0x1010,0',f'output: .space {HEADER}']
    return '\n'.join(a)+'\n'

def main():
    OUT.mkdir(parents=True,exist_ok=True);bodies,decoded=source();blobs={'u7f':bytearray(0x18E0),'dice':bytearray(0xE2E),'sys':bytearray(0x16BC)}
    for (kind,start),data in bodies.items():blobs['sys' if kind=='data' else kind][start:start+len(data)]=data
    paths=[]
    for kind in ('u7f','dice','sys'):
        p=OUT/(kind+'.bin');p.write_bytes(blobs[kind]);paths.append(p.resolve())
    f=library(OUT)
    cases=[dict(kind=k,seed=0x12345678) for k in (0,1,21,28,50,51,52,53,54,55,56,57,58,59,61,62,63,67,69,70,71,73,77,78,79,84,92,93,255)]
    cases += [dict(kind=61,seed=k) for k in (0,1,4,9)]
    cases += [dict(kind=1,sp=k) for k in (0x40,0x41,0xFFF5,0xFFF6)]
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
        cs,dice,system,ss,ds,retip,retcs=struct.unpack_from('<7H',result);regs=struct.unpack_from('<14H',result,14);assert system==SYSTEM
        m,c,_=fixture(**case,cs=cs,dice=dice,ss=ss,ds=ds);c.flags &= ~0x200
        put16(m,ss,c.sp,retip);put16(m,ss,c.sp+2,retcs);install(m,c,dice,bodies,decoded)
        ram=(ctypes.c_uint8*SIZE).from_buffer(m);status=f(ram,SIZE,ctypes.byref(c),dice,1024,256)
        assert status==0,(index,status)
        for n,v in zip(NAMES,regs):assert getattr(c,n)==v,(index,n,hex(getattr(c,n)),hex(v))
        offset=42
        for start,n in WINDOWS:
            actual=result[offset:offset+n];expected=bytes(m[physical(ss,start):physical(ss,start)+n]);assert actual==expected,(index,'stack',hex(start),[(i,a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10]);offset+=n
        actual=result[HEADER:];expected=bytes(m[physical(ds,0):physical(ds,0)+65536]);assert actual==expected,(index,'DS',[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:12])
        hashes.append(sha256(result).hexdigest());print('PASS native',index,flush=True)
    report=dict(target='OVL_007F_1010',cases=len(cases),full_returns=len(cases),cpu_words=14,stack_bytes=STACK_BYTES,data_bytes=65536,masked_flags=0,capture_sha256=hashes,native_route_measured=False,wrapped_frames_measured=False)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
