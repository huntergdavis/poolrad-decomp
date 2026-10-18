#!/usr/bin/env python3
"""Original U86 +051C complete-chain CPU14/1MiB parity; private owner media."""
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
import u86_sidecar_list_044e_audit as base
from u86_sidecar_list_044e_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16,check,gm,old,RES
CS,DS,SS=base.CS,base.DS,base.SS
ENTRY,END=0x051C,0x059C
BODY_SHA='ab8e34e0a8a47a7cefa2fc9754079107a0a0736b8a3e596cfee5ec10ae7e9c90'
class Limits(ctypes.Structure):
    _fields_=[('max_clear_visits',ctypes.c_size_t),('max_record_visits',ctypes.c_size_t),('sidecar',base.Limits)]
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('status',ctypes.c_int),('sidecar',base.Diagnostic)]
SOURCES=['overlay_u86_sidecar_gate_051c_cpu.c','overlay_u86_record_predicate_323d.c']+base.SOURCES

def source():
    image,code,decoded,dependency=base.source();md=Cs(CS_ARCH_X86,CS_MODE_16)
    body=code[ENTRY:END];ins=list(md.disasm(body,ENTRY))
    check(sha256(body).hexdigest()==BODY_SHA and len(ins)==44 and sum(i.size for i in ins)==128 and ins[-1].address==0x599 and ins[-1].mnemonic=='retf' and ins[-1].op_str=='2','caller differs')
    check([(i.address,i.op_str) for i in ins if i.mnemonic in ('call','lcall')]==[(0x572,'0x323d'),(0x57E,'0x44e')],'calls differ')
    check(sha256(code[0x323D:0x328A]).hexdigest()=='2ec1f95931eeb474593d0733ece3c8d582ab3a2a4d1c23b9de59d8f0bb5e87cd','predicate differs')
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit=next(u for u in old.find_units(exe[old.parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x86)
    refs=[]
    for f in unit['functions']:
        prev=None
        for i in md.disasm(code[f['rel']:f['rel']+f['size_upper_bound']],f['rel']):
            if i.mnemonic=='call' and i.op_str=='0x51c':
                check(prev.mnemonic=='push' and prev.op_str=='cs','missing PUSH CS');refs.append((f['rel'],i.address))
            prev=i
    check(refs==[(0x59C,0x69C),(0x59C,0x6A9)],f'caller refs differ {refs}')
    decoded.update({(CS,i.address):i for i in ins})
    decoded.update({(CS,i.address):i for i in md.disasm(code[0x323D:0x328A],0x323D)})
    target=dict(bytes=len(body),instructions=len(ins),sha256=BODY_SHA,file_start=unit['ovr_offset']+ENTRY,file_end=unit['ovr_offset']+END,callers=refs,sidecar_sha256=dependency['sha256'],predicate_sha256=sha256(code[0x323D:0x328A]).hexdigest())
    print('U86 +051C:',target,flush=True);return image,code,decoded,target

def library(directory):
    path=directory/'u86051c.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u86_sidecar_gate_051c_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f

def build(image,code,rng,*,records=1,record_cycle=False,kind=12,object_ptr=(0x5200,0x100),
          character_ptr=(0x5200,0x300),record_kinds=None,writes=(),**options):
    m,c,high=base.build(image,code,rng,kind=kind,object_ptr=object_ptr,character_ptr=character_ptr,**options);c.ip=ENTRY
    char,coff=character_ptr;obj,off=object_ptr
    put16(m,char,(coff+0xC8)&65535,off if records else 0);put16(m,char,(coff+0xCA)&65535,obj if records else 0)
    for j in range(records):
        here=(off+j*0x100)&65535;more=j+1<records or record_cycle
        nxt=(off+((j+1)%records)*0x100)&65535
        if j:
            for rel in (0x2E,0x35,0x3C,0x3D,0x3E):m[physical(obj,(here+rel)&65535)]=m[physical(obj,(off+rel)&65535)]
        if record_kinds is not None:
            m[physical(obj,(here+0x2E)&65535)]=j+2
            m[physical(DS,0x5582+16*(j+2))]=record_kinds[j]
        put16(m,obj,(here+0x2A)&65535,nxt if more else 0);put16(m,obj,(here+0x2C)&65535,obj if more else 0)
    # Seed cleared outputs; count must be reset regardless of input.
    for j in range(196):m[physical(DS,0x477F+j)]=(j*13+1)&255
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    return m,c,high

def compare(label,memory,cpu,high,decoded,function,*,clear_cap=400,record_cap=400,caller_cap=400,list_cap=400,lookup_cap=64,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,n))
    state=dict(stop=None,reason=None,returned=False,returning=None,calls=[],previous=None,caller_visits=0,list_visits=0,lookup_loops=0,heap_steps=0,callee=-1,nested=-1,clear_visits=0,record_visits=0)
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
        if here in ((CS,0x572),(CS,0x57E)):
            target=0x323D if ip==0x572 else 0x44E
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return halt(2,'direct binding')
            if ip==0x57E:state['caller_visits']=0
        if here==(CS,0x52B):
            state['clear_visits']+=1
            if state['clear_visits']>clear_cap:return halt(5,'clear budget')
        if here==(CS,0x560):
            state['record_visits']+=1
            if state['record_visits']>record_cap:return halt(5,'record budget')
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
        if ins.mnemonic in ('call','lcall'):state['calls'].append(((cs,ip+ins.size),here in ((CS,0x572),(CS,0x57E))))
        elif ins.mnemonic in ('ret','retf'):
            if here==(CS,0x599):state['returned']=True
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
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);l=Limits(clear_cap,record_cap,base.Limits(caller_cap,list_cap,lookup_cap,heap_cap));d=Diagnostic()
    result=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(l),ctypes.byref(d))
    check(result==state['stop'],f'status {result}/{d.callee}/{d.status}/{d.sidecar.nested_callee}/{d.sidecar.nested_status} expected {state}: {label}')
    want=bytes(uc.mem_read(0,SIZE));actual=bytes(ram)
    if actual!=want:
        dif=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'RAM {label}: {len(dif)} differences {dif[:8]}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):check(getattr(cpu,n)==(uc.reg_read(r)&65535),f'{n}: C={getattr(cpu,n):04x} original={uc.reg_read(r)&65535:04x}: {label}')
    expected={'lookup budget':(0,2,-1,-1),'RunError203':(1,3,0,2),'custom HeapError':(1,3,0,3),'heap budget':(1,3,0,5)}.get(state['reason'])
    if expected:check(d.callee==1 and d.status==3 and tuple(getattr(d.sidecar,n) for n,_ in d.sidecar._fields_)==expected,f'diagnostic {label}')
    return result,state['reason'],visited,edges

