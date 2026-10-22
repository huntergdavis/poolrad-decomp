#!/usr/bin/env python3
"""Raw 0709 +00B7 composition: original CPU/RAM/OUT/VRAM operands, native planes.
Original media and generated guest programs stay under ignored resources/."""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
CHECKER_ROWS=('RES_0709_00B7',)
CS,DS,SS,SYS=0x8800,0x5000,0x2000,0x6000
ENTRY,END=0xB7,0x171
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SOURCES=('resident_0709_rect_fill_00b7_cpu','resident_0709_page_select_00a0_cpu','resident_0709_ega_reset_0000_cpu','system_fillchar_bus_16a8_cpu','memory_bus','ega_hw')
WRITE_FN=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.c_uint16,ctypes.c_uint8)
class Ega(ctypes.Structure):
    _fields_=[('plane',(ctypes.c_uint8*65536)*4),('latch',ctypes.c_uint8*4),('seq_index',ctypes.c_uint8),('seq',ctypes.c_uint8*5),('gc_index',ctypes.c_uint8),('gc',ctypes.c_uint8*9)]
class Bus(ctypes.Structure):
    _fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('ega',ctypes.POINTER(Ega)),('ports',ctypes.POINTER(Env)),('on_vram_write',WRITE_FN),('ctx',ctypes.c_void_p)]
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('status',ctypes.c_int)]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA,'media identity')
    image=bytes(unpack(exe)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    spans=((0x709,0,0x71,58,'5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5'),
           (0x709,0xA0,0xB7,10,'6b28b416d116890c77ab785a4763866b260b60a9b0f540f224b5db1b64c3760d'),
           (0x709,ENTRY,END,89,'f26747c1e8836399f118763dfee5a2bdeace551495f75b5533772b888fa65ff5'),
           (0xAF8,0x16A8,0x16BC,7,'f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7'))
    bodies={};decoded={};meta={}
    for seg,a,b,n,digest in spans:
        blob=image[physical(seg,a):physical(seg,b)]
        # FillChar's only relocations are outside this leaf; loaded/original match.
        ins=list(md.disasm(blob,a));check(len(ins)==n and sha256(blob).hexdigest()==digest,f'identity {seg:x}:{a:x}')
        kind='caller' if seg==0x709 else 'sys';bodies[kind,a]=blob
        decoded.update({(kind,i.address):i for i in ins})
        meta[f'{seg:04X}:{a:04X}']=dict(bytes=len(blob),instructions=n,sha256=digest)
    return bodies,decoded,meta
def library(directory):
    path=directory/'rect.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_r0709_rect_fill_00b7_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,cs=CS,system=SYS):
    for (kind,start),blob in bodies.items():
        at=physical(cs if kind=='caller' else system,start);m[at:at+len(blob)]=blob
    put16(m,cs,0x143,system)
def build(bodies,*,args=(5,0,2,4,1,2),active=1,ss=SS,ds=DS,sp=0x8000,flags=2,seed=0,writes=()):
    m=bytearray([0xA5])*SIZE;install(m,bodies)
    c=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,CS,ENTRY,ds,0x1111,ss,flags)
    for j,v in enumerate((0x7000,0x9001,*[(0xA500|(x&255)) for x in args])):put16(m,ss,(sp+2*j)&65535,v)
    m[physical(ds,0x4A1A)]=active
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None);d=Diagnostic()
    status=lib.pr_r0709_rect_fill_00b7_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),limit,ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def compare(name,state,bodies,decoded,lib,limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    s=dict(stop=None,ret=False,outer=False,expected=None,callee=-1,status=-1,loops=0,last=None)
    seen=set();edges=set();events=[];writes=[]
    def stop(v):s['stop']=v;uc.emu_stop()
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=('caller' if cs==CS else 'sys' if cs==SYS else '?',ip)
        if s['ret']:
            if s['outer']:return stop(0)
            if (cs,ip)!=s['expected']:return stop(5)
            s['ret']=False
        check(key in decoded,f'escape {name} {cs:04x}:{ip:04x}')
        ins=decoded[key]
        if key==('caller',0x140) and bytes(uc.mem_read(address,3))!=b'\x9a\xa8\x16':return stop(2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:
            if ip<0x71:s['status']=2;return stop(3)
            return stop(4)
        if key==('caller',0x14F) and not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40:
            if s['loops']==limit:return stop(6)
            s['loops']+=1
        if s['last'] is not None:edges.add((s['last'],key))
        seen.add(key);s['last']=key if ins.mnemonic in ('ja','jne') else None
        if key[0]=='caller' and ins.mnemonic in ('call','lcall'):
            s['callee']=2 if ip==0x140 else 1 if ip==0x100 else 0;s['status']=0
            s['expected']=(cs,ip+ins.size)
        if ins.mnemonic=='retf':
            s['ret']=True;s['outer']=key==('caller',0x16E)
    def out(em,port,size,value,user):events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)))
    def write(em,access,address,size,value,user):
        for j in range(size):
            at=(address+j)&(SIZE-1)
            if 0xA0000<=at<0xB0000:writes.append((at-0xA0000,(value>>(8*j))&255))
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT);uc.hook_add(UC_HOOK_MEM_WRITE,write)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=3000000)
    check(s['stop'] is not None,f'instruction ceiling {name}')
    for (kind,start),blob in bodies.items():
        at=physical(CS if kind=='caller' else SYS,start)
        check(bytes(backing[at:at+len(blob)])==m[at:at+len(blob)],f'code mutation outside contract {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,d=run_c(lib,m,actual,limit,refuse)
    check(status==s['stop'],f'status {name}: {status}/{s}')
    check((d.callee,d.status)==(s['callee'],s['status']),f'diagnostic {name}')
    check(mine==events,f'OUTs {name}: {mine}/{events}')
    check(stores==writes,f'VRAM writes {name}: {stores[:3]}/{writes[:3]} ({len(stores)}/{len(writes)})')
    expected=bytes(backing)
    check(ram[:0xA0000]+ram[0xB0000:]==expected[:0xA0000]+expected[0xB0000:],f'RAM {name}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n} {getattr(actual,n):04x}/{want:04x}')
    return status,seen,edges,len(events),len(writes)
def cases(quick=False):
    result=[('default',{}),('one-cell',dict(args=(15,1,0,0,0,0))),('full-screen',dict(args=(6,0,24,39,0,0))),
      ('right-edge',dict(args=(9,1,24,39,24,39))),('zero-width',dict(args=(3,0,0,0,0,1))),
      ('row-limit',dict(args=(7,0,0,2,1,2),limit=1)),('signed-left',dict(args=(2,1,0,0xFF,0,0xFF))),
      ('ss-ds',dict(ss=DS)),('page-return-alias',dict(ss=DS,sp=0x4A2A)),('a20-stack',dict(ss=0xFFFF)),('odd-stack',dict(sp=0x8001))]
    if quick:return result
    for color in range(16):
        for page in range(8):result.append((f'color-{color}-page-{page}',dict(args=(color,page,2,3,1,1),active=7-page,flags=FLAGS[(color+page)%6])))
    for n in range(22):result.append((f'refuse-{n}',dict(refuse=n)))
    for sp in (0x20,0x21,0xFFEF,0xFFF0):result.append((f'sp-{sp:04x}',dict(sp=sp)))
    result += [('code-opcode',dict(writes=((CS,0x140,b'\x90'),))),('code-offset',dict(writes=((CS,0x141,b'\x00\x00'),))),
      ('zero-budget',dict(limit=0)),('wrap-width',dict(args=(4,0,0,0,0,2),limit=0)),
      ('signed-top',dict(args=(11,0,0xFF,3,0xFF,3)))]
    rng=random.Random(0x70900B7)
    for n in range(32):
        x,y=rng.randrange(40),rng.randrange(25);result.append((f'random-{n}',dict(args=(rng.randrange(256),rng.randrange(8),rng.randrange(y,25),rng.randrange(x,40),y,x),active=rng.randrange(8),flags=FLAGS[n%6])))
    return result
def differential(bodies,decoded,lib,quick=False):
    visited=set();edges=set();stops={};out_count=write_count=0
    specs=cases(quick)
    for n,(name,options) in enumerate(specs):
        options=dict(options);limit=options.pop('limit',65536);refuse=options.pop('refuse',None)
        print('BEGIN',n+1,name,flush=True)
        status,v,e,outs,writes=compare(name,build(bodies,**options),bodies,decoded,lib,limit,refuse)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;out_count+=outs;write_count+=writes
    caller={k for k in decoded if k[0]=='caller' and ENTRY<=k[1]<END}
    outcomes={e for e in edges if e[0] in caller}
    if not quick:check(caller<=visited and len(outcomes)==3,'caller coverage excluding unreachable JA taken')
    return dict(cases=len(specs),stops=stops,caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(outcomes),
        unreachable_outcome='011A JA taken: XOR AX,AX; CMP AX,word, so unsigned zero is never above it',
        chain_starts=len(visited),out_events=out_count,vram_write_operands=write_count,cpu_words=14,ram_bytes=SIZE-65536,flag_masks=0)
def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='rect00b7-') as tmp:
        lib=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        bodies,decoded,meta=source();report=dict(target=meta)
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from res0709_rect_fill_00b7_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['res0709_rect_fill_00b7_cpu_audit']=sys.modules[__name__]
    main()
