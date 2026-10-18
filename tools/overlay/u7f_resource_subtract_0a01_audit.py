#!/usr/bin/env python3
"""Original-byte/full-RAM parity for U7F public entry 8 and its closed +0020 callee."""
import argparse
import ctypes
import json
from hashlib import sha256
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from party_reorder_oracle import check
from u5e_party_value_2879_audit import Cpu,EXE_SHA,OVR_SHA,physical,put16
from u7f_record_subtract_0020_audit import source_audit as callee_audit

RAM_SIZE=1 << 20
ENTRY,END=0x0A01,0x0A7D
DIGEST="3e39eb5fa8ac9c4fa437e885b9a452cd5a7913ee3a3f2cfbefafde2d0f543aa2"

def source_audit(root, exe_path=None, ovr_path=None):
    helper,helper_instructions=callee_audit(root,exe_path,ovr_path)
    exe=(exe_path or root/"resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr=(ovr_path or root/"resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,"media hash changed")
    units=find_units(exe[parse_mz(exe)["header_bytes"]:],ovr)
    unit=next(u for u in units if u["stub_para"]==0x7F)
    check(unit["entries"][8]==ENTRY and unit["ovr_offset"]==0x24C89,"owner placement changed")
    body=ovr[unit["ovr_offset"]+ENTRY:unit["ovr_offset"]+END]
    decoder=Cs(CS_ARCH_X86,CS_MODE_16);instructions=list(decoder.disasm(body,ENTRY))
    check(len(body)==124 and sha256(body).hexdigest()==DIGEST,"owner byte hash changed")
    check(len(instructions)==49 and instructions[-1].address==0xA7A and
          instructions[-1].mnemonic=="retf" and instructions[-1].op_str=="8" and
          instructions[-1].address+instructions[-1].size==END,"owner extent changed")
    calls=[(i.address,i.mnemonic,i.op_str) for i in instructions if "call" in i.mnemonic]
    check(calls==[(0xA33,"call","0x20")],"dependency set changed")
    callers=[]
    for u in units:
        code=ovr[u["ovr_offset"]:u["ovr_offset"]+u["code_size"]]
        for f in u["functions"]:
            decoded=list(decoder.disasm(code[f["rel"]:f["rel"]+f["size_upper_bound"]],f["rel"]))
            for n,i in enumerate(decoded):
                if len(i.bytes)==5 and i.bytes[0]==0x9A and struct.unpack_from("<HH",i.bytes,1)==(0x48,0x8F):
                    check(i.address+3 in u["fixups"],"caller lacks segment fixup")
                    callers.append((u["stub_para"],f["rel"],i.address))
                    if callers[-1]==(0x77,0x24A7,0x270C):
                        previous={j.address:j for j in decoded[max(0,n-6):n]}
                        expected={0x26FC:"word ptr [0x5d94]",0x2700:"word ptr [0x5d92]",
                                  0x2704:"word ptr [bp - 0x12e]",0x270B:"ax"}
                        check(all(previous[a].mnemonic=="push" and previous[a].op_str==v
                                  for a,v in expected.items()),"natural caller argument frame changed")
    check(callers==[(0x77,0x24A7,0x270C)],"aligned far caller set changed")
    image=bytearray(END);image[0x20:0x3A]=helper;image[ENTRY:END]=body
    print(f"u7f +0A01..+0A7C:124 bytes/49 instructions; one resolved +0020 call; SHA256 {DIGEST}")
    print("aligned natural caller U77 owner+24A7 at+270C; game pointer DS5D92/94")
    return bytes(image),helper_instructions+instructions

def build_c(root,directory):
    library=directory/"owner.so"
    subprocess.run(["cc","-std=c11","-Wall","-Wextra","-Wpedantic","-Werror",
                    "-shared","-fPIC","-I",str(root/"include"),
                    str(root/"src/recovered/overlay_u7f_resource_subtract_0a01.c"),
                    str(root/"src/recovered/overlay_u7f_record_subtract_0020.c"),"-o",str(library)],check=True)
    function=ctypes.CDLL(str(library)).pr_u7f_resource_subtract_0a01
    function.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    function.restype=ctypes.c_int;return function

