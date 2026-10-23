#!/usr/bin/env python3
"""Pinned U38 +04BA and U9E callee chain: whole-RAM/CPU oracle."""
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
from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP,UC_X86_REG_SP,UC_X86_REG_SS,UC_X86_REG_BP
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
MASK=SIZE-1
PINS=((0x38,0x4BA,0x5D6,99,'0556f1d0f2eb60cc9d8d5e346274a4b6ba13c9e0a0dad054bc2d7eccbbcf010e'),
      (0x9E,0x10E4,0x110D,17,'12ce30806a561605b7f7be238444a28a0d22f7cc53bda4872d6e0800330f41db'),
      (0x9E,0x110D,0x115C,29,'63aaf31a1bb13ac3a652db1b1fef79afa6ddf2aee869fe426bc4bda7e5395830'))
SOURCES=('overlay_u38_pair_04ba','overlay_u9e_entry16_10e4','overlay_u9e_entry17_110d')


def source():
    e=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();o=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(e)==65231 and sha256(e).hexdigest()==EXE_SHA
    assert len(o)==206105 and sha256(o).hexdigest()==OVR_SHA
    us={u['stub_para']:u for u in find_units(e[parse_mz(e)['header_bytes']:],o)}
    assert us[0x38]['ovr_offset']==60507 and us[0x38]['entries'][12]==0x4BA
    assert us[0x9E]['memory_segment']==0xB0 and us[0x9E]['entries'][16:18]==[0x10E4,0x110D]
    bodies,decoded={},{}
    for u,s,end,count,digest in PINS:
        b=o[us[u]['ovr_offset']+s:us[u]['ovr_offset']+end];ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(b,s))
        assert sha256(b).hexdigest()==digest and len(ins)==count and sum(i.size for i in ins)==len(b)
        bodies[u,s]=b;decoded.update(((u,i.address),i) for i in ins)
    assert [(i.address,i.op_str) for (u,_),i in decoded.items() if i.mnemonic=='lcall']==[(0x4DD,'0xb0, 0x75'),(0x552,'0xb0, 0x75'),(0x5B0,'0xb0, 0x70')]
    assert [f for f in us[0x38]['fixups'] if 0x4BA<=f<0x5D6]==[0x4E0,0x555,0x5B3]
    assert not any(0x10E4<=f<0x115C for f in us[0x9E]['fixups'])
    return bodies,decoded


CHECKER_ROWS = ('OVL_0038_04BA',)


def library(directory):
    path=directory/'pair.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/f'{s}.c') for s in SOURCES]+['-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u38_pair_04ba.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_size_t]
    lib.pr_u38_pair_04ba.restype=ctypes.c_int;return lib


def fixture(*,attribute=5,nodes=((12,6),(38,106)),ss=0x2000,sp=0x8000,cs=0x6000,callee_cs=0x5000,stub_cs=0x8000,src=(0x3000,0x100),record=(0x4000,0x100),node_seg=0x3000,node_base=0x300,self_index=None,cycle=False,flags=0x202,background=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    c=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,cs,0x4BA,0x1000,0x1111,ss,flags)
    def byte(p,off,val):m[physical(p[0],(p[1]+off)&65535)]=val&255
    def word(p,off,val):put16(m,p[0],(p[1]+off)&65535,val)
    pointers=[src if i==self_index else (node_seg,(node_base+0x100*i)&65535) for i in range(len(nodes))]
    byte(src,0,12);byte(src,3,attribute);word(src,5,0);word(src,7,0)
    for i,(kind,a) in enumerate(nodes):
        p=pointers[i];n=pointers[(i+1)%len(nodes)] if cycle or i+1<len(nodes) else (0,0)
        byte(p,0,kind);byte(p,3,a);word(p,5,n[1]);word(p,7,n[0])
    first=pointers[0] if pointers else (0,0)
    word(record,0x7F,first[1]);word(record,0x81,first[0])
    for off,val in ((0,0x9000),(2,0x7000),(4,0xCAFE),(6,src[1]),(8,src[0]),(10,record[1]),(12,record[0])):put16(m,ss,(sp+off)&65535,val)
    return m,c,callee_cs,stub_cs


