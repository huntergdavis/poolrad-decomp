#!/usr/bin/env python3
"""Original-byte U3C6 +071A closed-chain CPU/RAM comparison; owner media only."""
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
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16
MASK=SIZE-1
PINS=((0,0x68,41,'05e4840e7db71675470a0c93a3741dafdde3f709d55970021096960a09506207'),(0x6E9,0x71A,18,'1495986874fc88753b5419d14380e9f7846b537a2b7015c57f7e24a1a761a7b9'),
      (0x71A,0x7D4,79,'97c632304df8ce07a32c07045300b149df14dbd48c88d52a96c51f61dfbb9b2c'),
      (0xC5F,0xCB9,32,'c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41'))
SOURCES=('overlay_u3c6_step_071a','overlay_u3c6_table_pair_0000','overlay_u3c6_in_range_06e9')


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(exe)==65231 and sha256(exe).hexdigest()==EXE_SHA
    assert len(ovr)==206105 and sha256(ovr).hexdigest()==OVR_SHA
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr)
    u=next(u for u in units if u['stub_para']==0x3C6)
    assert u['ovr_offset']==198458 and u['memory_segment']==0x3E0 and u['entries'][9]==0x071A
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    bodies,decoded={},{}
    for start,end,count,digest in PINS:
        b=code[start:end];ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(b,start))
        assert sum(i.size for i in ins)==len(b) and len(ins)==count
        if digest:assert sha256(b).hexdigest()==digest
        bodies[start]=b;decoded.update((i.address,i) for i in ins)
    assert [(i.address,i.op_str) for i in decoded.values() if 0x71A<=i.address<0x7D4 and i.mnemonic=='call']==[(0x72B,'0xc5f'),(0x772,'0'),(0x7A4,'0x6e9')]
    assert not any(s<=f<e for s,e,_,_ in PINS for f in u['fixups'])
    callers=(0x27A,0x345,0xA39,0xB15,0xBA7,0xE9D)
    raw=tuple(p for p in range(len(code)-2) if code[p]==0xE8 and
              (p+3+int.from_bytes(code[p+1:p+3],'little',signed=True))&65535==0x71A)
    assert raw==callers
    for site in callers:
        f=max((f for f in u['functions'] if f['rel']<=site),key=lambda f:f['rel'])
        ins={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[f['rel']:f['rel']+f['size_upper_bound']],f['rel'])}
        assert [(ins[p].mnemonic,ins[p].op_str) for p in (site-1,site,site+3)]==[('push','cs'),('call','0x71a'),('or','al, al')]
    far=[]
    for other in units:
        c=ovr[other['ovr_offset']:other['ovr_offset']+other['code_size']]
        for fx in other['fixups']:
            if fx>=3 and c[fx-3]==0x9A and int.from_bytes(c[fx:fx+2],'little')==0x3E0 and int.from_bytes(c[fx-2:fx],'little')==0x4D:
                f=max((f for f in other['functions'] if f['rel']<=fx-3),key=lambda f:f['rel'])
                ins={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(c[f['rel']:f['rel']+f['size_upper_bound']],f['rel'])}
                assert (ins[fx-3].mnemonic,ins[fx-3].op_str)==('lcall','0x3e0, 0x4d')
                far.append((other['stub_para'],fx-3))
    assert far==[(0x5E,0x18BB),(0x5E,0x1AD3),(0x86,0xE4C)]
    return bodies,decoded


def library(directory):
    path=directory/'step.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/f'{s}.c') for s in SOURCES]+['-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3c6_step_071a.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t]
    lib.pr_u3c6_step_071a.restype=ctypes.c_int
    return lib


def fixture(*,slot=1,count=3,mode=0,direction=1,x=3,y=3,pairs=((0,0),(1,0),(0,1),(1,1)),ss=0x2000,sp=0x8000,ds=0x1000,cs=0x6000,flags=0x202,background=None,match_offset=0x1234):
    memory=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,cs,0x71A,ds,0x1111,ss,flags)
    def byte(off,value):memory[physical(ds,off&65535)]=value&255
    byte(0x5F2A,count)
    for i in range(256):
        put16(memory,ds,0x65B9+4*i,0x9999);put16(memory,ds,0x65BB+4*i,0x8888)
    if slot is not None:
        put16(memory,ds,0x65B9+4*slot,match_offset);put16(memory,ds,0x65BB+4*slot,0x5678)
    s=slot or 0
    byte(0x5F2A+4*s,direction);byte(0x604A+s,x);byte(0x6092+s,y)
    for i,(a,b) in enumerate(pairs):byte(0x2844+8*direction+2*i,a);byte(0x2845+8*direction+2*i,b)
    for off,val in ((0,0x9000),(2,0x7000),(4,mode),(6,0x1234),(8,0x5678)):put16(memory,ss,(sp+off)&65535,val)
    return memory,cpu