def run_c(function,memory,cpu):
    ram=(ctypes.c_uint8*RAM_SIZE).from_buffer_copy(memory)
    status=function(ram,RAM_SIZE,ctypes.byref(cpu))
    check(status in (0,2),f"C rejected fixture: {status}")
    return bytes(ram),cpu,status

def signed_index(index):
    index&=255;return index if index<128 else index-256

def fixture(image,*,index=0,amount=1,mode=6,value=0x8000,base=0x8000,
            total=0xDEADFFFF,ss=0x2000,sp=0x8000,ds=0x1000,
            record_segment=0x4000,record_offset=0x9000,flags=0x0002,code_segment=0x3000):
    memory=bytearray([0xA5])*RAM_SIZE;cs=code_segment
    put16(memory,record_segment,(record_offset+0x88+2*signed_index(index))&65535,value)
    put16(memory,record_segment,(record_offset+0x102)&65535,base)
    acc=(0x67F4+4*signed_index(index))&65535
    put16(memory,ds,acc,total&65535);put16(memory,ds,(acc+2)&65535,total>>16)
    memory[physical(ds,0x49F3)]=mode
    for displacement,word in [(0,0x7000),(2,cs),(4,index|0xAB00),(6,amount),
                              (8,record_offset),(10,record_segment)]:
        put16(memory,ss,(sp+displacement)&65535,word)
    start=physical(cs,0x20);memory[start:start+26]=image[0x20:0x3A]
    start=physical(cs,ENTRY);memory[start:start+END-ENTRY]=image[ENTRY:END]
    cpu=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,
            0x7777,sp,cs,ENTRY,ds,0x1111,ss,flags)
    return memory,cpu

