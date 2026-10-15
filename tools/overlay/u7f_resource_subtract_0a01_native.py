"""Synthetic DOSBox-X full-CPU/write-footprint oracle for U7F +0A01."""
from hashlib import sha256
import os
from pathlib import Path
import struct
import subprocess
from party_reorder_oracle import check
from u5e_party_value_2879_audit import Cpu,physical,put16
from u7f_resource_subtract_0a01_audit import RAM_SIZE,ENTRY,END,run_c,signed_index

# (segment kind, offset, byte count): every native execution write lands
# inside these blocks. Code, capture buffers, and DOS bookkeeping are disjoint.
BLOCKS=[("stack",0x7F60,192),("stack",0xFF60,160),("stack",0,64),
        ("linear",0,64),("data",0x8F00,1056),("data",0x65F0,1040),("data",0x49F0,16)]
RECORD_SIZE=28+sum(n for _,_,n in BLOCKS)+20

def cases():
    rows=[]
    for index in range(256):
        for mode in (1,6):
            rows.append((0x8000,index,(index*257+mode)&65535,0x9000,0,mode,
                         (0xA55A^index*257)&65535,(0x8000-index)&65535,0xFFFF,0xFFFF))
    for mode in range(256):
        rows.append((0x8000,mode^128,(mode*257)&65535,0x9000,0,mode,
                     mode*257,0x8000,0xFFFF,0xDEAD))
    for sp in (0xFFFE,0xFFFF,10,11):
        for mode in (0,6):rows.append((sp,128,1,0x9000,0,mode,0x8000,0x8000,0xFFFF,0xDEAD))
    rows.append((0x8000,0,0xFFBD,0x7EF2,1,6,0x8000,0x8000,0xFFFF,0xDEAD))
    rows.append((0x8000,0,0,0x7EF2,1,6,0x8000,0x8000,0xFFFF,0xDEAD))
    # The two CS-overwrite fixtures compute amount from actual DOS allocation.
    rows.append((0x8000,0,0,0x7EF4,2,6,0x8000,0x8000,0xFFFF,0xDEAD))
    rows.append((0x8000,0,0,0x7EF4,3,6,0x8000,0x8000,0xFFFF,0xDEAD))
    return rows

def assembly(source):
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
    mov word ptr [inner_segment],ax
    mov word ptr [header+6],offset returned
    mov word ptr [header+8],offset image
    mov bx,0x1000
    mov ah,0x4a
    int 0x21
    jc failure
    mov bx,0x1000
    mov ah,0x48
    int 0x21
    jc failure
    mov word ptr [header+2],ax
    mov word ptr [entry_pointer+2],ax
    mov es,ax
    xor di,di
    mov si,offset image
    mov cx,image_end-image
    cld
    rep movsb
    mov bx,0x1100
    mov ah,0x48
    int 0x21
    jc failure
    mov word ptr [header+4],ax
    mov es,ax
    mov si,offset returned
    mov di,si
    mov cx,returned_end-returned
    cld
    rep movsb
    mov si,offset inner_exit
    mov di,si
    mov cx,inner_end-inner_exit
    rep movsb
    mov dx,offset input_filename
    mov ax,0x3d00
    int 0x21
    jc failure
    mov word ptr [input_handle],ax
    mov dx,offset filename
    xor cx,cx
    mov ah,0x3c
    int 0x21
    jc failure
    mov word ptr [handle],ax
    mov bx,ax
    mov dx,offset header
    mov cx,12
    mov ah,0x40
    int 0x21
again:
    mov bx,word ptr [input_handle]
    mov dx,offset fixture
    mov cx,20
    mov ah,0x3f
    int 0x21
    jc failure
    cmp ax,20
    jne failure
    cli
'''
    for kind,offset,size in BLOCKS:
        text+="    mov ax,word ptr [header"+({"data":"","stack":"+4","linear":"+4"}[kind])+"]\n"
        if kind=="linear":text+="    add ax,0x1000\n"
        text+=f"    mov es,ax\n    mov di,{offset}\n    mov ax,0xa5a5\n    mov cx,{size//2}\n    cld\n    rep stosw\n"
    text+=r'''
    mov ax,ds
    cmp word ptr [fixture+8],0
    je record_ready
    mov ax,word ptr [header+4]
record_ready:
    mov word ptr [record_segment],ax
    cmp word ptr [fixture+8],2
    jb amount_ready
    mov dx,ds
    cmp word ptr [fixture+8],3
    jne amount_cs
    mov dx,word ptr [header+4]
