#!/usr/bin/env python3
"""Full-state original-byte and synthetic native parity for u33 +0B97."""
import argparse
from collections import deque
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from party_reorder_oracle import check,build_c,run_c
from u5e_party_value_2879_audit import Cpu,EXE_SHA,OVR_SHA,physical,put16

RAM_SIZE=1 << 20
ENTRY,END=0x0B97,0x0BDE
DIGEST="75c464f3df7839fc6a44ab167138b0addb4a329758a775b3255c50a35108b52a"
TARGET={"symbol":"pr_u33_pair_writer_0b97","source":"overlay_u33_pair_writer_0b97.c"}
COORDINATES=[0,1,10,49,127,128,129,254,255]

def source_audit(root):
    exe=(root/"resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr=(root/"resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA,"START.EXE hash changed")
    check(sha256(ovr).hexdigest()==OVR_SHA,"GAME.OVR hash changed")
    unit=next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],ovr) if u["stub_para"]==0x33)
    check(unit["ovr_offset"]==0xC5D1 and unit["memory_segment"]==0x3B and
          unit["entries"][1]==0x1E31,"unit/public-entry placement changed")
    code=ovr[unit["ovr_offset"]:unit["ovr_offset"]+unit["code_size"]]
    body=code[ENTRY:END];decoder=Cs(CS_ARCH_X86,CS_MODE_16)
    check(len(body)==71 and sha256(body).hexdigest()==DIGEST,"leaf hash changed")
    decoded=list(decoder.disasm(body,ENTRY))
    check(len(decoded)==29 and decoded[-1].address==0x0BDB and
          decoded[-1].mnemonic=="retf" and decoded[-1].op_str=="6" and
          decoded[-1].address+decoded[-1].size==END,"leaf extent changed")
    check(all("call" not in i.mnemonic and not i.mnemonic.startswith(("j","loop"))
              for i in decoded),"leaf gained a call or branch")
    callers=[]
    for f in unit["functions"]:
        aligned={i.address:i for i in decoder.disasm(code[f["rel"]:f["rel"]+f["size_upper_bound"]],f["rel"])}
        for at,ins in aligned.items():
            if ins.mnemonic=="call" and ins.op_str=="0xb97":
                check(aligned[at-1].mnemonic=="push" and aligned[at-1].op_str=="cs" and
                      aligned[at-2].mnemonic=="push" and aligned[at-2].op_str=="bp",
                      "caller no longer passes BP/far frame")
                callers.append((f["rel"],at))
    check(callers==[(0x0BDE,0x0CB6),(0x0BDE,0x0CFB),(0x0BDE,0x0D0C)],"aligned caller set changed")
    pending,seen,path=deque([(unit["entries"][1],[])]),set(),None
    while pending:
        at,chain=pending.popleft()
        if at in seen or not 0<=at<len(code):continue
        seen.add(at)
        if at==ENTRY:path=chain;break
        ins=next(decoder.disasm(code[at:at+15],at,count=1),None)
        if not ins or ins.mnemonic.startswith("ret") or ins.mnemonic in ("int","iret"):continue
        if ins.mnemonic=="call":
            try:pending.append((int(ins.op_str,16),chain+[(at,int(ins.op_str,16))]))
            except ValueError:pass
        if ins.mnemonic.startswith(("j","loop")):
            try:pending.append((int(ins.op_str,16),chain))
            except ValueError:pass
            if ins.mnemonic=="jmp":continue
        pending.append((at+ins.size,chain))
    check(path==[(0x1EDE,0x1244),(0x12DA,0x11B4),(0x1219,0x0BDE),(0x0CFB,ENTRY)],"public-entry CFG path changed")
    print(f"u33 +0B97..+0BDD: 71 bytes, 29 instructions, zero calls/conditional branches; SHA-256 {DIGEST}")
    print("3 aligned BP/PUSH-CS callers:",[(hex(f),hex(at)) for f,at in callers])
    print("public entry 1 static call path:",[(hex(a),hex(b)) for a,b in path])
    return body,decoded

