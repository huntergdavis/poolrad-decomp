#!/usr/bin/env python3
"""Whole-RAM CPU parity for U5E +1BF8 and every reached original callee."""
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
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

MASK = SIZE-1
PINS = ((0x5E,0x1BF8,0x1E16,193,'e4d40ee52d77251769257793d73634d80def7043b5264c6e050ea98902c11e1a'),
        (0x3C6,0xBE7,0xC37,34,'747df8db81cfc6b78ff0ac95e85882142ec90197519513779fa46d82de19e479'),
        (0x3C6,0xC5F,0xCB9,32,'c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41'),
        (0x3C6,0x4BE,0x52B,42,'bf1db9df7666fe7c5615d826c14442fd10e08f768289f65ebfd3ac10daf2027e'),
        (0xAF8,0xDC0,0xDE1,13,'753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c'))
SET_SHA = 'bd8b38d0accf12b85906b44b8948209968e09cbada69948cc188e6e7b8daa876'


class Layout(ctypes.Structure):
    _fields_ = [('slots_cs',ctypes.c_uint16),('system_cs',ctypes.c_uint16)]


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(exe)==65231 and sha256(exe).hexdigest()==EXE_SHA
    assert len(ovr)==206105 and sha256(ovr).hexdigest()==OVR_SHA
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)}
    assert units[0x5E]['ovr_offset']+0x1BF8==82931 and units[0x5E]['entries'][13]==0x1BF8
    assert units[0x5E]['memory_segment']==0x68 and units[0x3C6]['memory_segment']==0x3E0
    assert units[0x3C6]['entries'][6]==0x4BE and units[0x3C6]['entries'][12:16]==[0xBE7,0xC0F,0xC37,0xC5F]
    expanded=bytes(unpack(exe)[0]);bodies={};decoded={}
    for unit,start,end,count,digest in PINS:
        b=(expanded[unit*16+start:unit*16+end] if unit==0xAF8 else
           ovr[units[unit]['ovr_offset']+start:units[unit]['ovr_offset']+end])
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(b,start))
        assert sha256(b).hexdigest()==digest and len(ins)==count and sum(i.size for i in ins)==len(b)
        bodies[(unit,start)]=b
        decoded.update(((unit,i.address),i) for i in ins)
    calls=[(i.address,i.op_str) for (u,_),i in decoded.items() if u==0x5E and i.mnemonic=='lcall']
    assert calls==[(0x1C1D,'0x3e0, 0x5c'),(0x1C3C,'0x3e0, 0x61'),(0x1C67,'0x3e0, 0x3e'),(0x1D73,'0xaf8, 0xdc0')]
    assert [f for f in units[0x5E]['fixups'] if 0x1BF8<=f<0x1E16]==[p+3 for p,_ in calls]
    assert [(i.address,i.op_str) for (u,_),i in decoded.items() if u==0x3C6 and i.mnemonic=='call']==[(0xBF4,'0xc5f'),(0xC1C,'0xc5f')]
    data=ovr[units[0x5E]['ovr_offset']+0x1BD8:units[0x5E]['ovr_offset']+0x1BF8]
    assert len(data)==32 and sha256(data).hexdigest()==SET_SHA
    return bodies,decoded,data


def library(directory):
    p=directory/'neighbor.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),
                    *[str(ROOT/'src/recovered'/s) for s in ('overlay_u5e_neighbor_1bf8.c','overlay_u3c6_04be_cpu.c','system_runtime_leaves_cpu.c')],
                    '-o',str(p)],check=True)
    lib=ctypes.CDLL(str(p))
    lib.pr_u5e_neighbor_1bf8.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Layout),ctypes.c_size_t]
    lib.pr_u5e_neighbor_1bf8.restype=ctypes.c_int
    return lib


