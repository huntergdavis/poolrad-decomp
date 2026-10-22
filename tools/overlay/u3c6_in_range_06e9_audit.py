#!/usr/bin/env python3
"""Pinned U3C6 entry8 +06E9 predicate, original caller ABI and full RAM/CPU parity."""
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
from unicorn.x86_const import UC_X86_REG_IP,UC_X86_REG_SP,UC_X86_REG_SS
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
ENTRY,END=0x6E9,0x71A
BODY_SHA='1495986874fc88753b5419d14380e9f7846b537a2b7015c57f7e24a1a761a7b9'
BRANCHES=(0x6F3,0x6F9,0x6FF,0x705)
CALLERS=(0x19B,0x615,0x696,0x7A4,0xAB2)


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    if len(exe)!=65231 or sha256(exe).hexdigest()!=EXE_SHA or len(ovr)!=206105 or sha256(ovr).hexdigest()!=OVR_SHA:raise ValueError('owner media differs')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr)
    u=next(u for u in units if u['stub_para']==0x3C6)
    if u['ovr_offset']!=198458 or u['memory_segment']!=0x3E0 or u['entries'][8]!=ENTRY:raise ValueError('U3C6 public entry placement differs')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];body=code[ENTRY:END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY));decoded={i.address:i for i in ins}
    if len(body)!=49 or sha256(body).hexdigest()!=BODY_SHA or len(ins)!=18 or sum(i.size for i in ins)!=49:raise ValueError('predicate extent differs')
    if ins[-1].address!=0x717 or ins[-1].mnemonic!='retf' or ins[-1].op_str!='4':raise ValueError('far return differs')
    if any('call' in i.mnemonic or i.mnemonic.startswith('int') for i in ins) or any(ENTRY<=p<END for p in u['fixups']):raise ValueError('new dependency or relocation')
    for i in ins:
        if i.mnemonic.startswith('j') and int(i.op_str,16) not in decoded:raise ValueError('branch leaves body')
    callers={}
    for f in u['functions']:
        for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[f['rel']:f['rel']+f['size_upper_bound']],f['rel']):callers[i.address]=i
    for site in CALLERS:
        expected={site-2:('push','ax'),site-1:('push','cs'),site:('call','0x6e9'),site+3:('or','al, al')}
        for ip,(op,operand) in expected.items():
            if callers[ip].mnemonic!=op or callers[ip].op_str!=operand:raise ValueError('aligned near caller/result differs')
    for ip,op,operand in ((0x68D,'mov','al, byte ptr [bp + 0xa]'),(0x690,'push','ax'),(0x691,'mov','al, byte ptr [bp + 8]')):
        if callers[ip].mnemonic!=op or callers[ip].op_str!=operand:raise ValueError('original two-argument prefix differs')
    raw=[p for p in range(len(code)-2) if code[p]==0xE8 and (p+3+int.from_bytes(code[p+1:p+3],'little',signed=True))&0xFFFF==ENTRY]
    if raw!=list(CALLERS):raise ValueError('complete raw near caller set differs')
    far=[]
    for other in units:
        c=ovr[other['ovr_offset']:other['ovr_offset']+other['code_size']]
        for fx in other['fixups']:
            if fx>=3 and c[fx-3]==0x9A and int.from_bytes(c[fx:fx+2],'little')==0x3E0 and int.from_bytes(c[fx-2:fx],'little')==0x48:
                f=max((f for f in other['functions'] if f['rel']<=fx-3),key=lambda f:f['rel'])
                d={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(c[f['rel']:f['rel']+f['size_upper_bound']],f['rel'])}
                if f['rel']!=0x712 or d[fx-3].mnemonic!='lcall' or d[fx-3].op_str!='0x3e0, 0x48':raise ValueError('aligned far caller differs')
                far.append((other['stub_para'],fx-3))
    if far!=[(0x5E,0x7F0)]:raise ValueError('complete far-fixup caller set differs')
    return body,decoded,dict(caller_prefix=code[0x68D:0x699],file_extent=[200227,200276])



CHECKER_ROWS = ('OVL_03C6_06E9',)


def library(directory):
    path=directory/'u3c6_06e9.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),
        str(ROOT/'src/recovered/overlay_u3c6_in_range_06e9.c'),'-o',str(path)],check=True,capture_output=True,text=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3c6_in_range_06e9.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u3c6_in_range_06e9.restype=ctypes.c_int;return lib


