"""DOSBox-X normal/386 original-chain oracle; generated media stays private.
Captures normal caller return: CPU14, SS1KiB, DS64KiB, heap64KiB.
"""
import ctypes
from hashlib import sha256
import os
import random
import struct
import subprocess
from u38_target_reset_2b7d_cpu_audit import Cpu,ROOT,SIZE,ENTRIES,physical,put16,check,build
CS,RES=0x5000,0x7000
DS,HEAP=0x1800,0x3000
STACK_TOP,LOW,WINDOW=0xF000,0xEC00,0x400
OUTPUT,HEADER=0xC000,0xD800
RECORD=30+WINDOW+0x10000+0x10000
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
REGS=dict(ax=0x1234,bx=0x3456,cx=0x5678,dx=0x789A,si=0x9ABC,di=0xBCDE,bp=0x7777)

MODES=('resident','select','reset')
FRAMES={'resident':12,'select':8,'reset':14}
def cases():
    out=[dict(mode='reset',link=(0,0),flags=2),dict(mode='reset',link=(0,0),kind=1,flags=2),dict(mode='reset',first=(0,0),second=(0,0),flags=0xD7)]
    for n in range(125):
        out.append(dict(mode='reset',bits=(0,1,8,9,10,16,17,24,25,128,129,136,137,144,145,152,153,255)[n%18],kind=(0,1,127,128,255)[n%5],value=(0,3,4,127,128,255)[(n//5)%6],byte32=(0,1,128,255)[(n//30)%4],amount=(n*3)&255,flags=FLAGS[n%6]))
    return out

def fixture(case,resident,blob,segment=0x0814,returned=0):
    mode=case['mode'];sp=STACK_TOP-FRAMES[mode]
    m,cpu=build(resident,blob,random.Random(0x2B7D),ss=segment,sp=sp,res=RES,**case)
    at=physical(segment,LOW);m[at:at+WINDOW]=bytes([0xA5])*WINDOW
    args=(0x6000,DS,0x1000,HEAP) if mode=='resident' else (0x1000,HEAP) if mode=='select' else (0xA100,0xA101,0xA102,0xA103,0xA104)
    for j,v in enumerate((returned,segment,*args)):put16(m,segment,sp+2*j,v)
    cpu=Cpu(*REGS.values(),sp,RES if mode=='resident' else CS,ENTRIES[mode],DS,segment,segment,case['flags'])
    return m,cpu

def native_audit(resident,blob,functions):
    directory=ROOT/'resources/u38-2b7d/native';directory.mkdir(parents=True,exist_ok=True)
    # Bind the original far call to loaded code outside DOS low memory.
    blob=bytearray(blob);struct.pack_into('<H',blob,0x1048,RES);blob=bytes(blob)
    specs=cases();inputs=bytearray()
    for case in specs:
        m,_=fixture(case,resident,blob)
        inputs+=struct.pack('<2H',case['flags'],MODES.index(case['mode']))
        for seg,off,length in ((DS,0,0x10000),(HEAP,0,0x10000)):
            at=physical(seg,off);inputs+=m[at:at+length]
    (directory/'INPUT.BIN').write_bytes(inputs)
    t=['.code16','.intel_syntax noprefix','.global _start','_start:','    jmp main','.org 0x200','main:',
       '    mov ax,cs','    mov ds,ax','    mov es,ax','    mov ss,ax',f'    mov sp,{STACK_TOP:#x}','    cld']
    for label,seg,off,blob in (('resident',RES,0,resident),('caller',CS,0,blob)):
        t += [f'    mov ax,{seg:#x}','    mov es,ax',f'    mov di,{off:#x}',f'    mov si,offset {label}',f'    mov cx,{len(blob)}','    rep movsb']
    t += ['    mov dx,offset input_name','    mov ax,0x3d00','    int 0x21','    jc failure','    mov word ptr [input_handle],ax',
          '    mov dx,offset output_name','    xor cx,cx','    mov ah,0x3c','    int 0x21','    jc failure','    mov word ptr [output_handle],ax',
          f'    mov word ptr [{HEADER:#x}],cs',f'    mov word ptr [{HEADER+2:#x}],offset returned',f'    mov word ptr [{HEADER+4:#x}],{len(specs)}',f'    mov word ptr [{HEADER+6:#x}],0',
          '    mov bx,ax',f'    mov dx,{HEADER:#x}','    mov cx,8','    mov ah,0x40','    int 0x21','    jc failure','again:',
          '    push cs','    pop ds',f'    mov sp,{STACK_TOP:#x}','    mov dx,offset input_flags','    mov cx,4','    call read_input']
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x8000),(HEAP,0x8000,0x8000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call read_input']
    t += ['    cli','    cld','    push cs','    pop ds','    push cs','    pop es','    mov al,0xa5',f'    mov di,{LOW:#x}',f'    mov cx,{WINDOW}','    rep stosb',
          '    cmp word ptr [input_mode],0','    je resident_args','    cmp word ptr [input_mode],1','    je select_args','    push 0xa104','    push 0xa103','    push 0xa102','    push 0xa101','    push 0xa100',f'    mov word ptr [target],{ENTRIES["reset"]}',f'    mov word ptr [target+2],{CS}','    jmp args_done','select_args:',f'    push {HEAP}','    push 0x1000',f'    mov word ptr [target],{ENTRIES["select"]}',f'    mov word ptr [target+2],{CS}','    jmp args_done','resident_args:',f'    push {HEAP}','    push 0x1000',f'    push {DS}','    push 0x6000',f'    mov word ptr [target],{ENTRIES["resident"]}',f'    mov word ptr [target+2],{RES}','args_done:','    push word ptr [input_flags]',f'    mov ax,{DS:#x}','    mov ds,ax']
    for reg,value in REGS.items():t.append(f'    mov {reg},{value:#x}')
    t += ['    popf','    call dword ptr cs:[target]','returned:']
    for j,reg in enumerate('ax bx cx dx si di bp sp'.split()):t.append(f'    mov word ptr cs:[{OUTPUT+2*j:#x}],{reg}')
    for off,reg in ((20,'ds'),(22,'es'),(24,'ss')):t.append(f'    mov word ptr cs:[{OUTPUT+off:#x}],{reg}')
    t += [f'    mov sp,{STACK_TOP+0x800:#x}','    pushf','    pop ax',f'    mov word ptr cs:[{OUTPUT+26:#x}],ax',f'    mov word ptr cs:[{OUTPUT+16:#x}],cs',f'    mov word ptr cs:[{OUTPUT+18:#x}],offset returned',f'    mov word ptr cs:[{OUTPUT+28:#x}],0','    cld','    push cs','    pop ds','    push cs','    pop es',
          f'    mov si,{LOW:#x}',f'    mov di,{OUTPUT+30:#x}',f'    mov cx,{WINDOW}','    rep movsb','    sti',
          f'    mov dx,{OUTPUT:#x}',f'    mov cx,{30+WINDOW}','    call write_output']
    for seg,off,length in ((DS,0,0x8000),(DS,0x8000,0x8000),(HEAP,0,0x8000),(HEAP,0x8000,0x8000)):
        t += [f'    mov ax,{seg:#x}','    mov ds,ax',f'    mov dx,{off:#x}',f'    mov cx,{length:#x}','    call write_output']
    t += ['    push cs','    pop ds','    inc word ptr [index]',f'    cmp word ptr [index],{len(specs)}','    jb again',
          '    mov bx,word ptr [output_handle]','    mov ah,0x3e','    int 0x21','    mov ax,0x4c00','    int 0x21',
          'read_input:','    mov bx,word ptr cs:[input_handle]','    mov ah,0x3f','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'write_output:','    mov bx,word ptr cs:[output_handle]','    mov ah,0x40','    int 0x21','    jc failure','    cmp ax,cx','    jne failure','    ret',
          'failure:','    mov ax,0x4c01','    int 0x21','input_name: .asciz "INPUT.BIN"','output_name: .asciz "RESULT.BIN"',
          'input_handle: .word 0','output_handle: .word 0','input_flags: .word 0','input_mode: .word 0','index: .word 0','target: .word 0,0']
    at=0x4600 # Keep embedded original unit bytes clear of the harness and captures.
    for label,blob in (('resident',resident),('caller',blob)):
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
    return compare_capture(resident,blob,functions,directory,specs)

def compare_capture(resident,blob,functions,directory,specs):
    """Compare an already-produced capture with the caller's ctypes diagnostic type."""
    output=directory/'RESULT.BIN'
    result=output.read_bytes();check(len(result)==8+len(specs)*RECORD,f'incomplete native output {len(result)}')
    segment,returned,count,_=struct.unpack_from('<4H',result);check(segment==0x0814 and count==len(specs),'native header differs')
    stops={}
    for j,case in enumerate(specs):
        record=result[8+j*RECORD:8+(j+1)*RECORD];m,cpu=fixture(case,resident,blob,segment,returned)
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
        function=functions[case['mode']]
        inner=None if case['mode']=='resident' else function.argtypes[-1]._type_(-1,-1)
        status=function(ram,SIZE,ctypes.byref(cpu),*([] if inner is None else [ctypes.byref(inner)]))
        check(status==0,f'native C stop {status}')
        check(struct.unpack_from('<H',record,28)[0]==0,f'native status case {j}')
        for (name,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(cpu,name)==want,f'native case {j} {name}: C={getattr(cpu,name):04x}, DOS={want:04x}')
        actual=b''.join(bytes(ram[physical(seg,off):physical(seg,off)+length]) for seg,off,length in ((segment,LOW,WINDOW),(DS,0,0x10000),(HEAP,0,0x10000)))
        if actual!=record[30:]:
            diff=[i for i,(a,b) in enumerate(zip(actual,record[30:])) if a!=b]
            raise ValueError(f'native case {j} RAM mismatch {[(hex(i),actual[i],record[30+i]) for i in diff[:8]]}; {len(diff)} bytes')
        name=case['mode']+':return';stops[name]=stops.get(name,0)+1
    report=dict(cases=len(specs),stops=stops,cpu_words=14,stack_bytes=WINDOW,ds_bytes=65536,heap_bytes=65536,result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
    print('Native parity:',report,flush=True);return report
