#!/usr/bin/env python3
"""Pinned original-byte CPU14/all-RAM audit of OVL_007F_063A and its real callees.

No callee stubs. Counted budget: 063A, 0054, 0000 and the 13AB/11EA closure.
003A, 05E0, FillChar and LongDiv retain their unbudgeted merged contracts.
Finite party traversals and immutable code are required. Exact FLAGS except
explicit last-writer pinned MUL/shift splits, asserted rather than masked.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import sys
import tempfile
import time
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as U
import ovl_007f_0054_cpu_audit as B
from ovl_007f_0000_cpu_audit import check
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from pinned_shift_flags import LastFlagWriter, declared_flags_split
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('OVL_007F_063A',)
ENTRY, END = 0x063A, 0x0A01
BODY_SHA = 'f23626a4d567fce56738f4d2c250c263f5b8b90a716121a070bfab52db85c736'
SP_MIN, SP_MAX = 0x0200, 0xFFFC
STATUS = ('RETURNED', 'ARGUMENT', 'CODE', 'LIMIT', 'TRANSFER', 'CALLEE')
SOURCES = B.SOURCES + ['overlay_u7f_063a_cpu', 'overlay_u7f_record_word_add_003a',
    'overlay_u7f_party_count_05e0', 'system_longdiv_0a76_cpu', 'system_fillchar_bus_16a8_cpu']
PINS = ((0x003A, 0x0054, 9, 'ef4d49e3fef030f2acc13a9d5f78563d091bb60ce167bb986143671b6e14059f'),
        (0x05E0, 0x063A, 30, 'd4e73837529d5f4bb1ac788885f216ba39ccf5f67778a8278862f846c752a969'),
        (ENTRY, END, 385, BODY_SHA))
FAR_SITES = {0x0664: 0x16A8, 0x0675: 0x16A8, 0x06BA: 0x0A76, 0x06E9: 0x0A76}
NEAR_SITES = {0x0652:0x05E0,0x075D:0x0054,0x07A8:0x003A,0x07D0:0x0054,
              0x0805:0x003A,0x086C:0x003A,0x08C7:0x0000,0x091D:0x003A,0x097E:0x003A}

class Diag(ctypes.Structure):
    _fields_ = [('callee',ctypes.c_int),('callee_status',ctypes.c_int),('site',ctypes.c_uint16),
                ('d0054',B.Diag54),('d0000',B.A.Diag)]

def library(directory, override=None):
    path=Path(directory)/'u7f063a.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include')]+
        [str((override or {}).get(s,ROOT/f'src/recovered/{s}.c')) for s in SOURCES]+['-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path))
    lib.pr_u7f_063a_run.argtypes=[ctypes.POINTER(B.Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(ctypes.c_size_t),ctypes.POINTER(Diag)]
    lib.pr_u7f_063a_run.restype=ctypes.c_int
    return lib

def source():
    bodies,decoded,meta=B.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media provenance')
    unit=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x7F)
    code=ovr[unit['ovr_offset']:unit['ovr_offset']+unit['code_size']]
    md=Cs(CS_ARCH_X86,CS_MODE_16)
    for lo,hi,n,digest in PINS:
        blob=code[lo:hi]; ins=list(md.disasm(blob,lo))
        check(sha256(blob).hexdigest()==digest and len(ins)==n and sum(i.size for i in ins)==len(blob),f'span {lo:04X}')
        bodies['w',lo]=blob; decoded.update({('w',i.address):i for i in ins})
    body=[i for (k,a),i in decoded.items() if k=='w' and ENTRY<=a<END]
    check({i.address:int(i.op_str,0) for i in body if i.mnemonic=='call'}==NEAR_SITES,'near call sites')
    check({i.address:int(i.op_str.split(', ')[1],0) for i in body if i.mnemonic=='lcall'}==FAR_SITES,'far call offsets')
    check([f for f in unit['fixups'] if ENTRY<=f<END]==[s+3 for s in FAR_SITES],'four segment relocations')
    starts={i.address for i in body}
    check(all(int(i.op_str,0) in starts for i in body if i.mnemonic.startswith('j')),'body branches stay inside')
    check([(i.address,i.op_str) for i in body if i.mnemonic=='retf']==[(0xA00,'')],'one RETF')
    image=bytes(unpack(exe)[0]); runtime=[]
    for lo,hi,n in ((0xA76,0xAE4,55),(0x16A8,0x16BC,7)):
        blob=image[physical(0xAF8,lo):physical(0xAF8,hi)];ins=list(md.disasm(blob,lo))
        check(len(ins)==n and sum(i.size for i in ins)==hi-lo,f'runtime {lo:04X}')
        bodies['sys',lo]=blob;decoded.update({('sys',i.address):i for i in ins})
        runtime.append(dict(start=lo,end=hi,instructions=n,sha256=sha256(blob).hexdigest()))
    return bodies,decoded,dict(body_bytes=967,body_instructions=385,body_sha256=BODY_SHA,
        file_offset=unit['ovr_offset']+ENTRY,near_sites=NEAR_SITES,far_sites=FAR_SITES,runtime=runtime)

def word(m,s,o):
    p=physical(s,o&65535);return m[p]|(m[(p+1)&(SIZE-1)]<<8)

def fixture(bodies, *, statuses=(0,0), pools=(19,7,0,0,0,0,0), totals=None, kinds=None,
            money=0, sp=0x8000, flags=0x0202, w_cs=0x6000,res_cs=0x7000,sys_cs=0x7400,
            ds=0x1000,ss=0x2000,rec_seg=0x3000,rec_off=0x100,nodes=None,regs=None,patches=()):
    m=bytearray([0xA5])*SIZE
    nodes=nodes or [(rec_seg,(rec_off+0x200*i)&65535) for i in range(len(statuses))]
    check(len(nodes)==len(statuses),'node/status count')
    totals=totals if totals is not None else [0]*len(nodes)
    kinds=kinds if kinds is not None else [0x0C]*len(nodes)
    for j,((seg,off),status) in enumerate(zip(nodes,statuses)):
        for k in range(0x11D):m[physical(seg,(off+k)&65535)]=0
        m[physical(seg,(off+0x10)&65535)]=kinds[j]
        m[physical(seg,(off+0x16)&65535)]=0x20
        m[physical(seg,(off+0x84)&65535)]=status
        for k in range(7):put16(m,seg,(off+0x88+2*k)&65535,money)
        put16(m,seg,(off+0x102)&65535,totals[j])
        nxt=nodes[j+1] if j+1<len(nodes) else (0,0)
        put16(m,seg,(off+0x104)&65535,nxt[1]);put16(m,seg,(off+0x106)&65535,nxt[0])
    head=nodes[0] if nodes else (0,0)
    put16(m,ds,0x5D96,head[1]);put16(m,ds,0x5D98,head[0])
    for k,v in enumerate(pools):
        put16(m,ds,0x67F4+4*k,v&65535);put16(m,ds,0x67F6+4*k,(v>>16)&65535)
    segs={'w':w_cs,'res':res_cs,'sys':sys_cs}
    for (kind,start),blob in bodies.items():
        at=physical(segs[kind],start);m[at:at+len(blob)]=blob
    put16(m,w_cs,0xF,res_cs)
    for site in FAR_SITES:put16(m,w_cs,site+3,sys_cs)
    r=regs or (0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x1111)
    c=Cpu(*r[:7],sp,w_cs,ENTRY,ds,r[7],ss,flags|2)
    put16(m,ss,sp,0x9000);put16(m,ss,(sp+2)&65535,0x8000)
    for seg,off,val,bits in patches:
        if bits==8:m[physical(seg,off)]=val
        else:put16(m,seg,off,val)
    return m,c,dict(segs=segs,nodes=nodes,statuses=statuses,pools=pools,totals=totals,kinds=kinds,money=money)

def run_c(lib,state,budget):
    m,c,_=state;ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);out=Cpu.from_buffer_copy(c)
    left=ctypes.c_size_t(budget);d=Diag();bus=B.Bus(ctypes.addressof(ram),None,None,None,None)
    status=lib.pr_u7f_063a_run(ctypes.byref(bus),SIZE,ctypes.byref(out),ctypes.byref(left),ctypes.byref(d))
    return STATUS[status],bytes(ram),out,left.value,d

def original(state,decoded,budget):
    m,c,info=state;backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc=Uc(UC_ARCH_X86,UC_MODE_16)
    for base in (0,SIZE):uc.mem_map_ptr(base,SIZE,7,ctypes.addressof(backing))
    for (_, _type),reg,value in zip(Cpu._fields_,REGISTERS,[getattr(c,n) for n,_ in Cpu._fields_]):uc.reg_write(reg,value)
    kind_of={v:k for k,v in info['segs'].items()};left=budget;calls=[];pending=None;status=None
    seen=set();edges=set();prev=None;writer=LastFlagWriter();trace=[]
    domain=dict(aperture_accesses=0,party_count_entries=0,party_count_returns=0)
    def memory_hook(em,access,address,size,value,user):
        for byte in range(size):
            if 0xA0000 <= ((address+byte)&(SIZE-1)) < 0xC0000:
                domain['aperture_accesses']+=1
    uc.hook_add(UC_HOOK_MEM_READ|UC_HOOK_MEM_WRITE,memory_hook)
    entry_ret=(word(m,c.ss,c.sp+2),word(m,c.ss,c.sp))
    def hook(em,address,size,user):
        nonlocal left,pending,status,prev
        cs=em.reg_read(U.UC_X86_REG_CS);ip=em.reg_read(U.UC_X86_REG_IP);key=(kind_of.get(cs),ip)
        writer.finish(em)
        if pending is not None:
            target,expected=pending;pending=None
            if expected is None or target!=expected:
                status=('RETURNED' if target==entry_ret else 'TRANSFER') if expected is None else 'TRANSFER'
                em.emu_stop();return
        if key==('sys',0xE2):status='CALLEE';em.emu_stop();return
        check(key in decoded,f'original unknown PC {cs:04X}:{ip:04X}')
        check(not 0xA0000 <= (address&(SIZE-1)) < 0xC0000,'code in excluded aperture')
        if key==('w',0x05E0):domain['party_count_entries']+=1
        if key==('w',0x0639):domain['party_count_returns']+=1
        ins=decoded[key]
        counted=key[0]=='res' or (key[0]=='w' and (ip<0x20 or 0x54<=ip<0xA8 or ENTRY<=ip<END))
        if counted:
            if not left:status='LIMIT';em.emu_stop();return
            left-=1;trace.append(key)
        if prev is not None:edges.add((prev,key))
        prev=key;seen.add(key);writer.step(em,address)
        if ins.mnemonic in ('call','lcall'):calls.append((cs,ip+ins.size))
        if ins.mnemonic in ('ret','retf'):
            ss=em.reg_read(U.UC_X86_REG_SS);sp=em.reg_read(U.UC_X86_REG_SP)
            target=(word(backing,ss,sp+2) if ins.mnemonic=='retf' else cs,word(backing,ss,sp))
            pending=(target,calls.pop() if calls else None)
    uc.hook_add(UC_HOOK_CODE,hook)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=max(100000,40*budget))
    check(status is not None,'original did not stop; nonterminating unbudgeted callee is outside fixture contract')
    check(domain['aperture_accesses']==0,'strict fixture reached excluded aperture')
    check(domain['party_count_entries']==domain['party_count_returns'],'strict fixture did not finish unbudgeted +05E0')
    out=Cpu(*(uc.reg_read(r)&65535 for r in REGISTERS))
    return status,bytes(backing),out,left,seen,edges,writer,trace,domain

def compare(name,state,decoded,lib,budget):
    e=original(state,decoded,budget);a=run_c(lib,state,budget)
    check(a[0]==e[0],f'{name}: stop C {a[0]} original {e[0]}')
    check(a[3]==e[3],f'{name}: budget C {a[3]} original {e[3]}')
    for field,_ in Cpu._fields_:
        if field=='flags':
            problem=declared_flags_split(a[2].flags,e[2].flags,e[6]);check(problem is None,f'{name}: {problem}')
        else:check(getattr(a[2],field)==getattr(e[2],field),f'{name}: {field} C={getattr(a[2],field):04X} original={getattr(e[2],field):04X}')
    if a[1]!=e[1]:
        diffs=[(hex(i),a[1][i],e[1][i]) for i in range(SIZE) if a[1][i]!=e[1][i]]
        raise AssertionError((name,'RAM',diffs[:12],len(diffs)))
    if e[0]=='CALLEE':check(a[4].callee==2 and a[4].callee_status==2,f'{name}: LongDiv error diagnostic')
    return e,a

def cases(quick=False):
    out=[]
    def add(name,budget=100000,**kw):out.append((name,kw,budget))
    add('empty',statuses=(),pools=(0,)*7)
    add('divide-zero-empty',statuses=(),pools=(1,0,0,0,0,0,0))
    for sts in ((0,),(0,0),(0,0,0),(0,0xB3),(1,0x7F,0x80,0xB3,0xFF,0),(0xB3,),(0x80,)):
        tag='-'.join(f'{s:02x}' for s in sts)
        for pool in (0,1,2,3,17,0xFFFF,0x10000,0x7FFFFFFF,0x80000000,0xFFFFFFFF):
            add(f'party-{tag}-p{pool:x}',statuses=sts,pools=(pool,)*7)
    for total in (0,1,1599,1600,1601,0x7FFF,0xFFFE,0xFFFF):
        for pool in (1,17,3200,0x10001):add(f'cap-{total}-{pool}',totals=(total,total),pools=(pool,)*7)
    for sts in ((0,0),(0,0xB3),(1,0x7F,0x80,0xB3,0xFF,0)):
        add('kinds-'+str(len(sts)),statuses=sts,kinds=tuple((0x0C,0x12,0x13,0x1D,0xFF,0)[i] for i in range(len(sts))),pools=(7,8,9,10,11,12,13))
    for sp in (SP_MIN,SP_MIN+1,0x7FFF,0xF001,SP_MAX-1,SP_MAX):add(f'sp-{sp:04x}',sp=sp)
    for flags in (2,0x202,0x602,0xAD7,0xE57,0x7202):add(f'flags-{flags:04x}',flags=flags)
    for off in (0,0xFE00,0xFEF8,0xFEFC,0xFF00,0xFFF0):add(f'offset-{off:04x}',statuses=(0,),rec_off=off)
    for seg in (0,0xFFF0,0xFFFF):add(f'a20-{seg:04x}',statuses=(0,),rec_seg=seg,rec_off=0x100)
    add('relocated',w_cs=0x6100,res_cs=0x7200,sys_cs=0x7600)
    add('ss-ds',ds=0x2000)
    for field in (0x88,0x102):
        for delta in (0,2,4):
            # If +102 aliases the return IP, +104 aliases return CS=8000:
            # initialize the resulting secondary record at 0000:8000 so
            # this live-frame alias remains a terminating-list fixture.
            patches=tuple((0,0x8000+k,0,8) for k in range(0x11D)) if field==0x102 and delta==0 else ()
            add(f'alias-{field:x}-ret{delta}',statuses=(0,),nodes=[(0x2000,0x8000+delta-field)],
                pools=(17,0,0,0,0,0,0),patches=patches)
    for budget in (range(0,90) if quick else range(0,450)):
        add(f'budget-{budget}',budget=budget)
    rng=random.Random(0x7F063A)
    for j in range(4 if quick else 40):
        n=rng.randrange(1,6);sts=tuple(rng.choice((0,0,1,0x7F,0x80,0xB3,0xFF)) for _ in range(n))
        add(f'random-{j}',statuses=sts,pools=tuple(rng.choice((0,1,rng.randrange(5000),0xFFFFFFFF)) for _ in range(7)),
            totals=tuple(rng.randrange(2000) for _ in range(n)),kinds=tuple(rng.choice((0,0x0C,0x12,0x13,0x1D)) for _ in range(n)))
    return out

def differential(bodies,decoded,lib,quick=False,only=None):
    chosen=[c for c in cases(quick) if only is None or c[0] in only]
    seen=set();edges=set();stops={};splits={};start=time.time();finite_calls=0
    for i,(name,kw,budget) in enumerate(chosen):
        if i%20==0:print(f'[{i+1}/{len(chosen)}] {name}',flush=True)
        state=fixture(bodies,**kw);e,a=compare(name,state,decoded,lib,budget)
        seen|=e[4];edges|=e[5];stops[e[0]]=stops.get(e[0],0)+1
        finite_calls+=e[8]['party_count_returns']
        if a[2].flags!=e[2].flags:
            rule=e[6].rule or 'shift';splits[rule]=splits.get(rule,0)+1
        for (kind,off),blob in bodies.items():
            at=physical(state[2]['segs'][kind],off)
            check(e[1][at:at+len(blob)]==state[0][at:at+len(blob)],f'{name}: wrote immutable code')
    wanted={('w',i) for k,i in decoded if k=='w' and ENTRY<=i<END}
    branches={(('w',a),('w',dest)) for (k,a),ins in decoded.items() if k=='w' and ENTRY<=a<END and ins.mnemonic.startswith('j') and ins.mnemonic!='jmp'
              for dest in (a+ins.size,int(ins.op_str,0))}
    return dict(cases=len(chosen),stops=stops,body_starts=len(seen&wanted),body_total=len(wanted),
        missing_starts=[f'{i:04X}' for _,i in sorted(wanted-seen)],conditional_edges=len(edges&branches),conditional_total=len(branches),
        missing_edges=[f'{a[1]:04X}->{b[1]:04X}' for a,b in sorted(branches-edges)],declared_flag_splits=splits,
        flags_masks=0,ram_bytes=SIZE,cpu_words=14,seconds=round(time.time()-start,1),
        domain=dict(aperture_accesses=0,completed_party_count_calls=finite_calls,
                    assertion='Every original memory access outside A0000..BFFFF; every entered +05E0 returns before C executes.'))

def exclusions(bodies,decoded,lib):
    """Construct excluded inputs; never run the unbounded C callee on a cycle.

    Aperture probe uses EGA read mode 0 / read map 0, with a concrete plane
    byte different from underlying RAM. This prefix performs no EGA writes.
    It is a hardware-read counterexample, not an EGA parity family.
    """
    receipts=[]
    for mode in ('self-cycle','aperture-read-map-0'):
        state=fixture(bodies,statuses=(0 if mode=='self-cycle' else 0x80,),
                      rec_seg=0x3000 if mode=='self-cycle' else 0xA000)
        m,c,info=state
        if mode=='self-cycle':
            put16(m,0x3000,0x204,0x100);put16(m,0x3000,0x206,0x3000)
        uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE)
        uc.mem_write(0,bytes(m));uc.mem_write(SIZE,bytes(m))
        for (field,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,field))
        steps=0;repeat=None;states={};arrived=False;aperture_reads=[]
        # A real backward edge into the party loop; derive it from original bytes.
        loop=min(int(ins.op_str,0) for (kind,addr),ins in decoded.items()
                 if kind=='w' and 0x5E0<=addr<0x63A and ins.mnemonic=='jmp' and int(ins.op_str,0)<addr)
        def memory(em,access,address,size,value,user):
            if 0xA0000<=address<0xC0000:
                check(mode!='self-cycle','cycle fixture reached aperture')
                aperture_reads.append(dict(address=address,size=size))
                # Plane 0 contains a B3 status and a terminating zero link.
                payload=bytes(0xB3 if p==0xA0184 else 0 for p in range(address,address+size))
                em.mem_write(address,payload)
        def hook(em,address,size,user):
            nonlocal steps,repeat,arrived
            steps+=1;cs=em.reg_read(U.UC_X86_REG_CS);ip=em.reg_read(U.UC_X86_REG_IP)
            if cs==c.cs and ip==0x655:
                arrived=True;em.emu_stop();return
            if mode=='self-cycle' and cs==c.cs and ip==loop:
                # Full CPU14 and 1 MiB RAM equality proves deterministic recurrence.
                key=(tuple(em.reg_read(r)&65535 for r in REGISTERS),sha256(bytes(em.mem_read(0,SIZE))).digest())
                if key in states:
                    repeat=dict(first_step=states[key],repeat_step=steps,period=steps-states[key],loop_ip=loop)
                    em.emu_stop()
                else:states[key]=steps
        uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_MEM_READ,memory)
        uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=20000)
        if mode=='self-cycle':
            check(repeat is not None and not arrived,'cyclic original must repeat before +05E0 returns')
            receipts.append(dict(family=mode,cases=1,original_full_state_repeat=repeat,c_model_run=False,
                state='DS party head 3000:0100; status 00; next=3000:0100; no interrupts or external writes'))
        else:
            check(arrived and aperture_reads,'aperture prefix must return from +05E0')
            actual_count=uc.reg_read(U.UC_X86_REG_AX)&255
            flat=run_c(lib,state,10)
            check(flat[0]=='LIMIT' and flat[2].ip==0x655,'flat comparison stops at same prefix')
            check(actual_count==1 and flat[2].ax&255==0,'constructed EGA/flat count divergence')
            receipts.append(dict(family=mode,cases=1,original_party_count=actual_count,flat_c_party_count=flat[2].ax&255,
                prefix_stop='063A through 0655',aperture_reads=aperture_reads,
                state='DS party head A000:0100; underlying RAM status 80; EGA plane 0 status B3; both next links zero; read mode/map 0'))
    return dict(families=2,cases=2,results=receipts,claimed_parity_cases=0)

def mutations(directory,bodies,decoded,baseline_lib):
    source_path=ROOT/'src/recovered/overlay_u7f_063a_cpu.c'
    original_text=source_path.read_text()
    candidates=[
        ('shift-af','szp(r,16u)|0x10u','szp(r,16u)|0x00u','budget-33',{},33),
        ('node-write','pr_bus_write8(b,p,(uint8_t)v)','pr_bus_write8(b,p,(uint8_t)(v+1u))','shares',{},100000),
        ('residual-flag','/* 09AD mov byte ptr [0x6dc0], 0 */','/* 09AD mov byte ptr [0x6dc0], 0 */',None,{},100000),
    ]
    # Select complete instruction lines so an unrelated identical expression
    # cannot receive a mutation accidentally.
    residual=next(line for line in original_text.splitlines() if '/* 09AD ' in line)
    candidates[2]=('residual-flag',residual,residual.replace('(0x0000u)','(0x0001u)'),'shares',{},100000)
    branch=next(line for line in original_text.splitlines() if '/* 0762 ' in line)
    if 'if(' in branch:
        candidates.append(('cap-branch',branch,branch.replace('if(', 'if(!(',1).replace(')goto', '))goto',1),
                           'cap',dict(statuses=(0,),totals=(1599,),pools=(17,0,0,0,0,0,0)),100000))
    receipts=[]
    for name,old,new,case,kw,budget in candidates:
        check(old!=new and original_text.count(old)==1,f'{name}: unique mutation anchor')
        compare(case,fixture(bodies,**kw),decoded,baseline_lib,budget)
        mutant=Path(directory)/f'{name}.c';mutant.write_text(original_text.replace(old,new))
        build=Path(directory)/name;build.mkdir()
        lib=library(build,{'overlay_u7f_063a_cpu':mutant})
        try:compare(case,fixture(bodies,**kw),decoded,lib,budget)
        except (AssertionError,ValueError) as error:
            receipts.append(dict(mutation=name,rejected=True,case=case,mismatch=str(error)))
        else:raise AssertionError(f'mutation survived: {name}')
    return receipts

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true')
    p.add_argument('--native',action='store_true')
    p.add_argument('--mutate',action='store_true')
    p.add_argument('--quick',action='store_true');p.add_argument('--only',action='append');p.add_argument('--json',type=Path)
    args=p.parse_args();report={}
    with tempfile.TemporaryDirectory(prefix='u7f063a-') as tmp:
        lib=library(tmp)
        if args.compile_only:print('CHECKER_BUILD_OK OVL_007F_063A');return
        bodies,decoded,meta=source();report['source']=meta
        if args.unicorn:
            report['exclusions']=exclusions(bodies,decoded,lib)
            report['unicorn']=differential(bodies,decoded,lib,args.quick,args.only)
        if args.native:
            from ovl_007f_063a_native import native
            report['native']=native(bodies,decoded,lib,args.quick)
        if args.mutate:report['mutations']=mutations(tmp,bodies,decoded,lib)
    if args.json:args.json.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
