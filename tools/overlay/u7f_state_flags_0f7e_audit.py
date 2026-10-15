#!/usr/bin/env python3
"""Original-byte full-state and synthetic native parity for u7F +0F7E."""
import argparse
from hashlib import sha256
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from party_reorder_oracle import check, build_c, run_c
from u5e_party_value_2879_audit import Cpu, EXE_SHA, OVR_SHA, physical, put16

RAM_SIZE = 1 << 20
ENTRY, END = 0x0F7E, 0x0FD2
DIGEST = "5db57d103e9038273938138f1980a39a0af093ac1b811d8e50fa178f5371c51b"
TARGET = {"symbol": "pr_u7f_state_flags_0f7e", "source": "overlay_u7f_state_flags_0f7e.c"}
HEADS = [1,0x80,0x100,0x8000,0xFFFF]
CALLERS = {(0x1E,0x0D4A),(0x1E,0x0E1E),(0x20,0x0DF2),(0x20,0x0EF1),
           (0x20,0x10A4),(0x23,0x05C9),(0x23,0x068D)}

def source_audit(root):
    exe = (root / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (root / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA, "START.EXE hash changed")
    check(sha256(ovr).hexdigest()==OVR_SHA, "GAME.OVR hash changed")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:],ovr)
    unit = next(u for u in units if u["stub_para"]==0x7F)
    check(unit["ovr_offset"]==0x24C89 and unit["memory_segment"]==0x8F and
          unit["entries"][9]==ENTRY, "public entry placement changed")
    body = ovr[0x24C89+ENTRY:0x24C89+END]
    check(len(body)==84 and sha256(body).hexdigest()==DIGEST, "leaf hash changed")
    decoder = Cs(CS_ARCH_X86,CS_MODE_16)
    decoded = list(decoder.disasm(body,ENTRY))
    check(len(decoded)==30 and decoded[-1].address==0x0FCF and
          decoded[-1].mnemonic=="retf" and decoded[-1].op_str=="8" and
          decoded[-1].address+decoded[-1].size==END, "leaf extent changed")
    starts = {i.address for i in decoded}
    for ins in decoded:
        check("call" not in ins.mnemonic, "leaf gained a call")
        if ins.mnemonic.startswith("j"):
            check(int(ins.op_str,16) in starts, "branch leaves leaf")
    found = set()
    for caller in units:
        code = ovr[caller["ovr_offset"]:caller["ovr_offset"]+caller["code_size"]]
        for fix in caller["fixups"]:
            if fix<3 or fix+2>len(code) or code[fix-3]!=0x9A: continue
            if struct.unpack_from("<HH",code,fix-2)!=(0x4D,0x8F): continue
            site = fix-3
            function = max((f for f in caller["functions"] if f["rel"]<=site),key=lambda f:f["rel"])
            aligned = {i.address:i for i in decoder.disasm(
                code[function["rel"]:function["rel"]+function["size_upper_bound"]],function["rel"])}
            check(site in aligned and aligned[site].mnemonic=="lcall", "unaligned caller")
            found.add((caller["stub_para"],site))
    check(found==CALLERS, f"caller set changed: {sorted(found)}")
    print(f"u7F public entry 9 +0F7E..+0FD1: 84 bytes, 30 instructions, zero calls; SHA-256 {DIGEST}")
    print("7 aligned fixup-backed callers:",[(hex(u),hex(at)) for u,at in sorted(found)])
    return body,decoded

def inputs(mask,head):
    words = []
    for i in range(7):
        value = (1 << (i+1)) if mask & (1 << i) else 0
        words += [0,value] if i%2 else [value,0]
    value = HEADS[mask%len(HEADS)] if head else 0
    return words,((0,value) if mask%2 else (value,0))

