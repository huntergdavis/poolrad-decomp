#!/usr/bin/env python3
"""Direct original/C parity for the four raw U3C6 slot APIs; no new credit."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
ENTRIES=(0xBE7,0xC0F,0xC37,0xC5F)
ENDS=(0xC0F,0xC37,0xC5F,0xCB9)
COUNTS=(17,17,17,32)
HASHES=('eb48355d64c585ee95a31739074ede53929cc991ba519157009c7c8e1c9b7ed5',
        'a1000b2f6a29a0d74f37a9b631b26844044f2c1a163b2de793672cbc0664395d',
        '71c5655d5b32a4688be0ab98761c4b5d2cfb4cc117056648ab92a552ffc06e48',
        'c153e951cb4be5e40f4fd65d9e6d114a0b2d3cbd615bc73025a3d2c4c387de41')
CS,DS,SS=0x8800,0x1800,0x2800
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
def check(v,msg):
    if not v:raise ValueError(msg)
def source():
    ex=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ov=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ex).hexdigest()==EXE_SHA and sha256(ov).hexdigest()==OVR_SHA,'media hashes')
    u=next(u for u in find_units(ex[parse_mz(ex)['header_bytes']:],ov) if u['stub_para']==0x3C6)
    code=ov[u['ovr_offset']:u['ovr_offset']+u['code_size']];md=Cs(CS_ARCH_X86,CS_MODE_16)
    decoded={};meta={}
    for entry,end,count,digest in zip(ENTRIES,ENDS,COUNTS,HASHES):
        body=code[entry:end];ins=list(md.disasm(body,entry))
        check(sha256(body).hexdigest()==digest and len(ins)==count and sum(i.size for i in ins)==end-entry,'body identity')
        public=u['entries'].index(entry);check(public==12+ENTRIES.index(entry),'public entry')
        check(ins[-1].mnemonic=='retf' and ins[-1].op_str=='4' and ins[-1].address==end-3,'terminal RETF')
        if entry!=0xC5F:check([(i.address,i.op_str) for i in ins if i.mnemonic=='call']==[(entry+13,'0xc5f')],'index call')
        decoded.update({i.address:i for i in ins})
        meta[f'{entry:04X}']=dict(bytes=end-entry,instructions=count,sha256=digest,public_entry=public,
            original_stub=f'03E0:{0x20+5*public:04X}',file_start=u['ovr_offset']+entry,file_end=u['ovr_offset']+end)
    return code,decoded,meta
def library(directory):
    path=directory/'getters.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),str(ROOT/'src/recovered/overlay_u3c6_slot_getters_cpu.c'),'-o',str(path)],check=True)
    f=ctypes.CDLL(str(path)).pr_u3c6_slot_getter_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t];f.restype=ctypes.c_int
    return f
def build(code,entry,*,count=5,selected=3,mismatch='none',ss=SS,sp=0x8000,flags=0x602,seed=0,writes=(),background=False):
    rng=random.Random(seed);m=bytearray(rng.randbytes(SIZE)) if background else bytearray([0xA5])*SIZE
    for i in range(256):
        put16(m,DS,0x65B9+4*i,0x1000+32*i);put16(m,DS,0x65BB+4*i,0x5000)
        for offset in (0x5F27,0x5F28,0x5F2A):m[physical(DS,offset+4*i)]=(i*17+offset)&255
    m[physical(DS,0x5F2A)]=count
    off=0x1000+32*selected;seg=0x5000
    if mismatch=='offset':off^=1
    if mismatch=='segment':seg^=1
    for j,v in enumerate((0x7000,0x9001,off,seg)):put16(m,ss,(sp+2*j)&65535,v)
    for start,end in zip(ENTRIES,ENDS):m[physical(CS,start):physical(CS,end)]=code[start:end]
    for seg,off,blob in writes:
        for j,value in enumerate(blob):m[physical(seg,(off+j)&65535)]=value
    cpu=Cpu(*[rng.randrange(65536) for _ in range(7)],sp,CS,entry,DS,rng.randrange(65536),ss,flags)
    return m,cpu
def compare(label,m,c,decoded,f,cap=512):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(m));uc.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,name))
    entry=c.ip;outer=ENDS[ENTRIES.index(entry)]-3
    state=dict(returned=False,inner_return=False,expected=None,status=None,visits=0,previous=None)
    seen,edges=set(),set()
    def halt(status):state['status']=status;uc.emu_stop()
    def hook(em,address,size,user):
        del em,address,size,user
        cs,ip=uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
        if state['returned']:return halt(0)
        if state['inner_return']:
            state['inner_return']=False
            if (cs,ip)!=state['expected']:return halt(3)
        check(cs==CS and ip in decoded,f'original escape {label} {cs:x}:{ip:x}')
        if ip==0xC71:
            if state['visits']==cap:return halt(2)
            state['visits']+=1
        seen.add(ip);previous=decoded.get(state['previous'])
        if previous and previous.mnemonic.startswith('j') and previous.mnemonic!='jmp':edges.add((previous.address,ip))
        ins=decoded[ip]
        if ins.mnemonic=='call':state['expected']=(cs,ip+ins.size)
        if ip==outer:state['returned']=True
        elif ip==0xCB6:state['inner_return']=True
        state['previous']=ip
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);em.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
    uc.emu_start(physical(CS,entry),2*SIZE,count=100000)
    check(state['status'] is not None,f'original timeout {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);status=f(ram,SIZE,ctypes.byref(c),cap)
    check(status==state['status'],f'status {label}: {status}/{state}')
    check(bytes(ram)==bytes(uc.mem_read(0,SIZE)),f'RAM {label}')
    exception=entry!=0xC5F and status==0
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(reg)&65535;got=getattr(c,name)
        if name=='flags' and exception:
            check(got^want==0x10 and got&0x10,f'explicit SHL-AF profile {label}: {got:x}/{want:x}')
        else:check(got==want,f'CPU {label}/{name}: {got:x}/{want:x}')
    return status,seen,edges,int(exception)
def cases():
    result=[]
    for count in (0,1,2,5,127,128,254,255):
        for selected in (0,1,2,5,127,128,254,255):
            result.append((f'count{count}-select{selected}',dict(count=count,selected=selected)))
    for mismatch in ('offset','segment'):
        result.append((mismatch,dict(count=5,selected=3,mismatch=mismatch)))
    result += [('zero-budget',dict(cap=0)),('one-budget',dict(cap=1)),
        ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),('random',dict(background=True)),
        ('table-is-frame',dict(ss=DS,sp=0x65BD)),
        ('count-is-local',dict(ss=DS,sp=0x5F38,cap=300))]
    for sp in (0x20,0x21,0x8001,0xFFF7,0xFFF8):result.append((f'sp{sp:x}',dict(sp=sp)))
    for delta in range(0,20,2):
        result.append((f'frame-table-alias{delta}',dict(ss=DS,sp=0x5F28+delta)))
    return result
def differential(code,decoded,f):
    reports={};total=exceptions=0
    for entry in ENTRIES:
        seen,edges,stops=set(),set(),{};exc=0
        for n,(label,options) in enumerate(cases()):
            options=dict(options);cap=options.pop('cap',512)
            m,c=build(code,entry,seed=n,flags=FLAGS[n%len(FLAGS)],**options)
            status,v,e,x=compare(f'{entry:x}/{label}',m,c,decoded,f,cap)
            seen|=v;edges|=e;exc+=x;stops[str(status)]=stops.get(str(status),0)+1
        required={i for i in decoded if entry<=i<ENDS[ENTRIES.index(entry)] or 0xC5F<=i<0xCB9}
        check(required<=seen,f'missing starts {entry:x}: {required-seen}')
        check(len(edges)==10,f'index branch outcomes {entry:x}: {edges}')
        reports[f'{entry:04X}']=dict(cases=sum(stops.values()),stops=stops,chain_starts=len(seen),conditional_outcomes=len(edges),unicorn_shl_af_exceptions=exc)
        total+=sum(stops.values());exceptions+=exc
    return dict(cases=total,identities=reports,cpu_words=14,ram_bytes=SIZE,unicorn_shl_af_exceptions=exceptions)
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    code,decoded,meta=source();report=dict(targets=meta)
    with tempfile.TemporaryDirectory(prefix='u3c6-getters-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(code,decoded,f)
        if a.native:
            from u3c6_slot_getters_cpu_native import native
            report['native']=native(code,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True),flush=True)
if __name__=='__main__':main()