def fixture(body,x,y,before,*,ds=0x1000,ss=0x2000,sp=0x8000,
            grid_segment=0x4000,grid_offset=0x100,frame=0x9002,flags=0x7202):
    memory=bytearray([0xA5])*RAM_SIZE
    cs=0x3000;at=physical(cs,ENTRY);memory[at:at+len(body)]=body
    put16(memory,ds,0x6716,grid_offset);put16(memory,ds,0x6718,grid_segment)
    memory[physical(ss,(frame-2)&0xFFFF)]=before
    for displacement,value in [(0,0x7000),(2,cs),(4,frame),(6,0xA500|x),(8,0x5A00|y)]:
        put16(memory,ss,(sp+displacement)&0xFFFF,value)
    cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
            0x7777,sp,cs,ENTRY,ds,0x1111,ss,flags)
    return memory,cpu

def specifications():
    cases=[(f"xy-{x}-{y}-counter-{v}",x,y,v,{}) for x in COORDINATES for y in COORDINATES
           for v in (0,0x0E,0x0F,0x7E,0x7F,0x80,0xFE,0xFF)]
    rng=random.Random(0x330B97)
    cases += [(f"random-{i}",rng.randrange(256),rng.randrange(256),rng.randrange(256),
               {"grid_offset":rng.randrange(65536)}) for i in range(100)]
    for sp in (0,1,2,3,4,5,6,7,8,9,0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
        cases.append((f"stack-{sp:04x}",255,128,0xFF,{"sp":sp}))
    for name,target in [("grid-saved-BP",0x7FFE),("grid-return-IP",0x8000),
                        ("grid-return-CS",0x8002),("grid-frame-argument",0x8004),
                        ("grid-column-argument",0x8006),("grid-row-argument",0x8008),
                        ("grid-counter-first",0x9000),("grid-counter-second",0x8FFF)]:
        cases.append((name,0,0,0x7F,{"grid_segment":0x2000,"grid_offset":(target-7)&0xFFFF}))
    cases += [
        ("grid-pointer-offset-alias",0,0,0xFF,{"grid_segment":0x1000,"grid_offset":0x670F}),
        ("grid-pointer-segment-alias",0,0,0xFF,{"grid_segment":0x1000,"grid_offset":0x6711}),
        ("counter-saved-BP",0,0,0xFF,{"frame":0x8000}),
        ("counter-return-IP",0,0,0xFF,{"frame":0x8002}),
        ("counter-offset-wrap",0,0,0xFF,{"frame":0}),
        ("grid-offset-wrap",0,0,0xFF,{"grid_offset":0xFFFF}),
        ("A20-grid",0,0,0xFF,{"grid_segment":0xFFFF,"grid_offset":9}),
        ("A20-DS",0,0,0xFF,{"ds":0xFFFF}),
        ("A20-stack",0,0,0xFF,{"ss":0xFFFF,"sp":0x11}),
        ("A20-counter",0,0,0xFF,{"ss":0xFFFF,"frame":0x12}),
        ("DS-SS-shared",0,0,0xFF,{"ss":0x1000}),
        ("preserved-DF",128,255,0x7F,{"flags":0x0603}),
        ("incoming-arithmetic-flags",127,128,0xFF,{"flags":0x0ED7}),
    ]
    return cases

def differential(body,decoded,function):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    registers=[getattr(regs,"UC_X86_REG_"+("EFLAGS" if n=="flags" else n.upper())) for n,_ in Cpu._fields_]
    visited,edges=set(),set()
    cases=specifications()
    for label,x,y,before,options in cases:
        memory,cpu=fixture(body,x,y,before,**options)
        original=Uc(UC_ARCH_X86,UC_MODE_16);original.mem_map(0,2*RAM_SIZE)
        original.mem_write(0,bytes(memory));original.mem_write(RAM_SIZE,bytes(memory))
        for (name,_),register in zip(Cpu._fields_,registers):original.reg_write(register,getattr(cpu,name))
        start,last=physical(cpu.cs,ENTRY),physical(cpu.cs,0x0BDB)
        returned,previous,case_visited,case_edges=[False],[None],set(),set()
        def instruction(emulator,address,size,user):
            del size,user
            if returned[0]:emulator.emu_stop();return
            check(start<=address<start+len(body),f"left original body: {label}")
            here=address-start+ENTRY;case_visited.add(here)
            if previous[0] is not None:case_edges.add((previous[0],here))
            previous[0]=here
            if address==last:returned[0]=True
        def mirror(emulator,access,address,size,value,user):
            del access,user
            for byte in range(size):
                raw=address+byte;at=raw&(RAM_SIZE-1)
                emulator.mem_write(at+RAM_SIZE if raw<RAM_SIZE else at,bytes([(value>>(8*byte))&0xFF]))
        original.hook_add(UC_HOOK_CODE,instruction);original.hook_add(UC_HOOK_MEM_WRITE,mirror)
        original.emu_start(start,2*RAM_SIZE,count=20000)
        check(returned[0],f"original did not return: {label}")
        expected=bytes(original.mem_read(0,RAM_SIZE))
        check(expected[start:start+len(body)]==body,f"fixture changed immutable original code: {label}")
        actual,cpu=run_c(function,memory,cpu)
        if actual!=expected:
            differences=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
            raise ValueError(f"RAM mismatch {label}: {differences[:8]}")
        for (name,_),register in zip(Cpu._fields_,registers):
            check(getattr(cpu,name)==original.reg_read(register)&0xFFFF,
                  f"{name} mismatch {label}: C={getattr(cpu,name):04x}, original={original.reg_read(register)&0xffff:04x}")
        visited.update(case_visited);edges.update(case_edges)
    check(len(visited)==len(decoded),f"instruction coverage {len(visited)}/{len(decoded)}")
    expected_edges={(a.address,b.address) for a,b in zip(decoded,decoded[1:])}
    check(edges==expected_edges,f"fallthrough coverage mismatch: {sorted(expected_edges-edges)}")
    print(f"{len(cases)} original-byte cases passed: full 1 MiB RAM, all 14 CPU words, "
          f"{len(visited)} instructions, all {len(edges)} internal fallthrough edges, zero conditional branches")

def native_cases():
    for before in range(256):
        x=COORDINATES[before%len(COORDINATES)]
        y=COORDINATES[(before//len(COORDINATES))%len(COORDINATES)]
        column=x if x<128 else x-256;row=y if y<128 else y-256
        window=0x4000+50*row+column+5
        yield x,y,before,window

def native_assembly(source):
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
    mov word ptr [header+2],offset returned
    mov word ptr [header+4],offset leaf
    mov word ptr [case_ptr],offset fixtures
    mov dx,offset filename
    xor cx,cx
    mov ah,0x3c
    int 0x21
    jc failure
    mov word ptr [handle],ax
    mov bx,ax
    mov dx,offset header
    mov cx,6
    mov ah,0x40
    int 0x21
again:
    cli
    mov sp,0xf000
    mov ax,cs
    mov es,ax
    mov di,0xefe0
    mov ax,0xa5a5
    mov cx,16
    cld
    rep stosw
    mov si,word ptr [case_ptr]
    lodsw
    mov word ptr [case_x],ax
    lodsw
    mov word ptr [case_y],ax
    lodsw
    mov word ptr [case_before],ax
    mov byte ptr [0x9000],al
    lodsw
    mov word ptr [case_window],ax
    mov word ptr [case_ptr],si
    mov di,ax
    mov ax,0xa5a5
    mov cx,4
    rep stosw
    mov word ptr [0x6716],0x4000
    mov ax,cs
    mov word ptr [0x6718],ax
    push 0x0002
    popf
    mov ax,0x1234
    mov bx,0x3456
    mov cx,0x5678
    mov dx,0x789a
    mov si,0x9abc
    mov di,0xbcde
    mov bp,0x7777
    push word ptr [case_y]
    push word ptr [case_x]
    push 0x9002
    push cs
    call leaf
returned:
'''
    for i,(name,_) in enumerate(Cpu._fields_[:-1]):
        text+=f"    mov word ptr [output+{2*i}],"+("offset returned" if name=="ip" else name)+"\n"
    for i in range(16):
        text+=f"    mov ax,word ptr [{0xEFE0+2*i}]\n    mov word ptr [output+{28+2*i}],ax\n"
    text+="    pushf\n    pop ax\n    mov word ptr [output+26],ax\n"
    for i in range(2):
        text+=f"    mov ax,word ptr [{0x6716+2*i}]\n    mov word ptr [output+{60+2*i}],ax\n"
    text+=r'''
    mov al,byte ptr [0x9000]
    mov byte ptr [output+64],al
    mov si,word ptr [case_window]
    mov di,offset output+65
    mov cx,4
    cld
    rep movsw
'''
    for i,name in enumerate(("case_x","case_y","case_before","case_window")):
        text+=f"    mov ax,word ptr [{name}]\n    mov word ptr [output+{73+2*i}],ax\n"
    text+=r'''
    sti
    mov bx,word ptr [handle]
    mov dx,offset output
    mov cx,81
    mov ah,0x40
    int 0x21
    jc failure
    cmp ax,81
    jne failure
    inc word ptr [index]
    cmp word ptr [index],256
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
header: .space 6
handle: .word 0
index: .word 0
case_ptr: .word 0
case_x: .word 0
case_y: .word 0
case_before: .word 0
case_window: .word 0
output: .space 81
fixtures:
'''
    for x,y,before,window in native_cases():
        text+="    .word "+",".join(str(v) for v in (0xA500|x,0x5A00|y,before,window))+"\n"
    return text+'leaf:\n.incbin "'+str(source.resolve())+'"\n'

def native_audit(root,body,function):
    directory=root/"resources/oracle-runs/u33-pair-writer-0b97-synthetic"
    directory.mkdir(parents=True,exist_ok=True)
    source=directory/"leaf.bin";source.write_bytes(body)
    asm,obj,com=directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    asm.write_text(native_assembly(source))
    subprocess.run(["as","--32",str(asm),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    check(0x100+com.stat().st_size<0x2600,"COM overlaps synthetic grid RAM")
    check(all(0x2600<=window and window+8<=0x5A00 for _,_,_,window in native_cases()),"snapshot outside synthetic grid")
    config=directory/"dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
                      "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c "+str(directory.resolve())+"\nc:\nORACLE.COM\nexit\n")
    result_path=directory/"RESULT.BIN"
    if result_path.exists():result_path.unlink()
    environment=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy")
    with (directory/"dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
                       env=environment,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=90)
    result=result_path.read_bytes()
    check(len(result)==6+256*81,"native output incomplete")
    segment,return_ip,leaf_ip=struct.unpack_from("<HHH",result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)]==body,"copied native leaf differs from original")
    for index,(x,y,before,window) in enumerate(native_cases()):
        record=result[6+index*81:6+(index+1)*81]
        check(struct.unpack_from("<4H",record,73)==(0xA500|x,0x5A00|y,before,window),"native metadata mismatch")
        memory=bytearray([0xA5])*RAM_SIZE
        put16(memory,segment,0x6716,0x4000);put16(memory,segment,0x6718,segment)
        memory[physical(segment,0x9000)]=before
        for offset,value in [(0xEFF6,return_ip),(0xEFF8,segment),(0xEFFA,0x9002),
                             (0xEFFC,0xA500|x),(0xEFFE,0x5A00|y)]:
            put16(memory,segment,offset,value)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,0xEFF6,segment,leaf_ip,segment,segment,segment,0x0002)
        actual,cpu=run_c(function,memory,cpu)
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native {name} mismatch {index}: C={getattr(cpu,name):04x}, DOS={expected:04x}")
        for offset,size,position in [(0xEFE0,32,28),(0x6716,4,60),(0x9000,1,64),(window,8,65)]:
            at=physical(segment,offset)
            check(actual[at:at+size]==record[position:position+size],f"native RAM mismatch {index}/{offset:04x}")
    print("256 DOSBox-X synthetic cases passed: all 14 CPU words, 32 frame bytes, "
          "4 grid-pointer bytes, counter byte and 8-byte grid/sentinel window; no flag mask")
    print("native result SHA-256 "+sha256(result).hexdigest())

def main():
    root=Path(__file__).resolve().parents[2]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn",action="store_true")
    parser.add_argument("--native",action="store_true")
    args=parser.parse_args();body,decoded=source_audit(root)
    with tempfile.TemporaryDirectory(prefix="poolrad-u33-pair-") as temp:
        function=build_c(root,Path(temp),TARGET)
        if args.unicorn:differential(body,decoded,function)
        if args.native:native_audit(root,body,function)

if __name__=="__main__":main()
