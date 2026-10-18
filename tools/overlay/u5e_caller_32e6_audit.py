#!/usr/bin/env python3
"""Original-code CPU14/whole-RAM audit of complete U5E +32E6."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
from unicorn import x86_const as R
from overlay_inventory import find_units,parse_mz
import u3c2_caller_0909_audit as prior
from u3c2_line_init_01a0_audit import Cpu,NAMES,REGS,physical,put16
ROOT=Path(__file__).resolve().parents[2]
SIZE=1<<20
ENTRY=0x32E6
SOURCES=('overlay_u5e_caller_32e6',)+prior.SOURCES
class Layout(ctypes.Structure):
    _fields_=[(n,ctypes.c_uint16) for n in ('slots_cs','chain_cs','system_cs')]
class Limits(ctypes.Structure):
    _fields_=[('slot_iterations',ctypes.c_size_t),('chain_iterations',ctypes.c_size_t),('line_iterations',ctypes.c_size_t),('sort_loopbacks',ctypes.c_uint),('copy_iterations',ctypes.c_size_t)]
DEFAULT=(256,20000,512,65536,256)

def source():
    old,_=prior.source();bodies={('chain' if k=='caller' else 'slots' if k=='pair' else k,a):v for (k,a),v in old.items()}
    p=ROOT/'resources/DOS/POOLRAD';e,o=(p/'START.EXE').read_bytes(),(p/'GAME.OVR').read_bytes()
    units={u['stub_para']:u for u in find_units(e[parse_mz(e)['header_bytes']:],o)}
    assert [f for f in units[0x5E]['fixups'] if ENTRY<=f<0x3389]==[0x32F5,0x3301,0x3314,0x3322,0x3379]
    incoming=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(o[units[0x5E]['ovr_offset']+0x3558:units[0x5E]['ovr_offset']+0x37E4],0x3558))
    sites=[i.address for i in incoming if i.mnemonic=='call' and i.op_str=='0x32e6']
    assert sites==[0x35E6,0x36EF,0x3736]
    assert all(incoming[j-1].mnemonic=='push' and incoming[j-1].op_str=='cs' for j,i in enumerate(incoming) if i.address in sites)
    for kind,u,start,end,count,digest in (
        ('caller',0x5E,ENTRY,0x3389,62,'456ced130d112bf573dafb528d31b76604c455891fa25622da2d4cfba1568772'),
        ('slots',0x3C6,0xBE7,0xC0F,17,'eb48355d64c585ee95a31739074ede53929cc991ba519157009c7c8e1c9b7ed5'),
        ('slots',0x3C6,0xC0F,0xC37,17,'a1000b2f6a29a0d74f37a9b631b26844044f2c1a163b2de793672cbc0664395d'),
        ('slots',0x3C6,0xC37,0xC5F,17,'71c5655d5b32a4688be0ab98761c4b5d2cfb4cc117056648ab92a552ffc06e48'),
        ('slots',0x3C6,0xC5F,0xCB9,32,'c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41')):
        data=o[units[u]['ovr_offset']+start:units[u]['ovr_offset']+end]
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,start))
        assert not count or len(ins)==count,(hex(start),len(ins))
        assert not digest or sha256(data).hexdigest()==digest
        bodies[kind,start]=data;print(kind,hex(start),len(data),len(ins),sha256(data).hexdigest())
    decoded={(k,i.address):i for (k,a),data in bodies.items() for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,a)}
    assert [(i.address,i.op_str) for (k,a),i in decoded.items() if k=='caller' and i.mnemonic=='lcall']==[(0x32F2,'0x3e0, 0x5c'),(0x32FE,'0x3e0, 0x61'),(0x3311,'0x3e0, 0x66'),(0x331F,'0x3db, 0x3e'),(0x3376,'0xaf8, 0xa3f')]
    return bodies,decoded

def library(directory):
    p=directory/'caller.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES]+['-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u5e_caller_32e6
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Layout),ctypes.POINTER(Limits)];f.restype=ctypes.c_int
    return f

def fixture(*,party=((10,10,1),(12,12,1)),selected=1,parent=0xE000,
            cs=0x1000,slots=0x3000,chain=0x4000,system=0x6000,ss=0x2000,ds=0x5000,
            sp=0xF000,flags=0x602,seed=0,map_offset=0x8000,**kw):
    m,c,_,_=prior.fixture(party=party,cs=chain,pair=slots,system=system,ss=ss,ds=ds,sp=sp,flags=flags,seed=seed,map_offset=map_offset,**kw)
    c.cs,c.ip=cs,ENTRY
    for i in range(256):
        put16(m,ds,0x65B9+4*i,0x1000+32*i);put16(m,ds,0x65BB+4*i,0x7000)
    # Exact pointer identity, including high word; 256 is deliberately absent.
    for i,w in enumerate((0x9000,0xA000,parent,0x1000+32*selected,0x7000)):put16(m,ss,sp+2*i,w)
    put16(m,ds,0x6716,map_offset);put16(m,ds,0x6718,ds)
    return m,c,Layout(slots,chain,system)

def call_target(kind,ip,ins):
    if ins.mnemonic=='call':return kind,int(ins.op_str,16)
    if kind=='caller':return {0x32F2:('slots',0xBE7),0x32FE:('slots',0xC0F),0x3311:('slots',0xC37),0x331F:('chain',0x909),0x3376:('sys',0xA3F)}[ip]
    return ('slots',0) if ip in (0x92A,0x9D8) else ('sys',0xA3F)

def install(m,c,layout,bodies,decoded):
    segments=dict(caller=c.cs,slots=layout.slots_cs,chain=layout.chain_cs,sys=layout.system_cs)
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        for (k,ip),ins in decoded.items():
            if k==kind and start<=ip<start+len(data) and ins.mnemonic=='lcall':
                target,off=call_target(kind,ip,ins)
                blob[ip-start+1:ip-start+5]=off.to_bytes(2,'little')+segments[target].to_bytes(2,'little')
        for i,v in enumerate(blob):m[physical(segments[kind],start+i)]=v

def compare(label,state,bodies,decoded,f,budgets=DEFAULT):
    m,c,layout=state;install(m,c,layout,bodies,decoded)
    segments=dict(caller=c.cs,slots=layout.slots_cs,chain=layout.chain_cs,sys=layout.system_cs)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for n,r in zip(NAMES,REGS):uc.reg_write(r,getattr(c,n))
    visited,edges=set(),set();previous=[None];last=[None];remaining=list(budgets);stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        del address,size,user
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535
        kind=next((k for k,s in segments.items() if s==cs),None);key=kind,ip
        assert key in decoded,(label,'unexpected PC',hex(cs),hex(ip))
        if key==('chain',0x422):
            sp=uc.reg_read(R.UC_X86_REG_SP);ss=uc.reg_read(R.UC_X86_REG_SS)
            if not 0x54<=sp<=0xFFE8 or any((physical(ss,sp-i)-physical(layout.chain_cs,0))%SIZE<0x2E or 0x1A0<=(physical(ss,sp-i)-physical(layout.chain_cs,0))%SIZE<0x57B for i in range(1,85)):
                stop[0]=7;uc.emu_stop();return
        index=0 if key==('slots',0xC71) else 1 if kind=='chain' and ip in prior.LOOPS else 2 if key==('chain',0x4DD) else 3 if kind=='chain' and ip in (0x72,0x56) and last[0] in (('chain',0x18E),('chain',0x199)) else 4 if key==('caller',0x3349) else None
        if index is not None:
            if not remaining[index]:stop[0]=index+2;uc.emu_stop();return
            remaining[index]-=1
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key);previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None;last[0]=key
        if ins.mnemonic in ('call','lcall'):
            target=call_target(kind,ip,ins);calls.append(((cs,ip+ins.size),target))
            if target in (('slots',0xBE7),('slots',0xC0F),('slots',0xC37)):remaining[0]=budgets[0]
            if target==('chain',0x422):remaining[2]=budgets[2]
            if target==('chain',0x2E):remaining[3]=budgets[3]
        if ins.mnemonic=='retf':stop[0]='ret';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook)
    status=0
    while True:
        stop[0]=None;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=2000000)
        assert stop[0] is not None,(label,'original instruction limit')
        if stop[0]!='ret':status=stop[0];break
        stepping[0]=True;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1);stepping[0]=False
        if not calls:break
        expected,target=calls.pop()
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=expected:
            status=7 if any(t==('chain',0x909) for _,t in calls) else 8;break
    for (kind,start),data in bodies.items():
        assert all(backing[physical(segments[kind],start+i)]==m[physical(segments[kind],start+i)] for i in range(len(data))),(label,'code modified')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c);limits=Limits(*budgets)
    got=f(ram,SIZE,ctypes.byref(actual),ctypes.byref(layout),ctypes.byref(limits))
    assert got==status,(label,'status',got,status)
    assert bytes(ram)==bytes(backing),(label,'RAM',[(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12])
    for n,r in zip(NAMES,REGS):assert getattr(actual,n)==uc.reg_read(r)&65535,(label,n,hex(getattr(actual,n)),hex(uc.reg_read(r)&65535))
    return visited,edges,status,actual

def specifications(quick=False):
    cases=[('default',{}),('empty',dict(party=())),('missing',dict(selected=256)),('slot-zero',dict(selected=0)),('second',dict(selected=2)),('zero-type',dict(party=((10,10,0),(12,12,1)))),('blocked',dict(mode=0,heights=(0,255)))]
    cases += [('zero-count-missing',dict(party=(),selected=256)),
              ('zero-count-wrap-match',dict(party=(),selected=0)),
              ('last-slot-255',dict(party=((10,10,1),)*255,selected=255,budgets=(256,1,512,65536,256))),
              ('missing-at-255',dict(party=((10,10,1),)*255,selected=256,budgets=(256,1,512,65536,256)))]
    for v in (0,1,49,50,127,128,255):
        for axis in (0,1):
            p=[10,10,1];p[axis]=v;cases.append((f'coord-{axis}-{v}',dict(party=(tuple(p),(12,12,1)))))
    for t in ((1,2,127,128,255) if quick else range(256)):
        cases.append((f'type-{t}',dict(party=((10,10,t),(12,12,1)))))
    for n in (3,5,10,20):cases.append((f'party-{n}',dict(party=tuple((20-i%20,20-i%20,1) for i in range(n)),selected=n)))
    for sp in (0xA4,0xA5,0xFFF5,0xFFF6):cases.append((f'stack-{sp:x}',dict(sp=sp)))
    for parent in (0,0xD9,0xDA,0xDB,0xDC,0xFFFF,0xF0DA,0xF0D7,0xF0D8,0xF0D9,0xF0DD):cases.append((f'dest-{parent:x}',dict(parent=parent)))
    for budget in (1,2,3):cases.append((f'slot-limit-{budget}',dict(selected=256,budgets=(budget,20000,512,65536,256))))
    for budget in (1,3,4,5,8,12,16,20,24,25,32,48,64):cases.append((f'chain-limit-{budget}',dict(budgets=(256,budget,512,65536,256))))
    for budget in (1,2,3,4):cases.append((f'line-limit-{budget}',dict(budgets=(256,20000,budget,65536,256))))
    for budget in (1,2,3):cases.append((f'sort-limit-{budget}',dict(party=tuple((20-i,20-i,1) for i in range(5)),budgets=(256,20000,512,budget,256))))
    for budget in (1,2):cases.append((f'copy-limit-{budget}',dict(party=((10,10,1),(11,11,1),(12,12,1)),budgets=(256,20000,512,65536,budget))))
    for sp in (0x5F20,0x5F40,0x6710,0x671B,0x671C,0x671D,0x6720,0x6724,0x6728,0x6730,0x6740,0x2800):cases.append((f'stack-data-{sp:x}',dict(ss=0x5000,sp=sp,budgets=(256,256,32,128,256))))
    cases += [('A20-stack',dict(ss=0xFFFF,sp=0x300)),('relocated',dict(cs=0x7000,slots=0x8000,chain=0x9000,system=0xA000)),('A20-code',dict(cs=0xFFFF))]
    rng=random.Random(0x5E32E6)
    for i in range(12 if quick else 100):
        party=tuple((rng.randrange(55),rng.randrange(30),rng.choice((0,1,2,3))) for _ in range(rng.randrange(1,6)))
        cases.append((f'random-{i}',dict(party=party,selected=rng.randrange(len(party)+2),seed=i,mode=rng.randrange(2),heights=(rng.randrange(3),rng.randrange(3)),shape=tuple((rng.randrange(-1,3),rng.randrange(-1,3)) for _ in range(4)),flags=rng.choice((2,0x602,0x8D7,0xCD7)))))
    return cases

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded=source();visited=set();edges=set();statuses={};stops={};count=0
    with tempfile.TemporaryDirectory() as tmp:
        f=library(Path(tmp))
        for label,kw in specifications(a.quick):
            kw=dict(kw);budgets=kw.pop('budgets',DEFAULT);print('BEGIN',count+1,label,flush=True)
            v,e,s,c=compare(label,fixture(**kw),bodies,decoded,f,budgets)
            visited.update(v);edges.update(e);statuses[str(s)]=statuses.get(str(s),0)+1;count+=1
            if s:stops[f'{s}:{c.cs:04X}:{c.ip:04X}']=stops.get(f'{s}:{c.cs:04X}:{c.ip:04X}',0)+1
    caller={k for k in decoded if k[0]=='caller'};cond={k for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    ce={e for e in edges if e[0] in caller};cc=cond&caller
    assert caller<=visited,sorted(caller-visited)
    report=dict(target='OVL_005E_32E6',cases=count,cpu_words=14,ram_bytes=SIZE,status_counts=statuses,stop_counts=stops,
      caller_sites=len(caller&visited),caller_sites_total=len(caller),caller_branch_outcomes=len(ce),caller_branch_outcomes_total=len(cc)*2,
      chain_sites=len(visited),chain_sites_total=len(decoded),chain_branch_outcomes=len(edges),chain_branch_outcomes_total=len(cond)*2,
      masked_flags=0,emulator_state_substitutions=0,uncovered_sites=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
      uncovered_edges=[(f'{k}:{ip:04X}',hex(target)) for (k,ip) in sorted(cond) for target in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,target)) not in edges])
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
