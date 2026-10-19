#!/usr/bin/env python3
"""Pinned U1C CLEARMONSTERS + real SYSTEM FreeMem: CPU14/full1MiB, unmasked."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
import system_freemem_0254_audit as fm
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,REGISTERS,physical,put16
check=fm.check
CS,SYSTEM,ENTRY,END=0x5000,0x0AF8,0x125D,0x12C6
DS,SS,HEAP=fm.DS,fm.SS,fm.HEAP
NAMES={0:'return',2:'CODE',3:'CALLEE',4:'TRANSFER',5:'LIMIT'}
OVR_SHA='872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
BODY_SHA='8112f2f70d8905153070bfe84d60f102d3a7e1e2aaa89e26c81fd79b10f4479d'
SOURCES=('overlay_u1c_clearmonsters_125d_cpu.c','system_freemem_0254_cpu.c','system_runtime_leaves_cpu.c')

def source():
    image,decoded,conditional,digests=fm.source()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes();check(sha256(ovr).hexdigest()==OVR_SHA,'GAME.OVR hash')
    blob=ovr[0x1FA3:0x200C];check(sha256(blob).hexdigest()==BODY_SHA,'caller hash')
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(blob,ENTRY))
    check(len(ins)==36 and sum(i.size for i in ins)==105 and ins[-1].mnemonic=='retf' and ins[-1].op_str=='','caller ABI')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(0x1282,'0xaf8, 0x16a8'),(0x12AE,'0xaf8, 0x254')],'caller binding')
    fill=image[physical(SYSTEM,0x16A8):physical(SYSTEM,0x16BC)]
    check(sha256(fill).hexdigest()=='f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7','FillChar hash')
    fi=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(fill,0x16A8));check(len(fi)==7,'FillChar starts')
    decoded.update({i.address:i for i in fi})
    digests['16A8-16BC']=dict(bytes=20,instructions=7,sha256=sha256(fill).hexdigest())
    allcode={(SYSTEM,a):i for a,i in decoded.items()};allcode.update({(CS,i.address):i for i in ins})
    edges={(SYSTEM,a):tuple((SYSTEM,b) for b in pair) for a,pair in conditional.items()};edges[(CS,0x128E)]=((CS,0x1290),(CS,0x12C2))
    digests['U1C:125D-12C6']=dict(bytes=105,instructions=36,sha256=BODY_SHA,file_offset=0x1FA3)
    return image,blob,allcode,edges,digests

class Diagnostic(ctypes.Structure):
    _fields_=[("callee",ctypes.c_int),("status",ctypes.c_int)]

def library(directory):
    path=directory/'clearmonsters.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u1c_clearmonsters_125d_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    return f

def build(image,blob,rng,*,nodes=((0x3100,0),),blocks=(),top=(0x3400,0),org=(0x3000,0),pc=0x1234,sp=0x8000,ss=SS,flags=0x602,writes=(),cycle=None,background=None,system=SYSTEM):
    m,c,_=fm.build(image,rng,blocks=blocks,top=top,org=org,ss=ss,sp=sp,flags=flags,background=background)
    p=physical(CS,ENTRY);m[p:p+len(blob)]=blob
    if system!=SYSTEM:
        for a,b in (*fm.SPANS,(0x16A8,0x16BC)):m[physical(system,a):physical(system,b)]=image[physical(SYSTEM,a):physical(SYSTEM,b)]
        put16(m,CS,0x1285,system);put16(m,CS,0x12B1,system)
    for j,node in enumerate(nodes):
        nxt=nodes[j+1] if j+1<len(nodes) else (nodes[cycle] if cycle is not None else (0,0))
        put16(m,node[0],(node[1]+0x2A)&65535,nxt[1]);put16(m,node[0],(node[1]+0x2C)&65535,nxt[0])
    head=nodes[0] if nodes else (0,0)
    put16(m,DS,0x49ED,pc);put16(m,DS,0x6810,head[1]);put16(m,DS,0x6812,head[0])
    for j,v in enumerate((0x7000,0x9001)):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,data in writes:
        p=physical(seg,off)
        for j,v in enumerate(data):m[(p+j)&(SIZE-1)]=v
    c.cs=CS;c.ip=ENTRY
    return m,c

def cases():
    out=[('empty',dict(nodes=())),('empty-pc-wrap',dict(nodes=(),pc=0xFFFF)),('one',{}),
      ('two',dict(nodes=((0x3100,0),(0x3200,8)))),('reverse',dict(nodes=((0x3200,0),(0x3100,0)))),
      ('adjacent',dict(nodes=((0x3100,0),(0x3104,0),(0x3108,0)))),
      ('adjacent-reverse',dict(nodes=((0x3108,0),(0x3104,0),(0x3100,0)))),
      ('merge-both',dict(blocks=((0x30FC,0,4,0),(0x3104,0,4,0)))),
      ('top',dict(nodes=((0x33FC,0),))),('top-chain',dict(nodes=((0x33F8,0),(0x33FC,0)))),
      ('offset8',dict(nodes=((0x3100,8),))),('offset-only',dict(nodes=((0,8),))),
      ('bad-offset',dict(nodes=((0x3100,1),))),('below-org',dict(nodes=((0x2FFF,0),))),
      ('below-org-off',dict(nodes=((0x3000,0),),org=(0x3000,8))),('above-top',dict(nodes=((0x3401,0),))),
      ('at-org',dict(nodes=((0x3000,0),))),('at-top',dict(nodes=((0x3400,0),))),
      ('same-top-seg',dict(nodes=((0x3400,0),),top=(0x3400,8))),
      ('duplicate',dict(blocks=((0x3100,0,9,8),))),('self-cycle',dict(cycle=0)),
      ('cycle-two',dict(nodes=((0x3100,0),(0x3200,0)),cycle=0)),
      ('list-limit',dict(cap=0)),('list-limit-two',dict(nodes=((0x3100,0),(0x3200,0)),cap=1)),
      ('heap-limit',dict(blocks=((0x3001,0,1,0),),heap_cap=0)),
      ('heap-cycle',dict(blocks=((0x3001,0,1,0),),writes=((0x3001,0,struct.pack('<HH',0,0x3001)),),heap_cap=3)),
      ('same-seg-walk',dict(nodes=((0x3100,8),),blocks=((0x3100,0,0,8),))),
      ('merge-top-offset',dict(blocks=((0x3104,0,0,8),),top=(0x3104,8))),
      ('same-segment-gap',dict(blocks=((0x3104,8,1,0),))),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
      ('fill-return-alias',dict(ss=DS,sp=0x6808)),
      ('near-return-alias',dict(ss=HEAP,sp=0x101A)),('far-return-alias',dict(ss=HEAP,sp=0x1012)),
      ('pc-stack-alias',dict(ss=DS,sp=0x49EF)),('head-stack-alias',dict(ss=DS,sp=0x6812,cap=0)),
      ('node-link-wrap',dict(nodes=((0x3100,0xFFD5),))),
      ('relocated-system',dict(system=0x7000)),
      ('bad-fill-opcode',dict(writes=((CS,0x1282,b'\x90'),))),('bad-fill-target',dict(writes=((CS,0x1283,b'\xa9\x16'),))),
      ('bad-free-opcode',dict(writes=((CS,0x12AE,b'\x90'),))),('bad-free-target',dict(writes=((CS,0x12AF,b'\x55\x02'),))),
      ('empty-bad-free',dict(nodes=(),writes=((CS,0x12AE,b'\x90'),)))]
    rng=random.Random(0x125D)
    for n in range(140):
        nodes=[(0x3100+16*j,rng.choice((0,8))) for j in range(n%9)];rng.shuffle(nodes)
        blocks=[(0x3000+16*j,0,1,0) for j in range(1,n%5+1)]
        out.append((f'random-{n}',dict(nodes=nodes,blocks=blocks,pc=rng.randrange(65536),flags=fm.FLAGS[n%len(fm.FLAGS)])))
    for pc in (0,0xF,0x7FFF,0x8000,0xFFFF):out.append((f'fill-refusal-pc-{pc:04X}',dict(pc=pc,writes=((CS,0x1282,b'\x90'),))))
    for sp in (0x32,0x33,0x8001,0xFFFA,0xFFFC):out.append((f'sp-{sp:X}',dict(sp=sp)))
    out.append(('random-background',dict(background=rng.randbytes(SIZE))))
    return out

def compare(label,m,c,decoded,conditional,f,cap=64,heap_cap=64,system=SYSTEM):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,inner=-1,callee=-1,previous=None,returned=False,returning=False,calls=[],loops=0,walks=0,unexpected=None)
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
            if (cs,ip)!=state['calls'].pop():return stop(4,0) if prev in ((SYSTEM,0x265),(SYSTEM,0x16B9)) else stop(3,fm.TRANSFER)
            if prev in ((SYSTEM,0x265),(SYSTEM,0x16B9)):state['inner']=0
        if key==(SYSTEM,0xE2):return stop(3,fm.RUNERROR)
        if key==(CS,0x1287) and state['previous']==(CS,0x12C0):
            if state['loops']==cap:return stop(5)
            state['loops']+=1
        if key==(SYSTEM,0x40F) and state['previous'] in ((SYSTEM,0x41A),(SYSTEM,0x420)):
            if state['walks']==heap_cap:return stop(3,fm.LIMIT)
            state['walks']+=1
        if key in ((CS,0x1282),(CS,0x12AE)):
            off=0x16A8 if ip==0x1282 else 0x254
            call=bytes(emulator.mem_read(physical(CS,ip),3))
            if call!=b'\x9a'+struct.pack('<H',off):return stop(2)
            state['walks']=0;state['callee']=0 if ip==0x1282 else 1
        if key not in decoded:state['unexpected']=(cs,ip);return emulator.emu_stop()
        if state['previous'] in conditional:edges.add((state['previous'],key))
        seen.add(key);ins=decoded[key]
        if ins.mnemonic in ('call','lcall'):state['calls'].append((cs,ip+ins.size))
        if ins.mnemonic in ('ret','retf'):
            if key==(CS,0x12C5):state['returned']=True
            else:state['returning']=key
        state['previous']=key
    def mirror(emulator,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);emulator.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror)
    u.emu_start(physical(CS,ENTRY),2*SIZE,count=2000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original did not stop in profile: {label} {state}')
    r=(ctypes.c_uint8*SIZE).from_buffer_copy(m);inner=Diagnostic(-999,-999);s=f(r,SIZE,ctypes.byref(c),cap,heap_cap,ctypes.byref(inner))
    check((s,inner.callee,inner.status)==(state['stop'],state['callee'],state['inner']),f'{label}: status {(s,inner.status)} vs {(state["stop"],state["inner"])}')
    want=bytes(u.mem_read(0,SIZE));actual=bytes(r)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b]
        raise ValueError(f'{label}: {len(diff)} RAM mismatches {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):check(getattr(c,name)==u.reg_read(reg)&65535,f'{label}: {name} C{getattr(c,name):04X} DOS{u.reg_read(reg)&65535:04X}')
    return s,inner.status,seen,edges

def differential(image,blob,decoded,conditional,f):
    seen,edges,stops=set(),set(),{};rng=random.Random(0x113F)
    for label,opts in cases():
        opts=dict(opts);cap=opts.pop('cap',64);heap_cap=opts.pop('heap_cap',64)
        m,c=build(image,blob,rng,**opts)
        s,inner,v,e=compare(label,m,c,decoded,conditional,f,cap,heap_cap,opts.get('system',SYSTEM));seen|=v;edges|=e
        name=NAMES[s]+(f':{inner}' if s==3 else '');stops[name]=stops.get(name,0)+1
        if not label.startswith('random-'):print(label,name,flush=True)
    caller={k for k in decoded if k[0]==CS};caller_edges={(a,b) for a,pair in conditional.items() if a[0]==CS for b in pair}
    check(caller<=seen and caller_edges<=edges,'caller coverage incomplete')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,caller_starts=len(caller&seen),caller_total=len(caller),caller_outcomes=len(caller_edges&edges),caller_outcomes_total=len(caller_edges),closure_starts=len(seen),closure_total=len(decoded),closure_outcomes=len(edges),closure_outcomes_total=2*len(conditional),missing_starts=[f'{s:04X}:{i:04X}' for s,i in sorted(set(decoded)-seen)],missing_outcomes=[f'{s:04X}:{i:04X}->{j:04X}' for (s,i),(_,j) in sorted({(a,b) for a,pair in conditional.items() for b in pair}-edges)])
    print('Original parity',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    image,blob,decoded,conditional,digests=source();report=dict(target=digests)
    with tempfile.TemporaryDirectory(prefix='u1c-125d-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,blob,decoded,conditional,f)
        if a.native:
            from u1c_clearmonsters_125d_cpu_native import native_audit
            report['native']=native_audit(image,blob,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()
