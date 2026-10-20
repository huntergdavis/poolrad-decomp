#!/usr/bin/env python3
"""U3B0 +06FB row clear composition: original CPU/RAM/OUT/VRAM operands, native planes.
Original media and generated guest programs stay under ignored resources/."""
import argparse,ctypes,json,random,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
CS,DS,SS,SYS=0x8800,0x5000,0x2000,0x6000
OVERLAY=0x4000
ENTRY,END=0x6FB,0x711
LOOPS={0x2CA:0x279,0x351:0x300,0x381:0x267,0x45B:0x3BD,0x468:0x3B4,0x5BC:0x4B2,0x5C8:0x4A5,0x5D4:0x49C,0x5E2:0x235}
FONT=0x7000
import res0709_space_05eb_cpu_audit as prior
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import OVR_SHA
from res00ba_display_1693_cpu_audit import PythonEga
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
CHECKER_ROWS = ('OVL_03B0_06FB',)
SOURCES=('overlay_u3b0_row_clear_06fb_cpu',)+prior.SOURCES
WRITE_FN=ctypes.CFUNCTYPE(None,ctypes.c_void_p,ctypes.c_uint16,ctypes.c_uint8)
class Ega(ctypes.Structure):
    _fields_=[('plane',(ctypes.c_uint8*65536)*4),('latch',ctypes.c_uint8*4),('seq_index',ctypes.c_uint8),('seq',ctypes.c_uint8*5),('gc_index',ctypes.c_uint8),('gc',ctypes.c_uint8*9)]
class Bus(ctypes.Structure):
    _fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('ega',ctypes.POINTER(Ega)),('ports',ctypes.POINTER(Env)),('on_vram_write',WRITE_FN),('ctx',ctypes.c_void_p)]
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('space_status','callee','status','glyph_callee','glyph_status')]
def source():
    bodies,decoded,meta=prior.source()
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest()==OVR_SHA,'GAME identity')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x3B0)
    check(u['entries'][3]==ENTRY,'entry3 identity')
    blob=ovr[u['ovr_offset']+ENTRY:u['ovr_offset']+END]
    ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(blob,ENTRY))
    digest='b306a980ebf183c4dd57a62414c019d47d68787e6f6577d61da8bec63e9fc09b'
    check(len(ins)==13 and sum(i.size for i in ins)==22 and sha256(blob).hexdigest()==digest,'06FB identity')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(0x70A,'0x709, 0x5eb')],'caller dependency')
    bodies['overlay',ENTRY]=blob;decoded.update({('overlay',i.address):i for i in ins})
    meta['U3B0:06FB']=dict(bytes=len(blob),instructions=len(ins),sha256=digest,file_offset=u['ovr_offset']+ENTRY)
    return bodies,decoded,meta
def library(directory):
    path=directory/'printer.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_u3b0_row_clear_06fb_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_size_t,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies,cs=CS,system=SYS):
    prior.install(m,{k:v for k,v in bodies.items() if k[0]!='overlay'},cs,system)
    for (kind,start),blob in bodies.items():
        if kind=='overlay':
            at=physical(OVERLAY,start);m[at:at+len(blob)]=blob
    put16(m,OVERLAY,0x70D,cs)
