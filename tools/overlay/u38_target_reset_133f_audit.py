#!/usr/bin/env python3
"""U38 +133F and raw RES00BA:2637 full-state original/native oracle."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
CS,RES,DS,SS,ENTRY=0x8800,0x7000,0x1800,0x2800,0x133F
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
SOURCES=['overlay_u38_target_reset_133f_cpu.c','resident_00ba_target_reset_2637_cpu.c']
def check(v,msg):
    if not v:raise ValueError(msg)
def source():
    ex=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ov=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ex).hexdigest()==EXE_SHA and sha256(ov).hexdigest()==OVR_SHA,'media hashes')
    u=next(u for u in find_units(ex[parse_mz(ex)['header_bytes']:],ov) if u['stub_para']==0x38);code=ov[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    caller=code[ENTRY:0x1355];image=bytes(unpack(ex)[0]);resident=image[physical(0xBA,0x2637):physical(0xBA,0x267D)]
    check(sha256(caller).hexdigest()=='f5d32541e8f8b2ce0e4d08ed887a5ddf9c52829f449f74b8c1553a6b0bbfe8ce','caller hash')
    check(sha256(resident).hexdigest()=='045d3c16b446425bd6ec344cfb54b83028fc2ed83c46f02ae561401fad734721','resident hash')
    md=Cs(CS_ARCH_X86,CS_MODE_16);ci=list(md.disasm(caller,ENTRY));ri=list(md.disasm(resident,0x2637))
    check(len(ci)==9 and len(ri)==20 and u['entries'][43]==ENTRY and 0x134B in u['fixups'],'extent/entry/fixup')
    check(ci[4].op_str=='0xba, 0x2637' and ci[6].op_str=='0x1351' and ci[-1].op_str=='0xa','caller ABI')
    check(ri[-1].op_str=='4' and not any(i.mnemonic in ('call','lcall') for i in ri),'resident ABI')
    installer=code[0x31A2:0x38A9]
    check(sha256(installer).hexdigest()=='4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519','installer hash')
    instructions=list(md.disasm(installer,0x31A2));slots=[]
    for n in range(138):
        a,b,c,d=instructions[2+4*n:6+4*n]
        if a.op_str=='ax, 0xf7' and b.op_str=='dx, 0x41':slots.append((int(c.op_str.split('[')[1].split(']')[0],0)-0x6828)//4)
    check(slots==[31,51,52,53],'caller dispatch slots')

    return caller,resident,dict(caller_sha256=sha256(caller).hexdigest(),resident_sha256=sha256(resident).hexdigest(),caller_bytes=22,caller_instructions=9,resident_bytes=70,resident_instructions=20,caller_file_start=u['ovr_offset']+ENTRY,resident_image_start=physical(0xBA,0x2637))
CHECKER_ROWS = ('OVL_0038_133F',)


def library(directory):
    p=directory/'reset.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    lib=ctypes.CDLL(str(p));functions=[]
    for name in ('pr_u38_target_reset_133f_cpu','pr_res_00ba_target_reset_2637_cpu'):
        f=getattr(lib,name);f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)];f.restype=ctypes.c_int;functions.append(f)
    return functions
def build(caller,resident,rng,*,direct=False,record=(0x5000,0x100),target=(0x6000,0x200),sp=0x8000,ss=SS,flags=0xED7,writes=(),background=None,res_segment=RES):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    p=physical(CS,ENTRY);m[p:p+len(caller)]=caller;put16(m,CS,0x134B,res_segment)
    p=physical(res_segment,0x2637);m[p:p+len(resident)]=resident
    values=(0x7000,0x9001,record[1],record[0]) if direct else (0x7000,0x9001,0x1111,0x2222,0x3333,record[1],record[0])
    for j,v in enumerate(values):put16(m,ss,sp+2*j,v)
    p=physical(record[0],(record[1]+0x108)&65535)
    for j,v in enumerate(struct.pack('<HH',target[1],target[0])):m[(p+j)&(SIZE-1)]=v
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    c=Cpu(*[rng.randrange(65536) for _ in range(7)],sp,res_segment if direct else CS,0x2637 if direct else ENTRY,DS,rng.randrange(65536),ss,flags)
    return m,c
def compare(label,memory,cpu,caller,resident,function,direct=False):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    res=cpu.cs if direct else struct.unpack_from('<H',memory,physical(CS,0x134B))[0]
    md=Cs(CS_ARCH_X86,CS_MODE_16);decoded={(CS,i.address):i for i in md.disasm(caller,ENTRY)};decoded.update({(res,i.address):i for i in md.disasm(resident,0x2637)})
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (name,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(cpu,name))
    state=dict(stop=None,returning=False,done=False);seen=set();branch_flags=[]
    def halt(status):state['stop']=status;uc.emu_stop()
    def hook(em,address,size,user):
        del size,user
        here=(em.reg_read(UC_X86_REG_CS),em.reg_read(UC_X86_REG_IP))
        if state['done']:return halt(0)
        if state['returning']:
            state['returning']=False
            if here!=(CS,0x134D):return halt(4)
        check(here in decoded,f'escape {label} {here}')
        if here==(CS,0x1348) and bytes(em.mem_read(address,3))!=bytes.fromhex('9a3726'):return halt(2)
        seen.add((0 if here[0]==CS else 1,here[1]))
        if here==(res,0x267A):
            if direct:state['done']=True
            else:state['returning']=True
        if here==(CS,0x1352):state['done']=True
        if here==(CS,0x134F):branch_flags.append(em.reg_read(REGISTERS[-1])&0x40)
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);em.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_MEM_WRITE,mirror);uc.emu_start(physical(cpu.cs,cpu.ip),2*SIZE,count=100)
    check(state['stop'] is not None,f'timeout {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);status=function(ram,SIZE,ctypes.byref(cpu));check(status==state['stop'],f'status {label}: {status}/{state}')
    actual,want=bytes(ram),bytes(uc.mem_read(0,SIZE))
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'RAM {label}: {diff[:8]}')
    for (name,_),r in zip(Cpu._fields_,REGISTERS):check(getattr(cpu,name)==(uc.reg_read(r)&65535),f'{label} {name}: C={getattr(cpu,name):04x} original={uc.reg_read(r)&65535:04x}')
    check(not any(branch_flags),'JE taken despite resident AL1')
    return status,seen
def cases(direct=False):
    rng=random.Random(0x382637);spec=[]
    for n in range(192 if not direct else 96):
        spec.append((f'random-{n}',dict(record=(0x5000,0x100+n*3),target=(0x6000,0x200+n*7),flags=FLAGS[n%len(FLAGS)])))
    spec += [('null',dict(record=(0,0),target=(0,0))),('record-wrap',dict(record=(0x5000,0xFEF7))),('target-wrap',dict(target=(0x6000,0xFFFD))),
             ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF,target=(0xFFFF,0x20))),('pointer-low-redirect',dict(target=(0x5000,0x205))),
             ('pointer-high-redirect',dict(record=(0x5011,0x100),target=(0x5011,0x207))),('random-ram',dict(background=rng.randbytes(SIZE))),
             ('resident-relocation',dict(res_segment=0x7100)),('saved-bp-alias',dict(target=(SS,0x7FFB))),('return-alias',dict(target=(SS,0x7FFD)))]
    for sp in ((4,5,0x8001,0xFFF8) if direct else (0x10,0x11,0x8001,0xFFF2)):spec.append((f'sp-{sp:x}',dict(sp=sp)))
    if not direct:spec += [('changed-inner-return',dict(target=(SS,0x7FF3))),('code',dict(writes=((CS,0x1348,b'\x90'),)))]
    return spec
def differential(caller,resident,functions):
    report={}
    for direct in (False,True):
        seen=set();stops={};rng=random.Random(0x133F)
        for label,options in cases(direct):
            m,c=build(caller,resident,rng,direct=direct,**options);status,v=compare(label,m,c,caller,resident,functions[int(direct)],direct)
            seen|=v;stops[str(status)]=stops.get(str(status),0)+1
        check(len(seen)==(20 if direct else 29),'instruction coverage')
        report['resident' if direct else 'caller']=dict(cases=sum(stops.values()),stops=stops,starts=len(seen),cpu_words=14,ram_bytes=SIZE,flag_masks=0)
    return report
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    caller,resident,meta=source();report=dict(target=meta)
    with tempfile.TemporaryDirectory(prefix='u38-133f-') as tmp:
        fs=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(caller,resident,fs)
        if a.native:
            from u38_target_reset_133f_native import native_audit
            report['native']=native_audit(caller,resident,fs[0])
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
