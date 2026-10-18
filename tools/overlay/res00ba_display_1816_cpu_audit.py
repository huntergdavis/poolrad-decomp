#!/usr/bin/env python3
"""Raw 00BA +1816 display composition: original CPU/RAM/OUT/VRAM operands, native planes.
Original media and generated guest programs stay under ignored resources/."""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
CS,DS,SS,SYS=0x8800,0x5000,0x2000,0x6000
ENTRY,END=0x1816,0x1892
DISPLAY=0x8000
LOOPS={0x2CA:0x279,0x351:0x300,0x381:0x267,0x45B:0x3BD,0x468:0x3B4,0x5BC:0x4B2,0x5C8:0x4A5,0x5D4:0x49C,0x5E2:0x235}
FONT=0x7000
STRING=0x4000
import res0709_printer_0626_cpu_audit as prior
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SOURCES=('resident_00ba_display_1816_cpu','system_string_leaves_cpu')+prior.SOURCES
WRITE_FN=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.c_uint16,ctypes.c_uint8)
class Ega(ctypes.Structure):
    _fields_=[('plane',(ctypes.c_uint8*65536)*4),('latch',ctypes.c_uint8*4),('seq_index',ctypes.c_uint8),('seq',ctypes.c_uint8*5),('gc_index',ctypes.c_uint8),('gc',ctypes.c_uint8*9)]