def specifications(quick=False):
    cases=[]
    indices=(0,1,61,127,128,255) if quick else range(256)
    for index in indices:
        for mode in (0,1,6,128):
            cases.append((f"index-{index}-mode-{mode}",dict(index=index,mode=mode,amount=index*257,
                         value=(0x8000-index)&65535,base=(0xFFFF-index*257)&65535)))
    for mode in ((0,1,6,128,255) if quick else range(256)):
        for index in (0,1,127,128,255):cases.append((f"mode-{mode}-{index}",dict(mode=mode,index=index)))
    rng=random.Random(0x7F0A01)
    for i in range(16 if quick else 256):
        cases.append((f"random-{i}",dict(index=rng.randrange(256),amount=rng.randrange(65536),
                     mode=rng.randrange(256),value=rng.randrange(65536),base=rng.randrange(65536),
                     total=rng.randrange(1<<32),record_offset=rng.randrange(65536))))
    # Unicorn's separately wrapped RETF CS pop is wrong at FFFE/FFFF, for
    # either the outer frame or the inner frame (outer SP minus 12).
    for sp in (0,1,2,3,4,5,6,7,8,9,12,13,14,15,0xFFF8,0xFFF9,0xFFFA,0xFFFB,0xFFFC,0xFFFD):
        cases.append((f"stack-{sp:04x}",dict(sp=sp)))
    for offset in (0,1,0xFEFD,0xFEFE,0xFEFF,0xFF00,0xFF77,0xFF78,0xFF79,0xFFFD,0xFFFE,0xFFFF):
        cases.append((f"record-{offset:04x}",dict(record_offset=offset,index=0)))
    # Both writes may alias arguments, saved BP, or either return frame.
    for target in range(0x7FE0,0x8010):
        cases.append((f"nested-alias-{target:04x}",dict(record_segment=0x2000,
                     record_offset=(target-0x102)&65535,amount=0xFFBD,index=0)))
    for target in (0x20,0x21,0x23,0x26,0x2B,0x2E,0x31,0x36,0x37,
                   0xA3B,0xA3D,0xA42,0xA44,0xA47,0xA49,0xA4B,0xA4D,0xA50,0xA51,
                   0xA53,0xA55,0xA57,0xA5B,0xA5F,0xA61,0xA63,0xA65,0xA67,0xA6A,
                   0xA6B,0xA6D,0xA6F,0xA71,0xA75,0xA79,0xA7A):
        cases.append((f"inner-return-target-{target:04x}",dict(record_segment=0x2000,
                     record_offset=0x7EF2,amount=(0xA36-target)&65535)))
    for target in (0x7FFE,0x8000,0x8002,0x8004,0x8006,0x8008,0x800A):
        cases.append((f"indexed-store-alias-{target:04x}",dict(record_segment=0x2000,
                     record_offset=(target-0x88)&65535,amount=1,index=0)))
    cases.extend([
        ("native-backed-inner-target-A79",dict(record_segment=0x2000,record_offset=0x7EF2,amount=0xFFBD)),
        ("inner-target-unaligned-A16",dict(record_segment=0x2000,record_offset=0x7EF2,amount=0x20)),
        ("A20-record",dict(record_segment=0xFFFF,record_offset=0xFF0D,index=0)),
        ("A20-stack",dict(ss=0xFFFF,sp=0x11)),
        ("shared-DS-record",dict(record_segment=0x1000)),
        ("indexed-store-mode-alias",dict(record_segment=0x1000,record_offset=0x496B,index=0,amount=1)),
        ("flags-preserved",dict(flags=0x0603,mode=0)),
    ])
    for index in (0,61,128,255):
        accumulator=(0x67F4+4*signed_index(index))&65535
        for target in (0x49F3,accumulator,(accumulator+2)&65535):
            for amount in (1,5,0xFFFF):
                for mode in (1,6):
                    cases.append((f"second-store-DS-{index}-{target:04x}-{amount}-{mode}",
                                  dict(index=index,record_segment=0x1000,
                                       record_offset=(target-0x102)&65535,
                                       amount=amount,mode=mode,total=0xFFFFFFFF)))
    for cs in (0x5000,0xFFFF):
        for index in (0,61,128,255):
            for mode in (0,1,6):
                cases.append((f"relocated-{cs:04x}-{index}-{mode}",
                              dict(code_segment=cs,index=index,mode=mode)))
    return cases

