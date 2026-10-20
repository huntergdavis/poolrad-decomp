#!/usr/bin/env python3
"""RES_0464 +2595 original-byte CPU14/full1MiB differential.
Loader/media and synthetic native artifacts remain in ignored resources/.
Reuses SYSTEM FreeMem and U6C release harness mechanics, not their results.
"""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import relocations
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
import system_freemem_0254_audit as fm
check=fm.check
CHECKER_ROWS=('RES_0464_2595',)
CS,DS,SS,SYSTEM=0x5000,0x1800,0x2800,0x0AF8
ENTRY,END,RETF=0x2595,0x2632,0x262F
SITES=(0x25F0,0x2605,0x261B)
OK,CALLEE,TRANSFER=0,2,3
NAMES={OK:'return',CALLEE:'CALLEE',TRANSFER:'TRANSFER'}
SOURCES=['resident_0464_bitmap_release_2595_cpu.c','system_freemem_0254_cpu.c']
FLAGS=fm.FLAGS
class Diagnostic(ctypes.Structure):
    _fields_=[('freemem_status',ctypes.c_int),('call_site',ctypes.c_uint16),('calls',ctypes.c_uint)]
def source():
    image,leaf,leaf_edges,digests=fm.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest()==OVR_SHA,'GAME provenance')
    body=image[physical(0x464,ENTRY):physical(0x464,END)]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,ENTRY))
    check(len(body)==157 and len(ins)==52 and sha256(body).hexdigest()=='d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700','2595 identity')
    check((ins[-1].address,ins[-1].mnemonic,ins[-1].op_str)==(RETF,'retf','4'),'RETF4 extent')
    relocs,_=relocations(exe,image)
    check(all(physical(0x464,a+3) in relocs for a in SITES),'loader-backed FreeMem segment bindings')
    check([(i.address,i.op_str) for i in ins if 'call' in i.mnemonic]==[(a,'0xaf8, 0x254') for a in SITES],'three FreeMem calls only')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);callers=[]
    expected={0x17:(0x6F8,),0x2F:(0x5F,),0x33:(0x1E46,0x1E50),0x68:(0x108A,0x1094),0x3B9:(0x4CA,),0x3CE:(0xE1,0x17A,0x1A5,0x635)}
    for unit,sites in expected.items():
        u=next(u for u in units if u['stub_para']==unit)
        for site in sites:
            at=u['ovr_offset']+site
            got=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(ovr[at:at+5],site))
            check(len(got)==1 and got[0].mnemonic=='lcall' and got[0].op_str=='0x464, 0x2595' and site+3 in u['fixups'],'fixup-backed incoming call')
            callers.append(dict(unit=f'{unit:04X}',site=f'{site:04X}'))
    decoded={i.address:i for i in ins}
    conditional={i.address:(i.address+i.size,int(i.op_str,16)) for i in ins if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    meta=dict(target='RES_0464_2595',bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),expanded_span=[0x6BD5,0x6C72],unit_span=[ENTRY,END],callers=callers,freemem_spans=digests)
    return body,image,decoded,conditional,leaf,leaf_edges,meta

def library(directory):
    p=directory/'release.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_r0464_bitmap_release_2595_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_uint,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    return f

def pair(m,s,o,p):
    at=physical(s,o&65535)
    for j,v in enumerate(struct.pack('<HH',p[1],p[0])):m[(at+j)&(SIZE-1)]=v

def build(body,image,rng,*,slot=(DS,0x6000),record=(0x3100,0),aux=(0,0),planes=1,stride=16,mode=0,
          blocks=(),top=(0x3800,0),org=(0x3000,0),sp=0x8000,ss=SS,flags=0x602,writes=(),background=None,system=SYSTEM,first=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background);m[:len(image)]=image
    fm.gm.heap(m,blocks=blocks,top=top,end=(0x4000,0),first=first);pair(m,DS,0x4400,org)
    if record!=(0,0):
        m[physical(record[0],(record[1]+8)&65535)]=planes
        put16(m,record[0],(record[1]+0x11)&65535,stride)
        pair(m,record[0],(record[1]+0x13)&65535,aux)
    pair(m,slot[0],slot[1],record);m[physical(DS,0x4A1E)]=mode
    for j,v in enumerate((0x7000,0x9001,slot[1],slot[0])):put16(m,ss,(sp+2*j)&65535,v)
    at=physical(CS,ENTRY);m[at:at+len(body)]=body
    for site in SITES:put16(m,CS,site+3,system)
    if system!=SYSTEM:
        for a,b in fm.SPANS:
            at=physical(system,a);m[at:at+b-a]=image[physical(SYSTEM,a):physical(SYSTEM,b)]
    for seg,off,blob in writes:
        at=physical(seg,off)
        for j,v in enumerate(blob):m[(at+j)&(SIZE-1)]=v
    c=Cpu(*[rng.randrange(65536) for _ in range(7)],sp,CS,ENTRY,DS,rng.randrange(65536),ss,flags)
    return m,c

