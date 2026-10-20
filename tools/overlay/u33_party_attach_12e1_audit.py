#!/usr/bin/env python3
"""Original U33 +12E1 composition, unmasked CPU14 and complete MiB differential.

The caller's 91 starts/all conditional outcomes are required. Resident and
allocator coverage is reported separately; fixed size22, default HeapError and
runtime-tail boundaries are never silently credited as full closure coverage.
"""
import argparse,ctypes,json,random,subprocess,tempfile
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP,UC_X86_REG_SP,UC_X86_REG_SS,UC_X86_REG_EFLAGS
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
from u9e_character_pair_115c_audit import PINS as PRIOR_PINS,INFEASIBLE as RES_INFEASIBLE,fixture as resident_fixture,cases as resident_cases
CHECKER_ROWS=('OVL_0033_12E1',)
ENTRY,END=0x12e1,0x1403
BODY_SHA='e463fe4b8a2db622ea31bec99ecf067a6a18483547fa251d8c9379a6aea35312'
SOURCES=('overlay_u33_party_attach_12e1_cpu','resident_00ba_character_0bb8_cpu','resident_00ba_character_helpers_cpu','system_getmem_023f_cpu','system_runtime_leaves_cpu')
SYS_SPANS=((0x23f,0x254),(0x308,0x3d3),(0x491,0x4a2),(0xa9,0xae),(0x16a8,0x16bc))
MASK=SIZE-1


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(exe)==65231 and sha256(exe).hexdigest()==EXE_SHA
    assert len(ovr)==206105 and sha256(ovr).hexdigest()==OVR_SHA
    unit=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x33)
    assert [f for f in unit['fixups'] if ENTRY<=f<END]==[0x130c,0x1318,0x133b]
    image=bytes(unpack(exe)[0]);bodies={};decoded={};pins=[]
    blob=ovr[unit['ovr_offset']+ENTRY:unit['ovr_offset']+END]
    assert sha256(blob).hexdigest()==BODY_SHA;bodies['u33',ENTRY]=blob
    for kind,start,end,count,digest in PRIOR_PINS:
        if kind!='res':continue
        blob=image[physical(0xba,start):physical(0xba,end)]
        assert sha256(blob).hexdigest()==digest;bodies[kind,start]=blob
    for start,end in SYS_SPANS:bodies['sys',start]=image[physical(0xaf8,start):physical(0xaf8,end)]
    for (kind,start),blob in bodies.items():
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(blob,start));assert sum(i.size for i in ins)==len(blob)
        decoded.update(((kind,i.address),i) for i in ins)
        pins.append(dict(kind=kind,start=f'{start:04X}',end=f'{start+len(blob):04X}',bytes=len(blob),instructions=len(ins),sha256=sha256(blob).hexdigest()))
    assert len([k for k in decoded if k[0]=='u33'])==91
    assert [(i.address,i.op_str) for (k,_),i in decoded.items() if k=='u33' and i.mnemonic=='lcall']==[(0x1309,'0xba, 0xbb8'),(0x1315,'0xaf8, 0x23f'),(0x1338,'0xaf8, 0x16a8')]
    return bodies,decoded,pins


def build_model(directory):
    path=Path(directory)/'party.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u33_party_attach_12e1_cpu.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_uint16,ctypes.POINTER(ctypes.c_size_t),ctypes.c_uint,ctypes.POINTER(ctypes.c_int)]
    lib.pr_u33_party_attach_12e1_cpu.restype=ctypes.c_int
    return lib


def install(memory,cpu,resident,system,bodies):
    for (kind,start),blob in bodies.items():
        segment={'u33':cpu.cs,'res':resident,'sys':system}[kind];at=physical(segment,start);memory[at:at+len(blob)]=blob
    for off,segment in ((0x130c,resident),(0x1318,system),(0x133b,system)):put16(memory,cpu.cs,off,segment)