def compare(label,initial,bodies,decoded,lib,budget=512):
    memory,cpu=initial
    for start,body in bodies.items():
        for i,b in enumerate(body):memory[(physical(cpu.cs,start)+i)&MASK]=b
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);c=Cpu.from_buffer_copy(cpu)
    status=lib.pr_u3c6_step_071a(ram,SIZE,ctypes.byref(c),budget)
    if status==1:raise ValueError(f'{label}: C argument rejected')
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(cpu,name))
    visited,edges=set(),set();prev=[None];calls=[];stop=[None];left=[budget];wrapped=[];les=[];stepping=[False]
    def hook(uc,address,size,user):
        if stepping[0]:return
        ip=uc.reg_read(UC_X86_REG_IP)
        if uc.reg_read(UC_X86_REG_CS)!=cpu.cs or ip not in decoded:raise ValueError(f'{label}: unexpected original transfer {address:X}')
        if ip in (0xC71,0x753):
            if left[0]==0:stop[0]='limit';uc.emu_stop();return
            left[0]-=1
        ins=decoded[ip]
        if prev[0] is not None:edges.add((prev[0],ip))
        prev[0]=ip if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        visited.add(ip)
        if ins.mnemonic=='call':calls.append(ip+ins.size)
        if ins.mnemonic=='les':
            # Public +0000 uses a contiguous four-byte operand. These seam
            # cases match Unicorn, not the separately documented native LES.
            bp=uc.reg_read(REGISTERS[6]);disp=6 if ip==0x4C else 10
            if ((bp+disp)&65535)>0xFFFC:les.append(ip)
        if ins.mnemonic=='retf':stop[0]='retf';uc.emu_stop()
    h=uc.hook_add(UC_HOOK_CODE,hook)
    expected_status=0
    while True:
        stop[0]=None
        uc.emu_start(physical(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)),2*SIZE,count=20000)
        if stop[0]=='limit':expected_status=2;break
        if stop[0]!='retf':raise ValueError(f'{label}: original instruction budget exceeded')
        ip=uc.reg_read(UC_X86_REG_IP);sp=uc.reg_read(UC_X86_REG_SP);ss=uc.reg_read(UC_X86_REG_SS)
        at=physical(ss,(sp+2)&65535);cs=backing[at]|(backing[(at+1)&MASK]<<8)
        stepping[0]=True;uc.emu_start(physical(cpu.cs,ip),2*SIZE,count=1);stepping[0]=False
        if sp>=0xFFFE:
            wrapped.append(ip);uc.reg_write(UC_X86_REG_CS,cs)
        if not calls:break
        target=calls.pop()
        if uc.reg_read(UC_X86_REG_CS)!=cpu.cs or uc.reg_read(UC_X86_REG_IP)!=target:expected_status=3;break
    if status!=expected_status:raise ValueError(f'{label}: status {status}/{expected_status}')
    for start,body in bodies.items():
        assert bytes(backing[physical(cpu.cs,start):physical(cpu.cs,start)+len(body)])==body,'code changed'
    if bytes(ram)!=bytes(backing):
        diffs=[i for i in range(SIZE) if ram[i]!=backing[i]];raise ValueError(f'{label}: RAM {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        if getattr(c,name)!=(uc.reg_read(reg)&65535):raise ValueError(f'{label}: {name} {getattr(c,name):04X}/{uc.reg_read(reg):04X}')
    return visited,edges,wrapped,les,status


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);args=p.parse_args()
    bodies,decoded=source();visited,edges=set(),set();count=wrap_cases=wrap_returns=les_cases=limits=0
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        def run(label,budget=512,**kw):
            nonlocal visited,edges,count,wrap_cases,wrap_returns,les_cases,limits
            v,e,w,l,s=compare(label,fixture(**kw),bodies,decoded,lib,budget)
            visited|=v;edges|=e;count+=1;wrap_cases+=bool(w);wrap_returns+=len(w);les_cases+=bool(l);limits+=s==2
        for mode in (0,1,255):
            for x in (0,6,7,127,128,255):
                for y in (0,6,7,127,128,255):run(f'boundary-{mode}-{x}-{y}',mode=mode,x=x,y=y)
        for direction in (0,1,2,127,255):
            for slot in (None,0,1,3,255):run(f'slot-{direction}-{slot}',direction=direction,slot=slot,count=0 if slot in (0,255) else 3)
        for pair in ((0,0),(6,6),(127,255),(128,0),(255,255),(0,128)):
            for mode in (0,1):run(f'pair-{pair}-{mode}',pairs=(pair,)*4,mode=mode)
        for ss in (0,0x2000,0xFFFF):
            for sp in (*range(40),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'seam-{ss:X}-{sp:X}',ss=ss,sp=sp,mode=1)
        for budget in (0,1,2,3,4,5,255,256):run(f'budget-{budget}',slot=None,direction=0,count=0,budget=budget)
        for sp in (0x2840,0x2850,0x5F30,0x5F32,0x5F40,0x6050,0x65C0):run(f'alias-{sp:X}',ds=0x2000,sp=sp,mode=1)
        run('matching-segment-wrong-offset',match_offset=0x1235)
        rng=random.Random(0x3C6071A)
        for i in range(40 if args.quick else 1200):
            run(f'random-{i}',mode=rng.randrange(256),direction=rng.randrange(1,32),x=rng.randrange(256),y=rng.randrange(256),slot=rng.randrange(1,8),count=7,
                pairs=tuple((rng.randrange(256),rng.randrange(256)) for _ in range(4)),flags=rng.choice((0x202,0xED7,0x47,0xA93)))
    required={(i.address,t) for i in decoded.values() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for t in (int(i.op_str,16),i.address+i.size)}
    missing_sites=sorted(set(decoded)-visited);missing_edges=sorted(required-edges)
    report=dict(target='OVL_03C6_071A',cases=count,whole_ram_bytes=SIZE,cpu_words=14,masked_flags=0,covered_sites=len(visited),source_sites=len(decoded),conditional_edges=len(edges),required_edges=len(required),missing_sites=missing_sites,missing_edges=missing_edges,
        wrapped_retf_hybrid_cases=wrap_cases,wrapped_retf_substitutions=wrap_returns,contiguous_les_seam_cases=les_cases,limit_cases=limits,
        bodies=[dict(start=f'{s:04X}',bytes=len(b),instructions=sum(s<=ip<s+len(b) for ip in decoded),sha256=sha256(b).hexdigest()) for s,b in bodies.items()],natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
    if missing_sites or missing_edges:raise ValueError('coverage incomplete')
if __name__=='__main__':main()
