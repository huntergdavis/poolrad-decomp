#!/usr/bin/env python3
"""Pinned U1C RETURN + real SYSTEM FreeMem: CPU14/full1MiB, unmasked."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
import system_freemem_0254_audit as fm
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,REGISTERS,physical,put16
check=fm.check
CS,SYSTEM,ENTRY,END=0x5000,0x0AF8,0x0A9F,0x0AED
DS,SS,HEAP=fm.DS,fm.SS,fm.HEAP
NAMES={0:'return',2:'CODE',3:'CALLEE',4:'TRANSFER'}
OVR_SHA='872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
BODY_SHA='a0b08ebfbd9bab9e5a735600cbc6120d0e56d67f77f585d2d07705db74d1681c'
SOURCES=('overlay_u1c_return_0a9f_cpu.c','system_freemem_0254_cpu.c')

def source():
    image,decoded,conditional,digests=fm.source()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes();check(sha256(ovr).hexdigest()==OVR_SHA,'GAME.OVR hash')
    blob=ovr[0x17E5:0x1833];check(sha256(blob).hexdigest()==BODY_SHA,'caller hash')
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(blob,ENTRY))
    check(len(ins)==25 and sum(i.size for i in ins)==78 and ins[-1].mnemonic=='retf' and ins[-1].op_str=='','caller ABI')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(0x0AD8,'0xaf8, 0x254')],'caller binding')
    allcode={(SYSTEM,a):i for a,i in decoded.items()};allcode.update({(CS,i.address):i for i in ins})
    edges={(SYSTEM,a):tuple((SYSTEM,b) for b in pair) for a,pair in conditional.items()};edges[(CS,0x0AAD)]=((CS,0x0AAF),(CS,0x0AEB))
    digests['U1C:0A9F-0AED']=dict(bytes=78,instructions=25,sha256=BODY_SHA,file_offset=0x17E5)
    return image,blob,allcode,edges,digests

def library(directory):
    path=directory/'return.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u1c_return_0a9f_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint,ctypes.POINTER(ctypes.c_int)];f.restype=ctypes.c_int
    return f

def build(image,blob,rng,*,pointer=(0x3100,0),next_pointer=(0x3200,8),saved_pc=0xBEEF,pc=0x1234,blocks=(),top=(0x3400,0),org=(0x3000,0),sp=0x8000,ss=SS,flags=0x602,writes=(),background=None,system=SYSTEM):
    m,c,_=fm.build(image,rng,blocks=blocks,top=top,org=org,ss=ss,sp=sp,flags=flags,background=background)
    p=physical(CS,ENTRY);m[p:p+len(blob)]=blob
    if system!=SYSTEM:
        for a,b in fm.SPANS:m[physical(system,a):physical(system,b)]=image[physical(SYSTEM,a):physical(SYSTEM,b)]
        put16(m,CS,0x0ADB,system)
    if pointer is not None:
        for j,v in enumerate((saved_pc,next_pointer[1],next_pointer[0])):put16(m,pointer[0],(pointer[1]+2*j)&65535,v)
    put16(m,DS,0x49ED,pc)
    put16(m,DS,0x6F70,pointer[1] if pointer else 0);put16(m,DS,0x6F72,pointer[0] if pointer else 0)
    for j,v in enumerate((0x7000,0x9001)):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,data in writes:
        p=physical(seg,off)
        for j,v in enumerate(data):m[(p+j)&(SIZE-1)]=v
    c.cs=CS;c.ip=ENTRY
    return m,c

def cases():
    out=[('empty',dict(pointer=None)),('empty-pc-wrap',dict(pointer=None,pc=0xFFFF)),('one',{}),
      ('offset8',dict(pointer=(0x3100,8))),('offset-only',dict(pointer=(0,8))),('next-nil',dict(next_pointer=(0,0))),
      ('next-offset-only',dict(next_pointer=(0,8))),('next-segment-only',dict(next_pointer=(0x3200,0))),
      ('bad-offset',dict(pointer=(0x3100,1))),('below-org',dict(pointer=(0x2FFF,0))),
      ('below-org-off',dict(pointer=(0x3000,0),org=(0x3000,8))),('at-org',dict(pointer=(0x3000,0))),
      ('above-top',dict(pointer=(0x3401,0))),('at-top',dict(pointer=(0x3400,0))),
      ('same-top-seg',dict(pointer=(0x3400,0),top=(0x3400,8))),
      ('duplicate',dict(blocks=((0x3100,0,9,8),))),
      ('same-segment-gap',dict(pointer=(0x3100,8),blocks=((0x3101,8,1,0),))),
      ('merge-next',dict(pointer=(0x3100,8),blocks=((0x3101,0,2,0),))),
      ('merge-prev',dict(pointer=(0x3100,8),blocks=((0x3100,0,0,8),))),
      ('merge-both',dict(pointer=(0x3100,8),blocks=((0x3100,0,0,8),(0x3101,0,2,0)))),
      ('shrink-top',dict(pointer=(0x33FF,8))),
      ('merge-top-offset',dict(blocks=((0x3100,8,0,8),),top=(0x3100,0x10))),
      ('heap-limit',dict(blocks=((0x3001,0,1,0),),heap_cap=0)),
      ('heap-cycle',dict(blocks=((0x3001,0,1,0),),writes=((0x3001,0,struct.pack('<HH',0,0x3001)),),heap_cap=3)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
      ('near-return-alias',dict(ss=HEAP,sp=0x1014)),('far-return-alias',dict(ss=HEAP,sp=0x100C)),
      ('node-word-wrap',dict(pointer=(0x3100,0xFFFF))),('node-a20-wrap',dict(pointer=(0xFFFF,8))),
      ('temp-heap-alias',dict(pointer=(DS+0x6F70//16,0),org=(DS,0))),
      ('pc-stack-alias',dict(ss=DS,sp=0x49EF)),('head-stack-alias',dict(ss=DS,sp=0x6F72)),
      ('temp-stack-alias',dict(ss=DS,sp=0x6F76)),
      ('relocated-system',dict(system=0x7000)),
      ('bad-call-opcode',dict(writes=((CS,0x0AD8,b'\x90'),))),('bad-call-target',dict(writes=((CS,0x0AD9,b'\x55\x02'),))),
      ('empty-bad-call',dict(pointer=None,writes=((CS,0x0AD8,b'\x90'),)))]
    rng=random.Random(0xA9F)
    for n in range(160):
        blocks=[(0x3000+j*0x10,rng.choice((0,8)),rng.randrange(1,8),rng.choice((0,8))) for j in range(1,n%8+1)]
        pointer=(0x3100+rng.randrange(128),rng.choice((0,8)))
        if n%3==0:blocks.append((pointer[0]+(pointer[1]+8)//16,(pointer[1]+8)&15,rng.randrange(1,8),0))
        out.append((f'random-{n}',dict(pointer=pointer,blocks=blocks,next_pointer=(rng.randrange(65536),rng.randrange(65536)),saved_pc=rng.randrange(65536),pc=rng.randrange(65536),flags=fm.FLAGS[n%len(fm.FLAGS)])))
    for sp in (0x2C,0x2D,0x8001,0xFFFA,0xFFFC):out.append((f'sp-{sp:X}',dict(sp=sp)))
    out.append(('random-background',dict(background=rng.randbytes(SIZE))))
    return out

def compare(label,m,c,decoded,conditional,f,heap_cap=64,system=SYSTEM):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,inner=-1,previous=None,returned=False,returning=False,calls=[],walks=0,unexpected=None)
    seen,edges=set(),set()
    def hook(emulator,address,size,user):
        del address,size,user
        cs,ip=emulator.reg_read(UC_X86_REG_CS),emulator.reg_read(UC_X86_REG_IP);key=(SYSTEM if cs==system else cs,ip)
        def stop(s,inner=None):
            state['stop']=s
            if inner is not None:state['inner']=inner
            emulator.emu_stop()
        if state['returned']:return stop(0)
        if state['returning']:
            prev=state['returning'];state['returning']=False
            if (cs,ip)!=state['calls'].pop():return stop(4,0) if prev==(SYSTEM,0x265) else stop(3,fm.TRANSFER)
            if prev==(SYSTEM,0x265):state['inner']=0
        if key==(SYSTEM,0xE2):return stop(3,fm.RUNERROR)
        if key==(SYSTEM,0x40F) and state['previous'] in ((SYSTEM,0x41A),(SYSTEM,0x420)):
            if state['walks']==heap_cap:return stop(3,fm.LIMIT)
            state['walks']+=1
        if key==(CS,0x0AD8):
            call=bytes(emulator.mem_read(physical(CS,0x0AD8),3))
            if call!=b'\x9a\x54\x02':return stop(2)
            state['walks']=0
        if key not in decoded:state['unexpected']=(cs,ip);return emulator.emu_stop()
        if state['previous'] in conditional:edges.add((state['previous'],key))
        seen.add(key);ins=decoded[key]
        if ins.mnemonic in ('call','lcall'):state['calls'].append((cs,ip+ins.size))
        if ins.mnemonic in ('ret','retf'):
            if key==(CS,0x0AEC):state['returned']=True
            else:state['returning']=key
        state['previous']=key
    def mirror(emulator,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);emulator.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror)
    u.emu_start(physical(CS,ENTRY),2*SIZE,count=2000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original did not stop in profile: {label} {state}')
    r=(ctypes.c_uint8*SIZE).from_buffer_copy(m);inner=ctypes.c_int(-999);s=f(r,SIZE,ctypes.byref(c),heap_cap,ctypes.byref(inner))
    check((s,inner.value)==(state['stop'],state['inner']),f'{label}: status {(s,inner.value)} vs {(state["stop"],state["inner"])}')
    want=bytes(u.mem_read(0,SIZE));actual=bytes(r)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b]
        raise ValueError(f'{label}: {len(diff)} RAM mismatches {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):check(getattr(c,name)==u.reg_read(reg)&65535,f'{label}: {name} C{getattr(c,name):04X} DOS{u.reg_read(reg)&65535:04X}')
    return s,inner.value,seen,edges

def differential(image,blob,decoded,conditional,f):
    seen,edges,stops=set(),set(),{};rng=random.Random(0x113F)
    for label,opts in cases():
        opts=dict(opts);heap_cap=opts.pop('heap_cap',64)
        m,c=build(image,blob,rng,**opts)
        s,inner,v,e=compare(label,m,c,decoded,conditional,f,heap_cap,opts.get('system',SYSTEM));seen|=v;edges|=e
        name=NAMES[s]+(f':{inner}' if s==3 else '');stops[name]=stops.get(name,0)+1
        if not label.startswith('random-'):print(label,name,flush=True)
    caller={k for k in decoded if k[0]==CS};caller_edges={(a,b) for a,pair in conditional.items() if a[0]==CS for b in pair}
    check(caller<=seen and caller_edges<=edges,'caller coverage incomplete')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,caller_starts=len(caller&seen),caller_total=len(caller),caller_outcomes=len(caller_edges&edges),caller_outcomes_total=len(caller_edges),closure_starts=len(seen),closure_total=len(decoded),closure_outcomes=len(edges),closure_outcomes_total=2*len(conditional),missing_starts=[f'{s:04X}:{i:04X}' for s,i in sorted(set(decoded)-seen)],missing_outcomes=[f'{s:04X}:{i:04X}->{j:04X}' for (s,i),(_,j) in sorted({(a,b) for a,pair in conditional.items() for b in pair}-edges)])
    print('Original parity',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    image,blob,decoded,conditional,digests=source();report=dict(target=digests)
    with tempfile.TemporaryDirectory(prefix='u1c-0a9f-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,blob,decoded,conditional,f)
        if a.native:
            from u1c_return_0a9f_cpu_native import native_audit
            report['native']=native_audit(image,blob,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()
