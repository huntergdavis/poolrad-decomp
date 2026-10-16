#!/usr/bin/env python3
"""Full-state original-byte and synthetic native parity for u7f +0020."""
import argparse
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
ENTRY,END=0x0020,0x003A
DIGEST="f1e091b372c0401a7b90dac9e6a32f12c64b898c8f0039a69df380669e24e13a"
TARGET={"symbol":"pr_u7f_record_subtract_0020","source":"overlay_u7f_record_subtract_0020.c"}
WORDS=[0,1,7,8,15,16,127,128,255,256,0x7FFE,0x7FFF,0x8000,0x8001,0xFFFE,0xFFFF]
NATIVE_COUNT=512

def source_audit(root):
    exe=(root/"resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr=(root/"resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA,"START.EXE hash changed")
    check(sha256(ovr).hexdigest()==OVR_SHA,"GAME.OVR hash changed")
    unit=next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:],ovr) if u["stub_para"]==0x7F)
    check(unit["ovr_offset"]==0x24C89 and unit["memory_segment"]==0x8F,"unit placement changed")
    check([unit["entries"][i] for i in (7,2,8)]==[0x44C,0x508,0xA01],"public caller entries changed")
    code=ovr[unit["ovr_offset"]:unit["ovr_offset"]+unit["code_size"]]
    body=code[ENTRY:END];decoder=Cs(CS_ARCH_X86,CS_MODE_16)
    check(len(body)==26 and sha256(body).hexdigest()==DIGEST,"leaf hash changed")
    decoded=list(decoder.disasm(body,ENTRY))
    check(len(decoded)==9 and decoded[-1].address==0x37 and decoded[-1].mnemonic=="retf" and
          decoded[-1].op_str=="6" and decoded[-1].address+decoded[-1].size==END,"extent changed")
    check(all("call" not in i.mnemonic and not i.mnemonic.startswith(("j","loop")) for i in decoded),
          "leaf gained a call or branch")
    callers=[];aligned={}
    for f in unit["functions"]:
        instructions=list(decoder.disasm(code[f["rel"]:f["rel"]+f["size_upper_bound"]],f["rel"]))
        if f["rel"] in (0x44C,0x508,0xA01):aligned.update({i.address:i for i in instructions})
        for n,ins in enumerate(instructions):
            if ins.mnemonic=="call" and ins.op_str=="0x20":
                check(instructions[n-1].mnemonic=="push" and instructions[n-1].op_str=="cs",
                      "caller far-return frame changed")
                callers.append((f["rel"],ins.address))
    check(callers==[(0x44C,0x4AD),(0x508,0x5A8),(0xA01,0xA33)],"caller set changed")
    pushes={0x4A2:"word ptr es:[di + 2]",0x4A6:"word ptr es:[di]",0x4A9:"word ptr [bp + 8]",
            0x591:"word ptr [bp - 2]",0x594:"word ptr [bp - 4]",0x5A2:"word ptr es:[di + 0x88]",
            0xA29:"word ptr [bp + 0xc]",0xA2C:"word ptr [bp + 0xa]",0xA2F:"word ptr [bp + 8]"}
    check(all(aligned[a].mnemonic=="push" and aligned[a].op_str==op for a,op in pushes.items()),
          "aligned caller argument frame changed")
    check(aligned[0x50E].op_str=="ax, word ptr [0x5d96]" and
          aligned[0x511].op_str=="dx, word ptr [0x5d98]","game party-head owner changed")
    print(f"u7f +0020..+0039:26bytes/9instructions/zero calls or branches; SHA-256 {DIGEST}")
    print("3 aligned direct public callers:",[(hex(f),hex(at)) for f,at in callers])
    return body,decoded

def fixture(body,value,amount,*,ss=0x2000,sp=0x8000,record_segment=0x4000,
            record_offset=0x100,flags=0x7202):
    memory=bytearray([0xA5])*RAM_SIZE
    cs=0x3000;at=physical(cs,ENTRY);memory[at:at+len(body)]=body
    put16(memory,record_segment,(record_offset+0x102)&65535,value)
    for displacement,word in [(0,0x7000),(2,cs),(4,amount),(6,record_offset),(8,record_segment)]:
        put16(memory,ss,(sp+displacement)&65535,word)
    cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
            0x7777,sp,cs,ENTRY,0x1000,0x1111,ss,flags)
    return memory,cpu

