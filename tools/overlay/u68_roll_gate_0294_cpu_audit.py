#!/usr/bin/env python3
"""Raw upgrade of U68 [0294,04DF): full-state original-byte and native evidence.
Only exact executable spans are extracted; original media stays in resources/."""
import argparse, ctypes, json, random, struct, subprocess, tempfile
from pathlib import Path
from hashlib import sha256
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16
import u33_grid_pair_0d13_audit as dice
import u33_cell_lookup_012b_audit as lookup

CS, DICE, FIELD, DS, SS = 0x3000, 0x4000, 0x7000, 0x1000, 0x2000
ENTRY, END = 0x0294, 0x04DF
ROLLS = (0x300,0x319,0x33D,0x35A,0x373,0x38C,0x3C2,0x3E0,0x3FD,0x425,0x442,0x45B)
MARKS = (0x491,0x4CF)
SOURCES = ("overlay_u68_roll_gate_0294", "overlay_u68_map_update_0109_cpu",
           "overlay_u3bd_field", "overlay_u3bd_nibble", "overlay_u3bd_lookup_fullstate",
           "overlay_u3bd_in_grid_031f", "overlay_u9e_entry8_fullstate_compose_0de3",
           "overlay_u9e_entry8_fullstate_prefix_0de3", "overlay_u9e_entry8_fullstate_loop_0e05",
           "overlay_u9e_entry8_fullstate_postcall_0e13", "overlay_u9e_entry8_fullstate_tail_0e1f",
           "overlay_u9e_entry8_system_call_0e0e", "system_random_fullstate_11c0", "system_seed_fullstate_120f")
FLAGS = (0x0602,0x0202,0x0ED7,0x0647,0x72D7,0x0403,0x0213)
check = dice.check
class Diagnostic(ctypes.Structure):
    _fields_ = [("callee",ctypes.c_int),("status",ctypes.c_int)]

def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media identity')
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)}
    u=units[0x68]
    specs=[('caller',0x68,ENTRY,END,247,'8a2393e6e0632118ba5094510ee1d8aa5a0a3a2c84ef2eb1ad27a0f85844ef48'),
           ('map',0x68,0x109,0x236,125,None),
           ('range',0x3BD,0x31F,0x350,None,lookup.LEAF_SHA),
           ('field',0x3BD,0x350,0x47B,120,lookup.TARGETS['0350']['sha']),
           ('nibble',0x3BD,0x47B,0x57A,103,lookup.TARGETS['047b']['sha']),
           ('dice',0x9E,0xDE3,0xE2E,None,None)]
    # Reuse the merged extraction's independent identity pins, not coverage.
    _,chain=dice.source()
    bodies={}; decoded={}; meta={}
    md=Cs(CS_ARCH_X86,CS_MODE_16)
    for name,unit,first,end,count,digest in specs:
        blob=ovr[units[unit]['ovr_offset']+first:units[unit]['ovr_offset']+end]
        ins=list(md.disasm(blob,first))
        check(sum(i.size for i in ins)==len(blob) and (count is None or len(ins)==count),name+' extent')
        if digest:check(sha256(blob).hexdigest()==digest,name+' identity')
        if name=='dice':check(blob==chain['dice'][:end-first],'dice prefix identity')
        check(ins[-1].mnemonic=='retf' and ins[-1].address+ins[-1].size==end,name+' ends at RETF')
        bodies[name]=(first,blob); decoded.update({(name,i.address):i for i in ins})
        meta[name]=dict(bytes=len(blob),instructions=len(ins),sha256=sha256(blob).hexdigest(),file_offset=units[unit]['ovr_offset']+first)
    for name,first,blob in (('random',0x11C0,chain['system'][:22]),('seed',0x120F,chain['system'][0x4F:0x85])):
        ins=list(md.disasm(blob,first))
        check(sum(i.size for i in ins)==len(blob),name+' extent')
        bodies[name]=(first,blob);decoded.update({(name,i.address):i for i in ins})
        meta[name]=dict(bytes=len(blob),instructions=len(ins),sha256=sha256(blob).hexdigest())
    operand=chain['system'][0x85:0x87]
    check(struct.unpack('<H',operand)[0]==0x8405,'SYSTEM live multiplier')
    bodies['seed_multiplier']=(0x1245,operand)
    meta['seed_multiplier']=dict(bytes=2,instructions=0,role='data',sha256=sha256(operand).hexdigest())
    calls=[(i.address,i.mnemonic,i.op_str) for (name,ip),i in decoded.items() if name=='caller' and 'call' in i.mnemonic]
    check(calls==[(0x2D4,'lcall','0x3d5, 0x2f')]+[(s,'lcall','0xb0, 0x48') for s in ROLLS]+[(s,'call','0x109') for s in MARKS],'call sites')
    check(all(s+3 in u['fixups'] for s in (0x2D4,*ROLLS)),'far call fixups')
    check(ovr[u['ovr_offset']+END]==0x55,'next prologue')
    return bodies,decoded,meta

