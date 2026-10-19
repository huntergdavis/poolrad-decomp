#!/usr/bin/env python3
"""Original U1C +2E7D CPU/RAM/OUT/VRAM-operand parity; native EGA oracle.
Reuses merged U25 and glyph hash/fixture scaffolding. Media stays private.
"""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
import u25_loader_0000_audit as loader
import u25_string_cores_audit as strings
import u25_load_word_0e0c_audit as loadword
import res0709_glyph_01df_cpu_audit as glyph
from res0709_glyph_01df_cpu_audit import Ega,Bus,Env,OUT_FN,IN_FN,WRITE_FN
check=glyph.check
CS,U25,GLYPH,SYS,STUB=0x9000,0x4000,0x8800,0xAF8,0x2B
DS,SS,FONT=0x1800,0x2800,0x7000
SEGMENTS={'root':CS,'u25':U25,'glyph':GLYPH,'sys':SYS}
ENTRY,END,RETF=0x2E7D,0x2F2D,0x2F2C
SITES={0x2E8F:(0x16A8,None,0),0x2E9C:(0x2A,0,1),0x2EA9:(0x4D,0x6AF,2),0x2EE9:(0x57,0xE0C,3),0x2F0D:(0x1DF,None,4)}
SOURCES=('overlay_u1c_text_2e7d_cpu','overlay_u25_loader_0000','overlay_u25_string_cores','overlay_u25_load_word_0e0c','overlay_u25_party_index_0756','system_runtime_leaves_cpu','resident_0709_glyph_01df_cpu','memory_bus','ega_hw')
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
class Limits(ctypes.Structure):
    _fields_=[('system_cs',ctypes.c_uint16)]+[(n,ctypes.c_size_t) for n in ('max_operands','max_string_iterations','max_loopbacks','max_glyph_loopbacks')]
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('callee','status','glyph_callee','glyph_status')]
def source():
    exe_path=ROOT/'resources/DOS/POOLRAD/START.EXE';ovr_path=ROOT/'resources/DOS/POOLRAD/GAME.OVR'
    bodies,groups=loader.source_audit(exe_path,ovr_path)
    data=ovr_path.read_bytes();exe=exe_path.read_bytes();image=bytes(unpack(exe)[0])
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(data).hexdigest()==OVR_SHA,'media')
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],data)}
    u=units[0x1C];body=data[u['ovr_offset']+ENTRY:u['ovr_offset']+END]
    md=Cs(CS_ARCH_X86,CS_MODE_16);ins=list(md.disasm(body,ENTRY))
    check(sha256(body).hexdigest()=='4c04ef11f1280fba187b51494683c9bda992c5ce66e4c57aa003fd2796d59cb0','root hash')
    check(len(body)==176 and len(ins)==64 and (ins[-1].address,ins[-1].mnemonic)==(RETF,'retf'),'caller extent')
    check([(i.address,i.op_str) for i in ins if 'call' in i.mnemonic]==[(0x2E8F,'0xaf8, 0x16a8'),(0x2E9C,'0x2b, 0x2a'),(0x2EA9,'0x2b, 0x4d'),(0x2EE9,'0x2b, 0x57'),(0x2F0D,'0x709, 0x1df')],'caller calls')
    check(all(s+3 in u['fixups'] for s in SITES),'caller fixups')
    bodies['root',ENTRY]=body;groups['root',ENTRY]=ins
    lb,_=loadword.source(ovr_path)
    for a,b in lb.items():bodies['u25',a]=b;groups['u25',a]=list(md.disasm(b,a))
    gb,gd,gm=glyph.source()
    for (kind,a),b in gb.items():
        k='glyph' if kind=='caller' else kind;bodies[k,a]=b;groups[k,a]=list(md.disasm(b,a))
    fill=image[physical(SYS,0x16A8):physical(SYS,0x16BC)]
    check(sha256(fill).hexdigest()=='f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7','FillChar')
    bodies['sys',0x16A8]=fill;groups['sys',0x16A8]=list(md.disasm(fill,0x16A8))
    decoded={(kind,i.address):i for (kind,a),group in groups.items() for i in group}
    meta={f'{kind}:{a:04X}':dict(bytes=len(b),instructions=len(groups[kind,a]),sha256=sha256(b).hexdigest()) for (kind,a),b in bodies.items()}
    meta['target']=dict(file_offset=u['ovr_offset']+ENTRY,entry_slots=[i for i,a in enumerate(u['entries']) if a==ENTRY])
    print('Caller',meta['root:2E7D'],meta['target'],flush=True)
    return bodies,decoded,meta
