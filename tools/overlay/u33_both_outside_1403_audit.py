#!/usr/bin/env python3
"""Pinned U33 +1403 predicate, original caller ABI and full RAM/CPU parity."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import CS_ARCH_X86,CS_MODE_16,Cs
from unicorn import Uc,UC_ARCH_X86,UC_HOOK_CODE,UC_MODE_16
from unicorn.x86_const import UC_X86_REG_IP
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
ENTRY,END=0x1403,0x1434
BODY_SHA='a749aec8ac16739928c52f412b1bd4447e3bb668500b56bd76b1a5140288d62b'
BRANCHES=(0x140D,0x1413,0x1419,0x141F)
CALLERS=(0x17C8,0x18C8)


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    if len(exe)!=65231 or sha256(exe).hexdigest()!=EXE_SHA or len(ovr)!=206105 or sha256(ovr).hexdigest()!=OVR_SHA:raise ValueError('owner media differs')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x33)
    if u['ovr_offset']!=50641 or u['memory_segment']!=0x3B:raise ValueError('U33 placement differs')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];body=code[ENTRY:END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY));decoded={i.address:i for i in ins}
    if len(body)!=49 or sha256(body).hexdigest()!=BODY_SHA or len(ins)!=18 or sum(i.size for i in ins)!=49:raise ValueError('predicate extent differs')
    if ins[-1].address!=0x1431 or ins[-1].mnemonic!='retf' or ins[-1].op_str!='4':raise ValueError('far return differs')
    if any('call' in i.mnemonic or i.mnemonic.startswith('int') for i in ins) or any(ENTRY<=p<END for p in u['fixups']):raise ValueError('new dependency or relocation')
    for i in ins:
        if i.mnemonic.startswith('j') and int(i.op_str,16) not in decoded:raise ValueError('branch leaves body')
    starts=sorted(f['rel'] for f in u['functions']);end=next(v for v in starts if v>0x1577)
    caller={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[0x1577:end],0x1577)}
    for site in CALLERS:
        expected={site-9:('mov','al, byte ptr [bp - 0x17]'),site-6:('push','ax'),site-5:('mov','al, byte ptr [bp - 0x18]'),
                  site-2:('push','ax'),site-1:('push','cs'),site:('call','0x1403'),site+3:('or','al, al')}
        for ip,(op,operand) in expected.items():
            if caller[ip].mnemonic!=op or caller[ip].op_str!=operand:raise ValueError('caller ABI or result use differs')
    raw=[p for p in range(len(code)-2) if code[p]==0xE8 and (p+3+int.from_bytes(code[p+1:p+3],'little',signed=True))&0xFFFF==ENTRY]
    if raw!=list(CALLERS):raise ValueError('raw caller set differs')
    return body,decoded,dict(caller_prefix=code[0x17BF:0x17CB],file_extent=[55764,55813])


def library(directory):
    path=directory/'u33_1403.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),
        str(ROOT/'src/recovered/overlay_u33_both_outside_1403.c'),'-o',str(path)],check=True,capture_output=True,text=True)
    lib=ctypes.CDLL(str(path));lib.pr_u33_both_outside_1403.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u33_both_outside_1403.restype=ctypes.c_int;return lib


def make_case(body,*,a=0,b=0,ax=0xABCD,ss=0x2000,sp=0x8000,flags=0x0202,background=None):
    memory=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu=Cpu(ax,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,0xF000,ENTRY,0x1000,0x1111,ss,flags)
    at=physical(ss,sp)
    for i,value in enumerate((0x12,7,cpu.cs & 255,cpu.cs>>8)):memory[(at+i)&(SIZE-1)]=value
    put16(memory,ss,(sp+4)&0xFFFF,a);put16(memory,ss,(sp+6)&0xFFFF,b)
    at=physical(cpu.cs,ENTRY);memory[at:at+len(body)]=body;return memory,cpu


def compare(label,memory,cpu,decoded,lib):
    original=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):original.reg_write(reg,getattr(cpu,name))
    entry=physical(cpu.cs,ENTRY);visited,edges=set(),set();previous,at_return=[None],[False]
    def hook(uc,address,size,user):
        del size,user
        ip=uc.reg_read(UC_X86_REG_IP)
        if not entry<=address<entry+END-ENTRY or ip not in decoded:raise ValueError(f'unsupported transfer {label}')
        if previous[0] in BRANCHES:edges.add((previous[0],ip))
        previous[0]=ip;visited.add(ip)
        if ip==0x1431:at_return[0]=True;uc.emu_stop()
    h=original.hook_add(UC_HOOK_CODE,hook);original.emu_start(entry,2*SIZE,count=32)
    if not at_return[0]:raise ValueError('original failed bounded predicate')
    original.hook_del(h);original.emu_start(physical(cpu.cs,0x1431),2*SIZE,count=1)
    if bytes(backing[entry:entry+END-ENTRY])!=memory[entry:entry+END-ENTRY]:raise ValueError('immutable code changed')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    if lib.pr_u33_both_outside_1403(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected valid case')
    if bytes(ram)!=bytes(backing):
        diffs=[i for i in range(SIZE) if ram[i]!=backing[i]];raise ValueError(f'RAM differs {label}: {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected=original.reg_read(reg)&0xFFFF
        if getattr(cpu,name)!=expected:raise ValueError(f'{name} differs {label}: {getattr(cpu,name):04X}/{expected:04X}')
    return visited,edges


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--quick',action='store_true');parser.add_argument('--output',type=Path);args=parser.parse_args()
    body,decoded,meta=source();visited,edges=set(),set();count=0
    with tempfile.TemporaryDirectory(prefix='poolrad-u33-1403-') as tmp:
        lib=library(Path(tmp))
        def run(label,**options):
            nonlocal visited,edges,count
            memory,cpu=make_case(body,**options);v,e=compare(label,memory,cpu,decoded,lib);visited|=v;edges|=e;count+=1
        for a in range(0,256,16 if args.quick else 1):
            for b in (0,10,11,127,128,255):run(f'a-{a}-{b}',a=a,b=b)
        for b in range(0,256,16 if args.quick else 1):
            for a in (0,5,6,127,128,255):run(f'b-{a}-{b}',a=a,b=b)
        for a in (0,5,6,11,127,128,255):
            for b in (0,5,6,10,11,127,128,255):run(f'boundary-{a}-{b}',a=a,b=b)
        for high in range(0,256,16 if args.quick else 1):
            for a,b in ((5,10),(6,11),(128,255)):
                run(f'upper-{high}-{a}-{b}',a=(high<<8)|a,b=((255-high)<<8)|b,ax=(high<<8)|0xCD)
        for ss in (0,0x2000,0xFFFF):
            for sp in (*range(16),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'wrap-{ss:X}-{sp:X}',ss=ss,sp=sp,a=255,b=11)
        rng=random.Random(0x331403)
        for i in range(8 if args.quick else 128):
            run(f'seeded-{i}',a=rng.randrange(65536),b=rng.randrange(65536),ax=rng.randrange(65536),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    required={(ip,target) for ip in BRANCHES for target in (int(decoded[ip].op_str,16),ip+decoded[ip].size)}
    if visited!=set(decoded) or edges!=required:raise ValueError('instruction/edge coverage incomplete')
    report=dict(target='OVL_0033_1403',source_sha256=BODY_SHA,cases=count,covered_sites=len(visited),source_sites=len(decoded),
        conditional_edges=len(edges),compared_ram_bytes=SIZE,compared_cpu_words=14,masked_flags=0,aligned_near_callers=['1577:+17C8','1577:+18C8'],
        file_extent=meta['file_extent'],natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
