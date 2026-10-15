#!/usr/bin/env python3
"""Bounded +0482 original/C parity and separate U3CE:+014E source gate.

No original bytes or game strings are embedded; owner media stays ignored.
"""
import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import sys
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from u6f_vector_merge_032f_audit import (
    Cpu, FIELDS, REGISTERS, RAM_SIZE, physical, put_word, put_far,
    reachable_caller, check, OVR_SHA,
)
from resident_loader_map import unpack
from overlay_inventory import find_units, parse_mz

ROOT=Path(__file__).resolve().parents[2]
EXE_SHA='d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
PREFIX_SHA='50e6f7712a661af42cc1cd4a0fee30b1e6aded0a63762789497a966d6d392acf'
HELPERS=(
    (0x802,0x94a,19,'9cb0afed6f73df887b61f47778db5d55bd9063b59c8f6067ab0c38ec3350d205'),
    (0xaf8,0xb0b,26,'4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f'),
    (0xaf8,0xc3b,18,'b2f0ef7b50632ff33ad1b2b251db935542c81b596360ac8b7cd6b391f3fca484'),
    (0xaf8,0xb98,44,'c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64'),
)

class Pending(ctypes.Structure):
    _fields_=[(name,ctypes.c_uint16) for name in
              ('call_ip','continuation_ip','target_cs','target_ip')]


def source_audit():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,
          'media identity mismatch')
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)}
    u=units[0x6f];code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    full=reachable_caller(code,0x482,0x674)
    check(len(full)==183 and sum(i.size for i in full.values())==498 and sha256(code[0x482:0x674]).hexdigest()=='be72ede6980e67ba2ef0d51e84c154a281ab9b27b9d00070b7701fcf4bd0a4a3', 'complete caller CFG mismatch')
    prefix=code[0x482:0x4dd]
    check(len(prefix)==91 and sha256(prefix).hexdigest()==PREFIX_SHA,'prefix extent mismatch')
    decoder=Cs(CS_ARCH_X86,CS_MODE_16)
    starts=list(decoder.disasm(prefix,0x482))
    check(starts[-1].address==0x4d8 and starts[-1].mnemonic=='lcall' and
          starts[-1].op_str=='0x3e9, 0x34','callback stop target mismatch')
    calls=[(i.address,i.op_str) for i in starts if i.mnemonic=='lcall']
    check(calls==[(0x48c,'0x802, 0x94a'),(0x4a6,'0xaf8, 0xb0b'),
                  (0x4c2,'0xaf8, 0xc3b'),(0x4c7,'0xaf8, 0xb98'),
                  (0x4d8,'0x3e9, 0x34')],'closed call sequence mismatch')
    check(all(a+3 in u['fixups'] for a,_ in calls),'prefix segment fixup missing')
    check(not any(i.mnemonic.startswith('j') for i in starts),'prefix acquired a branch')
    image=unpack(exe)[0];helpers=[]
    for seg,ip,size,digest in HELPERS:
        body=bytes(image[seg*16+ip:seg*16+ip+size])
        check(sha256(body).hexdigest()==digest,'closed helper identity mismatch')
        instructions=list(decoder.disasm(body,ip))
        check(instructions[-1].mnemonic=='retf' and
              not any('call' in i.mnemonic for i in instructions),'closed helper is not a leaf')
        helpers.append((seg,ip,body,instructions))
    child=units[0x3ce]
    check(child['memory_segment']==0x3e9 and child['entries'][3]==0x14e and
          child['entries'][4]==0x1c8,'U3CE public entry identity mismatch')
    child_code=ovr[child['ovr_offset']:child['ovr_offset']+child['code_size']]
    child_cfg=reachable_caller(child_code,0x14e,0x1b0)
    child_body=child_code[0x14e:0x1b0]
    check(sha256(child_body).hexdigest()=='244d2bbd214ceb86d9d6daa80252da6af77fa0a92e4a287ac555aaca26b3c047', 'U3CE executable hash mismatch')
    check(sum(i.size for i in child_cfg.values())==98 and
          [(a,i.op_str) for a,i in sorted(child_cfg.items()) if i.mnemonic=='lcall']==
          [(0x17a,'0x464, 0x2595'),(0x1a5,'0x464, 0x2595')],
          'U3CE conditional external-call boundary mismatch')
    check(all(a+3 in child['fixups'] for a in (0x17a,0x1a5)), 'U3CE fixup missing')
    check(child_cfg[0x1ad].mnemonic=='retf' and child_cfg[0x1ad].op_str=='2',
          'U3CE entry return mismatch')
    print(f'prefix: 86 executed caller bytes/{len(starts)-1} starts, then pre-call 04D8; four hash-locked closed helpers')
    print(f'U3CE +014E: 98 executable bytes/{len(child_cfg)} starts, RETF 2; conditional calls +017A/+01A5 to 0464:2595')
    print('U3CE executable SHA-256',sha256(child_body).hexdigest())
    return code,starts,helpers,child_code,child_cfg