amount_cs:
    mov ax,word ptr [header+2]
    sub ax,dx
    mov word ptr [fixture+4],ax
amount_ready:
    mov ax,word ptr [record_segment]
    mov es,ax
    mov al,byte ptr [fixture+2]
    cbw
    shl ax,1
    mov di,word ptr [fixture+6]
    add di,ax
    add di,0x88
    mov ax,word ptr [fixture+12]
    mov word ptr es:[di],ax
    mov di,word ptr [fixture+6]
    add di,0x102
    mov ax,word ptr [fixture+14]
    mov word ptr es:[di],ax
    mov al,byte ptr [fixture+2]
    cbw
    shl ax,1
    shl ax,1
    mov di,ax
    add di,0x67f4
    mov ax,word ptr [fixture+16]
    mov word ptr [di],ax
    mov ax,word ptr [fixture+18]
    mov word ptr [di+2],ax
    mov al,byte ptr [fixture+10]
    mov byte ptr [0x49f3],al
    mov ax,word ptr [header+4]
    mov es,ax
    mov di,word ptr [fixture]
    mov word ptr es:[di],offset returned
    add di,2
    mov ax,ds
    mov word ptr es:[di],ax
    add di,2
    mov ax,word ptr [fixture+2]
    or ax,0xab00
    mov word ptr es:[di],ax
    add di,2
    mov ax,word ptr [fixture+4]
    mov word ptr es:[di],ax
    add di,2
    mov ax,word ptr [fixture+6]
    mov word ptr es:[di],ax
    add di,2
    mov ax,word ptr [record_segment]
    mov word ptr es:[di],ax
    mov di,word ptr [fixture]
    cmp di,0xfffe
    jb inner_wrap
    add di,2
    jmp decoy
inner_wrap:
    cmp di,10
    jb entered
    cmp di,11
    ja entered
    sub di,10
decoy:
    mov ax,word ptr [header+4]
    add ax,0x1000
    mov es,ax
    mov ax,word ptr [header+4]
    mov word ptr es:[di],ax
entered:
    push 0x0002
    popf
    mov ax,word ptr [header+4]
    mov ss,ax
    mov sp,word ptr [fixture]
    mov ax,0x1111
    mov es,ax
    mov ax,0x1234
    mov bx,0x3456
    mov cx,0x5678
    mov dx,0x789a
    mov si,0x9abc
    mov di,0xbcde
    mov bp,0x7777
    .byte 0xff,0x2e
    .word entry_pointer
    .org 0x0a36-0x100
inner_exit:
    mov word ptr [output+16],cs
    mov word ptr [output+18],0x0a36
    .byte 0xea
    .word capture
inner_segment: .word 0
inner_end:
    .org 0x2000-0x100
returned:
    mov word ptr [output+16],cs
    mov word ptr [output+18],0x2000
    .byte 0xea
    .word capture
