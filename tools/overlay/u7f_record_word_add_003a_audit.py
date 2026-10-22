#!/usr/bin/env python3
"""Complete U7F +003A leaf, source/caller pins and full real-mode state parity."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_IP
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
ENTRY,END=0x003A,0x0054
BODY_SHA='ef4d49e3fef030f2acc13a9d5f78563d091bb60ce167bb986143671b6e14059f'
CALLERS={0x0203:(0x024D,0x028B),0x044C:(0x04E9,),0x063A:(0x07A8,0x0805,0x086C,0x091D,0x097E),0x0A88:(0x0B51,)}


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    if len(exe)!=65231 or sha256(exe).hexdigest()!=EXE_SHA or len(ovr)!=206105 or sha256(ovr).hexdigest()!=OVR_SHA:
        raise ValueError('owner-media provenance differs')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x7F)
    if u['ovr_offset']!=150665 or u['memory_segment']!=0x8F:raise ValueError('U7F placement differs')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']];body=code[ENTRY:END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY));decoded={i.address:i for i in ins}
    if len(body)!=26 or sha256(body).hexdigest()!=BODY_SHA or len(ins)!=9 or sum(i.size for i in ins)!=26:
        raise ValueError('leaf extent differs')
    if ins[-1].address!=0x51 or ins[-1].mnemonic!='retf' or ins[-1].op_str!='6':raise ValueError('far return differs')
    if any('call' in i.mnemonic or i.mnemonic.startswith(('j','int')) for i in ins) or any(ENTRY<=p<END for p in u['fixups']):
        raise ValueError('leaf gained a dependency, branch or fixup')
    starts=sorted(f['rel'] for f in u['functions'])
    for start,sites in CALLERS.items():
        end=next(v for v in starts if v>start)
        caller={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[start:end],start)}
        for site in sites:
            if caller[site].mnemonic!='call' or caller[site].op_str!='0x3a' or caller[site-1].mnemonic!='push' or caller[site-1].op_str!='cs':raise ValueError('aligned call differs')
    raw=[p for p in range(len(code)-2) if code[p]==0xE8 and (p+3+int.from_bytes(code[p+1:p+3],'little',signed=True))&0xFFFF==ENTRY]
    if raw!=sorted(p for sites in CALLERS.values() for p in sites):raise ValueError('complete raw caller set differs')
    # Original prefix uses active-record DS globals and an outer increment.
    active={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[0x0203:0x044C],0x0203)}
    for ip,operand in ((0x0241,'word ptr [0x5d94]'),(0x0245,'word ptr [0x5d92]'),(0x0249,'word ptr [bp - 2]')):
        if active[ip].mnemonic!='push' or active[ip].op_str!=operand:raise ValueError('active-record incoming ABI differs')
    party={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[0x063A:0x0A88],0x063A)}
    for ip,operand in ((0x0640,'ax, word ptr [0x5d96]'),(0x0643,'dx, word ptr [0x5d98]'),
        (0x0647,'word ptr [bp - 4], ax'),(0x064A,'word ptr [bp - 2], dx'),
        (0x08A8,'ax, word ptr [0x5d96]'),(0x08AB,'dx, word ptr [0x5d98]'),
        (0x08AF,'word ptr [bp - 4], ax'),(0x08B2,'word ptr [bp - 2], dx'),
        (0x087B,'ax, word ptr es:[di + 0x104]'),(0x0880,'dx, word ptr es:[di + 0x106]'),
        (0x0991,'ax, word ptr es:[di + 0x104]'),(0x0996,'dx, word ptr es:[di + 0x106]')):
        if party[ip].mnemonic!='mov' or party[ip].op_str!=operand:raise ValueError('party-record ownership chain differs')
    return body,decoded,dict(caller_prefix=code[0x0241:0x0250],file_extent=[150723,150749])


CHECKER_ROWS = ('OVL_007F_003A',)


def library(directory):
    path=directory/'u7f_003a.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),
        str(ROOT/'src/recovered/overlay_u7f_record_word_add_003a.c'),'-o',str(path)],check=True,capture_output=True,text=True)
    lib=ctypes.CDLL(str(path));lib.pr_u7f_record_word_add_003a.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u7f_record_word_add_003a.restype=ctypes.c_int;return lib


def make_case(body,*,value=1,increment=1,record=(0x4000,0x1000),ss=0x2000,sp=0x8000,flags=0x0202,background=None):
    memory=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    cpu=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,0xF000,ENTRY,0x1000,0x1111,ss,flags)
    rs,ro=record;put16(memory,rs,(ro+0x102)&0xFFFF,value)
    at=physical(ss,sp)
    for i,b in enumerate((0x12,7,cpu.cs & 255,cpu.cs>>8)):memory[(at+i)&(SIZE-1)]=b
    put16(memory,ss,(sp+4)&0xFFFF,increment)
    at=physical(ss,(sp+6)&0xFFFF)
    for i,b in enumerate((ro & 255,ro>>8,rs & 255,rs>>8)):memory[(at+i)&(SIZE-1)]=b
    at=physical(cpu.cs,ENTRY);memory[at:at+len(body)]=body
    return memory,cpu


def compare(label,memory,cpu,decoded,lib):
    original=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):original.reg_write(reg,getattr(cpu,name))
    entry=physical(cpu.cs,ENTRY);visited=set();at_return=[False]
    def hook(uc,address,size,user):
        del size,user
        ip=uc.reg_read(UC_X86_REG_IP)
        if not entry<=address<entry+END-ENTRY or ip not in decoded:raise ValueError(f'unsupported transfer {label}')
        visited.add(ip)
        if ip==0x51:at_return[0]=True;uc.emu_stop()
    handle=original.hook_add(UC_HOOK_CODE,hook);original.emu_start(entry,2*SIZE,count=20)
    if not at_return[0]:raise ValueError('original failed straight-line bound')
    original.hook_del(handle);original.emu_start(physical(cpu.cs,0x51),2*SIZE,count=1)
    if bytes(backing[entry:entry+END-ENTRY])!=memory[entry:entry+END-ENTRY]:raise ValueError(f'immutable code changed {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    if lib.pr_u7f_record_word_add_003a(ram,SIZE,ctypes.byref(cpu)):raise ValueError('C rejected valid case')
    if bytes(ram)!=bytes(backing):
        diffs=[i for i in range(SIZE) if ram[i]!=backing[i]];raise ValueError(f'RAM differs {label}: {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected=original.reg_read(reg)&0xFFFF
        if getattr(cpu,name)!=expected:raise ValueError(f'{name} differs {label}: {getattr(cpu,name):04X}/{expected:04X}')
    return visited


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--quick',action='store_true');parser.add_argument('--output',type=Path);parser.add_argument('--compile-only',action='store_true');args=parser.parse_args()
    if args.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    body,decoded,meta=source();visited=set();count=0
    with tempfile.TemporaryDirectory(prefix='poolrad-u7f-003a-') as tmp:
        lib=library(Path(tmp))
        def run(label,**options):
            nonlocal visited,count
            memory,cpu=make_case(body,**options);visited|=compare(label,memory,cpu,decoded,lib);count+=1
        for value in range(0,65536,4096 if args.quick else 257):
            for increment in (0,1,15,255,0x7FFF,0x8000,0xFFFF):run(f'sum-{value:X}-{increment:X}',value=value,increment=increment)
        for value in (0,1,15,16,127,128,255,256,0x7FFE,0x7FFF,0x8000,0x8001,0xFFFE,0xFFFF):
            for increment in (0,1,15,16,127,128,255,256,0x7FFF,0x8000,0xFFFF):run(f'boundary-{value:X}-{increment:X}',value=value,increment=increment)
        for ro in range(0x7EE0,0x7F10,4 if args.quick else 1):run(f'record-frame-{ro:X}',record=(0x2000,ro),increment=0x1357)
        for ss in (0,0x2000,0xFFFF):
            for sp in (*range(16),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF):run(f'frame-wrap-{ss:X}-{sp:X}',ss=ss,sp=sp)
        for rs in (0,0x4000,0xFFFF):
            for ro in (0,1,0xFEFC,0xFEFD,0xFEFE,0xFEFF,0xFFFE,0xFFFF):run(f'operand-wrap-{rs:X}-{ro:X}',record=(rs,ro),value=0xFFFF,increment=1)
        rng=random.Random(0x7F003A)
        for i in range(8 if args.quick else 128):
            run(f'seeded-{i}',value=rng.randrange(65536),increment=rng.randrange(65536),
                flags=rng.choice((0x0202,0x0ED7,0x0047,0x0A93)),background=rng.randbytes(SIZE))
    if visited!=set(decoded):raise ValueError('instruction coverage incomplete')
    report=dict(target='OVL_007F_003A',source_sha256=BODY_SHA,cases=count,covered_sites=len(visited),source_sites=len(decoded),
        conditional_edges=0,outgoing_calls=0,compared_ram_bytes=SIZE,compared_cpu_words=14,masked_flags=0,
        aligned_near_callers={f'{start:04X}':[f'{site:04X}' for site in sites] for start,sites in CALLERS.items()},
        file_extent=meta['file_extent'],natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