def fixture(*,party=({},),threshold=0,window_value=7,index=0,table_value=3,heap_mode='top',
            cs=0x6000,resident_cs=0x6400,system_cs=0x6200,ds=0x1000,ss=0x2000,sp=0x8000,
            record_seg=0x3000,extra_seg=0x4000,heap_seg=0x5000,flags=2,custom_heap=False,cycle=False,background=0xa5,resident_options=None,alias_stack_data=False):
    if alias_stack_data:ds=ss
    memory=bytearray([background])*SIZE;cpu=Cpu(0xabcd,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x7777,sp,cs,ENTRY,ds,0x1111,ss,flags)
    def byte(seg,off,val):memory[physical(seg,off&65535)]=val&255
    def word(seg,off,val):put16(memory,seg,off&65535,val&65535)
    def ptr(i):return (record_seg if i<160 else extra_seg,0x100+(i%160)*0x180)
    assert len(party)<=320,'fixture reserves two pages of160 members each'
    starts=sorted(physical(*ptr(i)) for i in range(len(party)))
    assert all(b-a>=0x140 for a,b in zip(starts,starts[1:])),'synthetic member records overlap'
    for i in range(0x1020):byte(ds,0x5580+i,0)
    template=None
    if resident_options is not None:
        template=resident_fixture(ds=ds,record_seg=record_seg,node_seg=heap_seg,**resident_options)[0]
        for segment,offset,length in ((ds,0x5580,0x1020),(heap_seg,0x2000,0x2000)):
            at=physical(segment,offset);memory[at:at+length]=template[at:at+length]
    for i,p in enumerate(party):
        seg,off=ptr(i)
        for j in range(0x140):byte(seg,off+j,template[physical(record_seg,0x100+j)] if template is not None else 0)
        fields={} if template is not None else {0x10:18,0x13:12,0x16:50,0x2d:10,0x2e:2,0x72:6}
        fields.update({0x84:p.get('tag',0),0x10e:p.get('mode',0),**p.get('fields',{})})
        for field,value in fields.items():byte(seg,off+field,value)
        nxt=ptr(i+1) if i+1<len(party) else ptr(0) if cycle else (0,0)
        word(seg,off+0x104,nxt[1]);word(seg,off+0x106,nxt[0])
    head=ptr(0) if party else (0,0);word(ds,0x5d96,head[1]);word(ds,0x5d98,head[0])
    word(ds,0x49d6,0x9000);word(ds,0x49d8,ds);word(ds,0x967c,threshold);word(ds,0x958c,window_value)
    byte(ds,0x6aaf,index)
    for i in range(128):byte(ds,0x2f2+i,table_value if not isinstance(table_value,list) else table_value[i%len(table_value)])
    top=(heap_seg+0x300,0);end=(heap_seg+0xff0,0);blocks=[]
    if heap_mode=='exact':blocks=[(heap_seg+0x10,0,1,8)]
    elif heap_mode=='split':blocks=[(heap_seg+0x10,0,8,0)]
    elif heap_mode=='split-carry':blocks=[(heap_seg+0x10,8,8,0)]
    elif heap_mode=='skip':blocks=[(heap_seg+0x10,0,0,8),(heap_seg+0x20,0,1,0),(heap_seg+0x30,0,1,8)]
    elif heap_mode=='fail':end=top
    elif heap_mode=='cyclic':blocks=[(heap_seg+0x10,0,0,8)]
    elif heap_mode=='top-exact':end=(top[0]+1,8)
    elif heap_mode=='top-granule-fail':end=(top[0]+1,0)
    elif heap_mode!='top':raise ValueError(heap_mode)
    for off,value in ((0x4400,0),(0x4402,heap_seg),(0x4404,top[1]),(0x4406,top[0]),(0x4408,end[1]),(0x440a,end[0]),(0x4414,0x1234 if custom_heap else 0xa9),(0x4416,system_cs)):
        word(ds,off,value)
    chain=blocks+[(top[0],top[1],0,0)];word(ds,0x440c,chain[0][1]);word(ds,0x440e,chain[0][0])
    for i,((seg,off,paragraphs,granule),nxt) in enumerate(zip(chain,chain[1:])):
        if heap_mode=='cyclic':nxt=chain[0]
        word(seg,off,nxt[1]);word(seg,off+2,nxt[0]);word(seg,off+4,granule);word(seg,off+6,paragraphs)
    word(ss,sp,0x9000);word(ss,sp+2,0x8000)
    return memory,cpu,resident_cs,system_cs


