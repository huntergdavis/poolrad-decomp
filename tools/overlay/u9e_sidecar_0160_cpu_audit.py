#!/usr/bin/env python3
"""U9E +0160 raw partial-chain parity; original media/captures stay private."""
from loaded_stub_fixtures import install_stub, loaded_sites
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from overlay_inventory import find_units,parse_mz
import u5e_caller_32e6_audit as base
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
CS,DS,SS,NODES=0x8800,0x5000,0x2000,0x7000
ENTRY,END=0x160,0x2FA
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
BODY_SHA='89a45482f4f42e1f4805cc4aa2448ebb685b777dbc7b91be985da4473bb208ba'
SET_SHA='251cddcd1fef74b67e089e40cf1dbed79db94489e43a5c00d273c5f1713d7c3b'
DISPATCH_SHA='1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217'
CALLS={0x18B:(0,'res',0x2199),0x1A5:(1,'sys',0xDC0),0x1BE:(2,'sys',0x1685),
 0x1EC:(0,'res',0x2199),0x21C:(3,'slots',0xBE7),0x22A:(3,'slots',0xC0F),
 0x242:(3,'slots',0xC37),0x250:(4,'chain',0x909),0x274:(3,'slots',0xC5F),0x2CF:(2,'sys',0x1685)}
class Layout(ctypes.Structure):
    _fields_=[(n,ctypes.c_uint16) for n in ('slots_cs','chain_cs','system_cs','resident_cs')]
class Limits(ctypes.Structure):
    _fields_=[(n,ctypes.c_size_t) for n in ('party_visits','result_visits','slot_visits','chain_iterations','line_iterations')]+[(n,ctypes.c_uint) for n in ('lookup_loopbacks','sort_loopbacks')]
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('status',ctypes.c_int),('handler_offset',ctypes.c_uint16),('handler_segment',ctypes.c_uint16)]
DEFAULT=(64,512,256,20000,512,64,65536)
SOURCES=('overlay_u9e_sidecar_0160_cpu','overlay_u3c6_slot_getters_cpu','resident_00ba_sidecar_lookup_2199_cpu','system_string_move_leaves_cpu')+base.prior.SOURCES
def check(v,message):
    if not v:raise ValueError(message)
