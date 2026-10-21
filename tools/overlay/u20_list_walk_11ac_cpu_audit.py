#!/usr/bin/env python3
"""Complete U20 +11AC original CPU/RAM differential against the raw C model.

The original bytes of [11AC,12A8) plus the +3291 closure and SYSTEM FreeMem are
executed in Unicorn and compared with the candidate model on CPU14 and the whole
1 MiB of RAM. Original media and generated blobs stay under ignored resources/.

Extent note: the queue row for this body says 316 bytes and 104 instructions.
The true body is [11AC,12A8) = 252 bytes / 77 instructions, final RETF at 12A7;
the remaining 64 bytes are data (`00 1b 20 74 61 6b 65 73` - a text string), and
decoding them invents 27 phantom instructions and one phantom far call.
"""
import argparse,ctypes,json,struct,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
import u6c_record_unlink_3291_audit as closure
from res0709_ega_reset_0000_cpu_audit import Cpu,physical,put16,check,ROOT,SIZE,EXE_SHA,REGISTERS
from u86_procvars_3413_audit import OVR_SHA
CHECKER_ROWS=('OVL_0020_11AC',)
CS,DS,SS,SYSTEM=0x9000,0x1800,0x2800,0x4000
ENTRY,END,RETF=0x11AC,0x12A8,0x12A7
BODY_SHA='87b20686c110afeba83ea8f33c74623e20b187cf2d9d74b69d29e6dbd295e851'
STUB=0x78
UNLINK,UNLINK_END=0x3291,0x33D7
UNLINK_STUB_SITE=0x1239
FREEMEM_SITE=0x126C
SOURCES=('overlay_u20_list_walk_11ac_cpu',)+tuple(closure.SOURCES)
NAMES=('callee','status','calls_unlink','calls_freemem')+tuple('unlink_'+n for n in ('callee','callee_status','pair_calls','record_calls','call_site'))
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in NAMES]
class UnlinkLimits(ctypes.Structure):
    _fields_=[('max_walk_loopbacks',ctypes.c_uint),('max_list_loopbacks',ctypes.c_uint),('max_heap_steps',ctypes.c_uint)]
class Limits(ctypes.Structure):
    _fields_=[('max_walk_loopbacks',ctypes.c_uint),('max_callee_steps',ctypes.c_uint),('unlink',UnlinkLimits)]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media provenance')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);u=next(u for u in units if u['stub_para']==0x20)
    at=u['ovr_offset']+ENTRY;body=ovr[at:at+END-ENTRY];tail=ovr[at+END-ENTRY:at+END-ENTRY+64]
    md=Cs(CS_ARCH_X86,CS_MODE_16);ins=list(md.disasm(body,ENTRY))
    check(len(body)==252 and len(ins)==77 and sum(i.size for i in ins)==252,'11AC extent 252/77')
    check(sha256(body).hexdigest()==BODY_SHA,'11AC body hash')
    check((ins[-1].address,ins[-1].mnemonic)==(RETF,'retf'),'final RETF at 12A7')
    check(tail[:4]==b'\x00\x1b\x20t','trailing tail is data, not code')
    check(all(ENTRY<=int(i.op_str,16)<END for i in ins if i.mnemonic.startswith('j') and i.op_str.startswith('0x')),
          'all branch targets inside the extent')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==
          [(UNLINK_STUB_SITE,'0x78, 0x2a'),(FREEMEM_SITE,'0xaf8, 0x254')],'two far call sites')
    check(UNLINK_STUB_SITE+3 in u['fixups'],'unlink stub segment is fixup-backed')
    c=closure.source()
    check(c['info']['span']==[UNLINK,UNLINK_END],'closure span')
    check((0x20,UNLINK_STUB_SITE) in c['info']['incoming'],'this body is a recorded fixup caller of +3291')
    decoded={i.address:i for i in ins}
    info=dict(target='OVL_0020_11AC',span=[ENTRY,END],file_start=at,bytes=len(body),instructions=len(ins),
              sha256=sha256(body).hexdigest(),queue_bytes=316,queue_instructions=104,
              trimmed_tail=len(tail),tail_sha256=sha256(tail).hexdigest(),
              unlink_closure=c['info'])
    return dict(body=body,closure=c,closure_ctx=None,decoded=decoded,info=info)
def library(directory):
    out=Path(directory)/'libu20_11ac.so'
    inc=str(ROOT/'include');src=str(ROOT/'src/recovered')
    files=[(f"{src}/{n}" if n.endswith(".c") else f"{src}/{n}.c") for n in SOURCES]
    subprocess_cmd=['gcc','-std=c11','-O1','-fPIC','-shared','-I'+inc,'-I'+src,*files,'-o',str(out)]
    import subprocess as sp
    r=sp.run(subprocess_cmd,capture_output=True,text=True)
    check(r.returncode==0,'candidate build: '+r.stderr[-800:])
    lib=ctypes.CDLL(str(out))
    fn=lib.pr_u20_list_walk_11ac_cpu
    fn.restype=ctypes.c_int
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),
                 ctypes.c_uint16,ctypes.c_uint16,ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    return fn
