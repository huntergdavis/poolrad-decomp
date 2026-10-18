#!/usr/bin/env python3
"""Original-code/CPU14/whole-RAM oracle for the full U25 script initializer."""
import argparse
import ctypes
import faulthandler
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as R
from u25_loader_0000_audit import source_audit as loader_sources
from overlay_inventory import find_units, parse_mz
from u25_string_cores_audit import Cpu, NAMES, REGS, LOOPS, physical, put16, far_pointer

ROOT=Path(__file__).resolve().parents[2]
SIZE=1<<20
ENTRY=0x1B6
CALLER_SHA='b1bf2753d995d35feb5b4586b09a6dd9c6231183220062f790ef5ff4501b4eb0'


def source():
    p=ROOT/'resources/DOS/POOLRAD';exe=p/'START.EXE';ovr=p/'GAME.OVR'
    bodies,groups=loader_sources(exe,ovr)
    e,o=exe.read_bytes(),ovr.read_bytes()
    unit=next(u for u in find_units(e[parse_mz(e)['header_bytes']:],o) if u['stub_para']==0x25)
    assert unit['entries'][3]==ENTRY
    assert all(off in unit['fixups'] for off in (0x1EE,0x21F))
    for key,data,digest in [(('u25',ENTRY),ovr.read_bytes()[0x7689:0x77F5],CALLER_SHA),
                            (('sys',0x16A8),exe.read_bytes()[0xAC76+0x16A8:0xAC76+0x16BC],
                             'f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7')]:
        assert sha256(data).hexdigest()==digest
        bodies[key]=data;groups[key]=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,key[1]))
        print(key,len(data),len(groups[key]),sha256(data).hexdigest())
    assert len(groups['u25',ENTRY])==135
    calls=[(i.address,i.op_str) for i in groups['u25',ENTRY] if i.mnemonic in ('call','lcall')]
    assert calls==[(0x1EB,'0xaf8, 0x16a8'),(0x21C,'0xaf8, 0x16a8')]+[(a,t) for a in (0x251,0x25D,0x267,0x273,0x27D,0x289,0x293,0x29F,0x2A9,0x2B5) for t in ('0' if a in (0x251,0x267,0x27D,0x293,0x2A9) else '0x6af',)]
    return bodies,{(kind,i.address):i for (kind,_),ins in groups.items() for i in ins}


def library(directory):
    p=directory/'init.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/f) for f in ('overlay_u25_init_01b6.c','overlay_u25_loader_0000.c','overlay_u25_string_cores.c','system_runtime_leaves_cpu.c')]+['-o',str(p)],check=True)
    lib=ctypes.CDLL(str(p));f=lib.pr_u25_script_init_01b6
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_size_t,ctypes.c_size_t,ctypes.c_size_t];f.restype=ctypes.c_int
    return f


def operand(tag,value=0x34,high=0x12):
    if tag in (1,2,3,0x81):return bytes((tag,value,high))
    if tag==0x80:return bytes((tag,3,4,32,63))
    return bytes((tag,value))


def fixture(*,operands=None,restored=0,seed=0,cs=0x3000,system=0x4000,ss=0x2000,ds=0x5000,sp=0xF000,flags=0x602,window0=None,window1=None,script=None,unterminated=False,**unused):
    del unused
    rng=random.Random(seed);m=bytearray([0xA5])*SIZE
    c=Cpu(*(rng.randrange(65536) for _ in NAMES));c.cs,c.ip,c.ds,c.ss,c.sp,c.bp,c.flags=cs,ENTRY,ds,ss,sp,0x7777,flags
    w0=window0 or (ds,0x1000);w1=window1 or (ds,0x3000);script=script or (ds,0x9000)
    for off,(seg,ptr) in zip((0x49D2,0x49D6,0x49DA,0x49DE,0x5D92),(w0,w1,(ds,0x5000),script,(ds,0xA000))):far_pointer(m,ds,off,seg,ptr)
    for seg,off in (w0,w1,(ds,0x5000)):
        for i in range(256 if unterminated else 12):put16(m,seg,(off+2*i)&65535,(i%95)+32)
        if not unterminated:put16(m,seg,(off+24)&65535,0)
    for off,b in enumerate(b'abc\0'):m[physical(script[0],script[1]+0x700+off)]=b
    m[physical(ds,0xA000):physical(ds,0xA000)+5]=bytes((4,))+b'Test'
    m[physical(ds,0x49F8)]=restored
    ops=operands or [operand(1,i+1,0x12) for i in range(5)]
    assert len(ops)==5
    stream=b''.join(o+b'\x7e' for o in ops)
    for i,b in enumerate(stream):m[physical(script[0],script[1]+1+i)]=b
    put16(m,ss,sp,0x9000);put16(m,ss,sp+2,0xA000)
    return m,c,system


def install(m,c,system,bodies,decoded):
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        for (k,ip),i in decoded.items():
            if k==kind and start<=ip<start+len(data) and i.mnemonic=='lcall':blob[ip-start+3:ip-start+5]=system.to_bytes(2,'little')
        for i,v in enumerate(blob):m[physical(system if kind=='sys' else c.cs,start+i)]=v