def library(directory):
    p=directory/'u1ctext.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(p)],check=True)
    lib=ctypes.CDLL(str(p));f=lib.pr_u1c_text_2e7d_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,stub=STUB):
    for (kind,a),b in bodies.items():
        p=physical(SEGMENTS[kind],a);m[p:p+len(b)]=b
    put16(m,CS,0x2F10,GLYPH)
    for off,ip in ((0x2A,0),(0x4D,0x6AF),(0x57,0xE0C)):m[physical(stub,off):physical(stub,off)+5]=struct.pack('<BHH',0xEA,ip,U25)
    for s,(off,ip,_) in SITES.items():
        if ip is not None:put16(m,CS,s+3,stub)
def pointer(m,s,o,seg,off):put16(m,s,o,off);put16(m,s,(o+2)&65535,seg)
def build(bodies,*,text=(64,65,0),address=0x4900,tag=2,mode=1,x=2,y=3,wrap=0,sp=0x8000,ss=SS,flags=2,fontseg=FONT,fontoff=0x1000,pattern=0,pages=(0xA000,0xA200),writes=(),stub=STUB,stream=None,background=None):
    # Synthetic glyph tables/font, then the complete original closure.
    gb={('caller' if k=='glyph' else k,a):b for (k,a),b in bodies.items() if k in ('glyph','sys')}
    m,c=glyph.build(gb,ds=DS,ss=ss,sp=sp,flags=flags,fontseg=fontseg,fontoff=fontoff,pattern=pattern,pages=pages,mode=mode)
    if background is not None:
        # Preserve initialized data/font/tables while randomizing unused memory.
        old=m;m=bytearray(background)
        for seg,off,n in ((DS,0,65536),(FONT,0,65536)):
            p=physical(seg,off);m[p:p+n]=old[p:p+n]
    install(m,bodies,stub)
    for off,seg in ((0x49D2,0x6000),(0x49D6,0x6100),(0x49DA,0x6200),(0x49DE,0x6300),(0x5D92,0x6400)):pointer(m,DS,off,seg,0x1000)
    pointer(m,DS,0x5D96,0,0);put16(m,DS,0x49ED,0x9900)
    operand=bytes((tag,address&255,(address>>8)&255)) if tag in (1,2,3,0x81) else bytes((tag,address&255))
    if stream is not None:operand=bytes(stream)
    p=physical(0x6300,0x1001);m[p:p+len(operand)+1]=operand+b'\x7e'
    for j,v in enumerate(text):put16(m,0x6000,(0x1000+2*j)&65535,v)
    # Deterministic fields for unsupported-address / active-character tests.
    for off in (0x5E24,0x5E25,0x84DE,0x84DF,0x84E0,0x84E2):m[physical(DS,off)]=0
    m[physical(DS,0x5E24)]=x;m[physical(DS,0x5E25)]=y;m[physical(DS,0x84DE)]=wrap
    put16(m,ss,sp,0x7000);put16(m,ss,(sp+2)&65535,0x9001)
    c.cs=CS;c.ip=ENTRY;c.ax=0x1234;c.bp=0x7777
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[(physical(seg,off)+j)&(SIZE-1)]=v
    return m,c