def build_c(directory):
    path=directory/'prefix.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror',
                    '-fPIC','-shared','-I',str(ROOT/'include'),
                    str(ROOT/'src/recovered/overlay_u6f_prefix_0482.c'),'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path))
    lib.pr_u6f_prefix_0482.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,
                                   ctypes.POINTER(Cpu),ctypes.POINTER(Pending)]
    lib.pr_u6f_prefix_0482.restype=ctypes.c_int
    return lib


def fixture(code,helpers,*,length=11,index=7,char=0x51,slot=4,sp=0x8000,
            ds=0x3000,ss=0x5000,cs=0x8000,resident=0x802,system=0xaf8,seed=0):
    rng=random.Random(seed);memory=bytearray([rng.randrange(256)])*RAM_SIZE
    cpu=Cpu(*(rng.randrange(65536) for _ in FIELDS))
    cpu.cs=cs;cpu.ip=0x482;cpu.ds=ds;cpu.ss=ss;cpu.sp=sp
    cpu.flags=0x7202 | (rng.randrange(2)<<10) | (rng.randrange(2)<<0)
    at=physical(cs,0);memory[at:at+len(code)]=code
    for seg,ip,body,_ in helpers:
        loaded=resident if seg==0x802 else system
        at=physical(loaded,ip);memory[at:at+len(body)]=body
    for site,segment in ((0x48c,resident),(0x4a6,system),(0x4c2,system),(0x4c7,system)):
        put_word(memory,cs,site+3,segment)
    memory[physical(cs,0x476)]=length
    put_far(memory,ds,0x5d92,(0x6000,0x1000))
    memory[physical(0x6000,0x10c0)]=index
    memory[physical(0x6000,0x10bd)]=slot
    memory[physical(ds,0x8c5+index)]=char
    memory[physical(ds,0x5376)]=0x72
    memory[physical(ds,0x5377)]=0x39
    put_word(memory,ss,sp,0x200);put_word(memory,ss,(sp+2)&0xffff,0x9000)
    put_word(memory,ss,(sp+4)&0xffff,0xab06)
    return memory,cpu


def emulator(memory,cpu):
    backing=(ctypes.c_uint8*RAM_SIZE).from_buffer_copy(memory)
    uc=Uc(UC_ARCH_X86,UC_MODE_16);ptr=ctypes.addressof(backing)
    uc.mem_map_ptr(0,RAM_SIZE,7,ptr);uc.mem_map_ptr(RAM_SIZE,0x10000,7,ptr)
    for name,reg in zip(FIELDS,REGISTERS):uc.reg_write(reg,getattr(cpu,name))
    return uc,backing


