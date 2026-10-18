"""DOSBox-X normal/386 original-chain oracle; generated media stays private.
Captures return or GetMem RunError entry, CPU14 + SS1KiB + DS64KiB + heap16KiB.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess
from u1c_gosub_0111_cpu_audit import Cpu,ROOT,SIZE,ENTRY,END,SITES,Limits,Diagnostic,physical,put16,check
from overlay_inventory import find_units,parse_mz
from u1c_goto_00f4_cpu_audit import stream
CS,U25,STUB,SYSTEM,DS,HEAP,SCRIPT=0x8800,0x7800,0x7F00,0x9000,0x5000,0x6000,0x2C00
STACK_TOP,LOW,WINDOW=0xF000,0xEC00,0x400
OUTPUT,HEADER=0xC000,0xD800
RECORD=30+WINDOW+0x10000+0x4000
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
REGS=dict(ax=0x1234,bx=0x3456,cx=0x5678,dx=0x789A,si=0x9ABC,di=0xBCDE,bp=0x7777)

def cases():
    rng=random.Random(0x1C0111)
    result=[]
    for n in range(72):
        tag=(0,1,2,3,0x81,0x10,0x55,rng.randrange(256),2)[n%9]
        result.append(dict(tag=tag,low=0 if tag==0x81 else rng.randrange(256),high=0x49 if tag==0x81 else rng.randrange(256),
            old=n%2,flags=FLAGS[n%len(FLAGS)],heap=n%4))
    return result

def fixture(case,caller,callee,span,segment=0x0814,returned=0):
    m=bytearray(SIZE)
    for seg,off,blob in ((CS,ENTRY,caller),(U25,0,callee),(SYSTEM,0,span)):
        at=physical(seg,off);m[at:at+len(blob)]=blob
    for off,target in set(SITES.values()):
        at=physical(STUB,off);m[at:at+5]=struct.pack('<BHH',0xEA,target,U25)
    for seg,length in ((DS,0x10000),(HEAP,0x4000)):
        at=physical(seg,0);m[at:at+length]=bytes([0xA5])*length
    top=(HEAP+0x200,0);end=(HEAP+0x300,0)
    blocks=[]
    if case['heap']==1:blocks=[(HEAP+0x10,0,0,8)]
    if case['heap']==2:blocks=[(HEAP+0x10,0,0,0),(HEAP+0x20,0,2,8)]
    if case['heap']==3:end=top
    head=blocks[0][:2] if blocks else top
    for off,value in ((0x4400,0),(0x4402,HEAP),(0x4404,top[1]),(0x4406,top[0]),(0x4408,end[1]),(0x440A,end[0]),
                      (0x440C,head[1]),(0x440E,head[0]),(0x4414,0xA9),(0x4416,SYSTEM),
                      (0x49DE,0x1000),(0x49E0,SCRIPT),(0x49D2,0x1000),(0x49D4,DS),(0x49ED,0x9900),
                      (0x6F70,0x100 if case['old'] else 0),(0x6F72,HEAP+0x380 if case['old'] else 0),(0x1000,0x51),(0x1002,0)):
        put16(m,DS,off,value)
    chain=blocks+[(top[0],top[1],0,0)]
    for block,nxt in zip(chain,chain[1:]):
        for d,v in ((0,nxt[1]),(2,nxt[0]),(4,block[3]),(6,block[2])):put16(m,block[0],block[1]+d,v)
    raw=bytes(stream([[case['tag'],case['low'],case['high']]]))
    check(len(raw)<1024,'script overflow');at=physical(SCRIPT,0x1001);m[at:at+1024]=raw+bytes([0x41])*(1024-len(raw))
    at=physical(segment,LOW);m[at:at+WINDOW]=bytes([0xA5])*WINDOW
    put16(m,segment,STACK_TOP-2,segment);put16(m,segment,STACK_TOP-4,returned)
    cpu=Cpu(*REGS.values(),STACK_TOP-4,CS,ENTRY,DS,segment,segment,case['flags'])
    return m,cpu

def native_audit(image,code1c,code25,function):
    directory=ROOT/'resources/u1c-0111/native';directory.mkdir(parents=True,exist_ok=True)
    caller=bytearray(code1c[ENTRY:END]);callee=bytearray(code25)
    for site in SITES:struct.pack_into('<H',caller,site+3-ENTRY,STUB)
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x25)
    for fixup in unit['fixups']:
        if struct.unpack_from('<H',callee,fixup)[0]==0x0AF8:struct.pack_into('<H',callee,fixup,SYSTEM)
    span=image[physical(0x0AF8,0):physical(0x0AF8,0x2000)]
    specs=cases();inputs=bytearray()
    for case in specs:
        m,_=fixture(case,caller,callee,span)
        inputs+=struct.pack('<H',case['flags'])
        for seg,off,length in ((DS,0,0x10000),(HEAP,0,0x4000),(SCRIPT,0x1001,1024)):
            at=physical(seg,off);inputs+=m[at:at+length]
    (directory/'INPUT.BIN').write_bytes(inputs)
    t=['.code16','.intel_syntax noprefix','.global _start','_start:','    jmp main','.org 0x200','main:',
       '    mov ax,cs','    mov ds,ax','    mov es,ax','    mov ss,ax',f'    mov sp,{STACK_TOP:#x}','    cld']
    for label,seg,off,blob in (('caller',CS,ENTRY,caller),('callee',U25,0,callee),('span',SYSTEM,0,span)):
        t += [f'    mov ax,{seg:#x}','    mov es,ax',f'    mov di,{off:#x}',f'    mov si,offset {label}',f'    mov cx,{len(blob)}','    rep movsb']
    t += [f'    mov ax,{STUB:#x}','    mov es,ax']
    for off,target in set(SITES.values()):
        t += [f'    mov byte ptr es:[{off:#x}],0xea',f'    mov word ptr es:[{off+1:#x}],{target:#x}',f'    mov word ptr es:[{off+3:#x}],{U25:#x}']
    t += [f'    mov ax,{SYSTEM:#x}','    mov es,ax','    mov byte ptr es:[0xe2],0xea','    mov word ptr es:[0xe3],offset trap','    mov word ptr es:[0xe5],cs',
          '    mov dx,offset input_name','    mov ax,0x3d00','    int 0x21','    jc failure','    mov word ptr [input_handle],ax',
          '    mov dx,offset output_name','    xor cx,cx','    mov ah,0x3c','    int 0x21','    jc failure','    mov word ptr [output_handle],ax',
          f'    mov word ptr [{HEADER:#x}],cs',f'    mov word ptr [{HEADER+2:#x}],offset returned',f'    mov word ptr [{HEADER+4:#x}],{len(specs)}',f'    mov word ptr [{HEADER+6:#x}],0',
          '    mov bx,ax',f'    mov dx,{HEADER:#x}','    mov cx,8','    mov ah,0x40','    int 0x21','    jc failure','again:',
          '    push cs','    pop ds',f'    mov sp,{STACK_TOP:#x}','    mov dx,offset input_flags','    mov cx,2','    call read_input']
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x4000),(SCRIPT,0x1001,1024)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call read_input']
    t += ['    cli','    cld','    push cs','    pop ds','    push cs','    pop es','    mov al,0xa5',f'    mov di,{LOW:#x}',f'    mov cx,{WINDOW}','    rep stosb',
          '    push word ptr [input_flags]',f'    mov ax,{DS:#x}','    mov ds,ax']
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
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x4000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call write_output']
    t += ['    push cs','    pop ds','    inc word ptr [index]',f'    cmp word ptr [index],{len(specs)}','    jb again',
          '    mov bx,word ptr [output_handle]','    mov ah,0x3e','    int 0x21','    mov ax,0x4c00','    int 0x21',
          'read_input:','    mov bx,word ptr cs:[input_handle]','    mov ah,0x3f','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'write_output:','    mov bx,word ptr cs:[output_handle]','    mov ah,0x40','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'failure:','    mov ax,0x4c01','    int 0x21','input_name: .asciz "INPUT.BIN"','output_name: .asciz "RESULT.BIN"',
          'input_handle: .word 0','output_handle: .word 0','input_flags: .word 0','status: .word 0','index: .word 0',f'target: .word {ENTRY:#x},{CS:#x}']
    at=0x1000
    for label,blob in (('caller',caller),('callee',callee),('span',span)):
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
        record=result[8+j*RECORD:8+(j+1)*RECORD];m,cpu=fixture(case,caller,callee,span,segment,returned)
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);limits=function.argtypes[3]._type_(SYSTEM,300,4000,64);d=function.argtypes[4]._type_()
        status=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(limits),ctypes.byref(d))
        check(status==0 or (status==3 and d.callee==1 and d.status==3 and d.nested_status==2),f'native C stop {status}/{d.status}')
        check(struct.unpack_from('<H',record,28)[0]==int(status==3),f'native status case {j}')
        for (name,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(cpu,name)==want,f'native case {j} {name}: C={getattr(cpu,name):04x}, DOS={want:04x}')
        actual=b''.join(bytes(ram[physical(seg,off):physical(seg,off)+length]) for seg,off,length in ((segment,LOW,WINDOW),(DS,0,0x10000),(HEAP,0,0x4000)))
        if actual!=record[30:]:
            diff=[i for i,(a,b) in enumerate(zip(actual,record[30:])) if a!=b]
            raise ValueError(f'native case {j} RAM mismatch {[(hex(i),actual[i],record[30+i]) for i in diff[:8]]}; {len(diff)} bytes')
        name='return' if status==0 else 'RunError203';stops[name]=stops.get(name,0)+1
    report=dict(cases=len(specs),stops=stops,cpu_words=14,stack_bytes=WINDOW,ds_bytes=65536,heap_bytes=16384,result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
    print('Native parity:',report,flush=True);return report
