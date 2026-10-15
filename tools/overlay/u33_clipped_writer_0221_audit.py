#!/usr/bin/env python3
"""Full-state original-byte and synthetic native parity for u33 +0221."""
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
ENTRY,END=0x0221,0x02BA
DIGEST="9a97dca1163b4aabd6193cea3a81427143709390e7b4aa2dec1a2a727236cd06"
TARGET={"symbol":"pr_u33_clipped_writer_0221","source":"overlay_u33_clipped_writer_0221.c"}
COLUMNS=[128,255,0,1,48,49,50,127]
ROWS=[128,255,0,1,23,24,25,127]
NATIVE_COUNT=512
CALLERS=[(0x2BA,a) for a in (0x2E1,0x315,0x327,0x33C,0x35E,0x36E)]
CALLERS += [(0x377,a) for a in (0x394,0x3A4,0x3B4,0x3C4,0x3D6,0x3E6,0x3F6,0x406)]
CALLERS += [(0x40F,a) for a in (0x5D9,0x5EA,0x5FB,0x60C)]
CALLERS += [(0x615,a) for a in (0x7D4,0x7E5,0x7F6,0x807)]

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
    check(len(body)==153 and sha256(body).hexdigest()==DIGEST,"leaf hash changed")
    decoded=list(decoder.disasm(body,ENTRY))
    check(len(decoded)==64 and decoded[-1].address==0x02B7 and
          decoded[-1].mnemonic=="retf" and decoded[-1].op_str=="8" and
          decoded[-1].address+decoded[-1].size==END,"leaf extent changed")
    check(all("call" not in i.mnemonic for i in decoded),"leaf gained a call")
    check(sum(i.mnemonic in ("jl","jg") for i in decoded)==4,"clip branch set changed")
    callers=[]
    for f in unit["functions"]:
        aligned=list(decoder.disasm(code[f["rel"]:f["rel"]+f["size_upper_bound"]],f["rel"]))
        for n,ins in enumerate(aligned):
            if ins.mnemonic=="call" and ins.op_str=="0x221":
                check(aligned[n-1].mnemonic=="push" and aligned[n-1].op_str=="cs" and
                      aligned[n-2].mnemonic=="push" and aligned[n-2].op_str=="word ptr [bp + 6]",
                      "caller no longer forwards inherited frame/far return")
                callers.append((f["rel"],ins.address))
    check(callers==CALLERS,"aligned caller set changed")
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
    check(path==[(0x1EDE,0x1244),(0x12D4,0x0810),(0x087E,0x02BA),(0x02E1,ENTRY)],"public-entry CFG path changed")
    print(f"u33 +0221..+02B9: 153 bytes, 64 instructions, zero calls/four conditional branches; SHA-256 {DIGEST}")
    print("22 aligned inherited-frame/PUSH-CS callers:",[(hex(f),hex(at)) for f,at in callers])
    print("public entry 1 static call path:",[(hex(a),hex(b)) for a,b in path])
    return body,decoded

def signed(value):
    return value if value<128 else value-256

def fixture(body,a,b,glyph,x,y,*,ds=0x1000,ss=0x2000,sp=0x8000,
            grid_segment=0x4000,grid_offset=0x100,frame=0x9002,flags=0x7202):
    memory=bytearray([0xA5])*RAM_SIZE
    cs=0x3000;at=physical(cs,ENTRY);memory[at:at+len(body)]=body
    put16(memory,ds,0x6716,grid_offset);put16(memory,ds,0x6718,grid_segment)
    memory[physical(ss,(frame-2)&0xFFFF)]=a
    memory[physical(ss,(frame-1)&0xFFFF)]=b
    for displacement,value in [(0,0x7000),(2,cs),(4,frame),(6,0xCC00|glyph),
                               (8,0xA500|y),(10,0x5A00|x)]:
        put16(memory,ss,(sp+displacement)&0xFFFF,value)
    cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
            0x7777,sp,cs,ENTRY,ds,0x1111,ss,flags)
    return memory,cpu

def coordinates(a,b,column,row,glyph):
    return a,b,glyph,(column-5*signed(a)-6*signed(b)-21)&255,(row-5*signed(a)-10)&255

