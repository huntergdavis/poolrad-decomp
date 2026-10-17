#!/usr/bin/env python3
"""Native U33 +1403 proof of separately wrapped RETF CS pops at FFFE/FFFF."""
from hashlib import sha256
import os
from pathlib import Path
import struct
import subprocess
import tempfile
from party_reorder_oracle import check,build_c,run_c
from u5e_party_value_2879_audit import Cpu,physical,put16
from u33_both_outside_1403_audit import ROOT,source
RAM_SIZE=1 << 20
TARGET={"symbol":"pr_u33_both_outside_1403","source":"overlay_u33_both_outside_1403.c"}

def wrap_assembly(source):
    text=r'''.code16
.intel_syntax noprefix
.global _start
_start:
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0xf000
    mov word ptr [header],ax
    mov word ptr [return_segment],ax
    mov word ptr [header+2],offset returned
    mov word ptr [header+4],offset leaf
    mov bx,0x1000
    mov ah,0x4a
    int 0x21
    jc failure
    mov bx,0x1100
    mov ah,0x48
    int 0x21
    jc failure
    mov word ptr [header+6],ax
    mov word ptr [stack_segment],ax
    mov es,ax
    mov si,offset returned
    mov di,si
    mov cx,returned_end-returned
    cld
    rep movsb
    mov dx,offset filename
    xor cx,cx
    mov ah,0x3c
    int 0x21
    jc failure
    mov word ptr [handle],ax
    mov bx,ax
    mov dx,offset header
    mov cx,8
    mov ah,0x40
    int 0x21
again:
    cli
    mov ax,word ptr [stack_segment]
    mov es,ax
    mov di,0xffe0
    mov ax,0xa5a5
    mov cx,16
    cld
    rep stosw
    xor di,di
    mov cx,16
    rep stosw
    mov ax,word ptr [stack_segment]
    add ax,0x1000
    mov es,ax
    xor di,di
    mov ax,0xa5a5
    mov cx,16
    rep stosw
    mov ax,cs
    mov es,ax
    mov di,0x9100
    mov ax,0xa5a5
    mov cx,8
    rep stosw
    mov word ptr [0x9102],0x8000
    mov ax,word ptr [index]
    add ax,0xfffe
    mov word ptr [case_sp],ax
    mov di,ax
    mov ax,word ptr [stack_segment]
    mov es,ax
    mov word ptr es:[di],offset returned
    add di,2
    mov ax,cs
    mov word ptr es:[di],ax
    add di,2
    mov ax,word ptr [index]
    neg ax
    and ax,6
    dec ax
    mov word ptr es:[di],ax
    add di,2
    mov ax,word ptr [index]
    neg ax
    and ax,11
    dec ax
    mov word ptr es:[di],ax
    add di,2
    mov ax,cs
    mov word ptr es:[di],ax
    mov ax,word ptr [stack_segment]
    add ax,0x1000
    mov es,ax
    mov di,word ptr [case_sp]
    add di,2
    mov ax,word ptr [stack_segment]
    mov word ptr es:[di],ax
    mov ax,cs
    mov es,ax
    push 0x0002
    popf
    mov ax,word ptr [stack_segment]
    mov ss,ax
    mov sp,word ptr [case_sp]
    mov ax,0x1234
    mov bx,0x3456
    mov cx,0x5678
    mov dx,0x789a
    mov si,0x9abc
    mov di,0xbcde
    mov bp,0x7777
    jmp leaf
returned:
    mov word ptr [output+16],cs
    .byte 0xea
    .word capture
return_segment: .word 0
returned_end:
capture:
'''
    for i,(name,_) in enumerate(Cpu._fields_[:-1]):
        if name=="cs":continue
        text+=f"    mov word ptr [output+{2*i}],"+("offset returned" if name=="ip" else name)+"\n"
    text+="    mov sp,0xf000\n    pushf\n    pop ax\n    mov word ptr [output+26],ax\n"
    text+="    mov ax,word ptr [stack_segment]\n    mov es,ax\n"
    for i in range(16):
        text+=f"    mov ax,word ptr es:[{0xFFE0+2*i}]\n    mov word ptr [output+{28+2*i}],ax\n"
    for i in range(16):
        text+=f"    mov ax,word ptr es:[{2*i}]\n    mov word ptr [output+{60+2*i}],ax\n"
    text+="    mov ax,word ptr [stack_segment]\n    add ax,0x1000\n    mov es,ax\n"
    for i in range(16):
        text+=f"    mov ax,word ptr es:[{2*i}]\n    mov word ptr [output+{92+2*i}],ax\n"
    for i in range(8):
        text+=f"    mov ax,word ptr [{0x9100+2*i}]\n    mov word ptr [output+{124+2*i}],ax\n"
    text+=r'''
    mov ax,word ptr [case_sp]
    mov word ptr [output+140],ax
    mov ax,cs
    mov ss,ax
    mov sp,0xf000
    sti
    mov bx,word ptr [handle]
    mov dx,offset output
    mov cx,142
    mov ah,0x40
    int 0x21
    jc failure
    cmp ax,142
    jne failure
    inc word ptr [index]
    cmp word ptr [index],2
    jb again
    mov bx,word ptr [handle]
    mov ah,0x3e
    int 0x21
    mov ax,0x4c00
    int 0x21
failure:
    mov ax,0x4c01
    int 0x21
filename: .asciz "RESULT.BIN"
header: .space 8
handle: .word 0
index: .word 0
case_sp: .word 0
stack_segment: .word 0
output: .space 142
leaf:
'''
    return text+'.incbin "'+str(source.resolve())+'"\n'

