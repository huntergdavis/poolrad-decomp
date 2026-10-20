#!/usr/bin/env python3
"""Pinned U38 +2E08 and raw +0000: unmasked CPU14/full1MiB comparison."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
CS,DS,SS,HEAP=0x5000,0x1800,0x2800,0x3000
ENTRY,END=0x2E08,0x2E37
SITES=(0x2E0F,0x2E16,0x2E1D,0x2E24)
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
BODY_SHA='7142253f1af3c81015b05a5e1743c3cf19ecf776ec43da551cc00b4e07d374a1'
HELPER_SHA='f62ad6b7474059693853b623f564cfd5c9298cd50e4628732b49b97eb6265240'
CHECKER_ROWS=('OVL_0038_2E08',)
SOURCES=('overlay_u38_clear_cycle_2e08_cpu.c','overlay_u38_helper_cluster_cpu.c')
NAMES={0:'return',2:'CODE',3:'CALLEE',4:'TRANSFER'}
def check(ok,msg):
    if not ok:raise ValueError(msg)
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media identity')
    unit=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x38)
    check(unit['ovr_offset']==0xEC5B and unit['entries'][109]==ENTRY,'unit identity')
    check(not [x for x in unit['fixups'] if ENTRY<=x<END],'no caller fixups')
    code=ovr[unit['ovr_offset']:unit['ovr_offset']+END];decoded={};digests={};md=Cs(CS_ARCH_X86,CS_MODE_16)
    for a,b,count,digest in ((ENTRY,END,23,BODY_SHA),(0,31,11,HELPER_SHA)):
        blob=code[a:b];ins=list(md.disasm(blob,a));check(sha256(blob).hexdigest()==digest and len(ins)==count and sum(i.size for i in ins)==b-a,'span identity')
        decoded.update({i.address:i for i in ins});digests[f'{a:04X}-{b:04X}']=dict(bytes=b-a,instructions=count,sha256=digest)
    check([(i.address,i.op_str) for i in decoded.values() if i.mnemonic=='call']==[(a,'0') for a in SITES],'near bindings')
    check(decoded[0x2E34].mnemonic=='retf' and decoded[0x2E34].op_str=='0xa','caller ABI')
    edges={a:(a+i.size,int(i.op_str,16)) for a,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    return code,decoded,edges,digests

def library(directory):
    path=directory/'cycle.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u38_clear_cycle_2e08_cpu;f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(ctypes.c_int)];f.restype=ctypes.c_int;return f

def build(code,rng,*,latch=0x0B,gate=0,amount=0xA7,value=0x12,ax=0xA5CD,flags=2,ss=SS,ds=DS,cs=CS,sp=0x8000,writes=(),background=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    for a,b in ((0,31),(ENTRY,END)):
        p=physical(cs,a)
        for j,v in enumerate(code[a:b]):m[(p+j)&(SIZE-1)]=v
    for off,v in ((0x6816,value),(0x6817,latch),(0x6818,amount),(0x682A,gate)):m[physical(ds,off)]=v
    for j,v in enumerate((0x7000,0x9001,0xA100,0xA101,0xA102,0xA103,0xA104)):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,data in writes:
        p=physical(seg,off)
        for j,v in enumerate(data):m[(p+j)&(SIZE-1)]=v
    return m,Cpu(ax,*[rng.randrange(65536) for _ in range(6)],sp,cs,ENTRY,ds,rng.randrange(65536),ss,flags)

def cases():
    out=[]
    for latch in range(256):
        for gate in (0,0x80):out.append((f'latch-{latch:02x}-{gate:02x}',dict(latch=latch,gate=gate,flags=FLAGS[latch%6],ax=(latch<<8)|0xCD)))
    for gate in (1,0x7F,0xFF):out.append((f'gate-{gate}',dict(gate=gate)))
    for i,key in enumerate((0x0B,0x35,0x34,0x37)):
        out.append((f'callee-transfer-{i}',dict(cs=0x5000|key,ss=DS,sp=0x681D)))
    for i,site in enumerate(SITES):
        for name,data in (('opcode',b'\x90'),('target',b'\xe8\x00\x00')):out.append((f'bad-{i}-{name}',dict(writes=((CS,site,data),),flags=0x8D7)))
    for sp in (0xA,0xB,0x8001,0xFFFB,0xFFFC):out.append((f'sp-{sp:X}',dict(sp=sp)))
    for sp in (0x6816,0x6817,0x6818,0x6819,0x681A,0x681B,0x681C,0x681D,0x681E,0x681F,0x6820,0x6822,0x682A):out.append((f'stack-alias-{sp:X}',dict(ss=DS,sp=sp)))
    out += [('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('a20-data',dict(ds=0xFFFF)),('a20-code',dict(cs=0xFFFF)),('a20-transfer',dict(cs=0x5037,ds=0xFFFF,ss=0xFFFF,sp=0x681D)),('random-background',dict(background=random.Random(7).randbytes(SIZE)))]
    return out

def compare(label,m,c,decoded,conditional,f):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    caller=c.cs;state=dict(stop=None,inner=-1,previous=None,returned=False,returning=False,calls=[],unexpected=None);seen,edges=set(),set()
    def hook(emulator,address,size,user):
        del address,size,user
        cs,ip=emulator.reg_read(UC_X86_REG_CS),emulator.reg_read(UC_X86_REG_IP)
        def stop(s):state['stop']=s;emulator.emu_stop()
        if state['returned']:return stop(0)
        if state['returning']:
            state['returning']=False;state['inner']=0
            if (cs,ip)!=state['calls'].pop():return stop(4)
        if cs!=caller or ip not in decoded:state['unexpected']=(cs,ip);return emulator.emu_stop()
        if ip in SITES and bytes(emulator.mem_read(physical(cs,ip),3))!=b'\xe8'+struct.pack('<H',(-(ip+3))&65535):return stop(2)
        if state['previous'] in conditional:edges.add((state['previous'],ip))
        seen.add(ip);ins=decoded[ip]
        if ins.mnemonic=='call':state['calls'].append((cs,ip+ins.size))
        if ins.mnemonic=='retf':
            if ip==0x2E34:state['returned']=True
            else:state['returning']=True
        state['previous']=ip
    def mirror(emulator,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);emulator.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror);u.emu_start(physical(caller,ENTRY),2*SIZE,count=10000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original did not stop: {label} {state}')
    r=(ctypes.c_uint8*SIZE).from_buffer_copy(m);inner=ctypes.c_int(-999);s=f(r,SIZE,ctypes.byref(c),ctypes.byref(inner))
    check((s,inner.value)==(state['stop'],state['inner']),f'{label}: status {(s,inner.value)} vs {state}')
    want=bytes(u.mem_read(0,SIZE));actual=bytes(r)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'{label}: {len(diff)} RAM mismatches {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):check(getattr(c,name)==u.reg_read(reg)&65535,f'{label}: {name} C{getattr(c,name):04X} DOS{u.reg_read(reg)&65535:04X}')
    return s,seen,edges

def differential(code,decoded,conditional,f):
    seen,edges,stops=set(),set(),{};rng=random.Random(0x2E08)
    for label,opts in cases():
        m,c=build(code,rng,**opts);s,v,e=compare(label,m,c,decoded,conditional,f);seen|=v;edges|=e;name=NAMES[s];stops[name]=stops.get(name,0)+1
        if not label.startswith('latch-'):print(label,name,flush=True)
    caller={a for a in decoded if ENTRY<=a<END};ce={(a,b) for a,pair in conditional.items() if a in caller for b in pair}
    check(set(decoded)<=seen and ce<=edges,'instruction/caller branch coverage incomplete')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,caller_starts=len(caller&seen),caller_total=len(caller),caller_outcomes=len(ce&edges),caller_outcomes_total=len(ce),closure_starts=len(seen),closure_total=len(decoded),closure_outcomes=len(edges),closure_outcomes_total=2*len(conditional),missing_starts=[f'{i:04X}' for i in sorted(set(decoded)-seen)],missing_outcomes=[f'{i:04X}->{j:04X}' for i,j in sorted({(a,b) for a,pair in conditional.items() for b in pair}-edges)])
    print('Original parity',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u38-2e08-') as tmp:
        f=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        code,decoded,conditional,digests=source();report=dict(target=digests)
        if a.unicorn:report['unicorn']=differential(code,decoded,conditional,f)
        if a.native:
            from u38_clear_cycle_2e08_cpu_native import native_audit
            report['native']=native_audit(code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()
