#!/usr/bin/env python3
"""U9E +02FA grouped sidecar composition with the inherited handler boundary.
Oracle mechanics reuse the accepted +0160 audit; original media stay private."""
from loaded_stub_fixtures import loaded_sites
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
import u9e_sidecar_0160_cpu_audit as prior
from u9e_sidecar_0160_cpu_audit import (Cpu,Layout,Limits,Diagnostic,ROOT,SIZE,CS,DS,SS,NODES,FLAGS,DEFAULT,REGISTERS,
    physical,put16,check,segments,call_target,CALLS)
base=prior.base
ENTRY,END=0x2FA,0xABB
BODY_SHA='9ce9d83e68edd4ddf42dbf1215f0e6f0450219a64b948c0604f39d6cc60a4bc5'
SOURCES=('overlay_u9e_sidecar_groups_02fa_cpu',)+prior.SOURCES
GROUPS=((126,), (63, 37, 25, 71), (85, 86, 87, 68, 79, 80, 76, 73, 77), (64, 65, 66, 67, 68, 69, 70, 79, 85, 86, 87), (29, 3, 6), (28, 41, 104, 120, 101, 115, 116, 119, 123, 96, 94, 60, 122, 117), (113, 61, 122, 60, 91, 10, 20, 105, 106, 112, 114, 118, 17, 93, 101, 28), (51, 52, 53, 31), (99, 81, 82, 89, 72, 56), (105, 106, 107, 108, 109, 110, 111, 112, 124, 125), (1, 2, 33, 36, 49, 3, 6, 18, 26), (33, 17, 8, 9, 45, 46, 30), (8, 9, 10, 17, 20, 33, 36, 45, 46, 49, 61, 111, 125, 90, 97), (99, 100, 103, 75, 74), (83, 84, 88, 121), (21, 30, 74, 75, 11), (25, 71, 37, 47, 48, 89), (1, 2, 11), (39, 42, 58), (98, 23, 72, 56, 11))
GROUP_CALLS={785: (0, 126), 805: (1, 63), 818: (1, 37), 831: (1, 25), 844: (1, 71), 864: (2, 85), 877: (2, 86), 890: (2, 87), 903: (2, 68), 916: (2, 79), 929: (2, 80), 942: (2, 76), 955: (2, 73), 968: (2, 77), 991: (3, 64), 1004: (3, 65), 1017: (3, 66), 1030: (3, 67), 1043: (3, 68), 1056: (3, 69), 1069: (3, 70), 1082: (3, 79), 1095: (3, 85), 1108: (3, 86), 1121: (3, 87), 1141: (4, 29), 1154: (4, 3), 1167: (4, 6), 1190: (5, 28), 1203: (5, 41), 1216: (5, 104), 1229: (5, 120), 1242: (5, 101), 1255: (5, 115), 1268: (5, 116), 1281: (5, 119), 1294: (5, 123), 1307: (5, 96), 1320: (5, 94), 1333: (5, 60), 1346: (5, 122), 1359: (5, 117), 1382: (6, 113), 1395: (6, 61), 1408: (6, 122), 1421: (6, 60), 1434: (6, 91), 1447: (6, 10), 1460: (6, 20), 1473: (6, 105), 1486: (6, 106), 1499: (6, 112), 1512: (6, 114), 1525: (6, 118), 1538: (6, 17), 1551: (6, 93), 1564: (6, 101), 1577: (6, 28), 1597: (7, 51), 1610: (7, 52), 1623: (7, 53), 1636: (7, 31), 1656: (8, 99), 1669: (8, 81), 1682: (8, 82), 1695: (8, 89), 1708: (8, 72), 1721: (8, 56), 1744: (9, 105), 1757: (9, 106), 1770: (9, 107), 1783: (9, 108), 1796: (9, 109), 1809: (9, 110), 1822: (9, 111), 1835: (9, 112), 1848: (9, 124), 1861: (9, 125), 1881: (10, 1), 1894: (10, 2), 1907: (10, 33), 1920: (10, 36), 1933: (10, 49), 1946: (10, 3), 1959: (10, 6), 1972: (10, 18), 1985: (10, 26), 2005: (11, 33), 2018: (11, 17), 2031: (11, 8), 2044: (11, 9), 2057: (11, 45), 2070: (11, 46), 2083: (11, 30), 2106: (12, 8), 2119: (12, 9), 2132: (12, 10), 2145: (12, 17), 2158: (12, 20), 2171: (12, 33), 2184: (12, 36), 2197: (12, 45), 2210: (12, 46), 2223: (12, 49), 2236: (12, 61), 2249: (12, 111), 2262: (12, 125), 2275: (12, 90), 2288: (12, 97), 2308: (13, 99), 2321: (13, 100), 2334: (13, 103), 2347: (13, 75), 2360: (13, 74), 2380: (14, 83), 2393: (14, 84), 2406: (14, 88), 2419: (14, 121), 2439: (15, 21), 2452: (15, 30), 2465: (15, 74), 2478: (15, 75), 2491: (15, 11), 2511: (16, 25), 2524: (16, 71), 2537: (16, 37), 2550: (16, 47), 2563: (16, 48), 2576: (16, 89), 2596: (17, 1), 2609: (17, 2), 2622: (17, 11), 2641: (18, 39), 2654: (18, 42), 2667: (18, 58), 2686: (19, 98), 2699: (19, 23), 2712: (19, 72), 2725: (19, 56), 2738: (19, 11)}
def source():
    bodies,decoded,inner=prior.source()
    e=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();o=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    u=next(u for u in find_units(e[parse_mz(e)['header_bytes']:],o) if u['stub_para']==0x9E)
    blob=o[u['ovr_offset']+ENTRY:u['ovr_offset']+END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(blob,ENTRY))
    check(len(blob)==1985 and len(ins)==911 and sum(i.size for i in ins)==len(blob) and sha256(blob).hexdigest()==BODY_SHA,'02FA identity')
    check(ins[-1].address==0xAB8 and ins[-1].mnemonic=='retf' and ins[-1].op_str=='6','02FA terminal')
    check({i.address for i in ins if i.mnemonic=='call'}==set(GROUP_CALLS) and all(i.op_str=='0x160' for i in ins if i.mnemonic=='call'),'02FA calls')
    check(not [f for f in u['fixups'] if ENTRY<=f<END],'02FA unexpected far fixup')
    bodies['caller',ENTRY]=blob;decoded.update({('caller',i.address):i for i in ins})
    return bodies,decoded,dict(bytes=len(blob),instructions=len(ins),sha256=BODY_SHA,file_start=u['ovr_offset']+ENTRY,
      file_end=u['ovr_offset']+END,next_public_entry=0xAE7,trailing_bytes_to_next_entry=0xAE7-END,group_calls=len(GROUP_CALLS),
      distinct_types=len({v[1] for v in GROUP_CALLS.values()}),inner=inner)