def library(directory):
    path=directory/'roll.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),
                    *[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));fn=lib.pr_u68_roll_gate_0294_cpu
    fn.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(Diagnostic)]
    fn.restype=ctypes.c_int
    return fn

def install(m,bodies,cs=CS,dice_cs=DICE,field_cs=FIELD,dice_stub=0xB0,field_stub=0x3D5):
    for name,(first,blob) in bodies.items():
        seg=cs if name in ('caller','map') else dice_cs if name=='dice' else 0xAF8 if name in ('random','seed','seed_multiplier') else field_cs
        at=physical(seg,first);m[at:at+len(blob)]=blob
    for site in ROLLS:put16(m,cs,site+3,dice_stub)
    put16(m,cs,0x2D7,field_stub)
    for seg,off,entry,target in ((dice_stub,0x48,0xDE3,dice_cs),(field_stub,0x2F,0x350,field_cs)):
        at=physical(seg,off);m[at:at+5]=struct.pack('<BHH',0xEA,entry,target)

def world(m,ds,rng,*,members=((0x5000,0x100,0x12,95),),field=3,x=3,y=7,d=0,seed=12345,map_at=(0x6000,0x100),cycle=False,map_id=0):
    put16(m,ds,0x4428,seed&65535);put16(m,ds,0x442A,seed>>16)
    put16(m,ds,0x5D96,members[0][1] if members else 0);put16(m,ds,0x5D98,members[0][0] if members else 0)
    for n,(seg,off,kind,value) in enumerate(members):
        nxt=members[n+1][:2] if n+1<len(members) else members[0][:2] if cycle else (0,0)
        m[physical(seg,(off+0x10)&65535)]=kind;m[physical(seg,(off+0x16)&65535)]=value
        put16(m,seg,(off+0x104)&65535,nxt[1]);put16(m,seg,(off+0x106)&65535,nxt[0])
    for off,value in ((0x6AAD,x),(0x6AAE,y),(0x6AAF,d),(0x84DC,map_id)):m[physical(ds,off)]=value
    for i in range(0x110):m[physical(ds,0x2736+i)]=rng.choice((0,1,255))
    put16(m,ds,0x6A5C,map_at[1]);put16(m,ds,0x6A5E,map_at[0])
    # Every nibble stays nonzero. Set the reached fourth-quarter cell only.
    index=(((y&15)<<4)+(x&15))&65535
    m[physical(map_at[0],(map_at[1]+0x300+index)&65535)]=(field&3)*0x55
    m[physical(ds,0x6DDE)]=0xA5

def build(bodies,rng,*,ss=SS,sp=0x8000,ds=DS,flags=0x202,patches=(),background=None,**kw):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    install(m,bodies);world(m,ds,rng,**kw)
    put16(m,ss,sp,0x7000);put16(m,ss,sp+2,0x9200)
    for seg,off,val in patches:m[physical(seg,off)]=val
    c=Cpu(*(rng.randrange(65536) for _ in range(7)),sp,CS,ENTRY,ds,rng.randrange(65536),ss,flags)
    return m,c

