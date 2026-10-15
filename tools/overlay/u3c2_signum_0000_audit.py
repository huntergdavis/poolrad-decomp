#!/usr/bin/env python3
"""U3C2 public entry1 signum: source/callers, full original RAM/CPU parity."""
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
ENTRY,END=0,0x2E
BODY_SHA='5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf'
BRANCHES=(0xA,0x17)


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    if len(exe)!=65231 or sha256(exe).hexdigest()!=EXE_SHA or len(ovr)!=206105 or sha256(ovr).hexdigest()!=OVR_SHA:raise ValueError('media provenance differs')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);u=next(u for u in units if u['stub_para']==0x3C2)
    if u['ovr_offset']!=195410 or u['memory_segment']!=0x3DB or u['entries'][1]!=0:raise ValueError('U3C2 public entry placement differs')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];body=code[:END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,0));decoded={i.address:i for i in ins}
    if len(body)!=46 or sha256(body).hexdigest()!=BODY_SHA or len(ins)!=17 or sum(i.size for i in ins)!=46:raise ValueError('signum extent differs')
    if ins[-1].address!=0x2B or ins[-1].mnemonic!='retf' or ins[-1].op_str!='2':raise ValueError('far return differs')
    if any('call' in i.mnemonic or i.mnemonic.startswith('int') for i in ins) or any(p<END for p in u['fixups']):raise ValueError('leaf dependency/fixup differs')
    for i in ins:
        if i.mnemonic.startswith('j') and int(i.op_str,16) not in decoded:raise ValueError('branch leaves body')
    if decoded[0x20].mnemonic!='xor' or decoded[0x20].op_str!='ax, ax':raise ValueError('zero flags path differs')
    caller={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[0x1A0:0x2CC],0x1A0)}
    for site,field in ((0x1F9,0x12),(0x213,0x13)):
        expected={site-2:('push','ax'),site-1:('push','cs'),site:('call','0'),site+3:('les','di, ptr [bp - 4]'),
                  site+6:('mov',f'byte ptr es:[di + {field:#x}], al')}
        for ip,(op,operand) in expected.items():
            if caller[ip].mnemonic!=op or caller[ip].op_str!=operand:raise ValueError('aligned near ABI/result store differs')
    raw=[p for p in range(len(code)-2) if code[p]==0xE8 and (p+3+int.from_bytes(code[p+1:p+3],'little',signed=True))&0xFFFF==0]
    if raw!=[0x1F9,0x213]:raise ValueError('complete near caller set differs')
    far=[]
    for other in units:
        c=ovr[other['ovr_offset']:other['ovr_offset']+other['code_size']]
        for fx in other['fixups']:
            if fx>=3 and c[fx-3]==0x9A and int.from_bytes(c[fx:fx+2],'little')==0x3DB and int.from_bytes(c[fx-2:fx],'little')==0x25:
                far.append((other['stub_para'],fx-3))
                starts=sorted(f['rel'] for f in other['functions']);start=max(v for v in starts if v<=fx-3);end=next((v for v in starts if v>start),len(c))
                d={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(c[start:end],start)}
                if start!=0x30D9 or d[fx-3].mnemonic!='lcall' or d[fx-3].op_str!='0x3db, 0x25':raise ValueError('aligned far caller differs')
    if far!=[(0x86,0x3150),(0x86,0x316D)]:raise ValueError('fixup-confirmed far caller set differs')
    return body,decoded,dict(caller_prefix=code[0x1F7:0x1FC],file_extent=[195410,195456])


def library(directory):
    path=directory/'u3c2_0000.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),
        str(ROOT/'src/recovered/overlay_u3c2_signum_0000.c'),'-o',str(path)],check=True,capture_output=True,text=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3c2_signum_0000.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u3c2_signum_0000.restype=ctypes.c_int;return lib


def make_case(body,*,value=0,ss=0x2000,sp=0x8000,ax=0xABCD,flags=0x0202,background=None):
    memory=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu=Cpu(ax,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,0xF000,0,0x1000,0x1111,ss,flags)
    at=physical(ss,sp)
    for i,v in enumerate((0x12,7,cpu.cs & 255,cpu.cs>>8)):memory[(at+i)&(SIZE-1)]=v
    put16(memory,ss,(sp+4)&0xFFFF,value);at=physical(cpu.cs,0);memory[at:at+len(body)]=body;return memory,cpu