def fixture(body,words,head,*,ds=0x1000,ss=0x2000,sp=0x8000,
            shared=(0x4000,0x100),head_output=(0x5000,0x200),flags=0x7202):
    memory = bytearray([0xA5])*RAM_SIZE
    cs = 0x3000
    at = physical(cs,ENTRY);memory[at:at+len(body)]=body
    for i,value in enumerate(words): put16(memory,ds,0x67F4+2*i,value)
    put16(memory,ds,0x6810,head[0]);put16(memory,ds,0x6812,head[1])
    for delta,value in [(0,0x7000),(2,cs),(4,head_output[1]),(6,head_output[0]),
                        (8,shared[1]),(10,shared[0])]:
        put16(memory,ss,(sp+delta)&0xFFFF,value)
    cpu = Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
              0x7777,sp,cs,ENTRY,ds,0x1111,ss,flags)
    return memory,cpu

def specifications():
    cases = [(f"mask-{mask}-head-{head}",*inputs(mask,head),{})
             for mask in range(128) for head in (0,1)]
    rng = random.Random(0x7F0F7E)
    for i in range(100):
        cases.append((f"random-{i}",[rng.randrange(65536) for _ in range(14)],
                      (rng.randrange(65536),rng.randrange(65536)),{}))
    words,head = inputs(65,1)
    for sp in (0,1,2,3,4,5,6,7,8,9,10,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):
        cases.append((f"stack-{sp:04x}",words,head,{"sp":sp}))
    for label,options in [
        ("same-outputs",{"head_output":(0x4000,0x100)}),
        ("shared-table-alias",{"shared":(0x1000,0x67F4)}),
        ("head-table-alias",{"head_output":(0x1000,0x680C)}),
        ("shared-head-alias",{"shared":(0x1000,0x6810)}),
        ("head-head-alias",{"head_output":(0x1000,0x6812)}),
        ("head-counter-alias",{"head_output":(0x2000,0x7FFD)}),
        ("shared-saved-BP-alias",{"shared":(0x2000,0x7FFE)}),
        ("head-return-IP-alias",{"head_output":(0x2000,0x8000)}),
        ("shared-head-argument-alias",{"shared":(0x2000,0x8006)}),
        ("shared-own-argument-alias",{"shared":(0x2000,0x8008)}),
        ("A20-outputs",{"shared":(0xFFFF,0x100),"head_output":(0xFFFF,0x110)}),
        ("A20-DS",{"ds":0xFFFF}),
        ("A20-stack",{"ss":0xFFFF,"sp":0x11}),
        ("DS-SS-shared",{"ss":0x1000}),
        ("preserved-DF",{"flags":0x0603}),
        ("incoming-arithmetic-flags",{"flags":0x0ED7}),
    ]: cases.append((label,words,head,options))
    single,_ = inputs(1,0)
    cases.append(("shared-counter-alias-terminating",single,(0,0),{"shared":(0x2000,0x7FFD)}))
    all_words,_ = inputs(127,0)
    cases.append(("shared-counter-alias-return-budget",all_words,(0,0),
                  {"shared":(0x2000,0x7FFD),"expect_nonreturn":True}))
    return cases