def specifications():
    cases=[]
    for a,b in ((0,0),(127,128),(128,127),(255,255)):
        for column in COLUMNS:
            for row in ROWS:
                args=coordinates(a,b,column,row,(column+row+a+b)&255)
                cases.append((f"clip-{a}-{b}-{column}-{row}",args,{}))
    rng=random.Random(0x330221)
    for i in range(500):
        cases.append((f"random-{i}",tuple(rng.randrange(256) for _ in range(5)),
                      {"grid_offset":rng.randrange(65536)}))
    for sp in (0,1,2,3,4,5,6,7,8,9,0xFFF8,0xFFF9,0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
        cases.append((f"stack-{sp:04x}",coordinates(0,0,0,0,255),{"sp":sp}))
    for frame in (0,1,2,0xFFFF,0x7FFD,0x7FFE,0x7FFF,0x8000,0x8001,0x8002,
                  0x8003,0x8004,0x8005,0x8006,0x8007,0x8008,0x8009,0x800A,0x800B,0x800C):
        cases.append((f"inherited-frame-{frame:04x}",coordinates(0,0,1,1,255),{"frame":frame}))
    for target in (0x7FFC,0x7FFD,0x7FFE,0x7FFF,0x8000,0x8001,0x8002,0x8003,
                   0x8004,0x8006,0x8008,0x800A,0x9000,0x9001):
        cases.append((f"grid-frame-alias-{target:04x}",coordinates(0,0,0,0,255),
                      {"grid_segment":0x2000,"grid_offset":(target-7)&65535}))
    cases += [
        ("grid-pointer-offset",coordinates(0,0,0,0,255),{"grid_segment":0x1000,"grid_offset":0x670F}),
        ("grid-pointer-segment",coordinates(0,0,0,0,255),{"grid_segment":0x1000,"grid_offset":0x6711}),
        ("grid-carry",coordinates(255,128,49,24,255),{"grid_offset":0xFB4F}),
        ("grid-offset-wrap",coordinates(0,0,49,24,255),{"grid_offset":0xFFFF}),
        ("A20-grid",coordinates(0,0,0,0,255),{"grid_segment":0xFFFF,"grid_offset":9}),
        ("A20-DS",coordinates(0,0,0,0,255),{"ds":0xFFFF}),
        ("A20-stack",coordinates(0,0,0,0,255),{"ss":0xFFFF,"sp":0x13}),
        ("A20-inherited-frame",coordinates(0,0,0,0,255),{"ss":0xFFFF,"frame":0x12}),
        ("DS-SS-shared",coordinates(0,0,0,0,255),{"ss":0x1000}),
        ("local-overwrites-grid-low",coordinates(0,0,0,0,255),{"ss":0x1000,"sp":0x671A}),
        ("local-overwrites-grid-high",coordinates(0,0,0,0,255),{"ss":0x1000,"sp":0x671C}),
        ("preserved-DF",coordinates(128,255,49,24,255),{"flags":0x0603}),
        ("incoming-arithmetic",coordinates(127,128,0,0,0),{"flags":0x0ED7}),
    ]
    return cases

def differential(body,decoded,function):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    registers=[getattr(regs,"UC_X86_REG_"+("EFLAGS" if n=="flags" else n.upper())) for n,_ in Cpu._fields_]
    visited,edges=set(),set()
    cases=specifications()
    for label,arguments,options in cases:
        memory,cpu=fixture(body,*arguments,**options)
        original=Uc(UC_ARCH_X86,UC_MODE_16);original.mem_map(0,2*RAM_SIZE)
        original.mem_write(0,bytes(memory));original.mem_write(RAM_SIZE,bytes(memory))
        for (name,_),register in zip(Cpu._fields_,registers):original.reg_write(register,getattr(cpu,name))
        start,last=physical(cpu.cs,ENTRY),physical(cpu.cs,0x02B7)
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
    expected_edges=set();branches=set()
    for ins in decoded[:-1]:
        expected_edges.add((ins.address,ins.address+ins.size))
        if ins.mnemonic in ("jl","jg"):
            target=int(ins.op_str,16)
            expected_edges.add((ins.address,target))
            branches.update(((ins.address,target),(ins.address,ins.address+ins.size)))
    check(edges==expected_edges,f"CFG coverage mismatch: {sorted(expected_edges-edges)}")
    check(len(branches)==8 and branches<=edges,"missing clipping outcome")
    print(f"{len(cases)} original-byte cases passed: full 1 MiB RAM, all 14 CPU words, "
          f"{len(visited)} instructions, all {len(edges)} internal CFG edges/eight clipping outcomes")

def native_cases():
    for index in range(NATIVE_COUNT):
        column=COLUMNS[index%8] if index<256 else index%50
        row=ROWS[(index//8)%8] if index<256 else (index//50)%25
        a=index&255;b=(73*index+31)&255
        a,b,glyph,x,y=coordinates(a,b,column,row,index&255)
        window=0x4000+50*(row if row<25 else 0)+(column if column<50 else 0)+5
        yield a,b,glyph,x,y,window

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
    mov word ptr [case_a],ax
    mov byte ptr [0x9000],al
    lodsw
    mov word ptr [case_b],ax
    mov byte ptr [0x9001],al
    lodsw
    mov word ptr [case_glyph],ax
    lodsw
    mov word ptr [case_x],ax
    lodsw
    mov word ptr [case_y],ax
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
    push word ptr [case_x]
    push word ptr [case_y]
    push word ptr [case_glyph]
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
    mov ax,word ptr [0x9000]
    mov word ptr [output+64],ax
    mov si,word ptr [case_window]
    mov di,offset output+66
    mov cx,4
    cld
    rep movsw
'''
    for i,name in enumerate(("case_a","case_b","case_glyph","case_x","case_y","case_window")):
        text+=f"    mov ax,word ptr [{name}]\n    mov word ptr [output+{74+2*i}],ax\n"
    text+=r'''
    sti
    mov bx,word ptr [handle]
    mov dx,offset output
    mov cx,86
    mov ah,0x40
    int 0x21
    jc failure
    cmp ax,86
    jne failure
    inc word ptr [index]
    cmp word ptr [index],512
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
case_a: .word 0
case_b: .word 0
case_glyph: .word 0
case_window: .word 0
output: .space 86
fixtures:
'''
    for a,b,glyph,x,y,window in native_cases():
        text+="    .word "+",".join(str(v) for v in (a,b,0xCC00|glyph,0x5A00|x,0xA500|y,window))+"\n"
    check(len(list(native_cases()))==512,"native assembly loop/count mismatch")
    return text+'leaf:\n.incbin "'+str(source.resolve())+'"\n'

def native_audit(root,body,function):
    paths,stored_glyphs=set(),set()
    for a,b,glyph,x,y,_ in native_cases():
        column=(5*signed(a)+6*signed(b)+21+x)&255
        row=(5*signed(a)+10+y)&255
        path=0 if column>=128 else 1 if column>49 else 2 if row>=128 else 3 if row>24 else 4
        paths.add(path)
        if path==4:stored_glyphs.add(glyph)
    check(paths==set(range(5)) and stored_glyphs==set(range(256)),
          "native fixtures omit clipping exit or stored glyph")
    directory=root/"resources/oracle-runs/u33-clipped-writer-0221-synthetic"
    directory.mkdir(parents=True,exist_ok=True)
    source=directory/"leaf.bin";source.write_bytes(body)
    asm,obj,com=directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    asm.write_text(native_assembly(source))
    subprocess.run(["as","--32",str(asm),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    check(0x100+com.stat().st_size<0x2600,"COM overlaps synthetic grid RAM")
    check(all(0x2600<=window and window+8<=0x5A00 for _,_,_,_,_,window in native_cases()),"snapshot outside synthetic grid")
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
    check(len(result)==6+NATIVE_COUNT*86,"native output incomplete")
    segment,return_ip,leaf_ip=struct.unpack_from("<HHH",result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)]==body,"copied native leaf differs from original")
    for index,(a,b,glyph,x,y,window) in enumerate(native_cases()):
        record=result[6+index*86:6+(index+1)*86]
        check(struct.unpack_from("<6H",record,74)==(a,b,0xCC00|glyph,0x5A00|x,0xA500|y,window),"native metadata mismatch")
        memory=bytearray([0xA5])*RAM_SIZE
        put16(memory,segment,0x6716,0x4000);put16(memory,segment,0x6718,segment)
        memory[physical(segment,0x9000)]=a
        memory[physical(segment,0x9001)]=b
        for offset,value in [(0xEFF4,return_ip),(0xEFF6,segment),(0xEFF8,0x9002),
                             (0xEFFA,0xCC00|glyph),(0xEFFC,0xA500|y),(0xEFFE,0x5A00|x)]:
            put16(memory,segment,offset,value)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,0xEFF4,segment,leaf_ip,segment,segment,segment,0x0002)
        actual,cpu=run_c(function,memory,cpu)
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native {name} mismatch {index}: C={getattr(cpu,name):04x}, DOS={expected:04x}")
        for offset,size,position in [(0xEFE0,32,28),(0x6716,4,60),(0x9000,2,64),(window,8,66)]:
            at=physical(segment,offset)
            check(actual[at:at+size]==record[position:position+size],f"native RAM mismatch {index}/{offset:04x}")
    print("512 DOSBox-X synthetic cases passed: all 14 CPU words, 32 frame bytes, "
          "4 grid-pointer bytes, two inherited-frame bytes and 8-byte grid/sentinel window; no flag mask")
    print("native result SHA-256 "+sha256(result).hexdigest())

def main():
    root=Path(__file__).resolve().parents[2]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn",action="store_true")
    parser.add_argument("--native",action="store_true")
    args=parser.parse_args();body,decoded=source_audit(root)
    with tempfile.TemporaryDirectory(prefix="poolrad-u33-clipped-") as temp:
        function=build_c(root,Path(temp),TARGET)
        if args.unicorn:differential(body,decoded,function)
        if args.native:native_audit(root,body,function)

if __name__=="__main__":main()