def library(directory):
    path=directory/'groups.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),
      *[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u9e_sidecar_groups_02fa_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Layout),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    f.restype=ctypes.c_int;return f
def build(bodies,decoded,*,group=0,**options):
    options.setdefault('members',())
    m,c,l=prior.build(bodies,decoded,**options);c.ip=ENTRY
    selected=options.get('selected',2)
    for j,v in enumerate(((0x1000+0x200*selected)&65535,NODES,0xA500|group)):put16(m,c.ss,(c.sp+4+2*j)&65535,v)
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
        if key==('caller',0x160):
            s['callee']=s['nested']=-1
            remaining[:]=budgets
            sp=uc.reg_read(R.UC_X86_REG_SP)
            if not 0x186<=sp<=0xFFF6:return halt(1)
        if kind=='caller' and ip in GROUP_CALLS:
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(0x160-ip-3)&65535):return halt(2)
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
    result=[(f'empty-group-{g}',dict(group=g,flags=FLAGS[g%6])) for g in range(20)]
    result += [('direct-first',dict(group=0,direct=(126,))),
      ('direct-late',dict(group=1,direct=(71,))),
      ('fallback-noncombat',dict(group=15,members=((21,),),combat=0)),
      ('fallback-near',dict(group=10,members=((49,),),party=((10,10,1),(11,10,1)))),
      ('invalid-group',dict(group=255))]
    if quick:return result
    for site,(g,key) in GROUP_CALLS.items():result.append((f'direct-site-{site:04x}',dict(group=g,direct=(key,),flags=FLAGS[site%6])))
    for g in range(20,256):result.append((f'invalid-{g}',dict(group=g,flags=FLAGS[g%6])))
    for site,(g,key) in GROUP_CALLS.items():result.append((f'bad-call-{site:04x}',dict(group=g,writes=((CS,site,b'\x90'),))))
    def cap(index,value):
        b=list(DEFAULT);b[index]=value;return tuple(b)
    result += [('party-limit',dict(group=15,budgets=cap(0,0))),('lookup-limit',dict(group=0,direct=(126,),budgets=cap(5,0))),
      ('lookup-cycle',dict(group=0,direct=(1,2),lookup_cycle=True,budgets=cap(5,3))),
      ('slot-limit',dict(group=15,members=((21,),),budgets=cap(2,0))),
      ('scan-limit',dict(group=15,members=((21,),),budgets=cap(3,1))),
      ('line-limit',dict(group=10,members=((49,),),budgets=cap(4,1))),
      ('sort-limit',dict(group=10,members=((49,),),party=tuple((10+i,10,1) for i in range(5)),budgets=cap(6,1))),
      ('candidate-limit',dict(group=10,members=((49,),),budgets=cap(1,0))),
      ('same-segment',dict(group=10,ss=DS)),('a20-stack',dict(group=1,ss=0xFFFF)),
      ('save-alias',dict(group=15,ss=DS,sp=0x6800)),('map-slot-alias',dict(group=10,members=((49,),),map_alias=True))]
    for sp in (0x196,0x197,0x8001,0xFFF5,0xFFF6):result.append((f'sp-{sp:x}',dict(group=15,sp=sp)))
    for ip in (*CALLS,0x2F1):result.append((f'inner-code-{ip:x}',dict(group=10,members=((49,),),writes=((CS,ip,b'\x90'),))))
    rng=random.Random(0x9E02FA)
    for n in range(24):
        g=rng.randrange(20);key=rng.choice(GROUPS[g]);result.append((f'random-{n}',dict(group=g,direct=(key,) if n%3 else (),
          members=((49,),) if n%4 else (),selected=rng.randrange(1,4),party=tuple((rng.randrange(9,13),rng.randrange(9,13),rng.randrange(3)) for _ in range(3)),
          flags=FLAGS[n%6])))
    return result
