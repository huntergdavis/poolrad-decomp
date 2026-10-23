#!/usr/bin/env python3
"""Hash-locked original U86 +028D with raw SYSTEM allocator/string chain."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
import system_getmem_023f_audit as gm
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
from pinned_shift_flags import LastFlagWriter,SPLIT_BY_RULE,declared_flags_split,split_used
SPLIT_ARRIVALS=[0]
CS,DS,SS,ENTRY,END=0x8000,gm.DS,0x2800,0x028D,0x044E
RET_CS,RET_IP=0x9001,0x7000
OK,CODE,CALLEE,TRANSFER,LIST_LIMIT=0,2,3,4,5
BODY_SHA='5f82bdbca3270a4d7dcf6a93f41fa14debd04f8f4a21d8860a9b4a64d2cd5038'
SITES={0x02AE:0x23F,0x031F:0x23F,0x038B:0xB25,0x039C:0x23F,0x03DE:0xB25,0x03F3:0xB25,0x0403:0xB0B,0x041C:0xB98,0x042A:0xB25}
KINDS={0x23F:0,0xB25:1,0xB0B:2,0xB98:3}
check=gm.check
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('status',ctypes.c_int)]
SOURCES=['overlay_u86_string_node_028d.c','system_getmem_023f_cpu.c','system_strstore_fullstate_0b25.c','system_string_leaves_cpu.c']

def source():
    image,alloc,_,_=gm.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest()==OVR_SHA,'GAME.OVR identity differs')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x86)
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];md=Cs(CS_ARCH_X86,CS_MODE_16)
    body=code[ENTRY:END];caller=list(md.disasm(body,ENTRY))
    check(len(body)==449 and len(caller)==165 and sha256(body).hexdigest()==BODY_SHA,'body differs')
    check(caller[-1].address==0x044B and caller[-1].mnemonic=='retf' and caller[-1].op_str=='2','exit differs')
    check([(i.address,i.op_str) for i in caller if i.mnemonic=='lcall']==[(s,f'0xaf8, {t:#x}') for s,t in SITES.items()],'calls differ')
    check(all(s+3 in u['fixups'] for s in SITES),'call fixups differ')
    refs=[]
    for f in u['functions']:
        ins=list(md.disasm(code[f['rel']:f['rel']+f['size_upper_bound']],f['rel']))
        for j,i in enumerate(ins):
            if i.mnemonic=='call' and i.op_str=='0x28d':
                check(j>0 and ins[j-1].mnemonic=='push' and ins[j-1].op_str=='cs','missing PUSH CS');refs.append((f['rel'],i.address))
    check(refs==[(0x44E,0x4EE),(0x59C,0x60E),(0x59C,0x64A),(0x59C,0x67A),(0x59C,0x70A)],f'incoming calls differ: {refs}')
    decoded={(CS,i.address):i for i in caller};decoded.update({(gm.SYSTEM_CS,a):i for a,i in alloc.items()})
    for lo,hi in ((0xB0B,0xB25),(0xB25,0xB49),(0xB98,0xBC4)):
        data=image[physical(gm.SYSTEM_CS,lo):physical(gm.SYSTEM_CS,hi)]
        decoded.update({(gm.SYSTEM_CS,i.address):i for i in md.disasm(data,lo)})
    prefixes=code[0x287:ENTRY]
    check(len(prefixes)==6 and prefixes[0]==2 and prefixes[3]==2,'prefix layout differs')
    target=dict(bytes=449,instructions=165,sha256=BODY_SHA,file_start=u['ovr_offset']+ENTRY,file_end=u['ovr_offset']+END,
                callers=refs,prefix_sha256=sha256(prefixes).hexdigest())
    print('U86 +028D:',target,flush=True);return image,code,decoded,target

CHECKER_ROWS = ('OVL_0086_028D',)


def library(directory):
    path=directory/'u86028d.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),
        *[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u86_string_node_028d
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.c_uint,ctypes.POINTER(Diagnostic)]
    f.restype=ctypes.c_int;return f

def build(image,code,rng,*,argument=3,length=0,node_flags=None,group=1,old_group=0,name_length=12,heading_length=20,
          blocks=(),top=(0x3400,0),end=(0x3600,0),heap_error=(0xA9,gm.SYSTEM_CS),sp=0x8000,ss=SS,
          flags=0x0602,background=None,writes=(),cycle=False):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background);m[:len(image)]=image
    at=physical(CS,0);m[at:at+len(code)]=code
    # Only the selected Pascal strings need initialization; the rest remains live RAM.
    item=argument&127;group_off=(0xA79+41*(group if group<128 else group-256))&65535
    for off,n in ((group_off,heading_length),(0x28E9+41*item,name_length)):
        m[physical(DS,off)]=n
        for j in range(1,n+1):m[physical(DS,(off+j)&65535)]=(j*29+item)&255
    for j in range(256):m[physical(DS,0x31FB+16*j)]=old_group
    m[physical(DS,0x31FB+16*item)]=group
    gm.heap(m,blocks=blocks,top=top,end=end,heap_error=heap_error)
    flags_list=node_flags if node_flags is not None else [j%2 for j in range(length)]
    check(len(flags_list)==length,'bad node flags')
    for j,flag in enumerate(flags_list):
        off=j*48;more=j+1<length or cycle;nxt=((j+1)%length)*48
        m[physical(0x5000,off)]=1;m[physical(0x5000,off+1)]=0x41;m[physical(0x5000,off+0x29)]=flag
        put16(m,0x5000,off+0x2A,nxt if more else 0);put16(m,0x5000,off+0x2C,0x5000 if more else 0)
    put16(m,DS,0x4766,0);put16(m,DS,0x4768,0x5000 if length else 0)
    count=(1+sum(f==0 for f in flags_list[:-1]))&255 if length else 0
    if length:
        previous=(count if count<128 else count-256)-1
        history=(0x476A+previous)&65535
        if not 0x4766<=history<=0x4769:m[physical(DS,history)]=126 if item!=126 else 125
    for j,v in enumerate((RET_IP,RET_CS,argument)):put16(m,ss,sp+2*j,v)
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    regs=[rng.randrange(65536) for _ in range(7)]
    return m,Cpu(*regs,sp,CS,ENTRY,DS,rng.randrange(65536),ss,flags),ss>=0xF000

def compare(label, memory, cpu, high, decoded, function, cap=64,list_cap=400):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2*SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name,_), reg in zip(Cpu._fields_, REGISTERS): original.reg_write(reg,getattr(cpu,name))
    state = dict(stop=None, callee=-1, returned=False, returning=None, calls=[], previous=None, steps=0, list_visits=0, kind=-1, unexpected=None)
    visited, edges = set(), set()
    writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

    def hook(uc, address, size, user):
        del size,user
        cs, ip = uc.reg_read(UC_X86_REG_CS), uc.reg_read(UC_X86_REG_IP)
        here=(cs,ip)
        if state["returned"]:
            state["stop"]=OK; return uc.emu_stop()
        if state["returning"] is not None:
            ret_site=state["returning"];state["returning"]=None
            want=state["calls"].pop()
            if here != want:
                direct = ret_site in ((gm.SYSTEM_CS,0x024B),(gm.SYSTEM_CS,0x0B46),(gm.SYSTEM_CS,0x0B22),(gm.SYSTEM_CS,0x0BC1))
                state["stop"]=TRANSFER if direct else CALLEE
                state["callee"]=0 if direct else gm.TRANSFER
                return uc.emu_stop()
        if here==(gm.SYSTEM_CS,gm.RUNERROR_IP):
            state["stop"]=CALLEE;state["callee"]=gm.RUNERROR;return uc.emu_stop()
        if here not in decoded:
            state["unexpected"]=here;return uc.emu_stop()
        if cs==gm.SYSTEM_CS and ip in gm.HEAP_ERROR_SITES:
            ptr=bytes(uc.mem_read(physical(cpu.ds,0x4414),4))
            if ptr != struct.pack("<HH",0xA9,gm.SYSTEM_CS):
                state["stop"]=CALLEE;state["callee"]=gm.HEAP_ERROR;return uc.emu_stop()
        if here==(gm.SYSTEM_CS,gm.WALK_HEAD) and state["previous"] in [(gm.SYSTEM_CS,p) for p in gm.WALK_JAS]:
            state["steps"]+=1
            if state["steps"]>cap:
                state["stop"]=CALLEE;state["callee"]=gm.LIMIT;return uc.emu_stop()
        if cs==CS and ip in SITES:
            raw=bytes(uc.mem_read(physical(cs,ip),3))
            if raw!=struct.pack('<BH',0x9A,SITES[ip]):
                state['stop']=CODE;return uc.emu_stop()
            state['kind']=KINDS[SITES[ip]];state['callee']=0
        if here==(gm.SYSTEM_CS,0x23F):state['steps']=0
        if here==(CS,0x02D4):
            state['list_visits']+=1
            if state['list_visits']>list_cap:
                state['stop']=LIST_LIMIT;return uc.emu_stop()
        ins=decoded[here];visited.add(here)
        prev=decoded.get(state["previous"])
        if prev and prev.mnemonic.startswith("j") and prev.mnemonic!="jmp":
            edges.add((state["previous"],here))
        if ins.mnemonic in ("call","lcall"):state["calls"].append((cs,ip+ins.size))
        elif ins.mnemonic in ("ret","retf"):
            if here==(CS,0x044B):state["returned"]=True
            else:state["returning"]=here
        writer.step(uc,address)
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
    writer.finish(original)
    expected=bytes(original.mem_read(0,SIZE))
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);callee=Diagnostic()
    status=function(ram,SIZE,ctypes.byref(cpu),list_cap,cap,ctypes.byref(callee))
    check(status==state["stop"] and callee.status==state["callee"],f"status C={status}/{callee.status}, original={state['stop']}/{state['callee']}: {label}")
    actual=bytes(ram)
    if actual!=expected:
        diff=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(p),actual[p],expected[p]) for p in diff[:8]]}, {len(diff)} bytes")
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        want=original.reg_read(reg)&65535
        if name=="flags":
            problem=declared_flags_split(cpu.flags,want,writer);check(problem is None,f"flags: {problem}: {label}")
            SPLIT_ARRIVALS[0]+=split_used(cpu.flags,want,writer);continue
        check(getattr(cpu,name)==want,f"{name}: C={getattr(cpu,name):04x}, original={want:04x}: {label}")
    check(callee.callee==state['kind'],f'callee identity mismatch {label}')
    return status,callee.status,visited,edges


def cases():
    rng=random.Random(0x86028D);spec=[]
    for arg in range(256):
        blocks,top,end=gm.random_heap(rng)
        spec.append((f'argument-{arg}',dict(argument=arg,length=arg%5,group=arg%8,old_group=(arg%8 if arg%3 else 0),
            name_length=(0,1,38,39,40,41,100,254,255)[arg%9],heading_length=rng.randrange(256),
            blocks=blocks,top=top,end=end,flags=gm.FLAGS[arg%len(gm.FLAGS)])))
    for length in (1,2,127,128,129,254,255,256,257):
        spec.append((f'list-count-{length}',dict(length=length,node_flags=[0]*length)))
    for group in (0,1,127,128,129,255):spec.append((f'group-{group}',dict(group=group,length=2,old_group=0)))
    spec += [
        ('nil-same-group',dict(group=0)),('list-same-group',dict(group=2,old_group=2,length=5)),
        ('cycle',dict(length=3,cycle=True,list_cap=8)),('list-budget',dict(length=3,list_cap=2)),
        ('first-allocation-full',dict(end=(0x3400,0))),
        ('second-allocation-full',dict(end=(0x3403,0))),
        ('old-list-allocation-full',dict(length=3,end=(0x3400,0))),
        ('custom-heap',dict(heap_error=(0x1234,0x5678))),
        ('heap-budget',dict(blocks=((0x3010,0,1,0),(0x3020,0,3,0)),cap=0)),
        ('heap-cycle',dict(blocks=((0x3010,0,1,0),),writes=((0x3010,0,struct.pack('<HH',0,0x3010)),),cap=3)),
        ('exact-free',dict(blocks=((0x3010,0,3,0),(0x3020,0,3,0)))),
        ('split-free',dict(blocks=((0x3010,8,8,8),))),
        ('random-background',dict(background=rng.randbytes(SIZE))),
        ('ss-ds',dict(ss=DS,length=3)),('a20-stack',dict(ss=0xFFFF)),
    ]
    for sp in (0x140,0x141,0x8001,0xFFF8,0xFFFA):spec.append((f'sp-{sp:x}',dict(sp=sp)))
    for site in SITES:
        spec.append((f'code-{site:x}',dict(length=1 if site==0x31F else 0,argument=0x83 if site==0x3DE else 3,writes=((CS,site,b'\x90'),))))
    # A new node at the caller's local pointer, and a node over string-call return frames.
    for off in (0x8000-6,0x8000-0x116,0x8000-0x112):
        seg=SS+off//16;tail=off%16
        spec.append((f'heap-stack-alias-{off:x}',dict(top=(seg,tail),end=(seg+0x100,0),group=0)))
    return spec

def differential(image,code,decoded,function):
    visited,edges,stops=set(),set(),{};rng=random.Random(0x86028D)
    for label,options in cases():
        cap=options.pop('cap',64);list_cap=options.pop('list_cap',400)
        memory,cpu,high=build(image,code,rng,**options)
        status,callee,v,e=compare(label,memory,cpu,high,decoded,function,cap,list_cap)
        visited|=v;edges|=e;key=f'{status}/{callee}';stops[key]=stops.get(key,0)+1
        if not label.startswith('argument'):print(label,key,flush=True)
    starts={p for p in decoded if p[0]==CS};caller_edges={e for e in edges if e[0][0]==CS}
    check(starts<=visited,f'caller starts missing: {starts-visited}')
    caller_total=2*sum(i.mnemonic.startswith('j') and i.mnemonic!='jmp' for p,i in decoded.items() if p[0]==CS)
    check(len(caller_edges)==caller_total,f'caller outcomes {len(caller_edges)}/{caller_total}')
    report=dict(cases=sum(stops.values()),stops=stops,declared_flag_split_arrivals=SPLIT_ARRIVALS[0],declared_flag_split_by_rule=dict(SPLIT_BY_RULE),caller_starts=len(starts),caller_total=len(starts),caller_outcomes=len(caller_edges),caller_outcomes_total=caller_total,
        chain_starts=len(visited),chain_total=len(decoded),chain_outcomes=len(edges),chain_outcomes_total=2*sum(i.mnemonic.startswith('j') and i.mnemonic!='jmp' for i in decoded.values()))
    print('Original-chain:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    image,code,decoded,target=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='u86-028d-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code,decoded,f)
        if a.native:
            from u86_string_node_028d_native import native_audit
            report['native']=native_audit(image,code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
