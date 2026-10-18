#!/usr/bin/env python3
"""Native full-chain U5E +1BF8 replay, with explicitly labelled partial captures."""
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import subprocess
from u5e_neighbor_1bf8_audit import Cpu, ROOT, SIZE, physical, put16, source, library, fixture, install

OUT=ROOT/'resources/u5e-1bf8/native'
STACK=tuple(range(0xEFC0,0xF020))+tuple(range(0x50))+tuple(range(0xFFC0,0x10000))
DATA_SIZE=0x8000
RECORD_SIZE=44+len(STACK)+DATA_SIZE


def options_for(options,cs,slots,system,stub,ss,ds):
    kw={k:v for k,v in options.items() if k not in ('alias','stop','budget')}
    kw=dict(dict(cs=cs,slots_cs=slots,system_cs=system,stub_cs=stub,ss=ss,ds=ds,sp=0xEFF8,
                 data_cs=ds,map_ptr=(ds,0x100),output=(ds,0x800),flags=2),**kw)
    if options.get('alias')=='record':kw['output']=(ds,0x2000)
    if options.get('alias')=='coordinates':kw['output']=(ds,0x5F2B)
    if options.get('alias')=='code':kw['output']=(cs,0x1BD8)
    return kw


def assembly(paths,options):
    kw=options_for(options,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000)
    memory,_,_,_=fixture(**kw);sp=kw['sp'];stop=options.get('stop')
    a=['.intel_syntax noprefix','.code16','.global _start','_start: jmp main',
       '.org 0x200','caller:',f'.incbin "{paths[0].resolve()}"',
       '.org 0x2200','slots:',f'.incbin "{paths[1].resolve()}"',
       '.org 0x2f00','system:',f'.incbin "{paths[2].resolve()}"',
       '.org 0x3e00','main:','mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
       'mov bx,0x7100','mov ah,0x4a','int 0x21','jc failure','mov dx,offset filename',
       'xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax','cli','cld']
    for name,delta,size,meta in (('caller',0x1000,0x2000,0),('slots',0x2000,0xD00,2),('system',0x3000,0xE00,4)):
        a+=['mov ax,cs',f'add ax,{delta}',f'mov word ptr cs:[output+{meta}],ax','mov es,ax',
            f'mov si,offset {name}','xor di,di',f'mov cx,{size}','rep movsb']
        if stop and stop[0]==name:
            a += [f'mov byte ptr es:[{stop[1]}],0xea',f'mov word ptr es:[{stop[1]+1}],offset returned',
                  'mov ax,cs',f'mov word ptr es:[{stop[1]+3}],ax']
    # Reproduce the loader's far-jump stubs; only link segment words change.
    a+=['mov ax,cs','add ax,0x4000','mov word ptr cs:[output+6],ax','mov es,ax']
    for off,target in ((0x5C,0xBE7),(0x61,0xC0F),(0x3E,0x4BE)):
        a += [f'mov byte ptr es:[{off}],0xea',f'mov word ptr es:[{off+1}],{target}',
              'mov ax,word ptr cs:[output+2]',f'mov word ptr es:[{off+3}],ax']
    a+=['mov ax,word ptr cs:[output]','mov word ptr cs:[dispatch+2],ax','mov es,ax',
        'mov ax,word ptr cs:[output+6]']
    for off in (0x1C20,0x1C3F,0x1C6A):a += [f'mov word ptr es:[{off}],ax']
    a+=['mov ax,word ptr cs:[output+4]','mov word ptr es:[0x1d76],ax',
        'mov ax,cs','add ax,0x5000','mov ss,ax','mov es,ax','mov word ptr cs:[output+8],ax',
        'xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb',
        'mov ax,ss','add ax,0x1000','mov es,ax','mov word ptr cs:[output+10],ax',
        'xor di,di','mov cx,0xffff','mov al,0xa5','rep stosb','stosb','mov ax,es','mov ds,ax']
    data=memory[physical(0x6000,0):physical(0x6000,0)+DATA_SIZE]
    i=0
    while i<len(data):
        if data[i]==0xA5:i+=1;continue
        end=i+1
        while end<len(data) and data[end]==data[i]:end+=1
        if end-i>=4:a += [f'mov di,{i}',f'mov cx,{end-i}',f'mov al,{data[i]}','rep stosb']
        else:a += [f'mov byte ptr [{j}],{data[j]}' for j in range(i,end)]
        i=end
    # Relocate only the synthetic fixture's DS-based far pointers.
    offsets={0x65BB+4*i for i in (1,2,3)}
    if options.get('subject_slot') is not None:offsets.add(0x65BB+4*options['subject_slot'])
    offsets.add(0x6718)
    offsets.update(0x66D8+7*i for i in range(1,len(kw.get('records',((4,0,0,5),(5,1,0,6),(6,0,1,9))))+1))
    for off in sorted(offsets):
        value=data[off]|(data[off+1]<<8)
        if value in (0x5FFF,0x6000):
            a += ['mov ax,ds']
            if value==0x5FFF:a+=['dec ax']
            a += [f'mov word ptr [{off}],ax']
    for off,value in ((0,'offset returned'),(4,str(kw['output'][1])),(8,'0x2000')):
        a += [f'mov word ptr ss:[{sp+off}],{value}']
    a += ['mov ax,word ptr cs:[output]' if kw['output'][0]==0x1000 else 'mov ax,ds',
          f'mov word ptr ss:[{sp+6}],ax','mov ax,ds',f'mov word ptr ss:[{sp+10}],ax',
          'mov ax,cs',f'mov word ptr ss:[{sp+2}],ax','mov word ptr cs:[output+14],ax',
          'mov word ptr cs:[output+12],offset returned','mov sp,0x8000','push 2','popf',
          'mov ax,0x1111','mov es,ax','mov ax,0xabcd','mov bx,0x3456','mov cx,0x5678',
          'mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',f'mov sp,{sp}',
          'jmp far ptr cs:[dispatch]','returned:']
    for i,reg in enumerate('ax bx cx dx si di bp sp cs'.split()):
        a += [f'mov word ptr cs:[output+{16+2*i}],{reg}']
    a += ['mov word ptr cs:[output+34],offset returned','mov word ptr cs:[output+36],ds',
          'mov word ptr cs:[output+38],es','mov word ptr cs:[output+40],ss','mov sp,0x8000',
          'pushf','pop word ptr cs:[output+42]']
    if stop:
        meta={'caller':0,'slots':2,'system':4}[stop[0]]
        a += [f'mov ax,word ptr cs:[output+{meta}]','mov word ptr cs:[output+32],ax',
              f'mov word ptr cs:[output+34],{stop[1]}']
    for i,off in enumerate(STACK):
        a += [f'mov al,byte ptr ss:[{off}]',f'mov byte ptr cs:[output+{44+i}],al']
    a += ['mov ax,cs','mov es,ax','xor si,si',f'mov di,offset output+{44+len(STACK)}',
          f'mov cx,{DATA_SIZE}','cld','rep movsb','mov ax,cs','mov ss,ax','mov ds,ax','mov sp,0xf000','sti',
          'mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{RECORD_SIZE}','mov ah,0x40','int 0x21',
          'jc failure',f'cmp ax,{RECORD_SIZE}','jne failure','mov bx,word ptr [handle]','mov ah,0x3e','int 0x21',
          'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"',
          'handle: .word 0','dispatch: .word 0x1bf8,0',f'output: .space {RECORD_SIZE}']
    return '\n'.join(a)+'\n'


