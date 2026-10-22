#!/usr/bin/env python3
"""Original-code CPU14/whole-RAM audit for U3C6 entry5 and both callees."""
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
from u3c2_line_init_01a0_audit import Cpu,NAMES,REGS,physical,put16
from u5e_party_value_2879_audit import EXE_SHA,OVR_SHA
ROOT=Path(__file__).resolve().parents[2]
SIZE=1<<20
ENTRY=0x3A0
DEFAULT=(512,4096)
SOURCES=('overlay_u3c6_rebuild_03a0','overlay_u3c6_table_pair_0000','system_runtime_leaves_cpu')

def source():
    p=ROOT/'resources/DOS/POOLRAD';e,o=(p/'START.EXE').read_bytes(),(p/'GAME.OVR').read_bytes()
    assert sha256(e).hexdigest()==EXE_SHA and sha256(o).hexdigest()==OVR_SHA
    units={u['stub_para']:u for u in find_units(e[parse_mz(e)['header_bytes']:],o)};u=units[0x3C6]
    assert u['entries'][5]==ENTRY and u['ovr_offset']==198458
    assert [f for f in u['fixups'] if ENTRY<=f<0x4BE]==[0x3B5]
    for start,end,site in ((0xE11,0x10A7,0x103E),(0x10A7,0x1298,0x1286)):
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(o[u['ovr_offset']+start:u['ovr_offset']+end],start))
        j=next(j for j,i in enumerate(ins) if i.address==site)
        assert ins[j].mnemonic=='call' and ins[j].op_str=='0x3a0' and ins[j-1].mnemonic=='push' and ins[j-1].op_str=='cs'
    for unit,start,end,site in ((0x5E,0x712,0x8B2,0x83F),(0x9E,0xF06,0x10D0,0xFAE),(0x33,0x19F5,0x1D8B,0x1A27),(0x33,0x19F5,0x1D8B,0x1D74)):
        caller=units[unit];ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(o[caller['ovr_offset']+start:caller['ovr_offset']+end],start))
        i=next(i for i in ins if i.address==site)
        assert i.mnemonic=='lcall' and i.op_str=='0x3e0, 0x39' and site+3 in caller['fixups']
    bodies={('u3c6',ENTRY):o[u['ovr_offset']+ENTRY:u['ovr_offset']+0x4BE],
            ('u3c6',0):o[u['ovr_offset']:u['ovr_offset']+0x68],
            ('sys',0x16A8):e[0xAC76+0x16A8:0xAC76+0x16BC]}
    pins={('u3c6',ENTRY):(122,'50c688892105995b01a842c1e26412f1b4c221e58a1be29ea1fa41bb7abfe400'),('u3c6',0):(41,'05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207'),('sys',0x16A8):(7,'f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7')}
    decoded={}
    for key,data in bodies.items():
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,key[1]));count,digest=pins[key]
        assert len(ins)==count and (not digest or sha256(data).hexdigest()==digest)
        decoded.update({(key[0],i.address):i for i in ins});print(key,len(data),len(ins),sha256(data).hexdigest())
    assert [(i.address,i.op_str) for (kind,ip),i in decoded.items() if kind=='u3c6' and ENTRY<=ip<0x4BE and 'call' in i.mnemonic]==[(0x3B2,'0xaf8, 0x16a8'),(0x40C,'0')]
    return bodies,decoded

CHECKER_ROWS = ('OVL_03C6_03A0',)


def library(directory):
    p=directory/'rebuild.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES]+['-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u3c6_rebuild_03a0
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_size_t,ctypes.c_size_t];f.restype=ctypes.c_int
    return f