def specifications():
    cases=[(f"words-{v}-{a}",v,a,{}) for v in WORDS for a in WORDS]
    rng=random.Random(0x7F0020)
    cases += [(f"random-{i}",rng.randrange(65536),rng.randrange(65536),
               {"record_offset":rng.randrange(65536)}) for i in range(256)]
    # Unicorn RETF pop-wrap differs at FFFE/FFFF; native_wrap_audit proves those cases.
    for sp in (0,1,2,3,4,5,6,7,8,9,0xFFF8,0xFFF9,0xFFFA,0xFFFB,0xFFFC,0xFFFD):
        cases.append((f"stack-{sp:04x}",0x8000,1,{"sp":sp}))
    for offset in (0,1,0xFEFC,0xFEFD,0xFEFE,0xFEFF,0xFF00,0xFFFD,0xFFFE,0xFFFF):
        cases.append((f"record-wrap-{offset:04x}",0x8000,1,{"record_offset":offset}))
    for target in (0x7FFD,0x7FFE,0x7FFF,0x8000,0x8001,0x8002,0x8003,
                   0x8004,0x8005,0x8006,0x8007,0x8008,0x8009,0x800A):
        cases.append((f"record-stack-alias-{target:04x}",0x8000,1,
                      {"record_segment":0x2000,"record_offset":(target-0x102)&65535}))
    cases += [
        ("A20-record-word",0x8000,1,{"record_segment":0xFFFF,"record_offset":0xFF0D}),
        ("A20-record-next",0x8000,1,{"record_segment":0xFFFF,"record_offset":0xFF0E}),
        ("A20-stack",0x8000,1,{"ss":0xFFFF,"sp":0x11}),
        ("record-DS-shared",0x8000,1,{"record_segment":0x1000}),
        ("preserved-DF",0x8000,1,{"flags":0x0603}),
        ("incoming-arithmetic-flags",0,1,{"flags":0x0ED7}),
    ]
    return cases