def fixture(*,x=10,y=10,occupants=(0,)*9,tiles=(0,)*9,ranks=(6,3,1),caps=(20,20,20),groups=(1,1,2),
            records=((4,0,0,5),(5,1,0,6),(6,0,1,9)),count=3,subject_slot=1,equivalent=None,
            ss=0x2000,sp=0x8000,ds=0x1000,cs=0x6000,slots_cs=0x7000,system_cs=0x8000,stub_cs=0x9000,
            data_cs=0x5000,map_ptr=(0x4000,0x100),output=(0x5000,0x800),flags=0x202,
            shifts=((-1,-1),(0,-1),(1,-1),(-1,0),(0,0),(1,0),(-1,1),(0,1),(1,1))):
    m=bytearray([0xA5])*SIZE
    c=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,cs,0x1BF8,ds,0x1111,ss,flags)
    layout=Layout(slots_cs,system_cs)
    def byte(seg,off,v):m[physical(seg,off&65535)]=v&255
    def ptr(slot):return (data_cs,0x1C00+slot*0x400)
    for i in range(256):
        put16(m,ds,0x65B9+4*i,0x9999);put16(m,ds,0x65BB+4*i,0x8888)
    for i in range(1250):byte(map_ptr[0],map_ptr[1]+i+7,0);byte(ds,0x60DB+i,0)
    # Occupancy bytes overlap the last few bytes of the raw slot-0 entry.
    # Build actual slot pointers afterward, then retain every later alias.
    pointers={}
    for slot in range(1,4):
        seg,off=ptr(slot);pointers[slot]=(seg,off)
        byte(seg,off+0x10E,groups[slot-1]);byte(seg,off+0x11B,ranks[slot-1]);byte(seg,off+0x32,caps[slot-1])
    if equivalent=='segment':pointers[2]=(data_cs-1,0x2010)
    elif equivalent=='offset':pointers[2]=(data_cs,0x2001)
    for slot,(seg,off) in pointers.items():
        put16(m,ds,0x65B9+4*slot,off);put16(m,ds,0x65BB+4*slot,seg)
    if subject_slot is None:
        put16(m,ds,0x65B9+4,0x9999);put16(m,ds,0x65BB+4,0x8888)
    elif subject_slot!=1:
        put16(m,ds,0x65B9+4,0x9999);put16(m,ds,0x65BB+4,0x8888)
        put16(m,ds,0x65B9+4*subject_slot,0x2000);put16(m,ds,0x65BB+4*subject_slot,data_cs)
    if equivalent=='offset':
        # Distinct offset with matching classification, enough to reach the
        # source's low-word pointer comparison after an earlier best choice.
        byte(data_cs,0x2001+0x10E,groups[0]);byte(data_cs,0x2001+0x11B,ranks[0]);byte(data_cs,0x2001+0x32,caps[0])
    byte(ds,0x5F27,x);byte(ds,0x5F28,y)
    byte(ds,0x5F2A,count)
    s=subject_slot or 0
    byte(ds,0x5F27+4*s,x);byte(ds,0x5F28+4*s,y)
    put16(m,ds,0x6716,map_ptr[1]);put16(m,ds,0x6718,map_ptr[0])
    byte(ds,0x6715,len(records))
    for i,(slot,dx,dy,kind) in enumerate(records,1):
        seg,off=ptr(slot) if slot else (0,0)
        put16(m,ds,0x66D6+7*i,off);put16(m,ds,0x66D8+7*i,seg)
        byte(ds,0x66DA+7*i,x+dx);byte(ds,0x66DB+7*i,y+dy)
        if slot:byte(seg,off+0x10C,kind)
    for i,(dx,dy) in enumerate(shifts):
        byte(ds,0x2736+i,dx);byte(ds,0x273F+i,dy)
        col=(x+dx)&255;row=(y+dy)&255
        if col<=49 and row<=24:
            byte(map_ptr[0],map_ptr[1]+7+50*row+col,tiles[i]);byte(ds,0x60DB+50*row+col,occupants[i])
    for off,v in ((0,0x9000),(2,0xA000),(4,output[1]),(6,output[0]),(8,0x2000),(10,data_cs)):
        put16(m,ss,(sp+off)&65535,v)
    return m,c,layout,stub_cs


