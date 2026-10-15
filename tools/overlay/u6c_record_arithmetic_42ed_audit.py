#!/usr/bin/env python3
"""Complete loaded U6C +42ED plus raw arithmetic; stop before SYSTEM error."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import subprocess
import tempfile
from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from unicorn import Uc, UC_ARCH_X86, UC_HOOK_CODE, UC_MODE_16
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

ENTRY,END = 0x42ED,0x4454
BODY_SHA = "c6e6d64b5bc9585934ecdd62ce2f7b5289a9401b74f4abc6d8c4fbd8dd7d3627"
EXTENTS = {"caller":(ENTRY,END,127,BODY_SHA),
 "multiply":(0xA5B,0xA76,16,"74b9ab3b26ee7b54bf51f067f0825dea48772933b210dc5b38897464e5f65745"),
 "divide":(0xA76,0xAE4,55,"131b97677c63a6daa216f4219868f045485e35f999a551feb16daf4ec50d69e5"),
 "absolute":(0xAFC,0xB0B,7,"23987438713f64d2c32a7535df1ebf23ea044fdba8848526cb97817fa84a25e2")}
CALLS = {0x42FE:(0xA5B,0x4303),0x4342:(0xA76,0x4347),0x438F:(0xA5B,0x4394),
         0x43D2:(0xAFC,0x43D7),0x4407:(0xA76,0x440C),0x4426:(0xA5B,0x442B)}
BOUND=100000
UNREACHABLE_SITES = {('caller',0x43C0)} | {('divide',ip) for ip in
    (0xA91,0xA93,0xA95,0xA98,0xABF,0xAC1,0xAC3,0xAC6,0xAC9,0xACA,0xACC)}
UNREACHABLE_EDGES = {('caller',0x43BE,0x43C0),('caller',0x43C7,0x43CC),
    ('divide',0xA8F,0xA91),('divide',0xABD,0xABF),('divide',0xACA,0xAD1),
    ('divide',0xACA,0xACC),('absolute',0xAFE,0xB0A)}
class Layout(ctypes.Structure):
    _fields_=[("system_cs",ctypes.c_uint16)]


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    if len(exe)!=65231 or sha256(exe).hexdigest()!=EXE_SHA or len(ovr)!=206105 or sha256(ovr).hexdigest()!=OVR_SHA:
        raise ValueError('media provenance differs')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x6C)
    if u['ovr_offset']!=104831 or u['memory_segment']!=0x78:raise ValueError('U6C placement differs')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    image,origin,_,_=unpack(exe)
    bodies,decoded,file_extents={},{},{}
    for name,(a,b,count,digest) in EXTENTS.items():
        body=code[a:b] if name=='caller' else bytes(image[0xAF80+a:0xAF80+b])
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,a))
        if sha256(body).hexdigest()!=digest or len(ins)!=count or sum(i.size for i in ins)!=b-a:
            raise ValueError(f'{name} extent differs')
        if name!='caller' and any(origin[p] is None or origin[p+1]!=origin[p]+1 for p in range(0xAF80+a,0xAF80+b-1)):
            raise ValueError('resident source origin differs')
        bodies[name],decoded[name]=body,{i.address:i for i in ins}
        file_extents[name]=[u['ovr_offset']+a,u['ovr_offset']+b] if name=='caller' else [origin[0xAF80+a]+0x200,origin[0xAF80+b-1]+0x201]
    fixups=[p for p in u['fixups'] if ENTRY<=p<END]
    if fixups!=[site+3 for site in CALLS]:raise ValueError('far relocation set differs')
    for site,(target,_) in CALLS.items():
        if decoded['caller'][site].mnemonic!='lcall' or decoded['caller'][site].op_str!=f'0xaf8, {target:#x}':raise ValueError('far call differs')
    if decoded['divide'][0xAE1].op_str!='0xe2':raise ValueError('software error boundary differs')
    starts=sorted(f['rel'] for f in u['functions']);end=next(v for v in starts if v>0x29AE)
    caller={i.address:i for i in Cs(CS_ARCH_X86,CS_MODE_16).disasm(code[0x29AE:end],0x29AE)}
    if caller[0x2ECF].mnemonic!='call' or caller[0x2ECF].op_str!='0x42ed' or caller[0x2ECE].op_str!='cs':raise ValueError('aligned caller differs')
    raw=[p for p in range(len(code)-2) if code[p]==0xE8 and (p+3+int.from_bytes(code[p+1:p+3],'little',signed=True))&0xFFFF==ENTRY]
    if raw!=[0x2ECF]:raise ValueError('raw caller set differs')
    return bodies,decoded,dict(file_extents=file_extents,caller_prefix=code[0x2EBF:0x2ED2])


def library(directory):
    path=directory/'u6c_42ed.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),
        str(ROOT/'src/recovered/overlay_u6c_record_arithmetic_42ed.c'),'-o',str(path)],check=True,capture_output=True,text=True)
    lib=ctypes.CDLL(str(path));lib.pr_u6c_record_arithmetic_42ed.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Layout)]
    lib.pr_u6c_record_arithmetic_42ed.restype=ctypes.c_int
    return lib


def make_case(bodies,*,amount=1,coins=None,denominations=None,record=(0x4000,0x1000),ss=0x2000,sp=0x8000,ds=0x1000,flags=0x0202):
    memory=bytearray([0xA5])*SIZE
    cpu=Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,0xF000,ENTRY,ds,0x1111,ss,flags)
    layout=Layout(0xD000)
    rs,ro=record
    for i,value in enumerate([100]*5 if coins is None else coins):put16(memory,rs,(ro+0x88+2*i)&0xFFFF,value)
    for i,value in enumerate([1,10,20,100,200] if denominations is None else denominations):
        put16(memory,ds,0xD24+4*i,value & 0xFFFF);put16(memory,ds,0xD26+4*i,(value>>16)&0xFFFF)
    at=physical(ss,sp)
    for i,b in enumerate((0x12,7,cpu.cs & 255,cpu.cs>>8)):memory[(at+i)&(SIZE-1)]=b
    at=physical(ss,(sp+4)&0xFFFF)
    for i,b in enumerate((ro & 255,ro>>8,rs & 255,rs>>8)):memory[(at+i)&(SIZE-1)]=b
    put16(memory,ss,(sp+8)&0xFFFF,amount & 0xFFFF);put16(memory,ss,(sp+10)&0xFFFF,(amount>>16)&0xFFFF)
    for name,(a,b,_,_) in EXTENTS.items():
        body=bytearray(bodies[name]);seg=cpu.cs if name=='caller' else layout.system_cs
        if name=='caller':
            for site in CALLS:body[site+3-ENTRY:site+5-ENTRY]=layout.system_cs.to_bytes(2,'little')
        at=physical(seg,a);memory[at:at+b-a]=body
    return memory,cpu,layout


def compare(label,memory,cpu,layout,decoded,lib):
    original=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    original.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));original.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):original.reg_write(reg,getattr(cpu,name))
    caller_cs=cpu.cs
    sites={physical(caller_cs if name=='caller' else layout.system_cs,ip):(name,ip) for name,ins in decoded.items() for ip in ins}
    snapshots=[(physical(caller_cs if name=='caller' else layout.system_cs,a),b-a) for name,(a,b,_,_) in EXTENTS.items()]
    visited,edges=set(),set();previous,pending,stop,undefined=[None],[None],[None],[0]
    call_return=[None]
    def hook(uc,address,size,user):
        del size,user
        if pending[0] is not None:
            cs,ip=uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
            expected=pending[0];pending[0]=None
            if expected=='outer':stop[0]=0;uc.emu_stop();return
            if (cs,ip)!=expected:stop[0]=2;uc.emu_stop();return
        if address & (SIZE-1)==physical(layout.system_cs,0xE2):stop[0]=3;uc.emu_stop();return
        key=sites.get(address & (SIZE-1))
        if key is None:raise ValueError(f'unmodeled dependency {label}')
        name,ip=key
        if previous[0] is not None:
            pn,pi=previous[0];ins=decoded[pn][pi]
            if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp':edges.add((pn,pi,ip))
        previous[0]=key;visited.add(key)
        if name=='caller' and ip in CALLS:call_return[0]=CALLS[ip][1]
        if name!='caller' and decoded[name][ip].mnemonic=='retf':pending[0]=(caller_cs,call_return[0])
        if name=='caller' and ip==0x4451:pending[0]='outer';undefined[0]=0
        if name=='multiply' and ip==0xA75:undefined[0]=0
        if name=='divide' and ip in (0xA79,0xA7D,0xA8C):undefined[0]=0x10
        if name=='divide' and ip in (0xA89,0xA98,0xAC6,0xAD8):undefined[0]=0
        if name=='divide' and ip in (0xABC,0xAC9,0xACE):undefined[0]=0x10
        if name=='absolute' and ip==0xAFC:undefined[0]=0x10
        if name=='absolute' and ip==0xB07:undefined[0]=0
    original.hook_add(UC_HOOK_CODE,hook);original.emu_start(physical(caller_cs,ENTRY),2*SIZE,count=BOUND)
    if stop[0] is None:raise TimeoutError(f'original exceeded {BOUND} instructions: {label}')
    for at,length in snapshots:
        if bytes(backing[at:at+length])!=memory[at:at+length]:raise ValueError(f'immutable code changed {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    status=lib.pr_u6c_record_arithmetic_42ed(ram,SIZE,ctypes.byref(cpu),ctypes.byref(layout))
    if status!=stop[0]:raise ValueError(f'status differs {label}: {status}/{stop[0]}')
    if bytes(ram)!=bytes(backing):
        diffs=[(i,ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]]
        raise ValueError(f'RAM differs {label}: {diffs[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        expected=original.reg_read(reg)&0xFFFF;mask=0xFFFF^undefined[0] if name=='flags' else 0xFFFF
        if (getattr(cpu,name)^expected)&mask:raise ValueError(f'{name} differs {label}: {getattr(cpu,name):04X}/{expected:04X}')
    return visited,edges,status


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path);args=parser.parse_args()
    bodies,decoded,meta=source();visited,edges=set(),set();counts={0:0,2:0,3:0};budget=[]
    with tempfile.TemporaryDirectory(prefix='poolrad-u6c-42ed-') as tmp:
        lib=library(Path(tmp))
        cases=[(f'amount-{a}',dict(amount=a)) for a in (0,1,-1,2,-2,5,-5,1000,-1000,0x80000000,0xFFFFFFFF,0x10000000)]
        cases += [('zero-divisor',dict(denominations=[0]*5)),('negative-divisor',dict(denominations=[-1&0xFFFFFFFF]*5)),
                  ('large-divisor',dict(denominations=[0x10000]*5)),('empty-record',dict(coins=[0]*5)),
                  ('original-1000-sufficient',dict(amount=1000,coins=[10000]*5)),
                  ('mixed-negative-divisor',dict(denominations=[0xFFFFFFFF,10,20,100,1],coins=[0,100,100,100,100])),
                  ('large-first-divisor',dict(denominations=[0x10000,10,20,100,1])),
                  ('large-high-divisor',dict(denominations=[0x10000000,10,20,100,1])),
                  ('negative-high-divisor',dict(denominations=[0x80000000,10,20,100,1],coins=[0,100,100,100,100]))]
        cases += [(f'frame-wrap-{ss:X}-{sp:X}',dict(ss=ss,sp=sp,amount=0))
                  for ss in (0,0x2000,0xFFFF) for sp in (*range(32),0xFFFA,0xFFFB,0xFFFC,0xFFFD,0xFFFE,0xFFFF)]
        cases += [(f'record-frame-{ro:X}',dict(record=(0x2000,ro))) for ro in range(0x7F50,0x8001,4)]
        cases += [(f'table-frame-{sp:X}',dict(ss=0x1000,sp=sp)) for sp in range(0xD20,0xD50,4)]
        for label,options in cases:
            memory,cpu,layout=make_case(bodies,**options)
            try:v,e,status=compare(label,memory,cpu,layout,decoded,lib)
            except TimeoutError:budget.append(label);continue
            visited|=v;edges|=e;counts[status]+=1
    required_sites={(name,ip) for name,ins in decoded.items() for ip in ins}
    required_edges={(name,ip,dest) for name,ins in decoded.items() for ip,i in ins.items()
        if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for dest in (int(i.op_str,16),ip+i.size)}
    if visited != required_sites-UNREACHABLE_SITES or edges != required_edges-UNREACHABLE_EDGES:
        raise ValueError(f'feasible coverage differs: missing {required_sites-UNREACHABLE_SITES-visited}; unexpected {visited & UNREACHABLE_SITES}')
    report=dict(target='OVL_006C_42ED',source_sha256=BODY_SHA,cases=sum(counts.values()),returns=counts[0],transfers=counts[2],
        unsupported_runtime=counts[3],budget_excluded=budget,original_instruction_bound=BOUND,covered_sites=len(visited),
        total_sites=sum(len(d) for d in decoded.values()),infeasible_sites=sorted(UNREACHABLE_SITES),infeasible_edges=sorted(UNREACHABLE_EDGES),
        feasible_sites=len(required_sites-UNREACHABLE_SITES),conditional_edges=len(edges),compared_ram_bytes=SIZE,compared_cpu_words=14,
        missing_sites=[f'{name}:{ip:04X}' for name,ins in decoded.items() for ip in ins if (name,ip) not in visited],
        observed_edges=sorted(edges),normal_return_flags_masked=0,undefined_OR_AF_chosen_zero_in_saved_flag_words=True,natural_game_route_measured=False,
        file_extents=meta['file_extents'])
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
