#!/usr/bin/env python3
"""Original-chain CPU14/full-RAM audit for U6A +0E37 (executable 135B/48ins).
Uses original GetMem and StrStore, never allocation/copy stubs.
The queue's 389B upper bound includes 254 trailing data bytes.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
import system_getmem_023f_audit as gm
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CS, DS, SS, ENTRY, END = 0x6000, gm.DS, 0x2000, 0x0E37, 0x0EBE
RET_CS, RET_IP = 0x9001, 0x7000
OK, ARGUMENT, GETMEM_STOP, STRSTORE_STOP, TRANSFER = range(5)
BODY_SHA = "5f63f1823287894ede57fcef122307f3e296293a633e530a1836f2f36d389234"
check = gm.check


def source():
    image, alloc, _, digests = gm.source()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, "GAME.OVR identity differs")
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    u = next(u for u in units if u["stub_para"] == 0x6A)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    body = code[ENTRY:END]; caller = list(md.disasm(body, ENTRY))
    check(len(body) == 135 and len(caller) == 48 and sha256(body).hexdigest() == BODY_SHA, "caller identity differs")
    check(caller[-1].address == 0x0EBB and caller[-1].mnemonic == "retf" and caller[-1].op_str == "8", "exit differs")
    check(not any(i.mnemonic.startswith("j") for i in caller), "unexpected caller branch")
    calls = [(i.address, i.op_str) for i in caller if i.mnemonic == "lcall"]
    check(calls == [(0x0E4B, "0xaf8, 0xb25"), (0x0E54, "0xaf8, 0x23f"), (0x0EA5, "0xaf8, 0xb25")], "calls differ")
    check(all(a + 3 in u["fixups"] for a, _ in calls), "far call lacks fixup")
    refs = []
    for f in u["functions"]:
        start = f["rel"]
        ins = list(md.disasm(code[start:start + f["size_upper_bound"]], start))
        for n, i in enumerate(ins):
            if i.mnemonic == "call" and i.op_str == "0xe37":
                check(n > 0 and ins[n-1].mnemonic == "push" and ins[n-1].op_str == "cs", "caller lacks PUSH CS")
                refs.append((start, i.address))
    check(refs == [(0x0FBC,0x102C),(0x0FBC,0x12DB),(0x0FBC,0x130C),(0x0FBC,0x1323)], f"aligned incoming calls differ: {refs}")
    decoded = {(CS, i.address): i for i in caller}
    decoded.update({(gm.SYSTEM_CS, a): i for a, i in alloc.items()})
    move = image[physical(gm.SYSTEM_CS, 0x0B25):physical(gm.SYSTEM_CS, 0x0B49)]
    check(sha256(move).hexdigest() == "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118", "StrStore differs")
    decoded.update({(gm.SYSTEM_CS, i.address): i for i in md.disasm(move, 0x0B25)})
    provenance = dict(bytes=135, instructions=48, sha256=BODY_SHA, file_start=u["ovr_offset"]+ENTRY,
                      file_end=u["ovr_offset"]+END, callers=refs, queue_upper_bytes=389, trailing_data_bytes=254)
    print("U6A +0E37:", provenance, flush=True)
    return image, code, decoded, provenance


def library(directory):
    path = directory / "u6a0e37.so"
    sources = ["overlay_u6a_string_node_0e37.c", "system_getmem_023f_cpu.c", "system_strstore_fullstate_0b25.c"]
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-shared", "-fPIC",
                    "-I", str(ROOT / "include"), *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)],
                   check=True, capture_output=True, text=True)
    f = ctypes.CDLL(str(path)).pr_u6a_string_node_0e37
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.c_uint,
                  ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def build(image, code, rng, *, length=40, attr=0xA55A, blocks=(), top=(0x3200, 0), end=(0x3300, 0),
          heap_error=(0x00A9, gm.SYSTEM_CS), sp=0x8000, ss=SS, flags=0x0602,
          source_ptr=(DS, 0x1000), parent=0x9000, old_node=(0x3380, 0x0100),
          background=None, writes=()):
    memory = bytearray([0xA5])*SIZE if background is None else bytearray(background)
    memory[:len(image)] = image
    at = physical(CS, 0); memory[at:at+len(code)] = code
    gm.heap(memory, blocks=blocks, top=top, end=end, heap_error=heap_error)
    for j in range(256):
        memory[physical(source_ptr[0], (source_ptr[1]+j)&65535)] = rng.randrange(256)
    memory[physical(*source_ptr)] = length
    put16(memory, ss, (parent-12)&65535, old_node[1])
    put16(memory, ss, (parent-10)&65535, old_node[0])
    for j, v in enumerate((RET_IP, RET_CS, parent, source_ptr[1], source_ptr[0], attr)):
        put16(memory, ss, (sp+2*j)&65535, v)
    for seg, off, blob in writes:
        for j,v in enumerate(blob): memory[physical(seg,(off+j)&65535)] = v
    regs = [rng.randrange(65536) for _ in range(7)]
    return memory, Cpu(*regs, sp, CS, ENTRY, DS, rng.randrange(65536), ss, flags), ss >= 0xF000 or source_ptr[0] >= 0xF000


def compare(label, memory, cpu, high, decoded, function, cap=64):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2*SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name,_), reg in zip(Cpu._fields_, REGISTERS): original.reg_write(reg,getattr(cpu,name))
    state = dict(stop=None, callee=0, returned=False, returning=None, calls=[], previous=None, steps=0, unexpected=None)
    visited, edges = set(), set()

    def hook(uc, address, size, user):
        del address,size,user
        cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
        here=(cs,ip)
        if state["returned"]:
            state["stop"]=OK; return uc.emu_stop()
        if state["returning"] is not None:
            ret_site=state["returning"];state["returning"]=None
            want=state["calls"].pop()
            if here != want:
                direct = ret_site in ((gm.SYSTEM_CS,0x024B),(gm.SYSTEM_CS,0x0B46))
                state["stop"]=TRANSFER if direct else GETMEM_STOP
                state["callee"]=0 if direct else gm.TRANSFER
                return uc.emu_stop()
        if here==(gm.SYSTEM_CS,gm.RUNERROR_IP):
            state["stop"]=GETMEM_STOP;state["callee"]=gm.RUNERROR;return uc.emu_stop()
        if here not in decoded:
            state["unexpected"]=here;return uc.emu_stop()
        if cs==gm.SYSTEM_CS and ip in gm.HEAP_ERROR_SITES:
            ptr=bytes(uc.mem_read(physical(cpu.ds,0x4414),4))
            if ptr != struct.pack("<HH",0xA9,gm.SYSTEM_CS):
                state["stop"]=GETMEM_STOP;state["callee"]=gm.HEAP_ERROR;return uc.emu_stop()
        if here==(gm.SYSTEM_CS,gm.WALK_HEAD) and state["previous"] in [(gm.SYSTEM_CS,p) for p in gm.WALK_JAS]:
            state["steps"]+=1
            if state["steps"]>cap:
                state["stop"]=GETMEM_STOP;state["callee"]=gm.LIMIT;return uc.emu_stop()
        ins=decoded[here];visited.add(here)
        prev=decoded.get(state["previous"])
        if prev and prev.mnemonic.startswith("j") and prev.mnemonic!="jmp":
            edges.add((state["previous"],here))
        if ins.mnemonic in ("call","lcall"):state["calls"].append((cs,ip+ins.size))
        elif ins.mnemonic in ("ret","retf"):
            if here==(CS,0x0EBB):state["returned"]=True
            else:state["returning"]=here
        state["previous"]=here

    def mirror(uc,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1)
            uc.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))

    original.hook_add(UC_HOOK_CODE,hook)
    if high:original.hook_add(UC_HOOK_MEM_WRITE,mirror)
    original.emu_start(physical(CS,ENTRY),2*SIZE,count=2000000)
    check(state["unexpected"] is None and state["stop"] is not None,f"original escape/timeout {state}: {label}")
    expected=bytes(original.mem_read(0,SIZE))
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);callee=ctypes.c_int(-1)
    status=function(ram,SIZE,ctypes.byref(cpu),cap,ctypes.byref(callee))
    check(status==state["stop"] and callee.value==state["callee"],f"status C={status}/{callee.value}, original={state['stop']}/{state['callee']}: {label}")
    actual=bytes(ram)
    if actual!=expected:
        diff=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(p),actual[p],expected[p]) for p in diff[:8]]}, {len(diff)} bytes")
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        want=original.reg_read(reg)&65535
        check(getattr(cpu,name)==want,f"{name}: C={getattr(cpu,name):04x}, original={want:04x}: {label}")
    return status,callee.value,visited,edges


def cases():
    rng=random.Random(0x6A0E37);spec=[]
    for i in range(256):
        blocks,top,end=gm.random_heap(rng)
        spec.append((f"length-{i}",dict(length=i,attr=rng.randrange(65536),blocks=blocks,top=top,end=end,
                     flags=gm.FLAGS[i%len(gm.FLAGS)])))
    for name,options in [
        ("exact",dict(blocks=((0x3010,0,3,0),))),
        ("second-exact",dict(blocks=((0x3010,0,1,0),(0x3020,0,3,0)))),
        ("split",dict(blocks=((0x3010,8,4,8),))),
        ("top",dict()),
        ("full",dict(end=(0x3200,0))),
        ("custom-top",dict(heap_error=(0x1234,0x5678))),
        ("custom-full",dict(end=(0x3200,0),heap_error=(0x00A9,0x0AF9))),
        ("cyclic",dict(blocks=((0x3010,0,1,0),),writes=((0x3010,0,struct.pack("<HH",0,0x3010)),),cap=3)),
        ("cap-short",dict(blocks=((0x3010,0,1,0),(0x3020,0,3,0)),cap=0)),
        ("cap-exact",dict(blocks=((0x3010,0,1,0),(0x3020,0,3,0)),cap=1)),
    ]: spec.append((name,options))
    for sp in (0x0060,0x0061,0x0100,0xFFF0,0xFFF4):
        spec.append((f"sp-{sp:04x}",dict(sp=sp)))
    spec += [
        ("ss-ds",dict(ss=DS)),
        ("ss-a20",dict(ss=0xFFFF,sp=0x8000)),
        ("source-wrap",dict(source_ptr=(0x5000,0xFFF0),length=255)),
        ("source-a20",dict(source_ptr=(0xFFFF,0xFFF0),length=255)),
        ("source-is-destination",dict(source_ptr=(SS,0x8000-0x2B),length=40)),
        ("source-overlap-forward",dict(source_ptr=(SS,0x8000-0x2C),length=40)),
        ("source-overlap-backward",dict(source_ptr=(SS,0x8000-0x2A),length=40)),
        ("old-node-is-allocation",dict(old_node=(0x3200,0))),
        ("random-background",dict(background=bytes(random.Random(21).randbytes(SIZE)))),
    ]
    for sp in (0x8002,0x800A):
        seg=SS+(sp-0x3A)//16;off=(sp-0x3A)%16
        spec.append((f"allocation-return-alias-{sp:04x}",dict(sp=sp,top=(seg,off),end=(seg+0x100,0),length=40)))
    return spec


def differential(image,code,decoded,function):
    visited,edges,stops=set(),set(),{}
    rng=random.Random(0x6A0E37)
    for label,options in cases():
        cap=options.pop("cap",64)
        memory,cpu,high=build(image,code,rng,**options)
        status,callee,v,e=compare(label,memory,cpu,high,decoded,function,cap)
        visited|=v;edges|=e;key=f"{status}/{callee}";stops[key]=stops.get(key,0)+1
        if not label.startswith("length"): print(label,key,flush=True)
    starts={p for p in decoded if p[0]==CS}
    check(starts<=visited,f"caller starts missing: {starts-visited}")
    report=dict(cases=sum(stops.values()),stops=stops,caller_starts=len(starts),caller_total=len(starts),
                chain_starts=len(visited),chain_total=len(decoded),chain_outcomes=len(edges),
                chain_outcomes_total=2*sum(i.mnemonic.startswith("j") and i.mnemonic!="jmp" for i in decoded.values()))
    print("Original-chain:",report,flush=True)
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn",action="store_true");parser.add_argument("--native",action="store_true")
    parser.add_argument("--output",type=Path);args=parser.parse_args()
    image,code,decoded,provenance=source();report=dict(target=provenance)
    with tempfile.TemporaryDirectory(prefix="u6a-0e37-") as temp:
        function=library(Path(temp))
        if args.unicorn:report["unicorn"]=differential(image,code,decoded,function)
        if args.native:
            from u6a_string_node_0e37_native import native_audit
            report["native"]=native_audit(image,code,function)
    if args.output:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+"\n")
    print(json.dumps(report,sort_keys=True))

if __name__=="__main__":main()
