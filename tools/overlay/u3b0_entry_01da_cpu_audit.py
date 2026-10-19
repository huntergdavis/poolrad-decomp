#!/usr/bin/env python3
"""Complete U3B0 +01DA original CPU/RAM/OUT/VRAM and native EGA comparison.
Original media and generated programs remain under ignored resources/.
"""
import argparse,ctypes,json,struct,subprocess,tempfile,sys,random
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
import res0709_glyph_01df_cpu_audit as glyph
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
from u86_procvars_3413_audit import OVR_SHA
from res03f1_clear_0137_cpu_audit import Ega,Bus,WRITE_FN
CS,DS,SS,SYS,GLYPH,FONT,DATA=0x9000,0x1800,0x2800,0x4000,0x8800,0x7000,0x6000
ENTRY,END,RETF=0x1DA,0x37D,0x37A
SEGMENTS=dict(root=CS,sys=SYS,video=GLYPH,rootdata=CS)
FLAGS=glyph.FLAGS
FAR={0x273:(0x1DF,'video',1),0x28F:(0xDC0,'sys',0),0x2CA:(0x1DF,'video',1),0x305:(0x1DF,'video',1),0x372:(0x1DF,'video',1)}
SOURCES=('overlay_u3b0_entry_01da_cpu',)+glyph.SOURCES
NAMES=('callee','status','glyph_callee','glyph_status')
class Diagnostic(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in NAMES]
class Limits(ctypes.Structure):_fields_=[(n,ctypes.c_size_t) for n in ('caller','glyph')]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media')
    image=bytes(unpack(exe)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    bb,_,_=glyph.source();bodies={('video' if k=='caller' else k,a):b for (k,a),b in bb.items()}
    bodies['sys',0xDC0]=image[physical(0xAF8,0xDC0):physical(0xAF8,0xDE1)]
    u=next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr) if u['stub_para']==0x3B0)
    base=u['ovr_offset'];bodies['root',ENTRY]=ovr[base+ENTRY:base+END];bodies['rootdata',0x1BA]=ovr[base+0x1BA:base+ENTRY]
    check(sha256(bodies['root',ENTRY]).hexdigest()=='8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce','caller identity')
    check(sha256(bodies['rootdata',0x1BA]).hexdigest()=='2d66b9ae33311a4e813f8a5263360a0aaeb0e64d86f53bdeeb2f0849d5e2cbe0','set identity')
    ins=list(md.disasm(bodies['root',ENTRY],ENTRY));check(len(ins)==172 and sum(i.size for i in ins)==419,'caller extent')
    check(ins[-1].address==RETF and ins[-1].op_str=='4','caller return')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(s,'0xaf8, 0xdc0' if t==0xDC0 else '0x709, 0x1df') for s,(t,_,_) in FAR.items()],'far sites')
    check(all(s+3 in u['fixups'] for s in FAR),'caller fixups')
    decoded={(k,i.address):i for (k,a),b in bodies.items() if not k.endswith('data') for i in md.disasm(b,a)}
    meta={f'{k}:{a:04X}':dict(bytes=len(b),instructions=0 if k.endswith('data') else len(list(md.disasm(b,a))),sha256=sha256(b).hexdigest()) for (k,a),b in bodies.items()}
    meta['target']=dict(file_offset=base+ENTRY,entry_slots=[i for i,a in enumerate(u['entries']) if a==ENTRY],queue_bytes=483,excluded_trailing_data_bytes=64)
    print('Caller',meta['root:01DA'],meta['target'],flush=True)
    return bodies,decoded,meta
def library(directory):
    path=directory/'u3b0entry.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3b0_entry_01da_cpu.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    lib.pr_u3b0_entry_01da_cpu.restype=ctypes.c_int;lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies):
    for (k,a),b in bodies.items():p=physical(SEGMENTS[k],a);m[p:p+len(b)]=b
    for site,(_,kind,_) in FAR.items():put16(m,CS,site+3,SEGMENTS[kind])
    put16(m,GLYPH,0x20A,SYS)