def fixture(*,party=((10,10,1),(12,12,1)),shape=((0,0),(1,0),(0,1),(-1,0)),origin=(3,4),
            cs=0x3000,system=0x6000,ss=0x2000,ds=0x5000,sp=0xF000,flags=0x602,seed=0,
            map_offset=0x8000,map_segment=None,**unused):
    del unused
    rng=random.Random(seed);m=bytearray([0xA5])*SIZE
    c=Cpu(*(rng.randrange(65536) for _ in NAMES));c.cs,c.ip,c.ss,c.sp,c.bp,c.ds,c.flags=cs,ENTRY,ss,sp,0x7777,ds,flags
    for t in range(256):
        for i,(x,y) in enumerate(shape):m[physical(ds,0x2844+8*t+2*i)]=x&255;m[physical(ds,0x2845+8*t+2*i)]=y&255
    m[physical(ds,0x5F2A)]=len(party)
    for i,(x,y,t) in enumerate(party,1):
        m[physical(ds,0x5F27+4*i)]=x&255;m[physical(ds,0x5F28+4*i)]=y&255;m[physical(ds,0x5F2A+4*i)]=t
    segment=ds if map_segment is None else map_segment
    put16(m,ds,0x6716,map_offset);put16(m,ds,0x6718,segment)
    m[physical(segment,(map_offset+2)&65535)]=origin[0]&255;m[physical(segment,(map_offset+3)&65535)]=origin[1]&255
    put16(m,ss,sp,0x9000);put16(m,ss,sp+2,0xA000)
    return m,c,system

def install(m,c,system,bodies,decoded):
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        if kind=='u3c6' and start==ENTRY:blob[0x3B5-start:0x3B7-start]=system.to_bytes(2,'little')
        for i,v in enumerate(blob):m[physical(system if kind=='sys' else c.cs,start+i)]=v

