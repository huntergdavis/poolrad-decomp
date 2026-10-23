#!/usr/bin/env python3
"""Raw 0709 +01DF glyph composition: original CPU/RAM/OUT/VRAM operands, native planes.
Original media and generated guest programs stay under ignored resources/."""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
CS,DS,SS,SYS=0x8800,0x5000,0x2000,0x6000
ENTRY,END=0x1DF,0x5EB
LOOPS={0x2CA:0x279,0x351:0x300,0x381:0x267,0x45B:0x3BD,0x468:0x3B4,0x5BC:0x4B2,0x5C8:0x4A5,0x5D4:0x49C,0x5E2:0x235}
FONT=0x7000
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SOURCES=('resident_0709_glyph_01df_cpu','system_runtime_leaves_cpu','memory_bus','ega_hw')
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
    spans=((0x709,ENTRY,END,403,'a9d1c72241b6810c3e0995b03cca98edc56cb76734e948333e360d82ef7e1840'),
           (0xAF8,0x182A,0x183D,8,'738c9a82c7faab6335f0952cc5d989d273cb85828a0735f34966220f370d012c'))
    bodies={};decoded={};meta={}
    for seg,a,b,n,digest in spans:
        blob=image[physical(seg,a):physical(seg,b)]
        # The caller's far segment is rebound in install(); UpCase is position-independent.
        ins=list(md.disasm(blob,a));check(len(ins)==n and sha256(blob).hexdigest()==digest,f'identity {seg:x}:{a:x}')
        kind='caller' if seg==0x709 else 'sys';bodies[kind,a]=blob
        decoded.update({(kind,i.address):i for i in ins})
        meta[f'{seg:04X}:{a:04X}']=dict(bytes=len(blob),instructions=n,sha256=digest)
    return bodies,decoded,meta
CHECKER_ROWS = ('RES_0709_01DF',)


def library(directory):
    path=directory/'glyph.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_r0709_glyph_01df_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,cs=CS,system=SYS):
    for (kind,start),blob in bodies.items():
        at=physical(cs if kind=='caller' else system,start);m[at:at+len(blob)]=blob
    put16(m,cs,0x20A,system)