def run_c(lib,m,c,*,outer=512,glyph_limit=65536,operands=64,strings_limit=4000,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    limits=Limits(SYS,operands,strings_limit,outer,glyph_limit);d=Diagnostic()
    status=lib.pr_u1c_text_2e7d_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def compare(name,state,bodies,decoded,lib,*,outer=512,glyph_limit=65536,operands=64,strings_limit=4000,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,callee=-1,status=-1,glyph_callee=-1,glyph_status=-1,
        outer=0,glyph_loops=0,headers=0,string_loops=0,previous=None)
    calls=[];seen=set();edges=set();events=[];writes=[]
    kinds={v:k for k,v in SEGMENTS.items()}
    stubseg=struct.unpack_from('<H',m,physical(CS,0x2E9F))[0]
    md=Cs(CS_ARCH_X86,CS_MODE_16)
    def stop(v):st['stop']=v;uc.emu_stop()
    def callee_stop(status):st['status']=status;stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'stub' if cs==stubseg else '?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,owner=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),owner)
                if owner=='root':return stop(4)
                return callee_stop(4 if st['callee']==1 else 2 if st['callee']==3 else 5)
        if key[0]=='stub':
            check(ip in (0x2A,0x4D,0x57),f'stub escape {name}')
            st['previous']=None
            return
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key]
        st['executing_kind']=key[0]
        previous=st['previous']
        if previous is not None:edges.add((previous,key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if key[0]=='root' and ip in SITES:
            off,target,which=SITES[ip];raw=bytes(uc.mem_read(address,5))
            if raw[:3]!=struct.pack('<BH',0x9A,off):return stop(2)
            if target is not None:
                seg=struct.unpack_from('<H',raw,3)[0]
                if bytes(uc.mem_read(physical(seg,off),3))!=struct.pack('<BH',0xEA,target):return stop(2)
            st['callee']=which;st['status']=0
            if which==1:st['headers']=0
            if which==4:st['glyph_callee']=st['glyph_status']=-1;st['glyph_loops']=0
        if key==('root',0x2F23) and uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40:
            if st['outer']==outer:return stop(5)
            st['outer']+=1
        if key==('u25',0x22) and st['callee']==1:
            if st['headers']==operands:return callee_stop(2)
            st['headers']+=1
        if key[0]=='u25' and ip in (0x1153,0x12FB):st['string_loops']=0
        if key[0]=='u25' and ip in strings.LOOPS and st['callee']==1:
            if st['string_loops']==strings_limit:return callee_stop(3)
            st['string_loops']+=1
        if key==('glyph',0x207):
            if bytes(uc.mem_read(address,3))!=bytes.fromhex('9a2a18'):return callee_stop(2)
            st['glyph_callee']=st['glyph_status']=0
        if key[0]=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40):
            if st['glyph_loops']==glyph_limit:return callee_stop(6)
            st['glyph_loops']+=1
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return callee_stop(4)
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):calls.append(((cs,ip+ins.size),key[0]))
        if ins.mnemonic in ('ret','retf'):
            if key==('root',RETF):st['root_return']=True
            else:st['ret']=True
    def out(em,port,size,value,user):events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)))
    protected=[(physical(SEGMENTS[k],a),physical(SEGMENTS[k],a)+len(b)) for (k,a),b in bodies.items()]
    def memory(em,access,address,size,value,user):
        kind=st.get('executing_kind')
        for j in range(size):
            at=(address+j)&(SIZE-1)
            if 0xA0000<=at<0xB0000:
                check(kind=='glyph',f'flat-RAM callee accessed EGA aperture {name}')
                if access==17:writes.append((at-0xA0000,(value>>(8*j))&255))
            if access==17:check(not any(a<=at<b for a,b in protected),f'code mutation {name}: {at:x}')
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT)
    uc.hook_add(UC_HOOK_MEM_WRITE,memory)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=4000000)
    check(st['stop'] is not None,f'instruction ceiling {name}')
    actual=Cpu.from_buffer_copy(c)
    status,ram,mine,stores,ega,d=run_c(lib,m,actual,outer=outer,glyph_limit=glyph_limit,operands=operands,strings_limit=strings_limit,refuse=refuse)
    check(status==st['stop'],f'status {name}: {status}/{st}')
    check(tuple(getattr(d,k) for k in ('callee','status','glyph_callee','glyph_status'))==tuple(st[k] for k in ('callee','status','glyph_callee','glyph_status')),f'diagnostic {name}: {list(bytes(d))}/{st}')
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(reg)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n}: C {getattr(actual,n):04X}, original {want:04X}')
    check(mine==events,f'OUTs {name}: {len(mine)}/{len(events)}')
    check(stores==writes,f'VRAM operands {name}: {len(stores)}/{len(writes)}')
    expected=bytes(backing)
    a=ram[:0xA0000]+ram[0xB0000:];b=expected[:0xA0000]+expected[0xB0000:]
    if a!=b:
        diff=[(hex(i),x,y) for i,(x,y) in enumerate(zip(a,b)) if x!=y]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    return status,seen,edges,len(events),len(writes)
