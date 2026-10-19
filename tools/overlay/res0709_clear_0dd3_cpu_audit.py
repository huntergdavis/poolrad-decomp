#!/usr/bin/env python3
"""Raw 0709 +0DD3 clear composition: original CPU/RAM/OUT/VRAM operands, native planes.
Original media and generated guest programs stay under ignored resources/."""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
CS,DS,SS,SYS=0x8800,0x5000,0x2000,0x6000
ENTRY,END=0xDD3,0xF82
CLEAR=CS
FILL_SITES=(0xE41,0xE63,0xF18,0xF35,0xF52,0xF6F)
RECT_SITES=(0xE8E,0xEA8)
import res0709_rect_fill_00b7_cpu_audit as prior
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SOURCES=('resident_0709_clear_0dd3_cpu',)+prior.SOURCES
WRITE_FN=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.c_uint16,ctypes.c_uint8)
class Ega(ctypes.Structure):
    _fields_=[('plane',(ctypes.c_uint8*65536)*4),('latch',ctypes.c_uint8*4),('seq_index',ctypes.c_uint8),('seq',ctypes.c_uint8*5),('gc_index',ctypes.c_uint8),('gc',ctypes.c_uint8*9)]
class Bus(ctypes.Structure):
    _fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('ega',ctypes.POINTER(Ega)),('ports',ctypes.POINTER(Env)),('on_vram_write',WRITE_FN),('ctx',ctypes.c_void_p)]
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('callee','status','rect_callee','rect_status')]
def source():
    bodies,decoded,meta=prior.source()
    image=bytes(unpack((ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes())[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    blob=image[physical(0x709,ENTRY):physical(0x709,END)];ins=list(md.disasm(blob,ENTRY))
    digest='0ca48149f5b50fcb93ed265bb72528f33ae5ec659cd134f205f6f312c50905f2'
    check(len(ins)==190 and sha256(blob).hexdigest()==digest,'0DD3 identity')
    bodies['clear',ENTRY]=blob;decoded.update({('clear',i.address):i for i in ins})
    meta['0709:0DD3']=dict(bytes=len(blob),instructions=len(ins),sha256=digest)
    return bodies,decoded,meta
def library(directory):
    path=directory/'clear.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_r0709_clear_0dd3_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,cs=CS,system=SYS,clear=CLEAR):
    prior.install(m,{k:v for k,v in bodies.items() if k[0] in ('caller','sys')},cs,system)
    for (kind,start),blob in bodies.items():
        if kind=='clear':
            at=physical(clear,start);m[at:at+len(blob)]=blob
    for at in FILL_SITES:put16(m,clear,at+3,system)
def build(bodies,*,args=(2,4,1,2),mode=1,**options):
    m,c=prior.build({k:v for k,v in bodies.items() if k[0] in ('caller','sys')},**options)
    install(m,bodies);c.cs=CLEAR;c.ip=ENTRY
    for j,v in enumerate(args):put16(m,c.ss,(c.sp+4+2*j)&65535,0xA500|(v&255))
    for off in (12,14):put16(m,c.ss,(c.sp+off)&65535,0xA5A5)
    m[physical(c.ds,0x4A1E)]=mode
    for seg,off,blob in options.get('writes',()):m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,limit=65536,refuse=None,rect_limit=65536):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None);d=Diagnostic()
    status=lib.pr_r0709_clear_0dd3_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),limit,rect_limit,ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def compare(name,state,bodies,decoded,lib,limit=65536,refuse=None,rect_limit=65536):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    names=[n for n,_ in Diagnostic._fields_]
    s=dict.fromkeys(names,-1);s.update(stop=None,ret=False,loops=0,rect_loops=0,last=None)
    calls=[];seen=set();edges=set();events=[];writes=[]
    def stop(v):s['stop']=v;uc.emu_stop()
    def rect_stop(v):s['status']=v;stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)
        key=('clear' if cs==CS and ENTRY<=ip<END else 'caller' if cs==CS else 'sys' if cs==SYS else '?',ip)
        if s['ret']:
            if not calls:return stop(0)
            expected,owner=calls.pop()
            if (cs,ip)!=expected:
                if owner=='rect':return rect_stop(5)
                return stop(5)
            s['ret']=False
        check(key in decoded,f'escape {name} {cs:04x}:{ip:04x}')
        ins=decoded[key]
        if key[0]=='clear' and ip in FILL_SITES+RECT_SITES:
            expected=(bytes([0xE8])+struct.pack('<H',(0xB7-ip-3)&65535)) if ip in RECT_SITES else bytes.fromhex('9aa816')
            if bytes(uc.mem_read(address,3))!=expected:return stop(2)
        if key==('caller',0x140) and bytes(uc.mem_read(address,3))!=bytes.fromhex('9aa816'):return rect_stop(2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:
            if ip<0x71:s['rect_status']=2;return rect_stop(3)
            return rect_stop(4)
        if key[0]=='clear' and ip in (0xE6E,0xF7A) and not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40:
            if s['loops']==limit:return stop(6)
            s['loops']+=1
        if key==('caller',0x14F) and not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40:
            if s['rect_loops']==rect_limit:return rect_stop(6)
            s['rect_loops']+=1
        if s['last'] is not None:edges.add((s['last'],key))
        seen.add(key);s['last']=key if ins.mnemonic in ('ja','jne','je','jbe','jb') else None
        if key[0] in ('clear','caller') and ins.mnemonic in ('call','lcall'):
            owner='clear' if key[0]=='clear' else 'rect';calls.append(((cs,ip+ins.size),owner))
            if owner=='clear':
                s['callee']=1 if ip==0xE8E else 2 if ip==0xEA8 else 0;s['status']=0
                if ip in RECT_SITES:s['rect_callee']=s['rect_status']=-1;s['rect_loops']=0
            else:s['rect_callee']=2 if ip==0x140 else 1 if ip==0x100 else 0;s['rect_status']=0
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
        at=physical(CLEAR if kind=='clear' else CS if kind=='caller' else SYS,start)
        check(bytes(backing[at:at+len(blob)])==m[at:at+len(blob)],f'code mutation outside contract {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,d=run_c(lib,m,actual,limit,refuse,rect_limit)
    check(status==s['stop'],f'status {name}: {status}/{s}')
    check(tuple(getattr(d,n) for n in names)==tuple(s[n] for n in names),f'diagnostic {name}: {[getattr(d,n) for n in names]}/{s}')
    check(mine==events,f'OUTs {name}: {mine}/{events}')
    check(stores==writes,f'VRAM writes {name}: {stores[:3]}/{writes[:3]} ({len(stores)}/{len(writes)})')
    expected=bytes(backing)
    check(ram[:0xA0000]+ram[0xB0000:]==expected[:0xA0000]+expected[0xB0000:],f'RAM {name}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n} {getattr(actual,n):04x}/{want:04x}')
    return status,seen,edges,len(events),len(writes)
def cases(quick=False):
    result=[(f'mode{m}',dict(mode=m)) for m in (0,1,2,3,255)]
    for m in range(3):
        result += [(f'm{m}-one-cell',dict(mode=m,args=(0,0,0,0))),
          (f'm{m}-zero-width',dict(mode=m,args=(0,0,0,1))),
          (f'm{m}-signed',dict(mode=m,args=(255,255,255,255))),
          (f'm{m}-reversed-rows',dict(mode=m,args=(0,2,1,2),rect_limit=1)),
          (f'm{m}-budget0',dict(mode=m,limit=0,rect_limit=0))]
    result += [('counter-alias',dict(mode=0,ss=0xB800,sp=0x8E,args=(0,25,0,25),limit=4)),
               ('return-alias',dict(mode=0,ss=0xB800,sp=0x50,args=(0,39,0,0)))]
    if quick:return result
    result += [('reached-one-up-box',dict(args=(22,38,17,1))),('reached-row-five',dict(args=(5,38,5,17)))]
    for ip in FILL_SITES+RECT_SITES:
        result.append((f'operand-{ip:x}',dict(mode=0 if ip in FILL_SITES[:2] else 2 if ip in FILL_SITES else 1,writes=((CS,ip+1,bytes(2)),))))
    for active in range(8):
        for flags in FLAGS:result.append((f'active{active}-flags{flags:x}',dict(active=active,flags=flags)))
    for n in range(44):result.append((f'port{n}',dict(refuse=n)))
    for sp in (0x3E,0x3F,0x8001,0xFFF3,0xFFF4):result.append((f'sp-{sp:x}',dict(sp=sp)))
    for m in (0,2):
        for args in ((128,255,128,255),(127,0,127,255),(255,127,254,126),(0,0,0,2)):
            result.append((f'm{m}-sign-{args}',dict(mode=m,args=args,limit=0)))
    for ip in FILL_SITES+RECT_SITES:
        result.append((f'code-{ip:x}',dict(mode=0 if ip in FILL_SITES[:2] else 2 if ip in FILL_SITES else 1,writes=((CLEAR,ip,bytes([0x90])),))))
    result += [('rect-code',dict(writes=((CS,0x140,bytes([0x90])),))),
       ('ss-ds',dict(ss=DS)),('mode-stack-alias',dict(ss=DS,sp=0x4A20)),('a20-stack',dict(ss=0xFFFF)),
       ('full-screen',dict(args=(24,39,0,0))),('rect-wrap-width',dict(args=(0,0,0,2),rect_limit=0))]
    rng=random.Random(0x3F10137)
    for n in range(36):
        x,y=rng.randrange(40),rng.randrange(25)
        result.append((f'random-{n}',dict(mode=n%3,args=(rng.randrange(y,25),rng.randrange(x,40),y,x),active=rng.randrange(8),flags=FLAGS[n%6])))
    return result
def differential(bodies,decoded,lib,quick=False):
    visited=set();edges=set();stops={};out_count=write_count=0
    specs=cases(quick)
    for n,(name,options) in enumerate(specs):
        options=dict(options);limit=options.pop('limit',65536);refuse=options.pop('refuse',None);rect_limit=options.pop('rect_limit',65536)
        print('BEGIN',n+1,name,flush=True)
        status,v,e,outs,writes=compare(name,build(bodies,**options),bodies,decoded,lib,limit,refuse,rect_limit)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;out_count+=outs;write_count+=writes
    caller={k for k in decoded if k[0]=='clear' and ENTRY<=k[1]<END}
    outcomes={e for e in edges if e[0] in caller}
    if not quick:check(caller<=visited,'caller instruction coverage')
    return dict(cases=len(specs),stops=stops,caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(outcomes),
        uncovered_starts=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
        uncovered_outcomes=[f'{k}:{ip:04X}->{dest:04X}' for k,ip in sorted(caller) if decoded[k,ip].mnemonic in ('ja','jne','je','jbe') for dest in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,dest)) not in edges],
        chain_starts=len(visited),out_events=out_count,vram_write_operands=write_count,cpu_words=14,ram_bytes=SIZE-65536,flag_masks=0)
def main():
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='clear0dd3-') as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from res0709_clear_0dd3_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['res0709_clear_0dd3_cpu_audit']=sys.modules[__name__]
    main()