def differential(body,decoded,function):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    registers=[getattr(regs,"UC_X86_REG_"+("EFLAGS" if n=="flags" else n.upper())) for n,_ in Cpu._fields_]
    visited,edges=set(),set()
    cases=specifications()
    for label,value,amount,options in cases:
        memory,cpu=fixture(body,value,amount,**options)
        original=Uc(UC_ARCH_X86,UC_MODE_16);original.mem_map(0,2*RAM_SIZE)
        original.mem_write(0,bytes(memory));original.mem_write(RAM_SIZE,bytes(memory))
        for (name,_),register in zip(Cpu._fields_,registers):original.reg_write(register,getattr(cpu,name))
        start,last=physical(cpu.cs,ENTRY),physical(cpu.cs,0x0037)
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
    rng=random.Random(0x7F0021)
    for index in range(NATIVE_COUNT):
        value=WORDS[index%16] if index<256 else rng.randrange(65536)
        amount=WORDS[index//16] if index<256 else rng.randrange(65536)
        yield value,amount,0x9000+(index%8)

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
    mov di,0x9100
    mov cx,8
    rep stosw
    mov si,word ptr [case_ptr]
    lodsw
    mov word ptr [case_value],ax
    lodsw
    mov word ptr [case_amount],ax
    lodsw
    mov word ptr [case_record],ax
    mov word ptr [case_ptr],si
    mov di,ax
    add di,0x102
    mov ax,word ptr [case_value]
    mov word ptr [di],ax
    push 0x0002
    popf
    mov ax,0x1234
    mov bx,0x3456
    mov cx,0x5678
    mov dx,0x789a
    mov si,0x9abc
    mov di,0xbcde
    mov bp,0x7777
    push cs
    push word ptr [case_record]
    push word ptr [case_amount]
    push cs
    call leaf
returned:
'''
    for i,(name,_) in enumerate(Cpu._fields_[:-1]):
        text+=f"    mov word ptr [output+{2*i}],"+("offset returned" if name=="ip" else name)+"\n"
    for i in range(16):
        text+=f"    mov ax,word ptr [{0xEFE0+2*i}]\n    mov word ptr [output+{28+2*i}],ax\n"
    text+="    pushf\n    pop ax\n    mov word ptr [output+26],ax\n"
    for i in range(8):
        text+=f"    mov ax,word ptr [{0x9100+2*i}]\n    mov word ptr [output+{60+2*i}],ax\n"
    for i,name in enumerate(("case_value","case_amount","case_record")):
        text+=f"    mov ax,word ptr [{name}]\n    mov word ptr [output+{76+2*i}],ax\n"
    text+=r'''
    sti
    mov bx,word ptr [handle]
    mov dx,offset output
    mov cx,82
    mov ah,0x40
    int 0x21
    jc failure
    cmp ax,82
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
case_value: .word 0
case_amount: .word 0
case_record: .word 0
output: .space 82
fixtures:
'''
    check(len(list(native_cases()))==512,"native loop/count mismatch")
    for value,amount,record in native_cases():
        text+="    .word "+",".join(str(v) for v in (value,amount,record))+"\n"
    return text+'leaf:\n.incbin "'+str(source.resolve())+'"\n'

def native_audit(root,body,function):
    directory=root/"resources/oracle-runs/u7f-record-subtract-0020-synthetic"
    directory.mkdir(parents=True,exist_ok=True)
    source=directory/"leaf.bin";source.write_bytes(body)
    asm,obj,com=directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    asm.write_text(native_assembly(source))
    subprocess.run(["as","--32",str(asm),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    check(0x100+com.stat().st_size<0x2600,"COM overlaps synthetic data")
    check(all(0x9100<=record+0x102 and record+0x104<=0x9110 for _,_,record in native_cases()),
          "record write outside primed sentinel block")
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
    check(len(result)==6+NATIVE_COUNT*82,"native output incomplete")
    segment,return_ip,leaf_ip=struct.unpack_from("<HHH",result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)]==body,
          "copied native leaf differs from original")
    flags_seen={bit:set() for bit in (1,4,0x10,0x40,0x80,0x800)}
    for index,(value,amount,record_offset) in enumerate(native_cases()):
        record=result[6+index*82:6+(index+1)*82]
        check(struct.unpack_from("<3H",record,76)==(value,amount,record_offset),"native metadata mismatch")
        memory=bytearray([0xA5])*RAM_SIZE
        put16(memory,segment,record_offset+0x102,value)
        for offset,word in [(0xEFF6,return_ip),(0xEFF8,segment),(0xEFFA,amount),
                            (0xEFFC,record_offset),(0xEFFE,segment)]:
            put16(memory,segment,offset,word)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,0xEFF6,segment,leaf_ip,segment,segment,segment,0x0002)
        actual,cpu=run_c(function,memory,cpu)
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native {name} mismatch {index}:C={getattr(cpu,name):04x},DOS={expected:04x}")
        for offset,size,position in [(0xEFE0,32,28),(0x9100,16,60)]:
            at=physical(segment,offset)
            check(actual[at:at+size]==record[position:position+size],f"native RAM mismatch {index}/{offset:04x}")
        for bit in flags_seen:flags_seen[bit].add(bool(cpu.flags&bit))
    check(all(states=={False,True} for states in flags_seen.values()),"native omits SUB flag state")
    print("512 DOSBox-X synthetic cases passed:all14 CPU words,32 frame bytes,16 record/sentinel bytes;"
          " both states of all six SUB flags; no masks")
    print("native result SHA-256 "+sha256(result).hexdigest())

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
    mov word ptr es:[di],1
    add di,2
    mov word ptr es:[di],0x9000
    add di,2
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
    directory=root/"resources/oracle-runs/u7f-record-subtract-0020-wrap"
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
        for displacement,word in [(0,return_ip),(2,segment),(4,1),(6,0x9000),(8,segment)]:
            put16(memory,stack,(sp+displacement)&65535,word)
        put16(memory,stack+0x1000,(sp+2)&65535,stack)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,sp,segment,leaf_ip,segment,segment,stack,0x0002)
        actual,cpu=run_c(function,memory,cpu)
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native wrap {name} mismatch SP={sp:04x}:C={getattr(cpu,name):04x},DOS={expected:04x}")
        for seg,offset,size,position in [(stack,0xFFE0,32,28),(stack,0,32,60),
                                        (stack+0x1000,0,32,92),(segment,0x9100,16,124)]:
            at=physical(seg,offset)
            check(actual[at:at+size]==record[position:position+size],
                  f"native wrap RAM mismatch SP={sp:04x}/{seg:04x}:{offset:04x}")
    print("2 DOSBox-X SP=FFFE/FFFF cases passed:all14 CPU words,96 stack/sentinel bytes,"
          "16 record/sentinel bytes; distinct linear-CS decoy rejected; no masks")
    print("native wrap result SHA-256 "+sha256(result).hexdigest())

def main():
    root=Path(__file__).resolve().parents[2]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn",action="store_true")
    parser.add_argument("--native",action="store_true")
    args=parser.parse_args();body,decoded=source_audit(root)
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-subtract-") as temp:
        function=build_c(root,Path(temp),TARGET)
        if args.unicorn:differential(body,decoded,function)
        if args.native:
            native_audit(root,body,function)
            native_wrap_audit(root,body,function)

if __name__=="__main__":main()