def main():
    bodies,_,data=source();OUT.mkdir(parents=True,exist_ok=True)
    blobs={0x5E:bytearray(0x2000),0x3C6:bytearray(0xD00),0xAF8:bytearray(0xE00)}
    for (unit,start),b in bodies.items():blobs[unit][start:start+len(b)]=b
    blobs[0x5E][0x1BD8:0x1BF8]=data
    paths=[]
    for unit,name in ((0x5E,'caller'),(0x3C6,'slots'),(0xAF8,'system')):
        p=OUT/(name+'.bin');p.write_bytes(blobs[unit]);paths.append(p)
    lib=library(OUT)
    cases=[dict(),dict(occupants=(2,)*9),dict(occupants=(3,)*9),dict(occupants=(1,)*9),
           dict(occupants=(2,1,2,1,0,0,0,0,0),ranks=(2,1,3)),
           dict(occupants=(2,1,2,1,0,0,0,0,0),ranks=(10,1,3)),
           dict(occupants=(2,)*9,ranks=(6,20,1)),dict(tiles=(31,)*9),
           dict(tiles=(31,)*9,records=()),dict(tiles=(31,)*9,records=((0,0,0,5),)),
           dict(tiles=(31,)*9,records=((4,0,0,6),)),dict(tiles=(31,)*9,records=((4,0,0,7),)),
           dict(tiles=(31,)*9,records=((4,0,0,8),)),dict(tiles=(31,)*9,records=((4,0,0,9),))]
    cases += [dict(tiles=(31,)*9,occupants=(2,0,0,0,0,0,0,0,0),ranks=(6,r,1)) for r in (0,7,8,19)]
    cases += [dict(x=x,y=y,tiles=(31,)*9) for x,y in ((0,0),(49,24),(128,255),(50,25))]
    cases += [dict(subject_slot=None),dict(subject_slot=255,count=0,budget=8192),
              dict(equivalent='segment',occupants=(1,2,0,0,0,0,0,0,0),ranks=(2,2,1)),
              dict(equivalent='offset',occupants=(1,2,0,0,0,0,0,0,0),ranks=(2,2,1))]
    cases += [dict(alias=a,occupants=(2,)*9) for a in ('record','coordinates')]
    cases += [dict(sp=sp,tiles=(31,)*9) for sp in (42,0xFFF4)]
    cases += [dict(budget=1,stop=('slots',0xC71)),
              dict(budget=3,stop=('caller',0x1D42),tiles=(31,)*9),
              dict(alias='code',stop=('caller',0x1DD0)),
              dict(alias='code',stop=('caller',0x1DF0),tiles=(31,)*9)]
    hashes=[];statuses={};stop_counts={}
    for index,options in enumerate(cases):
        d=OUT/f'case-{index:02d}';d.mkdir(exist_ok=True)
        asm,obj,com=d/'oracle.S',d/'oracle.o',d/'ORACLE.COM'
        asm.write_text(assembly(paths,options))
        subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
        subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
        assert com.stat().st_size<0xEF00,com.stat().st_size
        cfg=d/'dosbox.conf'
        cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(d)+'\nc:\nORACLE.COM\nexit\n')
        output=d/'RESULT.BIN';output.unlink(missing_ok=True)
        with (d/'dosbox.log').open('w') as log:
            subprocess.run(['dosbox-x','-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],
                           env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),
                           stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
        result=output.read_bytes();assert len(result)==RECORD_SIZE,(index,len(result))
        cs,slots,system,stub,ss,ds,return_ip,return_cs=struct.unpack_from('<8H',result)
        regs=struct.unpack_from('<14H',result,16)
        memory,cpu,layout,_=fixture(**options_for(options,cs,slots,system,stub,ss,ds))
        put16(memory,ss,cpu.sp,return_ip);put16(memory,ss,(cpu.sp+2)&65535,return_cs)
        install(memory,cpu,layout,stub,bodies,data)
        ram=(ctypes.c_uint8*SIZE).from_buffer(memory)
        status=lib.pr_u5e_neighbor_1bf8(ram,SIZE,ctypes.byref(cpu),ctypes.byref(layout),options.get('budget',4096))
        expected=3 if options.get('stop') and 'budget' in options else 2 if options.get('alias')=='code' else 0
        assert status==expected,(index,'status',status,expected)
        for (name,_),value in zip(Cpu._fields_,regs):
            assert getattr(cpu,name)==value,(index,name,hex(getattr(cpu,name)),hex(value))
        for i,off in enumerate(STACK):
            assert memory[physical(ss,off)]==result[44+i],(index,'stack',hex(off),memory[physical(ss,off)],result[44+i])
        expected_data=memory[physical(ds,0):physical(ds,0)+DATA_SIZE];actual_data=result[44+len(STACK):]
        assert expected_data==actual_data,(index,'data',[(i,a,b) for i,(a,b) in enumerate(zip(expected_data,actual_data)) if a!=b][:12])
        hashes.append(sha256(result).hexdigest());statuses[str(status)]=statuses.get(str(status),0)+1
        if status:stop_counts[f'{status}:{cpu.ip:04X}']=stop_counts.get(f'{status}:{cpu.ip:04X}',0)+1
        print(f'PASS native case {index}: {options}',flush=True)
    report=dict(target='OVL_005E_1BF8',native_cases=len(cases),full_returns=statuses.get('0',0),
                partial_stops=len(cases)-statuses.get('0',0),cpu_words=14,stack_bytes=len(STACK),
                data_bytes=DATA_SIZE,masked_flags=0,status_counts=statuses,stop_counts=stop_counts,
                native_route_measured=False,les_operand_seams_measured=False,wrapped_return_frames_measured=False,
                result_sha256=hashes)
    (OUT/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report,sort_keys=True))


if __name__=='__main__':main()