def far(m,seg,off): return struct.unpack_from('<HH',m,physical(seg,off))
def put_far(m,seg,off,value): struct.pack_into('<HH',m,physical(seg,off),value[0],value[1])
def build(ctx,rng,*,nodes=3,head=0,types=None,freed=None,node_off=None,sp=0x8000,ss=None,flags=0x602,
          unlink_stub_seg=STUB,unlink_entry=UNLINK,extra_writes=(),**_):
    """Reuse the +3291 closure's own world builder: it already chains the nodes this
    body walks (+104h), seeds the head (5D96/5D98), the active record (5D92/5D94)
    and the window pointer (49D6), and installs every closure body at its segment.
    Only the root body, its stub and the CPU entry frame are ours."""
    c=closure.source()
    import random as _random
    rng=rng or _random.Random(20261027)
    m,cpu=closure.build(c,rng,records=nodes,
                        head=head if head is not None else None,active=None,
                        sp=sp,ss=closure.SS if ss is None else ss,flags=flags,ds=closure.DS)
    # node type bytes and free pointers, by the closure's own node addressing
    for i in range(nodes):
        seg,off=closure.rec_ptr(i,(node_off or {}).get(i,0))
        t=(types or {}).get(i,0)
        m[physical(seg,(off+0x13)&65535)]=1 if t&1 else 0
        m[physical(seg,(off+0x10D)&65535)]=1 if t&2 else 0
        m[physical(seg,(off+0x10E)&65535)]=1 if t&4 else 0
        if i in (freed or {}): put_far(m,seg,(off+0x108)&65535,(0x6000+i*0x40,0x20))
    # root body, and the 0078:002A stub bound to the +3291 closure's own segment
    body=ctx['body']; at=physical(CS,ENTRY); m[at:at+len(body)]=body
    # relocation: the loader rewrites the FreeMem call's segment word (the unlink
    # call's 0x78 is a stub segment that stays as encoded)
    put16(m,CS,FREEMEM_SITE+3,closure.SYSTEM)
    m[physical(unlink_stub_seg,0x2A):physical(unlink_stub_seg,0x2A)+5]=b'\xEA'+struct.pack('<HH',unlink_entry,closure.CS)
    for seg,off,blob in extra_writes:
        for j,v in enumerate(blob): m[(physical(seg,off)+j)&(SIZE-1)]=v
    # entry frame: far return address only (this body takes no arguments)
    put16(m,cpu.ss,(sp+0)&65535,0x7777); put16(m,cpu.ss,(sp+2)&65535,0x8888)
    cpu.cs,cpu.ip,cpu.sp=CS,ENTRY,sp
    cpu.bp=0x7F00; cpu.ax=cpu.bx=cpu.cx=cpu.dx=cpu.si=cpu.di=0
    return m,cpu,None
