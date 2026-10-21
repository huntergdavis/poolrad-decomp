#!/usr/bin/env python3
"""Raw 0709 +05EB space composition: original CPU/RAM/OUT/VRAM operands, native planes.
Original media and generated guest programs stay under ignored resources/."""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
CHECKER_ROWS=('RES_0709_05EB',)
CS,DS,SS,SYS=0x8800,0x5000,0x2000,0x6000
ENTRY,END=0x5EB,0x626
LOOPS={0x2CA:0x279,0x351:0x300,0x381:0x267,0x45B:0x3BD,0x468:0x3B4,0x5BC:0x4B2,0x5C8:0x4A5,0x5D4:0x49C,0x5E2:0x235}
FONT=0x7000
import res0709_glyph_01df_cpu_audit as prior
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SOURCES=('resident_0709_space_05eb_cpu',)+prior.SOURCES
WRITE_FN=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.c_uint16,ctypes.c_uint8)
class Ega(ctypes.Structure):
    _fields_=[('plane',(ctypes.c_uint8*65536)*4),('latch',ctypes.c_uint8*4),('seq_index',ctypes.c_uint8),('seq',ctypes.c_uint8*5),('gc_index',ctypes.c_uint8),('gc',ctypes.c_uint8*9)]
class Bus(ctypes.Structure):
    _fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('ega',ctypes.POINTER(Ega)),('ports',ctypes.POINTER(Env)),('on_vram_write',WRITE_FN),('ctx',ctypes.c_void_p)]
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('callee','status','glyph_callee','glyph_status')]
def source():
    bodies,decoded,meta=prior.source()
    image=bytes(unpack((ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes())[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    blob=image[physical(0x709,ENTRY):physical(0x709,END)];ins=list(md.disasm(blob,ENTRY))
    digest='a41af7187ac3e0bf5a09311bad9fae220c498f0f9aad9492438661381d4b1171'
    check(len(ins)==27 and sha256(blob).hexdigest()==digest,'05EB identity')
    bodies['caller',ENTRY]=blob;decoded.update({('caller',i.address):i for i in ins})
    meta['0709:05EB']=dict(bytes=len(blob),instructions=len(ins),sha256=digest)
    return bodies,decoded,meta
def library(directory):
    path=directory/'printer.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_r0709_space_05eb_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,cs=CS,system=SYS):prior.install(m,bodies,cs,system)
def build(bodies,*,args=(3,5,2,3),ax=None,**options):
    m,c=prior.build(bodies,**options);install(m,bodies);c.ip=ENTRY
    for j,v in enumerate(args):put16(m,c.ss,(c.sp+4+2*j)&65535,0xA500|(v&255))
    for off in (12,14):put16(m,c.ss,(c.sp+off)&65535,0xA5A5)
    if ax is not None:c.ax=ax
    for seg,off,blob in options.get('writes',()):m[physical(seg,off):physical(seg,off)+len(blob)]=blob
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
    status=lib.pr_r0709_space_05eb_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),limit,ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def compare(name,state,bodies,decoded,lib,limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    s=dict(stop=None,ret=False,callee=-1,status=-1,glyph_callee=-1,glyph_status=-1,loops=0,glyph_loops=0,last=None)
    calls=[];seen=set();edges=set();events=[];writes=[]
    def stop(v):s['stop']=v;uc.emu_stop()
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=('caller' if cs==CS else 'sys' if cs==SYS else '?',ip)
        if s['ret']:
            if not calls:return stop(0)
            expected,owner=calls.pop()
            if (cs,ip)!=expected:
                if owner=='glyph':s['status']=5;return stop(3)
                return stop(5)
            s['ret']=False
        check(key in decoded,f'escape {name} {cs:04x}:{ip:04x}')
        ins=decoded[key]
        if key==('caller',0x61F) and bytes(uc.mem_read(address,3))!=bytes.fromhex('e8bdfb'):return stop(2)
        if key==('caller',0x207) and bytes(uc.mem_read(address,3))!=bytes.fromhex('9a2a18'):s['status']=2;return stop(3)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:s['status']=4;return stop(3)
        if key[0]=='caller' and ip in LOOPS and (ins.mnemonic=='jmp' or not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40):
            if s['glyph_loops']==limit:s['status']=6;return stop(3)
            s['glyph_loops']+=1
        if s['last'] is not None:edges.add((s['last'],key))
        seen.add(key);s['last']=key if ins.mnemonic in ('ja','jne','je','jbe','jb') else None
        if key[0]=='caller' and ins.mnemonic in ('call','lcall'):
            owner='space' if ip==0x61F else 'glyph'
            calls.append(((cs,ip+ins.size),owner))
            if ip==0x61F:
                s['callee']=0;s['status']=0;s['glyph_callee']=s['glyph_status']=-1;s['glyph_loops']=0
            else:s['glyph_callee']=0;s['glyph_status']=0
        if ins.mnemonic=='retf':s['ret']=True
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
    check((d.callee,d.status,d.glyph_callee,d.glyph_status)==tuple(s[k] for k in ('callee','status','glyph_callee','glyph_status')),f'diagnostic {name}: {d.callee}/{d.status}/{d.glyph_callee}/{d.glyph_status}, {s}')
    check(mine==events,f'OUTs {name}: {mine}/{events}')
    check(stores==writes,f'VRAM writes {name}: {stores[:3]}/{writes[:3]} ({len(stores)}/{len(writes)})')
    expected=bytes(backing)
    check(ram[:0xA0000]+ram[0xB0000:]==expected[:0xA0000]+expected[0xB0000:],f'RAM {name}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n} {getattr(actual,n):04x}/{want:04x}')
    return status,seen,edges,len(events),len(writes)
def cases(quick=False):
    result=[(f'mode{m}-count{n}',dict(mode=m,args=(n,5,2,3))) for m in (0,1,2,3) for n in (0,1,3)]
    result += [('x-out',dict(args=(3,5,2,40))),('y-out',dict(args=(3,5,25,3))),
      ('near-code',dict(writes=((CS,0x61F,b'\x90'),))),('near-operand',dict(writes=((CS,0x620,bytes(2)),))),
      ('glyph-code',dict(writes=((CS,0x207,b'\x90'),))),('glyph-limit0',dict(limit=0))]
    if quick:return result
    for mode in range(3):
        for color in range(16):result.append((f'm{mode}-c{color}',dict(mode=mode,args=(3,color,24,38),flags=FLAGS[color%6])))
        for limit in (1,7,31,32,100):result.append((f'm{mode}-limit{limit}',dict(mode=mode,limit=limit)))
    for n in (2,40,255):result.append((f'count{n}',dict(args=(n,5,0,0))))
    for x in (0,39,127,128,255):
        for y in (0,24,25,255):result.append((f'x{x}-y{y}',dict(args=(1,5,y,x))))
    for flags in FLAGS:result.append((f'flags{flags:x}',dict(flags=flags)))
    for n in range(35):result.append((f'port{n}',dict(refuse=n)))
    for sp in (0x2C,0x2D,0x8001,0xFFF3,0xFFF4):result.append((f'sp{sp:x}',dict(sp=sp)))
    result += [('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
      ('mode-stack-alias',dict(ss=DS,sp=0x4A20)),('ahff',dict(ax=0xFF34)),
      ('zero-font',dict(writes=((DS,0x5E20,bytes(4)),))),('zero-pattern',dict(pattern=1)),
      ('ones-pattern',dict(pattern=2)),('shared-pages',dict(pages=(0xA000,0xA000))),
      ('ram-pages',dict(pages=(0x3000,0x3100))),('font-wrap',dict(fontoff=0xFFFE))]
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
    if not quick:
        check(caller<=visited,'caller instruction coverage');check(len(outcomes)==6,'all feasible caller outcomes')
    return dict(cases=len(specs),stops=stops,caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(outcomes),caller_outcomes_feasible=6,caller_outcomes_syntactic=8,impossible_outcomes=['05F2->0606','05FE->0606'],
        uncovered_starts=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
        uncovered_outcomes=[f'{k}:{ip:04X}->{dest:04X}' for k,ip in sorted(caller) if decoded[k,ip].mnemonic in ('ja','jne','je','jbe','jb') for dest in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,dest)) not in edges],
        chain_starts=len(visited),out_events=out_count,vram_write_operands=write_count,cpu_words=14,ram_bytes=SIZE-65536,flag_masks=0)
def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    if a.compile_only:
        # media-free: this gate runs on a tree without resources/DOS/POOLRAD, so nothing may read START.EXE here
        with tempfile.TemporaryDirectory(prefix='space05eb-') as tmp:library(Path(tmp))
        print('CHECKER_BUILD_OK');return
    bodies,decoded,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='space05eb-') as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from res0709_space_05eb_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['res0709_space_05eb_cpu_audit']=sys.modules[__name__]
    main()
