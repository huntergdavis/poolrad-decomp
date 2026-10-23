#!/usr/bin/env python3
"""Original-code CPU14/whole-RAM audit of complete U3C2 entry6."""
from loaded_stub_fixtures import install_stub, loaded_sites
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
from u3c2_line_check_0422_audit import source as line_sources
from u3c2_line_init_01a0_audit import Cpu,NAMES,REGS,physical,put16
ROOT=Path(__file__).resolve().parents[2]
SIZE=1<<20
ENTRY=0x909
LOOPS=(0x918,0x993,0x9BA,0xA48,0xA61,0xB73)
SOURCES=('overlay_u3c2_caller_0909','overlay_u3c6_table_pair_0000',
         'overlay_u3c2_record_sort_002e_cpu','system_runtime_leaves_cpu',
         'overlay_u3c2_entry4_057b','overlay_u3c2_entry5_0422',
         'overlay_u3c2_line_init_01a0','overlay_u3c2_sign_entry1_0000','overlay_u3c2_entry3_02cc')
PINS=(('caller',0x2E,0x1A0,158,'81e890a18c92bd83b78b50a33c1295975183a2706ec5b03b52e7cf550544ca3f'),
      ('caller',0x57B,0x909,410,'59750d0bfa553507f37af613b31ff0f733f2c63c584d768ed42b180122d1e2ae'),
      ('caller',0x909,0xBD9,297,'38a673986257d14dbe4da3ede438a98c0b516884c6fc92c79228088a68dba9ed'),
      ('pair',0,0x68,41,'05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207'),
      ('sys',0xA3F,0xA57,9,'5bf8af52bdcd4af1b7e73b160b3ee02e6de7812b7e686aae02f73d9ccbc061aa'))

def source():
    p=ROOT/'resources/DOS/POOLRAD';e,o=(p/'START.EXE').read_bytes(),(p/'GAME.OVR').read_bytes()
    old,groups=line_sources(p/'START.EXE',p/'GAME.OVR')
    bodies={('caller',a):b for a,b in old.items()}
    units={u['stub_para']:u for u in find_units(e[parse_mz(e)['header_bytes']:],o)}
    assert units[0x3C2]['entries'][6]==ENTRY
    assert [f for f in units[0x3C2]['fixups'] if ENTRY<=f<0xBD9]==[0x92D,0x9DB]
    for kind,start,end,count,digest in PINS:
        data=e[0xAC76+start:0xAC76+end] if kind=='sys' else o[units[0x3C2 if kind=='caller' else 0x3C6]['ovr_offset']+start:units[0x3C2 if kind=='caller' else 0x3C6]['ovr_offset']+end]
        assert not digest or sha256(data).hexdigest()==digest
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,start))
        assert not count or len(ins)==count
        bodies[kind,start]=data
        print(kind,hex(start),len(data),len(ins),sha256(data).hexdigest())
    decoded={(kind,i.address):i for (kind,a),data in bodies.items() for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,a)}
    calls=[(i.address,i.op_str) for (kind,a),i in decoded.items() if kind=='caller' and ENTRY<=a<0xBD9 and 'call' in i.mnemonic]
    assert calls==[(0x92A,'0x3e0, 0x25'),(0x9D8,'0x3e0, 0x25'),(0xAA2,'0x57b'),(0xAF5,'0x422'),(0xBA4,'0x57b'),(0xBD0,'0x2e')]
    return bodies,decoded

CHECKER_ROWS = ('OVL_03C2_0909',)


def library(directory):
    p=directory/'caller.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES]+['-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u3c2_caller_0909
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_size_t,ctypes.c_size_t,ctypes.c_uint];f.restype=ctypes.c_int
    return f

def fixture(*,origin=(10,10),party=((12,12,1),),kind=1,direction=8,radius=100,
            shape=((0,0),(1,0),(0,1),(-1,0)),mode=1,heights=(0,0),
            cs=0x3000,pair=0x4000,system=0x6000,ss=0x2000,ds=0x5000,
            sp=0xF000,flags=0x602,seed=0,map_offset=0x8000,**unused):
    del unused
    rng=random.Random(seed);m=bytearray([0xA5])*SIZE
    c=Cpu(*(rng.randrange(65536) for _ in NAMES));c.cs,c.ip,c.ss,c.sp,c.bp,c.ds,c.flags=cs,ENTRY,ss,sp,0x7777,ds,flags
    # Synthetic coordinate offsets for every nonzero shape type.
    for t in range(256):
        for i,(x,y) in enumerate(shape):
            m[physical(ds,0x2844+8*t+2*i)]=x&255;m[physical(ds,0x2845+8*t+2*i)]=y&255
    for i,v in enumerate((0,1,1,1,0,-1,-1,-1,0)):m[physical(ds,0x2736+i)]=v&255
    for i,v in enumerate((-1,-1,0,1,1,1,0,-1,0)):m[physical(ds,0x273F+i)]=v&255
    for i in range(64):
        m[physical(ds,0x2745+4*i)]=heights[0];m[physical(ds,0x2746+4*i)]=heights[1]
    for i in range(9):m[physical(ds,0x25C0+i)]=0
    for i in range(0x1800):m[physical(ds,map_offset+i)]=0
    m[physical(ds,map_offset+6)]=mode
    m[physical(ds,0x5F2A)]=len(party)
    for i,(x,y,t) in enumerate(party,1):
        m[physical(ds,0x5F27+4*i)]=x&255;m[physical(ds,0x5F28+4*i)]=y&255
        m[physical(ds,0x5F2A+4*i)]=t
    # Seven words: map offset/segment, shape kind, direction, radius, origin Y/X.
    for i,w in enumerate((0x9000,0xA000,map_offset,ds,kind,direction,radius,origin[1]&255,origin[0]&255)):
        put16(m,ss,sp+2*i,w)
    return m,c,pair,system