class Bus(ctypes.Structure):
    _fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('ega',ctypes.POINTER(Ega)),('ports',ctypes.POINTER(Env)),('on_vram_write',WRITE_FN),('ctx',ctypes.c_void_p)]
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('callee','status','printer_callee','printer_status','glyph_callee','glyph_status')]
def source():
    bodies,decoded,meta=prior.source()
    image=bytes(unpack((ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes())[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    for seg,a,b,n,digest in ((0xBA,ENTRY,END,49,'645caa0e81651dc9b2be1f156ea343d47f83a195f64f84a5c6fd828a89f50175'),
       (0xAF8,0xB0B,0xB25,12,'4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f')):
        blob=image[physical(seg,a):physical(seg,b)];ins=list(md.disasm(blob,a))
        check(len(ins)==n and sha256(blob).hexdigest()==digest,f'identity {seg:x}:{a:x}')
        kind='display' if seg==0xBA else 'sys';bodies[kind,a]=blob;decoded.update({(kind,i.address):i for i in ins})
        meta[f'{seg:04X}:{a:04X}']=dict(bytes=len(blob),instructions=n,sha256=digest)
    literal=image[physical(0xBA,0x1813):physical(0xBA,0x1816)]
    check(sha256(literal).hexdigest()=='4bd4cdfadef19a0803e203a5c05b37a6a3e79e76d25454fa559743b912409844','literal identity')
    bodies['data',0x1813]=literal
    return bodies,decoded,meta
def library(directory):
    path=directory/'display.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_r00ba_display_1816_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,cs=CS,system=SYS,display=DISPLAY):
    prior.install(m,{k:v for k,v in bodies.items() if k[0] in ('caller','sys')},cs,system)
    for (kind,start),blob in bodies.items():
        if kind in ('display','data'):
            at=physical(display,start);m[at:at+len(blob)]=blob
    for at,segment in ((0x1856,cs),(0x1885,system),(0x188A,cs)):put16(m,display,at,segment)
def build(bodies,*,args=(1,2,3),fields=(0,0),text=b'AbZ',source_seg=STRING,source_off=0x800,suffix=None,**options):
    m,c=prior.build({k:v for k,v in bodies.items() if k[0] in ('caller','sys')},text=text,source_seg=source_seg,source_off=source_off,**options)
    install(m,bodies);c.cs=DISPLAY;c.ip=ENTRY
    for j,v in enumerate((*[0xA500|(x&255) for x in args],source_off,source_seg)):put16(m,c.ss,(c.sp+4+2*j)&65535,v)
    for j,v in enumerate(fields):m[physical(source_seg,(source_off+0x10D+j)&65535)]=v
    if suffix is not None:
        check(len(suffix)==2,'suffix length');m[physical(DISPLAY,0x1813):physical(DISPLAY,0x1816)]=bytes([2])+suffix
    for seg,off,blob in options.get('writes',()):m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,limit=65536,refuse=None,glyph_limit=65536):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None);d=Diagnostic()
    status=lib.pr_r00ba_display_1816_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),limit,glyph_limit,ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def compare(name,state,bodies,decoded,lib,limit=65536,refuse=None,glyph_limit=65536):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    names=[n for n,_ in Diagnostic._fields_]
    s=dict.fromkeys(names,-1);s.update(stop=None,ret=False,loops=0,glyph_loops=0,last=None)
    calls=[];seen=set();edges=set();events=[];writes=[]
    def stop(v):s['stop']=v;uc.emu_stop()
    def printer_stop(v):s['status']=v;stop(3)
    def glyph_stop(v):s['printer_status']=v;printer_stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)
        key=('display' if cs==DISPLAY else 'caller' if cs==CS else 'sys' if cs==SYS else '?',ip)
        if s['ret']:
            if not calls:return stop(0)
            expected,owner=calls.pop()
            if (cs,ip)!=expected:
                if owner=='glyph':return glyph_stop(5)
                if owner=='printer':return printer_stop(5)
                return stop(5)
            s['ret']=False
        check(key in decoded,f'escape {name} {cs:04x}:{ip:04x}')
        ins=decoded[key]
        if key[0]=='display' and ip in (0x1853,0x1882,0x1887):
            dest=0xB0B if ip==0x1882 else 0x626
            if bytes(uc.mem_read(address,3))!=bytes([0x9A])+struct.pack('<H',dest):return stop(2)
        if key==('caller',0x63C) and bytes(uc.mem_read(address,3))!=bytes.fromhex('9a250b'):return printer_stop(2)
        if key==('caller',0x691) and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return printer_stop(2)
        if key==('caller',0x207) and bytes(uc.mem_read(address,3))!=bytes.fromhex('9a2a18'):return glyph_stop(2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return glyph_stop(4)
        if key==('caller',0x69F) and not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40:
            if s['loops']==limit:return printer_stop(6)
            s['loops']+=1
        if key[0]=='caller' and ip in LOOPS and (ins.mnemonic=='jmp' or not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40):
            if s['glyph_loops']==glyph_limit:return glyph_stop(6)
            s['glyph_loops']+=1
        if s['last'] is not None:edges.add((s['last'],key))
        seen.add(key);s['last']=key if ins.mnemonic in ('ja','jne','je','jbe','jb') else None
        if key[0] in ('display','caller') and ins.mnemonic in ('call','lcall'):
            owner='display' if key[0]=='display' else 'printer' if ip in (0x63C,0x691) else 'glyph'
            calls.append(((cs,ip+ins.size),owner))
            if owner=='display':
                s['callee']={0x1853:0,0x1882:1,0x1887:2}[ip];s['status']=0
                if ip!=0x1882:
                    for n in names[2:]:s[n]=-1
                    s['loops']=0
            elif ip==0x63C:s['printer_callee']=0;s['printer_status']=0
            elif ip==0x691:
                s['printer_callee']=1;s['printer_status']=0;s['glyph_callee']=s['glyph_status']=-1;s['glyph_loops']=0
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
        at=physical(DISPLAY if kind in ('display','data') else CS if kind=='caller' else SYS,start)
        check(bytes(backing[at:at+len(blob)])==m[at:at+len(blob)],f'code/literal mutation outside contract {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,d=run_c(lib,m,actual,limit,refuse,glyph_limit)
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
    result=[(f'm{m}-f{flag}-color{n}',dict(mode=m,args=(flag,2,3),fields=fields)) for m in range(4) for flag in (0,1) for n,fields in enumerate(((0,0),(1,1),(1,2)))]
    result += [('empty',dict(text=b'')),('x-out',dict(args=(1,2,40))),('y-out',dict(args=(1,40,3))),
      ('y25',dict(args=(1,25,3))),('late-length',dict(text=b'A',source_off=0x283,fields=(1,2),pattern=2,pages=(STRING,STRING+0x200))),
      ('printer-limit',dict(limit=0)),('glyph-limit',dict(glyph_limit=0))]
    if quick:return result
    for f in FLAGS:result.append((f'flags-{f:x}',dict(flags=f,text=bytes([0,63,64,96,97,255]))))
    for n in (0,1,31,33,34,67,101,102,103,135,169):result.append((f'port{n}',dict(refuse=n)))
    for m in range(3):
        for cap in (0,1,7,31,32):result.append((f'm{m}-glyph-cap{cap}',dict(mode=m,glyph_limit=cap)))
    for sp in (0x144,0x145,0x8001,0xFFF1,0xFFF2):result.append((f'sp-{sp:x}',dict(sp=sp)))
    for ip in (0x1853,0x1882,0x1887,0x63C,0x691,0x207):
        result.append((f'code-{ip:x}',dict(writes=((DISPLAY if ip>=ENTRY else CS,ip,bytes([0x90])),))))
    result += [('second-glyph-code',dict(text=b'',writes=((CS,0x207,bytes([0x90])),))),
      ('second-printer-limit',dict(text=b'A',limit=0)),('second-glyph-limit',dict(text=b'',glyph_limit=0)),
      ('max-length',dict(mode=3,text=bytes(range(255)))),('x-wrap',dict(args=(1,2,39),text=bytes(range(220)))),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('node-wrap',dict(source_off=0xFFFE)),
      ('node-copy-alias',dict(source_seg=SS,source_off=0x7EEA)),
      ('forward-overlap',dict(source_seg=SS,source_off=0x7EE9,text=b'ABCD')),
      ('backward-overlap',dict(source_seg=SS,source_off=0x7EEB,text=b'ABCD')),
      ('shared-pages',dict(pages=(0xA000,0xA000))),('ram-pages',dict(pages=(0x3000,0x3100))),
      ('font-wrap',dict(fontoff=0xFFFE)),('no-font',dict(writes=((DS,0x5E20,bytes(4)),))),
      ('synthetic-suffix',dict(suffix=b'!?'))]
    rng=random.Random(0xBA1816)
    for n in range(36):result.append((f'random-{n}',dict(mode=n%3,args=(rng.randrange(256),rng.randrange(40),rng.randrange(40)),fields=(rng.randrange(3),rng.randrange(3)),text=bytes(rng.randrange(256) for _ in range(rng.randrange(10))),flags=FLAGS[n%6])))
    return result
def differential(bodies,decoded,lib,quick=False):
    visited=set();edges=set();stops={};out_count=write_count=0
    specs=cases(quick)
    for n,(name,options) in enumerate(specs):
        options=dict(options);limit=options.pop('limit',65536);refuse=options.pop('refuse',None);glyph_limit=options.pop('glyph_limit',65536)
        print('BEGIN',n+1,name,flush=True)
        status,v,e,outs,writes=compare(name,build(bodies,**options),bodies,decoded,lib,limit,refuse,glyph_limit)
        visited|=v;edges|=e;stops[str(status)]=stops.get(str(status),0)+1;out_count+=outs;write_count+=writes
    caller={k for k in decoded if k[0]=='display' and ENTRY<=k[1]<END}
    outcomes={e for e in edges if e[0] in caller}
    if not quick:check(caller<=visited,'caller instruction coverage')
    return dict(cases=len(specs),stops=stops,caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(outcomes),
        uncovered_starts=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
        uncovered_outcomes=[f'{k}:{ip:04X}->{dest:04X}' for k,ip in sorted(caller) if decoded[k,ip].mnemonic in ('ja','jne','je','jbe') for dest in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,dest)) not in edges],
        chain_starts=len(visited),out_events=out_count,vram_write_operands=write_count,cpu_words=14,ram_bytes=SIZE-65536,flag_masks=0)
def main():
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='display1816-') as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from res00ba_display_1816_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['res00ba_display_1816_cpu_audit']=sys.modules[__name__]
    main()