def build(bodies,*,context=0x4000,selection=1,text=b'Ab:Z',length=None,column=2,first=1,last=2,selected=15,normal=10,set_bytes=None,
          mode=1,ss=SS,sp=0x8000,flags=2,fontseg=FONT,fontoff=0x1000,pages=(0xA000,0xA200),pattern=0,writes=()):
    gb={('caller' if k=='video' else k,a):b for (k,a),b in bodies.items() if k in ('sys','video')}
    m,c=glyph.build(gb,ss=ss,ds=DS,sp=sp,flags=flags,mode=mode,fontseg=fontseg,fontoff=fontoff,pages=pages,pattern=pattern)
    install(m,bodies);c.cs=CS;c.ip=ENTRY
    for j,v in enumerate(text):m[physical(ss,(context-0x52+j)&65535)]=v
    for off,v in ((2*(selection&255)-0x8F,first),(2*(selection&255)-0x8E,last),(-0x64,len(text) if length is None else length),(-0x65,column),(14,selected),(16,normal)):
        m[physical(ss,(context+off)&65535)]=v&255
    put16(m,ss,sp+4,context);put16(m,ss,sp+6,selection)
    if set_bytes is not None:
        check(len(set_bytes)==32,'set size');m[physical(CS,0x1BA):physical(CS,ENTRY)]=set_bytes
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,caller_limit=65536,glyph_limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    d=Diagnostic();limits=Limits(caller_limit,glyph_limit)
    status=lib.pr_u3b0_entry_01da_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def owner(key):return 'glyph' if key[0]=='video' else key[0]
def compare(name,state,bodies,decoded,lib,caller_limit=65536,glyph_limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,previous=None,executing=None)
    d=[-1]*4;calls=[];seen=set();edges=set();events=[];writes=[];loops=dict(caller=0,glyph=0)
    kinds={CS:"root",SYS:"sys",GLYPH:"video"}
    def stop(v):st['stop']=v;uc.emu_stop()
    def fail(o,v):
        if o=='root':return stop(v)
        d[1]=v;return stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,caller=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),caller)
                return fail(caller,5 if caller=='glyph' else 4)
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key];o=owner(key);st['executing']=o
        if st['previous'] is not None:edges.add((st['previous'],key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        expected=FAR[ip][0] if o=='root' and ip in FAR else 0x182A if o=='glyph' and ip==0x207 else None
        if expected is not None and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,expected):return fail(o,2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return fail('glyph',4)
        flag=uc.reg_read(R.UC_X86_REG_EFLAGS)
        limited=('caller' if o=='root' and ip==0x312 else
                 'glyph' if o=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not flag&0x40) else None)
        if limited:
            cap=caller_limit if limited=='caller' else glyph_limit
            if loops[limited]==cap:return fail(o,5 if o=='root' else 6)
            loops[limited]+=1
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):
            calls.append(((cs,ip+ins.size),o))
            if o=='root':
                d[0]=FAR[ip][2];d[1]=0
                if d[0]==1:d[2:4]=[-1]*2;loops['glyph']=0
            elif o=='glyph':d[2:4]=[0,0]
        if ins.mnemonic in ('ret','retf'):
            if key==('root',RETF):st['root_return']=True
            else:st['ret']=True
    def out(em,port,size,value,user):events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)))
    protected=[(physical(SEGMENTS[k],a),physical(SEGMENTS[k],a)+len(b)) for (k,a),b in bodies.items()]
    def write(em,access,address,size,value,user):
        for j in range(size):
            at=(address+j)&(SIZE-1)
            check(not any(a<=at<b for a,b in protected),f'code mutation {name}/{at:x}')
            if 0xA0000<=at<0xB0000:
                check(st['executing']=='glyph',f'flat-RAM aperture write {name}/{st["executing"]}')
                writes.append((at-0xA0000,(value>>(8*j))&255))
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT);uc.hook_add(UC_HOOK_MEM_WRITE,write)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=4000000)
    check(st['stop'] is not None,f'instruction ceiling {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,diag=run_c(lib,m,actual,caller_limit,glyph_limit,refuse)
    check(status==st['stop'],f'status {name}: {status}/{st}')
    check([getattr(diag,n) for n in NAMES]==d,f'diagnostic {name}: {[getattr(diag,n) for n in NAMES]}/{d}')
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(reg)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n}: C {getattr(actual,n):04X}, original {want:04X}')
    check(mine==events,f'OUTs {name}: {len(mine)}/{len(events)}')
    check(stores==writes,f'VRAM operands {name}: {len(stores)}/{len(writes)}')
    expected=bytes(backing);a=ram[:0xA0000]+ram[0xB0000:];b=expected[:0xA0000]+expected[0xB0000:]
    if a!=b:
        diff=[(hex(i),x,y) for i,(x,y) in enumerate(zip(a,b)) if x!=y]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    return status,seen,edges,len(events),len(writes)
