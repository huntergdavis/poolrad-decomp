#!/usr/bin/env python3
"""CPU14/whole-RAM original-chain audit of U7F entry13."""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
from unicorn import x86_const as R
from overlay_inventory import find_units,parse_mz
from u25_string_cores_audit import Cpu,NAMES,REGS,physical,put16
from u5e_party_value_2879_audit import EXE_SHA,OVR_SHA
ROOT=Path(__file__).resolve().parents[2]
SIZE=1<<20
ENTRY=0x1010
END=0x18E0
SYSTEM=0xAF8
LOOPS=(0x1608,0x181A,0x18AA)
SOURCES=('overlay_u7f_caller_1010','overlay_u7f_roll_class_0fd2_cpu',
 'overlay_u9e_entry8_fullstate_compose_0de3','overlay_u9e_entry8_fullstate_prefix_0de3',
 'overlay_u9e_entry8_fullstate_postcall_0e13','overlay_u9e_entry8_fullstate_loop_0e05',
 'overlay_u9e_entry8_fullstate_tail_0e1f','overlay_u9e_entry8_system_call_0e0e',
 'system_random_fullstate_11c0','system_seed_fullstate_120f','system_runtime_leaves_cpu')
PINS=(('u7f',ENTRY,END,826,'2509b684aaa48da83c6d04bcc270cf3e4b731cd521f08b70ef9fc9f45a2bacd9'),
 ('u7f',0xFD2,0x1010,27,'9205417006da7a4c928b547d116107e3f4d269d3d267d17b2f7cad2f3e63fe27'),
 ('dice',0xDE3,0xE2E,29,'7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba'),('sys',0x11C0,0x11D6,10,'e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9'),('sys',0x120F,0x1245,22,'f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242'),
 ('data',0x1245,0x1247,0,'d9221ae2959bbc366aaadbb08e200b236d45dbaa8bf463cd13da45a9115baa0d'),
 ('sys',0x16A8,0x16BC,7,'f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7'))

def source():
    p=ROOT/'resources/DOS/POOLRAD';e,o=(p/'START.EXE').read_bytes(),(p/'GAME.OVR').read_bytes()
    assert sha256(e).hexdigest()==EXE_SHA and sha256(o).hexdigest()==OVR_SHA
    units={u['stub_para']:u for u in find_units(e[parse_mz(e)['header_bytes']:],o)}
    assert units[0x7F]['entries'][13]==ENTRY and units[0x7F]['ovr_offset']==0x24C89
    assert units[0x9E]['entries'][8]==0xDE3
    bodies={};decoded={}
    for kind,start,end,count,digest in PINS:
        offset=0xAC76 if kind in ('sys','data') else units[0x7F if kind=='u7f' else 0x9E]['ovr_offset']
        data=(e if kind in ('sys','data') else o)[offset+start:offset+end]
        ins=[] if kind=='data' else list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(data,start))
        assert not count or len(ins)==count
        assert not digest or sha256(data).hexdigest()==digest
        bodies[kind,start]=data;decoded.update({(kind,i.address):i for i in ins})
        print(kind,f'{start:04X}',len(data),len(ins),sha256(data).hexdigest(),flush=True)
    fixups=[f for f in units[0x7F]['fixups'] if ENTRY<=f<END]
    assert fixups==[i.address+3 for (k,a),i in decoded.items() if k=='u7f' and ENTRY<=a<END and i.mnemonic=='lcall']
    assert all(i.mnemonic not in ('call','lcall') or i.op_str in ('0xfd2','0xb0, 0x48','0xaf8, 0x16a8','0xaf8, 0x11c0','0x120f') for i in decoded.values())
    return bodies,decoded

CHECKER_ROWS = ('OVL_007F_1010',)


def library(directory):
    p=directory/'caller.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC','-I',str(ROOT/'include')]+[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES]+['-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u7f_caller_1010
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_size_t,ctypes.c_uint];f.restype=ctypes.c_int
    return f

