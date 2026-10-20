#!/usr/bin/env python3
"""U3CE +014E original CPU14/all1MiB composed with raw resident2595/FreeMem."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
import res0464_bitmap_release_2595_audit as r
from res0464_bitmap_release_2595_audit import Cpu,ROOT,SIZE,REGISTERS,physical,put16,check
fm=r.fm
CHECKER_ROWS=('OVL_03CE_014E',)
CS,RESIDENT,DS,SS,SYSTEM=0x4000,r.CS,r.DS,r.SS,r.SYSTEM
ENTRY,END,RETF=0x14E,0x1B0,0x1AD
SITES=(0x17A,0x1A5)
SOURCES=['overlay_u3ce_bitmap_pair_release_014e_cpu.c',*r.SOURCES]
class Diagnostic(ctypes.Structure):
    _fields_=[('resident_status',ctypes.c_int),('resident',r.Diagnostic),('calls',ctypes.c_uint),('call_site',ctypes.c_uint16)]
def source():
    resident,image,rd,rc,fd,fc,rm=r.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);u=next(u for u in units if u['stub_para']==0x3CE)
    body=ovr[u['ovr_offset']+ENTRY:u['ovr_offset']+END];dis=Cs(CS_ARCH_X86,CS_MODE_16);ins=list(dis.disasm(body,ENTRY))
    check(len(body)==98 and len(ins)==40 and sha256(body).hexdigest()=='244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047','014E extent/hash')
    check((ins[-1].address,ins[-1].mnemonic,ins[-1].op_str)==(RETF,'retf','2'),'014E RETF2')
    check(u['entries'][3]==ENTRY and u['memory_segment']==0x3E9,'public entry3')
    check([(i.address,i.op_str) for i in ins if 'call' in i.mnemonic]==[(a,'0x464, 0x2595') for a in SITES],'only two static resident calls')
    check(all(s+3 in u['fixups'] for s in SITES),'outgoing fixups')
    incoming=[]
    for unit in units:
        for fix in unit['fixups']:
            b=ovr[unit['ovr_offset']+fix-3:unit['ovr_offset']+fix+2]
            if len(b)==5 and b[0]==0x9A and struct.unpack_from('<HH',b,1)==(0x2F,0x3E9):incoming.append((unit['stub_para'],fix-3))
    check(incoming==[(0x6C,0x3321),(0x6C,0x337B),(0x6C,0x3EBF),(0x6F,0x65F)],f'incoming fixups {incoming}')
    dd={'caller':{i.address:i for i in ins},'resident':rd,'freemem':fd}
    cc={w:{a:(a+i.size,int(i.op_str,16)) for a,i in table.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'} for w,table in dd.items()}
    meta=dict(target=CHECKER_ROWS[0],span=[ENTRY,END],file_start=u['ovr_offset']+ENTRY,bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),former_queue_bytes=122,trimmed_tail=24,incoming=incoming,resident=rm)
    return body,resident,image,dd,cc,meta

def library(directory):
    p=directory/'pairrelease.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u3ce_bitmap_pair_release_014e_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_uint16,ctypes.c_uint,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    return f

def build(body,resident,image,rng,*,index=0xA500,which=0,other=(0,0),ds=DS,bp=None,root_writes=(),**opts):
    opts=dict(opts);writes=opts.pop('writes',());ss=opts.get('ss',SS);sp=opts.get('sp',0x8000);system=opts.get('system',SYSTEM)
    slot=(DS,0x5E5A+8*(index&255)+4*which)
    m,c=r.build(resident,image,rng,slot=slot,**opts)
    if other!=(0,0):
        m[physical(other[0],(other[1]+8)&65535)]=1;put16(m,other[0],(other[1]+0x11)&65535,16);r.pair(m,other[0],(other[1]+0x13)&65535,(0,0))
    r.pair(m,DS,0x5E5A+8*(index&255)+4*(1-which),other)
    if ds!=DS:
        at=physical(ds,0);m[at:at+65536]=m[physical(DS,0):physical(DS,0)+65536]
        for a,b in fm.SPANS:
            at=physical(system,a);m[at:at+b-a]=image[physical(SYSTEM,a):physical(SYSTEM,b)]
    for j,v in enumerate((0x7000,0x9001,index)):put16(m,ss,sp+2*j,v)
    at=physical(CS,ENTRY);m[at:at+len(body)]=body
    for site in SITES:put16(m,CS,site+3,RESIDENT)
    for seg,off,blob in (*writes,*root_writes):
        for j,v in enumerate(blob):m[(physical(seg,off)+j)&(SIZE-1)]=v
    c.cs=CS;c.ip=ENTRY;c.ds=ds
    if bp is not None:c.bp=bp
    return m,c

def cases():
    result=[]
    for label,opts in r.cases():
        if 'slot' in opts or label=='callee-return-alias':continue
        opts=dict(opts)
        if opts.get('sp',0x8000)<0x80:opts['sp']+=0x40
        for which in (0,1):result.append((f'resident-{which}-{label}',dict(opts,which=which)))
    for index in range(256):
        result.append((f'index-{index}',dict(index=0xA500+index,which=index%2,record=(0x3100,0) if index%4 else (0,0),other=(0x3300,0) if index%3 else (0,0),flags=fm.FLAGS[index%len(fm.FLAGS)])))
    result.extend([
      ('first-clears-other-header',dict(record=(0x3100,0),other=(0x3100,0))),
      ('argument-on-heap-header',dict(ss=0x3000,sp=0x1018,record=(0x3101,8),root_writes=((DS,0x5E5E+8*8,struct.pack('<HH',0,0x3300)),(0x3300,8,bytes([1])),(0x3300,0x11,struct.pack('<HHH',16,0,0))))),
      ('resident-near-return-alias',dict(ss=0x3000,sp=0x1026,record=(0x3300,0),aux=(0x3100,0))),
      ('sp-max',dict(sp=0xFFFA,other=(0x3300,0))),
      ('resident-far-return-alias',dict(ss=0x3000,sp=0x102C,index=0,record=(0x3102,0),root_writes=((0x3000,0x1032,bytes(2)),))),
      ('push-ds-clears-tested-pointer',dict(ds=0,ss=0,sp=0x5E60,index=0,record=(0,0xF000),bp=1)),
    ])
    return result
def compare(label,m,c,dd,cc,f,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    system=struct.unpack_from('<H',m,physical(RESIDENT,r.SITES[0]+3))[0]
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,returning=None,root_return=False,stack=[],last=None,previous=None,heap_loops=0,resident_status=-1,inner=-1,site=0,inner_calls=0,calls=0,site_root=0,unexpected=None)
    visited,edges=set(),set()
    def stop(status,rs=None,inner=None):
        state['stop']=status
        if rs is not None:state['resident_status']=rs
        if inner is not None:state['inner']=inner
        u.emu_stop()
    def hook(emu,address,size,user):
        cs,ip=emu.reg_read(UC_X86_REG_CS),emu.reg_read(UC_X86_REG_IP)
        if state['root_return']:return stop(0)
        if state['returning']:
            expected,who,far=state['returning'];state['returning']=None
            if who=='resident':state['resident_status']=0
            if who=='freemem' and far:state['inner']=0
            if (cs,ip)!=expected:
                if who=='resident':return stop(3)
                return stop(2,3 if far else 2,None if far else fm.TRANSFER)
        if cs==system and ip==fm.RUNERROR_IP:return stop(2,2,fm.RUNERROR)
        who='caller' if cs==CS else 'resident' if cs==RESIDENT else 'freemem'
        if cs not in (CS,RESIDENT,system) or ip not in dd[who]:
            state['unexpected']=(cs,ip);return emu.emu_stop()
        here=(who,ip);prev=state['previous']
        if prev and prev[1] in cc[prev[0]]:edges.add((*prev,ip))
        if who=='freemem' and ip==fm.WALK_HEAD and state['last'] in {('freemem',a) for a in fm.WALK_JAS}:
            if state['heap_loops']==heap_cap:return stop(2,2,fm.LIMIT)
            state['heap_loops']+=1
        if who=='caller' and ip in SITES:
            state['site_root']=ip;state['calls']+=1;state['inner']=-1;state['site']=0;state['inner_calls']=0
        if who=='resident' and ip in r.SITES:
            state['site']=ip;state['inner_calls']+=1;state['heap_loops']=0
        visited.add(here);ins=dd[who][ip]
        if ins.mnemonic in ('call','lcall'):state['stack'].append(((cs,ip+ins.size),ins.mnemonic=='lcall'))
        elif ins.mnemonic in ('ret','retf'):
            if who=='caller' and ip==RETF:state['root_return']=True
            else:
                expected,far=state['stack'].pop();state['returning']=(expected,who,far)
        state['previous']=state['last']=here
    protected=[(physical(CS,ENTRY),physical(CS,END)),(physical(RESIDENT,r.ENTRY),physical(RESIDENT,r.END))]+[(physical(system,a),physical(system,b)) for a,b in fm.SPANS]
    def mirror(emu,access,address,size,value,user):
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);check(not any(a<=at<b for a,b in protected),f'code write {label}:{at:x}')
            emu.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror);u.emu_start(physical(CS,ENTRY),2*SIZE,count=3000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original escaped/stalled {label}: {state}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=Diagnostic();status=f(ram,SIZE,ctypes.byref(c),RESIDENT,system,heap_cap,ctypes.byref(d))
    check(status==state['stop'],f'status {label}: {status} != {state["stop"]}')
    actual=(d.resident_status,d.resident.freemem_status,d.resident.call_site,d.resident.calls,d.calls,d.call_site)
    want=tuple(state[k] for k in ('resident_status','inner','site','inner_calls','calls','site_root'))
    check(actual==want,f'diagnostic {label}: {actual} != {want}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        want=u.reg_read(reg)&65535;check(getattr(c,name)==want,f'{label} {name}: C {getattr(c,name):04X}, original {want:04X}')
    actual=bytes(ram);want=bytes(u.mem_read(0,SIZE))
    if actual!=want:
        diff=[i for i,(x,y) in enumerate(zip(actual,want)) if x!=y];raise ValueError(f'{label}: {len(diff)} RAM differences {[(hex(p),actual[p],want[p]) for p in diff[:8]]}')
    if label=='argument-on-heap-header':
        check(d.calls==2 and d.call_site==0x1A5 and ram[physical(c.ss,0x101C)]==8,'argument must reload changed index8')
    name={0:'return',2:'CALLEE',3:'TRANSFER',4:'LIMIT'}[status]
    if status==2:name+='_'+str(d.resident_status)+'_'+str(d.resident.freemem_status)
    return name,visited,edges,d.calls

def differential(body,resident,image,dd,cc,f):
    visited,edges,stops=set(),set(),{};calls=0;rng=random.Random(0x3CE014E)
    for label,opts in cases():
        opts=dict(opts);hc=opts.pop('cap',64);m,c=build(body,resident,image,rng,**opts)
        name,v,e,a=compare(label,m,c,dd,cc,f,hc);visited|=v;edges|=e;calls+=a;stops[name]=stops.get(name,0)+1
        if not label.startswith(('resident-0-random','resident-1-random','index-')):print(label,name,flush=True)
    wanted={(w,a) for w,t in dd.items() for a in t};wanted_edges={(w,a,b) for w,t in cc.items() for a,ends in t.items() for b in ends}
    missing=sorted(wanted-visited);missing_edges=sorted(wanted_edges-edges)
    check(not missing and not missing_edges,f'closure coverage incomplete: {missing}, {missing_edges}')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,masked_bits=0,caller_starts=sum(w=='caller' for w,a in visited),caller_total=len(dd['caller']),caller_outcomes=sum(w=='caller' for w,a,b in edges),caller_outcomes_total=2*len(cc['caller']),chain_starts=len(visited),chain_total=len(wanted),chain_outcomes=len(edges),chain_outcomes_total=len(wanted_edges),missing_starts=missing,missing_outcomes=missing_edges,resident_calls=calls)
    print('Original parity:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u3ce-014e-') as tmp:
        f=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        body,resident,image,dd,cc,meta=source();report=dict(target=meta)
        if a.unicorn:report['unicorn']=differential(body,resident,image,dd,cc,f)
        if a.native:
            from u3ce_bitmap_pair_release_014e_native import native_audit
            report['native']=native_audit(body,resident,image,f)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    import sys
    sys.modules['u3ce_bitmap_pair_release_014e_audit']=sys.modules[__name__]
    main()