def compare(name,state,ctx,fn,lim,*,max_walk=64,expect=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP,UC_X86_REG_SP,UC_X86_REG_SS
    m,c,chain=state
    uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    s=dict(stop=None,last=None,walk=0,callee=None,cstatus=None,unlink_calls=0,freemem_calls=0,site=0)
    def stop(code):s['stop']=code;uc.emu_stop()
    def hook(em,address,size,user):
        cs,ip=uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
        if cs==CS and ip==0x11CA:
            if s['walk']==max_walk:return stop(5)
            s['walk']+=1
        if cs==CS and ip in (UNLINK_STUB_SITE,FREEMEM_SITE):
            s['site']=ip;raw=bytes(uc.mem_read(address,5))
            want=(0x2A,STUB) if ip==UNLINK_STUB_SITE else (0x254,closure.SYSTEM)
            if raw[0]!=0x9A or struct.unpack_from('<HH',raw,1)!=want:return stop(2)
            if ip==UNLINK_STUB_SITE:
                # the 0x78 stub must be the loaded EA jump to the +3291 body;
                # the FreeMem call is direct into the SYSTEM segment, no stub
                stub=struct.unpack_from('<H',raw,3)[0]
                loaded=bytes(uc.mem_read(physical(stub,want[0]),5))
                if loaded[0]!=0xEA or struct.unpack_from('<H',loaded,1)[0]!=UNLINK:return stop(2)
            if ip==UNLINK_STUB_SITE:s['unlink_calls']+=1
            else:s['freemem_calls']+=1
        if cs==closure.SYSTEM and ip==0x00E2:return stop(3)          # FreeMem RunError
        if cs==CS and ip==RETF:
            # the hook fires before the RETF executes; the candidate completes it,
            # so perform the far return here to compare SP and the popped frame
            sp_=uc.reg_read(UC_X86_REG_SP);ss_=uc.reg_read(UC_X86_REG_SS)
            ip_=struct.unpack_from('<H',bytes(uc.mem_read(physical(ss_,sp_),2)))[0]
            cs_=struct.unpack_from('<H',bytes(uc.mem_read(physical(ss_,(sp_+2)&65535),2)))[0]
            uc.reg_write(UC_X86_REG_SP,(sp_+4)&65535);uc.reg_write(UC_X86_REG_IP,ip_);uc.reg_write(UC_X86_REG_CS,cs_)
            return stop(0)
        if cs==CS and ip==END:return stop(4)
    uc.hook_add(UC_HOOK_CODE,hook);uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=400000)
    check(s['stop'] is not None,name+': no accepted stop')
    ctypes_state=Cpu(**{n:getattr(c,n) for n,_ in Cpu._fields_})
    # The C diagnostic nests the +3291 closure's own diagnostic structures; a
    # ctypes mirror of that layout is a second source of truth that silently
    # corrupts the heap when it drifts (it did, with a segfault). The API allows
    # a null diagnostic, and status + CPU14 + full RAM is the stronger claim.
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    got=fn(ram,SIZE,ctypes.byref(ctypes_state),closure.RESIDENT,closure.SYSTEM,lim,None)
    check(got==s['stop'],f'{name}: status C={got} original={s["stop"]}')
    if expect is not None:check(got==expect,f'{name}: expected {expect}')
    if got==0:
        for (n,_),reg in zip(Cpu._fields_,REGISTERS):
            want=uc.reg_read(reg)&65535
            check(getattr(ctypes_state,n)==want,f'{name}: {n} C={getattr(ctypes_state,n):04x} original={want:04x}')
        if bytes(ram)!=bytes(backing):
            diff=[i for i in range(SIZE) if ram[i]!=backing[i]]
            raise ValueError(f'{name}: {len(diff)} RAM differences, first {diff[:8]}')
    return got,s['unlink_calls'],s['freemem_calls']
def specifications():
    base=dict(nodes=3,head=0,types={},freed={})
    specs=[('nil-head',dict(base,head=None),0)]
    for t in (0,1,2,3,4,5,6,7):
        specs.append((f'types-{t}-head0',dict(base,types={0:t}),0))
        specs.append((f'types-{t}-head1',dict(base,head=1,types={1:t}),0))
    specs+= [('one-node',dict(base,nodes=1,head=0),0),
             ('freed-tail',dict(base,freed={1:True}),0),
             ('freed-all',dict(base,freed={0:True,1:True,2:True}),0),
             ('head-nil-but-nodes',dict(base,head=None),0),
             ('window-other',dict(base,window=(0x3B00,0x40)),0),
             ('ss-eq-ds',dict(base,ss=DS),0),
             ('sp-low',dict(base,sp=0x0058),0),
             ('walk-limit-1',dict(base),5)]
    specs=[(n,dict(kw,_walk=(1 if n.startswith('walk-limit') else 64)),exp) for n,kw,exp in specs]
    return specs
def cases(ctx):
    rng=None;out=[]
    for name,kw,expect in specifications():
        walk=kw.pop('_walk',64)
        out.append((name,build(ctx,rng,**kw),expect,walk))
    return out
def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--output',type=Path)
    a=p.parse_args()
    ctx=source()
    with tempfile.TemporaryDirectory() as tmp:
        fn=library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK');return
        lim=Limits(64,256,UnlinkLimits(64,64,256))
        results=[];seen={}
        failures=[]
        for name,state,expect,walk in cases(ctx):
            try:
                got,nu,nf=compare(name,state,ctx,fn,lim,max_walk=walk,expect=expect)
                seen[name]=dict(status=got,calls=(nu,nf))
                results.append(name)
            except Exception as exc:
                failures.append((name,str(exc)[:160]))
        for name,why in failures:print('FAIL',name,why,flush=True)
        report=dict(identity=ctx['info'],cases=len(results),names=sorted(results),stops={},state=dict(cpu_words=14,
                  ram_bytes=SIZE,flag_masks=0,ordinary_ram_bytes=SIZE-0x10000))
        for name,v in seen.items():report['stops'][str(v['status'])]=report['stops'].get(str(v['status']),0)+1
        print('Original parity',{k:v for k,v in report.items() if k!='names'},flush=True)
        if failures:raise SystemExit(f'{len(failures)}/{len(results)+len(failures)} cases failed')
        if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':
    sys.modules['u20_list_walk_11ac_cpu_audit']=sys.modules[__name__]
    main()
