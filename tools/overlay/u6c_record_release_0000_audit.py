#!/usr/bin/env python3
"""U6C +0000 original-byte CPU14/full1MiB differential and native replay.
Private media/artifacts stay in resources/. FreeMem harness idioms are reused
from system_freemem_0254_{audit,native}.py; no original code bytes are embedded.
"""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
import system_freemem_0254_audit as fm
check=fm.check
CS,DS,SS,SYSTEM=0x5000,0x1800,0x2800,0x0AF8
ENTRY,END,RETF=0,0xE2,0xDF
SITES=(0x2F,0x76,0xBE,0xCF)
OK,CODE,CALLEE,TRANSFER,LIMIT=0,2,3,4,5
NAMES={OK:'return',CODE:'CODE',CALLEE:'CALLEE',TRANSFER:'TRANSFER',LIMIT:'LIMIT'}
SOURCES=['overlay_u6c_record_release_0000_cpu.c','system_freemem_0254_cpu.c']
FLAGS=fm.FLAGS
class Limits(ctypes.Structure):
    _fields_=[('max_list_loopbacks',ctypes.c_uint),('max_heap_loopbacks',ctypes.c_uint)]
class Diagnostic(ctypes.Structure):
    _fields_=[('freemem_status',ctypes.c_int),('call_site',ctypes.c_uint16)]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media provenance')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x6C)
    body=ovr[u['ovr_offset']:u['ovr_offset']+END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(body,0))
    check(u['ovr_offset']==104831 and 0 not in u['entries'] and min(u['entries'])==0x161,'identity/upper bound')
    check(sha256(body).hexdigest()=='89b9da1045a0ae35910cc989734674536cd458aa8f7d74389f5460635f9e6983','body hash')
    check(len(ins)==73 and sum(i.size for i in ins)==END and (ins[-1].address,ins[-1].mnemonic,ins[-1].op_str)==(RETF,'retf','4'),'body extent')
    check([(i.address,i.op_str) for i in ins if 'call' in i.mnemonic]==[(a,'0xaf8, 0x254') for a in SITES] and all(a+3 in u['fixups'] for a in SITES),'call bindings')
    # Two internal push-CS/near-CALL sites, not an exported entry.
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    for site in (0x334A,0x33B2):
        check(code[site-1]==0x0E and code[site]==0xE8 and (site+3+struct.unpack_from('<h',code,site+1)[0])&65535==0,'internal caller')
    image,leaf,leaf_edges,digests=fm.source()
    decoded={i.address:i for i in ins}
    conditional={i.address:(i.address+i.size,int(i.op_str,16)) for i in ins if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    meta=dict(target='OVL_006C_0000',bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),file_offset=u['ovr_offset'],queue_upper_bound=353,excluded_trailing_bytes=127,entry_slots=[],internal_call_sites=[0x334A,0x33B2],freemem_spans=digests)
    print('Target',meta,flush=True)
    return body,image,decoded,conditional,leaf,leaf_edges,meta
def library(directory):
    p=directory/'u6c.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u6c_record_release_0000_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    f.restype=ctypes.c_int
    return f
def pair(m,s,o,p):
    put16(m,s,o&65535,p[1]);put16(m,s,(o+2)&65535,p[0])