def compare(label,memory,cpu,starts,helpers,library):
    uc,backing=emulator(memory,cpu)
    parent_cs=cpu.cs
    allowed={physical(parent_cs,i.address) for i in starts}
    ret_sites={};code_ranges=[(physical(parent_cs,0x482),91)]
    for seg,ip,body,instructions in helpers:
        loaded=memory[physical(parent_cs,0x48f)] | (memory[physical(parent_cs,0x490)]<<8) if seg==0x802 else memory[physical(parent_cs,0x4a9)] | (memory[physical(parent_cs,0x4aa)]<<8)
        allowed.update(physical(loaded,i.address) for i in instructions)
        ret_sites[physical(loaded,instructions[-1].address)]=int(instructions[-1].op_str,0)
        code_ranges.append((physical(loaded,ip),len(body)))
    stop=[None];ignore=[False];visited=set()
    def hook(u,address,size,data):
        if ignore[0]:return
        visited.add(address)
        if address==physical(parent_cs,0x4d8):stop[0]='callback';u.emu_stop()
        elif address in ret_sites:stop[0]='retf';u.emu_stop()
        elif address not in allowed:stop[0]='unexpected';u.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook)
    next_address=physical(cpu.cs,cpu.ip);return_index=0;status=None
    while status is None:
        stop[0]=None;uc.emu_start(next_address,0x110000,count=10000)
        check(stop[0] is not None,label+': original budget exhausted')
        if stop[0]=='callback':status=1;break
        check(stop[0]=='retf',label+': original escaped bounded instructions')
        ret_address=physical(uc.reg_read(REGISTERS[8]),uc.reg_read(REGISTERS[9]))
        ss=uc.reg_read(REGISTERS[12]);sp=uc.reg_read(REGISTERS[7])
        if sp>=0xfffe:
            # Direct original-byte native probes pin this rule for all four helpers.
            # Unicorn fetches the CS word linearly instead of wrapping its offset.
            def stack_word(offset):
                at=physical(ss,offset)
                return backing[at] | (backing[(at+1)&0xfffff]<<8)
            ip=stack_word(sp);cs=stack_word((sp+2)&0xffff)
            uc.reg_write(REGISTERS[8],cs);uc.reg_write(REGISTERS[9],ip)
            uc.reg_write(REGISTERS[7],(sp+4+ret_sites[ret_address])&0xffff)
        else:
            ignore[0]=True;uc.emu_start(ret_address,0x110000,count=1);ignore[0]=False
        expected=(0x491,0x4ab,0x4c7,0x4cc)[return_index];return_index+=1
        if uc.reg_read(REGISTERS[8])!=parent_cs or uc.reg_read(REGISTERS[9])!=expected:
            status=2;break
        next_address=physical(parent_cs,expected)
    for at,size in code_ranges:
        check(bytes(backing[at:at+size])==bytes(memory[at:at+size]),label+': immutable code violated')
    actual=(ctypes.c_uint8*RAM_SIZE).from_buffer_copy(memory);out=Cpu.from_buffer_copy(cpu)
    pending=Pending(0xdead,0xbeef,0xaaaa,0x5555)
    got=library.pr_u6f_prefix_0482(actual,RAM_SIZE,ctypes.byref(out),ctypes.byref(pending))
    check(got==status,label+': stop status mismatch')
    check(bytes(actual)==bytes(backing),label+': full 1 MiB RAM mismatch')
    for name,reg in zip(FIELDS,REGISTERS):
        check(getattr(out,name)==(uc.reg_read(reg)&0xffff),label+': CPU '+name+' mismatch')
    if status==1:
        check(tuple(getattr(pending,k) for k,_ in Pending._fields_)==
              (0x4d8,0x4dd,0x3e9,0x34),label+': pending call mismatch')
    else:check(bytes(pending)==bytes(Pending(0xdead,0xbeef,0xaaaa,0x5555)),label+': control stop changed pending')
    return visited,status


