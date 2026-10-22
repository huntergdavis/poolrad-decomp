#!/usr/bin/env python3
"""Original-byte CPU14/whole-RAM audit for U7F entry 2 and both callees."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as R
from overlay_inventory import find_units, parse_mz
from u25_string_cores_audit import Cpu, NAMES, REGS, physical, put16, far_pointer
from u7f_record_subtract_0020_audit import source_audit as callee_source
from u5e_party_value_2879_audit import EXE_SHA, OVR_SHA

ROOT=Path(__file__).resolve().parents[2]
SIZE=1<<20
ENTRY=0x508
CALLER_SHA='a9f79cf66963228814271af27cb8b03f11fd48cab64f778db4f2a2a35efb7efd'

def source():
    p=ROOT/'resources/DOS/POOLRAD';exe=p/'START.EXE';ovr=p/'GAME.OVR'
    e,o=exe.read_bytes(),ovr.read_bytes()
    assert sha256(e).hexdigest()==EXE_SHA and sha256(o).hexdigest()==OVR_SHA
    helper,hi=callee_source(ROOT,exe,ovr)
    unit=next(u for u in find_units(e[parse_mz(e)['header_bytes']:],o) if u['stub_para']==0x7F)
    assert unit['entries'][2]==ENTRY and unit['ovr_offset']==0x24C89
    assert [f for f in unit['fixups'] if ENTRY<=f<0x5E0]==[0x5C4]
    bodies={('u7f',0x20):helper,('u7f',ENTRY):o[0x25191:0x25269],
            ('sys',0x16A8):e[0xAC76+0x16A8:0xAC76+0x16BC]}
    assert sha256(bodies['u7f',ENTRY]).hexdigest()==CALLER_SHA
    assert sha256(bodies['sys',0x16A8]).hexdigest()=='f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7'
    groups={key:list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,key[1])) for key,data in bodies.items()}
    assert len(groups['u7f',ENTRY])==79
    assert [(i.address,i.op_str) for i in groups['u7f',ENTRY] if 'call' in i.mnemonic]==[(0x5A8,'0x20'),(0x5C1,'0xaf8, 0x16a8')]
    assert groups['u7f',ENTRY][-1].address==0x5DF and groups['u7f',ENTRY][-1].mnemonic=='retf'
    return bodies,{(kind,i.address):i for (kind,_),ins in groups.items() for i in ins}

CHECKER_ROWS = ('OVL_007F_0508',)


def library(directory):
    p=directory/'clear.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/f) for f in ('overlay_u7f_clear_0508.c','overlay_u7f_record_subtract_0020.c','system_runtime_leaves_cpu.c')]+['-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u7f_clear_0508
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_size_t,ctypes.c_size_t];f.restype=ctypes.c_int
    return f

def fixture(*,types=(0,),values=None,total=0xFFFF,base=0x8000,cs=0x3000,system=0x4000,ss=0x2000,ds=0x5000,sp=0xF000,flags=0x602,records=None,cycle=False,seed=0,**unused):
    del unused
    rng=random.Random(seed);m=bytearray([0xA5])*SIZE
    c=Cpu(*(rng.randrange(65536) for _ in NAMES));c.cs,c.ip,c.ds,c.ss,c.sp,c.bp,c.flags=cs,ENTRY,ds,ss,sp,0x7777,flags
    records=records if records is not None else [(ds,0x1000+0x200*i) for i in range(len(types))]
    assert len(records)==len(types)
    for i,((seg,off),kind) in enumerate(zip(records,types)):
        m[physical(seg,(off+0x84)&65535)]=kind
        for j in range(7):put16(m,seg,(off+0x88+2*j)&65535,values[j] if values else j+1)
        put16(m,seg,(off+0x102)&65535,base)
        nxt=records[(i+1)%len(records)] if cycle or i+1<len(records) else (0,0)
        # Actual next-pointer reads are two separately addressed words.
        put16(m,seg,(off+0x104)&65535,nxt[1]);put16(m,seg,(off+0x106)&65535,nxt[0])
    for i in range(7):
        put16(m,ds,0x67F4+4*i,total&65535);put16(m,ds,0x67F6+4*i,total>>16)
    seg,off=records[0] if records else (0,0)
    far_pointer(m,ds,0x5D96,seg,off)
    put16(m,ss,sp,0x9000);put16(m,ss,sp+2,0xA000)
    return m,c,system

def install(m,c,system,bodies,decoded):
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        for (k,ip),i in decoded.items():
            if k==kind and start<=ip<start+len(data) and i.mnemonic=='lcall':blob[ip-start+3:ip-start+5]=system.to_bytes(2,'little')
        for i,v in enumerate(blob):m[physical(system if kind=='sys' else c.cs,start+i)]=v


def compare(label,state,bodies,decoded,function,budgets=(32,256)):
    m,c,system=state;install(m,c,system,bodies,decoded)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for n,r in zip(NAMES,REGS):uc.reg_write(r,getattr(c,n))
    visited,edges=set(),set();previous=[None];remaining=list(budgets);stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        del address,size,user
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535;kind='u7f' if cs==c.cs else 'sys' if cs==system else None;key=(kind,ip)
        assert key in decoded,(label,'unexpected PC',hex(cs),hex(ip))
        index=0 if key==('u7f',0x520) else 1 if key==('u7f',0x54D) else None
        if index is not None:
            if not remaining[index]:stop[0]=index+2;uc.emu_stop();return
            remaining[index]-=1
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key);previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):
            calls.append((cs,ip+ins.size))
        if ins.mnemonic=='retf':stop[0]='ret';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook)
    status=0
    while True:
        stop[0]=None;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=2000000)
        assert stop[0] is not None,(label,'original instruction limit')
        if stop[0]!='ret':status=stop[0];break
        stepping[0]=True;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1);stepping[0]=False
        if not calls:break
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=calls.pop():status=4;break
    for (kind,start),data in bodies.items():
        seg=system if kind=='sys' else c.cs
        assert all(backing[physical(seg,start+i)]==m[physical(seg,start+i)] for i in range(len(data))),(label,'code modified: out of contract')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c)
    got=function(ram,SIZE,ctypes.byref(actual),system,*budgets)
    assert got==status,(label,'status',got,status)
    assert bytes(ram)==bytes(backing),(label,'RAM',[(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12])
    for n,r in zip(NAMES,REGS):assert getattr(actual,n)==(uc.reg_read(r)&65535),(label,n,hex(getattr(actual,n)),hex(uc.reg_read(r)&65535))
    return visited,edges,status,actual


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');args=p.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded=source();visited=set();edges=set();statuses={};stops={};count=0
    with tempfile.TemporaryDirectory() as tmp:
        f=library(Path(tmp))
        def run(label,budgets=(32,256),**kw):
            nonlocal count
            v,e,s,c=compare(label,fixture(**kw),bodies,decoded,f,budgets)
            visited.update(v);edges.update(e);statuses[str(s)]=statuses.get(str(s),0)+1;count+=1
            if s:stops[f'{s}:{c.cs:04X}:{c.ip:04X}']=stops.get(f'{s}:{c.cs:04X}:{c.ip:04X}',0)+1
            if count%50==0:print('Compared',count,label,flush=True)
        run('empty',types=())
        for kind in (0,1,179,255) if args.quick else range(256):run(f'type-{kind}',types=(kind,))
        for types in ((0,179,1),(1,0,179),(255,128,127),(0,)*10,(179,)*10):run('multi-'+str(types),types=types)
        for total in (0,1,0x7FFFFFFF,0x80000000,0xFFFFFFFF):
            for value in (0,1,0x7FFF,0x8000,0xFFFF):run(f'arithmetic-{total}-{value}',total=total,values=[value]*7,base=value)
        for sp in (0x14,0x15,0x16,0x17,0xFFFB,0xFFFC):run(f'stack-{sp:x}',sp=sp)
        for off in (0,1,0xFEF8,0xFEFB,0xFEFC,0xFEFD,0xFEFE,0xFEFF,0xFF00,0xFF77,0xFF78,0xFF79,0xFFFD,0xFFFE,0xFFFF):
            run(f'record-seam-{off:x}',records=[(0x5000,off)])
        for nodes in (1,2,3,5):run(f'node-budget-{nodes}',types=(0,179,1),budgets=(nodes,64))
        for items in (1,2,6,7,8,13,14,15):run(f'item-budget-{items}',types=(0,179,0),budgets=(32,items))
        run('cycle-skipped',types=(1,),cycle=True,budgets=(4,64))
        run('cycle-active',cycle=True,budgets=(4,64))
        for target in (0x67F4,0x67F6,0x67F8,0x5D96,0x6DC0):
            run(f'data-alias-{target:x}',records=[(0x5000,(target-0x88)&65535)],budgets=(8,64))
        for target in (0xEFD0,0xEFEC,0xEFEE,0xEFF0,0xEFF6,0xEFF8,0xEFF9,0xEFFA,0xEFFC,0xEFFE,0xF000,0xF002):
            run(f'subtract-stack-alias-{target:x}',records=[(0x2000,(target-0x102)&65535)],budgets=(4,32))
        for off in (0xEF68,0xEF72,0xEF78):
            run(f'clear-stack-alias-{off:x}',records=[(0x2000,off)],budgets=(4,32))
        run('A20-record',records=[(0xFFFF,0xFF0D)])
        run('A20-stack',ss=0xFFFF,sp=0x200)
        for cs,system in ((0x6000,0x7000),(0xFFFF,0x4000)):
            run(f'relocated-{cs:x}',cs=cs,system=system)
        rng=random.Random(0x7F0508)
        for i in range(12 if args.quick else 250):
            run(f'random-{i}',types=tuple(rng.choice((0,179,1,255)) for _ in range(rng.randrange(6))),values=[rng.randrange(65536) for _ in range(7)],total=rng.randrange(1<<32),base=rng.randrange(65536),seed=i,flags=rng.choice((2,0x602,0x8D7,0xCD7)))
    caller={key for key in decoded if key[0]=='u7f' and ENTRY<=key[1]<0x5E0}
    assert set(decoded)<=visited,sorted(set(decoded)-visited)
    assert len(edges)==8,edges
    report=dict(target='OVL_007F_0508',cases=count,cpu_words=14,ram_bytes=SIZE,status_counts=statuses,stop_counts=stops,
                caller_sites=len(caller&visited),caller_sites_total=len(caller),chain_sites=len(visited),chain_sites_total=len(decoded),
                branch_outcomes=len(edges),branch_outcomes_total=8,masked_flags=0,emulator_state_substitutions=0)
    if args.output:args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