def source():
    old,_=base.source();bodies={k:v for k,v in old.items() if k[0]!='caller'}
    e=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();o=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(e).hexdigest()==EXE_SHA and sha256(o).hexdigest()==OVR_SHA,'media pins')
    units={u['stub_para']:u for u in find_units(e[parse_mz(e)['header_bytes']:],o)};u=units[0x9E]
    code=o[u['ovr_offset']:u['ovr_offset']+u['code_size']];image=bytes(unpack(e)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    for a,b,n,digest in ((ENTRY,END,140,BODY_SHA),(0,0x26,16,DISPATCH_SHA)):
        body=code[a:b];ins=list(md.disasm(body,a));check(sha256(body).hexdigest()==digest and len(ins)==n and sum(i.size for i in ins)==b-a,'caller identity')
        bodies['caller',a]=body
    check(sha256(code[0x140:0x160]).hexdigest()==SET_SHA,'set identity');bodies['data',0x140]=code[0x140:0x160]
    check([f for f in u['fixups'] if ENTRY<=f<END]==[a+3 for a in CALLS],'caller fixups')
    check(units[0x3C6]['entries'][12:16]==[0xBE7,0xC0F,0xC37,0xC5F] and units[0x3C2]['entries'][6]==0x909,'stub targets')
    for kind,seg,a,b,n,digest in (('res',0xBA,0x2199,0x2208,37,'721e86627eac3e72deed41d2a26283e7c95444d99fc4f762a9714b11504dfa9c'),
        ('sys',0xAF8,0xDC0,0xDE1,13,'753159dded0f3f45bb0352cb9483a15271d85888b6c1b127f8631879473ec62c'),
        ('sys',0xAF8,0x1685,0x16A8,16,'1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972')):
        body=image[physical(seg,a):physical(seg,b)]
        check(len(list(md.disasm(body,a)))==n and (digest is None or sha256(body).hexdigest()==digest),'leaf identity')
        bodies[kind,a]=body
    decoded={(k,i.address):i for (k,a),blob in bodies.items() if k!='data' for i in md.disasm(blob,a)}
    original_calls=[(ip,i.op_str) for (k,ip),i in decoded.items() if k=='caller' and i.mnemonic=='lcall' and ip!=0x1E]
    check(original_calls==[(0x18B,'0xba, 0x2199'),(0x1A5,'0xaf8, 0xdc0'),(0x1BE,'0xaf8, 0x1685'),(0x1EC,'0xba, 0x2199'),(0x21C,'0x3e0, 0x5c'),(0x22A,'0x3e0, 0x61'),(0x242,'0x3e0, 0x66'),(0x250,'0x3db, 0x3e'),(0x274,'0x3e0, 0x6b'),(0x2CF,'0xaf8, 0x1685')],'original call graph')
    meta=dict(bytes=410,instructions=140,sha256=BODY_SHA,file_start=u['ovr_offset']+ENTRY,file_end=u['ovr_offset']+END,
        set_sha256=SET_SHA,dispatcher_sha256=DISPATCH_SHA,dependencies={f'{k}:{a:04X}':dict(bytes=len(v),sha256=sha256(v).hexdigest()) for (k,a),v in bodies.items()})
    return bodies,decoded,meta
def library(directory):
    path=directory/'0160.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u9e_sidecar_0160_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Layout),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f
def segments(c,l):return dict(caller=c.cs,data=c.cs,slots=l.slots_cs,chain=l.chain_cs,sys=l.system_cs,res=l.resident_cs)
def call_target(kind,ip,ins):
    if ins.mnemonic=='call':return kind,int(ins.op_str,16)
    if kind=='caller':return CALLS[ip][1:]
    return base.call_target(kind,ip,ins)
def install(m,c,l,bodies,decoded):
    segs=segments(c,l)
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        for (k,ip),ins in decoded.items():
            if k==kind and start<=ip<start+len(data) and ins.mnemonic=='lcall' and ip!=0x1E:
                target,off=call_target(k,ip,ins)
                if k=='chain' and target=='slots':
                    install_stub(m,blob,start,ins,segs[target],off)
                else:
                    blob[ip-start+1:ip-start+5]=struct.pack('<HH',off,segs[target])
        for j,v in enumerate(blob):m[physical(segs[kind],start+j)]=v
def build(bodies,decoded,*,key=0x15,direct=(),members=((0x15,),),selected=2,combat=5,party=((10,10,1),(12,12,1)),
          party_cycle=False,lookup_cycle=False,map_alias=False,ss=SS,sp=0x8000,flags=0x602,seed=0,writes=(),**options):
    m,c,_=base.fixture(party=party,selected=selected,cs=CS,ds=DS,ss=ss,sp=sp,flags=flags,seed=seed,**options)
    l=Layout(0x3000,0x4000,0x6000,0x9800);c.ip=ENTRY
    for i in range(256):
        put16(m,DS,0x65B9+4*i,(0x1000+0x200*i)&65535);put16(m,DS,0x65BB+4*i,NODES)
        put16(m,DS,0x6828+4*i,0x1000+i);put16(m,DS,0x682A+4*i,0xA000+i)
    # The 256-entry pointer initializer overlaps DS:6716/6718. Restore the
    # intended map pointer AFTER it, except in the explicit alias case.
    if not map_alias:
        put16(m,DS,0x6716,options.get('map_offset',0x8000));put16(m,DS,0x6718,DS)
    # Character frames are disjoint; slot pointers compare as far identities.
    all_records=[(i+1,keys) for i,keys in enumerate(members)]+[(selected,direct)]
    for j,(index,keys) in enumerate(all_records):
        off=(0x1000+0x200*index)&65535;node=0x8000+j*0x400
        put16(m,NODES,off+0x7F,node if keys else 0);put16(m,NODES,off+0x81,NODES if keys else 0)
        for k,value in enumerate(keys):
            at=node+16*k;more=k+1<len(keys) or lookup_cycle
            m[physical(NODES,at)]=value;put16(m,NODES,at+5,node+16*((k+1)%len(keys)) if more else 0);put16(m,NODES,at+7,NODES if more else 0)
    for j in range(len(members)):
        off=0x1200+0x200*j;more=j+1<len(members) or party_cycle
        put16(m,NODES,off+0x104,0x1200+0x200*((j+1)%len(members)) if more else 0);put16(m,NODES,off+0x106,NODES if more else 0)
    put16(m,DS,0x5D96,0x1200 if members else 0);put16(m,DS,0x5D98,NODES if members else 0);m[physical(DS,0x49F3)]=combat
    for j,v in enumerate((0x7000,0x9001,0xA500|key,(0x1000+0x200*selected)&65535,NODES)):put16(m,ss,(sp+2*j)&65535,v)
    install(m,c,l,bodies,decoded)
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    return m,c,l
def compare(label,state,bodies,decoded,f,budgets=DEFAULT):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
    from unicorn import x86_const as R
    m,c,l=state;segs=segments(c,l);del segs['data'];backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    stubs=loaded_sites(m,segs,decoded)
    visited,edges=set(),set();calls=[];remaining=list(budgets)
    s=dict(stop=None,previous=None,last=None,stepping=False,callee=-1,nested=-1)
    def halt(status,nested=None):
        s['stop']=status
        if nested is not None:s['nested']=nested
        uc.emu_stop()
    def hook(em,address,size,user):
        del em,size,user
        if s['stepping']:return
        cs,ip=uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)
        if (cs,ip) in stubs:return
        kind=next((k for k,v in segs.items() if v==cs),None);key=kind,ip
        check(key in decoded,f'original escape {label}: {cs:04x}:{ip:04x}')
        if key==('caller',0x1E):return halt(5)
        if kind=='caller' and ip in CALLS:
            which,target,off=CALLS[ip]
            if bytes(uc.mem_read(address,5))!=struct.pack('<BHH',0x9A,off,segs[target]):return halt(2)
        if key==('caller',0x2F1) and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,0xFD0C):return halt(2)
        if key==('chain',0x909):
            sp=uc.reg_read(R.UC_X86_REG_SP)
            if not 0x8E<=sp<=0xFFEE:return halt(3,1)
        if kind=='slots' and ip in (0xBE7,0xC0F,0xC37,0xC5F) and s['last'] and s['last'][0]=='caller':
            if not 0x20<=uc.reg_read(R.UC_X86_REG_SP)<=0xFFF8:return halt(3,1)
        if key==('chain',0x422):
            sp=uc.reg_read(R.UC_X86_REG_SP);ss=uc.reg_read(R.UC_X86_REG_SS)
            if not 0x54<=sp<=0xFFE8 or any((physical(ss,sp-i)-physical(l.chain_cs,0))%SIZE<0x2E or 0x1A0<=(physical(ss,sp-i)-physical(l.chain_cs,0))%SIZE<0x57B for i in range(1,85)):return halt(3,5)
        index=0 if key==('caller',0x1C3) else 1 if key==('caller',0x26E) else 2 if key==('slots',0xC71) else 3 if kind=='chain' and ip in base.prior.LOOPS else 4 if key==('chain',0x4DD) else 5 if key==('res',0x21B9) and s['last']==('res',0x21F7) else 6 if kind=='chain' and ip in (0x72,0x56) and s['last'] in (('chain',0x18E),('chain',0x199)) else None
        if index is not None:
            if not remaining[index]:return halt(6 if index<2 else 3,{2:2,3:2,4:3,5:2,6:4}.get(index))
            remaining[index]-=1
        if s['previous'] is not None:edges.add((s['previous'],key))
        ins=decoded[key];visited.add(key);s['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None;s['last']=key
        if ins.mnemonic in ('call','lcall'):
            target=call_target(kind,ip,ins);calls.append(((cs,ip+ins.size),target,key))
            if kind=='caller' and ip in CALLS:
                s['callee']=CALLS[ip][0];s['nested']=0
                if s['callee']==0:remaining[5]=budgets[5]
                if s['callee']==3:remaining[2]=budgets[2]
                if s['callee']==4:remaining[3]=budgets[3]
            if target==('chain',0x422):remaining[4]=budgets[4]
            if target==('chain',0x2E):remaining[6]=budgets[6]
        if ins.mnemonic=='retf':halt('ret')
    uc.hook_add(UC_HOOK_CODE,hook)
    while True:
        s['stop']=None;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=2000000)
        check(s['stop'] is not None,f'original instruction ceiling {label}')
        if s['stop']!='ret':status=s['stop'];break
        s['stepping']=True;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1);s['stepping']=False
        if not calls:status=0;break
        expected,target,site=calls.pop()
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=expected:
            status=4 if site[0]=='caller' else 3
            if status==3:s['nested']=3 if site[0]=='slots' else 6 if site[0]=='chain' and 0x909<=site[1]<0xBD9 else 5
            break
    for (kind,start),blob in bodies.items():
        check(all(backing[physical((segs['caller'] if kind=='data' else segs[kind]),start+j)]==m[physical((segs['caller'] if kind=='data' else segs[kind]),start+j)] for j in range(len(blob))),f'code/set modified outside contract {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c);limits=Limits(*budgets);d=Diagnostic()
    got=f(ram,SIZE,ctypes.byref(actual),ctypes.byref(l),ctypes.byref(limits),ctypes.byref(d))
    check(got==status,f'status {label}: {got}/{status}, {d.callee}/{d.status}, {s}')
    check((d.callee,d.status)==(s['callee'],s['nested']),f'diagnostic {label}: {d.callee}/{d.status}, {s}')
    if bytes(ram)!=bytes(backing):raise ValueError(f'RAM {label}: '+str([(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12]))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535;value=getattr(actual,n)
        if n=='flags' and (status==5 or (status==2 and actual.ip in (0x22A,0x250))):check(value^want==0x10 and value&0x10,f'pending SHL AF {label}: {value:x}/{want:x}')
        else:check(value==want,f'CPU {label}/{n}: {value:x}/{want:x}')
    if status==5:
        at=physical(actual.ds,(actual.di+0x6828)&65535);want=bytes(backing[at:at+4])
        check(struct.pack('<HH',d.handler_offset,d.handler_segment)==want,f'pending target {label}')
    return visited,edges,status,actual,d
def cases(quick=False):
    result=[('default',{}),('direct',dict(direct=(0x15,))),('outside-set',dict(key=4)),('nil-party',dict(members=())),
        ('party-miss',dict(members=((1,2),))),('noncombat-hit',dict(combat=0)),('large-radius',dict(key=0x31,members=((0x31,),))),
        ('near-candidate',dict(party=((10,10,1),(11,10,1)))),('zero-candidates',dict(party=((10,10,0),(12,12,0)))),
        ('found-before-tail',dict(combat=0,members=((0x15,),()),selected=3,party=((10,10,1),(11,10,1),(12,10,1)))),
        ('second-member',dict(members=((),(0x15,)),selected=3,party=((10,10,1),(11,10,1),(12,10,1))))]
    if quick:return result
    for key in range(256):
        result += [(f'direct-type-{key}',dict(key=key,direct=(key,),flags=FLAGS[key%6])),
                   (f'missing-type-{key}',dict(key=key,members=(),flags=FLAGS[key%6]))]
    for key in (0x15,0x2D,0x2E,0x31):
        for combat in (0,5):
            for selected in (0,1,2,3):
                result.append((f'scan-{key}-{combat}-{selected}',dict(key=key,combat=combat,selected=selected,members=((key,),),party=((10,10,1),(11,10,1),(12,10,1)))))
    def cap(index,value):
        b=list(DEFAULT);b[index]=value;return tuple(b)
    result += [('party-limit',dict(budgets=cap(0,0))),('result-limit',dict(key=0x31,members=((0x31,),),budgets=cap(1,0))),
        ('lookup-limit',dict(direct=(0x15,),budgets=cap(5,0))),('lookup-cycle',dict(direct=(1,2),lookup_cycle=True,budgets=cap(5,3))),
        ('party-cycle',dict(members=((),),party_cycle=True,budgets=cap(0,3))),('slot-limit',dict(budgets=cap(2,0))),
        ('chain-limit',dict(budgets=cap(3,1))),('line-limit',dict(key=0x31,members=((0x31,),),budgets=cap(4,1))),
        ('sort-limit',dict(key=0x31,members=((0x31,),),party=tuple((10+i,10,1) for i in range(5)),budgets=cap(6,1))),
        ('map-global-slot-alias',dict(map_alias=True)),('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),('save-alias',dict(ss=DS,sp=0x6800)),
        ('pending-live-target',dict(direct=(0x15,),writes=((DS,0x6828+4*0x15,struct.pack('<HH',0xBEEF,0xCAFE)),)))]
    for sp in (0x186,0x187,0x8001,0xFFF5,0xFFF6):result.append((f'sp-{sp:x}',dict(sp=sp)))
    for ip in (*CALLS,0x2F1):result.append((f'code-{ip:x}',dict(key=0x31,members=((0x31,),),writes=((CS,ip,b'\x90'),))))
    rng=random.Random(0x9E160)
    for n in range(24):
        key=rng.choice((0x15,0x2D,0x2E,0x31));result.append((f'random-{n}',dict(key=key,direct=(key,) if n%5==0 else (),members=((key,),) if n%3 else ((1,2),),combat=rng.choice((0,1,5)),selected=rng.randrange(4),party=tuple((rng.randrange(9,14),rng.randrange(9,14),rng.randrange(3)) for _ in range(3)),flags=FLAGS[n%6],seed=n)))
    return result
def differential(bodies,decoded,f,quick=False):
    visited,edges=set(),set();stops={};diagnostics={};pending=0;binding_af=0
    specs=cases(quick)
    for n,(label,options) in enumerate(specs):
        options=dict(options);budgets=options.pop('budgets',DEFAULT)
        print('BEGIN',n+1,label,flush=True)
        v,e,status,c,d=compare(label,build(bodies,decoded,**options),bodies,decoded,f,budgets)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1
        if status==5:pending+=1
        if status==2 and c.ip in (0x22A,0x250):binding_af+=1
        if status==3:diagnostics[f'{d.callee}:{d.status}:{c.cs:04X}:{c.ip:04X}']=diagnostics.get(f'{d.callee}:{d.status}:{c.cs:04X}:{c.ip:04X}',0)+1
    caller={p for p in decoded if p[0]=='caller' and ENTRY<=p[1]<END}
    branch={p for p in caller if decoded[p].mnemonic.startswith('j') and decoded[p].mnemonic!='jmp'}
    ce={e for e in edges if e[0] in branch}
    if not quick:check(caller<=visited,f'missing caller starts {caller-visited}');check(len(ce)==2*len(branch),f'caller branch coverage {len(ce)}/{2*len(branch)}')
    return dict(cases=len(specs),stops=stops,callee_stops=diagnostics,cpu_words=14,ram_bytes=SIZE,
        caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(ce),caller_outcomes_total=2*len(branch),
        dispatcher_prefix_starts=len({p for p in visited if p[0]=='caller' and p[1]<0x1E}),
        chain_starts=len(visited),chain_outcomes=len(edges),unicorn_pending_af_exceptions=pending,unicorn_binding_af_exceptions=binding_af,
        uncovered_sites=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)])
def main():
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='u9e0160-') as tmp:
        f=library(Path(tmp))
        from loaded_stub_checks import verify_refusals
        memory,cpu,layout=build(bodies,decoded)
        limits=Limits(*DEFAULT);diagnostic=Diagnostic()
        report['loaded_stub_guards']=verify_refusals(memory,cpu,
            lambda ram,state: f(ram,SIZE,ctypes.byref(state),ctypes.byref(layout),ctypes.byref(limits),ctypes.byref(diagnostic)),
            [(layout.chain_cs,p) for p in (0x92A,0x9D8)])
        if a.unicorn:report['unicorn']=differential(bodies,decoded,f,a.quick)
        if a.native:
            from u9e_sidecar_0160_cpu_native import native
            report['native']=native(bodies,decoded,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True),flush=True)
if __name__=='__main__':main()
