#!/usr/bin/env python3
"""Original four-body parity for the existing U3C6 +0CB9 raw CPU model."""
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
from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP,UC_X86_REG_SP,UC_X86_REG_SS,UC_X86_REG_BP
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
MASK=SIZE-1
PINS=((0,0x68,41,'05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207'),
      (0x4BE,0x52B,42,'bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e'),
      (0xC5F,0xCB9,32,'c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41'),
      (0xCB9,0xE11,139,'63f0434e2d8cadf426464601eed98ae42fa5d13b708303f45ac5440faf95d43b'))


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(exe)==65231 and sha256(exe).hexdigest()==EXE_SHA
    assert len(ovr)==206105 and sha256(ovr).hexdigest()==OVR_SHA
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);u=next(u for u in units if u['stub_para']==0x3C6)
    assert u['ovr_offset']==198458 and u['memory_segment']==0x3E0 and u['entries'][16]==0xCB9
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];bodies,decoded={},{}
    for start,end,count,digest in PINS:
        b=code[start:end];ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(b,start))
        assert sha256(b).hexdigest()==digest and len(ins)==count and sum(i.size for i in ins)==len(b)
        bodies[start]=b;decoded.update((i.address,i) for i in ins)
    assert [(i.address,i.op_str) for i in decoded.values() if i.mnemonic=='call']==[(0xCD8,'0xc5f'),(0xD2A,'0'),(0xD85,'0x4be')]
    assert not any(s<=f<e for s,e,_,_ in PINS for f in u['fixups'])
    return bodies,decoded


CHECKER_ROWS = ('OVL_03C6_0CB9',)


def library(directory):
    p=directory/'scan.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),str(ROOT/'src/recovered/overlay_u3c6_0cb9_cpu.c'),'-o',str(p)],check=True)
    lib=ctypes.CDLL(str(p));lib.pr_u3c6_0cb9_cpu_run.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_int]
    lib.pr_u3c6_0cb9_cpu_run.restype=ctypes.c_int;return lib


def fixture(*,slot=1,count=3,direction=1,x=3,y=3,k=0,bias=(0,0),pairs=((0,0),(1,0),(2,0),(3,0)),cells=(2,5,7,9),occupants=(0,1,2,3),ranks=(1,2,3,4),ss=0x2000,sp=0x8000,ds=0x1000,cs=0x6000,map_ptr=(0x4000,0x100),p6=(0x5000,0x100),p10=(0x5000,0x110),match_offset=0x1234,flags=0x202):
    m=bytearray([0xA5])*SIZE;c=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,cs,0xCB9,ds,0x1111,ss,flags)
    def byte(seg,off,val):m[physical(seg,off&65535)]=val&255
    for i in range(256):
        put16(m,ds,0x65B9+4*i,0x9999);put16(m,ds,0x65BB+4*i,0x8888)
        byte(ds,0x2744+4*i,0)
    # Default map contents let aliased/shifted coordinates remain deterministic.
    for i in range(1250):byte(map_ptr[0],map_ptr[1]+7+i,2);byte(ds,0x60DB+i,0)
    if slot is not None:put16(m,ds,0x65B9+4*slot,match_offset);put16(m,ds,0x65BB+4*slot,0x5678)
    s=slot or 0
    byte(ds,0x5F2A,count);byte(ds,0x5F27+4*s,x);byte(ds,0x5F28+4*s,y);byte(ds,0x5F2A+4*s,direction)
    byte(ds,0x2736+k,bias[0]);byte(ds,0x273F+k,bias[1])
    put16(m,ds,0x6716,map_ptr[1]);put16(m,ds,0x6718,map_ptr[0])
    for i,(a,b) in enumerate(pairs):
        byte(ds,0x2844+8*direction+2*i,a);byte(ds,0x2845+8*direction+2*i,b)
        col=(x+a+bias[0])&255;row=(y+b+bias[1])&255
        if col<=49 and row<=24:
            byte(map_ptr[0],map_ptr[1]+7+row*50+col,cells[i]);byte(ds,0x60DB+row*50+col,occupants[i])
        byte(ds,0x2744+4*cells[i],ranks[i])
    for off,val in ((0,0x9000),(2,0x7000),(4,p6[1]),(6,p6[0]),(8,p10[1]),(10,p10[0]),(12,k),(14,0x1234),(16,0x5678)):
        put16(m,ss,(sp+off)&65535,val)
    return m,c


