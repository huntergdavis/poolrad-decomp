#!/usr/bin/env python3
"""U9E +0026 partial-chain original/C oracle; no handler effects invented."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
import system_freemem_0254_audit as fm
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
check=fm.check
CS,DS,SS,ENTRY=0x8800,fm.DS,fm.SS,0x26
class Limits(ctypes.Structure):
    _fields_=[('max_list_loopbacks',ctypes.c_uint),('max_heap_loopbacks',ctypes.c_uint)]
class Diagnostic(ctypes.Structure):
    _fields_=[('freemem_status',ctypes.c_int),('handler_offset',ctypes.c_uint16),('handler_segment',ctypes.c_uint16)]
SOURCES=['overlay_u9e_list_release_0026_cpu.c','system_freemem_0254_cpu.c']

def source():
    image,free,_,spans=fm.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest()==OVR_SHA,'overlay media hash')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr)
    u=next(u for u in units if u['stub_para']==0x9E);code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    md=Cs(CS_ARCH_X86,CS_MODE_16);body=code[0x26:0x140];ins=list(md.disasm(body,0x26));prefix=list(md.disasm(code[:0x26],0))
    check(sha256(body).hexdigest()=='12578dfdfb77080908623cf336552b20354b56128580f377b3d26ad2caf1df12','body hash')
    check(sha256(code[:0x26]).hexdigest()=='1572342d9be429f7a7b7f6b015b2a609726b4d5580e34235fda31eb2a43b9217','dispatcher hash')
    check(len(ins)==94 and ins[-1].address==0x13D and ins[-1].op_str=='0xa','body extent')
    check(u['entries'][2]==0x26 and 0x138 in u['fixups'],'entry/fixup')
    check([(i.address,i.op_str) for i in ins if i.mnemonic in ('call','lcall')]==[(0xA1,'0'),(0x135,'0xaf8, 0x254')],'call targets')
    check(prefix[-3].address==0x1E and prefix[-3].mnemonic=='lcall','indirect call boundary')
    decoded={(fm.SYSTEM_CS,a):i for a,i in free.items()};decoded.update({(CS,i.address):i for i in ins+prefix})
    target=dict(bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),file_start=u['ovr_offset']+0x26,file_end=u['ovr_offset']+0x140,queue_bytes=314,trailing_data_bytes=32,dispatcher_bytes=38,dispatcher_instructions=16,freemem_spans=spans)
    return image,code,decoded,target

def library(directory):
    p=directory/'release.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u9e_list_release_0026_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;return f

def build(image,code,rng,*,nodes=((0x3100,0),(0x3120,0),(0x3140,0)),selected=1,key=0x42,keys=None,active=0,record=(0x5000,0x100),cycle=None,search=False,writes=(),sp=0x8000,ss=SS,**options):
    m,c,high=fm.build(image,rng,sp=sp,ss=ss,**options)
    at=physical(CS,0);m[at:at+len(code)]=code
    for j,(seg,off) in enumerate(nodes):
        m[physical(seg,off)]=keys[j] if keys is not None else (key if j==selected else (key+1)&255)
        m[physical(seg,(off+4)&65535)]=active
        nxt=nodes[j+1] if j+1<len(nodes) else (nodes[cycle] if cycle is not None else (0,0))
        put16(m,seg,(off+5)&65535,nxt[1]);put16(m,seg,(off+7)&65535,nxt[0])
    head=nodes[0] if nodes else (0,0);put16(m,record[0],(record[1]+0x7F)&65535,head[1]);put16(m,record[0],(record[1]+0x81)&65535,head[0])
    item=(0,0) if search or not nodes else nodes[selected]
    for j,v in enumerate((0x7000,0x9001,item[1],item[0],0xA500|key,record[1],record[0])):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    c.cs=CS;c.ip=ENTRY;return m,c,high

def compare(label,memory,cpu,high,decoded,function,list_cap=64,heap_cap=64):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,n))
    state=dict(stop=None,reason=None,returned=False,returning=None,previous=None,calls=[],list_loops=0,heap_loops=0)
    visited,edges=set(),set()
    def halt(status,reason):state['stop']=status;state['reason']=reason;uc.emu_stop()
    def hook(em,address,size,user):
        del em,size,user
        here=(uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP));cs,ip=here
        if state['returned']:return halt(0,'return')
        if state['returning'] is not None:
            expected,direct=state['returning'];state['returning']=None
            if here!=expected:return halt(4 if direct else 3,'changed direct return' if direct else 'changed nested return')
        if here==(fm.SYSTEM_CS,0xE2):return halt(3,'RunError204')
        check(here in decoded,f'original escape {here}: {label}')
        if cs==CS and ip in (0xA1,0x135):
            expected=bytes.fromhex('e85cff') if ip==0xA1 else bytes.fromhex('9a5402')
            if bytes(uc.mem_read(address,3))!=expected:return halt(2,'binding')
        if here==(CS,0x1E):return halt(5,'handler pending')
        if (state['previous'],here) in (((CS,0x76),(CS,0x52)),((CS,0x113),(CS,0xE5))):
            state['list_loops']+=1
            if state['list_loops']>list_cap:return halt(6,'list budget')
        if here==(fm.SYSTEM_CS,0x40F) and state['previous'] in [(fm.SYSTEM_CS,j) for j in fm.WALK_JAS]:
            state['heap_loops']+=1
            if state['heap_loops']>heap_cap:return halt(3,'heap budget')
        ins=decoded[here];visited.add(here);prev=decoded.get(state['previous'])
        if prev and prev.mnemonic.startswith('j') and prev.mnemonic!='jmp':edges.add((state['previous'],here))
        if ins.mnemonic in ('call','lcall'):state['calls'].append(((cs,ip+ins.size),cs==CS))
        elif ins.mnemonic in ('ret','retf'):
            if here==(CS,0x13D):state['returned']=True
            else:state['returning']=state['calls'].pop()
        state['previous']=here
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);em.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    uc.hook_add(UC_HOOK_CODE,hook)
    if high:uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
    uc.emu_start(physical(CS,ENTRY),2*SIZE,count=2000000)
    check(state['stop'] is not None,f'original timeout {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);d=Diagnostic();l=Limits(list_cap,heap_cap)
    status=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(l),ctypes.byref(d))
    check(status==state['stop'],f'status {status}/{d.freemem_status}, expected {state}: {label}')
    actual=bytes(ram);want=bytes(uc.mem_read(0,SIZE))
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'RAM {label}: {len(diff)} differences {diff[:8]}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        actual_word,want_word=getattr(cpu,n),uc.reg_read(r)&65535
        if n=='flags' and status==5:
            # SHL AF is undefined: measured DOSBox-X normal/386=1, Unicorn=0.
            check(actual_word&0x10 and not want_word&0x10,'shift AF profile')
            actual_word&=~0x10;want_word&=~0x10
        check(actual_word==want_word,f'{label} {n}: C={getattr(cpu,n):04x} original={uc.reg_read(r)&65535:04x}')
    if status==5:
        p=physical(cpu.ds,(cpu.di+0x6828)&65535);check((d.handler_offset,d.handler_segment)==struct.unpack_from('<HH',want,p),'pending pointer')
        check(d.freemem_status==-1,'pending FreeMem was called')
    if state['reason']=='RunError204':check(d.freemem_status==2,'RunError diagnostic')
    if state['reason']=='heap budget':check(d.freemem_status==4,'heap budget diagnostic')
    return status,state['reason'],visited,edges

def cases():
    specs=[]
    for n in range(256):
        nodes=tuple((0x3100+j*0x20,8 if n&4 else 0) for j in range(1+n%5))
        specs.append((f'random-{n}',dict(nodes=nodes,selected=n%len(nodes),search=bool(n&1),key=n,active=1 if n%4==0 else 0,flags=fm.FLAGS[n%len(fm.FLAGS)],blocks=tuple((0x3000+j*0x10,0,1,0) for j in range(n%5)))))
    specs += [('empty',dict(nodes=())),('not-found',dict(search=True,keys=(1,2,3))),
        ('head',dict(selected=0)),('tail',dict(selected=2)),('search-tail',dict(selected=2,search=True)),
        ('same-segment',dict(nodes=((0x3100,0),(0x3100,0x20),(0x3100,0x40)),selected=2)),
        ('missing-predecessor',dict(writes=((SS,0x8004,struct.pack('<HH',0,0x3200)),(0x3200,4,bytes(5))))),
        ('no-head',dict(writes=((0x5000,0x17F,bytes(4)),))),
        ('search-limit',dict(search=True,selected=2,list_cap=0)),
        ('search-cycle',dict(search=True,keys=(1,2,3),cycle=0,list_cap=4)),
        ('walk-limit',dict(selected=2,list_cap=0)),
        ('walk-cycle',dict(writes=((SS,0x8004,struct.pack('<HH',0,0x3200)),(0x3200,4,bytes(5))),cycle=0,list_cap=4)),
        ('heap-limit',dict(blocks=((0x3001,0,1,0),(0x3003,0,1,0)),heap_cap=0)),
        ('duplicate',dict(blocks=((0x3120,0,1,0),))),
        ('bad-pointer',dict(nodes=((0x3100,1),),selected=0)),
        ('merge-both',dict(nodes=((0x3100,0),),selected=0,blocks=((0x30FF,0,1,0),(0x3101,0,4,0)))),
        ('shrink-top',dict(nodes=((0x33FF,0),),selected=0)),
        ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),
        ('record-wrap',dict(record=(0x5000,0xFF80))),
        ('record-is-local',dict(record=(SS,0x7F77))),
        ('record-is-frame',dict(record=(SS,0x7F81))),
        ('code-free',dict(writes=((CS,0x135,b'\x90'),))),
        ('code-dispatch',dict(active=1,writes=((CS,0xA1,b'\x90'),))),
        ('skip-bad-bindings',dict(nodes=(),writes=((CS,0x135,b'\x90'),(CS,0xA1,b'\x90')))),
    ]
    for sp in (0x40,0x41,0x8001,0xFFF0,0xFFF2):specs.append((f'sp-{sp:x}',dict(sp=sp)))
    for key in (0,1,88,139,140,255):specs.append((f'dispatch-{key}',dict(key=key,active=1)))
    return specs

def differential(image,code,decoded,function):
    visited,edges,stops,reasons=set(),set(),{},{};rng=random.Random(0x9E0026)
    for label,options in cases():
        caps={k:options.pop(k) for k in list(options) if k.endswith('_cap')}
        m,c,h=build(image,code,rng,**options);status,reason,v,e=compare(label,m,c,h,decoded,function,**caps)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;reasons[reason]=reasons.get(reason,0)+1
        if not label.startswith('random-'):print(label,status,reason,flush=True)
    starts={p for p in decoded if p[0]==CS and p[1]>=0x26};ce={e for e in edges if e[0] in starts}
    expected={(p,(CS,b)) for p,i in decoded.items() if p in starts and i.mnemonic.startswith('j') and i.mnemonic!='jmp' for b in (i.address+i.size,int(i.op_str,16))}
    check(starts<=visited and expected<=ce,f'caller coverage missing {starts-visited}, edges {expected-ce}')
    report=dict(cases=sum(stops.values()),stops=stops,reasons=reasons,caller_starts=len(starts),caller_outcomes=len(ce),dispatcher_prefix_starts=len([p for p in visited if p[0]==CS and p[1]<0x26]),chain_starts=len(visited),chain_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE,undefined_shift_af_exceptions=stops.get('5',0))
    print('Original partial chain:',report,flush=True);return report

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    image,code,decoded,target=source();report=dict(target=target)
    with tempfile.TemporaryDirectory(prefix='u9e-0026-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,code,decoded,f)
        if a.native:
            from u9e_list_release_0026_native import native_audit
            report['native']=native_audit(image,code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