def run_c(lib,state,budget=1000000,cap=64):
    memory,cpu,resident,system=state;ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);out=Cpu.from_buffer_copy(cpu);left=ctypes.c_size_t(budget);diag=ctypes.c_int(-999)
    status=lib.pr_u33_party_attach_12e1_cpu(ram,SIZE,ctypes.byref(out),resident,system,ctypes.byref(left),cap,ctypes.byref(diag))
    return status,bytes(ram),out,left.value,diag.value


def original(state,decoded,budget=1000000,cap=64,stop_at=None):
    memory,cpu,resident,system=state;backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(cpu,name))
    seen=set();edges=set();previous=[None];last=[None];left=[budget];calls=[];pending=[None];stop=[None];diag=[-1];walk=[0]
    def word(seg,off):
        at=physical(seg,off&65535);return backing[at]|(backing[(at+1)&MASK]<<8)
    def hook(uc,address,size,user):
        cs=uc.reg_read(UC_X86_REG_CS);ip=uc.reg_read(UC_X86_REG_IP)
        if pending[0] is not None:
            target,expected,was_getmem=pending[0];pending[0]=None
            assert (cs,ip)==target,('original return differs',target,(cs,ip))
            if was_getmem:diag[0]=0
            if expected is None or target!=expected:
                stop[0]=0 if expected is None else 4
                if expected is not None and last[0][0]=='sys' and last[0][1] not in (0x24b,0x16b9):diag[0]=4
                uc.emu_stop();return
        key=('u33' if cs==cpu.cs else 'res' if cs==resident else 'sys' if cs==system else None,ip)
        if key==('sys',0xe2):stop[0]=3;diag[0]=2;uc.emu_stop();return
        if key==('sys',0x23f):walk[0]=0
        if key[0]=='sys' and ip in (0x3b7,0x3c3) and (word(cpu.ds,0x4414),word(cpu.ds,0x4416))!=(0xa9,system):stop[0]=3;diag[0]=3;uc.emu_stop();return
        if key==('sys',0x319) and last[0] in (('sys',0x32c),('sys',0x334)):
            walk[0]+=1
            if walk[0]>cap:stop[0]=3;diag[0]=5;uc.emu_stop();return
        if key==stop_at:stop[0]=2;uc.emu_stop();return
        assert key in decoded,('unexpected original PC',key,cs)
        ins=decoded[key]
        if key[0]!='sys':
            if not left[0]:stop[0]=2;uc.emu_stop();return
            left[0]-=1
        seen.add(key)
        if previous[0] is not None:edges.add((previous[0],key))
        previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):calls.append((cs,(ip+ins.size)&65535))
        if ins.mnemonic in ('ret','retf'):
            sp=uc.reg_read(UC_X86_REG_SP);ss=uc.reg_read(UC_X86_REG_SS)
            target=(word(ss,sp+2) if ins.mnemonic=='retf' else cs,word(ss,sp));pending[0]=(target,calls.pop() if calls else None,key==('sys',0x24b))
        last[0]=key
    uc.hook_add(UC_HOOK_CODE,hook);uc.emu_start(physical(cpu.cs,cpu.ip),2*SIZE,count=max(100000,4*budget))
    assert stop[0] is not None,'original did not stop'
    return stop[0],bytes(backing),Cpu(*(uc.reg_read(r)&65535 for r in REGISTERS)),left[0],diag[0],seen,edges