def compare(label,initial,bodies,decoded,lib,policy=1,alias_stop=None):
    m,c=initial
    for start,b in bodies.items():m[physical(c.cs,start):physical(c.cs,start)+len(b)]=b
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,name))
    visited,edges=set(),set();prev=[None];calls=[];stop=[None];stepping=[False];wrapped=[];steps=[0]
    def hook(uc,address,size,user):
        if stepping[0]:return
        ip=uc.reg_read(UC_X86_REG_IP)
        if uc.reg_read(UC_X86_REG_CS)!=c.cs or ip not in decoded:raise ValueError(f'{label}: unexpected PC {address:X}')
        if alias_stop==ip:stop[0]='alias';uc.emu_stop();return
        if ip==0xD29:
            sp=uc.reg_read(UC_X86_REG_SP)
            if sp>0xFFFC or ((sp+4)&65535)>0xFFFC:stop[0]='seam';uc.emu_stop();return
        i=decoded[ip]
        if i.mnemonic=='les':
            if '[bp' in i.op_str:
                op=i.op_str.split('[bp')[1].split(']')[0].strip();disp=int(op[1:].strip(),0)*(1 if op[0]=='+' else -1)
                if ((uc.reg_read(UC_X86_REG_BP)+disp)&65535)>0xFFFC:raise ValueError(f'{label}: excluded native-LES operand seam at {ip:X}')
        if prev[0] is not None:edges.add((prev[0],ip))
        prev[0]=ip if i.mnemonic.startswith('j') and i.mnemonic!='jmp' else None
        visited.add(ip);steps[0]+=1
        if steps[0]>20000:raise ValueError(f'{label}: nonterminating fixture (not passed to unbounded C)')
        if i.mnemonic=='call':calls.append(ip+i.size)
        if i.mnemonic=='retf':stop[0]='return';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook);expected=0
    while True:
        stop[0]=None;uc.emu_start(physical(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)),2*SIZE,count=20001)
        if stop[0]=='alias':expected=2;break
        if stop[0]=='seam':expected=5;break
        if stop[0]!='return':raise ValueError(f'{label}: original budget exceeded')
        if policy==0:expected=4;break
        ip=uc.reg_read(UC_X86_REG_IP);sp=uc.reg_read(UC_X86_REG_SP);ss=uc.reg_read(UC_X86_REG_SS)
        at=physical(ss,(sp+2)&65535);wrapped_cs=backing[at]|(backing[(at+1)&MASK]<<8)
        stepping[0]=True;uc.emu_start(physical(c.cs,ip),2*SIZE,count=1);stepping[0]=False
        if sp>=0xFFFE and policy==1:wrapped.append(ip);uc.reg_write(UC_X86_REG_CS,wrapped_cs)
        if not calls:break
        if (uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP))!=(c.cs,calls.pop()):expected=3;break
    for start,b in bodies.items():assert bytes(backing[physical(c.cs,start):physical(c.cs,start)+len(b)])==b,'original code changed'
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);candidate=Cpu.from_buffer_copy(c)
    status=lib.pr_u3c6_0cb9_cpu_run(ram,SIZE,ctypes.byref(candidate),policy)
    if status!=expected:raise ValueError(f'{label}: status {status}/{expected}')
    if bytes(ram)!=bytes(backing):raise ValueError(f'{label}: RAM differs at {[i for i in range(SIZE) if ram[i]!=backing[i]][:10]}')
    af_masked=0
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        actual=getattr(candidate,name);reference=uc.reg_read(reg)&65535
        # The fail-closed +0D29 state exposes SHL's undefined AF. Existing C
        # uses DOSBox-X AF=1; Unicorn uses AF=0. Only this declared stop masks AF.
        mask=0x10 if name=='flags' and expected==5 else 0
        if mask and actual!=reference:af_masked=1
        if (actual^reference)&~mask:raise ValueError(f'{label}: {name} {actual:04X}/{reference:04X}')
    return visited,edges,wrapped,status,af_masked


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');args=p.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded=source();visited,edges=set(),set();count=wrap_cases=wrap_returns=af_masks=0;statuses={};policies={}
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        def run(label,policy=1,alias_stop=None,**kw):
            nonlocal visited,edges,count,wrap_cases,wrap_returns,af_masks
            v,e,w,s,a=compare(label,fixture(**kw),bodies,decoded,lib,policy,alias_stop)
            visited|=v;edges|=e;count+=1;wrap_cases+=bool(w);wrap_returns+=len(w);af_masks+=a
            statuses[str(s)]=statuses.get(str(s),0)+1;policies[str(policy)]=policies.get(str(policy),0)+1
        for policy in (1,2):
            for x in (0,1,48,49,50,127,128,255):
                for y in (0,23,24,25,127,128,255):run(f'bounds-{policy}-{x}-{y}',policy=policy,x=x,y=y)
            for cells in ((0,2,5,7),(2,0,5,7),(30,2,5,7),(2,30,5,7),(2,5,7,9),(128,255,23,9)):
                for ranks in ((0,0,0,0),(1,1,1,1),(4,3,2,1),(1,2,3,4)):run(f'rank-{policy}-{cells}-{ranks}',policy=policy,cells=cells,ranks=ranks)
            for direction in (0,1,2,255):
                for slot in (None,0,1,3,255):run(f'slot-{policy}-{direction}-{slot}',policy=policy,direction=direction,slot=slot,count=0 if slot in (0,255) else 3)
            for sp in (*range(40),0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'frame-{policy}-{sp:X}',policy=policy,sp=sp)
            run(f'offset-mismatch-{policy}',policy=policy,match_offset=0x1235)
            run(f'negative-delta-{policy}',policy=policy,pairs=((128,0),)*4)
        for sp in (0,0x8000,0xFFFE,0xFFFF):run(f'unmeasured-{sp:X}',policy=0,sp=sp)
        run('alias-p10',p10=(0x6000,0x10),alias_stop=0xCC2)
        run('alias-p6',p6=(0x6000,0x4BE),alias_stop=0xCC9)
        for ptr in ((0x2000,0x8000),(0x2000,0x8002),(0x5000,0x110),(0x1000,0x5F2E)):
            run(f'output-alias-{ptr}',p6=ptr)
        rng=random.Random(0x3C60CB9)
        for i in range(30 if args.quick else 800):
            run(f'random-{i}',policy=1+i%2,x=rng.randrange(50),y=rng.randrange(25),k=rng.randrange(9),bias=(rng.randrange(256),rng.randrange(256)),
                pairs=tuple((rng.choice((0,1,2,127,128,255)),rng.randrange(256)) for _ in range(4)),cells=tuple(rng.choice((0,2,5,7,30,128,255)) for _ in range(4)),
                occupants=tuple(rng.randrange(4) for _ in range(4)),ranks=tuple(rng.randrange(8) for _ in range(4)),flags=rng.choice((0x202,0xED7,0x47,0xA93)))
    required={(i.address,t) for i in decoded.values() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for t in (int(i.op_str,16),i.address+i.size)}
    report=dict(target='OVL_03C6_0CB9',cases=count,whole_ram_bytes=SIZE,cpu_words=14,covered_sites=len(visited),source_sites=len(decoded),conditional_edges=len(edges),required_edges=len(required),missing_sites=sorted(set(decoded)-visited),missing_edges=sorted(required-edges),
        wrapped_retf_hybrid_cases=wrap_cases,wrapped_retf_substitutions=wrap_returns,undefined_af_masked_seam_stops=af_masks,status_counts=statuses,policy_counts=policies,native_les_seams_compared=False,natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
    if report['missing_sites'] or report['missing_edges']:raise ValueError('coverage incomplete')
if __name__=='__main__':main()
