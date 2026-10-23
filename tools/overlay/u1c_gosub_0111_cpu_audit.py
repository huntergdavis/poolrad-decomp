#!/usr/bin/env python3
"""Hash-locked original-chain U1C +0111 CPU14/1MiB differential.
Original media and generated native captures remain in ignored resources/.
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
from resident_loader_map import unpack
import u1c_goto_00f4_cpu_audit as old
import system_getmem_023f_audit as gm
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY,END=0x0111,0x0126
CS,U25_CS,STUB_CS,SYSTEM_CS,DS,SS=old.CALLER_CS,old.U25_CS,old.STUB_SEG,old.SYSTEM_CS,old.DS,old.SS
SITES={0x0117:(0x2A,0),0x011F:(0x75,0x1951)}
KINDS={0:0,0x1951:1}
BODY_SHA='7917b6be7b4ae2f3d53de4c2a9d80ec430fba207ca34e689e88ac2186c4b0edc'
check=old.check
NAMES={0:'return',2:'CODE',3:'CALLEE',4:'TRANSFER'}
class Limits(ctypes.Structure):
    _fields_=[('system_cs',ctypes.c_uint16),('max_operands',ctypes.c_size_t),('max_string_iterations',ctypes.c_size_t),('max_heap_steps',ctypes.c_uint)]
class Diagnostic(ctypes.Structure):
    _fields_=[(s,ctypes.c_int) for s in ('callee','status','nested_callee','nested_status')]
SOURCES=['overlay_u1c_gosub_0111_cpu.c','overlay_u25_loader_0000.c','overlay_u25_string_cores.c','overlay_u25_alloc_1951_cpu.c','system_getmem_023f_cpu.c']

def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media provenance differs')
    image=bytes(unpack(exe)[0]);units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)}
    a,b=units[0x1C],units[0x25]
    caller=ovr[a['ovr_offset']:a['ovr_offset']+a['code_size']];callee=ovr[b['ovr_offset']:b['ovr_offset']+b['code_size']]
    body=caller[ENTRY:END];decoded=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY))
    check(len(body)==21 and sha256(body).hexdigest()==BODY_SHA and decoded[-1].address==0x0125 and decoded[-1].mnemonic=='retf','body differs')
    check(sum(i.size for i in decoded)==21 and [(i.address,i.op_str) for i in decoded if i.mnemonic=='lcall']==[(s,f'0x2b, {off:#x}') for s,(off,_) in SITES.items()],'calls differ')
    check(all(s+3 in a['fixups'] for s in SITES),'caller fixups differ')
    target=dict(bytes=21,instructions=len(decoded),sha256=BODY_SHA,file_start=a['ovr_offset']+ENTRY,file_end=a['ovr_offset']+END,
                queue_upper_bytes=21,trailing_data_bytes=0,u25_sha256=sha256(callee).hexdigest())
    print('U1C +0111:',target,flush=True)
    return target,caller,callee,image

CHECKER_ROWS = ('OVL_001C_0111',)


def library(directory):
    path=directory/'u1c0111.so'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include'),
                    *[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u1c_gosub_0111_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    f.restype=ctypes.c_int;return f

def build(caller,callee,image,rng,*,tag=2,low=0x43,high=0x90,ops=None,old_head=(0,0),blocks=(),
          top=(0x3400,0),end=(0x3600,0),heap_error=(0xA9,SYSTEM_CS),writes=(),**kwargs):
    spec=[[tag,low,high]] if ops is None else ops
    memory,cpu,a20=old.build(caller,callee,image,rng,dict(operands=spec),**kwargs)
    cpu.ip=ENTRY
    for offset,target in set(SITES.values()):
        at=physical(STUB_CS,offset);memory[at:at+5]=struct.pack('<BHH',0xEA,target,U25_CS)
    gm.heap(memory,blocks=blocks,top=top,end=end,heap_error=heap_error)
    put16(memory,cpu.ds,0x6F70,old_head[1]);put16(memory,cpu.ds,0x6F72,old_head[0])
    for seg,off,blob in writes:
        for j,v in enumerate(blob):memory[physical(seg,(off+j)&65535)]=v
    return memory,cpu,a20

def compare(label,memory,cpu,high,function,*,operands_cap=300,strings_cap=4000,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE)
    uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (name,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,name))
    state=dict(stop=None,returned=False,returning=None,calls=[],previous=None,operand_visits=0,caller_visits=0,heap_steps=0,string_visits=0,callee=-1,reason=None)
    visited,edges=set(),set();md=Cs(CS_ARCH_X86,CS_MODE_16);cache={}
    def halt(status,reason):state['stop']=status;state['reason']=reason;uc.emu_stop()
    def hook(em,address,size,user):
        del em,user
        here=(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP));cs,ip=here
        if state['returned']:return halt(0,'return')
        if state['returning'] is not None:
            expected,direct=state['returning'];state['returning']=None
            if here!=expected:return halt(4 if direct else 3,'changed return')
        if here==(SYSTEM_CS,gm.RUNERROR_IP):return halt(3,'RunError203')
        check((cs==CS and ENTRY<=ip<END) or cs in (U25_CS,SYSTEM_CS) or (cs==STUB_CS and ip in {v[0] for v in SITES.values()}),f'original escape {here}: {label}')
        if cs==CS and ip in SITES:
            off,target=SITES[ip];raw=bytes(uc.mem_read(address,5));stub=physical(int.from_bytes(raw[3:5],'little'),off)
            if raw[:3]!=struct.pack('<BH',0x9A,off) or bytes(uc.mem_read(stub,3))!=struct.pack('<BH',0xEA,target):return halt(2,'call binding')
            state['callee']=KINDS[target]
            if target==0:state['operand_visits']=0
        if here==(U25_CS,0x22):
            state['operand_visits']+=1
            if state['operand_visits']>operands_cap:return halt(3,'operand budget')
        if cs==U25_CS and ip in (0x1153,0x12FB):state['string_visits']=0
        if cs==U25_CS and ip in (0x1172,0x1327,0x13D3,0x1453,0x14D2):
            state['string_visits']+=1
            if state['string_visits']>strings_cap:return halt(3,'string budget')
        if cs==SYSTEM_CS and ip in gm.HEAP_ERROR_SITES:
            if bytes(uc.mem_read(physical(cpu.ds,0x4414),4))!=struct.pack('<HH',0xA9,SYSTEM_CS):return halt(3,'custom HeapError')
        if here==(SYSTEM_CS,gm.WALK_HEAD) and state['previous'] in [(SYSTEM_CS,j) for j in gm.WALK_JAS]:
            state['heap_steps']+=1
            if state['heap_steps']>heap_cap:return halt(3,'heap budget')
        if here not in cache:cache[here]=next(md.disasm(bytes(uc.mem_read(address,size)),ip))
        ins=cache[here];visited.add(here)
        prev=cache.get(state['previous'])
        if prev and prev.mnemonic.startswith('j') and prev.mnemonic!='jmp':edges.add((state['previous'],here))
        if ins.mnemonic in ('call','lcall'):state['calls'].append(((cs,ip+ins.size),cs==CS))
        elif ins.mnemonic in ('ret','retf'):
            if here==(CS,0x0125):state['returned']=True
            else:
                check(bool(state['calls']),f'unmatched return {here}')
                state['returning']=state['calls'].pop()
        state['previous']=here
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);em.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(j*8))&255]))
    uc.hook_add(UC_HOOK_CODE,hook)
    if high:uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
    uc.emu_start(physical(CS,ENTRY),2*SIZE,count=3000000)
    check(state['stop'] is not None,f'original timeout {label}: {state}')
    expected=bytes(uc.mem_read(0,SIZE));ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    limits=Limits(SYSTEM_CS,operands_cap,strings_cap,heap_cap);d=Diagnostic()
    status=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(limits),ctypes.byref(d))
    check(status==state['stop'],f'status C={status}/{list(bytes(d))}, original={state["stop"]} {state["reason"]}: {label}')
    actual=bytes(ram)
    if actual!=expected:
        diff=[i for i,(a,b) in enumerate(zip(actual,expected)) if a!=b]
        raise ValueError(f'RAM mismatch {label}: {[(hex(i),actual[i],expected[i]) for i in diff[:8]]}; {len(diff)} bytes')
    for (name,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535
        check(getattr(cpu,name)==want,f'{name}: C={getattr(cpu,name):04x}, original={want:04x}: {label}')
    expected_d={'operand budget':(0,2,-1,-1),'string budget':(0,3,-1,-1),'RunError203':(1,3,0,2),'custom HeapError':(1,3,0,3),'heap budget':(1,3,0,5)}.get(state['reason'])
    if expected_d:check(tuple(getattr(d,n) for n,_ in d._fields_)==expected_d,f'diagnostic mismatch {label}')
    return status,state['reason'],visited,edges

def cases():
    rng=random.Random(0x1C0111);spec=[]
    for tag in range(256):
        blocks,top,end=gm.random_heap(rng)
        spec.append((f'tag-{tag:02x}',dict(tag=tag,low=0 if tag==0x81 else rng.randrange(256),high=0x49 if tag==0x81 else rng.randrange(256),
            old_head=(0x3380,0x100) if tag%2 else (0,0),blocks=blocks,top=top,end=end,flags=gm.FLAGS[tag%len(gm.FLAGS)],stale=rng.randrange(256))))
    spec += [('string-limit',dict(tag=0x81,low=0,high=0x49,strings_cap=1)),
        ('operand-limit-alias',dict(ss=DS,sp=0x7097,ops=[[0,3,0],[0,4,0],[0,5,0]],operands_cap=1)),
        ('operand-alias-complete',dict(ss=DS,sp=0x7097,ops=[[0,3,0],[0,4,0],[0,5,0]])),
        ('heap-full',dict(end=(0x3400,0))),('custom-heap',dict(heap_error=(0x1234,0x5678))),
        ('heap-limit',dict(blocks=((0x3010,0,0,0),),heap_cap=0)),
        ('heap-cycle',dict(blocks=((0x3010,0,0,0),),heap_cap=3,writes=((0x3010,0,struct.pack('<HH',0,0x3010)),))),
        ('exact-free',dict(blocks=((0x3010,0,0,8),))),('split-free',dict(blocks=((0x3010,8,2,8),))),
        ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
        ('random-background',dict(background=rng.randbytes(SIZE))),
        ('changed-loader-return',dict(ss=DS,sp=0x708E)),
        ('node-is-target',dict(top=(DS,0x7086),end=(DS+0x800,0))),
        ('node-is-old-head',dict(old_head=(0x3400,0))),
    ]
    for sp in (0x0232,0x0233,0x8001,0xFFF0,0xFFFC):spec.append((f'sp-{sp:x}',dict(sp=sp)))
    for offset in (0x7FF8,0x7FFC,0x7FFE):
        seg=SS+offset//16;off=offset%16
        spec.append((f'node-stack-alias-{offset:x}',dict(top=(seg,off),end=(seg+0x100,0))))
    for site,(off,_) in SITES.items():
        spec.append((f'code-{site:x}',dict(writes=((CS,site,b'\x90'),))))
        spec.append((f'stub-{site:x}',dict(writes=((STUB_CS,off,b'\x90'),))))
    return spec

def differential(target,caller,callee,image,function):
    visited,edges,stops,reasons=set(),set(),{},{};rng=random.Random(0x1C18F8)
    for label,options in cases():
        caps={k:options.pop(k) for k in list(options) if k.endswith('_cap')}
        mem,cpu,high=build(caller,callee,image,rng,**options)
        status,reason,v,e=compare(label,mem,cpu,high,function,**caps)
        stops[NAMES[status]]=stops.get(NAMES[status],0)+1;reasons[reason]=reasons.get(reason,0)+1;visited|=v;edges|=e
    cv={ip for cs,ip in visited if cs==CS};ce={e for e in edges if e[0][0]==CS}
    check(len(cv)==target['instructions'] and len(ce)==0,f'caller coverage {len(cv)}/{target["instructions"]}, edges {len(ce)}/0')
    report=dict(cases=sum(stops.values()),stops=stops,reasons=reasons,caller_starts=len(cv),caller_outcomes=len(ce),chain_starts=len(visited),chain_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE)
    print('Original-chain parity:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');args=p.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    target,caller,callee,image=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='poolrad-u1c0111-') as temp:
        f=library(Path(temp))
        if args.unicorn:report['unicorn']=differential(target,caller,callee,image,f)
        if args.native:
            from u1c_gosub_0111_cpu_native import native_audit
            report['native']=native_audit(image,caller,callee,f)
    if args.output:args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
