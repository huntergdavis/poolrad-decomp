#!/usr/bin/env python3
"""Synthetic direct calls into original getters, DOSBox-X normal/386."""
# NOT_A_ROW_CHECKER: native companion of u3c6_slot_getters_cpu_audit.py.
from native_emulator_provenance import describe_dosbox
import ctypes,os,struct,subprocess
from hashlib import sha256
from u3c6_slot_getters_cpu_audit import ROOT,SIZE,CS,DS,FLAGS,ENTRIES,Cpu,physical,put16,build,check

def native(code,function):
    dosbox_x=describe_dosbox('dosbox-x')
    directory=ROOT/'resources/u3c6-getters/native';directory.mkdir(parents=True,exist_ok=True)
    cases=[(entry,count,flags,k) for entry in ENTRIES for count in (0,1,2,5,127,128,254,255) for k,flags in enumerate(FLAGS)]
    low=0xEFC0;window=64;record_size=28+window+65536;inputs=bytearray();fixtures=[]
    for entry,count,flags,k in cases:
        selected=(0,1,count,255,2,5)[k];m,c=build(code,entry,count=count,selected=selected,flags=flags,mismatch=('segment' if k==4 else 'offset' if k==5 else 'none'))
        off=struct.unpack_from('<H',m,physical(c.ss,c.sp+4))[0];seg=struct.unpack_from('<H',m,physical(c.ss,c.sp+6))[0]
        inputs+=struct.pack('<4H',flags,entry,off,seg)+m[physical(DS,0):physical(DS,0)+65536]
        fixtures.append((m,entry,flags,off,seg))
    (directory/'INPUT.BIN').write_bytes(inputs)
    blob=directory/'code.bin';blob.write_bytes(code[:0xCB9])
    asm=['.code16','.intel_syntax noprefix','.global _start','_start: jmp main','.org 0x200','main:',
         'mov ax,cs','mov ds,ax','mov ss,ax','mov sp,0xf000','cld',f'mov ax,{CS}','mov es,ax',
         'xor di,di','mov si,offset body',f'mov cx,{0xCB9}','rep movsb',
         'mov word ptr [header],cs','mov word ptr [header+2],offset returned',f'mov word ptr [header+4],{len(cases)}',
         'mov dx,offset inputname','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [inputhandle],ax',
         'mov dx,offset outputname','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [handle],ax','mov bx,ax',
         'mov dx,offset header','mov cx,6','mov ah,0x40','int 0x21','jc failure',
         'again:','push cs','pop ds','mov bx,word ptr [inputhandle]','mov dx,offset spec','mov cx,8','call read_exact',
         'mov ax,word ptr [spec+2]','mov word ptr [target],ax',f'mov ax,{DS}','mov ds,ax',
         'xor dx,dx','mov cx,0x8000','call read_exact','mov dx,0x8000','mov cx,0x8000','call read_exact',
         'cli','cld','mov sp,0xf000','push cs','pop es',f'mov di,{low}',f'mov cx,{window}','mov al,0xa5','rep stosb',
         'push word ptr cs:[spec+6]','push word ptr cs:[spec+4]','push word ptr cs:[spec]',
         'mov ax,0x1234','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',
         'popf','call dword ptr cs:[target]','returned:']
    for j,reg in enumerate('ax bx cx dx si di bp sp cs'.split()):asm.append(f'mov word ptr cs:[output+{2*j}],{reg}')
    asm += ['mov word ptr cs:[output+18],offset returned','mov word ptr cs:[output+20],ds','mov word ptr cs:[output+22],es','mov word ptr cs:[output+24],ss']
    for j in range(0,window,2):asm += [f'mov ax,word ptr ss:[{low+j}]',f'mov word ptr cs:[output+{28+j}],ax']
    asm += ['pushf','pop ax','mov word ptr cs:[output+26],ax','cld','sti','push cs','pop ds',
            'mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{28+window}','call write_exact',
            f'mov ax,{DS}','mov ds,ax','xor dx,dx','mov cx,0x8000','call write_exact','mov dx,0x8000','mov cx,0x8000','call write_exact',
            'push cs','pop ds','inc word ptr [index]',f'cmp word ptr [index],{len(cases)}','jb again',
            'mov bx,word ptr [handle]','mov ah,0x3e','int 0x21','mov bx,word ptr [inputhandle]','mov ah,0x3e','int 0x21','mov ax,0x4c00','int 0x21',
            'read_exact:','mov ah,0x3f','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
            'write_exact:','mov ah,0x40','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
            'failure:','mov ax,0x4c01','int 0x21',
            'inputname: .asciz "INPUT.BIN"','outputname: .asciz "RESULT.BIN"','header: .space 6','spec: .space 8',
            'inputhandle: .word 0','handle: .word 0','index: .word 0',f'target: .word 0,{CS}',f'output: .space {28+window}',
            'body:',f'.incbin "{blob}"']
    path=directory/'oracle.S';obj=directory/'oracle.o';path.write_text('\n'.join(asm)+'\n')
    subprocess.run(['as','--32',str(path),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(directory/'ORACLE.COM')],check=True)
    config=directory/'dosbox.conf';config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    with (directory/'dosbox.log').open('w') as log:subprocess.run([dosbox_x['path'],'-conf',str(config),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
    result=output.read_bytes();check(len(result)==6+len(cases)*record_size,f'native length {len(result)}')
    segment,returned,got=struct.unpack_from('<3H',result);check(got==len(cases),'native count')
    for n,(m,entry,flags,off,seg) in enumerate(fixtures):
        p=physical(segment,low);m[p:p+window]=bytes([0xA5])*window;sp=0xF000-8
        for j,v in enumerate((returned,segment,off,seg)):put16(m,segment,sp+2*j,v)
        c=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,CS,entry,DS,segment,segment,flags)
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);check(function(ram,SIZE,ctypes.byref(c),512)==0,'native C status')
        record=result[6+n*record_size:6+(n+1)*record_size]
        for (name,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(c,name)==want,f'native {n}/{name}: {getattr(c,name):04x}/{want:04x}')
        check(bytes(ram[p:p+window])==record[28:28+window],f'native stack {n}')
        check(bytes(ram[physical(DS,0):physical(DS,0)+65536])==record[28+window:],f'native DS {n}')
    return dict(dosbox_x=dosbox_x,cases=len(cases),cases_per_identity=len(cases)//4,cpu_words=14,stack_bytes=window,ds_bytes=65536,flag_masks=0,result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
