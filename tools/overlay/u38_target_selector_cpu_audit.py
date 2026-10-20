#!/usr/bin/env python3
"""Raw resident selector / U38 101D / U38 25BC: CPU14/full1MiB parity."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
CS,RES,DS,SS,HEAP=0x5000,0xBA,0x1800,0x2800,0x3000
ENTRIES={'resident':0x2E8B,'select':0x101D,'mask':0x25BC}
RETURNS={'resident':(RES,0x2F54),'select':(CS,0x1083),'mask':(CS,0x2602)}
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
CHECKER_ROWS=('OVL_0038_101D',)
SOURCES=('resident_character_target_select.c','overlay_u38_target_select_101d.c','overlay_u38_target_mask81_25bc.c')
NAMES={0:'return',2:'CODE',3:'CALLEE',4:'TRANSFER'}
def check(ok,msg):
    if not ok:raise ValueError(msg)
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media identity')
    unit=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x38)
    check(unit['ovr_offset']==0xEC5B and 0x1048 in unit['fixups'],'unit/fixup identity')
    image=bytes(unpack(exe)[0]);resident=image[physical(RES,0):physical(RES,0x2F57)];code=ovr[0xEC5B:0xEC5B+0x2605]
    spans=((RES,0x2E8B,0x2F57,resident[0x2E8B:],69,'c7aa0a429219bbc7e162996a4af3e1a8e1c95bad8b847edcd0244e42ed54af67'),(CS,0x101D,0x1086,code[0x101D:0x1086],36,'632b609ad14b0fff262cd578a81e144d5dbeab6418ee8ef68303ff9e60cea33a'),(CS,0x25BC,0x2605,code[0x25BC:0x2605],29,'4921365d3d512d870ac24085a63d408b38ab60e6f98e4136f583a7c5c78fe753'))
    decoded={};digests={};md=Cs(CS_ARCH_X86,CS_MODE_16)
    for seg,a,b,blob,count,digest in spans:
        ins=list(md.disasm(blob,a));check(sha256(blob).hexdigest()==digest and len(ins)==count and sum(i.size for i in ins)==b-a,'span identity')
        decoded.update({(seg,i.address):i for i in ins});digests[f'{seg:04X}:{a:04X}-{b:04X}']=dict(bytes=b-a,instructions=count,sha256=digest)
    check([(k,i.op_str) for k,i in decoded.items() if i.mnemonic in ('call','lcall')]==[((CS,0x1045),'0xba, 0x2e8b'),((CS,0x25CB),'0x101d')],'bindings')
    edges={k:((k[0],i.address+i.size),(k[0],int(i.op_str,16))) for k,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    return resident,code,decoded,edges,digests
class Diagnostic(ctypes.Structure):
    _fields_=[('selector_status',ctypes.c_int),('resident_status',ctypes.c_int)]
def library(directory):
    path=directory/'selector.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));functions={}
    for mode,name in [('resident','pr_res_target_select_2e8b_cpu'),('select','pr_u38_target_select_101d_cpu'),('mask','pr_u38_target_mask81_25bc_cpu')]:
        f=getattr(lib,name);f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]+([ctypes.c_int] if mode=='mask' else [])+([] if mode=='resident' else [ctypes.POINTER(Diagnostic)]);f.restype=ctypes.c_int;functions[mode]=f
    return functions

def build(resident,code,rng,*,mode='mask',bits=0x99,mask=0x81,amount=0xA7,index=7,character=(HEAP,0x1000),link=(HEAP,0x2000),first=(HEAP,0x3000),second=(HEAP,0x4000),output=(DS,0x6000),sp=0x8000,ss=SS,ds=DS,res=RES,flags=2,writes=(),background=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    for seg,data in ((CS,code),(res,resident)):
        p=physical(seg,0)
        for j,v in enumerate(data):m[(p+j)&(SIZE-1)]=v
    put16(m,CS,0x1048,res)
    for field,pointer in ((0xCC,link),(0xF8,first),(0xFC,second)):
        put16(m,character[0],(character[1]+field)&65535,pointer[1]);put16(m,character[0],(character[1]+field+2)&65535,pointer[0])
    for j,pointer in enumerate((link,first,second)):
        if pointer!=(0,0):m[physical(pointer[0],(pointer[1]+0x2E)&65535)]=(index+2*j)&255
        m[physical(ds,0x5589+16*((index+2*j)&255))]=mask
    m[physical(ds,0x5590+16*index)]=bits;m[physical(ds,0x6818)]=amount
    put16(m,ds,0x5D92,character[1]);put16(m,ds,0x5D94,character[0])
    args=(output[1],output[0],character[1],character[0]) if mode=='resident' else (character[1],character[0]) if mode=='select' else (0xA100,0xA101,0xA102,0xA103,0xA104)
    for j,v in enumerate((0x7000,0x9001,*args)):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,data in writes:
        p=physical(seg,off)
        for j,v in enumerate(data):m[(p+j)&(SIZE-1)]=v
    return m,Cpu(*[rng.randrange(65536) for _ in range(7)],sp,res if mode=='resident' else CS,ENTRIES[mode],ds,rng.randrange(65536),ss,flags)

def cases():
    out=[]
    for mode in ENTRIES:
        for bits in range(256):out.append((f'{mode}-bits-{bits:02X}',dict(mode=mode,bits=bits,mask=(0,1,0x80,0x81)[bits%4],amount=bits,flags=FLAGS[bits%6])))
        out.extend([(mode+'-nil',dict(mode=mode,link=(0,0))),(mode+'-null-alts',dict(mode=mode,bits=0x99,first=(0,0),second=(0,0))),(mode+'-special-null',dict(mode=mode,bits=0xA,first=(0,0),second=(0,0))),(mode+'-offset-only',dict(mode=mode,link=(0,0x2000))),(mode+'-segment-only',dict(mode=mode,link=(HEAP,0))),(mode+'-char-wrap',dict(mode=mode,character=(HEAP,0xFF33))),(mode+'-index255',dict(mode=mode,index=255)),(mode+'-ss-ds',dict(mode=mode,ss=DS)),(mode+'-a20-stack',dict(mode=mode,ss=0xFFFF)),(mode+'-a20-data',dict(mode=mode,ds=0xFFFF)),(mode+'-resident-relocated',dict(mode=mode,res=0x6000))])
        lo,hi={'resident':(8,0xFFF4),'select':(0x20,0xFFF8),'mask':(0x2E,0xFFFC)}[mode]
        for sp in (lo,lo+1,0x8001,hi-1,hi):out.append((f'{mode}-sp-{sp:X}',dict(mode=mode,sp=sp)))
    for off in (0x6000,0x10CC,0x10F8,0x10FC,0x7FF9,0x7FFA,0x7FFB,0x7FFC,0x7FFE,0x8000,0x8004):out.append((f'output-alias-{off:X}',dict(mode='resident',output=(SS,off),character=(SS,0x1000),link=(SS,0x2000),first=(SS,0x3000),second=(SS,0x4000))))
    for mode in ('mask','select'):
        for site,data in ((0x1045,b'\x90'),(0x1046,b'\x8c\x2e')):out.append((f'{mode}-bad-{site:X}',dict(mode=mode,writes=((CS,site,data),))))
    for site,data in ((0x25CB,b'\x90'),(0x25CC,b'\x50\xea')):out.append((f'mask-bad-{site:X}',dict(writes=((CS,site,data),))))
    for mode in ('mask','select'):
        for delta in (-4,-2,0,2,4,6,8,10,12):
            # Saved character links overlap live stack locals; finite original branches.
            out.append((f'{mode}-stack-record-{delta}',dict(mode=mode,character=(SS,(0x8000-0xCC+delta)&65535))))
    out.append(('random-background',dict(background=random.Random(7).randbytes(SIZE))))
    return out

def compare(label,m,c,decoded,conditional,functions,mode='mask',res=RES):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    state=dict(stop=None,selector=-1,resident=-1,previous=None,returned=False,returning=None,calls=[],unexpected=None);seen,edges=set(),set()
    def hook(emulator,address,size,user):
        del address,size,user
        cs,ip=emulator.reg_read(UC_X86_REG_CS),emulator.reg_read(UC_X86_REG_IP);key=(RES if cs==res else cs,ip)
        def stop(s):state['stop']=s;emulator.emu_stop()
        if state['returned']:return stop(0)
        if state['returning']:
            previous=state['returning'];state['returning']=None
            if previous==(RES,0x2F54):state['resident']=0
            else:state['selector']=0
            if (cs,ip)!=state['calls'].pop():
                if previous==(RES,0x2F54) and mode=='mask':state['selector']=4;return stop(3)
                return stop(4)
        if key in ((CS,0x1045),(CS,0x25CB)):
            want=b'\x9a\x8b\x2e' if ip==0x1045 else b'\xe8\x4f\xea'
            if bytes(emulator.mem_read(physical(cs,ip),3))!=want:
                if ip==0x1045 and mode=='mask':state['selector']=2;return stop(3)
                return stop(2)
        if key not in decoded:state['unexpected']=(cs,ip);return emulator.emu_stop()
        if state['previous'] in conditional:edges.add((state['previous'],key))
        seen.add(key);ins=decoded[key]
        if ins.mnemonic in ('call','lcall'):state['calls'].append((cs,ip+ins.size))
        if ins.mnemonic=='retf':
            if key==RETURNS[mode]:state['returned']=True
            else:state['returning']=key
        state['previous']=key
    def mirror(emulator,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;p=raw&(SIZE-1);emulator.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror);u.emu_start(physical(c.cs,c.ip),2*SIZE,count=10000)
    check(state['unexpected'] is None and state['stop'] is not None,f'original stop {label}: {state}')
    r=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=Diagnostic(-1,-1);f=functions[mode]
    s=f(r,SIZE,ctypes.byref(c),*([0] if mode=='mask' else []),*([] if mode=='resident' else [ctypes.byref(d)]))
    check((s,d.selector_status,d.resident_status)==(state['stop'],state['selector'],state['resident']),f'{label}: status {(s,d.selector_status,d.resident_status)} vs {state}')
    actual=bytes(r);want=bytes(u.mem_read(0,SIZE))
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b];raise ValueError(f'{label}: {len(diff)} RAM mismatches {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):check(getattr(c,name)==u.reg_read(reg)&65535,f'{label}: {name} C{getattr(c,name):04X} DOS{u.reg_read(reg)&65535:04X}')
    return s,seen,edges

def differential(resident,code,decoded,conditional,functions):
    seen,edges,stops=set(),set(),{};rng=random.Random(0x25BC);modes={}
    for label,opts in cases():
        mode=opts.get('mode','mask');m,c=build(resident,code,rng,**opts);s,v,e=compare(label,m,c,decoded,conditional,functions,mode,opts.get('res',RES));seen|=v;edges|=e;name=mode+':'+NAMES[s];stops[name]=stops.get(name,0)+1;modes[mode]=modes.get(mode,0)+1
        if '-bits-' not in label:print(label,NAMES[s],flush=True)
    check(set(decoded)<=seen,'closure starts missing')
    report=dict(cases=sum(stops.values()),modes=modes,stops=stops,cpu_words=14,ram_bytes=SIZE,shift_af_profile='Unicorn: zero',closure_starts=len(seen),closure_total=len(decoded),closure_outcomes=len(edges),closure_outcomes_total=2*len(conditional),missing_starts=[f'{s:04X}:{i:04X}' for s,i in sorted(set(decoded)-seen)],missing_outcomes=[f'{s:04X}:{i:04X}->{j:04X}' for (s,i),(_,j) in sorted({(a,b) for a,pair in conditional.items() for b in pair}-edges)])
    print('Original parity',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='target-selector-') as tmp:
        fs=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        resident,code,decoded,conditional,digests=source();report=dict(target=digests)
        if a.unicorn:report['unicorn']=differential(resident,code,decoded,conditional,fs)
        if a.native:
            from u38_target_selector_cpu_native import native_audit
            report['native']=native_audit(resident,code,fs)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
if __name__=='__main__':main()
