#!/usr/bin/env python3
"""U9E +115C and all eleven 00BA +0BB8 dependencies, original CPU14/RAM oracle.

No output masks, CPU substitutions, handler callbacks or bytecode interpreters.
The original caller's one far segment word receives its loader relocation.
Synthetic input records do not assert a naturally reached game route.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SP, UC_X86_REG_SS
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('OVL_009E_115C', 'RES_00BA_0BB8', 'RES_00BA_1168', 'RES_00BA_13AB', 'RES_00BA_0000', 'RES_00BA_01FB', 'RES_00BA_0282', 'RES_00BA_03A0', 'RES_00BA_10E6', 'RES_00BA_11EA', 'RES_00BA_1282', 'RES_00BA_1325')
MASK = SIZE - 1
PINS = (
 ('u9e',0x10e4,0x110d,17,'12ce30806a561605b7f7be238444a28a0d22f7cc53bda4872d6e0800330f41db'),
 ('u9e',0x110d,0x115c,29,'63aaf31a1bb13ac3a652db1b1fef79afa6ddf2aee869fe426bc4bda7e5395830'),
 ('u9e',0x115c,0x1287,109,'e122e18cccee13655b750e38c4d5e6eb664fca16b125cc72bcd724580af6fb8b'),
 ('res',0x0000,0x01fb,179,'dc1d899f356871351977cc5ffdba6f1445bdc9e51a0f42f0758deaae63f28917'),
 ('res',0x01fb,0x0282,44,'231ccedb42ed924477cf8c0db9d2415eb541fc21e58a52af19052ba2dad9a21f'),
 ('res',0x0282,0x03a0,104,'92bfa47643e8418793ce20a0ad79475c84ef340046e61327682efb132f01cf3a'),
 ('res',0x03a0,0x042d,52,'56c28fb6c276cd2242687ca09e1cb39adf8b0f22d19e9a3dddd1feecb9ac1f98'),
 ('res',0x0bb8,0x1037,375,'09ba382d51f6ee4dbf498705b08ab910bef47345fca5488aa069753324e3a7e0'),
 ('res',0x10e6,0x1168,54,'fac332383b03ed081b3c863de7a7f086277f2a3db34505e6a0ecb6959c9f101c'),
 ('res',0x1168,0x11ea,54,'4c5c49b63f5a382b3de7df784840f406cd807921aeeffbb88948483067ac49a4'),
 ('res',0x11ea,0x1282,62,'8331051d67bce5f352938484858963a8e7e6758d43e47bd131f72b9c2980e020'),
 ('res',0x1282,0x1325,67,'3c9c2c535ca66703d223b5e65c9fa6b13e04d8cc05603f8a88c6024376059bf4'),
 ('res',0x1325,0x13ab,54,'bf71db2bfc2033faa964f74e185be79e0cd2789ff3a6c0c5d5ff828b19b53a36'),
 ('res',0x13ab,0x148b,89,'22732f3cfe79dc6feb87722de05c3da242b1d3c7f984db735f8c3d38ef3b7bbe'))
SOURCES = ('overlay_u9e_character_pair_115c_cpu','resident_00ba_character_0bb8_cpu',
           'resident_00ba_character_helpers_cpu','overlay_u9e_entry16_10e4','overlay_u9e_entry17_110d')

# These taken edges are impossible for every input under immutable code: the
# immediately preceding comparisons establish the contradicting unsigned/signed
# range without changing the compared register. They remain in the report.
INFEASIBLE = {
 (0x021e,0x0236): 'CMP AX,0 cannot set unsigned-below.',
 (0x0239,0x024b): 'Earlier 0223 leaves AX>=151; CMP AX,151 cannot be below.',
 (0x03d7,0x03eb): 'Negative local was clamped to zero at03CC; AX is nonnegative.',
 (0x03ee,0x03fb): 'Earlier range rejected AX<=512; AX cannot now be below513.',
 (0x03fe,0x040b): 'Earlier adjacent ranges rejected AX<=768; AX cannot be below769.',
 (0x0c8e,0x0cb3): 'CMP AL,0 cannot set unsigned-below.',
 (0x117d,0x1189): 'CMP AL,0 cannot set unsigned-below.',
 (0x118b,0x119e): 'Earlier range rejected AL<=2; AL cannot be below3.',
 (0x11f9,0x120b): 'CMP AL,0 cannot set unsigned-below.',
 (0x1222,0x122e): 'Earlier equality rejected AL=0; unsigned AL cannot be below1.',
 (0x1230,0x123c): 'Earlier range rejected AL<=50; AL cannot be below51.',
 (0x123e,0x124a): 'Earlier range rejected AL<=75; AL cannot be below76.',
 (0x124c,0x1258): 'Earlier range rejected AL<=90; AL cannot be below91.',
 (0x1264,0x1279): 'Earlier ranges rejected AL<=17 and AL=18; AL cannot be below19.',
}


def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert len(exe)==65231 and sha256(exe).hexdigest()==EXE_SHA
    assert len(ovr)==206105 and sha256(ovr).hexdigest()==OVR_SHA
    unit=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x9e)
    assert [p for p in unit['fixups'] if 0x10e4<=p<0x1287]==[0x1266]
    resident=bytes(unpack(exe)[0]);bodies={};decoded={}
    for kind,start,end,count,digest in PINS:
        at=unit['ovr_offset']+start if kind=='u9e' else physical(0xba,start)
        blob=(ovr if kind=='u9e' else resident)[at:at+end-start]
        ins=list(Cs(CS_ARCH_X86,CS_MODE_16).disasm(blob,start))
        assert sha256(blob).hexdigest()==digest and len(ins)==count
        assert sum(i.size for i in ins)==len(blob)
        bodies[kind,start]=blob;decoded.update(((kind,i.address),i) for i in ins)
    assert [(k,i.op_str) for k,i in decoded.items() if i.mnemonic=='lcall']==[(('u9e',0x1263),'0xba, 0xbb8')]
    return bodies,decoded


def build_model(directory):
    path=Path(directory)/'character.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared',
                    '-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));args=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u9e_character_pair_115c.argtypes=args+[ctypes.c_uint16,ctypes.POINTER(ctypes.c_size_t)]
    lib.pr_u9e_character_pair_115c.restype=ctypes.c_int
    for name in ('pr_res00ba_0bb8_cpu','pr_res00ba_core_cpu','pr_res00ba_helpers_cpu'):
        getattr(lib,name).argtypes=args+[ctypes.POINTER(ctypes.c_size_t)];getattr(lib,name).restype=ctypes.c_int
    return lib


def install(memory,cpu,resident_cs,bodies):
    for (kind,start),blob in bodies.items():
        segment=cpu.cs if kind=='u9e' else resident_cs
        at=physical(segment,start);memory[at:at+len(blob)]=blob
    put16(memory,cpu.cs,0x1266,resident_cs)


def fixture(*,major=18,minor=50,old_major=17,old_minor=0,record_fields=None,
            items=(),nodes=(),tables=None,entry=0x115c,kind='u9e',flags=2,
            cs=0x6000,resident_cs=0x7000,ds=0x1000,ss=0x2000,sp=0x8000,
            record_seg=0x3000,record_off=0x100,node_seg=0x4000,out_seg=0x5000,
            cycle=False,background=0xa5):
    memory=bytearray([background])*SIZE
    cpu=Cpu(0xabcd,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x7777,sp,cs,entry,ds,0x1111,ss,flags)
    def byte(seg,off,value):memory[physical(seg,off&65535)]=value&255
    def word(seg,off,value):put16(memory,seg,off&65535,value&65535)
    def rb(off,value):byte(record_seg,record_off+off,value)
    def rw(off,value):word(record_seg,record_off+off,value)
    for i in range(0x140):rb(i,0)
    for i in range(0x1020):byte(ds,0x5580+i,0)
    for off,value in {0x10:old_major,0x16:old_minor,0x13:12,0x2d:10,0x2e:2,0x72:6}.items():rb(off,value)
    for off,value in (record_fields or {}).items():rb(off,value)
    for index,node in enumerate(nodes):
        at=0x100+index*0x100;nxt=at+0x100 if index+1<len(nodes) else (0x100 if cycle else 0)
        for off in range(0x40):byte(node_seg,at+off,0)
        byte(node_seg,at,node.get('kind',12));byte(node_seg,at+3,node.get('attribute',6))
        word(node_seg,at+5,nxt);word(node_seg,at+7,node_seg if nxt else 0)
    rw(0x7f,0x100 if nodes else 0);rw(0x81,node_seg if nodes else 0)
    for index,item in enumerate(items):
        at=0x2000+index*0x100;nxt=at+0x100 if index+1<len(items) else (0x2000 if cycle else 0)
        for off in range(0x50):byte(node_seg,at+off,0)
        word(node_seg,at+0x2a,nxt);word(node_seg,at+0x2c,node_seg if nxt else 0)
        for off,key,default in ((0x2e,'id',index),(0x2f,'subtype',0),(0x32,'bonus',0),(0x33,'bonus2',0),(0x34,'equipped',1),(0x39,'quantity',0)):
            byte(node_seg,at+off,item.get(key,default))
        word(node_seg,at+0x37,item.get('weight',1))
    rw(0xc8,0x2000 if items else 0);rw(0xca,node_seg if items else 0)
    if kind=='res' and entry==0 and items:rw(0xcc,0x2000);rw(0xce,node_seg)
    for itemid,values in (tables or {}).items():
        for off,value in values.items():byte(ds,0x5580+itemid*16+off,value)
    args=[0x100,out_seg,minor,major,record_off,record_seg] if kind=='u9e' else [record_off,record_seg]
    if kind=='res' and entry==0x1fb:args=[0x2000,node_seg,record_off,record_seg]
    if kind=='res' and entry==0x282:args=[0x200,out_seg,0x210,out_seg,0x2000,node_seg,record_off,record_seg]
    for index,value in enumerate([0x9000,0x8000]+args):word(ss,sp+2*index,value)
    return memory,cpu,resident_cs,kind


def run_c(lib,state,budget):
    memory,cpu,resident_cs,kind=state
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);out=Cpu.from_buffer_copy(cpu);left=ctypes.c_size_t(budget)
    if kind=='u9e':status=lib.pr_u9e_character_pair_115c(ram,SIZE,ctypes.byref(out),resident_cs,ctypes.byref(left))
    else:
        out.cs=resident_cs
        f=lib.pr_res00ba_helpers_cpu if cpu.ip>=0x10e6 else lib.pr_res00ba_core_cpu
        status=f(ram,SIZE,ctypes.byref(out),ctypes.byref(left))
    return status,bytes(ram),out,left.value


def original(state,decoded,budget):
    memory,cpu,resident_cs,kind=state;backing=(ctypes.c_uint8*SIZE).from_buffer_copy(memory)
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(cpu,name))
    if kind=='res':uc.reg_write(UC_X86_REG_CS,resident_cs)
    seen=set();edges=set();previous=[None];left=[budget];calls=[];pending=[None];status=[None]
    def word(segment,off):
        at=physical(segment,off&65535);return backing[at]|(backing[(at+1)&MASK]<<8)
    def hook(uc,address,size,user):
        cs=uc.reg_read(UC_X86_REG_CS);ip=uc.reg_read(UC_X86_REG_IP)
        if pending[0] is not None:
            target,expected=pending[0];pending[0]=None
            assert (cs,ip)==target,('original return mismatch',target,(cs,ip))
            if expected is None or target!=expected:
                status[0]=0 if expected is None else 3;uc.emu_stop();return
        key=('res' if cs==resident_cs else 'u9e' if cs==cpu.cs else None,ip)
        if key not in decoded:raise AssertionError(('unexpected original PC',key,cs))
        ins=decoded[key];counted=not(key[0]=='u9e' and 0x10e4<=ip<0x115c)
        if counted:
            if not left[0]:status[0]=2;uc.emu_stop();return
            left[0]-=1
        seen.add(key)
        if previous[0] is not None:edges.add((previous[0],key))
        previous[0]=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if ins.mnemonic in ('call','lcall'):calls.append((cs,(ip+ins.size)&65535))
        if ins.mnemonic=='retf':
            sp=uc.reg_read(UC_X86_REG_SP);ss=uc.reg_read(UC_X86_REG_SS)
            target=(word(ss,sp+2),word(ss,sp));pending[0]=(target,calls.pop() if calls else None)
    uc.hook_add(UC_HOOK_CODE,hook)
    uc.emu_start(physical(uc.reg_read(UC_X86_REG_CS),cpu.ip),2*SIZE,count=max(100000,4*budget))
    assert status[0] is not None,'original execution did not reach return/limit'
    out=Cpu(*(uc.reg_read(r)&65535 for r in REGISTERS))
    return status[0],bytes(backing),out,left[0],seen,edges


def compare(label,state,bodies,decoded,lib,budget=100000):
    install(state[0],state[1],state[2],bodies)
    actual=run_c(lib,state,budget);expected=original(state,decoded,budget)
    assert actual[0]==expected[0],(label,'status',actual[0],expected[0])
    assert actual[3]==expected[3],(label,'budget',actual[3],expected[3])
    for (name,_) in Cpu._fields_:
        assert getattr(actual[2],name)==getattr(expected[2],name),(label,name,hex(getattr(actual[2],name)),hex(getattr(expected[2],name)))
    if actual[1]!=expected[1]:
        diffs=[i for i,(a,b) in enumerate(zip(actual[1],expected[1])) if a!=b]
        raise AssertionError((label,'RAM',[(hex(i),actual[1][i],expected[1][i]) for i in diffs[:16]]))
    for (kind,start),blob in bodies.items():
        segment=state[1].cs if kind=='u9e' else state[2];at=physical(segment,start)
        assert expected[1][at:at+len(blob)]==state[0][at:at+len(blob)],(label,'code write outside profile')
    return expected[4:]


def cases(quick=False):
    for major,minor,oldmajor,oldminor in ((1,0,18,50),(18,1,18,50),(18,50,18,50),(19,0,18,50),(17,0,17,0),(255,255,0,0)):
        for nodes in ((),({'kind':0},{'kind':38,'attribute':6}),({'kind':12,'attribute':128},{'kind':12,'attribute':100},{'kind':38}),({'kind':12,'attribute':127},)):
            yield f'caller-{major}-{minor}-{oldmajor}-{oldminor}-{len(nodes)}',dict(major=major,minor=minor,old_major=oldmajor,old_minor=oldminor,nodes=nodes),100000
    values=(0,1,3,4,6,7,14,15,17,18,19,20,21,23,24,25,26,127,128,255)
    for val in values:
        yield f'abilities-{val}',dict(major=val,minor=val,record_fields={0x13:val,0x14:val,0x15:val,0x17:val,0x98:val,0x2e:val}),100000
    for slot in range(11):
        for magic in (0,128,129,255):
            items=tuple(dict(id=i,bonus=(i*19)%256,bonus2=3,weight=160,quantity=2) for i in (0,1,2,0x1c,0x49))
            tables={i:{2:slot,3:5,8:magic,0x10:0x87} for i in (0,1,2,0x1c,0x49)}
            yield f'inventory-{slot}-{magic}',dict(items=items,tables=tables),100000
    rng=random.Random(0x9e115c)
    for index in range(40 if quick else 400):
        items=tuple(dict(id=rng.choice((0,1,2,0x24,0x25,0x28,0x29,0x2c,0x2d,0x1c,0x49)),subtype=rng.choice((0,0xba)),bonus=rng.randrange(256),bonus2=rng.randrange(256),weight=rng.choice((0,1,150,151,399,400,5000,65535)),quantity=rng.randrange(4),equipped=rng.randrange(2)) for _ in range(rng.randrange(7)))
        tables={i:{2:rng.randrange(11),3:rng.randrange(256),8:rng.choice((0,127,128,129,255)),0x10:rng.randrange(256)} for i in {n['id'] for n in items}}
        yield f'random-{index}',dict(major=rng.randrange(256),minor=rng.randrange(256),old_major=rng.randrange(32),old_minor=rng.randrange(128),record_fields={off:rng.randrange(256) for off in (0x13,0x14,0x15,0x17,0x2d,0x2e,0x72,0xa9,0x98)},items=items,tables=tables,flags=rng.choice((2,0x47,0x8d7,0x402))),100000
    for entry in (0x10e6,0x1168,0x11ea,0x1282,0x1325,0x13ab):
        for val in range(256):
            yield f'helper-{entry:x}-{val}',dict(kind='res',entry=entry,old_major=val,old_minor=val,record_fields={0x13:val,0x14:val,0x15:val,0x17:val}),100000

    for entry in (0x11ea,0x1282,0x1325,0x13ab):
        for minor in range(256):
            yield f'exceptional-{entry:x}-{minor}',dict(kind='res',entry=entry,old_major=18,old_minor=minor),100000
    for itemid in (0x24,0x25,0x28,0x29,0x2c,0x2d):
        yield f'weapon-kind-{itemid}',dict(items=({'id':itemid},),tables={itemid:{2:0,0x10:0x87}}),100000
    for weight in (0,150,151,399,400,5000):
        yield f'armor-weight-{weight}',dict(items=({'id':1,'weight':weight},),tables={1:{2:2}}),100000

    for major in range(27):
        for minor in (0,1,50,51,75,76,90,91,99,100,101,255):
            yield f'full-gated-abilities-{major}-{minor}',dict(major=major,minor=minor,old_major=0,record_fields={0xaa:1,0x13:major}),100000

    for budget in range(16):
        yield f'caller-prefix-limit-{budget}',dict(),budget
    for budget in (0,1,2,3,25,100,1000):
        yield f'cyclic-list-{budget}',dict(nodes=({'kind':0},),cycle=True),budget
    for record_off in (0,0xff00,0xff80,0xfffe,0xffff):
        yield f'record-offset-{record_off:x}',dict(record_off=record_off),100000
    yield 'output-alias-record',dict(out_seg=0x3000),100000
    yield 'record-a20-wrap',dict(record_seg=0xffff,record_off=0x200),100000

    for dexterity in range(27):
        yield f'weapon-dexterity-{dexterity}',dict(record_fields={0x13:dexterity,0xaa:1},items=({'id':0},),tables={0:{2:0,0x10:0x87}}),100000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-only',action='store_true');parser.add_argument('--quick',action='store_true')
    parser.add_argument('--output',type=Path);parser.add_argument('--allow-incomplete',action='store_true')
    args=parser.parse_args();seen=set();edges=set();count=0;full_seen=set();full_edges=set();full_count=0
    with tempfile.TemporaryDirectory() as directory:
        lib=build_model(directory)
        if args.compile_only:print('CHECKER_BUILD_OK');return
        bodies,decoded=source()
        for label,options,budget in cases(args.quick):
            s,e=compare(label,fixture(**options),bodies,decoded,lib,budget);seen|=s;edges|=e;count+=1
            if options.get('kind','u9e')=='u9e':full_seen|=s;full_edges|=e;full_count+=1
    required={((kind,ip),(kind,target)) for (kind,ip),ins in decoded.items() if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' for target in (int(ins.op_str,16),ip+ins.size)}
    infeasible={(('res',a),('res',b)) for a,b in INFEASIBLE}
    assert infeasible<=required and not infeasible&edges,'infeasible edge proof contradicted'
    report=dict(target='OVL_009E_115C',cases=count,cpu_words=14,whole_ram_bytes=SIZE,masked_flags=0,oracle_cpu_substitutions=0,
                body_replacements=0,synthetic_inputs=True,natural_game_route_measured=False,covered_starts=len(seen),source_starts=len(decoded),
                covered_outcomes=len(edges),source_outcomes=len(required),feasible_outcomes=len(required-infeasible),missing_starts=sorted(set(decoded)-seen),missing_outcomes=sorted(required-edges-infeasible),
                infeasible_outcomes=[dict(site=f'{a:04X}',target=f'{b:04X}',reason=r) for (a,b),r in INFEASIBLE.items()],
                full_caller_cases=full_count,full_caller_starts=len(full_seen),full_caller_outcomes=len(full_edges),
                full_caller_missing_starts=sorted(set(decoded)-full_seen),full_caller_missing_outcomes=sorted(required-full_edges-infeasible),
                bodies=[dict(kind=k,start=f'{s:04X}',end=f'{e:04X}',instructions=n,sha256=h,covered_starts=sum((k,i) in seen for i in range(s,e)),full_caller_starts=sum((k,i) in full_seen for i in range(s,e)),covered_outcomes=sum(a[0]==k and s<=a[1]<e for a,b in edges)) for k,s,e,n,h in PINS])
    if args.output:args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report,sort_keys=True))
    if not args.allow_incomplete:
        assert not report['missing_starts'] and not report['missing_outcomes'],'coverage incomplete'
        assert not report['full_caller_missing_starts'] and not report['full_caller_missing_outcomes'],'full caller composition coverage incomplete'
if __name__=='__main__':main()