def cases():
    spec=[];rng=random.Random(0x86051C)
    for n in range(256):
        spec.append((f'category-{n}',dict(mode=n%2,kind=n,slots=(n,(n+127)&255,(n+128)&255),
            gate=0,signed_gate=n,lookup=(1,0x10) if n%4==0 else (2,3),
            count=n,length=n%4,group=n%4,flags=gm.FLAGS[n%len(gm.FLAGS)])))
    for n in range(64):
        spec.append((f'accepted-{n}',dict(mode=n%2,kind=11+n%3,slots=((n*17)&255,(n*29)&255,(n*37)&255),
            records=1+n%3,lookup=(1,0x10) if n%4==0 else (),signed_gate=(0,1,127,128,255)[n%5],
            gate=n%2,length=n%4,group=n%4,flags=gm.FLAGS[n%len(gm.FLAGS)])))
    spec += [
        ('nil',dict(records=0)),('mixed-chain',dict(records=3,record_kinds=(10,12,14))),
        ('record-cycle',dict(record_cycle=True,slots=(0,0,0),record_cap=3)),
        ('clear-budget',dict(clear_cap=1)),('terminal-record-budget',dict(record_cap=1)),
        ('lookup-budget',dict(lookup=(1,2,0x10),lookup_cap=1)),
        ('lookup-cycle',dict(lookup=(1,2),lookup_cycle=True,lookup_cap=3)),
        ('caller-budget',dict(caller_cap=1)),('list-budget',dict(length=3,list_cap=1)),
        ('list-cycle',dict(length=3,cycle=True,list_cap=5)),
        ('heap-full',dict(end=(0x3400,0))),('heap-second-full',dict(end=(0x3403,0))),
        ('custom-heap',dict(heap_error=(0x1234,0x5678))),
        ('heap-budget',dict(blocks=((0x3010,0,1,0),(0x3020,0,3,0)),heap_cap=0)),
        ('exact-free',dict(blocks=((0x3010,0,3,0),(0x3020,0,3,0)))),
        ('split-free',dict(blocks=((0x3010,8,8,8),))),
        ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),
        ('random-background',dict(background=rng.randbytes(SIZE))),
        ('object-wrap',dict(object_ptr=(0x5400,0xFFD4))),
        ('character-wrap',dict(character_ptr=(0x5500,0xFF37))),
        ('predicate-code',dict(writes=((CS,0x572,b'\x90'),))),
        ('sidecar-code',dict(writes=((CS,0x57E,b'\x90'),))),
        ('lookup-code',dict(writes=((CS,0x464,b'\x90'),))),
        ('append-code',dict(writes=((CS,0x4EE,b'\x90'),))),
        ('nested-code',dict(writes=((CS,0x2AE,b'\x90'),))),
        # Clear-loop aliases its own local index: the cleared byte changes live iteration.
        ('clear-local-alias',dict(ss=DS,sp=0x4786,clear_cap=100)),
        ('saved-bp-clear',dict(ss=DS,sp=0x4781,records=0)),
        ('outer-return-clear',dict(ss=DS,sp=0x4780,records=0)),
    ]
    for off in (0x7FE2,0x7ED2,0x7ED6,0x7FE8,0x7FF3,0x7FF6,0x7FF8):
        seg=SS+off//16;tail=off%16
        spec.append((f'heap-stack-{off:x}',dict(top=(seg,tail),end=(seg+0x100,0),group=0,slots=(1,0,0),caller_cap=5,record_cap=5)))
    for sp in (0x160,0x161,0x8001,0xFFF7,0xFFF8):spec.append((f'sp-{sp:x}',dict(sp=sp)))
    return spec