def cases():
    result=[('empty',dict(text=(0,))),('ff-terminator',dict(text=(255,))),('word-wrap',dict(text=(65535,))),
        ('two',{}),('one',dict(text=(64,0))),('reset-position',dict(wrap=1,y=14,x=20)),
        ('no-reset-bound',dict(wrap=1,y=13)),('wrap-y',dict(y=255)),('wrap-x',dict(x=255)),
        ('high-words',dict(text=(0xAB40,0xCC41,0))),('glyph-low-bound',dict(text=(1,0))),
        ('no-font',dict(fontseg=0,fontoff=0)),('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
        ('relocated-stub',dict(stub=0x4500)),('outer-limit0',dict(outer=0)),
        ('outer-limit1',dict(outer=1)),('glyph-limit0',dict(glyph_limit=0)),
        ('glyph-limit1',dict(glyph_limit=1)),('port0',dict(refuse=0)),('port4',dict(refuse=4)),
        ('glyph-code',dict(writes=((GLYPH,0x207,b'\x90'),))),
        ('string-limit',dict(tag=0x81,strings_limit=1)),
        ('packed-string-limit',dict(stream=(0x80,65,66,0),strings_limit=1)),
    ]
    result += [(f'cursor-stack-{sp:x}',dict(ss=DS,sp=sp)) for sp in (0x84C8,0x84DE,0x5E26,0x5E2C)]
    result.append(('byte-index-wrap',dict(text=(64,)*256,mode=3,outer=257)))
    for mode in (0,1,2,3):
        for x,y in ((0,0),(38,24),(39,25),(40,3),(128,13),(255,255)):
            result.append((f'mode{mode}-x{x}-y{y}',dict(mode=mode,x=x,y=y)))
    for site in SITES:
        result.append((f'code-{site:x}',dict(writes=((CS,site,b'\x90'),))))
        result.append((f'offset-{site:x}',dict(writes=((CS,site+1,bytes(2)),))))
    for off in (0x2A,0x4D,0x57):result.append((f'stub-{off:x}',dict(writes=((STUB,off,b'\x90'),))))
    for sp in (0x23A,0x23B,0x8001,0xFFFB,0xFFFC):result.append((f'sp-{sp:x}',dict(sp=sp)))
    for flags in FLAGS:result.append((f'flags-{flags:x}',dict(flags=flags)))
    rng=random.Random(0x1C2E7D)
    for n in range(100):
        chars=[rng.randrange(1,255) for _ in range(n%9)]+[rng.choice((0,255,65535))]
        result.append((f'random-{n}',dict(text=tuple(chars),mode=n%4,x=rng.randrange(42),y=rng.randrange(26),wrap=n%2,flags=FLAGS[n%6])))
    return result
def differential(bodies,decoded,lib):
    seen=set();edges=set();stops={};outs=stores=0
    for name,raw in cases():
        options=dict(raw);limits={k:options.pop(k) for k in ('outer','glyph_limit','operands','strings_limit','refuse') if k in options}
        status,v,e,o,w=compare(name,build(bodies,**options),bodies,decoded,lib,**limits)
        seen|=v;edges|=e;outs+=o;stores+=w;stops[str(status)]=stops.get(str(status),0)+1
        print(name,status,flush=True)
    caller={k for k in decoded if k[0]=='root'}
    wanted={(k,(k[0],b)) for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for b in (i.address+i.size,int(i.op_str,16))}
    check(caller<=seen,'missing caller starts')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ordinary_ram_bytes=SIZE-65536,flag_masks=0,
        caller_starts=len(caller&seen),caller_total=len(caller),caller_outcomes=sum(a[0]=='root' for a,b in edges),caller_outcomes_total=sum(a[0]=='root' for a,b in wanted),
        chain_starts=len(seen),chain_total=len(decoded),chain_outcomes=len(edges&wanted),chain_outcomes_total=len(wanted),
        missing_starts=sorted(set(decoded)-seen),missing_outcomes=sorted(wanted-edges),out_events=outs,vram_write_operands=stores)
    print('Original parity',{k:v for k,v in report.items() if not k.startswith('missing_')},flush=True);return report
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded,meta=source();report={'identity':meta}
    with tempfile.TemporaryDirectory(prefix='u1c-2e7d-') as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib)
        if a.native:
            from u1c_text_2e7d_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':
    sys.modules['u1c_text_2e7d_cpu_audit']=sys.modules[__name__]
    main()
