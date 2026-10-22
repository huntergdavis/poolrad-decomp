#!/usr/bin/env python3
"""Original U86 +059C complete-chain CPU14/1MiB parity; private owner media."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import tempfile
import subprocess
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
import u86_sidecar_gate_051c_audit as base
from u86_sidecar_list_044e_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16,check,gm,old,RES
CS,DS,SS=base.CS,base.DS,base.SS
ENTRY,END=0x059C,0x0729
BODY_SHA='0c3e43c549277fd6e825ba20a28005b834de4be688dfe7f4cfa184f355d71b4d'
DIRECT={0x60E:0x28D,0x64A:0x28D,0x67A:0x28D,0x68E:0x44E,0x69C:0x51C,0x6A9:0x51C,0x70A:0x28D}
class Limits(ctypes.Structure):
    _fields_=[('max_history_visits',ctypes.c_size_t),('max_item_visits',ctypes.c_size_t),('gate',base.Limits)]
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('status',ctypes.c_int),('append',old.Diagnostic),('sidecar',base.base.Diagnostic),('gate',base.Diagnostic)]
SOURCES=['overlay_u86_list_build_059c_cpu.c']+base.SOURCES

def source():
    image,code,decoded,dependency=base.source();md=Cs(CS_ARCH_X86,CS_MODE_16)
    body=code[ENTRY:END];ins=list(md.disasm(body,ENTRY))
    check(sha256(body).hexdigest()==BODY_SHA and len(ins)==154 and sum(i.size for i in ins)==397 and ins[-1].address==0x726 and ins[-1].mnemonic=='retf' and ins[-1].op_str=='2','caller differs')
    check({i.address:int(i.op_str,16) for i in ins if i.mnemonic=='call'}==DIRECT,'calls differ')
    decoded.update({(CS,i.address):i for i in ins})
    target=dict(bytes=len(body),instructions=len(ins),sha256=BODY_SHA,file_start=161204,file_end=161601,gate_sha256=dependency['sha256'])
    print('U86 +059C:',target,flush=True);return image,code,decoded,target

CHECKER_ROWS = ('OVL_0086_059C',)


def library(directory):
    path=directory/'u86059c.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u86_list_build_059c_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f

def build(image,code,rng,*,carried=(1,0x82,0,127,128,255),owned=(1,2,55),eligible=1,category=0,writes=(),**options):
    m,c,high=base.build(image,code,rng,**options);c.ip=ENTRY
    char=options.get('character_ptr',(0x5200,0x300));seg,off=char
    for j in range(21):m[physical(seg,(off+0x17+j)&65535)]=carried[j] if j<len(carried) else 0
    for j in range(1,57):
        m[physical(seg,(off+0x32+j)&65535)]=1 if j in owned else 0
        m[physical(DS,0x31FA+16*j)]=category
    group=options.get('group',1)
    address=(off+0xB1+3*(category if category<128 else category-256)+(group if group<128 else group-256))&65535
    m[physical(seg,address)]=eligible
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    return m,c,high

def compare(label,memory,cpu,high,decoded,function,*,clear_cap=400,record_cap=400,caller_cap=400,list_cap=400,lookup_cap=64,heap_cap=64,history_cap=400,item_cap=400):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,n))
    state=dict(stop=None,reason=None,returned=False,returning=None,calls=[],previous=None,caller_visits=0,list_visits=0,lookup_loops=0,heap_steps=0,callee=-1,nested=-1,clear_visits=0,record_visits=0,history_visits=0,item_visits=0)
    visited,edges=set(),set()
    def halt(status,reason):state['stop']=status;state['reason']=reason;uc.emu_stop()
    def hook(em,address,size,user):
        del em,size,user
        here=(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP));cs,ip=here
        if state['returned']:return halt(0,'return')
        if state['returning'] is not None:
            expected,direct=state['returning'];state['returning']=None
            if here!=expected:return halt(4 if direct else 3,'changed direct return' if direct else 'changed nested return')
        if here==(gm.SYSTEM_CS,gm.RUNERROR_IP):return halt(3,'RunError203')
        check(here in decoded,f'original escape {here}: {label}')
        if cs==CS and ip in DIRECT:
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(DIRECT[ip]-ip-3)&65535):return halt(2,'direct binding')
            if DIRECT[ip]==0x28D:state['list_visits']=0
            if DIRECT[ip]==0x44E:state['caller_visits']=0
            if DIRECT[ip]==0x51C:state['clear_visits']=state['record_visits']=0
        if here==(CS,0x5B7):
            state['history_visits']+=1
            if state['history_visits']>history_cap:return halt(5,'history budget')
        if cs==CS and ip in (0x5D9,0x627,0x663,0x6BB):
            state['item_visits']+=1
            if state['item_visits']>item_cap:return halt(5,'item budget')
        if here in ((CS,0x572),(CS,0x57E)):
            target=0x323D if ip==0x572 else 0x44E
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return halt(3,'gate binding')
            if ip==0x57E:state['caller_visits']=0
        if here==(CS,0x52B):
            state['clear_visits']+=1
            if state['clear_visits']>clear_cap:return halt(3,'clear budget')
        if here==(CS,0x560):
            state['record_visits']+=1
            if state['record_visits']>record_cap:return halt(3,'record budget')
        if here==(CS,0x464):
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,0x2199):return halt(3,'lookup binding')
            state['callee']=0;state['nested']=-1;state['lookup_loops']=0
        if here==(CS,0x4EE):
            if bytes(uc.mem_read(address,3))!=bytes([0xE8,0x9C,0xFD]):return halt(3,'append binding')
            state['callee']=1;state['nested']=-1;state['list_visits']=0
        if here==(RES,0x21B9) and state['previous']==(RES,0x21F7):
            state['lookup_loops']+=1
            if state['lookup_loops']>lookup_cap:return halt(3,'lookup budget')
        if here==(CS,0x4AD):
            state['caller_visits']+=1
            if state['caller_visits']>caller_cap:return halt(3,'caller budget')
        if here==(CS,0x2D4):
            state['list_visits']+=1
            if state['list_visits']>list_cap:return halt(3,'list budget')
        if cs==CS and ip in old.SITES:
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,old.SITES[ip]):return halt(3,'nested binding')
            state['nested']=old.KINDS[old.SITES[ip]]
        if here==(gm.SYSTEM_CS,0x23F):state['heap_steps']=0
        if cs==gm.SYSTEM_CS and ip in gm.HEAP_ERROR_SITES:
            if bytes(uc.mem_read(physical(cpu.ds,0x4414),4))!=struct.pack('<HH',0xA9,gm.SYSTEM_CS):return halt(3,'custom HeapError')
        if here==(gm.SYSTEM_CS,gm.WALK_HEAD) and state['previous'] in [(gm.SYSTEM_CS,j) for j in gm.WALK_JAS]:
            state['heap_steps']+=1
            if state['heap_steps']>heap_cap:return halt(3,'heap budget')
        ins=decoded[here];visited.add(here);prev=decoded.get(state['previous'])
        if prev and prev.mnemonic.startswith('j') and prev.mnemonic!='jmp':edges.add((state['previous'],here))
        if ins.mnemonic in ('call','lcall'):state['calls'].append(((cs,ip+ins.size),(cs==CS and ip in DIRECT)))
        elif ins.mnemonic in ('ret','retf'):
            if here==(CS,0x726):state['returned']=True
            else:state['returning']=state['calls'].pop()
        state['previous']=here
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);em.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    uc.hook_add(UC_HOOK_CODE,hook)
    if high:uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
    uc.emu_start(physical(CS,ENTRY),2*SIZE,count=5000000)
    check(state['stop'] is not None,f'original timeout {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);l=Limits(history_cap,item_cap,base.Limits(clear_cap,record_cap,base.base.Limits(caller_cap,list_cap,lookup_cap,heap_cap)));d=Diagnostic()
    result=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(l),ctypes.byref(d))
    check(result==state['stop'],f'status {result}/{d.callee}/{d.status}/{d.append.callee}/{d.append.status} expected {state}: {label}')
    want=bytes(uc.mem_read(0,SIZE));actual=bytes(ram)
    if actual!=want:
        dif=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'RAM {label}: {len(dif)} differences {dif[:8]}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):check(getattr(cpu,n)==(uc.reg_read(r)&65535),f'{n}: C={getattr(cpu,n):04x} original={uc.reg_read(r)&65535:04x}: {label}')
    if state['reason'] in ('RunError203','custom HeapError','heap budget'):
        nested=d.append if d.callee==0 else d.sidecar if d.callee==1 else d.gate.sidecar
        got=nested.status if d.callee==0 else nested.nested_status
        check(got=={'RunError203':2,'custom HeapError':3,'heap budget':5}[state['reason']],f'heap diagnostic {label}')
    return result,state['reason'],visited,edges

def cases():
    spec=[]
    for mode in range(256):
        spec.append((f'mode-{mode}',dict(mode=mode,flags=gm.FLAGS[mode%len(gm.FLAGS)])))
    for mode in range(7):
        for flags in gm.FLAGS:spec.append((f'flags-{mode}-{flags:x}',dict(mode=mode,flags=flags,group=0)))
    for mode in (0,1,4,5):
        spec += [(f'empty-{mode}',dict(mode=mode,carried=(),owned=(),eligible=0)),
                 (f'heap-full-{mode}',dict(mode=mode,end=(0x3400,0))),
                 (f'custom-heap-{mode}',dict(mode=mode,heap_error=(0x1234,0x5678))),
                 (f'list-budget-{mode}',dict(mode=mode,list_cap=1)),
                 (f'item-budget-{mode}',dict(mode=mode,item_cap=1)),
                 (f'exact-free-{mode}',dict(mode=mode,blocks=((0x3010,0,3,0),(0x3020,0,3,0)))),
                 (f'split-free-{mode}',dict(mode=mode,blocks=((0x3010,8,8,8),)))]
    for mode in (2,3,6):
        spec += [(f'heap-full-{mode}',dict(mode=mode,end=(0x3400,0))),
                 (f'lookup-budget-{mode}',dict(mode=mode,lookup=(1,2),lookup_cap=0)),
                 (f'lookup-cycle-{mode}',dict(mode=mode,lookup=(1,2),lookup_cycle=True,lookup_cap=2)),
                 (f'side-budget-{mode}',dict(mode=mode,caller_cap=1)),
                 (f'blocked-gate-{mode}',dict(mode=mode,gate=1,signed_gate=0))]
    for mode in (3,6):
        spec += [(f'clear-budget-{mode}',dict(mode=mode,clear_cap=1)),
                 (f'record-budget-{mode}',dict(mode=mode,record_cap=1)),
                 (f'no-records-{mode}',dict(mode=mode,records=0)),
                 (f'record-cycle-{mode}',dict(mode=mode,record_cycle=True,slots=(0,0,0),record_cap=2)),
                 (f'category-reject-{mode}',dict(mode=mode,kind=14))]
    for cat in (0,1,127,128,255):
        for group in (0,1,127,128,255):spec.append((f'signed-{cat}-{group}',dict(mode=4,category=cat,group=group,owned=tuple(range(1,56)))))
    modes={0x60E:0,0x64A:5,0x67A:1,0x68E:2,0x69C:3,0x6A9:6,0x70A:4}
    for ip,mode in modes.items():
        spec += [(f'code-{ip:x}',dict(mode=mode,writes=((CS,ip,b'X'),))),
                 (f'operand-{ip:x}',dict(mode=mode,writes=((CS,ip+1,bytes(2)),)))]
    spec += [('history-budget',dict(mode=0,history_cap=1)),
       ('nested-code',dict(mode=0,writes=((CS,0x2AE,b'X'),))),
       ('gate-code',dict(mode=3,writes=((CS,0x572,b'X'),))),
       ('lookup-code',dict(mode=2,writes=((CS,0x464,b'X'),))),
       ('heap-budget',dict(mode=0,blocks=((0x3010,0,1,0),(0x3020,0,3,0)),heap_cap=0)),
       ('direct-return-alias',dict(mode=0,carried=(1,),group=0,top=(SS+0x7FF,6),end=(SS+0x900,0))),
       ('ss-ds',dict(mode=0,ss=DS)),('a20',dict(mode=0,ss=0xFFFF)),
       ('history-local-alias',dict(mode=7,ss=DS,sp=0x4770,history_cap=40)),
       ('outer-return-clear',dict(mode=7,ss=DS,sp=0x4766,carried=())),
       ('character-wrap',dict(mode=0,character_ptr=(0x5500,0xFFE0)))]
    for sp in (0x16A,0x16B,0x8001,0xFFF9,0xFFFA):spec.append((f'sp-{sp:x}',dict(mode=0,sp=sp)))
    return spec

def differential(image,code,decoded,function):
    visited,edges,stops,reasons=set(),set(),{},{};rng=random.Random(0x86051C)
    for label,options in cases():
        caps={k:options.pop(k) for k in list(options) if k.endswith('_cap')}
        m,c,h=build(image,code,rng,**options);status,reason,v,e=compare(label,m,c,h,decoded,function,**caps)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;reasons[reason]=reasons.get(reason,0)+1
        if not label.startswith(('mode-','flags-')):print(label,status,reason,flush=True)
    starts={p for p in decoded if p[0]==CS and ENTRY<=p[1]<END};ce={e for e in edges if e[0] in starts}
    total=2*sum(decoded[p].mnemonic.startswith('j') and decoded[p].mnemonic!='jmp' for p in starts)
    check(starts<=visited,f'missing caller starts {starts-visited}');check(len(ce)==total,f'caller edges {len(ce)}/{total}')
    report=dict(cases=sum(stops.values()),stops=stops,reasons=reasons,caller_starts=len(starts),caller_outcomes=len(ce),caller_outcomes_total=total,chain_starts=len(visited),chain_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE,flag_masks=0,uncovered_starts=[f"{cs:04X}:{ip:04X}" for cs,ip in sorted(set(decoded)-visited)])
    print('Original-chain:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    image,code,decoded,target=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='u86-059c-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code,decoded,f)
        if a.native:
            from u86_list_build_059c_native import native_audit
            report['native']=native_audit(image,code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