def build(body,image,rng,*,record=(0x3100,0),side=None,list63=(),list9=(),blocks=(),top=(0x3800,0),org=(0x3000,0),sp=0x8000,ss=SS,flags=0x602,writes=(),background=None,systems=None,first=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    m[:len(image)]=image
    fm.gm.heap(m,blocks=blocks,top=top,end=(0x4000,0),first=first)
    pair(m,DS,0x4400,org)
    for off in (0x108,0xC8,0x7F):pair(m,record[0],record[1]+off,(0,0))
    if side is not None:pair(m,record[0],record[1]+0x108,side)
    for nodes,field,link in ((list63,0xC8,0x2A),(list9,0x7F,5)):
        if nodes:pair(m,record[0],record[1]+field,nodes[0])
        for p,nxt in zip(nodes,list(nodes[1:])+[(0,0)]):pair(m,p[0],p[1]+link,nxt)
    for j,v in enumerate((0x7000,0x9001,record[1],record[0])):put16(m,ss,(sp+2*j)&65535,v)
    at=physical(CS,0);m[at:at+len(body)]=body
    if systems:
        for site,seg in systems.items():
            put16(m,CS,site+3,seg)
            for a,b in fm.SPANS:
                at=physical(seg,a);m[at:at+b-a]=image[physical(SYSTEM,a):physical(SYSTEM,b)]
    for seg,off,blob in writes:
        at=physical(seg,off)
        for j,v in enumerate(blob):m[(at+j)&(SIZE-1)]=v
    c=Cpu(*[rng.randrange(65536) for _ in range(7)],sp,CS,0,DS,rng.randrange(65536),ss,flags)
    return m,c
def cases():
    a,b=(0x3300,0),(0x3500,0)
    result=[('empty',{}),('side',dict(side=(0x3200,0))),
        ('list63',dict(list63=(a,))),('list9',dict(list9=(b,))),
        ('all',dict(side=(0x3200,8),list63=(a,(0x3304,0)),list9=(b,(0x3501,0)))) ,
        ('merge-root-prev',dict(blocks=((0x30FE,0,2,0),))),
        ('merge-root-next',dict(blocks=((0x3112,0,3,0),))),
        ('merge-root-both',dict(blocks=((0x30FE,0,2,0),(0x3112,0,3,0)))),
        ('root-shrink-top',dict(top=(0x3112,0))),
        ('all-shrink-top',dict(record=(0x3100,0),side=(0x3112,0),list63=((0x3113,8),),list9=((0x3117,8),),top=(0x3118,8))),
        ('record-offset8',dict(record=(0x3100,8))),
        ('side-invalid',dict(side=(0x3200,1))),
        ('63-invalid',dict(list63=((0x3300,1),))),
        ('9-invalid',dict(list9=((0x3500,1),))),
        ('record-invalid',dict(record=(0x3100,1))),
        ('record-nil',dict(record=(0,0))),
        ('record-below-org',dict(record=(0x2FFF,0))),
        ('record-at-top',dict(record=(0x3800,0))),
        ('record-above-top',dict(record=(0x3801,0))),
        ('record-below-org-offset',dict(record=(0x3000,0),org=(0x3000,8))),
        ('record-at-org',dict(record=(0x3000,0))),
        ('record-same-top-seg',dict(record=(0x3100,0),top=(0x3100,8))),
        ('side-duplicate',dict(side=(0x3200,0),blocks=((0x3200,0,2,0),))),
        ('record-duplicate',dict(blocks=((0x3100,0,0x12,0),))),
        ('same-list-node-twice',dict(list63=(a,),list9=(a,))),
        ('cycle63',dict(list63=(a,),writes=((a[0],0x2A,struct.pack('<HH',0,a[0])),))),
        ('cycle9',dict(list9=(b,),writes=((b[0],5,struct.pack('<HH',0,b[0])),))),
        ('limit63',dict(list63=(a,),list_cap=0)),
        ('limit9',dict(list9=(b,),list_cap=0)),
        ('limit-shared',dict(list63=(a,),list9=(b,),list_cap=1)),
        ('heap-limit',dict(blocks=((0x3001,0,1,0),),heap_cap=0)),
        ('heap-cycle',dict(blocks=((0x3001,0,1,0),),writes=((0x3001,0,struct.pack('<HH',0,0x3001)),),heap_cap=2)),
        ('side-alias-final-argument',dict(ss=0x3000,sp=0x2000,side=(0x3200,0))),
        ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
        ('two-system-segments',dict(side=(0x3200,0),list63=(a,),list9=(b,),systems={0x2F:0x6000,0x76:0x6100,0xBE:SYSTEM,0xCF:0x6000})),
    ]
    for site,opts in zip(SITES,(dict(side=(0x3200,0)),dict(list63=(a,)),dict(list9=(b,)),{})):
        for what,off,blob in (('opcode',site,b'\x90'),('offset',site+1,b'\x55\x02')):
            result.append((f'bad-{site:x}-{what}',dict(opts,writes=((CS,off,blob),))))
    result.append(('unreached-bad-sites',dict(writes=tuple((CS,s,b'\x90') for s in SITES[:-1]))))
    for sp in (0x40,0x41,0x8001,0xFFF7,0xFFF8):result.append((f'sp-{sp:x}',dict(sp=sp,side=(0x3200,0),list63=(a,),list9=(b,))))
    result += [
        ('same-seg-free-lower',dict(record=(0x3100,8),blocks=((0x3100,0,0,8),))),
        ('next-same-top-seg',dict(blocks=((0x3112,0,0,8),),top=(0x3112,8))),
        ('next-offset-gap',dict(blocks=((0x3112,8,2,0),))),
    ]
    for sp in (0x0FF0,0x1000,0x1002,0x1004,0x1008,0x1010,0x1018,0x1020,0x1028,0x1030,0x1040):
        result.append((f'heap-stack-{sp:x}',dict(ss=0x3000,sp=sp)))
    rng=random.Random(0x6C0000)
    for n in range(360):
        n63,n9=n%7,(n//7)%7
        offset=rng.choice((0,8))
        result.append((f'random-{n}',dict(record=(0x3100,offset),side=(0x3200,rng.choice((0,8))) if n%2 else None,
            list63=tuple((0x3300+j*8,rng.choice((0,8))) for j in range(n63)),
            list9=tuple((0x3500+j*4,rng.choice((0,8))) for j in range(n9)),
            blocks=tuple((0x3001+j*8,0,1,0) for j in range(n%5)),flags=FLAGS[n%len(FLAGS)])))
    result.append(('random-ram',dict(background=rng.randbytes(SIZE),side=(0x3200,0),list63=(a,),list9=(b,))))
    return result
def compare(label,m,c,decoded,conditional,leaf,leaf_edges,f,list_cap=64,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),r in zip(Cpu._fields_,REGISTERS):u.reg_write(r,getattr(c,name))
    systems={struct.unpack_from('<H',m,physical(CS,s+3))[0] for s in SITES}
    state=dict(stop=None,returning=None,root_return=False,stack=[],previous=None,last=None,list_loops=0,heap_loops=0,
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
        if who=='caller' and ((ip==0x47 and state['last']==('caller',0x7B)) or (ip==0x8F and state['last']==('caller',0xC3))):
            if state['list_loops']==list_cap:return stop(LIMIT)
            state['list_loops']+=1
        if who=='freemem' and ip==fm.WALK_HEAD and state['last'] in {('freemem',a) for a in fm.WALK_JAS}:
            if state['heap_loops']==heap_cap:return stop(CALLEE,fm.LIMIT)
            state['heap_loops']+=1
        if who=='caller' and ip in SITES:
            state['site']=ip
            if bytes(emu.mem_read(physical(cs,ip),3))!=b'\x9a\x54\x02':return stop(CODE)
            state['heap_loops']=0;state['release_calls']+=1
        visited.add(here);ins=table[ip]
        if ins.mnemonic in ('call','lcall'):state['stack'].append(((cs,ip+ins.size),ins.mnemonic=='lcall'))
        elif ins.mnemonic in ('ret','retf'):
            if who=='caller' and ip==RETF:state['root_return']=True
            else:state['returning']=state['stack'].pop()
        state['last']=here;state['previous']=here
    protected=[(physical(CS,0),physical(CS,END))]
    protected += [(physical(s,a),physical(s,b)) for s in systems for a,b in fm.SPANS]
    def mirror(emu,access,address,size,value,user):
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1)
            check(not any(a<=at<b for a,b in protected),f'immutable code written: {label} {at:x}')
            emu.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror)
    u.emu_start(physical(CS,0),2*SIZE,count=2000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original escaped/stalled {label}: {state}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);limits=Limits(list_cap,heap_cap);d=Diagnostic()
    status=f(ram,SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    check(status==state['stop'],f'status {label}: {status} != {state["stop"]}')
    check((d.freemem_status,d.call_site)==(state['inner'],state['site']),f'diagnostic {label}: {(d.freemem_status,d.call_site)} != {(state["inner"],state["site"])}')
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
        options=dict(options);lc=options.pop('list_cap',64);hc=options.pop('heap_cap',64)
        m,c=build(body,image,rng,**options)
        name,v,e,calls_made=compare(label,m,c,decoded,conditional,leaf,leaf_edges,f,lc,hc)
        visited|=v;edges|=e;stops[name]=stops.get(name,0)+1;total_calls+=calls_made
        if not label.startswith('random-'):print(label,name,flush=True)
    wanted={('caller',a) for a in decoded}|{('freemem',a) for a in leaf}
    want_edges={(who,a,b) for who,cond in (('caller',conditional),('freemem',leaf_edges)) for a,ends in cond.items() for b in ends}
    missing=sorted(wanted-visited);missing_edges=sorted(want_edges-edges)
    check(not [x for x in missing if x[0]=='caller'] and not [x for x in missing_edges if x[0]=='caller'],'caller coverage incomplete')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,masked_bits=0,
        caller_starts=sum(w=='caller' for w,a in visited),caller_total=len(decoded),
        caller_outcomes=sum(w=='caller' for w,a,b in edges),caller_outcomes_total=2*len(conditional),
        chain_starts=len(visited),chain_total=len(wanted),chain_outcomes=len(edges),chain_outcomes_total=len(want_edges),
        missing_starts=missing,missing_outcomes=missing_edges,freemem_calls=total_calls)
    print('Original parity',report,flush=True);return report
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    body,image,decoded,conditional,leaf,leaf_edges,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='u6c-release-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(body,image,decoded,conditional,leaf,leaf_edges,f)
        if a.native:
            from u6c_record_release_0000_native import native_audit
            report['native']=native_audit(body,image,f)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    import sys
    sys.modules['u6c_record_release_0000_audit']=sys.modules[__name__]
    main()
