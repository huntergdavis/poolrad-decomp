#!/usr/bin/env python3
"""Original/C zero/first-external gate; all media stays ignored."""
import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from unicorn import UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from u6f_vector_merge_032f_audit import (
    Cpu,FIELDS,REGISTERS,RAM_SIZE,physical,put_word,put_far,check,reachable_caller,OVR_SHA,
)
from u6f_prefix_0482_audit import emulator
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack,relocations
ROOT=Path(__file__).resolve().parents[2]
EXE_SHA='d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
BODY_SHA='244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047'
RELEASE_SHA='d3cd5846fe2109fc1eb33b500561830c73601d21bea64575f7a08488301eb700'
class Pending(ctypes.Structure):
    _fields_=[(k,ctypes.c_uint16) for k in ('call_ip','continuation_ip','target_cs','target_ip')]

def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media identity mismatch')
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x3ce)
    check(u['memory_segment']==0x3e9 and u['entries'][3]==0x14e,'public entry mismatch')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    body=code[0x14e:0x1b0];cfg=reachable_caller(code,0x14e,0x1b0)
    check(len(body)==98 and len(cfg)==40 and sum(i.size for i in cfg.values())==98 and sha256(body).hexdigest()==BODY_SHA,'executable extent mismatch')
    check([(a,i.op_str) for a,i in sorted(cfg.items()) if i.mnemonic=='lcall']==[(0x17a,'0x464, 0x2595'),(0x1a5,'0x464, 0x2595')],'external calls mismatch')
    check(all(a+3 in u['fixups'] for a in (0x17a,0x1a5)),'call segment fixup missing')
    check(cfg[0x1ad].mnemonic=='retf' and cfg[0x1ad].op_str=='2','return mismatch')
    check(len([a for a in cfg if a>=0x17f])==20 and sha256(body[49:]).hexdigest()=='fdbc2e759f77faaae556c294375ca453b738ed32cd4962186b1cbe52d6680e67','independent continuation identity mismatch')
    image=unpack(exe)[0];release=bytes(image[0x6bd5:0x6c72])
    check(len(release)==157 and sha256(release).hexdigest()==RELEASE_SHA,'resident release extent mismatch')
    decoded=reachable_caller(image[0x4640:],0x2595,0x2632)
    check(len(decoded)==52 and sum(i.size for i in decoded.values())==157,'release CFG mismatch')
    calls=[(a,i.op_str) for a,i in sorted(decoded.items()) if i.mnemonic=='lcall']
    check(calls==[(0x25f0,'0xaf8, 0x254'),(0x2605,'0xaf8, 0x254'),(0x261b,'0xaf8, 0x254')],'first-free call set mismatch')
    relocs=set(relocations(exe,image)[0])
    check(all(0x4640+a+3 in relocs for a,_ in calls),'resident call relocation missing')
    print('U3CE body: 98 bytes/40 starts; +017F follow-on 49 bytes; RETF 2 or pre-call +017A/+01A5')
    print('resident follow-on: 157 bytes/52 starts; first SYSTEM:0254 stop +25F0/+2605/+261B, or nil RETF 4')
    return body,cfg,release

def build(path):
    library=path/'gate.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-fPIC','-shared','-I',str(ROOT/'include'),str(ROOT/'src/recovered/overlay_u3ce_gate_014e.c'),'-o',str(library)],check=True)
    lib=ctypes.CDLL(str(library));lib.pr_u3ce_gate_014e.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Pending)]
    lib.pr_u3ce_gate_014e.restype=ctypes.c_int;return lib

def fixture(body,*,entry=0x14e,slot=0,first=0,second=0,sp=0x8000,ds=0x3000,ss=0x5000,cs=0x8000,seed=0,after_sp=None):
    rng=random.Random(seed);m=bytearray([rng.randrange(256)])*RAM_SIZE
    c=Cpu(*(rng.randrange(65536) for _ in FIELDS));c.cs=cs;c.ip=entry;c.ds=ds;c.ss=ss
    c.flags=0x7202 | rng.randrange(0x1000)&0xcd5
    c.sp=sp if entry==0x14e else (sp-4)&0xffff
    parent_bp=c.bp
    if entry==0x17f:c.bp=(sp-2)&0xffff
    at=physical(cs,0x14e);m[at:at+98]=body
    for site in (0x17a,0x1a5):put_word(m,cs,site+3,0x1234)
    table=0x5e5a+8*slot
    put_far(m,ds,table,((first>>16)&0xffff,first&0xffff))
    put_far(m,ds,table+4,((second>>16)&0xffff,second&0xffff))
    if entry==0x17f:put_word(m,ss,(sp-2)&0xffff,parent_bp)
    put_far(m,ss,sp,(0x9000,0x200));put_word(m,ss,(sp+4)&0xffff,0xab00|slot)
    if after_sp is not None:c.sp=after_sp
    return m,c