def differential(bodies,decoded,f,quick=False):
    visited=set();edges=set();stops={};pending=0;binding=0;diagnostics={}
    specs=cases(quick)
    for n,(name,options) in enumerate(specs):
        options=dict(options);budgets=options.pop('budgets',DEFAULT)
        print('BEGIN',n+1,name,flush=True)
        v,e,status,c,d=compare(name,build(bodies,decoded,**options),bodies,decoded,f,budgets)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1
        if status==5:pending+=1
        if status==2 and c.ip in (0x22A,0x250):binding+=1
        if status==3:diagnostics[f'{d.callee}:{d.status}:{c.cs:04X}:{c.ip:04X}']=diagnostics.get(f'{d.callee}:{d.status}:{c.cs:04X}:{c.ip:04X}',0)+1
    caller={k for k in decoded if k[0]=='caller' and ENTRY<=k[1]<END}
    branches={k for k in caller if decoded[k].mnemonic in ('je','jne')}
    outcomes={e for e in edges if e[0] in branches}
    if not quick:check(caller<=visited and len(outcomes)==2*len(branches),'02FA coverage')
    return dict(cases=len(specs),stops=stops,callee_stops=diagnostics,cpu_words=14,ram_bytes=SIZE,caller_starts=len(caller&visited),
      caller_starts_total=len(caller),caller_outcomes=len(outcomes),caller_outcomes_total=2*len(branches),
      reached_group_calls=len({ip for kind,ip in visited if kind=='caller' and ip in GROUP_CALLS}),
      chain_starts=len(visited),chain_outcomes=len(edges),unicorn_pending_af_exceptions=pending,unicorn_binding_af_exceptions=binding,
      uncovered_sites=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)])
def main():
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='u9e02fa-') as tmp:
        f=library(Path(tmp))
        from loaded_stub_checks import verify_refusals
        memory,cpu,layout=build(bodies,decoded)
        limits=Limits(*DEFAULT);diagnostic=Diagnostic()
        report['loaded_stub_guards']=verify_refusals(memory,Cpu.from_buffer_copy(cpu),
            lambda ram,state: f(ram,SIZE,ctypes.byref(state),ctypes.byref(layout),ctypes.byref(limits),ctypes.byref(diagnostic)),
            [(layout.chain_cs,p) for p in (0x92A,0x9D8)])
        if a.unicorn:report['unicorn']=differential(bodies,decoded,f,a.quick)
        if a.native:
            from u9e_sidecar_groups_02fa_cpu_native import native
            report['native']=native(bodies,decoded,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+chr(10))
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['u9e_sidecar_groups_02fa_cpu_audit']=sys.modules[__name__]
    main()