def install(m,c,pair,system,bodies,decoded):
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        for (k,ip),ins in decoded.items():
            if k==kind and start<=ip<start+len(data) and ins.mnemonic=='lcall':
                off,seg=(0,pair) if ins.op_str=='0x3e0, 0x25' else (0xA3F,system)
                if ins.op_str=='0x3e0, 0x25':
                    install_stub(m,blob,start,ins,pair,0)
                else:
                    blob[ip-start+1:ip-start+5]=off.to_bytes(2,'little')+seg.to_bytes(2,'little')
        for i,v in enumerate(blob):m[physical({'caller':c.cs,'pair':pair,'sys':system}[kind],start+i)]=v

def compare(label,state,bodies,decoded,f,budgets=(20000,512,65536)):
    m,c,pair,system=state;install(m,c,pair,system,bodies,decoded)
    stubs=loaded_sites(m,dict(caller=c.cs,pair=pair,sys=system),decoded)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for n,r in zip(NAMES,REGS):uc.reg_write(r,getattr(c,n))
    visited,edges=set(),set();previous=[None];last=[None];remaining=list(budgets);stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        del address,size,user
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535
        if (cs,ip) in stubs:return
        kind='caller' if cs==c.cs else 'pair' if cs==pair else 'sys' if cs==system else None;key=(kind,ip)
        assert key in decoded,(label,'unexpected PC',hex(cs),hex(ip))
        if key==('caller',0x422):
            sp=uc.reg_read(R.UC_X86_REG_SP)
            if not 0x54<=sp<=0xFFE8:stop[0]=5;uc.emu_stop();return
            ss=uc.reg_read(R.UC_X86_REG_SS)
            if any((physical(ss,sp-i)-physical(c.cs,0))%SIZE<0x2E or 0x1A0<=(physical(ss,sp-i)-physical(c.cs,0))%SIZE<0x57B for i in range(1,85)):
                stop[0]=5;uc.emu_stop();return
        index=0 if kind=='caller' and ip in LOOPS else 1 if key==('caller',0x4DD) else 2 if kind=='caller' and ip in (0x72,0x56) and last[0] in (('caller',0x18E),('caller',0x199)) else None
        if index is not None:
            if not remaining[index]:stop[0]=index+2;uc.emu_stop();return
            remaining[index]-=1
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key);previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None;last[0]=key
        if ins.mnemonic in ('call','lcall'):
            target=(kind,int(ins.op_str,16)) if ins.mnemonic=='call' else ('pair',0) if ip in (0x92A,0x9D8) else ('sys',0xA3F)
            calls.append(((cs,ip+ins.size),target))
            if target==('caller',0x422):remaining[1]=budgets[1]
            if target==('caller',0x2E):remaining[2]=budgets[2]
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
            status=5 if any(t in (('caller',0x422),('caller',0x2E)) for _,t in calls) else 6;break
    for (kind,start),data in bodies.items():
        seg={'caller':c.cs,'pair':pair,'sys':system}[kind]
        assert all(backing[physical(seg,start+i)]==m[physical(seg,start+i)] for i in range(len(data))),(label,'code modified')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c)
    got=f(ram,SIZE,ctypes.byref(actual),pair,*budgets)
    assert got==status,(label,'status',got,status)
    assert bytes(ram)==bytes(backing),(label,'RAM',[(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12])
    for n,r in zip(NAMES,REGS):assert getattr(actual,n)==uc.reg_read(r)&65535,(label,n,hex(getattr(actual,n)),hex(uc.reg_read(r)&65535))
    return visited,edges,status,actual

def specifications(quick=False):
    cases=[('default',{}),('empty',dict(party=())),('zero-origin-type',dict(kind=0)),
           ('zero-target-type',dict(party=((12,12,0),))),
           ('blocked-map',dict(mode=0,heights=(0,255))),
           ('negative-shape',dict(shape=((-1,0),)*4))]
    for direction in ((0,1,2,3,4,5,6,7,8,9,254,255) if quick else range(256)):
        cases.append((f'direction-{direction}',dict(direction=direction)))
    for x,y in ((0,0),(1,1),(49,24),(50,25),(127,127),(128,128),(255,255),(10,10),(12,8),(8,12)):
        cases.append((f'origin-{x}-{y}',dict(origin=(x,y))))
        cases.append((f'target-{x}-{y}',dict(party=((x,y,1),))))
    for radius in (0,1,2,3,0x7FFF,0x8000,0xFFFF):
        for mode in (0,1):cases.append((f'radius-{radius}-{mode}',dict(radius=radius,mode=mode,heights=(0,255))))
    for party in (((20,20,1),(11,11,1)),((10,10,1),(10,10,1),(12,12,0)),((12,12,0),(11,11,1),(12,12,1)),tuple((i+4,i%20,1) for i in range(10))):
        cases.append(('party-'+str(party),dict(party=party)))
    for sp in (0x8E,0x8F,0xFFED,0xFFEE):cases.append((f'stack-{sp:x}',dict(sp=sp)))
    for budget in (1,3,4,5,8,12,16,20,24,25,32,48,64):cases.append((f'caller-limit-{budget}',dict(budgets=(budget,512,65536))))
    for budget in (1,2,3,4):cases.append((f'line-limit-{budget}',dict(budgets=(20000,budget,65536))))
    for budget in (1,2,3):cases.append((f'sort-limit-{budget}',dict(party=tuple((20-i,20-i,1) for i in range(5)),budgets=(20000,512,budget))))
    for sp in (0x5F20,0x5F40,0x6710,0x671B,0x671C,0x671D,0x6720,0x6724,0x6728,0x6730,0x6740,0x2800):
        cases.append((f'stack-data-{sp:x}',dict(ss=0x5000,sp=sp,budgets=(256,32,128))))
    cases += [('A20-stack',dict(ss=0xFFFF,sp=0x200)),('relocated',dict(cs=0x7000,pair=0x8000,system=0x9000)),('A20-code',dict(cs=0xFFFF))]
    rng=random.Random(0x3C20909)
    for i in range(12 if quick else 100):
        cases.append((f'random-{i}',dict(origin=(rng.randrange(55),rng.randrange(30)),
                     party=tuple((rng.randrange(55),rng.randrange(30),rng.choice((0,1,2,3))) for _ in range(rng.randrange(5))),
                     direction=rng.choice((0,1,2,3,4,5,6,7,8,255)),radius=rng.randrange(30),
                     shape=tuple((rng.randrange(-1,3),rng.randrange(-1,3)) for _ in range(4)),
                     seed=i,mode=rng.randrange(2),heights=(rng.randrange(3),rng.randrange(3)),
                     flags=rng.choice((2,0x602,0x8D7,0xCD7)))))
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
        from loaded_stub_checks import verify_refusals
        m,c,pair,system=fixture();install(m,c,pair,system,bodies,decoded)
        stub_guards=verify_refusals(m,c,lambda ram,state: f(ram,SIZE,ctypes.byref(state),pair,20000,512,65536),
                                    [(c.cs,p) for p in (0x92A,0x9D8)])
        print("LOADED_STUB_GUARDS",stub_guards,flush=True)
        for label,kw in specifications(a.quick):
            kw=dict(kw);budgets=kw.pop('budgets',(20000,512,65536))
            print('BEGIN',count+1,label,flush=True)
            v,e,s,c=compare(label,fixture(**kw),bodies,decoded,f,budgets)
            visited.update(v);edges.update(e);statuses[str(s)]=statuses.get(str(s),0)+1;count+=1
            if s:stops[f'{s}:{c.cs:04X}:{c.ip:04X}']=stops.get(f'{s}:{c.cs:04X}:{c.ip:04X}',0)+1
    caller={k for k in decoded if k[0]=='caller' and ENTRY<=k[1]<0xBD9}
    cond={k for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    ce={e for e in edges if e[0] in caller};cc=cond&caller
    assert caller<=visited,sorted(caller-visited)
    assert len(ce)==34,ce
    report=dict(loaded_stub_guards=stub_guards,target='OVL_03C2_0909',cases=count,cpu_words=14,ram_bytes=SIZE,status_counts=statuses,stop_counts=stops,
      caller_sites=len(caller&visited),caller_sites_total=len(caller),caller_branch_outcomes=len(ce),caller_branch_outcomes_total=len(cc)*2,
      chain_sites=len(visited),chain_sites_total=len(decoded),chain_branch_outcomes=len(edges),chain_branch_outcomes_total=len(cond)*2,
      masked_flags=0,emulator_state_substitutions=0,uncovered_sites=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
      uncovered_edges=[(f'{k}:{ip:04X}',hex(target)) for (k,ip) in sorted(cond) for target in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,target)) not in edges])
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