def compare(label,state,bodies,decoded,lib,budget=1000000,cap=64):
    install(*state,bodies);a=run_c(lib,state,budget,cap);b=original(state,decoded,budget,cap)
    assert (a[0],a[3],a[4])==(b[0],b[3],b[4]),(label,'status/budget/diagnostic',(a[0],a[3],a[4]),(b[0],b[3],b[4]))
    for name,_ in Cpu._fields_:assert getattr(a[2],name)==getattr(b[2],name),(label,name,hex(getattr(a[2],name)),hex(getattr(b[2],name)))
    if a[1]!=b[1]:raise AssertionError((label,'RAM',[(hex(i),x,y) for i,(x,y) in enumerate(zip(a[1],b[1])) if x!=y][:12]))
    for (kind,start),blob in bodies.items():
        at=physical({'u33':state[1].cs,'res':state[2],'sys':state[3]}[kind],start)
        assert b[1][at:at+len(blob)]==state[0][at:at+len(blob)],(label,'immutable code changed')
    return b[5],b[6],a[0],a[4]


def cases(quick=False):
    yield 'empty',dict(party=()),1000000,64
    for mode in (0,1,2,255):
        for tag in (0,1,102,103,127,128,230,255):
            for threshold in (0,1,65535):
                yield f'branches-{mode}-{tag}-{threshold}',dict(party=({'mode':mode,'tag':tag},),threshold=threshold),1000000,64
    for val in (0,1,3,4,7,8,127,128,251,252,255):
        yield f'division-{val}',dict(party=({'mode':1},),table_value=val,index=255),1000000,64
    for mode in ('exact','split','split-carry','skip','top','top-exact','fail','top-granule-fail','cyclic'):
        yield f'heap-{mode}',dict(heap_mode=mode),1000000,2
    for mode in ('top','fail'):
        yield f'custom-{mode}',dict(heap_mode=mode,custom_heap=True),1000000,64
    for size in (2,3,8,255,256,257):
        yield f'party-size-{size}',dict(party=tuple(dict(mode=i%3,tag=(i*17)&255) for i in range(size)),threshold=2),1000000,64
    rng=random.Random(0x3312e1)
    for i in range(20 if quick else 150):
        party=tuple(dict(mode=rng.randrange(3),tag=rng.randrange(256),fields={0x13:rng.randrange(26),0x10:rng.randrange(26),0x16:rng.randrange(101),0xaa:rng.randrange(2)}) for _ in range(rng.randrange(5)))
        yield f'random-{i}',dict(party=party,heap_mode=rng.choice(('top','exact','split','skip')),threshold=rng.randrange(5),window_value=rng.randrange(65536),index=rng.randrange(256),table_value=rng.randrange(256),flags=rng.choice((2,0xd7,0x8d7,0x402))),1000000,64
    for budget in (0,1,2,4,10,30,100):yield f'limit-{budget}',dict(),budget,64
    for sp in (0x100,0x101,0x7fff,0xffef,0xfff0):yield f'stack-{sp:x}',dict(sp=sp),1000000,64

    yield 'failure-after-first',dict(party=({},{}),heap_mode='top-exact'),1000000,64
    yield 'party-cycle-limit',dict(party=({},),cycle=True),2500,64

    for label,options,budget in resident_cases(quick):
        if options.get('kind','u9e')!='u9e' or budget<100000:continue
        # Copy only synthetic data; the U33 original code, stack, links, globals
        # and heap are constructed independently. U9E would have updated the
        # member's pair before calling this resident body.
        resident_options={k:v for k,v in options.items() if k in ('items','tables','record_fields')}
        resident_options.update(old_major=options.get('major',18),old_minor=options.get('minor',50))
        yield 'resident-'+label,dict(resident_options=resident_options),1000000,64

    yield 'ds-ss-disjoint-alias',dict(alias_stack_data=True),1000000,64
    for sp in (0x100,0x8000,0xfff0):
        yield f'high-ss-a20-{sp:x}',dict(ss=0xffff,sp=sp),1000000,64


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compile-only',action='store_true');p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);args=p.parse_args()
    seen=set();edges=set();statuses={};diags={};count=0
    with tempfile.TemporaryDirectory() as tmp:
        lib=build_model(tmp)
        if args.compile_only:print('CHECKER_BUILD_OK');return
        bodies,decoded,pins=source()
        for label,options,budget,cap in cases(args.quick):
            v,e,status,diag=compare(label,fixture(**options),bodies,decoded,lib,budget,cap);seen|=v;edges|=e;count+=1
            statuses[status]=statuses.get(status,0)+1;diags[diag]=diags.get(diag,0)+1
    required={(key,(key[0],target)) for key,ins in decoded.items() if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' for target in (int(ins.op_str,16),ins.address+ins.size)}
    caller_sites={k for k in decoded if k[0]=='u33'};caller_edges={e for e in required if e[0][0]=='u33'}
    resident_sites={k for k in decoded if k[0]=='res'};resident_edges={e for e in required if e[0][0]=='res'}
    infeasible_res={(('res',a),('res',b)) for a,b in RES_INFEASIBLE}
    assert not edges&infeasible_res,'resident branch proof contradicted'
    report=dict(target='OVL_0033_12E1',cases=count,cpu_words=14,ram_bytes=SIZE,masked_flags=0,cpu_substitutions=0,body_replacements=0,statuses=statuses,getmem_diagnostics=diags,
                caller_starts=len(caller_sites&seen),caller_total_starts=len(caller_sites),caller_outcomes=len(caller_edges&edges),caller_total_outcomes=len(caller_edges),
                resident_starts=len(resident_sites&seen),resident_total_starts=len(resident_sites),resident_outcomes=len(resident_edges&edges),resident_feasible_outcomes=len(resident_edges-infeasible_res),
                resident_missing_starts=sorted(resident_sites-seen),resident_missing_outcomes=sorted(resident_edges-edges-infeasible_res),
                resident_infeasible_outcomes=[dict(site=f'{a:04X}',target=f'{b:04X}',reason=r) for (a,b),r in RES_INFEASIBLE.items()],
                caller_missing_starts=sorted(caller_sites-seen),caller_missing_outcomes=sorted(caller_edges-edges),
                closure_starts=len(seen),closure_total_starts=len(decoded),closure_outcomes=len(edges),closure_total_outcomes=len(required),
                closure_missing_starts=sorted(set(decoded)-seen),closure_missing_outcomes=sorted(required-edges),pins=pins,
                body_coverage=[dict(**pin,covered_starts=sum((pin['kind'],ip) in seen for ip in range(int(pin['start'],16),int(pin['end'],16)))) for pin in pins],
                system_infeasible_starts=[dict(offset=f'{ip:04X}',reason='Size22 is nonzero: nil-size branch excluded.' if ip in (0x383,0x385,0x386) else 'Default HeapError returns AL0: AL1 nil and AL>=2 retry continuations excluded.') for ip in (0x383,0x385,0x386,0x3cb,0x3cd,0x3d0)],
                system_infeasible_outcomes=[dict(site=f'{a:04X}',target=f'{b:04X}',reason='Size22 cannot take zero-size branch.' if a==0x30a else 'Default HeapError returns AL0; alternate handler continuations are outside boundary.') for a,b in ((0x30a,0x383),(0x3c9,0x3cb),(0x3cb,0x383),(0x3cb,0x3cd))],
                closure_limits=['Constant allocation size22 excludes zero-size GetMem path and other rounding values.',
                  'Default HeapError always returns zero; retry/nil custom-handler continuations are outside this composition.',
                  'RunError is an exact-state stop at SYSTEM00E2; runtime termination tail is not executed.',
                  'Custom HeapError and allocator walk limits propagate exact stops; no fake successful return.',
                  'Resident root-origin starts/outcomes are independently required here; reused synthetic data does not transfer prior coverage counts.'],natural_game_route_measured=False)
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True));assert not report['caller_missing_starts'] and not report['caller_missing_outcomes'],'caller coverage incomplete'
    assert not report['resident_missing_starts'] and not report['resident_missing_outcomes'],'resident root-origin coverage incomplete'
    assert {ip for kind,ip in report['closure_missing_starts'] if kind=='sys'}=={0x383,0x385,0x386,0x3cb,0x3cd,0x3d0},'runtime start gaps changed'
    assert {(a[1],b[1]) for a,b in report['closure_missing_outcomes'] if a[0]=='sys'}=={(0x30a,0x383),(0x3c9,0x3cb),(0x3cb,0x383),(0x3cb,0x3cd)},'runtime outcome gaps changed'
if __name__=='__main__':main()