def differential(image,code,decoded,function):
    visited,edges,stops,reasons=set(),set(),{},{};rng=random.Random(0x86051C)
    for label,options in cases():
        caps={k:options.pop(k) for k in list(options) if k.endswith('_cap')}
        m,c,h=build(image,code,rng,**options);status,reason,v,e=compare(label,m,c,h,decoded,function,**caps)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;reasons[reason]=reasons.get(reason,0)+1
        if not label.startswith(('category-','accepted-')):print(label,status,reason,flush=True)
    starts={p for p in decoded if p[0]==CS and ENTRY<=p[1]<END};ce={e for e in edges if e[0] in starts}
    total=2*sum(decoded[p].mnemonic.startswith('j') and decoded[p].mnemonic!='jmp' for p in starts)
    check(starts<=visited,f'missing caller starts {starts-visited}');check(len(ce)==total,f'caller edges {len(ce)}/{total}')
    report=dict(cases=sum(stops.values()),stops=stops,reasons=reasons,caller_starts=len(starts),caller_outcomes=len(ce),caller_outcomes_total=total,chain_starts=len(visited),chain_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE)
    print('Original-chain:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    image,code,decoded,target=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='u86-051c-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code,decoded,f)
        if a.native:
            from u86_sidecar_gate_051c_native import native_audit
            report['native']=native_audit(image,code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