def compare(label,m,c,body,cfg,lib):
    uc,back=emulator(m,c);stop=[None];visited=set();ret=physical(c.cs,0x1ad)
    def hook(u,address,size,data):
        visited.add(address-(c.cs<<4))
        if address in (physical(c.cs,0x17a),physical(c.cs,0x1a5),ret):stop[0]=address-(c.cs<<4);u.emu_stop()
    handle=uc.hook_add(UC_HOOK_CODE,hook)
    uc.emu_start(physical(c.cs,c.ip),0x110000,count=1000)
    check(stop[0] is not None,label+': original budget exhausted')
    uc.hook_del(handle)
    status=0 if stop[0]==0x1ad else 1
    if status==0:uc.emu_start(ret,0x110000,count=1)
    at=physical(c.cs,0x14e)
    check(bytes(back[at:at+98])==bytes(m[at:at+98]),label+': immutable code violated')
    actual=(ctypes.c_uint8*RAM_SIZE).from_buffer_copy(m);cpu=Cpu.from_buffer_copy(c)
    pending=Pending(0xdead,0xbeef,0xaaaa,0x5555)
    got=lib.pr_u3ce_gate_014e(actual,RAM_SIZE,ctypes.byref(cpu),ctypes.byref(pending))
    check(got==status,label+': status mismatch')
    check(bytes(actual)==bytes(back),label+': full 1 MiB RAM mismatch')
    for name,reg in zip(FIELDS,REGISTERS):
        value=uc.reg_read(reg)&0xffff
        check(getattr(cpu,name)==value,f'{label}: {name} C={getattr(cpu,name):04x} original={value:04x}')
    if status:
        check(tuple(getattr(pending,k) for k,_ in Pending._fields_)==(stop[0],stop[0]+5,0x1234,0x2595),label+': callback mismatch')
    else:check(bytes(pending)==bytes(Pending(0xdead,0xbeef,0xaaaa,0x5555)),label+': returned path changed pending')
    return visited,status,stop[0]

def release_gate(body):
    count=0;routes=set();route_counts={}
    for mode in (0,1,2,255):
        for planes in (0,1,255):
            for stride in (0,1,65535):
                for auxiliary in (0,0x2000,0x70000000,0x70002000):
                    m=bytearray(RAM_SIZE);c=Cpu(*([0]*14));c.cs=0x8000;c.ip=0x2595;c.ss=0x5000;c.sp=0x8000;c.ds=0x3000;c.bp=0x1357;c.flags=0x7202
                    m[physical(c.cs,c.ip):physical(c.cs,c.ip)+157]=body
                    put_far(m,c.ss,c.sp,(0x9000,0x200));put_far(m,c.ss,c.sp+4,(c.ds,0x2000))
                    put_far(m,c.ds,0x2000,(0x6000,0x1000))
                    m[physical(0x6000,0x1008)]=planes;put_word(m,0x6000,0x1011,stride)
                    put_far(m,0x6000,0x1013,(auxiliary>>16,auxiliary&0xffff));m[physical(c.ds,0x4a1e)]=mode
                    uc,back=emulator(m,c);stopped=[None];writes=[]
                    def hook(u,address,size,data):
                        ip=address-(c.cs<<4)
                        if ip in (0x25f0,0x2605,0x261b,0x262f):stopped[0]=ip;u.emu_stop()
                    h=uc.hook_add(UC_HOOK_CODE,hook)
                    w=uc.hook_add(UC_HOOK_MEM_WRITE,lambda u,access,a,s,v,d:writes.extend(range(a,a+s)))
                    uc.emu_start(physical(c.cs,c.ip),0x110000,count=1000)
                    expected=0x261b if not auxiliary else 0x25f0 if mode==1 else 0x2605
                    check(stopped[0]==expected,'resident first-heap boundary mismatch')
                    sp=uc.reg_read(REGISTERS[7]);at=physical(c.ss,sp)
                    words=tuple(int.from_bytes(bytes(back[at+i:at+i+2]),'little') for i in (0,2,4))
                    product=(planes*stride)&65535
                    size=((product>>2) if mode==1 else product) if auxiliary else (product+23)&65535
                    block=(auxiliary&65535,auxiliary>>16) if auxiliary else (0x1000,0x6000)
                    check(words==(size,*block),'resident FreeMem argument mismatch')
                    check(set(writes)<={physical(c.ss,c.sp-i) for i in range(1,15)},'resident prefix wrote outside stack')
                    uc.hook_del(h);uc.hook_del(w);routes.add(stopped[0]);route_counts[stopped[0]]=route_counts.get(stopped[0],0)+1;count+=1
        # Nil caller pointer: separately execute its RETF 4, never any FreeMem.
        m=bytearray(RAM_SIZE);c=Cpu(*([0]*14));c.cs=0x8000;c.ip=0x2595;c.ss=0x5000;c.sp=0x8000;c.ds=0x3000;c.bp=0x1357;c.flags=0x7202
        m[physical(c.cs,c.ip):physical(c.cs,c.ip)+157]=body
        put_far(m,c.ss,c.sp,(0x9000,0x200));put_far(m,c.ss,c.sp+4,(c.ds,0x2000))
        m[physical(c.ds,0x4a1e)]=mode
        uc,back=emulator(m,c);stop=[False]
        def ret_hook(u,address,size,data):
            if address==physical(c.cs,0x262f):stop[0]=True;u.emu_stop()
        h=uc.hook_add(UC_HOOK_CODE,ret_hook)
        uc.emu_start(physical(c.cs,c.ip),0x110000,count=1000);check(stop[0],'nil resident return missing')
        uc.hook_del(h);uc.emu_start(physical(c.cs,0x262f),0x110000,count=1)
        check((uc.reg_read(REGISTERS[8]),uc.reg_read(REGISTERS[9]),uc.reg_read(REGISTERS[7]),uc.reg_read(REGISTERS[6]))==(0x9000,0x200,0x8008,0x1357),'resident nil RETF 4 ABI mismatch')
        count+=1
    check(route_counts=={0x25f0:27,0x2605:81,0x261b:36},'resident route case counts mismatch')
    print('resident external stop counts',{hex(k):v for k,v in sorted(route_counts.items())},'nil returns',4)
    print(f'resident original boundary probes: {count}; first-free sites {sorted(hex(x) for x in routes)}, nil RETF 4; no heap call executed')