def differential(image,decoded,function,quick=False):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
    from unicorn import x86_const as regs
    registers=[getattr(regs,"UC_X86_REG_"+("EFLAGS" if n=="flags" else n.upper())) for n,_ in Cpu._fields_]
    backing=ctypes.create_string_buffer(RAM_SIZE);original=Uc(UC_ARCH_X86,UC_MODE_16)
    original.mem_map_ptr(0,RAM_SIZE,7,ctypes.addressof(backing))
    original.mem_map_ptr(RAM_SIZE,RAM_SIZE,7,ctypes.addressof(backing))
    starts={i.address for i in decoded};mnemonics={i.address:i.mnemonic for i in decoded}
    visited=set();edges=set();completed=0;nonterminating=[];undefined_af=0
    returns=0;exits=[]
    for label,options in specifications(quick):
        memory,cpu=fixture(image,**options);ctypes.memmove(backing,bytes(memory),RAM_SIZE)
        for (name,_),register in zip(Cpu._fields_,registers):original.reg_write(register,getattr(cpu,name))
        code_segment=cpu.cs;state={"outer_return":False,"stopped":False,"previous":None,"last_flags":None}
        def instruction(emulator,address,size,user):
            del address,size,user
            here=emulator.reg_read(regs.UC_X86_REG_IP);segment=emulator.reg_read(regs.UC_X86_REG_CS)
            if state["outer_return"] or segment!=code_segment or here not in starts:
                state["stopped"]=True;emulator.emu_stop();return
            visited.add(here)
            if state["previous"] is not None:edges.add((state["previous"],here))
            state["previous"]=here
            if mnemonics[here] in ("add","adc","sub","cmp","xor","shl"):
                state["last_flags"]=mnemonics[here]
            if here==0xA7A:state["outer_return"]=True
        hook=original.hook_add(UC_HOOK_CODE,instruction)
        original.emu_start(physical(cpu.cs,ENTRY),2*RAM_SIZE,count=20000)
        original.hook_del(hook)
        if not state["stopped"]:
            nonterminating.append(label);continue
        expected=bytes(backing)
        for entry,end in ((0x20,0x3A),(ENTRY,END)):
            at=physical(code_segment,entry)
            check(expected[at:at+end-entry]==image[entry:end],f"fixture changed original code: {label}")
        actual,cpu,status=run_c(function,memory,cpu)
        check(status==(0 if state["outer_return"] else 2),f"transfer status mismatch {label}")
        if actual!=expected:
            offsets=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
            raise ValueError(f"whole RAM mismatch {label}: {offsets[:12]}")
        for (name,_),register in zip(Cpu._fields_,registers):
            value=original.reg_read(register)&65535
            if name=="flags" and state["last_flags"]=="shl":
                # SHL AF is undefined. Unicorn clears it; DOSBox-X normal 386
                # sets it in the native final-SHL cases. Keep this sole
                # one-bit exception visible; native checks the full word.
                check((getattr(cpu,name)&~0x10)==(value&~0x10),f"{label}/defined SHL flags mismatch")
                check(cpu.flags&0x10 and not value&0x10,f"{label}/SHL backend AF behavior changed")
                undefined_af+=1;continue
            check(getattr(cpu,name)==value,f"{label}/{name}: C={getattr(cpu,name):04x},original={value:04x}")
        completed+=1
        if status==0:returns+=1
        else:exits.append({"case":label,"cs":cpu.cs,"ip":cpu.ip,"sp":cpu.sp})
    check(visited==starts,f"missing instruction starts: {sorted(starts-visited)}")
    check(all(edge in edges for edge in ((0xA3B,0xA44),(0xA3B,0xA3D),(0xA42,0xA44),(0xA42,0xA79))),
          "missing conditional branch edge")
    print(f"{completed} original-byte cases:whole 1 MiB/all14 CPU fields; {len(visited)}/{len(starts)} starts; both edges of both conditionals")
    print(f"Sole Unicorn exception: architecturally undefined final-SHL AF bit in {undefined_af} cases; native checks full FLAGS with no mask")
    check(not nonterminating,f"nonterminating alias fixtures need separate contract review: {nonterminating}")
    print(f"Returns {returns}; control exits {len(exits)}; exact full CPU {completed-undefined_af}; no exclusions")
    return {"cases":completed,"returns":returns,"control_exits":exits,
            "exact_cpu_cases":completed-undefined_af,"final_shl_af_exceptions":undefined_af,
            "instruction_starts":len(visited),"conditional_outcomes":4,"budget_exclusions":[]}

def main():
    root=Path(__file__).resolve().parents[2]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn",action="store_true");parser.add_argument("--native",action="store_true")
    parser.add_argument("--quick",action="store_true")
    parser.add_argument("--exe",type=Path);parser.add_argument("--ovr",type=Path)
    parser.add_argument("--report",type=Path)
    args=parser.parse_args()
    image,decoded=source_audit(root,args.exe,args.ovr)
    report={"owner_sha256":DIGEST,"exe_sha256":EXE_SHA,"ovr_sha256":OVR_SHA}
    with tempfile.TemporaryDirectory(prefix="poolrad-u7f-owner-") as temporary:
        function=build_c(root,Path(temporary))
        if args.unicorn:report["unicorn"]=differential(image,decoded,function,args.quick)
        if args.native:
            from u7f_resource_subtract_0a01_native import native_audit
            report["native"]=native_audit(root,image,function)
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+"\n")

if __name__=="__main__":main()