def child_gate(child_code,child_cfg):
    count=0
    for slot in (0,1,255):
        for first,second in ((0,0),(1,0),(0,1),(1,1)):
            memory=bytearray(RAM_SIZE);cpu=Cpu(*([0]*14))
            cpu.cs=0x8000;cpu.ip=0x14e;cpu.ds=0x3000;cpu.ss=0x5000;cpu.sp=0x8000;cpu.bp=0x1234;cpu.flags=0x7202
            memory[0x80000:0x80000+len(child_code)]=child_code
            put_far(memory,cpu.ss,cpu.sp,(0x9000,0x200));put_word(memory,cpu.ss,cpu.sp+4,0xab00|slot)
            table=0x5e5a+8*slot
            put_far(memory,cpu.ds,table,(0x6000,0x1000) if first else (0,0))
            put_far(memory,cpu.ds,table+4,(0x6000,0x2000) if second else (0,0))
            uc,backing=emulator(memory,cpu);stopped=[None];writes=[]
            def stop(u,address,size,data):
                if address in {physical(cpu.cs,x) for x in (0x17a,0x1a5,0x1ad)}:
                    stopped[0]=address-(cpu.cs<<4);u.emu_stop()
            hook=uc.hook_add(UC_HOOK_CODE,stop)
            uc.hook_add(UC_HOOK_MEM_WRITE,lambda u,access,a,s,v,d:writes.extend(range(a,a+s)))
            uc.emu_start(physical(cpu.cs,cpu.ip),0x110000,count=1000)
            expected=0x17a if first else 0x1a5 if second else 0x1ad
            check(stopped[0]==expected,'U3CE first side-effect boundary mismatch')
            stack={physical(cpu.ss,(cpu.sp-i)&0xffff) for i in range(1,9)}
            check(set(writes)<=stack,'U3CE pre-call wrote outside its stack')
            if first or second:
                sp=uc.reg_read(REGISTERS[7]);at=physical(cpu.ss,sp)
                off=int.from_bytes(bytes(backing[at:at+2]),'little')
                seg=int.from_bytes(bytes(backing[at+2:at+4]),'little')
                check((seg,off)==(cpu.ds,table+(0 if first else 4)), 'U3CE free argument is not address of pointer pair')
            else:
                uc.hook_del(hook)
                uc.emu_start(physical(cpu.cs,0x1ad),0x110000,count=1)
                check((uc.reg_read(REGISTERS[8]),uc.reg_read(REGISTERS[9]),uc.reg_read(REGISTERS[7]),uc.reg_read(REGISTERS[6]))==(0x9000,0x200,0x8006,0x1234), 'U3CE zero-pointer RETF 2 ABI mismatch')
            count+=1
    print(f'U3CE boundary probes: {count}; zero/first/second/both pointer guards, slots 0/1/255; stack-only writes before callback')


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--quick',action='store_true');parser.add_argument('--source-only',action='store_true');parser.add_argument('--boundary-only',action='store_true');args=parser.parse_args()
    code,starts,helpers,child_code,child_cfg=source_audit();child_gate(child_code,child_cfg)
    if args.source_only:return
    cases=[]
    lengths=() if args.boundary_only else (0,1,10,11,254,255) if args.quick else range(256)
    for length in lengths:cases.append((f'length-{length}',dict(length=length)))
    indexes=() if args.boundary_only else (0,1,127,128,255) if args.quick else range(256)
    for index in indexes:cases.append((f'index-{index}',dict(index=index,char=(index*37+11)&255)))
    for sp in (0,1,2,3,0x2a,0x7fff,0xfffd,0xfffe,0xffff,*range(0x229,0x23c)):cases.append((f'sp-{sp:04x}',dict(sp=sp)))
    for sp in (0x8000,0x6000,0x5fbe,0x55a5,0x55a9,0x55ab):cases.append((f'shared-stack-{sp:04x}',dict(ds=0x5000,sp=sp)))
    cases.append(('relocated-helpers',dict(resident=0x1200,system=0x1800)))
    for label,options in (('a20-ds',dict(ds=0xffff)),('a20-ss',dict(ss=0xffff)),('a20-shared',dict(ds=0xffff,ss=0xffff)),('a20-stack-wrap',dict(ss=0xffff,sp=0x231)),('shared-code-data',dict(ds=0x8000)),('shared-code-stack-data',dict(ds=0x8000,ss=0x8000))):cases.append((label,options))
    for seed in range(0 if args.boundary_only else 8 if args.quick else 128):
        rng=random.Random(seed)
        cases.append((f'seed-{seed}',dict(seed=seed+100,length=rng.randrange(256),index=rng.randrange(256),char=rng.randrange(256),slot=rng.randrange(256))))
    visited=set();statuses={1:0,2:0}
    with tempfile.TemporaryDirectory() as temporary:
        lib=build_c(Path(temporary))
        for label,options in cases:
            seen,status=compare(label,*fixture(code,helpers,**options),starts,helpers,lib)
            visited.update(seen);statuses[status]+=1
    print(f'{len(cases)} original/C prefix cases: full 1 MiB RAM and all 14 CPU words equal; no flag mask')
    print(f'stops: {statuses[1]} unresolved-call callbacks, {statuses[2]} changed closed-helper returns; no unresolved callee executed')
    helper_starts={physical(seg,i.address) for seg,ip,body,instructions in helpers for i in instructions}
    if not args.boundary_only:check(helper_starts<=visited,'a closed helper start was not reached')
    print(f'covered closed helper starts {len(helper_starts & visited)}/{len(helper_starts)}')
    print(f'covered caller starts {sum(physical(0x8000,i.address) in visited for i in starts[:-1])}/{len(starts)-1}')

if __name__=='__main__':main()