def cases():
    result=[('aux-at-top',dict(aux=(0x3800,0))),('nil',dict(record=(0,0))),('no-aux',{}),('aux-mode0',dict(aux=(0x3200,0))),
      ('aux-mode1',dict(aux=(0x3200,8),mode=1)),('aux-mode255',dict(aux=(0x3200,0),mode=255)),
      ('zero-product',dict(planes=0,aux=(0,1))),('quarter-zero',dict(stride=3,mode=1,aux=(0,1))),
      ('product-wrap',dict(planes=255,stride=65535)),('primary-size-zero',dict(planes=1,stride=65513)),
      ('bad-primary',dict(record=(0x3100,1))),('bad-aux-mode0',dict(aux=(0x3200,1))),
      ('bad-aux-mode1',dict(aux=(0x3200,1),mode=1)),('bad-primary-after-aux',dict(record=(0x3100,1),aux=(0x3200,0))),
      ('duplicate-primary',dict(blocks=((0x3100,0,2,0),))),
      ('duplicate-aux',dict(aux=(0x3200,0),blocks=((0x3200,0,2,0),))),
      ('merge-next',dict(blocks=((0x3102,8,4,0),))),('merge-prev',dict(blocks=((0x30FF,0,1,0),))),
      ('merge-both',dict(blocks=((0x30FF,0,1,0),(0x3102,8,4,0)))),
      ('shrink-top',dict(top=(0x3102,8))),('merge-prev-top',dict(top=(0x3102,8),blocks=((0x30FF,0,1,0),))),
      ('list-limit',dict(blocks=((0x3001,0,1,0),(0x3003,0,1,0)),cap=0)),
      ('list-cycle',dict(blocks=((0x3001,0,1,0),),writes=((0x3001,0,struct.pack('<HH',0,0x3001)),),cap=3)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('slot-wrap',dict(slot=(DS,0xFFFF))),
      ('record-wrap',dict(record=(0x3100,0xFFF8))),('relocated-system',dict(system=0x6000)),
      ('slot-in-bitmap',dict(slot=(0x3100,0x30))),('slot-in-aux',dict(slot=(0x3200,0x30),aux=(0x3200,0))),
      ('aux-is-primary',dict(aux=(0x3100,0))),('slot-on-heap-head',dict(slot=(DS,0x4408))),
      ('partial-null-segment',dict(record=(0,0xF000))),('partial-null-offset',dict(record=(0x3100,0))),
    ]
    result.append(('callee-return-alias',dict(ss=0x3000,sp=0x101A,record=(0x3300,0),aux=(0x3100,0))))
    for off in (0x7FF6,0x7FF8,0x7FFA,0x7FFC):
        result.append((f'stack-slot-{off:x}',dict(slot=(SS,off))))
    for mode in (0,1,2,3):
        for product in (0,1,3,4,7,8,15,16,17,65513,65535):
            result.append((f'product-{mode}-{product}',dict(mode=mode,stride=product,aux=(0x3200,8))))
    for sp in (0x40,0x41,0x8001,0xFFF7,0xFFF8):result.append((f'sp-{sp:x}',dict(sp=sp,aux=(0x3200,0),mode=1)))
    # Run FreeMem's targeted cases through the optional allocation call.
    # Its pointer/size become real header fields; no callee is bypassed.
    for name,opts in fm.cases():
        if name.startswith('random-') or any(k in opts for k in ('ss','sp','background')) or name.startswith(('alias-','globals-')):continue
        opts=dict(opts);opts['aux']=opts.pop('pointer',(0x3100,0));opts['stride']=opts.pop('size',16)
        opts['record']=(0x3700,0);opts['planes']=1;opts['mode']=0
        result.append(('freemem-'+name,opts))
    rng=random.Random(0x4642595)
    for n in range(256):
        result.append((f'random-{n}',dict(planes=n,stride=rng.randrange(65536),mode=n%4,aux=(0x3200,rng.choice((0,8))) if n%3 else (0,0),record=(0x3100,rng.choice((0,8))),flags=FLAGS[n%len(FLAGS)],blocks=tuple((0x3001+j*8,0,1,0) for j in range(n%5)))))
    result.append(('random-ram',dict(background=rng.randbytes(SIZE))))
    return result
def compare(label,m,c,decoded,conditional,leaf,leaf_edges,f,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),r in zip(Cpu._fields_,REGISTERS):u.reg_write(r,getattr(c,name))
    systems={struct.unpack_from('<H',m,physical(CS,s+3))[0] for s in SITES}
    state=dict(stop=None,returning=None,root_return=False,stack=[],previous=None,last=None,heap_loops=0,
        inner=-1,site=0,unexpected=None,release_calls=0)
    visited,edges=set(),set()
    def stop(status,inner=None):
        state['stop']=status
        if inner is not None:state['inner']=inner
        u.emu_stop()
    def hook(emu,address,size,user):
        cs,ip=emu.reg_read(UC_X86_REG_CS),emu.reg_read(UC_X86_REG_IP)
        if state['root_return']:return stop(OK)
        if state['returning']:
            expected,is_far=state['returning'];state['returning']=None
            if is_far:state['inner']=0
            if (cs,ip)!=expected:return stop(TRANSFER if is_far else CALLEE,None if is_far else fm.TRANSFER)
        if cs in systems and ip==fm.RUNERROR_IP:return stop(CALLEE,fm.RUNERROR)
        who='caller' if cs==CS else 'freemem'
        table=decoded if who=='caller' else leaf
        cond=conditional if who=='caller' else leaf_edges
        if (cs!=CS and cs not in systems) or ip not in table:
            state['unexpected']=(cs,ip);return emu.emu_stop()
        here=(who,ip);previous=state['previous']
        if previous is not None:
            pwho,pip=previous;pc=conditional if pwho=='caller' else leaf_edges
            if pip in pc:edges.add((pwho,pip,ip))
        if who=='freemem' and ip==fm.WALK_HEAD and state['last'] in {('freemem',a) for a in fm.WALK_JAS}:
            if state['heap_loops']==heap_cap:return stop(CALLEE,fm.LIMIT)
            state['heap_loops']+=1
        if who=='caller' and ip in SITES:
            state['site']=ip
            state['heap_loops']=0;state['release_calls']+=1
        visited.add(here);ins=table[ip]
        if ins.mnemonic in ('call','lcall'):state['stack'].append(((cs,ip+ins.size),ins.mnemonic=='lcall'))
        elif ins.mnemonic in ('ret','retf'):
            if who=='caller' and ip==RETF:state['root_return']=True
            else:state['returning']=state['stack'].pop()
        state['last']=here;state['previous']=here
    protected=[(physical(CS,ENTRY),physical(CS,END))]
    protected += [(physical(s,a),physical(s,b)) for s in systems for a,b in fm.SPANS]
    def mirror(emu,access,address,size,value,user):
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1)
            check(not any(a<=at<b for a,b in protected),f'immutable code written: {label} {at:x}')
            emu.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror)
    u.emu_start(physical(CS,ENTRY),2*SIZE,count=2000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original escaped/stalled {label}: {state}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=Diagnostic()
    status=f(ram,SIZE,ctypes.byref(c),next(iter(systems)),heap_cap,ctypes.byref(d))
    check(status==state['stop'],f'status {label}: {status} != {state["stop"]}')
    check((d.freemem_status,d.call_site)==(state['inner'],state['site']),f'diagnostic {label}: {(d.freemem_status,d.call_site)} != {(state["inner"],state["site"])}')
    check(d.calls==state['release_calls'],f'call count {label}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        want=u.reg_read(reg)&65535;actual=getattr(c,name)
        check(actual==want,f'{label} {name}: C {actual:04X}, original {want:04X}')
    actual=bytes(ram);want=bytes(u.mem_read(0,SIZE))
    if actual!=want:
        diff=[i for i,(x,y) in enumerate(zip(actual,want)) if x!=y]
        raise ValueError(f'{label}: {len(diff)} RAM differences: {[(hex(p),actual[p],want[p]) for p in diff[:8]]}')
    name='CALLEE_'+fm.NAMES[d.freemem_status] if status==CALLEE else NAMES[status]
    return name,visited,edges,state['release_calls']
def differential(body,image,decoded,conditional,leaf,leaf_edges,f):
    visited,edges,stops=set(),set(),{};total_calls=0;rng=random.Random(0x6C)
    for label,options in cases():
        options=dict(options);hc=options.pop('cap',64)
        m,c=build(body,image,rng,**options)
        name,v,e,calls_made=compare(label,m,c,decoded,conditional,leaf,leaf_edges,f,hc)
        visited|=v;edges|=e;stops[name]=stops.get(name,0)+1;total_calls+=calls_made
        if not label.startswith('random-'):print(label,name,flush=True)
    wanted={('caller',a) for a in decoded}|{('freemem',a) for a in leaf}
    want_edges={(who,a,b) for who,cond in (('caller',conditional),('freemem',leaf_edges)) for a,ends in cond.items() for b in ends}
    missing=sorted(wanted-visited);missing_edges=sorted(want_edges-edges)
    check(not missing and not missing_edges,f'closure coverage incomplete: {missing}, {missing_edges}')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,masked_bits=0,
        caller_starts=sum(w=='caller' for w,a in visited),caller_total=len(decoded),
        caller_outcomes=sum(w=='caller' for w,a,b in edges),caller_outcomes_total=2*len(conditional),
        chain_starts=len(visited),chain_total=len(wanted),chain_outcomes=len(edges),chain_outcomes_total=len(want_edges),
        missing_starts=missing,missing_outcomes=missing_edges,freemem_calls=total_calls)
    print('Original parity',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='res0464-release-') as tmp:
        f=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        body,image,decoded,conditional,leaf,leaf_edges,meta=source();report=dict(target=meta)
        if a.unicorn:report['unicorn']=differential(body,image,decoded,conditional,leaf,leaf_edges,f)
        if a.native:
            from res0464_bitmap_release_2595_native import native_audit
            report['native']=native_audit(body,image,f)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    import sys
    sys.modules['res0464_bitmap_release_2595_audit']=sys.modules[__name__]
    main()