def compare(name,state,bodies,decoded,fn,limit=16,dice_limit=256):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    seen,edges=set(),set();s=dict(stop=None,last=None,loops=0,callee=0,status=-1,outer_return=False,pending=None,system_calls=0)
    def stop(code):s['stop']=code;uc.emu_stop()
    def hook(em,address,size,user):
        cs,ip=uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
        if s['outer_return']:return stop(0)
        if cs==CS and s['pending']==ip:
            s['status']=0;s['pending']=None
        if cs==CS and ip==0x2AB and s['last']==('caller',0x47B):
            if s['loops']==limit:return stop(5)
            s['loops']+=1
        if cs==DICE and ip==0xE0E:
            if s['system_calls']==dice_limit:s['status']=11;return stop(3)
            s['system_calls']+=1
        key=next(((n,ip) for n,(first,blob) in bodies.items() if
                  cs==(CS if n in ('caller','map') else DICE if n=='dice' else 0xAF8 if n in ('random','seed','seed_multiplier') else FIELD)
                  and first<=ip<first+len(blob)),None)
        if key is None:
            check((cs,ip) in ((0xB0,0x48),(0x3D5,0x2F)),f'{name}: unexpected fetch {cs:04x}:{ip:04x}')
            s['last']=None;return
        check(key in decoded,f'{name}: noninstruction fetch {key}')
        if s['last']:edges.add((s['last'],key))
        seen.add(key);s['last']=key
        if key[0]=='caller' and ip in (0x2D4,*ROLLS,*MARKS):
            s['callee']=1 if ip==0x2D4 else 3 if ip in MARKS else 2;s['status']=-1
            raw=bytes(uc.mem_read(address,5))
            if ip in MARKS:
                if raw[0]!=0xE8 or (ip+3+struct.unpack_from('<H',raw,1)[0])&65535!=0x109:return stop(2)
                s['pending']=ip+3
            else:
                off=0x2F if ip==0x2D4 else 0x48;entry=0x350 if ip==0x2D4 else 0xDE3
                if raw[0]!=0x9A or struct.unpack_from('<H',raw,1)[0]!=off:return stop(2)
                stub=struct.unpack_from('<H',raw,3)[0];loaded=bytes(uc.mem_read(physical(stub,off),5))
                if loaded[0]!=0xEA or struct.unpack_from('<H',loaded,1)[0]!=entry:return stop(2)
                s['pending']=ip+5
                if ip in ROLLS:s['system_calls']=0
        if key==('caller',0x4DE):s['outer_return']=True
    uc.hook_add(UC_HOOK_CODE,hook);uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=2000000)
    check(s['stop'] is not None,name+': no accepted stop')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);r=Diagnostic()
    got=fn(ram,SIZE,ctypes.byref(c),limit,dice_limit,ctypes.byref(r))
    check((got,r.callee,r.status)==(s['stop'],s['callee'],s['status']),f'{name}: status C={(got,r.callee,r.status)} original={(s["stop"],s["callee"],s["status"])}')
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(reg)&65535;check(getattr(c,n)==want,f'{name}: {n} C={getattr(c,n):04x} original={want:04x}')
    if bytes(ram)!=bytes(backing):
        diff=[i for i in range(SIZE) if ram[i]!=backing[i]]
        raise ValueError(f'{name}: {len(diff)} RAM differences, first {diff[:8]}')
    return got,r.callee,r.status,seen,edges