def compare(label,state,bodies,decoded,function,budgets=(128,32,512)):
    m,c,system=state;install(m,c,system,bodies,decoded)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for n,r in zip(NAMES,REGS):uc.reg_write(r,getattr(c,n))
    visited,edges=set(),set();previous=[None];remaining=list(budgets);stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        del address,size,user
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535;kind='u25' if cs==c.cs else 'sys' if cs==system else None;key=(kind,ip)
        assert key in decoded,(label,'unexpected PC',hex(cs),hex(ip))
        if kind=='u25' and ip in (0,0x1153,0x12FB):
            sp=uc.reg_read(R.UC_X86_REG_SP);lo,hi=(0x22A,0xFFFA) if ip==0 else (0x218,0xFFF8)
            if not lo<=sp<=hi:stop[0]=5;uc.emu_stop();return
        index=0 if key in (('u25',0x2D7),('u25',0x2FC)) else 1 if key==('u25',0x22) else 2 if kind=='u25' and ip in LOOPS else None
        if index is not None:
            if not remaining[index]:stop[0]=index+2;uc.emu_stop();return
            remaining[index]-=1
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key);previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):
            calls.append((cs,ip+ins.size))
            if ins.mnemonic=='call' and ins.op_str=='0':remaining[1]=budgets[1]
            if ins.mnemonic=='call' and ins.op_str in ('0x1153','0x12fb'):remaining[2]=budgets[2]
        if ins.mnemonic=='retf':stop[0]='ret';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook)
    status=0
    while True:
        stop[0]=None;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=2000000)
        assert stop[0] is not None,(label,'original instruction limit')
        if stop[0]!='ret':status=stop[0];break
        stepping[0]=True;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1);stepping[0]=False
        if not calls:break
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=calls.pop():status=6;break
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
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);args=p.parse_args()
    bodies,decoded=source();visited=set();edges=set();statuses={};stops={};count=0
    with tempfile.TemporaryDirectory() as tmp:
        f=library(Path(tmp))
        def run(label,budgets=(128,32,512),**kw):
            nonlocal count
            print('BEGIN',count+1,label,flush=True)
            faulthandler.dump_traceback_later(45,repeat=True)
            v,e,s,c=compare(label,fixture(**kw),bodies,decoded,f,budgets);faulthandler.cancel_dump_traceback_later();visited.update(v);edges.update(e);statuses[str(s)]=statuses.get(str(s),0)+1;count+=1
            if s:stops[f'{s}:{c.ip:04X}']=stops.get(f'{s}:{c.ip:04X}',0)+1
            if count%50==0:print('Compared',count,label,flush=True)
        run('default')
        for restored in (1,2,128,255):run(f'restored-{restored}',restored=restored)
        for tag in range(256):run(f'tag-{tag:02x}',operands=[operand(tag)]*5,restored=tag%2)
        for address in (0,0x48FF,0x4900,0x4CFF,0x4D00,0x6B00,0x6B01,0x6E00,0x96FF,0x9700,0xA000,0xC000,0xFFFF):
            run(f'address-{address:04x}',operands=[operand(0x81,address&255,address>>8)]*5)
        for n in (0,1,2,3,4,7,12,31,64,127,255):
            for value in (0,4,31,32,63,64,127,128,255):run(f'packed-{n}-{value}',operands=[bytes((0x80,n))+bytes([value])*n]*5)
        for sp in (0x236,0x237,0xFFFB,0xFFFC):run(f'stack-{sp:04x}',sp=sp,operands=[operand(0x80)]*5)
        for budget in (1,31,32,33,41,42):run(f'clear-limit-{budget}',budgets=(budget,32,512))
        run('packed-limit',operands=[bytes((0x80,12))+bytes([255])*12]*5,budgets=(128,32,2))
        for addr in (0x4900,0x6B01,0x9700,0xC000):run(f'address-limit-{addr:x}',operands=[operand(0x81,addr&255,addr>>8)]*5,unterminated=True,budgets=(128,32,2))
        for off in (0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'window-seam-{off:x}',window0=(0x5000,off))
        for off in (0x49F8-0x1CC,0x49D2-0x200,0x7046-0x200,0x49DE-0x200):run(f'window-alias-{off:x}',window0=(0x5000,off))
        for sp in (0x7010,0x7090,0x72F2,0x8500):run(f'stack-data-{sp:x}',ss=0x5000,sp=sp,budgets=(64,8,64))
        rng=random.Random(0x2501B6)
        for i in range(20 if args.quick else 400):
            ops=[]
            for _ in range(5):
                tag=rng.choice((0,1,2,3,0x80,0x81,255));lo=rng.randrange(256);hi=rng.randrange(256)
                if tag==0x80:
                    n=rng.randrange(32);ops.append(bytes((tag,n))+rng.randbytes(n))
                else:ops.append(operand(tag,lo,hi))
            run(f'random-{i}',operands=ops,seed=i,restored=rng.randrange(2),flags=rng.choice((2,0x602,0x8D7,0xCD7)))
    caller={key for key in decoded if key[0]=='u25' and ENTRY<=key[1]<0x322};conditional={k for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    caller_edges={e for e in edges if e[0] in caller}
    assert caller<=visited,sorted(caller-visited)
    assert len(caller_edges)==6,caller_edges
    report=dict(target='OVL_0025_01B6',cases=count,cpu_words=14,ram_bytes=SIZE,status_counts=statuses,stop_counts=stops,
                caller_sites=len(caller&visited),caller_sites_total=len(caller),caller_branch_outcomes=len(caller_edges),caller_branch_outcomes_total=6,
                chain_sites=len(visited),chain_sites_total=len(decoded),chain_branch_outcomes=len(edges),chain_branch_outcomes_total=2*len(conditional),
                masked_flags=0,emulator_state_substitutions=0,uncovered_sites=[f'{k}:{a:04X}' for k,a in sorted(set(decoded)-visited)])
    if args.output:args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))

if __name__=='__main__':main()
