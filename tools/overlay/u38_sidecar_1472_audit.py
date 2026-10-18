#!/usr/bin/env python3
"""Original U38 +1472 full-chain CPU14/1MiB differential; media stays private."""
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
import u9e_list_append_0e52_cpu_audit as old
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
CS,DICE,STUB,DS,SS=0x8800,0x7800,0xB0,old.DS,0x2800
ENTRY,END=0x1472,0x1490
SITES={0x1487:(0x52,0xE52)}
check=old.check;gm=old.g
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('append_status','getmem_status')]
SOURCES=['overlay_u38_sidecar_1472_cpu.c','overlay_u9e_list_append_0e52_cpu.c','system_getmem_023f_cpu.c']

def source():
    append,code9e,image,parts=old.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x38)
    code38=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];body=code38[ENTRY:END];md=Cs(CS_ARCH_X86,CS_MODE_16);ins=list(md.disasm(body,ENTRY))
    check(sha256(body).hexdigest()=='80480d29f87a095087d3bfe3485de2a659702f7214c758e797a03beffdaff680','caller hash differs')
    check(u['entries'][49]==ENTRY and len(body)==30 and len(ins)==15 and ins[-1].address==0x148D and ins[-1].mnemonic=='retf' and ins[-1].op_str=='0xa','caller extent differs')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(a,f'0xb0, {v[0]:#x}') for a,v in SITES.items()],'caller targets differ')
    check(all(a+3 in u['fixups'] for a in SITES),'caller fixups differ')
    check(next(v for v in sorted(f['rel'] for f in u['functions']) if v>ENTRY)==END,'next function bound differs')
    decoded={(DICE if kind=='u9e' else gm.SYSTEM_CS,at):i for (kind,at),i in parts.items()}
    decoded.update({(CS,i.address):i for i in ins})
    for off,target in SITES.values():
        i=next(md.disasm(struct.pack('<BHH',0xEA,target,DICE),off));decoded[(STUB,off)]=i
    target=dict(bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),file_start=u['ovr_offset']+ENTRY,file_end=u['ovr_offset']+END,
        queue_bytes=30,trailing_data_bytes=0,slot=49,append_sha256=append['sha'])
    print('U38 +1472:',target,flush=True);return image,code38,code9e,decoded,target

def library(directory):
    path=directory/'u381472.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u38_sidecar_1472_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint,ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f

def build(image,code38,code9e,rng,*,record=(0x5000,0x100),length=0,cycle=None,nodes=None,sp=0x8000,ss=SS,writes=(),**options):
    if nodes is None:nodes=[(0x5200,j*16) for j in range(length)]
    m,c,high=old.build(code9e,image,rng,record=record,nodes=nodes,cycle=cycle,sp=sp,ss=ss,**options)
    for seg,code in ((CS,code38),(DICE,code9e)):
        at=physical(seg,0);m[at:at+len(code)]=code
    for off,target in SITES.values():
        at=physical(STUB,off);m[at:at+5]=struct.pack('<BHH',0xEA,target,DICE)
    for j,v in enumerate((0x7000,0x9001,0x1111,0x2222,0x3333,record[1],record[0])):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    c.cs=CS;c.ip=ENTRY;return m,c,high