def specifications():
    rng=random.Random(0x680294)
    cases=[]
    def add(name,*,limit=16,dice_limit=256,**kw):cases.append((name,kw,limit,dice_limit))
    add('empty',members=())
    for field in (1,3):
        for kind in range(256):
            add(f'type-{field}-{kind}',field=field,members=((0x5000,0x100,kind,95),),seed=rng.randrange(2**32))
        for value in range(256):
            add(f'value-{field}-{value}',field=field,members=((0x5000,0x100,0x12,value),),seed=rng.randrange(2**32))
    routes=[(3,0x12,95),(3,0x12,100),(3,0x13,0),(3,0x15,0),(3,0x17,0),(3,0x18,0),
            (1,3,0),(1,8,0),(1,0x10,0),(1,0x12,0),(1,0x12,51),(1,0x12,100)]
    for site,(field,kind,value) in zip(ROLLS,routes):
        opts=dict(field=field,members=((0x5000,0x100,kind,value),))
        for n in range(40):add(f'roll-{site:x}-{n}',**opts,seed=rng.randrange(2**32),flags=FLAGS[n%len(FLAGS)])
        for off,val in ((site,0x90),(site+1,0)):
            add(f'guard-{site:x}-{off:x}',**opts,patches=((CS,off,val),))
        add(f'dice-limit-{site:x}',**opts,dice_limit=0,flags=0x2D7)
    for off,val in ((0x2D4,0x90),(0x2D5,0),(0x2F,0x90),(0x30,0)):
        add(f'field-guard-{off:x}',patches=((CS if off>0x100 else 0x3D5,off,val),))
    for off,val in ((0x48,0x90),(0x49,0)):add(f'dice-stub-{off}',patches=((0xB0,off,val),))
    for site in MARKS:
        for off,val in ((site,0x90),(site+1,0)):
            add(f'mark-guard-{site:x}-{off:x}',field=3,members=((0x5000,0x100,0x19,0),),patches=((CS,off,val),))
    for limit in (0,1,3):
        add(f'list-limit-{limit}',members=((0x5000,0x100,0,0),),cycle=True,limit=limit)
    for d in range(8):
        for x,y in ((0,0),(15,15),(16,255),(128,254)):
            for map_id in (0,11):
                add(f'coordinate-{d}-{x}-{y}-{map_id}',field=3,d=d,x=x,y=y,map_id=map_id,
                    members=((0x5000,0x100,0x19,0),))
    for n in range(40):
        members=tuple((0x5000+i*0x200,0x100,rng.randrange(3,27),rng.randrange(256)) for i in range(rng.randrange(1,5)))
        add(f'list-{n}',members=members,field=rng.choice((1,3)),seed=rng.randrange(2**32),d=rng.randrange(8),flags=rng.choice(FLAGS))
    for ss,sp in ((0xFFFF,0x40),(0xFFFF,0xFFFC),(SS,0x40),(SS,0xFFFC)):
        add(f'stack-{ss}-{sp}',ss=ss,sp=sp,field=3,members=((0x5000,0x100,0x19,0),))
    return cases

def differential(bodies,decoded,fn):
    rng=random.Random(0x294);seen=set();edges=set();counts={}
    cases=specifications()
    for name,options,limit,dice_limit in cases:
        code,callee,status,visited,transitions=compare(name,build(bodies,rng,**options),bodies,decoded,fn,limit,dice_limit)
        key=f'{code}/{callee}/{status}';counts[key]=counts.get(key,0)+1;seen|=visited;edges|=transitions
    all_edges={(key,(key[0],dest)) for key,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'
               for dest in (i.address+i.size,int(i.op_str,16))}
    caller_edges={e for e in all_edges if e[0][0]=='caller'}
    report=dict(cases=len(cases),statuses=counts,cpu_words=14,ram_bytes=SIZE,masks=[],
        caller_starts=sum(n=='caller' for n,ip in seen),closure_starts=len(seen),closure_total=len(decoded),
        caller_edges=len(caller_edges&edges),caller_edges_total=len(caller_edges),
        closure_edges=len(all_edges&edges),closure_edges_total=len(all_edges),
        missing_starts=[f'{n}:{ip:04X}' for n,ip in sorted(set(decoded)-seen)],
        missing_edges=[f'{a[0]}:{a[1]:04X}->{b[1]:04X}' for a,b in sorted(all_edges-edges)])
    print(json.dumps(report,sort_keys=True),flush=True)
    return report

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--unicorn',action='store_true');parser.add_argument('--native',action='store_true')
    parser.add_argument('--output',type=Path);args=parser.parse_args()
    bodies,decoded,meta=source();report=dict(spans=meta,closure_bytes=sum(m['bytes'] for m in meta.values() if m.get('role')!='data'),closure_data_bytes=2,closure_instructions=len(decoded))
    with tempfile.TemporaryDirectory(prefix='poolrad-u68-roll-') as tmp:
        fn=library(Path(tmp))
        if args.unicorn:report['unicorn']=differential(bodies,decoded,fn)
        if args.native:
            from u68_roll_gate_0294_cpu_native import native_audit
            report['native']=native_audit(bodies,fn)
    if args.output:args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
