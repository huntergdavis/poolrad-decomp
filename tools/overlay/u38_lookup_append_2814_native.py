"""DOSBox-X normal/386 original-chain oracle; generated media stays private.
Captures return or GetMem RunError entry: CPU14, SS1KiB, DS64KiB, heap64KiB, nodes16KiB.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess
from u38_lookup_append_2814_audit import Cpu,ROOT,SIZE,ENTRY,END,physical,put16,check,build,SITES,LOOKUPS
CS,SYSTEM,DS,HEAP,NODES,DICE,STUB=0x8800,0x0AF8,0x1800,0x3000,0x5000,0x7800,0x7F00
RES=0x7000
STACK_TOP,LOW,WINDOW=0xF000,0xEC00,0x400
OUTPUT,HEADER=0xC000,0xD800
RECORD=30+WINDOW+0x10000+0x10000+0x4000
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
REGS=dict(ax=0x1234,bx=0x3456,cx=0x5678,dx=0x789A,si=0x9ABC,di=0xBCDE,bp=0x7777)

def cases():
    result=[]
    for n in range(120):
        keys=tuple(range(1,n%5+1))
        if n%3==1:keys+=(0x62,)
        if n%3==2:keys+=(0x3B,)
        options=dict(keys=keys,flags=FLAGS[n%6])
        if n%9==1:options['blocks']=((0x3010,0,1,0),)
        if n%9==2:options['blocks']=((0x3010,8,8,8),)
        if n%9==3:options['end']=(0x3400,0)
        result.append(options)
    return result

def fixture(case,caller,callee,span,lookup,segment=0x0814,returned=0):
    m,cpu,_=build(b'',caller,callee,random.Random(0x382814),ss=segment,sp=STACK_TOP-14,**case)
    at=physical(SYSTEM,0);m[at:at+len(span)]=span
    at=physical(RES,0x2199);m[at:at+len(lookup)]=lookup
    for off,target in SITES.values():
        at=physical(STUB,off);m[at:at+5]=struct.pack('<BHH',0xEA,target,DICE)
    at=physical(segment,LOW);m[at:at+WINDOW]=bytes([0xA5])*WINDOW
    for j,v in enumerate((returned,segment,0x1111,0x2222,0x3333,0x100,NODES)):put16(m,segment,STACK_TOP-14+2*j,v)
    cpu=Cpu(*REGS.values(),STACK_TOP-14,CS,ENTRY,DS,segment,segment,case['flags'])
    return m,cpu

def native_audit(image,code38,code9e,function):
    directory=ROOT/'resources/u38-2814/native';directory.mkdir(parents=True,exist_ok=True)
    caller=bytearray(code38[:END]);callee=code9e
    for site in SITES:struct.pack_into('<H',caller,site+3,STUB)
    for site in LOOKUPS:struct.pack_into('<H',caller,site+3,RES)
    lookup=image[physical(0xBA,0x2199):physical(0xBA,0x2208)]
    span=image[physical(SYSTEM,0):physical(SYSTEM,0x1247)]
    specs=cases();inputs=bytearray()
    for case in specs:
        m,_=fixture(case,caller,callee,span,lookup)
        inputs+=struct.pack('<H',case['flags'])
        for seg,off,length in ((DS,0,0x10000),(HEAP,0,0x10000),(NODES,0,0x4000)):
            at=physical(seg,off);inputs+=m[at:at+length]
    (directory/'INPUT.BIN').write_bytes(inputs)
    t=['.code16','.intel_syntax noprefix','.global _start','_start:','    jmp main','.org 0x200','main:',
       '    mov ax,cs','    mov ds,ax','    mov es,ax','    mov ss,ax',f'    mov sp,{STACK_TOP:#x}','    cld']
    for label,seg,off,blob in (('caller',CS,0,caller),('callee',DICE,0,callee),('span',SYSTEM,0,span),('lookup',RES,0x2199,lookup)):
        t += [f'    mov ax,{seg:#x}','    mov es,ax',f'    mov di,{off:#x}',f'    mov si,offset {label}',f'    mov cx,{len(blob)}','    rep movsb']
    t += [f'    mov ax,{STUB:#x}','    mov es,ax']
    for off,target in SITES.values():
        t += [f'    mov byte ptr es:[{off:#x}],0xea',f'    mov word ptr es:[{off+1:#x}],{target:#x}',f'    mov word ptr es:[{off+3:#x}],{DICE:#x}']
    t += [f'    mov ax,{SYSTEM:#x}','    mov es,ax','    mov byte ptr es:[0xe2],0xea','    mov word ptr es:[0xe3],offset trap','    mov word ptr es:[0xe5],cs',
          '    mov dx,offset input_name','    mov ax,0x3d00','    int 0x21','    jc failure','    mov word ptr [input_handle],ax',
          '    mov dx,offset output_name','    xor cx,cx','    mov ah,0x3c','    int 0x21','    jc failure','    mov word ptr [output_handle],ax',
          f'    mov word ptr [{HEADER:#x}],cs',f'    mov word ptr [{HEADER+2:#x}],offset returned',f'    mov word ptr [{HEADER+4:#x}],{len(specs)}',f'    mov word ptr [{HEADER+6:#x}],0',
          '    mov bx,ax',f'    mov dx,{HEADER:#x}','    mov cx,8','    mov ah,0x40','    int 0x21','    jc failure','again:',
          '    push cs','    pop ds',f'    mov sp,{STACK_TOP:#x}','    mov dx,offset input_flags','    mov cx,2','    call read_input']
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x8000),(HEAP,0x8000,0x8000),(NODES,0,0x4000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call read_input']
    t += ['    cli','    cld','    push cs','    pop ds','    push cs','    pop es','    mov al,0xa5',f'    mov di,{LOW:#x}',f'    mov cx,{WINDOW}','    rep stosb',
          f'    push {NODES:#x}','    push 0x100','    push 0x3333','    push 0x2222','    push 0x1111','    push word ptr [input_flags]',f'    mov ax,{DS:#x}','    mov ds,ax']
    for reg,value in REGS.items():t.append(f'    mov {reg},{value:#x}')
    t += ['    popf','    call dword ptr cs:[target]','returned:','    mov word ptr cs:[status],0','    jmp record','trap:','    mov word ptr cs:[status],1','record:']
    for j,reg in enumerate('ax bx cx dx si di bp sp'.split()):t.append(f'    mov word ptr cs:[{OUTPUT+2*j:#x}],{reg}')
    for off,reg in ((20,'ds'),(22,'es'),(24,'ss')):t.append(f'    mov word ptr cs:[{OUTPUT+off:#x}],{reg}')
    t += [f'    mov sp,{STACK_TOP+0x800:#x}','    pushf','    pop ax',f'    mov word ptr cs:[{OUTPUT+26:#x}],ax','    cmp word ptr cs:[status],0','    jne trapped',
          f'    mov word ptr cs:[{OUTPUT+16:#x}],cs',f'    mov word ptr cs:[{OUTPUT+18:#x}],offset returned','    jmp segments','trapped:',
          f'    mov word ptr cs:[{OUTPUT+16:#x}],{SYSTEM:#x}',f'    mov word ptr cs:[{OUTPUT+18:#x}],0xe2','segments:',
          '    mov ax,word ptr cs:[status]',f'    mov word ptr cs:[{OUTPUT+28:#x}],ax','    cld','    push cs','    pop ds','    push cs','    pop es',
          f'    mov si,{LOW:#x}',f'    mov di,{OUTPUT+30:#x}',f'    mov cx,{WINDOW}','    rep movsb','    sti',
          f'    mov dx,{OUTPUT:#x}',f'    mov cx,{30+WINDOW}','    call write_output']
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x8000),(HEAP,0x8000,0x8000),(NODES,0,0x4000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call write_output']
    t += ['    push cs','    pop ds','    inc word ptr [index]',f'    cmp word ptr [index],{len(specs)}','    jb again',
          '    mov bx,word ptr [output_handle]','    mov ah,0x3e','    int 0x21','    mov ax,0x4c00','    int 0x21',
          'read_input:','    mov bx,word ptr cs:[input_handle]','    mov ah,0x3f','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'write_output:','    mov bx,word ptr cs:[output_handle]','    mov ah,0x40','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'failure:','    mov ax,0x4c01','    int 0x21','input_name: .asciz "INPUT.BIN"','output_name: .asciz "RESULT.BIN"',
          'input_handle: .word 0','output_handle: .word 0','input_flags: .word 0','status: .word 0','index: .word 0',f'target: .word {ENTRY:#x},{CS:#x}']
    at=0x4100 # Reserve COM offsets 2E40..4087 for absolute SYSTEM code.
    for label,blob in (('caller',caller),('callee',callee),('span',span),('lookup',lookup)):
        t += [f'.org {at-0x100:#x}',label+':']
        for j in range(0,len(blob),16):t.append('    .byte '+','.join(hex(x) for x in blob[j:j+16]))
        at=(at+len(blob)+15)&~15
    check(at<OUTPUT,'COM code overlaps capture')
    asm=directory/'oracle.S';obj=directory/'oracle.o';asm.write_text('\n'.join(t)+'\n')
    subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(directory/'ORACLE.COM')],check=True)
    config=directory/'dosbox.conf';config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x','-conf',str(config),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
    result=output.read_bytes();check(len(result)==8+len(specs)*RECORD,f'incomplete native output {len(result)}')
    segment,returned,count,_=struct.unpack_from('<4H',result);check(segment==0x0814 and count==len(specs),'native header differs')
    stops={}
    for j,case in enumerate(specs):
        record=result[8+j*RECORD:8+(j+1)*RECORD];m,cpu=fixture(case,caller,callee,span,lookup,segment,returned)
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=function.argtypes[4]._type_();l=function.argtypes[3]._type_(64,64,64)
        status=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(l),ctypes.byref(d))
        check(status==0 or (status==3 and d.callee==1 and d.status==3 and d.getmem_status==2),f'native C stop {status}/{d.callee}/{d.status}')
        check(struct.unpack_from('<H',record,28)[0]==int(status==3),f'native status case {j}')
        for (name,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(cpu,name)==want,f'native case {j} {name}: C={getattr(cpu,name):04x}, DOS={want:04x}')
        actual=b''.join(bytes(ram[physical(seg,off):physical(seg,off)+length]) for seg,off,length in ((segment,LOW,WINDOW),(DS,0,0x10000),(HEAP,0,0x10000),(NODES,0,0x4000)))
        if actual!=record[30:]:
            diff=[i for i,(a,b) in enumerate(zip(actual,record[30:])) if a!=b]
            raise ValueError(f'native case {j} RAM mismatch {[(hex(i),actual[i],record[30+i]) for i in diff[:8]]}; {len(diff)} bytes')
        name='return' if status==0 else 'RunError203';stops[name]=stops.get(name,0)+1
    report=dict(cases=len(specs),stops=stops,cpu_words=14,stack_bytes=WINDOW,ds_bytes=65536,heap_bytes=65536,node_bytes=16384,result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
    print('Native parity:',report,flush=True);return report