def differential(body,decoded,function):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn import x86_const as regs
    registers = [getattr(regs,"UC_X86_REG_"+("EFLAGS" if n=="flags" else n.upper())) for n,_ in Cpu._fields_]
    conditional = {i.address:(i.address+i.size,int(i.op_str,16)) for i in decoded
                   if i.mnemonic.startswith("j") and i.mnemonic!="jmp"}
    visited,edges,nonreturns = set(),set(),0
    cases = specifications()
    for label,words,head,options in cases:
        options = dict(options);expect_nonreturn = options.pop("expect_nonreturn",False)
        memory,cpu = fixture(body,words,head,**options)
        original = Uc(UC_ARCH_X86,UC_MODE_16);original.mem_map(0,2*RAM_SIZE)
        original.mem_write(0,bytes(memory));original.mem_write(RAM_SIZE,bytes(memory))
        for (name,_),register in zip(Cpu._fields_,registers): original.reg_write(register,getattr(cpu,name))
        start,last = physical(cpu.cs,ENTRY),physical(cpu.cs,0x0FCF)
        returned,previous,case_visited,case_edges = [False],[None],set(),set()
        def instruction(emulator,address,size,user):
            del size,user
            if returned[0]: emulator.emu_stop();return
            check(start<=address<start+len(body),f"left original body: {label}")
            here = address-start+ENTRY;case_visited.add(here)
            if previous[0] in conditional: case_edges.add((previous[0],here))
            previous[0]=here
            if address==last: returned[0]=True
        def mirror(emulator,access,address,size,value,user):
            del access,user
            for byte in range(size):
                raw=address+byte;at=raw&(RAM_SIZE-1)
                emulator.mem_write(at+RAM_SIZE if raw<RAM_SIZE else at,bytes([(value>>(8*byte))&0xFF]))
        original.hook_add(UC_HOOK_CODE,instruction);original.hook_add(UC_HOOK_MEM_WRITE,mirror)
        original.emu_start(start,2*RAM_SIZE,count=20000)
        if expect_nonreturn:
            check(not returned[0],f"expected original return-budget failure changed: {label}")
            nonreturns+=1;continue
        check(returned[0],f"original did not return within 20,000 instructions: {label}")
        expected=bytes(original.mem_read(0,RAM_SIZE));actual,cpu=run_c(function,memory,cpu)
        if actual!=expected:
            differences=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
            raise ValueError(f"RAM mismatch {label}: {differences[:8]}")
        for (name,_),register in zip(Cpu._fields_,registers):
            check(getattr(cpu,name)==original.reg_read(register)&0xFFFF,
                  f"{name} mismatch {label}: C={getattr(cpu,name):04x}, original={original.reg_read(register)&0xffff:04x}")
        visited.update(case_visited);edges.update(case_edges)
    check(len(visited)==len(decoded),f"instruction coverage {len(visited)}/{len(decoded)}")
    missing={(site,outcome) for site,outcomes in conditional.items() for outcome in outcomes}-edges
    check(not missing,f"missing conditional outcomes: {sorted(missing)}")
    print(f"{len(cases)-nonreturns} original-byte cases passed: full 1 MiB RAM, all 14 CPU words, "
          f"{len(visited)} instructions, all {sum(len(x) for x in conditional.values())} conditional outcomes")
    print(f"{nonreturns} malformed original fixture fails the 20,000-instruction return budget; C not invoked")

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
    mov word ptr [case_mask],ax
    lodsw
    mov word ptr [case_head],ax
'''
    for i in range(16): text+=f"    lodsw\n    mov word ptr [{0x67F4+2*i}],ax\n"
    text+=r'''
    mov word ptr [case_ptr],si
    mov byte ptr [0x4000],0xa5
    mov byte ptr [0x4100],0xa5
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
    push 0x4000
    push cs
    push 0x4100
    push cs
    call leaf
