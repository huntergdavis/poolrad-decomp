#!/usr/bin/env python3
"""U3B9 +048F: unmasked original CPU14/1MiB, raw resident/FreeMem/FillChar.
Reuses accepted resident harness fixtures and loader mapping, not results.
"""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
import res0464_bitmap_release_2595_audit as r
from res0464_bitmap_release_2595_audit import Cpu,ROOT,SIZE,REGISTERS,physical,put16,check
fm=r.fm
CHECKER_ROWS=('OVL_03B9_048F',)
CS,RESIDENT,DS,SS,SYSTEM=0x4000,r.CS,r.DS,r.SS,r.SYSTEM
ENTRY,END,RETF=0x48F,0x4F8,0x4F5
SOURCES=['overlay_u3b9_bitmap_set_release_048f_cpu.c',*r.SOURCES,'system_runtime_leaves_cpu.c']
class Diagnostic(ctypes.Structure):
    _fields_=[('resident_status',ctypes.c_int),('resident',r.Diagnostic),('release_calls',ctypes.c_uint),('fill_calls',ctypes.c_uint)]
def source():
    resident,image,rd,rc,fd,fc,rm=r.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);u=next(u for u in units if u['stub_para']==0x3B9)
    body=ovr[u['ovr_offset']+ENTRY:u['ovr_offset']+END];dis=Cs(CS_ARCH_X86,CS_MODE_16)
    ins=list(dis.disasm(body,ENTRY));check(len(body)==105 and len(ins)==41 and sha256(body).hexdigest()=='08c8d4e1d889b5373c9d85613dd0530f4485892ee29681f5a14efcbfb203dcde','048F extent/hash')
    check((ins[-1].address,ins[-1].mnemonic,ins[-1].op_str)==(RETF,'retf','4'),'048F RETF4')
    check(u['entries'][3]==ENTRY and u['memory_segment']==0x3D0,'public entry3')
    check([(i.address,i.op_str) for i in ins if 'call' in i.mnemonic]==[(0x4CA,'0x464, 0x2595'),(0x4E3,'0xaf8, 0x16a8')],'only static raw dependencies')
    check(all(s+3 in u['fixups'] for s in (0x4CA,0x4E3)),'outgoing fixups')
    incoming=[]
    for unit in units:
        for fix in unit['fixups']:
            b=ovr[unit['ovr_offset']+fix-3:unit['ovr_offset']+fix+2]
            if len(b)==5 and b[0]==0x9A and struct.unpack_from('<HH',b,1)==(0x2F,0x3D0):incoming.append((unit['stub_para'],fix-3))
    near=list(dis.disasm(ovr[u['ovr_offset']+0xDE:u['ovr_offset']+0xE2],0xDE))
    check([(i.mnemonic,i.op_str) for i in near]==[('push','cs'),('call','0x48f')],'local far-frame near caller')
    # Public overlay entry stubs are five bytes each: entry3 => 002F after the 20h header.
    check(incoming==[(0x33,0x1E3C),(0x68,0x1080),(0x75,0x109)],f'incoming public calls {incoming}')
    fill=image[physical(SYSTEM,0x16A8):physical(SYSTEM,0x16BC)]
    check(sha256(fill).hexdigest()=='f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7','FillChar identity')
    dd={'caller':{i.address:i for i in ins},'resident':rd,'freemem':fd,'fill':{i.address:i for i in dis.disasm(fill,0x16A8)}}
    cc={w:{a:(a+i.size,int(i.op_str,16)) for a,i in table.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'} for w,table in dd.items()}
    meta=dict(target=CHECKER_ROWS[0],span=[ENTRY,END],file_start=u['ovr_offset']+ENTRY,bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),queue_stale_bytes=106,trimmed_trailing_bytes=1,incoming=incoming,near_caller=0xDF,resident=rm,fill_sha256=sha256(fill).hexdigest())
    return body,resident,image,dd,cc,meta

def library(directory):
    p=directory/'setrelease.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u3b9_bitmap_set_release_048f_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_uint16,ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    return f

def build(body,resident,image,rng,*,count=1,set_pointer=(DS,0x6000),populate=False,root_writes=(),**opts):
    opts=dict(opts);writes=opts.pop('writes',());ss=opts.get('ss',SS);sp=opts.get('sp',0x8000);system=opts.get('system',SYSTEM)
    slot=(set_pointer[0],(set_pointer[1]+6)&65535)
    m,c=r.build(resident,image,rng,slot=slot,**opts)
    m[physical(*set_pointer)]=count
    for n in range(2,128):
        rec=(0x3100+4*n,0) if populate else (0,0)
        r.pair(m,set_pointer[0],(set_pointer[1]+8*n-2)&65535,rec)
        if populate:
            m[physical(rec[0],8)]=1;put16(m,rec[0],0x11,16);r.pair(m,rec[0],0x13,(0,0))
    for j,v in enumerate((0x7000,0x9001,set_pointer[1],set_pointer[0])):put16(m,ss,sp+2*j,v)
    at=physical(CS,ENTRY);m[at:at+len(body)]=body;put16(m,CS,0x4CD,RESIDENT);put16(m,CS,0x4E6,system)
    if system!=SYSTEM:
        at=physical(system,0x16A8);m[at:at+20]=image[physical(SYSTEM,0x16A8):physical(SYSTEM,0x16BC)]
    for seg,off,blob in (*writes,*root_writes):
        for j,v in enumerate(blob):m[(physical(seg,off)+j)&(SIZE-1)]=v
    c.cs=CS;c.ip=ENTRY
    return m,c

def cases():
    result=[]
    for label,opts in r.cases():
        if 'slot' in opts or label=='callee-return-alias':continue
        opts=dict(opts)
        if opts.get('sp',0x8000)<0x80:opts['sp']+=0x40
        result.append(('resident-'+label,opts))
    for n in range(256):result.append((f'count-{n}',dict(count=n,populate=n%7==0,cap=512,flags=fm.FLAGS[n%len(fm.FLAGS)])))
    result.extend([
      ('release-limit-zero',dict(count=2,release_cap=0)),('release-limit-one',dict(count=2,release_cap=1)),
      ('all127',dict(count=127,populate=True,cap=512)),('set-wrap',dict(count=2,set_pointer=(DS,0xFFFA))),
      ('set-a20',dict(count=2,set_pointer=(0xFFFF,0x500))),
      ('set-is-counter',dict(count=1,set_pointer=(DS,0x70C7))),
      ('set-over-counter',dict(count=1,set_pointer=(DS,0x70C1),release_cap=3)),
      ('clear-return-frame',dict(count=0,set_pointer=(SS,0x7FE4))),
      ('resident-near-return-alias',dict(ss=0x3000,sp=0x102A,record=(0x3300,0),aux=(0x3100,0))),
      ('clear-saved-bp',dict(count=0,set_pointer=(SS,0x7FFE))),
      ('clear-local-bound',dict(count=0,set_pointer=(SS,0x7FF9))),
      ('heap-over-live-counter',dict(count=2,record=(DS,0x70C0),org=(DS,0),top=(DS,0xF000),release_cap=4)),
    ])
    return result

def compare(label,m,c,dd,cc,f,heap_cap=64,release_cap=256):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    system=struct.unpack_from('<H',m,physical(CS,0x4E6))[0]
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,returning=None,root_return=False,stack=[],last=None,previous=None,heap_loops=0,resident_status=-1,inner=-1,site=0,inner_calls=0,calls=0,fills=0,unexpected=None)
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
                if who in ('resident','fill'):return stop(3)
                return stop(2,3 if far else 2,None if far else fm.TRANSFER)
        if cs==system and ip==fm.RUNERROR_IP:return stop(2,2,fm.RUNERROR)
        who='caller' if cs==CS else 'resident' if cs==RESIDENT else 'fill' if cs==system and ip in dd['fill'] else 'freemem'
        if cs not in (CS,RESIDENT,system) or ip not in dd[who]:
            state['unexpected']=(cs,ip);return emu.emu_stop()
        here=(who,ip);prev=state['previous']
        if prev and prev[1] in cc[prev[0]]:edges.add((*prev,ip))
        if who=='freemem' and ip==fm.WALK_HEAD and state['last'] in {('freemem',a) for a in fm.WALK_JAS}:
            if state['heap_loops']==heap_cap:return stop(2,2,fm.LIMIT)
            state['heap_loops']+=1
        if who=='caller' and ip==0x4CA:
            if state['calls']==release_cap:return stop(4)
            state['calls']+=1;state['inner']=-1;state['site']=0;state['inner_calls']=0
        if who=='caller' and ip==0x4E3:state['fills']+=1
        if who=='resident' and ip in r.SITES:
            state['site']=ip;state['inner_calls']+=1;state['heap_loops']=0
        visited.add(here);ins=dd[who][ip]
        if ins.mnemonic in ('call','lcall'):state['stack'].append(((cs,ip+ins.size),ins.mnemonic=='lcall'))
        elif ins.mnemonic in ('ret','retf'):
            if who=='caller' and ip==RETF:state['root_return']=True
            else:
                expected,far=state['stack'].pop();state['returning']=(expected,who,far)
        state['previous']=state['last']=here
    protected=[(physical(CS,ENTRY),physical(CS,END)),(physical(RESIDENT,r.ENTRY),physical(RESIDENT,r.END)),(physical(system,0x16A8),physical(system,0x16BC))]+[(physical(system,a),physical(system,b)) for a,b in fm.SPANS]
    def mirror(emu,access,address,size,value,user):
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);check(not any(a<=at<b for a,b in protected),f'code write {label}:{at:x}')
            emu.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror);u.emu_start(physical(CS,ENTRY),2*SIZE,count=3000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original escaped/stalled {label}: {state}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=Diagnostic();status=f(ram,SIZE,ctypes.byref(c),RESIDENT,system,release_cap,heap_cap,ctypes.byref(d))
    check(status==state['stop'],f'status {label}: {status} != {state["stop"]}')
    actual=(d.resident_status,d.resident.freemem_status,d.resident.call_site,d.resident.calls,d.release_calls,d.fill_calls)
    want=tuple(state[k] for k in ('resident_status','inner','site','inner_calls','calls','fills'))
    check(actual==want,f'diagnostic {label}: {actual} != {want}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        want=u.reg_read(reg)&65535;check(getattr(c,name)==want,f'{label} {name}: C {getattr(c,name):04X}, original {want:04X}')
    actual=bytes(ram);want=bytes(u.mem_read(0,SIZE))
    if actual!=want:
        diff=[i for i,(x,y) in enumerate(zip(actual,want)) if x!=y];raise ValueError(f'{label}: {len(diff)} RAM differences {[(hex(p),actual[p],want[p]) for p in diff[:8]]}')
    name={0:'return',2:'CALLEE',3:'TRANSFER',4:'LIMIT'}[status]
    if status==2:name+='_'+str(d.resident_status)+'_'+str(d.resident.freemem_status)
    return name,visited,edges,d.release_calls,d.fill_calls

def differential(body,resident,image,dd,cc,f):
    visited,edges,stops=set(),set(),{};calls=fills=0;rng=random.Random(0x3B9048F)
    for label,opts in cases():
        opts=dict(opts);hc=opts.pop('cap',64);rc=opts.pop('release_cap',256)
        m,c=build(body,resident,image,rng,**opts);name,v,e,a,b=compare(label,m,c,dd,cc,f,hc,rc)
        visited|=v;edges|=e;calls+=a;fills+=b;stops[name]=stops.get(name,0)+1
        if not label.startswith(('resident-random','count-')):print(label,name,flush=True)
    wanted={(w,a) for w,t in dd.items() for a in t};wanted_edges={(w,a,b) for w,t in cc.items() for a,ends in t.items() for b in ends}
    missing=sorted(wanted-visited);missing_edges=sorted(wanted_edges-edges)
    check(not missing and not missing_edges,f'closure coverage incomplete: {missing}, {missing_edges}')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,masked_bits=0,caller_starts=sum(w=='caller' for w,a in visited),caller_total=len(dd['caller']),caller_outcomes=sum(w=='caller' for w,a,b in edges),caller_outcomes_total=2*len(cc['caller']),chain_starts=len(visited),chain_total=len(wanted),chain_outcomes=len(edges),chain_outcomes_total=len(wanted_edges),missing_starts=missing,missing_outcomes=missing_edges,release_calls=calls,fill_calls=fills)
    print('Original parity:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u3b9-048f-') as tmp:
        f=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        body,resident,image,dd,cc,meta=source();report=dict(target=meta)
        if a.unicorn:report['unicorn']=differential(body,resident,image,dd,cc,f)
        if a.native:
            from u3b9_bitmap_set_release_048f_native import native_audit
            report['native']=native_audit(body,resident,image,f)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    import sys
    sys.modules['u3b9_bitmap_set_release_048f_audit']=sys.modules[__name__]
    main()
