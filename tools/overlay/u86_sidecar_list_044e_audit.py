#!/usr/bin/env python3
"""Original U86 +044E full-chain CPU14/1MiB parity; media stays private."""
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
import u86_string_node_028d_audit as old
from u86_string_node_028d_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16,check,gm
CS,DS,SS=old.CS,old.DS,old.SS
ENTRY,END=0x044E,0x051C
RES=0xBA
BODY_SHA='a6b87b8c682cc8664e5ada6c55e970983f20987a9780bb65b5bda61770c04606'
class Limits(ctypes.Structure):
    _fields_=[('max_caller_visits',ctypes.c_size_t),('max_list_visits',ctypes.c_size_t),('max_lookup_loopbacks',ctypes.c_uint),('max_heap_steps',ctypes.c_uint)]
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('callee','status','nested_callee','nested_status')]
SOURCES=['overlay_u86_sidecar_list_044e.c','resident_00ba_sidecar_lookup_2199_cpu.c']+old.SOURCES

def source():
    image,code,decoded,dependency=old.source();md=Cs(CS_ARCH_X86,CS_MODE_16)
    body=code[ENTRY:END];ins=list(md.disasm(body,ENTRY))
    check(sha256(body).hexdigest()==BODY_SHA and len(ins)==71 and ins[-1].address==0x519 and ins[-1].mnemonic=='retf' and ins[-1].op_str=='2','caller differs')
    check([(i.address,i.op_str) for i in ins if i.mnemonic in ('call','lcall')]==[(0x464,'0xba, 0x2199'),(0x4EE,'0x28d')],'calls differ')
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit=next(u for u in old.find_units(exe[old.parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x86)
    check(0x467 in unit['fixups'],'lookup segment fixup missing')
    refs=[]
    for f in unit['functions']:
        prev=None
        for i in md.disasm(code[f['rel']:f['rel']+f['size_upper_bound']],f['rel']):
            if i.mnemonic=='call' and i.op_str=='0x44e':
                check(prev.mnemonic=='push' and prev.op_str=='cs','missing PUSH CS');refs.append((f['rel'],i.address))
            prev=i
    check(refs==[(0x51C,0x57E),(0x59C,0x68E)],f'caller refs differ {refs}')
    decoded.update({(CS,i.address):i for i in ins})
    raw=image[physical(RES,0x2199):physical(RES,0x2208)]
    decoded.update({(RES,i.address):i for i in md.disasm(raw,0x2199)})
    target=dict(bytes=len(body),instructions=len(ins),sha256=BODY_SHA,file_start=unit['ovr_offset']+ENTRY,file_end=unit['ovr_offset']+END,callers=refs,append_sha256=dependency['sha256'],lookup_sha256=sha256(raw).hexdigest())
    print('U86 +044E:',target,flush=True);return image,code,decoded,target

CHECKER_ROWS = ('OVL_0086_044E',)


def library(directory):
    path=directory/'u86044e.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u86_sidecar_list_044e
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f

def build(image,code,rng,*,mode=0,slots=(1,0x82,3),gate=0,signed_gate=0,kind=0,lookup=(),lookup_cycle=False,
          count=0,group=1,object_ptr=(0x5200,0x100),character_ptr=(0x5200,0x300),writes=(),**options):
    m,c,high=old.build(image,code,rng,argument=mode,group=0,**options);c.ip=ENTRY
    # Synthetic names/groups for every item, written before group fields to preserve overlap.
    for item in range(128):
        off=0x28E9+41*item;m[physical(DS,off)]=4
        for j in range(1,5):m[physical(DS,off+j)]=(item+j)&255
    for g in range(4):
        off=0xA79+41*g;m[physical(DS,off)]=3
        for j in range(1,4):m[physical(DS,off+j)]=0x41+g+j
    for item in range(256):m[physical(DS,0x31FB+16*item)]=group
    obj,off=object_ptr;char,coff=character_ptr
    for address,value in ((0x5D92,coff),(0x5D94,char),(0x6B80,off),(0x6B82,obj)):
        put16(m,DS,address,value)
    m[physical(char,(coff+0x96)&65535)]=signed_gate
    m[physical(obj,(off+0x2E)&65535)]=2;m[physical(obj,(off+0x35)&65535)]=gate
    m[physical(DS,0x5582+32)]=kind
    for j,value in enumerate(slots):m[physical(obj,(off+0x3C+j)&65535)]=value
    put16(m,char,(coff+0x7F)&65535,0);put16(m,char,(coff+0x81)&65535,0x5300 if lookup else 0)
    for j,key in enumerate(lookup):
        at=j*16;more=j+1<len(lookup) or lookup_cycle;nxt=((j+1)%len(lookup))*16
        m[physical(0x5300,at)]=key;put16(m,0x5300,at+5,nxt if more else 0);put16(m,0x5300,at+7,0x5300 if more else 0)
    m[physical(DS,0x4843)]=count
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    return m,c,high

def compare(label,memory,cpu,high,decoded,function,*,caller_cap=400,list_cap=400,lookup_cap=64,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,n))
    state=dict(stop=None,reason=None,returned=False,returning=None,calls=[],previous=None,caller_visits=0,list_visits=0,lookup_loops=0,heap_steps=0,callee=-1,nested=-1)
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
        if here==(CS,0x464):
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,0x2199):return halt(2,'lookup binding')
            state['callee']=0;state['nested']=-1;state['lookup_loops']=0
        if here==(CS,0x4EE):
            if bytes(uc.mem_read(address,3))!=bytes([0xE8,0x9C,0xFD]):return halt(2,'append binding')
            state['callee']=1;state['nested']=-1;state['list_visits']=0
        if here==(RES,0x21B9) and state['previous']==(RES,0x21F7):
            state['lookup_loops']+=1
            if state['lookup_loops']>lookup_cap:return halt(3,'lookup budget')
        if here==(CS,0x4AD):
            state['caller_visits']+=1
            if state['caller_visits']>caller_cap:return halt(5,'caller budget')
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
        if ins.mnemonic in ('call','lcall'):state['calls'].append(((cs,ip+ins.size),here in ((CS,0x464),(CS,0x4EE))))
        elif ins.mnemonic in ('ret','retf'):
            if here==(CS,0x519):state['returned']=True
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
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);l=Limits(caller_cap,list_cap,lookup_cap,heap_cap);d=Diagnostic()
    result=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(l),ctypes.byref(d))
    check(result==state['stop'],f'status {result}/{d.callee}/{d.status}/{d.nested_callee}/{d.nested_status} expected {state}: {label}')
    want=bytes(uc.mem_read(0,SIZE));actual=bytes(ram)
    if actual!=want:
        dif=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'RAM {label}: {len(dif)} differences {dif[:8]}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):check(getattr(cpu,n)==(uc.reg_read(r)&65535),f'{n}: C={getattr(cpu,n):04x} original={uc.reg_read(r)&65535:04x}: {label}')
    expected={'lookup budget':(0,2,-1,-1),'RunError203':(1,3,0,2),'custom HeapError':(1,3,0,3),'heap budget':(1,3,0,5)}.get(state['reason'])
    if expected:check(tuple(getattr(d,n) for n,_ in d._fields_)==expected,f'diagnostic {label}')
    return result,state['reason'],visited,edges

