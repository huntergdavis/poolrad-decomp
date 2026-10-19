#!/usr/bin/env python3
"""U38 +2B20 / U9E +0E2E raw closure, CPU14 and full 1 MiB unmasked."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
CHECKER_ROWS = ("OVL_0038_2B20",)
CS,DICE,STUB,SYSTEM,DS,SS,HEAP=0x5000,0x6000,0x7000,0xAF8,0x1800,0x2800,0x3000
ENTRIES={'caller':0x2B20,'latch':0x0E2E}
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SOURCES=('overlay_u38_link_roll_2b20.c','overlay_u9e_entry9_roll_latch_0e2e.c',
 'overlay_u9e_entry8_fullstate_compose_0de3.c','overlay_u9e_entry8_fullstate_prefix_0de3.c',
 'overlay_u9e_entry8_fullstate_loop_0e05.c','overlay_u9e_entry8_fullstate_postcall_0e13.c',
 'overlay_u9e_entry8_fullstate_tail_0e1f.c','overlay_u9e_entry8_system_call_0e0e.c',
 'system_random_fullstate_11c0.c','system_seed_fullstate_120f.c')
def check(ok,msg):
    if not ok:raise ValueError(msg)
class Diagnostic(ctypes.Structure):
    _fields_=[('latch_status',ctypes.c_int),('dice_status',ctypes.c_int)]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media identity')
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)}
    check(units[0x38]['entries'][101]==0x2B20 and 0x2B55 in units[0x38]['fixups'] and units[0x9E]['entries'][9]==0xE2E,'slot/fixup identity')
    caller=ovr[units[0x38]['ovr_offset']:units[0x38]['ovr_offset']+0x2B63]
    dice=ovr[units[0x9E]['ovr_offset']:units[0x9E]['ovr_offset']+0xE52]
    system=bytes(unpack(exe)[0])[physical(SYSTEM,0):physical(SYSTEM,0x1247)]
    specs=((CS,0x2B20,0x2B63,caller,25,'9a322acac07936548b9d1257854e48fc256e29bf2dec0f4c58dfab9ca639366b'),
           (DICE,0xE2E,0xE52,dice,16,'08dc2c3413d0fcfa12d26b589873f619761741e3f38c235477e89ed3a7350443'),
           (DICE,0xDE3,0xE2E,dice,29,'7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba'),
           (SYSTEM,0x11C0,0x11D6,system,10,None),(SYSTEM,0x120F,0x1245,system,22,None))
    decoded={};digests={};md=Cs(CS_ARCH_X86,CS_MODE_16)
    for seg,a,b,code,n,want in specs:
        blob=code[a:b];ins=list(md.disasm(blob,a));digest=sha256(blob).hexdigest()
        check((want is None or want==digest) and len(ins)==n and sum(i.size for i in ins)==b-a,'span identity')
        decoded.update({(seg,i.address):i for i in ins});digests[f'{seg:04X}:{a:04X}-{b:04X}']=dict(bytes=b-a,instructions=n,sha256=digest)
    check(system[0x1245:0x1247]==b'\x05\x84','SYSTEM multiplier')
    calls=[(k,i.mnemonic,i.op_str) for k,i in decoded.items() if 'call' in i.mnemonic]
    check(calls==[((CS,0x2B52),'lcall','0xb0, 0x4d'),((DICE,0xE43),'call','0xde3'),((DICE,0xE0E),'lcall','0xaf8, 0x11c0'),((SYSTEM,0x11C0),'call','0x120f')],'calls')
    conditional={k:((k[0],i.address+i.size),(k[0],int(i.op_str,16))) for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    return caller,dice,system,decoded,conditional,digests
def library(directory):
    path=directory/'chain.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/p) for p in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));out={}
    for mode,name in [('caller','pr_u38_link_roll_2b20_cpu'),('latch','pr_u9e_roll_latch_0e2e_cpu')]:
        f=getattr(lib,name);f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int;out[mode]=f
    return out
def build(caller,dice,system,rng,*,mode='caller',kind=0x55,amount=0xA7,latch=0x97,count=1,sides=6,seed=0,
          character=(HEAP,0x1000),link=(HEAP,0x2000),sp=0x8000,ss=SS,ds=DS,flags=2,
          caller_cs=CS,dice_cs=DICE,stub=STUB,writes=(),background=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    for seg,code in ((caller_cs,caller),(dice_cs,dice),(SYSTEM,system)):
        for j,v in enumerate(code):m[(physical(seg,0)+j)&(SIZE-1)]=v
    put16(m,caller_cs,0x2B55,stub)
    m[physical(stub,0x4D):physical(stub,0x4D)+5]=struct.pack('<BHH',0xEA,0xE2E,dice_cs)
    put16(m,ds,0x5D92,character[1]);put16(m,ds,0x5D94,character[0])
    put16(m,character[0],(character[1]+0xCC)&65535,link[1]);put16(m,character[0],(character[1]+0xCE)&65535,link[0])
    m[physical(link[0],(link[1]+0x2E)&65535)]=kind
    m[physical(ds,0x6818)]=amount;m[physical(ds,0x681C)]=latch
    put16(m,ds,0x4428,seed&65535);put16(m,ds,0x442A,seed>>16)
    args=(0xA100,0xA101,0xA102,0xA103,0xA104) if mode=='caller' else (0xA500|sides,0x5A00|count)
    for j,v in enumerate((0x7000,0x9001,*args)):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,data in writes:
        for j,v in enumerate(data):m[(physical(seg,off)+j)&(SIZE-1)]=v
    return m,Cpu(*[rng.randrange(65536) for _ in range(7)],sp,caller_cs if mode=='caller' else dice_cs,ENTRIES[mode],ds,rng.randrange(65536),ss,flags)
def cases():
    out=[]
    for kind in range(256):out.append((f'kind-{kind:02X}',dict(kind=kind,amount=kind,flags=FLAGS[kind%6])))
    for n in range(256):out.append((f'seed-{n}',dict(seed=(n*0x01000193)&0xFFFFFFFF,flags=FLAGS[n%6])))
    for count in (0,1,2,3,16,255):
        for sides in (0,1,6,127,255):
            out.append((f'latch-{count}-{sides}',dict(mode='latch',count=count,sides=sides,seed=(count*0x1020304+sides)&0xFFFFFFFF,flags=FLAGS[count%6])))
    out.extend([('null',dict(link=(0,0))),('offset-only',dict(link=(0,0x2000))),('segment-only',dict(link=(HEAP,0))),('char-wrap',dict(character=(HEAP,0xFF34))),('target-wrap',dict(link=(HEAP,0xFFD2))),('a20-target',dict(link=(0xFFFF,0xFFF0))),('a20-stack',dict(ss=0xFFFF)),('a20-data',dict(ds=0xFFFF)),('relocated',dict(caller_cs=0x5800,dice_cs=0x6800,stub=0x7800)),('random-background',dict(background=random.Random(9).randbytes(SIZE)))])
    for mode,minsp in (('caller',0x40),('latch',0x20)):
        for sp in (minsp,minsp+1,0x8001,0xFFFB,0xFFFC):out.append((f'{mode}-sp-{sp:X}',dict(mode=mode,sp=sp)))
        for sp in (0x8000,0x7000,0x6818,0x681E,0x6820,0x6830):
            out.append((f'{mode}-ss-ds-{sp:X}',dict(mode=mode,ss=DS,sp=sp)))
    for delta in (-4,-2,0,2,4,6,8,10,12):
        out.append((f'character-stack-{delta}',dict(character=(SS,(0x8000-0xCC+delta)&65535))))
    return out
def compare(label,m,c,decoded,conditional,functions,*,mode='caller',max_calls=256,caller_cs=CS,dice_cs=DICE,stub=STUB):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP,UC_X86_REG_AX
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,latch=-1,dice=-1,previous=None,returned=False,returning=None,calls=[],system_calls=0,unexpected=None,rolls=[])
    seen,edges=set(),set()
    def hook(emulator,address,size,user):
        del address,size,user
        cs,ip=emulator.reg_read(UC_X86_REG_CS),emulator.reg_read(UC_X86_REG_IP)
        key=(CS if cs==caller_cs else DICE if cs==dice_cs else cs,ip)
        def stop(s):state['stop']=s;emulator.emu_stop()
        def dice_stop(s):
            state['dice']=s
            if mode=='caller':state['latch']=3
            stop(3)
        if state['returned']:return stop(0)
        if state['returning']:
            prev=state['returning'];state['returning']=None
            if prev==(DICE,0xE2B):state['dice']=0
            elif prev==(DICE,0xE4F):state['latch']=0
            if (cs,ip)!=state['calls'].pop():
                if prev==(SYSTEM,0x1244):return dice_stop(6)
                if prev==(SYSTEM,0x11D3):return dice_stop(7)
                if prev==(DICE,0xE2B):
                    if mode=='caller':state['latch']=4;return stop(3)
                    return stop(4)
                return stop(4)
        if key==(CS,0x2B52):
            valid=bytes(emulator.mem_read(physical(cs,ip),3))==b'\x9a\x4d\x00'
            segment=int.from_bytes(emulator.mem_read(physical(cs,ip+3),2),'little')
            valid=valid and bytes(emulator.mem_read(physical(segment,0x4D),3))==b'\xea\x2e\x0e'
            if not valid:return stop(2)
        if key==(DICE,0xE43) and bytes(emulator.mem_read(physical(cs,ip),3))!=b'\xe8\x9d\xff':
            if mode=='caller':state['latch']=2;return stop(3)
            return stop(2)
        if key==(DICE,0xE0E):
            if state['system_calls']>=max_calls:return dice_stop(11)
            state['system_calls']+=1
        if key==(CS,0x2B57):state['rolls'].append(emulator.reg_read(UC_X86_REG_AX)&255)
        if cs==stub and ip==0x4D:
            state['previous']=None;return
        if key not in decoded:state['unexpected']=(cs,ip);return emulator.emu_stop()
        if state['previous'] in conditional:edges.add((state['previous'],key))
        seen.add(key);ins=decoded[key]
        if ins.mnemonic in ('call','lcall'):state['calls'].append((cs,ip+ins.size))
        if ins.mnemonic in ('ret','retf'):
            if key==((CS,0x2B60) if mode=='caller' else (DICE,0xE4F)):state['returned']=True
            else:state['returning']=key
        state['previous']=key
    def mirror(emulator,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);emulator.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror);u.emu_start(physical(c.cs,c.ip),2*SIZE,count=3000000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original stop {label}: {state}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=Diagnostic(-1,-1);s=functions[mode](ram,SIZE,ctypes.byref(c),max_calls,ctypes.byref(d))
    check((s,d.latch_status,d.dice_status)==(state['stop'],state['latch'],state['dice']),f'{label}: statuses {(s,d.latch_status,d.dice_status)} vs {state}')
    want=bytes(u.mem_read(0,SIZE));actual=bytes(ram)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'{label}: {len(diff)} RAM differences {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):check(getattr(c,name)==u.reg_read(reg)&65535,f'{label}: {name} C{getattr(c,name):04X} DOS{u.reg_read(reg)&65535:04X}')
    return s,d.latch_status,d.dice_status,seen,edges,state['rolls']
def differential(caller,dice,system,decoded,conditional,functions):
    seen,edges,rolls=set(),set(),set();stops={};modes={};rng=random.Random(0x2B20)
    specs=[(label,opts,256) for label,opts in cases()]
    for mode in ('caller','latch'):
        for ss in (SS,DS,0xFFFF):
            specs.append((mode+'-budget-zero',dict(mode=mode,ss=ss),0))
            if mode=='latch':specs.append(('latch-budget-one',dict(mode=mode,ss=ss,count=3),1))
    for seg,site,data in ((CS,0x2B52,b'\x90'),(CS,0x2B53,b'\x4e\x00'),(STUB,0x4D,b'\x90'),(STUB,0x4E,b'\x2f\x0e'),(DICE,0xE43,b'\x90'),(DICE,0xE44,b'\x9e\xff')):
        specs.append((f'bad-{seg:X}-{site:X}',dict(writes=((seg,site,data),)),256))
        if seg==DICE:specs.append((f'latch-bad-{site:X}',dict(mode='latch',writes=((seg,site,data),)),256))
    specs.append(('latch-limit15',dict(mode='latch',count=16),15))
    for mode,positions in (('caller',(0x4450,0x444E,0x4442,0x4436,0x4428)),('latch',(0x4442,0x4440,0x4434,0x4428))):
        for sp in positions:specs.append((f'{mode}-seed-frame-{sp:X}',dict(mode=mode,ss=DS,sp=sp,seed=0),256))
    for label,opts,budget in specs:
        mode=opts.get('mode','caller');m,c=build(caller,dice,system,rng,**opts)
        s,l,d,v,e,r=compare(label,m,c,decoded,conditional,functions,mode=mode,max_calls=budget,**{k:opts[k] for k in ('caller_cs','dice_cs','stub') if k in opts})
        seen|=v;edges|=e;rolls|=set(r);key=f'{mode}:{s}/{l}/{d}';stops[key]=stops.get(key,0)+1;modes[mode]=modes.get(mode,0)+1
        if not label.startswith(('seed-','kind-')):print(label,key,flush=True)
    check(set(decoded)<=seen,'missing closure instruction starts')
    out=dict(cases=len(specs),modes=modes,stops=stops,cpu_words=14,ram_bytes=SIZE,closure_starts=len(seen),closure_total=len(decoded),closure_outcomes=len(edges),closure_outcomes_total=2*len(conditional),missing_outcomes=[f'{s:04X}:{i:04X}->{j:04X}' for (s,i),(_,j) in sorted({(a,b) for a,pair in conditional.items() for b in pair}-edges)],caller_roll_results=sorted(rolls))
    print('Original parity',out,flush=True);return out
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:
        with tempfile.TemporaryDirectory() as directory:library(Path(directory))
        print("CHECKER_BUILD_OK")
        return
    caller,dice,system,decoded,conditional,digests=source();report=dict(target=digests,multiplier_sha256=sha256(system[0x1245:0x1247]).hexdigest())
    with tempfile.TemporaryDirectory() as directory:
        fs=library(Path(directory))
        if a.unicorn:report['unicorn']=differential(caller,dice,system,decoded,conditional,fs)
        if a.native:
            from u38_link_roll_2b20_cpu_native import native_audit
            report['native']=native_audit(caller,dice,system,fs)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