return_segment: .word 0
returned_end:
capture:
'''
    for i,(name,_) in enumerate(Cpu._fields_[:-1]):
        if name not in ("cs","ip"):text+=f"    mov word ptr [output+{2*i}],{name}\n"
    text+="    mov sp,0xf000\n    pushf\n    pop ax\n    mov word ptr [output+26],ax\n"
    position=28
    for kind,offset,size in BLOCKS:
        text+="    mov ax,word ptr [header"+({"data":"","stack":"+4","linear":"+4"}[kind])+"]\n"
        if kind=="linear":text+="    add ax,0x1000\n"
        text+=f"    mov dx,ds\n    mov es,dx\n    mov ds,ax\n    mov si,{offset}\n    mov di,offset output+{position}\n    mov cx,{size//2}\n    cld\n    rep movsw\n    mov ds,dx\n"
        position+=size
    text+=f"    mov ax,ds\n    mov es,ax\n    mov si,offset fixture\n    mov di,offset output+{position}\n    mov cx,10\n    rep movsw\n"
    text+=f'''    mov ax,ds
    mov ss,ax
    mov sp,0xf000
    sti
    mov bx,word ptr [handle]
    mov dx,offset output
    mov cx,{RECORD_SIZE}
    mov ah,0x40
    int 0x21
    jc failure
    cmp ax,{RECORD_SIZE}
    jne failure
    inc word ptr [index]
    cmp word ptr [index],{len(cases())}
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
input_filename: .asciz "CASES.BIN"
header: .word 0,0,0,0,0,{len(cases())}
entry_pointer: .word 0x0a01,0
handle: .word 0
input_handle: .word 0
index: .word 0
record_segment: .word 0
fixture: .space 20
output: .space {RECORD_SIZE}
image:
.incbin "{source.resolve()}"
image_end:
'''
    return text

def native_audit(root,image,function):
    directory=root/"resources/oracle-runs/u7f-resource-subtract-0a01-native"
    directory.mkdir(parents=True,exist_ok=True)
    source=directory/"image.bin";source.write_bytes(image)
    (directory/"CASES.BIN").write_bytes(b"".join(struct.pack("<10H",*row) for row in cases()))
    asm,obj,com=directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    asm.write_text(assembly(source))
    subprocess.run(["as","--32",str(asm),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    check(0x100+com.stat().st_size<0x49F0,"native harness overlaps synthetic data")
    config=directory/"dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c "+str(directory.resolve())+"\nc:\nORACLE.COM\nexit\n")
    result_path=directory/"RESULT.BIN"
    if result_path.exists():result_path.unlink()
    with (directory/"dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
                       env=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy"),
                       stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
    result=result_path.read_bytes()
    check(len(result)==12+len(cases())*RECORD_SIZE,"incomplete native owner proof")
    data,code,stack,return_ip,image_ip,count=struct.unpack_from("<6H",result)
    check(return_ip==0x2000 and count==len(cases()),"native header changed")
    check(data+0x1000<=code and code+0x1000<=stack and stack+0x1100<=0xA000,"native allocations overlap")
    check(com.read_bytes()[image_ip-0x100:image_ip-0x100+len(image)]==image,"copied original image differs")
    shifted_af=set();statuses=set()
    for number,original_row in enumerate(cases()):
        row=list(original_row)
        if row[4] in (2,3):row[2]=(code-(data if row[4]==2 else stack))&65535
        sp,index,amount,record_offset,style,mode,value,base,low,high=row
        record=result[12+number*RECORD_SIZE:12+(number+1)*RECORD_SIZE]
        check(struct.unpack_from("<10H",record,RECORD_SIZE-20)==tuple(row),"native case metadata mismatch")
        memory=bytearray([0xA5])*RAM_SIZE;record_segment=data if style==0 else stack
        put16(memory,record_segment,(record_offset+0x88+2*signed_index(index))&65535,value)
        put16(memory,record_segment,(record_offset+0x102)&65535,base)
        acc=(0x67F4+4*signed_index(index))&65535
        put16(memory,data,acc,low);put16(memory,data,(acc+2)&65535,high)
        memory[physical(data,0x49F3)]=mode
        for displacement,word in [(0,return_ip),(2,data),(4,index|0xAB00),(6,amount),
                                  (8,record_offset),(10,record_segment)]:
            put16(memory,stack,(sp+displacement)&65535,word)
        if sp>=0xFFFE:put16(memory,stack+0x1000,(sp+2)&65535,stack)
        if sp in (10,11):put16(memory,stack+0x1000,(sp-10)&65535,stack)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,sp,code,ENTRY,data,0x1111,stack,0x0002)
        actual,cpu,status=run_c(function,memory,cpu);statuses.add(status)
        check(status==(2 if style in (2,3) else 0),f"native control transfer mismatch {number}")
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native {number}/{name}: C={getattr(cpu,name):04x},DOS={expected:04x}")
        position=28;ranges=[]
        for kind,offset,size in BLOCKS:
            segment={"data":data,"stack":stack,"linear":stack+0x1000}[kind]
            at=physical(segment,offset)
            check(actual[at:at+size]==record[position:position+size],f"native RAM mismatch {number}/{kind}:{offset:04x}")
            ranges.append((at,at+size));position+=size
        last=0
        for start,end in sorted(ranges):
            check(actual[last:start]==memory[last:start],"C wrote outside native-observed footprint")
            last=end
        check(actual[last:]==memory[last:],"C wrote outside native-observed footprint")
        if mode in (1,6) and style==0:shifted_af.add(bool(cpu.flags&0x10))
    check(shifted_af=={True} and statuses=={0,2},"native SHL/transfer evidence incomplete")
    print(f"{len(cases())} DOSBox-X cases:all14 CPU words/{sum(n for _,_,n in BLOCKS)} RAM bytes; all index/mode bytes; outer and inner RETF wraps; overwritten IP and CS; no masks")
    print("native result SHA256 "+sha256(result).hexdigest())