returned:
'''
    for i,(name,_) in enumerate(Cpu._fields_[:-1]):
        text+=f"    mov word ptr [output+{2*i}],"+("offset returned" if name=="ip" else name)+"\n"
    for i in range(16):
        text+=f"    mov ax,word ptr [{0xEFE0+2*i}]\n    mov word ptr [output+{28+2*i}],ax\n"
    text+="    pushf\n    pop ax\n    mov word ptr [output+26],ax\n"
    for i in range(16):
        text+=f"    mov ax,word ptr [{0x67F4+2*i}]\n    mov word ptr [output+{60+2*i}],ax\n"
    text+=r'''
    mov al,byte ptr [0x4000]
    mov byte ptr [output+92],al
    mov al,byte ptr [0x4100]
    mov byte ptr [output+93],al
    mov ax,word ptr [case_mask]
    mov word ptr [output+94],ax
    mov ax,word ptr [case_head]
    mov word ptr [output+96],ax
    sti
    mov bx,word ptr [handle]
    mov dx,offset output
    mov cx,98
    mov ah,0x40
    int 0x21
    jc failure
    cmp ax,98
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
case_mask: .word 0
case_head: .word 0
output: .space 98
fixtures:
'''
    for mask in range(128):
        for head in (0,1):
            words,pair=inputs(mask,head)
            text+="    .word "+",".join(str(v) for v in [mask,head]+words+list(pair))+"\n"
    return text+'leaf:\n.incbin "'+str(source.resolve())+'"\n'

def native_audit(root,body,function):
    directory=root/"resources/oracle-runs/u7f-state-flags-0f7e-synthetic"
    directory.mkdir(parents=True,exist_ok=True)
    source=directory/"leaf.bin";source.write_bytes(body)
    asm,obj,com=directory/"oracle.S",directory/"oracle.o",directory/"ORACLE.COM"
    asm.write_text(native_assembly(source))
    subprocess.run(["as","--32",str(asm),"-o",str(obj)],check=True)
    subprocess.run(["ld","-m","elf_i386","-Ttext","0x100","--oformat","binary",str(obj),"-o",str(com)],check=True)
    check(0x100+com.stat().st_size<0x4000,"COM overlaps tested output RAM")
    config=directory/"dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n"
                      "[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c "+str(directory.resolve())+"\nc:\nORACLE.COM\nexit\n")
    result_path=directory/"RESULT.BIN"
    if result_path.exists(): result_path.unlink()
    environment=dict(os.environ,SDL_VIDEODRIVER="dummy",SDL_AUDIODRIVER="dummy")
    with (directory/"dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x","-conf",str(config),"-fastlaunch","-exit","-noconsole"],
                       env=environment,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)
    result=result_path.read_bytes()
    check(len(result)==6+256*98,"native output incomplete")
    segment,return_ip,leaf_ip=struct.unpack_from("<HHH",result)
    check(com.read_bytes()[leaf_ip-0x100:leaf_ip-0x100+len(body)]==body,"copied native leaf differs from original")
    for index in range(256):
        record=result[6+index*98:6+(index+1)*98]
        mask,selector=struct.unpack_from("<HH",record,94)
        check(mask==index//2 and selector==index%2,"native metadata mismatch")
        words,head=inputs(mask,selector)
        memory=bytearray([0xA5])*RAM_SIZE
        for i,value in enumerate(words+list(head)): put16(memory,segment,0x67F4+2*i,value)
        for offset,value in [(0xEFF4,return_ip),(0xEFF6,segment),(0xEFF8,0x4100),
                             (0xEFFA,segment),(0xEFFC,0x4000),(0xEFFE,segment)]:
            put16(memory,segment,offset,value)
        cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
                0x7777,0xEFF4,segment,leaf_ip,segment,segment,segment,0x0002)
        actual,cpu=run_c(function,memory,cpu)
        for (name,_),expected in zip(Cpu._fields_,struct.unpack_from("<14H",record)):
            check(getattr(cpu,name)==expected,
                  f"native {name} mismatch {index}: C={getattr(cpu,name):04x}, DOS={expected:04x}")
        for offset,size,position in [(0xEFE0,32,28),(0x67F4,32,60),(0x4000,1,92),(0x4100,1,93)]:
            at=physical(segment,offset)
            check(actual[at:at+size]==record[position:position+size],f"native RAM mismatch {index}/{offset:04x}")
    print("256 DOSBox-X synthetic cases passed: all 14 CPU words, 32 frame bytes, "
          "seven dwords/head and both output bytes; no flag mask")
    print("native result SHA-256 "+sha256(result).hexdigest())

def main():
    root=Path(__file__).resolve().parents[2]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn",action="store_true")
    parser.add_argument("--native",action="store_true")
    args=parser.parse_args();body,decoded=source_audit(root)
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-flags-") as temp:
        function=build_c(root,Path(temp),TARGET)
        if args.unicorn: differential(body,decoded,function)
        if args.native: native_audit(root,body,function)

if __name__=="__main__": main()