def compare(label,state,bodies,decoded,f,budgets=DEFAULT):
    m,c,system=state;install(m,c,system,bodies,decoded)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for n,r in zip(NAMES,REGS):uc.reg_write(r,getattr(c,n))
    visited,edges=set(),set();previous=[None];remaining=list(budgets);stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        del address,size,user
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535
        kind='u3c6' if cs==c.cs else 'sys' if cs==system else None;key=kind,ip
        assert key in decoded,(label,'unexpected PC',hex(cs),hex(ip))
        index=0 if key==('u3c6',0x3CF) else 1 if key==('u3c6',0x3ED) else None
        if index is not None:
            if not remaining[index]:stop[0]=index+2;uc.emu_stop();return
            remaining[index]-=1
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key);previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):calls.append((cs,ip+ins.size))
        if ins.mnemonic=='retf':stop[0]='ret';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook);status=0
    while True:
        stop[0]=None;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=2000000)
        assert stop[0] is not None,(label,'original instruction limit')
        if stop[0]!='ret':status=stop[0];break
        stepping[0]=True;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1);stepping[0]=False
        if not calls:break
        expected=calls.pop()
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=expected:status=4;break
    for (kind,start),data in bodies.items():
        seg=system if kind=='sys' else c.cs
        assert all(backing[physical(seg,start+i)]==m[physical(seg,start+i)] for i in range(len(data))),(label,'code modified')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c);got=f(ram,SIZE,ctypes.byref(actual),system,*budgets)
    assert got==status,(label,'status',got,status)
    assert bytes(ram)==bytes(backing),(label,'RAM',[(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12])
    for n,r in zip(NAMES,REGS):assert getattr(actual,n)==uc.reg_read(r)&65535,(label,n,hex(getattr(actual,n)),hex(uc.reg_read(r)&65535))
    return visited,edges,status,actual

def specifications(quick=False):
    cases=[('default',{}),('empty',dict(party=())),('inactive',dict(party=((10,10,0),))),('negative-shape',dict(shape=((-1,0),)*4)),('four-valid',dict(shape=((0,0),(1,0),(0,1),(1,1)))),('colliding-slots',dict(party=((10,10,1),(10,10,1))))]
    for t in ((0,1,2,127,128,255) if quick else range(256)):cases.append((f'type-{t}',dict(party=((10,10,t),))))
    for v in (0,1,49,50,127,128,255):
        for axis in (0,1):
            p=[10,10,1];p[axis]=v;cases.append((f'coord-{axis}-{v}',dict(party=(tuple(p),))))
            origin=[3,4];origin[axis]=v;cases.append((f'origin-{axis}-{v}',dict(origin=tuple(origin))))
    for x,y in ((127,127),(0,128),(0,255),(128,0),(255,255),(127,255)):
        cases.append((f'shape-{x}-{y}',dict(shape=((x,y),)*4)))
    for n in (3,5,72,73,108,109,254,255):cases.append((f'count-{n}',dict(party=tuple((i%50,i%25,1) for i in range(n)))))
    for sp in (0x1C,0x1D,0xFFFB,0xFFFC):cases.append((f'stack-{sp:x}',dict(sp=sp)))
    for off in (0xFFFD,0xFFFE,0xFFFF,0x60DB,0x6048,0x6090):cases.append((f'map-{off:x}',dict(map_offset=off)))
    for sp in (0x5F2C,0x5F40,0x604A,0x604F,0x60D0,0x60DB,0x60E0,0x65B9,0x65BD,0x65C1,0x6716,0x671B,0x2850):cases.append((f'alias-{sp:x}',dict(ss=0x5000,sp=sp,budgets=(256,1024))))
    for budget in (1,2,3):cases.append((f'slot-limit-{budget}',dict(party=((10,10,1),)*5,budgets=(budget,4096))))
    for budget in (1,2,3,4,5,7):cases.append((f'cell-limit-{budget}',dict(budgets=(512,budget))))
    cases += [('A20-stack',dict(ss=0xFFFF,sp=0x300)),('A20-code',dict(cs=0xFFFF)),('A20-data',dict(ds=0xFFFF)),('relocated',dict(cs=0x7000,system=0x8000))]
    rng=random.Random(0x3C603A0)
    for i in range(12 if quick else 100):
        cases.append((f'random-{i}',dict(party=tuple((rng.randrange(256),rng.randrange(256),rng.randrange(256)) for _ in range(rng.randrange(9))),origin=(rng.randrange(256),rng.randrange(256)),shape=tuple((rng.randrange(256),rng.randrange(256)) for _ in range(4)),seed=i,flags=rng.choice((2,0x602,0x8D7,0xCD7)))))
    return cases

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded=source();visited=set();edges=set();statuses={};stops={};count=0
    with tempfile.TemporaryDirectory() as tmp:
        f=library(Path(tmp))
        for label,kw in specifications(a.quick):
            kw=dict(kw);budgets=kw.pop('budgets',DEFAULT);print('BEGIN',count+1,label,flush=True)
            v,e,s,c=compare(label,fixture(**kw),bodies,decoded,f,budgets)
            visited.update(v);edges.update(e);statuses[str(s)]=statuses.get(str(s),0)+1;count+=1
            if s:stops[f'{s}:{c.cs:04X}:{c.ip:04X}']=stops.get(f'{s}:{c.cs:04X}:{c.ip:04X}',0)+1
    caller={k for k in decoded if k[0]=='u3c6' and ENTRY<=k[1]<0x4BE};cond={k for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    ce={e for e in edges if e[0] in caller};cc=cond&caller
    assert caller<=visited,sorted(caller-visited)
    report=dict(target='OVL_03C6_03A0',cases=count,cpu_words=14,ram_bytes=SIZE,status_counts=statuses,stop_counts=stops,
      caller_sites=len(caller&visited),caller_sites_total=len(caller),caller_branch_outcomes=len(ce),caller_branch_outcomes_total=len(cc)*2,
      chain_sites=len(visited),chain_sites_total=len(decoded),chain_branch_outcomes=len(edges),chain_branch_outcomes_total=len(cond)*2,
      masked_flags=0,emulator_state_substitutions=0,uncovered_sites=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
      uncovered_edges=[(f'{k}:{ip:04X}',hex(target)) for (k,ip) in sorted(cond) for target in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,target)) not in edges])
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
