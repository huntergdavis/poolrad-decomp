"""U6C +3291 DOSBox-X normal/386 original-chain oracle; media stays private.
Adapted from u3ce_bitmap_pair_release_014e_native.py; executes the original caller, +0000, U3CE +014E,
resident +2595 and SYSTEM FreeMem bodies. The overlay-manager stub 03E9:002F is synthetic: the harness
stages the loaded `EA` jump to +014E at a free segment, as the C model's contract requires.
Captures return or the FreeMem RunError entry: CPU14, SS 1 KiB, DS 64 KiB, heap 64 KiB.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess
from u6c_record_unlink_3291_audit import (Cpu,ROOT,SIZE,ENTRY,physical,put16,check,build,cases as original_cases,Limits,Diagnostic,
    CS,U3CE,STUB,RESIDENT,DS,SYSTEM,FAR_SITES,CALLEE,OK,ua,ub,r as res)
HEAP=0x3000
STACK_TOP,LOW,WINDOW=0xF000,0xEC00,0x400
OUTPUT,HEADER=0xC000,0xD800
RECORD=30+WINDOW+0x10000+0x10000
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
REGS=dict(ax=0x1234,bx=0x3456,cx=0x5678,dx=0x789A,si=0x9ABC,di=0xBCDE,bp=0x7777)
DEFAULTS=dict(walk_cap=64,list_cap=64,heap_cap=64)

def eligible(label,o):
    """Ordinary-RAM cases with default frame, segments and caps: stops are return or the FreeMem RunError trap."""
    if o.get('writes') or o.get('systems') or o.get('stub') or o.get('background') or o.get('ds') is not None:return False
    if o.get('system',SYSTEM)!=SYSTEM or o.get('ss',ua.r.SS)!=ua.r.SS or o.get('sp',0x8000)!=0x8000:return False
    if any(o.get(k,v)!=v for k,v in DEFAULTS.items()) or o.get('window',(0x3A00,0))!=(0x3A00,0):return False
    return 'cycle' not in label and 'limit' not in label and not label.startswith(('push-ds','heap-stack','sp-','ss-','a20','ds-','stub-','bad-'))
def confined(ctx,case,span,function,segment=0x0814):
    """A case is native-comparable only if it depends on nothing outside the captured windows (stack, DS, heap) and
    the staged code: the C model runs it on two RAM images whose other bytes differ (A5 vs 5A); any read of those
    bytes changes the result, and any write outside is visible. Real DOS memory elsewhere is not reproducible."""
    m,cpu,_,_,_=fixture(ctx,case,span,segment,0)
    keep=[(physical(segment,LOW),physical(segment,LOW)+WINDOW),(physical(DS,0),physical(DS,0)+0x10000),(physical(HEAP,0),physical(HEAP,0)+0x10000),
          (physical(CS,0),physical(CS,0x3400)),(physical(U3CE,0),physical(U3CE,0x200)),(physical(RESIDENT,0x2500),physical(RESIDENT,0x2700)),
          (physical(SYSTEM,0),physical(SYSTEM,0x500)),(physical(STUB,0),physical(STUB,0x40))]
    keep.sort();alt=bytearray(m);at=0;outside=[]
    for lo,hi in keep:
        if lo>at:outside.append((at,lo));alt[at:lo]=bytes([0x5A])*(lo-at)
        at=max(at,hi)
    outside.append((at,SIZE));alt[at:SIZE]=bytes([0x5A])*(SIZE-at)
    runs=[]
    for image in (m,alt):
        c=Cpu(*[getattr(cpu,n) for n,_ in Cpu._fields_]);ram=(ctypes.c_uint8*SIZE).from_buffer_copy(image)
        d=Diagnostic();limits=Limits(64,64,64)
        status=function(ram,SIZE,ctypes.byref(c),RESIDENT,SYSTEM,ctypes.byref(limits),ctypes.byref(d))
        final=bytes(ram)
        if any(final[lo:hi]!=bytes(image[lo:hi]) for lo,hi in outside):return False
        runs.append((status,tuple(getattr(c,n) for n,_ in Cpu._fields_),d.pair.resident.freemem_status,d.record.freemem_status,d.callee,
                     b''.join(final[lo:hi] for lo,hi in keep)))
    if runs[0]!=runs[1]:return False
    status,_,pin,rin,callee,_=runs[0]
    return status==OK or (status==CALLEE and ((callee==1 and rin==2) or (callee==0 and pin==2)))
def cases(ctx=None,function=None):
    picked=[(l,dict(o)) for l,o in original_cases() if eligible(l,o)]
    if ctx is not None:
        span=ctx['image'][physical(SYSTEM,0):physical(SYSTEM,0x500)]
        import time;t0=time.time();kept=[]
        for n,(l,o) in enumerate(picked):
            if confined(ctx,dict(o,flags=0x602),span,function):kept.append((l,o))
            if n%100==0:print(f'confinement pre-pass {n}/{len(picked)} kept {len(kept)} {time.time()-t0:.0f}s',flush=True)
        picked=kept
    named=[x for x in picked if not x[0].startswith(('random-','res-','rel-'))]
    other=[x for x in picked if x[0].startswith(('random-','res-','rel-'))]
    chosen=named+other[::max(1,len(other)//90)]
    return [(l,dict(o,flags=FLAGS[n%len(FLAGS)])) for n,(l,o) in enumerate(chosen)]

def fixture(ctx,case,span,segment=0x0814,returned=0):
    case=dict(case)
    for k in (*DEFAULTS,'sp','ss','flags_unused'):case.pop(k,None)
    flags=case.pop('flags')
    m,cpu=build(ctx,random.Random(0x3291),ss=segment,sp=STACK_TOP-8,flags=flags,nil_tables=True,**case)
    at=physical(SYSTEM,0);m[at:at+len(span)]=span
    at=physical(segment,LOW);m[at:at+WINDOW]=bytes([0xA5])*WINDOW
    b6=(case.get('b6hi',0)<<8)|case.get('b6',1);b8=(case.get('b8hi',0)<<8)|case.get('b8',1)
    for j,v in enumerate((returned,segment,b6,b8)):put16(m,segment,STACK_TOP-8+2*j,v)
    cpu=Cpu(*REGS.values(),STACK_TOP-8,CS,ENTRY,DS,segment,segment,flags)
    return m,cpu,flags,b6,b8

def native_audit(ctx,function):
    body=bytearray(ctx['body']);pair_body=bytearray(ctx['pair_body']);body0=bytearray(ctx['body0'])
    for site in FAR_SITES:struct.pack_into('<H',body,site+3-ENTRY,STUB)
    for site in ua.SITES:struct.pack_into('<H',pair_body,site+3-ua.ENTRY,RESIDENT)
    for site in ub.SITES:struct.pack_into('<H',body0,site+3,SYSTEM)
    body,pair_body,body0=bytes(body),bytes(pair_body),bytes(body0);resident=ctx['resident'];image=ctx['image']
    stub=bytes([0xEA])+struct.pack('<HH',ua.ENTRY,U3CE)
    directory=ROOT/'resources/oracle-runs/u6c-3291-synthetic';directory.mkdir(parents=True,exist_ok=True)
    span=image[physical(SYSTEM,0):physical(SYSTEM,0x500)]
    specs=cases(ctx,function);inputs=bytearray()
    for _,case in specs:
        m,_,flags,b6,b8=fixture(ctx,case,span)
        inputs+=struct.pack('<3H',flags,b6,b8)
        for seg,off,length in ((DS,0,0x10000),(HEAP,0,0x10000)):
            at=physical(seg,off);inputs+=m[at:at+length]
    (directory/'INPUT.BIN').write_bytes(inputs)
    blobs=(('span',SYSTEM,0,span),('resident',RESIDENT,res.ENTRY,resident),('pair',U3CE,ua.ENTRY,pair_body),
           ('body0',CS,0,body0),('body',CS,ENTRY,body),('stub',STUB,0x2F,stub))
    t=['.code16','.intel_syntax noprefix','.global _start','_start:','    jmp main','.org 0x200','main:',
       '    mov ax,cs','    mov ds,ax','    mov es,ax','    mov ss,ax',f'    mov sp,{STACK_TOP:#x}','    cld']
    for label,seg,off,blob in blobs:
        t += [f'    mov ax,{seg:#x}','    mov es,ax',f'    mov di,{off:#x}',f'    mov si,offset {label}',f'    mov cx,{len(blob)}','    rep movsb']
    t += [f'    mov ax,{SYSTEM:#x}','    mov es,ax','    mov byte ptr es:[0xe2],0xea','    mov word ptr es:[0xe3],offset trap','    mov word ptr es:[0xe5],cs',
          '    mov dx,offset input_name','    mov ax,0x3d00','    int 0x21','    jc failure','    mov word ptr [input_handle],ax',
          '    mov dx,offset output_name','    xor cx,cx','    mov ah,0x3c','    int 0x21','    jc failure','    mov word ptr [output_handle],ax',
          f'    mov word ptr [{HEADER:#x}],cs',f'    mov word ptr [{HEADER+2:#x}],offset returned',f'    mov word ptr [{HEADER+4:#x}],{len(specs)}',f'    mov word ptr [{HEADER+6:#x}],0',
          '    mov bx,ax',f'    mov dx,{HEADER:#x}','    mov cx,8','    mov ah,0x40','    int 0x21','    jc failure','again:',
          '    push cs','    pop ds',f'    mov sp,{STACK_TOP:#x}','    mov dx,offset input_flags','    mov cx,6','    call read_input']
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x8000),(HEAP,0x8000,0x8000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call read_input']
    t += ['    cli','    cld','    push cs','    pop ds','    push cs','    pop es','    mov al,0xa5',f'    mov di,{LOW:#x}',f'    mov cx,{WINDOW}','    rep stosb',
          '    push word ptr [input_b8]','    push word ptr [input_b6]','    push word ptr [input_flags]',f'    mov ax,{DS:#x}','    mov ds,ax']
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
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x8000),(HEAP,0x8000,0x8000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call write_output']
    t += ['    push cs','    pop ds','    inc word ptr [index]',f'    cmp word ptr [index],{len(specs)}','    jb again',
          '    mov bx,word ptr [output_handle]','    mov ah,0x3e','    int 0x21','    mov ax,0x4c00','    int 0x21',
          'read_input:','    mov bx,word ptr cs:[input_handle]','    mov ah,0x3f','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'write_output:','    mov bx,word ptr cs:[output_handle]','    mov ah,0x40','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'failure:','    mov ax,0x4c01','    int 0x21','input_name: .asciz "INPUT.BIN"','output_name: .asciz "RESULT.BIN"',
          'input_handle: .word 0','output_handle: .word 0','input_flags: .word 0','input_b6: .word 0','input_b8: .word 0','status: .word 0','index: .word 0',
          f'target: .word {ENTRY:#x},{CS:#x}']
    at=0x6000 # Leave absolute SYSTEM FreeMem destination windows clear.
    for label,_,_,blob in blobs:
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
        subprocess.run(['dosbox-x','-conf',str(config),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=900)
    result=output.read_bytes();check(len(result)==8+len(specs)*RECORD,f'incomplete native output {len(result)}')
    segment,returned,count,_=struct.unpack_from('<4H',result);check(segment==0x0814 and count==len(specs),'native header differs')
    stops={}
    for j,(label,case) in enumerate(specs):
        record=result[8+j*RECORD:8+(j+1)*RECORD];m,cpu,_,_,_=fixture(ctx,case,span,segment,returned)
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);diagnostic=Diagnostic();limits=Limits(64,64,64)
        status=function(ram,SIZE,ctypes.byref(cpu),RESIDENT,SYSTEM,ctypes.byref(limits),ctypes.byref(diagnostic))
        check(status in (OK,CALLEE),f'native C stop {label}: {status}')
        check(struct.unpack_from('<H',record,28)[0]==int(status==CALLEE),f'native status {label}')
        for (name,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(cpu,name)==want,f'native {label} {name}: C={getattr(cpu,name):04x}, DOS={want:04x}')
        actual=b''.join(bytes(ram[physical(seg,off):physical(seg,off)+length]) for seg,off,length in ((segment,LOW,WINDOW),(DS,0,0x10000),(HEAP,0,0x10000)))
        if actual!=record[30:]:
            diff=[i for i,(a,b) in enumerate(zip(actual,record[30:])) if a!=b]
            raise ValueError(f'native {label} RAM mismatch {[(hex(i),actual[i],record[30+i]) for i in diff[:8]]}; {len(diff)} bytes')
        name='return' if status==OK else 'RunError204';stops[name]=stops.get(name,0)+1
    report=dict(cases=len(specs),stops=stops,cpu_words=14,masked_bits=0,stack_bytes=WINDOW,ds_bytes=65536,heap_bytes=65536,result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}',stub='synthetic loaded EA jump 03E9:002F -> U3CE +014E')
    print('Native parity:',report,flush=True);return report