def cases():
    result=[('default',{}),('empty',dict(text=b'')),('all-selected',dict(first=1,last=255)),
      ('all-member',dict(first=0,last=0,set_bytes=bytes([255])*32)),('all-normal',dict(first=0,last=0,set_bytes=bytes(32))),
      ('zero-selected',dict(selected=0,first=1,last=255)),('reversed-range',dict(first=3,last=1)),
      ('caller-limit0',dict(caller_limit=0)),('caller-limit1',dict(caller_limit=1)),
      ('glyph-limit0',dict(glyph_limit=0)),('glyph-limit1',dict(glyph_limit=1)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('stack-min',dict(sp=0x40)),('stack-max',dict(sp=0xFFF4)),
      ('context-wrap',dict(context=0x20)),('no-font',dict(fontseg=0,fontoff=0)),
      ('tail-38',dict(text=b'A',column=37)),('no-tail-39',dict(text=b'A',column=38)),('no-tail-40',dict(text=b'A',column=39)),
      ('tail-only-guard',dict(writes=((CS,0x372,b'\x90'),))),
      ('bad-selected',dict(first=1,last=255,writes=((CS,0x273,b'\x90'),))),
      ('bad-set',dict(first=0,last=0,writes=((CS,0x28F,b'\x90'),))),
      ('bad-member',dict(first=0,last=0,set_bytes=bytes([255])*32,writes=((CS,0x2CA,b'\x90'),))),
      ('bad-normal',dict(first=0,last=0,set_bytes=bytes(32),writes=((CS,0x305,b'\x90'),))),
      ('bad-upcase',dict(writes=((GLYPH,0x207,b'\x90'),)))]
    for length in (1,2,38,39,40,95,96,97,127,128,254,255):result.append((f'length{length}',dict(text=b'A'*length,mode=3)))
    for selection in (0,2,3,63,64,127,128,255,0xFF01):result.append((f'selection{selection}',dict(selection=selection,mode=3)))
    for column in (0,1,39,127,128,254,255):result.append((f'column{column}',dict(column=column)))
    for mode in range(4):
        for flags in FLAGS:result.append((f'mode{mode}-flags{flags:x}',dict(mode=mode,flags=flags)))
    for ch in (0,7,8,31,32,63,64,127,128,255):
        result.append((f'element{ch}',dict(text=bytes([ch]),first=0,last=0,set_bytes=bytes([0xAA])*32,mode=3)))
    for n in (0,1,4,33,34,68,136):result.append((f'port{n}',dict(refuse=n)))
    return result
def differential(bodies,decoded,lib):
    seen=set();edges=set();stops={};outs=stores=0;allcases=cases()
    for name,options in allcases:
        options=dict(options);limits={k:options.pop(k) for k in tuple(options) if k.endswith('_limit') or k=='refuse'}
        status,s,e,o,w=compare(name,build(bodies,**options),bodies,decoded,lib,**limits)
        seen|=s;edges|=e;outs+=o;stores+=w;stops[status]=stops.get(status,0)+1;print(name,status,flush=True)
    expected={((k,a),(k,v)) for (k,a),i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for v in (a+i.size,int(i.op_str,16))}
    caller={k for k in decoded if k[0]=='root' and k[1]>=ENTRY};ce={e for e in expected if e[0] in caller}
    return dict(cases=len(allcases),stops=stops,cpu_words=14,ordinary_ram_bytes=SIZE-65536,flag_masks=0,out_events=outs,vram_write_operands=stores,
        caller_starts=len(seen&caller),caller_total=len(caller),caller_outcomes=len(edges&ce),caller_outcomes_total=len(ce),
        chain_starts=len(seen),chain_total=len(decoded),chain_outcomes=len(edges&expected),chain_outcomes_total=len(expected),
        missing_starts=sorted(set(decoded)-seen),missing_outcomes=sorted(expected-edges))
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,decoded,meta=source();report=dict(identity=meta)
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib)
        if a.native:
            sys.modules.setdefault('u3b0_entry_01da_cpu_audit',sys.modules[__name__])
            from u3b0_entry_01da_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print({k:({kk:vv for kk,vv in v.items() if not kk.startswith('missing')} if k!='identity' else v) for k,v in report.items()})
if __name__=='__main__':main()