def install(m,c,callee,stub,bodies):
    immutable={}
    for (unit,start),body in bodies.items():
        segment=c.cs if unit==0x38 else callee
        data=bytearray(body)
        if unit==0x38:
            for p in (0x4E0,0x555,0x5B3):data[p-start:p-start+2]=stub.to_bytes(2,'little')
        for i,b in enumerate(data):m[(physical(segment,start)+i)&MASK]=b
        immutable[segment,start]=bytes(data)
    for offset,entry in ((0x70,0x10E4),(0x75,0x110D)):
        b=b'\xEA'+entry.to_bytes(2,'little')+callee.to_bytes(2,'little')
        m[physical(stub,offset):physical(stub,offset)+5]=b;immutable[stub,offset]=b
    return immutable


def compare(label,initial,bodies,decoded,lib,budget=32):
    m,c,callee,stub=initial;immutable=install(m,c,callee,stub,bodies)
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);candidate=Cpu.from_buffer_copy(c)
    status=lib.pr_u38_pair_04ba(ram,SIZE,ctypes.byref(candidate),callee,budget)
    if status==1:raise ValueError(f'{label}: C rejected fixture')
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    visited,edges=set(),set();previous=[None];calls=[];stop=[None];stepping=[False];left=[budget];wraps=[];les_seams=[]
    def hook(uc,address,size,user):
        if stepping[0]:return
        cs=uc.reg_read(UC_X86_REG_CS);ip=uc.reg_read(UC_X86_REG_IP)
        if cs==stub and ip in (0x70,0x75):return
        unit=0x38 if cs==c.cs else 0x9E if cs==callee else None
        key=(unit,ip)
        if key not in decoded:raise ValueError(f'{label}: unexpected PC {cs:X}:{ip:X}')
        if key==(0x38,0x518):
            if not left[0]:stop[0]='limit';uc.emu_stop();return
            left[0]-=1
        ins=decoded[key];visited.add(key)
        if previous[0] is not None:edges.add((previous[0],key))
        previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic=='lcall':calls.append((cs,ip+ins.size))
        if ins.mnemonic=='les':
            operand=ins.op_str.split('[bp')[1].split(']')[0].strip()
            displacement=0 if not operand else int(operand[1:].strip(),0)*(1 if operand[0]=='+' else -1)
            if ((uc.reg_read(UC_X86_REG_BP)+displacement)&65535)>0xFFFC:les_seams.append(key)
        if ins.mnemonic=='retf':stop[0]='return';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook);expected_status=0
    while True:
        stop[0]=None
        uc.emu_start(physical(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)),2*SIZE,count=20000)
        if stop[0]=='limit':expected_status=2;break
        if stop[0]!='return':raise ValueError(f'{label}: original instruction budget')
        sp=uc.reg_read(UC_X86_REG_SP);ss=uc.reg_read(UC_X86_REG_SS);return_site=(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP))
        at=physical(ss,(sp+2)&65535);wrapped_cs=backing[at]|(backing[(at+1)&MASK]<<8)
        stepping[0]=True;uc.emu_start(physical(*return_site),2*SIZE,count=1);stepping[0]=False
        if sp>=0xFFFE:wraps.append(return_site);uc.reg_write(UC_X86_REG_CS,wrapped_cs)
        if not calls:break
        if (uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP))!=calls.pop():expected_status=3;break
    if status!=expected_status:raise ValueError(f'{label}: status {status}/{expected_status}')
    for (seg,off),b in immutable.items():
        at=physical(seg,off)
        if bytes(backing[at:at+len(b)])!=b:raise ValueError(f'{label}: excluded code-writing fixture')
    if bytes(ram)!=bytes(backing):
        diffs=[i for i in range(SIZE) if ram[i]!=backing[i]];raise ValueError(f'{label}: RAM {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        if getattr(candidate,name)!=(uc.reg_read(reg)&65535):raise ValueError(f'{label}: {name} {getattr(candidate,name):04X}/{uc.reg_read(reg):04X}')
    return visited,edges,wraps,les_seams,status


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');args=p.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded=source();visited,edges=set(),set();count=wrap_cases=wrap_returns=seam_cases=limits=transfers=0
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        from loaded_stub_checks import verify_refusals
        m,c,callee,stub=fixture();install(m,c,callee,stub,bodies)
        stub_guards=verify_refusals(m,c,lambda ram,state: lib.pr_u38_pair_04ba(ram,SIZE,ctypes.byref(state),callee,32),
                                    [(c.cs,p) for p in (0x4DD,0x552,0x5B0)])
        print("LOADED_STUB_GUARDS",stub_guards,flush=True)
        def run(label,budget=32,**kw):
            nonlocal visited,edges,count,wrap_cases,wrap_returns,seam_cases,limits,transfers
            v,e,w,l,s=compare(label,fixture(**kw),bodies,decoded,lib,budget)
            visited|=v;edges|=e;count+=1;wrap_cases+=bool(w);wrap_returns+=len(w);seam_cases+=bool(l);limits+=s==2;transfers+=s==3
        for a in range(0,256,16 if args.quick else 1):
            for nodes in ((),((12,5),),((12,101),(38,106)),((0,0),(255,255))):run(f'source-{a}-{nodes}',attribute=a,nodes=nodes)
        for a in range(0,256,16 if args.quick else 1):
            for kind in (0,12,38,255):run(f'candidate-{kind}-{a}',nodes=((kind,a),))
        for attrs in ((1,1),(1,2),(101,0),(127,106),(106,127),(127,127),(101,101),(101,100),(100,101),(128,255)):
            run(f'ordering-{attrs}',nodes=tuple((12,a) for a in attrs))
        for i in (0,1,2):run(f'self-{i}',nodes=((12,5),(38,6),(12,7)),self_index=i)
        for ss in (0,0x2000,0xFFFF):
            for sp in (*range(40),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'seam-{ss:X}-{sp:X}',ss=ss,sp=sp,budget=8)
        for n in (0,1,2,8):run(f'cycle-{n}',cycle=True,budget=n)
        for off in (0,1,0xFFF8,0xFFFB,0xFFFF):run(f'node-offset-{off:X}',node_base=off,nodes=((12,5),))
        run('different-node-segment',node_seg=0x3100)
        for off in (0x7FEA,0x7FF0,0x7FFA,0x7FFB,0x7FFF,0x8000):
            run(f'record-frame-{off:X}',record=(0x2000,off),budget=8)
        run('source-is-record',src=(0x4000,0x100))
        run('node-is-record',node_seg=0x4000,node_base=0x100,nodes=((12,7),))
        run('node-link-wrap',node_base=0xFFFB,nodes=((12,7),(38,111)))
        rng=random.Random(0x3804BA)
        for i in range(30 if args.quick else 800):
            run(f'seeded-{i}',attribute=rng.randrange(128),nodes=tuple((rng.choice((0,12,38,255)),rng.randrange(256)) for _ in range(rng.randrange(1,7))),flags=rng.choice((0x202,0xED7,0x47,0xA93)))
    required={((u,i.address),(u,t)) for (u,_),i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for t in (int(i.op_str,16),i.address+i.size)}
    missing_sites=sorted(set(decoded)-visited);missing_edges=sorted(required-edges)
    report=dict(loaded_stub_guards=stub_guards,target='OVL_0038_04BA',cases=count,whole_ram_bytes=SIZE,cpu_words=14,masked_flags=0,covered_sites=len(visited),source_sites=len(decoded),conditional_edges=len(edges),required_edges=len(required),missing_sites=missing_sites,missing_edges=missing_edges,
        wrapped_retf_hybrid_cases=wrap_cases,wrapped_retf_substitutions=wrap_returns,contiguous_les_seam_cases=seam_cases,limit_cases=limits,transfer_cases=transfers,natural_game_route_measured=False,
        bodies=[dict(unit=f'{u:04X}',start=f'{s:04X}',bytes=len(b),sha256=sha256(b).hexdigest()) for (u,s),b in bodies.items()])
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
    if missing_sites or missing_edges:raise ValueError('coverage incomplete')
if __name__=='__main__':main()