def install(m,c,l,stub,bodies,data):
    segments={0x5E:c.cs,0x3C6:l.slots_cs,0xAF8:l.system_cs}
    for (unit,start),b in bodies.items():
        for i,v in enumerate(b):m[physical(segments[unit],start+i)]=v
    for i,v in enumerate(data):m[physical(c.cs,0x1BD8+i)]=v
    for off in (0x1C20,0x1C3F,0x1C6A):put16(m,c.cs,off,stub)
    put16(m,c.cs,0x1D76,l.system_cs)
    for off,target in ((0x5C,0xBE7),(0x61,0xC0F),(0x3E,0x4BE)):
        at=physical(stub,off)
        m[at:at+5]=bytes((0xEA,target&255,target>>8,l.slots_cs&255,l.slots_cs>>8))
    return segments


def compare(label,initial,bodies,decoded,data,lib,budget=4096):
    m,c,l,stub=initial
    segments=install(m,c,l,stub,bodies,data);unit_by_cs={v:k for k,v in segments.items()}
    regions=[(physical(segments[u],s),e-s) for u,s,e,_,_ in PINS]+[(physical(c.cs,0x1BD8),32)]
    def protected(p):return any(((p-start)&MASK)<length for start,length in regions)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,name))
    visited,edges=set(),set();previous=[None];remaining=[budget];stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535
        if cs==stub:
            assert ip in (0x3E,0x5C,0x61),(label,'stub',hex(ip))
            return
        unit=unit_by_cs.get(cs);key=(unit,ip)
        assert key in decoded,(label,'PC',hex(cs),hex(ip))
        if key in ((0x5E,0x1C17),(0x5E,0x1D42),(0x3C6,0xC71)):
            if remaining[0]==0:stop[0]=3;uc.emu_stop();return
            remaining[0]-=1
        if unit==0x5E and ip in (0x1C8B,0x1C8E,0x1DD0,0x1DD3,0x1DF0,0x1DF3):
            off=(uc.reg_read(R.UC_X86_REG_DI)+(2 if ip in(0x1C8E,0x1DD3,0x1DF3) else 0))&65535
            at=physical(uc.reg_read(R.UC_X86_REG_ES),off)
            if protected(at) or protected((at+1)&MASK):stop[0]=2;uc.emu_stop();return
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key)
        previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):calls.append((cs,ip+ins.size))
        if ins.mnemonic=='retf':stop[0]='ret';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook)
    status=0
    while True:
        stop[0]=None
        uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1000000)
        assert stop[0] is not None,(label,'original instruction budget')
        if stop[0]!='ret':status=stop[0];break
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)
        stepping[0]=True;uc.emu_start(physical(cs,ip),2*SIZE,count=1);stepping[0]=False
        if not calls:break
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=calls.pop():
            status=5;break
    for start,length in regions:
        assert all(backing[(start+i)&MASK]==m[(start+i)&MASK] for i in range(length)),(label,'code/data modified')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c)
    got=lib.pr_u5e_neighbor_1bf8(ram,SIZE,ctypes.byref(actual),ctypes.byref(l),budget)
    assert got==status,(label,'status',got,status)
    assert bytes(ram)==bytes(backing),(label,'RAM',[(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12])
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected=uc.reg_read(reg)&65535
        assert getattr(actual,name)==expected,(label,name,hex(getattr(actual,name)),hex(expected))
    return visited,edges,status,actual


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);args=p.parse_args()
    bodies,decoded,data=source();visited=set();edges=set();count=0;statuses={};stops={};results=set()
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        def run(label,budget=4096,**kw):
            nonlocal count
            v,e,s,c=compare(label,fixture(**kw),bodies,decoded,data,lib,budget)
            visited.update(v);edges.update(e);count+=1;statuses[str(s)]=statuses.get(str(s),0)+1
            if s:stops[f'{s}:{c.ip:04X}']=stops.get(f'{s}:{c.ip:04X}',0)+1
            else:results.add(c.ax&255)
        run('empty')
        for rank in (0,1,7,8,9,19,20,127,128,255):
            for cap in (0,1,10,20,255):
                run(f'candidate-{rank}-{cap}',occupants=(2,)*9,ranks=(6,rank,1),caps=(20,cap,20))
        for occupants in ((1,)*9,(3,)*9,(2,1,2,1,3,0,0,0,0),(3,2,1,0,0,0,0,0,0)):
            for ranks in ((6,3,1),(2,1,3),(6,1,3),(10,2,3)):
                run(f'order-{occupants}-{ranks}',occupants=occupants,ranks=ranks)
        for equivalent in ('segment','offset'):
            run(f'raw-pointer-{equivalent}',occupants=(1,2,0,0,0,0,0,0,0),equivalent=equivalent,ranks=(2,2,1))
        for kind in (0,5,6,7,8,9,255):
            for records in ((),((0,0,0,kind),),((4,0,0,kind),),((4,1,0,kind),),((4,0,1,kind),)):
                run(f'record-{kind}-{records}',tiles=(31,)*9,records=records)
        for rank in (0,7,8,20):
            run(f'best-or-record-{rank}',tiles=(31,)*9,occupants=(2,0,0,0,0,0,0,0,0),ranks=(6,rank,1))
        for x in (0,49,50,127,128,255):
            for y in (0,24,25,127,128,255):run(f'bounds-{x}-{y}',x=x,y=y,tiles=(31,)*9)
        for slot in (None,0,1,3,255):
            run(f'lookup-{slot}',subject_slot=slot,count=0 if slot in(0,255) else 3)
        for budget in (1,2,3,4,6,8,12,20,27,28,40,100):
            run(f'budget-{budget}',budget=budget,tiles=(31,)*9)
        for sp in (42,43,0x100,0xFFF3,0xFFF4):
            run(f'frame-{sp}',sp=sp,tiles=(31,)*9)
        for output in ((0x2000,0x8000),(0x2000,0x8002),(0x2000,0x8004),(0x2000,0x7FFE),
                       (0x1000,0x5F2B),(0x5000,0x2000),(0x5000,0x100)):
            run(f'alias-{output}',output=output,occupants=(2,)*9)
        for output in ((0x6000,0x1BF8),(0x6000,0x1BD8),(0x7000,0x4BE),(0x8000,0xDC0)):
            run(f'code-alias-{output}',output=output,occupants=(2,)*9)
            run(f'final-code-alias-{output}',output=output)
        for output in ((0x6000,0x1BD6),(0x7000,0x4BC)):
            run(f'second-word-alias-{output}',output=output,occupants=(2,)*9)
            run(f'final-second-word-alias-{output}',output=output)
        for output in ((0x6000,0x1BD8),(0x6000,0x1BD6)):
            run(f'record-code-alias-{output}',output=output,tiles=(31,)*9)
        rng=random.Random(0x5E1BF8)
        for i in range(10 if args.quick else 400):
            run(f'random-{i}',x=rng.randrange(50),y=rng.randrange(25),occupants=tuple(rng.randrange(4) for _ in range(9)),
                tiles=tuple(rng.choice((0,31,1)) for _ in range(9)),ranks=tuple(rng.randrange(25) for _ in range(3)),
                caps=tuple(rng.randrange(25) for _ in range(3)),groups=tuple(rng.randrange(3) for _ in range(3)),
                flags=rng.choice((0x202,0xED7,0x47,0xA93)))
    required={(key,(key[0],dest)) for key,ins in decoded.items() if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp'
              for dest in (ins.address+ins.size,int(ins.op_str,16))}
    report=dict(target='OVL_005E_1BF8',cases=count,whole_ram_bytes=SIZE,cpu_words=14,source_sites=len(decoded),covered_sites=len(visited),
                conditional_edges=len(edges),required_edges=len(required),missing_sites=sorted(set(decoded)-visited),missing_edges=sorted(required-edges),
                status_counts=statuses,stop_counts=stops,returned_al=sorted(results),masked_flags=0,emulator_state_substitutions=0,
                native_route_measured=False)
    if args.output:args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
    assert not report['missing_sites'] and not report['missing_edges'],'coverage incomplete'


if __name__=='__main__':main()