def fixture(*,kind=0,seed=0,cs=0x3000,dice=0x4000,ss=0x2000,ds=0x5000,
            sp=0xF000,flags=0x602,record_seg=None,record_off=0x8000,**unused):
    del unused
    rng=random.Random(seed);m=bytearray([0xA5])*SIZE
    c=Cpu(*(rng.randrange(65536) for _ in NAMES));c.cs,c.ip,c.ss,c.sp,c.bp,c.ds,c.flags=cs,ENTRY,ss,sp,0x7777,ds,flags
    # Synthetic table: no original game data copied.
    for i in range(512):put16(m,ds,0x970+2*i,(i*317+19)&65535)
    put16(m,ds,0x4428,seed&65535);put16(m,ds,0x442A,seed>>16)
    for i,w in enumerate((0x9000,0xA000,record_off,ds if record_seg is None else record_seg,kind)):
        put16(m,ss,sp+2*i,w)
    return m,c,dice

def install(m,c,dice,bodies,decoded):
    for (kind,start),data in bodies.items():
        blob=bytearray(data)
        for (k,ip),ins in decoded.items():
            if k==kind and start<=ip<start+len(data) and ins.mnemonic=='lcall' and ins.op_str=='0xb0, 0x48':
                blob[ip-start+1:ip-start+5]=(0x48).to_bytes(2,'little')+((dice+0x100)&65535).to_bytes(2,'little')
        for i,v in enumerate(blob):m[physical({'u7f':c.cs,'dice':dice,'sys':SYSTEM,'data':SYSTEM}[kind],start+i)]=v

    stub=bytes((0xEA,0xE3,0x0D,dice&255,dice>>8))
    for i,v in enumerate(stub):m[physical(dice+0x100,0x48+i)]=v

def compare(label,state,bodies,decoded,f,budgets=(1024,256)):
    m,c,dice=state;install(m,c,dice,bodies,decoded)
    backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for n,r in zip(NAMES,REGS):uc.reg_write(r,getattr(c,n))
    visited,edges=set(),set();previous=[None];remaining=list(budgets);stop=[None];calls=[];stepping=[False]
    def hook(uc,address,size,user):
        del address,size,user
        if stepping[0]:return
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP)&65535
        kind='u7f' if cs==c.cs else 'dice' if cs==dice else 'sys' if cs==SYSTEM else None;key=(kind,ip)
        if cs==dice+0x100 and ip==0x48:return
        assert key in decoded,(label,'unexpected PC',hex(cs),hex(ip))
        index=0 if kind=='u7f' and ip in LOOPS else 1 if key==('dice',0xE0E) else None
        if index is not None:
            if not remaining[index]:stop[0]=index+2;uc.emu_stop();return
            remaining[index]-=1
        if previous[0] is not None:edges.add((previous[0],key))
        ins=decoded[key];visited.add(key);previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):
            target=(kind,int(ins.op_str,16)) if ins.mnemonic=='call' else ('dice',0xDE3) if ins.op_str=='0xb0, 0x48' else ('sys',0x16A8 if ins.op_str=='0xaf8, 0x16a8' else 0x11C0)
            calls.append(((cs,ip+ins.size),target))
            if target==('dice',0xDE3):remaining[1]=budgets[1]
        if ins.mnemonic in ('retf','ret'):stop[0]='ret';uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE,hook);status=0
    while True:
        stop[0]=None;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=2000000)
        assert stop[0] is not None,(label,'original instruction limit')
        if stop[0]!='ret':status=stop[0];break
        stepping[0]=True;uc.emu_start(physical(uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)),2*SIZE,count=1);stepping[0]=False
        if not calls:break
        expected,target=calls.pop()
        if (uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP))!=expected:
            status=4 if target in (('sys',0x120F),('sys',0x11C0)) or calls and calls[-1][1]==('u7f',0xFD2) else 5;break
    for (kind,start),data in bodies.items():
        seg={'u7f':c.cs,'dice':dice,'sys':SYSTEM,'data':SYSTEM}[kind]
        assert all(backing[physical(seg,start+i)]==m[physical(seg,start+i)] for i in range(len(data))),(label,'code modified')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);actual=Cpu.from_buffer_copy(c)
    got=f(ram,SIZE,ctypes.byref(actual),dice,*budgets)
    assert got==status,(label,'status',got,status)
    assert bytes(ram)==bytes(backing),(label,'RAM',[(hex(i),ram[i],backing[i]) for i in range(SIZE) if ram[i]!=backing[i]][:12])
    for n,r in zip(NAMES,REGS):assert getattr(actual,n)==uc.reg_read(r)&65535,(label,n,hex(getattr(actual,n)),hex(uc.reg_read(r)&65535))
    return visited,edges,status,actual