def build(bodies,*,args=(1,97,1,5,2,3),mode=1,ss=SS,ds=DS,sp=0x8000,flags=2,fontoff=0x1000,fontseg=FONT,pages=(0xA000,0xA200),pattern=0,mask=(1,2,4,8),writes=()):
    m=bytearray([0xA5])*SIZE;install(m,bodies)
    c=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,CS,ENTRY,ds,0x1111,ss,flags)
    for j,v in enumerate((0x7000,0x9001,*[(0xA500|(x&255)) for x in args])):put16(m,ss,(sp+2*j)&65535,v)
    m[physical(ds,0x4A1E)]=mode
    put16(m,ds,0x4A16,pages[0]);put16(m,ds,0x4A18,pages[1])
    put16(m,ds,0x5E20,fontoff);put16(m,ds,0x5E22,fontseg)
    for j,v in enumerate(mask):m[physical(ds,0x25C9+j)]=v
    for j,v in enumerate((0,0x5555,0xAAAA,0xFFFF)):put16(m,ds,0x25CD+2*j,v)
    for j in range(256):m[physical(ds,0x25D5+j)]=j%4
    for j in range(8):put16(m,ds,0x25E5+2*j,0xC000>>(2*j))
    for j in range(2048):m[physical(fontseg,(fontoff+j)&65535)]=((j*37+(j>>3)*13)^0xA6)&255 if not pattern else (0 if pattern==1 else 255)
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
    status=lib.pr_r0709_glyph_01df_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),limit,ctypes.byref(d))
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
        if key==('caller',0x207) and bytes(uc.mem_read(address,3))!=b'\x9a\x2a\x18':return stop(2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:
            return stop(4)
        if key[0]=='caller' and ip in LOOPS and (ins.mnemonic=='jmp' or not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40):
            if s['loops']==limit:return stop(6)
            s['loops']+=1
        if s['last'] is not None:edges.add((s['last'],key))
        seen.add(key);s['last']=key if ins.mnemonic in ('ja','jne','je','jbe','jb') else None
        if key[0]=='caller' and ins.mnemonic in ('call','lcall'):
            s['callee']=0;s['status']=0
            s['expected']=(cs,ip+ins.size)
        if ins.mnemonic=='retf':
            s['ret']=True;s['outer']=key==('caller',0x5E8)
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
    result=[(f'mode-{mode}-pattern-{pattern}',dict(mode=mode,pattern=pattern)) for mode in range(4) for pattern in range(3)]
    result += [('no-font',dict(fontoff=0,fontseg=0,writes=((DS,0x5E20,bytes(4)),))),
      ('x-out',dict(args=(1,97,1,5,2,40))),('y-out',dict(args=(1,97,1,5,25,3))),
      ('zero-count',dict(args=(1,97,0,5,2,3))),('raw-char',dict(args=(0,255,1,15,24,39))),
      ('upper-before',dict(args=(1,96,1,5,2,3))),('upper-after',dict(args=(1,123,1,5,2,3))),
      ('upper-last',dict(args=(1,122,1,5,2,3)))]
    if quick:return result
    for mode in range(3):
        for color in range(16):result.append((f'm{mode}-color{color}',dict(mode=mode,args=(color%2,97+color,2,color,24,39),flags=FLAGS[color%6])))
    for mode in range(3):
        for limit in (0,1,7,8,31,32,63,64,100):result.append((f'm{mode}-limit{limit}',dict(mode=mode,limit=limit)))
    for n in range(34):result.append((f'port-{n}',dict(refuse=n)))
    for sp in (0x1A,0x1B,0x8001,0xFFEF,0xFFF0):result.append((f'stack-{sp:x}',dict(sp=sp)))
    result += [('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
       ('font-offset-wrap',dict(fontoff=0xFFF0)),('same-page',dict(pages=(0xA000,0xA000))),
       ('ram-page',dict(pages=(0x3000,0x3100))),('zero-mask',dict(mask=(0,0,0,0))),
       ('combined-mask',dict(mask=(3,6,12,15))),('opcode',dict(writes=((CS,0x207,b'\x90'),))),
       ('call-offset',dict(writes=((CS,0x208,bytes(2)),))),
       ('frame-mode-alias',dict(ss=DS,sp=0x4A12,args=(0,1,1,1,1,1),limit=300)),
       ('repeat255',dict(mode=3,args=(0,255,255,0,0,39))),
       ('bank-stack-alias',dict(mode=2,ss=0xB800,sp=0x29C,args=(0,65,1,15,2,3),pattern=2,limit=200)),
       ('font-output-alias',dict(pages=(FONT,FONT),fontoff=0xF00,args=(0,0,1,5,12,0)))]
    rng=random.Random(0x70901DF)
    for n in range(36):result.append((f'random-{n}',dict(mode=n%3,args=(rng.randrange(256),rng.randrange(256),rng.randrange(1,4),rng.randrange(256),rng.randrange(25),rng.randrange(40)),flags=FLAGS[n%6])))
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
    if not quick:check(caller<=visited,'caller instruction coverage')
    return dict(cases=len(specs),stops=stops,caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(outcomes),
        uncovered_starts=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
        uncovered_outcomes=[f'{k}:{ip:04X}->{dest:04X}' for k,ip in sorted(caller) if decoded[k,ip].mnemonic in ('ja','jne','je','jbe') for dest in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,dest)) not in edges],
        chain_starts=len(visited),out_events=out_count,vram_write_operands=write_count,cpu_words=14,ram_bytes=SIZE-65536,flag_masks=0)
def main():
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='glyph01df-') as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from res0709_glyph_01df_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['res0709_glyph_01df_cpu_audit']=sys.modules[__name__]
    main()