def native_wrap_audit(root,body,function):
    directory=root/"resources/oracle-runs/u33-both-outside-1403-wrap"
    directory.mkdir(parents=True,exist_ok=True)
    source=directory/"leaf.bin";source.write_bytes(body)
    asm,obj,com=directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    asm.write_text(wrap_assembly(source))
    subprocess.run(["as","--32",str(asm),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    config=directory/"dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c "+str(directory.resolve())+"\nc:\nORACLE.COM\nexit\n")
    result_path=directory/"RESULT.BIN"
    if result_path.exists():result_path.unlink()
    with (directory/"dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
                       env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy"),
                       stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    check(0x100+com.stat().st_size<0x2600,"wrap harness overlaps record")
    result=result_path.read_bytes();check(len(result)==8+2*142,"incomplete native wrap proof")
    segment,return_ip,leaf_ip,stack=struct.unpack_from("<4H",result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)]==body,"wrap copied leaf differs")
    check(stack>=segment+0x1000 and stack+0x1100<=0xA000,"wrap allocation/disjointness changed")
    for index,sp in enumerate((0xFFFE,0xFFFF)):
        record=result[8+index*142:8+(index+1)*142]
        check(struct.unpack_from("<H",record,140)[0]==sp,"wrap metadata")
        cs=struct.unpack_from("<H",record,16)[0]
        wrapped=struct.unpack_from("<H",record,60+((sp+2)&65535))[0]
        linear=struct.unpack_from("<H",record,92+((sp+2)&65535))[0]
        check(cs==wrapped==segment and linear==stack and wrapped!=linear,
              "native RETF did not select separately wrapped CS word")
        memory=bytearray([0xA5])*RAM_SIZE
        put16(memory,segment,0x9102,0x8000)
        a,b=(0xFFFF,0xFFFF) if index==0 else (5,10)
        for displacement,word in [(0,return_ip),(2,segment),(4,a),(6,b),(8,segment)]:
            put16(memory,stack,(sp+displacement)&65535,word)
        put16(memory,stack+0x1000,(sp+2)&65535,stack)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,sp,segment,leaf_ip,segment,segment,stack,0x0002)
        actual,cpu=run_c(function,memory,cpu)
        check(cpu.ax==0x1201-index and cpu.sp==(sp+8)&65535 and
              cpu.flags==(0x0086 if index==0 else 0x0097),
              "native wrap predicate/return expectation differs")
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native wrap {name} mismatch SP={sp:04x}:C={getattr(cpu,name):04x},DOS={expected:04x}")
        for seg,offset,size,position in [(stack,0xFFE0,32,28),(stack,0,32,60),
                                        (stack+0x1000,0,32,92),(segment,0x9100,16,124)]:
            at=physical(seg,offset)
            check(actual[at:at+size]==record[position:position+size],
                  f"native wrap RAM mismatch SP={sp:04x}/{seg:04x}:{offset:04x}")
    print("2 DOSBox-X SP=FFFE/FFFF cases passed:all14 CPU words,96 stack/sentinel bytes,"
          "16 auxiliary sentinel bytes; true/false predicates; distinct linear-CS decoy rejected; no masks")
    print("native wrap result SHA-256 "+sha256(result).hexdigest())

def main():
    body,_,_=source()
    with tempfile.TemporaryDirectory(prefix="poolrad-u33-wrap-") as temp:
        function=build_c(ROOT,Path(temp),TARGET)
        native_wrap_audit(ROOT,body,function)

if __name__=="__main__":main()