def compare(label,memory,cpu,decoded,lib):
    original=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):original.reg_write(reg,getattr(cpu,name))
    entry=physical(cpu.cs,0);visited,edges=set(),set();previous,at_return,zero_path=[None],[False],[False]
    def hook(uc,address,size,user):
        del size,user
        ip=uc.reg_read(UC_X86_REG_IP)
        if not entry<=address<entry+END or ip not in decoded:raise ValueError(f'unexpected transfer {label}')
        if previous[0] in BRANCHES:edges.add((previous[0],ip))
        previous[0]=ip;visited.add(ip)
        if ip==0x20:zero_path[0]=True
        if ip==0x2B:at_return[0]=True;uc.emu_stop()
    h=original.hook_add(UC_HOOK_CODE,hook);original.emu_start(entry,2*SIZE,count=32)
    if not at_return[0]:raise ValueError('original did not reach finite return')
    original.hook_del(h);original.emu_start(physical(cpu.cs,0x2B),2*SIZE,count=1)
    if bytes(backing[entry:entry+END])!=memory[entry:entry+END]:raise ValueError('immutable code changed')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    if lib.pr_u3c2_signum_0000(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected valid case')
    if bytes(ram)!=bytes(backing):
        diffs=[i for i in range(SIZE) if ram[i]!=backing[i]];raise ValueError(f'RAM differs {label}: {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected=original.reg_read(reg)&0xFFFF;mask=0xFFEF if name=='flags' and zero_path[0] else 0xFFFF
        if (getattr(cpu,name)^expected)&mask:raise ValueError(f'{name} differs {label}: {getattr(cpu,name):04X}/{expected:04X}')
    return visited,edges,int(zero_path[0])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--quick',action='store_true');parser.add_argument('--output',type=Path);args=parser.parse_args()
    body,decoded,meta=source();visited,edges=set(),set();count,zeros=0,0
    with tempfile.TemporaryDirectory(prefix='poolrad-u3c2-0000-') as tmp:
        lib=library(Path(tmp))
        def run(label,**options):
            nonlocal visited,edges,count,zeros
            memory,cpu=make_case(body,**options);v,e,z=compare(label,memory,cpu,decoded,lib);visited|=v;edges|=e;zeros+=z;count+=1
        for high in range(0,256,16 if args.quick else 1):
            for low in (0,1,127,128,255):run(f'high-{high}-{low}',value=(high<<8)|low)
        for low in range(0,256,16 if args.quick else 1):
            for high in (0,1,127,128,255):run(f'low-{high}-{low}',value=(high<<8)|low)
        for value in (0,1,2,3,15,16,127,128,255,256,0x7FFE,0x7FFF,0x8000,0x8001,0xFFFE,0xFFFF):run(f'boundary-{value:X}',value=value)
        for ss in (0,0x2000,0xFFFF):
            for sp in (*range(16),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'wrap-{ss:X}-{sp:X}',ss=ss,sp=sp,value=0xFFFF)
        for flags in (0x0202,0x0ED7,0x0047,0x0A93):run(f'zero-flags-{flags:X}',value=0,flags=flags)
        rng=random.Random(0x3C20000)
        for i in range(8 if args.quick else 128):
            run(f'seeded-{i}',value=rng.randrange(65536),ax=rng.randrange(65536),flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    required={(ip,target) for ip in BRANCHES for target in (int(decoded[ip].op_str,16),ip+decoded[ip].size)}
    if visited!=set(decoded) or edges!=required:raise ValueError('instruction/edge coverage incomplete')
    report=dict(target='OVL_03C2_0000',source_sha256=BODY_SHA,cases=count,zero_path_cases=zeros,covered_sites=len(visited),source_sites=len(decoded),
        conditional_edges=len(edges),compared_ram_bytes=SIZE,compared_cpu_words=14,zero_path_undefined_AF_mask=0x10,nonzero_masked_flags=0,
        aligned_near_callers=['01A0:+01F9','01A0:+0213'],aligned_far_callers=['U86:30D9:+3150','U86:30D9:+316D'],public_stub='03DB:0025',
        file_extent=meta['file_extent'],natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