def cases():
    spec=[];rng=random.Random(0x86044E)
    for n in range(256):
        spec.append((f'byte-{n}',dict(mode=n%2,slots=(n,(n+127)&255,(n+128)&255),gate=0,signed_gate=n,kind=12 if n%3 else 0,
            lookup=(1,0x10) if n%4==0 else (2,3),count=n,length=n%4,group=n%4,flags=gm.FLAGS[n%len(gm.FLAGS)])))
    for signed in (0,1,127,128,255):
        for kind in (0,12):spec.append((f'gate-{signed}-{kind}',dict(gate=1,signed_gate=signed,kind=kind)))
    spec += [('found-clears',dict(gate=255,lookup=(0x10,))),('nil-lookup',dict(lookup=(),gate=1)),
        ('lookup-budget',dict(lookup=(1,2,0x10),lookup_cap=1)),('lookup-cycle',dict(lookup=(1,2),lookup_cycle=True,lookup_cap=3)),
        ('caller-budget',dict(caller_cap=1)),('list-budget',dict(length=3,list_cap=1)),('list-cycle',dict(length=3,cycle=True,list_cap=5)),
        ('heap-full',dict(end=(0x3400,0))),('heap-second-full',dict(end=(0x3403,0))),('custom-heap',dict(heap_error=(0x1234,0x5678))),
        ('heap-budget',dict(blocks=((0x3010,0,1,0),(0x3020,0,3,0)),heap_cap=0)),
        ('exact-free',dict(blocks=((0x3010,0,3,0),(0x3020,0,3,0)))),('split-free',dict(blocks=((0x3010,8,8,8),))),
        ('all-zero',dict(slots=(0,0,0))),('threshold',dict(mode=1,slots=(0x7F,0x80,0x81))),
        ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),('random-background',dict(background=rng.randbytes(SIZE))),
        ('object-wrap',dict(object_ptr=(0x5400,0xFFC4))),
        ('lookup-code',dict(writes=((CS,0x464,b'\x90'),))),('append-code',dict(writes=((CS,0x4EE,b'\x90'),))),
        ('nested-code',dict(writes=((CS,0x2AE,b'\x90'),))),
    ]
    for off in (0x7FEC,0x7EDC,0x7EE0,0x7FF2,0x7FFD):
        seg=SS+off//16;tail=off%16
        spec.append((f'heap-stack-{off:x}',dict(top=(seg,tail),end=(seg+0x100,0),group=0,slots=(1,0,0),caller_cap=5)))
    for sp in (0x150,0x151,0x8001,0xFFF8,0xFFFA):spec.append((f'sp-{sp:x}',dict(sp=sp)))
    return spec

def differential(image,code,decoded,function):
    visited,edges,stops,reasons=set(),set(),{},{};rng=random.Random(0x86044E)
    for label,options in cases():
        caps={k:options.pop(k) for k in list(options) if k.endswith('_cap')}
        m,c,h=build(image,code,rng,**options);status,reason,v,e=compare(label,m,c,h,decoded,function,**caps)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;reasons[reason]=reasons.get(reason,0)+1
        if not label.startswith('byte-'):print(label,status,reason,flush=True)
    starts={p for p in decoded if p[0]==CS and ENTRY<=p[1]<END};ce={e for e in edges if e[0] in starts}
    total=2*sum(decoded[p].mnemonic.startswith('j') and decoded[p].mnemonic!='jmp' for p in starts)
    check(starts<=visited,f'missing caller starts {starts-visited}');check(len(ce)==total,f'caller edges {len(ce)}/{total}')
    report=dict(cases=sum(stops.values()),stops=stops,reasons=reasons,caller_starts=len(starts),caller_outcomes=len(ce),caller_outcomes_total=total,chain_starts=len(visited),chain_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE)
    print('Original-chain:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    image,code,decoded,target=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='u86-044e-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code,decoded,f)
        if a.native:
            from u86_sidecar_list_044e_native import native_audit
            report['native']=native_audit(image,code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