def make_case(body,*,a=0,b=0,ax=0xABCD,ss=0x2000,sp=0x8000,flags=0x0202,background=None):
    memory=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu=Cpu(ax,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,0xF000,ENTRY,0x1000,0x1111,ss,flags)
    put16(memory,ss,sp,0x0712);put16(memory,ss,(sp+2)&0xFFFF,cpu.cs)
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
        if ip==0x717:at_return[0]=True;uc.emu_stop()
    h=original.hook_add(UC_HOOK_CODE,hook);original.emu_start(entry,2*SIZE,count=32)
    if not at_return[0]:raise ValueError('original failed bounded predicate')
    return_sp=original.reg_read(UC_X86_REG_SP);return_ss=original.reg_read(UC_X86_REG_SS)
    cs_at=physical(return_ss,(return_sp+2)&0xFFFF)
    wrapped_cs=backing[cs_at]|(backing[(cs_at+1)&(SIZE-1)]<<8)
    original.hook_del(h);original.emu_start(physical(cpu.cs,0x717),2*SIZE,count=1)
    if bytes(backing[entry:entry+END-ENTRY])!=memory[entry:entry+END-ENTRY]:raise ValueError('immutable code changed')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    if lib.pr_u3c6_in_range_06e9(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected valid case')
    if bytes(ram)!=bytes(backing):
        diffs=[i for i in range(SIZE) if ram[i]!=backing[i]];raise ValueError(f'RAM differs {label}: {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected=original.reg_read(reg)&0xFFFF
        # Unicorn 2.1.4 RETF reads CS linearly at SP FFFE/FFFF. The
        # original DOSBox-X return vectors establish a separate wrapped
        # word pop. Report these six substitutions, never silently mask CS.
        if name=='cs' and return_sp>=0xFFFE:expected=wrapped_cs
        if getattr(cpu,name)!=expected:raise ValueError(f'{name} differs {label}: {getattr(cpu,name):04X}/{expected:04X}')
    return visited,edges,int(return_sp>=0xFFFE)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--quick',action='store_true');parser.add_argument('--output',type=Path);parser.add_argument('--compile-only',action='store_true');args=parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    body,decoded,meta=source();visited,edges=set(),set();count,wraps=0,0
    with tempfile.TemporaryDirectory(prefix='poolrad-u3c6-06e9-') as tmp:
        lib=library(Path(tmp))
        def run(label,**options):
            nonlocal visited,edges,count,wraps
            memory,cpu=make_case(body,**options);v,e,w=compare(label,memory,cpu,decoded,lib);visited|=v;edges|=e;count+=1;wraps+=w
        for a in range(0,256,16 if args.quick else 1):
            for b in (0,6,7,127,128,255):run(f'a-{a}-{b}',a=a,b=b)
        for b in range(0,256,16 if args.quick else 1):
            for a in (0,6,7,127,128,255):run(f'b-{a}-{b}',a=a,b=b)
        for a in (0,5,6,7,127,128,255):
            for b in (0,5,6,7,127,128,255):run(f'boundary-{a}-{b}',a=a,b=b)
        for high in range(0,256,16 if args.quick else 1):
            for a,b in ((6,6),(7,0),(128,255)):
                run(f'upper-{high}-{a}-{b}',a=(high<<8)|a,b=((255-high)<<8)|b,ax=(high<<8)|0xCD)
        for ss in (0,0x2000,0xFFFF):
            for sp in (*range(16),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'wrap-{ss:X}-{sp:X}',ss=ss,sp=sp,a=255,b=6)
        rng=random.Random(0x3C606E9)
        for i in range(8 if args.quick else 128):
            run(f'seeded-{i}',a=rng.randrange(65536),b=rng.randrange(65536),ax=rng.randrange(65536),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    required={(ip,target) for ip in BRANCHES for target in (int(decoded[ip].op_str,16),ip+decoded[ip].size)}
    if visited!=set(decoded) or edges!=required:raise ValueError('instruction/edge coverage incomplete')
    report=dict(target='OVL_03C6_06E9',source_sha256=BODY_SHA,cases=count,covered_sites=len(visited),source_sites=len(decoded),
        conditional_edges=len(edges),native_verified_retf_cs_expectations=wraps,compared_ram_bytes=SIZE,compared_cpu_words=14,masked_flags=0,aligned_near_callers=['0127:+019B','052B:+0615','052B:+0696','071A:+07A4','0996:+0AB2'],aligned_far_callers=['U5E:0712:+07F0'],public_stub='03E0:0048',
        file_extent=meta['file_extent'],natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