def main():
    p=argparse.ArgumentParser();p.add_argument('--quick',action='store_true');p.add_argument('--source-only',action='store_true');args=p.parse_args()
    body,cfg,release=source();release_gate(release)
    if args.source_only:return
    cases=[];slots=(0,1,7,127,255) if args.quick else range(256)
    for slot in slots:
        for first,second in ((0,0),(0x60001000,0),(0,0x70002000),(0x60001000,0x70002000)):
            cases.append((f'entry-slot{slot}-{first:x}-{second:x}',dict(slot=slot,first=first,second=second)))
        for second in (0,0x70002000):cases.append((f'post-slot{slot}-{second:x}',dict(entry=0x17f,slot=slot,first=0x60001000,second=second)))
    for sp in (0,1,2,3,4,5,6,7,8,9,65533,65534,65535):
        for first,second in ((0,0),(1,0),(0,1),(1,1)):cases.append((f'sp{sp}-{first}-{second}',dict(sp=sp,slot=255,first=first,second=second)))
    for name,options in (('a20-data',dict(ds=0xffff)),('a20-stack',dict(ss=0xffff)),('a20-shared',dict(ds=0xffff,ss=0xffff)),('a20-ret',dict(ss=0xffff,sp=65535)),('shared-code-data',dict(ds=0x8000)),('shared-code-stack-data',dict(ds=0x8000,ss=0x8000))):
        for first,second in ((0,0),(1,0),(0,1),(1,1)):cases.append((f'{name}-{first}-{second}',dict(first=first,second=second,**options)))
    for first,second in ((1,0),(0x60000000,0),(0,1),(0,0x70000000)):cases.append((f'partial-{first:x}-{second:x}',dict(first=first,second=second)))
    for sp in (0x5e5c,0x5e5e,0x5e60,0x5e62,0x5e64):cases.append((f'live-stack-table-{sp:x}',dict(ds=0x5000,sp=sp)))
    for sp in (0,1,65535):
        for second in (0,1):cases.append((f'post-sp{sp}-{second}',dict(entry=0x17f,after_sp=sp,second=second)))
    for seed in range(8 if args.quick else 64):
        r=random.Random(seed)
        for entry in (0x14e,0x17f):cases.append((f'random-{seed}-{entry:x}',dict(entry=entry,seed=seed+100,slot=r.randrange(256),first=r.choice((0,r.randrange(1,1<<32))),second=r.choice((0,r.randrange(1,1<<32))))))
    visited=set();counts={0:0,1:0};stops={};post=0
    with tempfile.TemporaryDirectory() as d:
        lib=build(Path(d))
        for label,options in cases:
            seen,status,stop=compare(label,*fixture(body,**options),body,cfg,lib)
            visited.update(seen);counts[status]+=1;stops[stop]=stops.get(stop,0)+1;post+=options.get('entry',0x14e)==0x17f
    print(f'{len(cases)} original/C cases: full 1 MiB RAM/all 14 CPU words equal; no flag mask; {post} independently entered +017F cases')
    print('stops',counts,'source boundaries',{hex(k):v for k,v in sorted(stops.items())})
    print(f'covered gate instruction starts {len(visited)}/{len(cfg)}; external CALL instructions are stop sites, not executed')

if __name__=='__main__':main()