def compare(label,memory,cpu,high,decoded,function,*,heap_cap=64,link_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,n))
    state=dict(stop=None,reason=None,returned=False,returning=None,calls=[],previous=None,links=0,heap_steps=0)
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
        if cs==CS and ip in SITES:
            off,target=SITES[ip];raw=bytes(uc.mem_read(address,5));stub=physical(int.from_bytes(raw[3:5],'little'),off)
            if raw[:3]!=struct.pack('<BH',0x9A,off) or bytes(uc.mem_read(stub,3))!=struct.pack('<BH',0xEA,target):return halt(2,'binding')
        if here==(DICE,0xE5C):
            if bytes(uc.mem_read(address,3))!=bytes([0x9A,0x3F,0x02]):return halt(3,'nested binding')
        if here==(DICE,0xEA8):
            state['links']+=1
            if state['links']>link_cap:return halt(3,'list budget')
        if cs==gm.SYSTEM_CS and ip in gm.HEAP_ERROR_SITES:
            if bytes(uc.mem_read(physical(cpu.ds,0x4414),4))!=struct.pack('<HH',0xA9,gm.SYSTEM_CS):return halt(3,'custom HeapError')
        if here==(gm.SYSTEM_CS,gm.WALK_HEAD) and state['previous'] in [(gm.SYSTEM_CS,j) for j in gm.WALK_JAS]:
            state['heap_steps']+=1
            if state['heap_steps']>heap_cap:return halt(3,'heap budget')
        ins=decoded[here];visited.add(here);prev=decoded.get(state['previous'])
        if prev and prev.mnemonic.startswith('j') and prev.mnemonic!='jmp':edges.add((state['previous'],here))
        if ins.mnemonic in ('call','lcall'):state['calls'].append(((cs,ip+ins.size),cs==CS))
        elif ins.mnemonic in ('ret','retf'):
            if here==(CS,0x148D):state['returned']=True
            else:state['returning']=state['calls'].pop()
        state['previous']=here
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);em.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    uc.hook_add(UC_HOOK_CODE,hook)
    if high:uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
    uc.emu_start(physical(CS,ENTRY),2*SIZE,count=4000000)
    check(state['stop'] is not None,f'original timeout {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);d=Diagnostic()
    result=function(ram,SIZE,ctypes.byref(cpu),heap_cap,link_cap,ctypes.byref(d))
    check(result==state['stop'],f'status {result}/{d.append_status}/{d.getmem_status}, expected {state}: {label}')
    want=bytes(uc.mem_read(0,SIZE));actual=bytes(ram)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'RAM {label}: {len(diff)} differences {diff[:8]}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):check(getattr(cpu,n)==(uc.reg_read(r)&65535),f'{n}: C={getattr(cpu,n):04x} original={uc.reg_read(r)&65535:04x}: {label}')
    expected={'RunError203':(3,2),'custom HeapError':(3,3),'heap budget':(3,5),'list budget':(5,0),'nested binding':(2,-1)}.get(state['reason'])
    if expected:check(tuple(getattr(d,n) for n,_ in d._fields_)==expected,f'diagnostic {label}')
    return result,state['reason'],visited,edges

def cases():
    spec=[];rng=random.Random(0x381472)
    for n in range(256):
        blocks,top,end=gm.random_heap(rng)
        spec.append((f'heap-{n}',dict(length=n%7,blocks=blocks,top=top,end=end,flags=gm.FLAGS[n%len(gm.FLAGS)])))
    spec += [('list-budget',dict(length=4,link_cap=1)),('list-cycle',dict(length=3,cycle=0,link_cap=5)),
        ('heap-full',dict(end=(0x3400,0))),('custom-heap',dict(heap_error=(0x1234,0x5678))),
        ('heap-budget',dict(blocks=((0x3010,0,0,0),(0x3020,0,1,0)),heap_cap=0)),
        ('heap-cycle',dict(blocks=((0x3010,0,0,0),),writes=((0x3010,0,struct.pack('<HH',0,0x3010)),),heap_cap=3)),
        ('exact-free',dict(blocks=((0x3010,0,1,0),))),('split-free',dict(blocks=((0x3010,8,8,8),))),
        ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),('random-background',dict(background=rng.randbytes(SIZE))),
        ('record-wrap',dict(record=(0x5000,0xFF80))),('node-is-record',dict(record=(0x3400,0))),
        ('node-is-old-node',dict(nodes=[(0x3400,0)])),
        ('nested-code',dict(writes=((DICE,0xE5C,b'\x90'),))),
    ]
    for off in (0x7FEE,0x7FE8,0x7FFD):
        seg=SS+off//16;tail=off%16
        spec.append((f'heap-stack-{off:x}',dict(top=(seg,tail),end=(seg+0x100,0))))
    for sp in (0x40,0x41,0x8001,0xFFF0,0xFFF2):spec.append((f'sp-{sp:x}',dict(sp=sp)))
    for site,(off,_) in SITES.items():
        spec.append((f'code-{site:x}',dict(writes=((CS,site,b'\x90'),))))
        spec.append((f'stub-{site:x}',dict(writes=((STUB,off,b'\x90'),))))
    return spec

def differential(image,code38,code9e,decoded,function):
    visited,edges,stops,reasons=set(),set(),{},{};rng=random.Random(0x381472)
    for label,options in cases():
        caps={k:options.pop(k) for k in list(options) if k.endswith('_cap')}
        m,c,h=build(image,code38,code9e,rng,**options);status,reason,v,e=compare(label,m,c,h,decoded,function,**caps)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;reasons[reason]=reasons.get(reason,0)+1
        if not label.startswith('heap-') or label.startswith('heap-stack'):print(label,status,reason,flush=True)
    starts={p for p in decoded if p[0]==CS};ce={e for e in edges if e[0] in starts}
    check(starts<=visited and len(ce)==0,f'caller coverage {len(starts&visited)}/{len(starts)}, edges {len(ce)}')
    report=dict(cases=sum(stops.values()),stops=stops,reasons=reasons,caller_starts=len(starts),caller_outcomes=len(ce),chain_starts=len(visited),chain_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE)
    print('Original-chain:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    image,code38,code9e,decoded,target=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='u38-1472-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code38,code9e,decoded,f)
        if a.native:
            from u38_sidecar_1472_native import native_audit
            report['native']=native_audit(image,code38,code9e,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
