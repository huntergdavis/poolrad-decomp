#!/usr/bin/env python3
"""U7D [047A,0563) + actual FillChar/03A3/02F2, CPU14/full1MiB oracle."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
from pinned_shift_flags import LastFlagWriter,SPLIT_BY_RULE,declared_flags_split,split_used
SPLIT_ARRIVALS=[0]
from u7d_update_03a3_native_audit import source as unit_source
CS,SYSTEM,ENTRY,END=0x5000,0x0AF8,0x047A,0x0563
DS,SS,HEAP=0x1800,0x2800,0x3000
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
BODY_SHA='471d0464b3c216b69f1c3188b7a8aa2cc69ca1f81109de5217e3f4dd574da8b7'
SOURCES=('overlay_u7d_update_047a_cpu.c','overlay_u7d_update_03a3.c','overlay_u7d_carry_02f2.c','system_runtime_leaves_cpu.c')
NAMES={0:'return',2:'CODE',3:'CALLEE',4:'TRANSFER',5:'LIMIT'}
def check(ok,msg):
    if not ok:raise ValueError(msg)
def source():
    code=unit_source()[:END];check(sha256(code[ENTRY:END]).hexdigest()==BODY_SHA,'caller hash')
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();check(sha256(exe).hexdigest()==EXE_SHA,'START hash')
    image=bytes(unpack(exe)[0]);decoded={};digests={};md=Cs(CS_ARCH_X86,CS_MODE_16)
    spans=((CS,ENTRY,END,code[ENTRY:END],92,BODY_SHA),(CS,0x3A3,0x3D5,code[0x3A3:0x3D5],19,'3cef292a9346dadb89b80e93d5b78f701878140bf60bcd95aaba8d7022fc01ad'),(CS,0x2F2,0x3A3,code[0x2F2:0x3A3],69,'8f39276ad357b6e05367ffc080afedc0df5a5cca2f3cbf54b3d96db18bea3dd9'),(SYSTEM,0x16A8,0x16BC,image[physical(SYSTEM,0x16A8):physical(SYSTEM,0x16BC)],7,'f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7'))
    for seg,a,b,blob,count,digest in spans:
        ins=list(md.disasm(blob,a));check(len(ins)==count and sum(i.size for i in ins)==b-a and sha256(blob).hexdigest()==digest,'span identity')
        decoded.update({(seg,i.address):i for i in ins});digests[f'{seg:04X}:{a:04X}-{b:04X}']=dict(bytes=b-a,instructions=count,sha256=digest)
    check(decoded[(CS,0x560)].mnemonic=='retf' and decoded[(CS,0x560)].op_str=='4','RETF4')
    check([(i.address,i.op_str) for (s,a),i in decoded.items() if s==CS and ENTRY<=a<END and i.mnemonic in ('call','lcall')]==[(0x4F0,'0xaf8, 0x16a8'),(0x55A,'0x3a3')],'bindings')
    edges={k:((k[0],i.address+i.size),(k[0],int(i.op_str,16))) for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    return image,code,decoded,edges,digests
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('status',ctypes.c_int)]
CHECKER_ROWS = ('OVL_007D_047A',)


def library(directory):
    path=directory/'update.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u7d_update_047a_cpu;f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f

def build(image,code,rng,*,values=(8,2,3,4,5,0,0),limits=(10,10,10,10,10,10,10),index=0,amount=3,party=0,sp=0x8000,ss=SS,flags=2,writes=(),background=None,system=SYSTEM):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    span=image[physical(SYSTEM,0):physical(SYSTEM,0x16BC)]
    m[physical(system,0):physical(system,0)+len(span)]=span
    m[physical(CS,0):physical(CS,0)+len(code)]=code
    put16(m,CS,0x4F3,system)
    for j,v in enumerate(values):put16(m,DS,0x6DC2+2*j,v)
    for j,v in enumerate(limits):put16(m,DS,0x363A+2*j,v)
    put16(m,DS,0x5D96,0x8000 if party else 0);put16(m,DS,0x5D98,DS if party else 0)
    for j in range(party):
        node=0x8000+j*0x200;nxt=node+0x200 if j+1<party else 0
        put16(m,DS,node+0x30,0xFFFF if j==0 else j);put16(m,DS,node+0x104,nxt);put16(m,DS,node+0x106,DS if nxt else 0)
    for j,v in enumerate((0x7000,0x9001,index,amount)):put16(m,ss,sp+2*j,v)
    for seg,off,data in writes:
        p=physical(seg,off)
        for j,v in enumerate(data):m[(p+j)&(SIZE-1)]=v
    return m,Cpu(*[rng.randrange(65536) for _ in range(7)],sp,CS,ENTRY,DS,rng.randrange(65536),ss,flags)
def cases():
    out=[('empty',dict(values=(99,0,0,0,0,12,13))),('subtract',{}),('equal',dict(amount=8)),('zero-amount',dict(amount=0)),('borrow',dict(amount=9)),('borrow-repeat',dict(values=(0,0,0,0,4,0,0),amount=255)),('clear',dict(values=(0,0,0,0,1,0,0),index=4,amount=2)),('carry-party',dict(values=(20,20,20,20,20,20,20),party=3)),('cap99',dict(values=(20,20,20,20,20,20,20),limits=(0xFFFF,)*7)),('signed-index',dict(index=0xFFFF)),('high-arg-bytes',dict(index=0xAB00,amount=0xCD03)),('signed-wrap-scan',dict(index=127,amount=255,writes=((DS,0x6EC0,b'\x00\x00'),))),('no-progress',dict(values=(0,1,0,0,0,0,0),limits=(0,)*7,amount=1,cap=400)),('cap0',dict(cap=0)),('cap1',dict(cap=1)),('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('relocated-fill',dict(values=(0,0,0,0,1,0,0),index=4,amount=2,system=0x7000)),('random-background',dict(background=random.Random(7).randbytes(SIZE)))]
    out += [('fill-return-alias',dict(ss=DS,sp=0x6DD4,values=(0,0,0,0,1,0,0),index=4,amount=2)),('update-return-alias',dict(ss=DS,sp=0x6DCC)),('carry-return-alias',dict(ss=DS,sp=0x6DD6))]
    for j in range(1,5):
        values=[0]*7;values[j]=1;out.append((f'gate-{j}',dict(values=values,amount=1)))
    for index in (0,1,2,3,4,5,6,127,128,254,255):out.append((f'index-{index}',dict(index=index)))
    for a,data in ((0x4F0,b'\x90'),(0x4F1,b'\xa9\x16'),(0x55A,b'\x90'),(0x55B,b'\x45\xfe')):
        out.append((f'bad-call-{a:04x}',dict(values=(0,0,0,0,1,0,0),index=4,amount=2,writes=((CS,a,data),))))
    for sp in (0x20,0x21,0x8001,0xFFF7,0xFFF8):out.append((f'sp-{sp:x}',dict(sp=sp)))
    rng=random.Random(0x47A)
    for n in range(160):
        limits=[rng.randrange(1,80) for _ in range(7)];values=[rng.randrange(65536) if n%3==0 else rng.randrange(limits[j]) for j in range(7)]
        out.append((f'random-{n}',dict(values=values,limits=limits,index=n%5,amount=rng.randrange(256),party=n%5,flags=FLAGS[n%6])))
    # Prefix stops expose transient flags and partial writes, not only final state.
    for cap in range(0,160):out.append((f'prefix-{cap}',dict(values=(0,0,0,1,0,0,0),amount=20,cap=cap)))
    return out

def compare(label,m,c,decoded,conditional,f,cap=10000,system=SYSTEM):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,inner=-1,callee=-1,previous=None,returned=False,returning=None,calls=[],steps=0,unexpected=None)
    seen,edges=set(),set()
    writer=LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags
    def hook(emulator,address,size,user):
        del size,user
        cs,ip=emulator.reg_read(UC_X86_REG_CS),emulator.reg_read(UC_X86_REG_IP);key=(SYSTEM if cs==system else cs,ip)
        def stop(s,inner=None):
            state['stop']=s
            if inner is not None:state['inner']=inner
            emulator.emu_stop()
        if state['returned']:return stop(0)
        if state['returning']:
            prev=state['returning'];state['returning']=None
            if (cs,ip)!=state['calls'].pop():return stop(3,3) if prev==(CS,0x3A0) else stop(4,0)
            if prev in ((SYSTEM,0x16B9),(CS,0x3D4)):state['inner']=0
        if cs==CS and ENTRY<=ip<END:
            if state['steps']==cap:return stop(5)
            state['steps']+=1
        if key in ((CS,0x4F0),(CS,0x55A)):
            want=b'\x9a\xa8\x16' if ip==0x4F0 else b'\xe8\x46\xfe'
            if bytes(emulator.mem_read(physical(CS,ip),3))!=want:return stop(2)
            state['callee']=int(ip==0x55A)
        if key not in decoded:state['unexpected']=(cs,ip);return emulator.emu_stop()
        if state['previous'] in conditional:edges.add((state['previous'],key))
        seen.add(key);ins=decoded[key]
        if ins.mnemonic in ('call','lcall'):state['calls'].append((cs,ip+ins.size))
        if ins.mnemonic in ('ret','retf'):
            if key==(CS,0x560):state['returned']=True
            else:state['returning']=key
        writer.step(emulator,address)
        state['previous']=key
    def mirror(emulator,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);emulator.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror)
    u.emu_start(physical(CS,ENTRY),2*SIZE,count=2000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original did not stop: {label} {state}')
    writer.finish(u)
    r=(ctypes.c_uint8*SIZE).from_buffer_copy(m);inner=Diagnostic(-999,-999);s=f(r,SIZE,ctypes.byref(c),cap,ctypes.byref(inner))
    check((s,inner.callee,inner.status)==(state['stop'],state['callee'],state['inner']),f'{label}: status {(s,inner.callee,inner.status)} vs {state}')
    want=bytes(u.mem_read(0,SIZE));actual=bytes(r)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'{label}: {len(diff)} RAM mismatches {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        if name=='flags':
            problem=declared_flags_split(c.flags,u.reg_read(reg)&65535,writer);check(problem is None,f'{label}: {problem}')
            SPLIT_ARRIVALS[0]+=split_used(c.flags,u.reg_read(reg)&65535,writer);continue
        check(getattr(c,name)==u.reg_read(reg)&65535,f'{label}: {name} C{getattr(c,name):04X} DOS{u.reg_read(reg)&65535:04X}')
    return s,inner.status,seen,edges

def differential(image,code,decoded,conditional,f):
    seen,edges,stops=set(),set(),{};rng=random.Random(0x047A)
    for label,opts in cases():
        opts=dict(opts);cap=opts.pop('cap',10000);m,c=build(image,code,rng,**opts)
        s,inner,v,e=compare(label,m,c,decoded,conditional,f,cap,opts.get('system',SYSTEM));seen|=v;edges|=e
        name=NAMES[s]+(f':{inner}' if s==3 else '');stops[name]=stops.get(name,0)+1
        if not label.startswith(('random-','prefix-')):print(label,name,flush=True)
    caller={k for k in decoded if k[0]==CS and ENTRY<=k[1]<END};ce={(a,b) for a,pair in conditional.items() if a in caller for b in pair}
    check(caller<=seen,'caller starts incomplete')
    report=dict(cases=sum(stops.values()),stops=stops,declared_flag_split_arrivals=SPLIT_ARRIVALS[0],declared_flag_split_by_rule=dict(SPLIT_BY_RULE),cpu_words=14,ram_bytes=SIZE,caller_starts=len(caller&seen),caller_total=len(caller),caller_outcomes=len(ce&edges),caller_outcomes_total=len(ce),closure_starts=len(seen),closure_total=len(decoded),closure_outcomes=len(edges),closure_outcomes_total=2*len(conditional),missing_starts=[f'{s:04X}:{i:04X}' for s,i in sorted(set(decoded)-seen)],missing_outcomes=[f'{s:04X}:{i:04X}->{j:04X}' for (s,i),(_,j) in sorted({(a,b) for a,pair in conditional.items() for b in pair}-edges)])
    print('Original parity',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    image,code,decoded,conditional,digests=source();report=dict(target=digests)
    with tempfile.TemporaryDirectory(prefix='u7d-047a-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code,decoded,conditional,f)
        if a.native:
            from u7d_update_047a_cpu_native import native_audit
            report['native']=native_audit(image,code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()