def specifications(quick=False):
    types=(0,1,21,61,62,63,69,70,71,77,78,92,93,255) if quick else range(256)
    cases=[(f'type-{t}',dict(kind=t,seed=0x12345678)) for t in types]
    for seed in range(16 if quick else 256):
        for t in (21,61,62,69,70,71,77,78):cases.append((f'seed-{t}-{seed}',dict(kind=t,seed=seed)))
    for sp in (0x40,0x41,0xFFF5,0xFFF6):
        for t in (0,1,61,63):cases.append((f'stack-{sp:x}-{t}',dict(sp=sp,kind=t)))
    for t in (61,63):
        for budget in (1,2,3,4,5):cases.append((f'caller-limit-{t}-{budget}',dict(kind=t,budgets=(budget,256))))
    for t in (1,61):cases.append((f'dice-limit-{t}',dict(kind=t,budgets=(1024,0))))
    cases.extend([('A20-data',dict(record_seg=0xFFFF,record_off=0x400)),('word-wrap',dict(record_off=0xFFCA,kind=1)),('A20-code',dict(cs=0xFFFF)),('A20-stack',dict(ss=0xFFFF,sp=0x200,kind=61)),('relocated',dict(cs=0x7000,dice=0x8000,kind=61))])
    return cases

def alias_specifications():
    return [('fill-overwrites-return',dict(record_seg=0x2000,record_off=0xEFE8)),
      ('fill-overwrites-return-cs',dict(record_seg=0x2000,record_off=0xEFEA)),
      ('record-over-seed',dict(kind=61,record_off=0x4428)),
      ('record-over-table',dict(kind=63,record_off=0x9F8)),
      ('physical-word-seam',dict(kind=1,record_off=0xFFC8)),
      ('physical-word-seam-A20',dict(kind=1,record_seg=0xFFFF,record_off=0xFFC8)),
      ('shared-DS-SS',dict(kind=61,ss=0x5000,seed=0xFFFFFFFF)),
      ('three-rolls-limit',dict(kind=61,seed=0xFFFFFFFF,budgets=(1,256)))]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--quick',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--aliases-only',action='store_true');p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded=source();visited=set();edges=set();statuses={};stops={};count=0
    with tempfile.TemporaryDirectory() as tmp:
        f=library(Path(tmp))
        from loaded_stub_checks import verify_refusals
        m,c,dice=fixture();install(m,c,dice,bodies,decoded)
        stub_guards=verify_refusals(m,c,lambda ram,state: f(ram,SIZE,ctypes.byref(state),dice,1024,256),
                                    [(c.cs,p) for p in (0x10C7,0x1596,0x1611,0x1630,0x1649,0x1662,0x167E,0x1692,0x16AB,0x170F,0x174B,0x17A1,0x17E6)])
        print("LOADED_STUB_GUARDS",stub_guards,flush=True)
        for label,kw in (alias_specifications() if a.aliases_only else specifications(a.quick)):
            kw=dict(kw);budgets=kw.pop('budgets',(1024,256));v,e,s,c=compare(label,fixture(**kw),bodies,decoded,f,budgets)
            visited.update(v);edges.update(e);statuses[str(s)]=statuses.get(str(s),0)+1;count+=1
            if s:stops[f'{s}:{c.cs:04X}:{c.ip:04X}']=stops.get(f'{s}:{c.cs:04X}:{c.ip:04X}',0)+1
            if count%50==0:print('Compared',count,label,flush=True)
    caller={k for k in decoded if k[0]=='u7f' and ENTRY<=k[1]<END}
    cond={k for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    report=dict(loaded_stub_guards=stub_guards,cases=count,statuses=statuses,stops=stops,caller_sites=len(caller&visited),caller_total=len(caller),chain_sites=len(visited),chain_total=len(decoded),caller_edges=len([e for e in edges if e[0] in caller]),caller_edges_total=2*len(cond&caller),chain_edges=len(edges),chain_edges_total=2*len(cond),unvisited_sites=[f'{k}:{a:04X}' for k,a in sorted(set(decoded)-visited)],missing_edges=[f'{k}:{a:04X}->{dest:04X}' for k,a in sorted(cond) for dest in (int(decoded[k,a].op_str,16),a+decoded[k,a].size) if ((k,a),(k,dest)) not in edges],flags_masks=0,emulator_substitutions=0)
    print(json.dumps(report,indent=2))
    if a.output:a.output.write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