def build(bodies,*,ax=None,**options):
    resident={k:v for k,v in bodies.items() if k[0]!='overlay'}
    m,c=prior.prior.build(resident,args=(),**options);install(m,bodies);c.ip=ENTRY;c.cs=OVERLAY
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
    status=lib.pr_u3b0_row_clear_06fb_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),limit,ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def compare(name,state,bodies,decoded,lib,limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);device=PythonEga()
    def vread(em,off,size,user):return sum(device.read((off+j)&65535)<<(8*j) for j in range(size))
    def vwrite(em,off,size,value,user):
        for j in range(size):device.write((off+j)&65535,(value>>(8*j))&255)
    for origin in (0,SIZE):  # the aperture is an independent device; all other backing RAM is flat, including the unused A0000..AFFFF backing bytes
        uc.mem_map_ptr(origin,0xA0000,7,ctypes.addressof(backing));uc.mmio_map(origin+0xA0000,65536,vread,None,vwrite,None)
        uc.mem_map_ptr(origin+0xB0000,0x50000,7,ctypes.addressof(backing)+0xB0000)
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    s=dict(stop=None,ret=False,space_status=-1,callee=-1,status=-1,glyph_callee=-1,glyph_status=-1,loops=0,glyph_loops=0,last=None)
    calls=[];seen=set();edges=set();events=[];writes=[]
    def stop(v):s['stop']=v;uc.emu_stop()
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=('overlay' if cs==OVERLAY else 'caller' if cs==CS else 'sys' if cs==SYS else '?',ip)
        if s['ret']:
            if not calls:return stop(0)
            expected,owner=calls.pop()
            if (cs,ip)!=expected:
                if owner=='glyph':s['status']=5;s['space_status']=3;return stop(3)
                if owner=='space':s['space_status']=5;return stop(3)
                return stop(4)
            s['ret']=False
        check(key in decoded,f'escape {name} {cs:04x}:{ip:04x}')
        ins=decoded[key]
        if key==('overlay',0x70A) and bytes(uc.mem_read(address,3))!=bytes.fromhex('9aeb05'):return stop(2)
        if key==('caller',0x61F) and bytes(uc.mem_read(address,3))!=bytes.fromhex('e8bdfb'):s['space_status']=2;return stop(3)
        if key==('caller',0x207) and bytes(uc.mem_read(address,3))!=bytes.fromhex('9a2a18'):s['status']=2;s['space_status']=3;return stop(3)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:s['status']=4;s['space_status']=3;return stop(3)
        if key[0]=='caller' and ip in LOOPS and (ins.mnemonic=='jmp' or not uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40):
            if s['glyph_loops']==limit:s['status']=6;s['space_status']=3;return stop(3)
            s['glyph_loops']+=1
        if s['last'] is not None:edges.add((s['last'],key))
        seen.add(key);s['last']=key if ins.mnemonic in ('ja','jne','je','jbe','jb') else None
        if key[0] in ('caller','overlay') and ins.mnemonic in ('call','lcall'):
            owner='overlay' if key[0]=='overlay' else 'space' if ip==0x61F else 'glyph'
            calls.append(((cs,ip+ins.size),owner))
            if key[0]=='overlay':s['space_status']=0
            elif ip==0x61F:
                s['callee']=0;s['status']=0;s['glyph_callee']=s['glyph_status']=-1;s['glyph_loops']=0
            else:s['glyph_callee']=0;s['glyph_status']=0
        if ins.mnemonic=='retf':s['ret']=True
    def out(em,port,size,value,user):
        events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)));device.out(port,value,size)
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=3000000)
    check(s['stop'] is not None,f'instruction ceiling {name}')
    for (kind,start),blob in bodies.items():
        at=physical(OVERLAY if kind=='overlay' else CS if kind=='caller' else SYS,start)
        check(bytes(backing[at:at+len(blob)])==m[at:at+len(blob)],f'code mutation outside contract {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,d=run_c(lib,m,actual,limit,refuse)
    check(status==s['stop'],f'status {name}: {status}/{s}')
    check((d.space_status,d.callee,d.status,d.glyph_callee,d.glyph_status)==tuple(s[k] for k in ('space_status','callee','status','glyph_callee','glyph_status')),f'diagnostic {name}: space={d.space_status}, {d.callee}/{d.status}/{d.glyph_callee}/{d.glyph_status}, {s}')
    check(mine==events,f'OUTs {name}: {mine}/{events}')
    check(stores==device.writes,f'VRAM writes {name}: {stores[:3]}/{device.writes[:3]} ({len(stores)}/{len(device.writes)})')
    expected=bytes(backing)
    if ram!=expected:  # the whole 1,048,576-byte backing array, A0000..AFFFF included
        diff=[(hex(i),x,y) for i,(x,y) in enumerate(zip(ram,expected)) if x!=y]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    if ega!=device.packed():
        want=device.packed();diff=[(hex(i),x,y) for i,(x,y) in enumerate(zip(ega,want)) if x!=y]
        per=[sum(1 for i in range(q*65536,(q+1)*65536) if ega[i]!=want[i]) for q in range(4)]
        raise ValueError(f'EGA {name}: {diff[:8]} ({len(diff)} bytes) planes={per} C-regs={list(ega[262144:])} model-regs={list(want[262144:])} reads={len(device.reads)}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n} {getattr(actual,n):04x}/{want:04x}')
    return status,seen,edges,len(events),len(device.writes)
def cases(quick=False):
    result=[(f'mode{m}',dict(mode=m)) for m in (0,1,2,3)]
    result += [('caller-opcode',dict(writes=((OVERLAY,0x70A,b'\x90'),))),
      ('caller-operand',dict(writes=((OVERLAY,0x70B,bytes(2)),))),
      ('near-code',dict(writes=((CS,0x61F,b'\x90'),))),
      ('near-operand',dict(writes=((CS,0x620,bytes(2)),))),
      ('glyph-code',dict(writes=((CS,0x207,b'\x90'),))),('glyph-limit0',dict(limit=0))]
    if quick:return result
    for m in range(3):
        for limit in (1,7,31,32,100,1000):result.append((f'm{m}-limit{limit}',dict(mode=m,limit=limit)))
    for flags in FLAGS:result.append((f'flags{flags:x}',dict(flags=flags)))
    for ah in (0,1,0x7F,0x80,0xFF):result.append((f'ah{ah:x}',dict(ax=(ah<<8)|0x93)))
    for n in (0,1,2,32,33,34,35,100,1000,1359):result.append((f'port{n}',dict(refuse=n)))
    for sp in (0x3A,0x3B,0x8001,0xFFFB,0xFFFC):result.append((f'sp{sp:x}',dict(sp=sp)))
    result += [('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),
      ('mode-stack-alias',dict(ss=DS,sp=0x4A20)),
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
    caller={k for k in decoded if k[0]=='overlay' and ENTRY<=k[1]<END}
    outcomes={e for e in edges if e[0] in caller}
    if not quick:
        check(caller<=visited,'caller instruction coverage');check(len(outcomes)==0,'caller has no conditional branches')
    return dict(cases=len(specs),stops=stops,caller_starts=len(caller&visited),caller_starts_total=len(caller),caller_outcomes=len(outcomes),caller_outcomes_feasible=0,caller_outcomes_syntactic=0,
        uncovered_starts=[f'{k}:{ip:04X}' for k,ip in sorted(set(decoded)-visited)],
        uncovered_outcomes=[f'{k}:{ip:04X}->{dest:04X}' for k,ip in sorted(caller) if decoded[k,ip].mnemonic in ('ja','jne','je','jbe','jb') for dest in (int(decoded[k,ip].op_str,16),ip+decoded[k,ip].size) if ((k,ip),(k,dest)) not in edges],
        chain_starts=len(visited),out_events=out_count,vram_write_operands=write_count,cpu_words=14,ram_bytes=SIZE,ram_excluded_ranges=[],ega_bytes=len(PythonEga().packed()),flag_masks=0)
def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--quick',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u3b0-06fb-') as tmp:
        lib=library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        bodies,decoded,meta=source();report=dict(target=meta)
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from u3b0_row_clear_06fb_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':
    sys.modules['u3b0_row_clear_06fb_cpu_audit']=sys.modules[__name__]
    main()
