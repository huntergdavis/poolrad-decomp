#!/usr/bin/env python3
"""Complete U7D +05F0 original CPU/RAM/OUT/VRAM and native EGA comparison.
Original media and generated programs remain under ignored resources/.
"""
import argparse,ctypes,json,struct,subprocess,tempfile,sys,random
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
import res0709_printer_0626_cpu_audit as printer
import res0709_glyph_01df_cpu_audit as glyph
import u7d_two_digits_0565_cpu_audit as number
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
from u86_procvars_3413_audit import OVR_SHA
from res03f1_clear_0137_cpu_audit import Ega,Bus,WRITE_FN
CS,DS,SS,SYS,GLYPH,FONT,DATA=0x9000,0x1800,0x2800,0x4000,0x8800,0x7000,0x6000
ENTRY,END,RETF=0x5F0,0x6D8,0x6D5
RES=0x8400
SEGMENTS=dict(root=CS,sys=SYS,video=GLYPH,res=RES,rootdata=CS,sysdata=SYS)
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
NEAR={0x674:(0x565,1),0x6CA:(0x565,1)}
FAR={0x633:(0xB0B,'sys',0),0x638:(0x626,'video',2),0x677:(0x626,'video',2),0x695:(0xB0B,'sys',0),0x69A:(0x626,'video',2),0x6CD:(0x626,'video',2)}
SOURCES=tuple(dict.fromkeys(('overlay_u7d_entry_05f0_cpu','overlay_u7d_two_digits_0565_cpu','resident_00ba_byte_string_1037_cpu','system_str_1366_cpu','system_string_leaves_cpu','system_string_move_leaves_cpu')+printer.SOURCES))
NAMES=('callee','status','format_callee','format_status','printer_callee','printer_status','glyph_callee','glyph_status')
class Diagnostic(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in NAMES]
class Limits(ctypes.Structure):_fields_=[(n,ctypes.c_size_t) for n in ('caller','printer','glyph')]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media')
    _,code,image,_,_=number.source();md=Cs(CS_ARCH_X86,CS_MODE_16)
    bb,_,_=printer.source();bodies={('video' if k=='caller' else k,a):b for (k,a),b in bb.items()}
    for a,b,count in number.SYSTEM_SPANS.values():
        blob=image[physical(number.SYSTEM_CS,a):physical(number.SYSTEM_CS,b)]
        check(('sys',a) not in bodies or bodies['sys',a]==blob,'shared SYSTEM identity');bodies['sys',a]=blob
    a,b,_=number.RES_SPAN;bodies['res',a]=image[physical(number.RES_CS,a):physical(number.RES_CS,b)]
    bodies['root',0x565]=code[0x565:0x5E3];bodies['root',ENTRY]=code[ENTRY:END]
    bodies['rootdata',0x563]=code[0x563:0x565];bodies['rootdata',0x5E3]=code[0x5E3:0x5F0]
    bodies['sysdata',number.TABLE]=image[physical(number.SYSTEM_CS,number.TABLE):physical(number.SYSTEM_CS,number.TABLE+40)]
    check(sha256(bodies['root',ENTRY]).hexdigest()=='ac3eb0a915fa30d77b06b262429879f2400d3b8584b6e97ba413dde924709c7d','caller hash')
    check(len(bodies['root',ENTRY])==232,'caller bytes')
    check(sha256(code[0x5E3:0x5EE]).hexdigest()=='2fb758ba5e4efe1d14056efb9e1c64c33bac1464d7e8226c47d503f0ee3d3bba','title identity')
    check(sha256(code[0x5EE:0x5F0]).hexdigest()=='63ded01247acc7888c74eae4707f8bd28a532c761ebcbe16d3e0bb7996945a28','separator identity')
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)};u=units[0x7D]
    ins=list(md.disasm(bodies['root',ENTRY],ENTRY));check(len(ins)==102 and sum(i.size for i in ins)==232,'caller instruction extent')
    check(ins[-1].address==RETF and ins[-1].op_str=='2','caller return')
    check([(i.address,int(i.op_str,16)) for i in ins if i.mnemonic=='call']==[(s,v[0]) for s,v in NEAR.items()],'near sites')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(s,'0xaf8, 0xb0b' if v[0]==0xB0B else '0x709, 0x626') for s,v in FAR.items()],'far sites')
    check(all(s+3 in u['fixups'] for s in FAR),'caller fixups')
    decoded={(k,i.address):i for (k,a),b in bodies.items() if not k.endswith('data') for i in md.disasm(b,a)}
    meta={f'{k}:{a:04X}':dict(bytes=len(b),instructions=0 if k.endswith('data') else len(list(md.disasm(b,a))),sha256=sha256(b).hexdigest()) for (k,a),b in bodies.items()}
    meta['target']=dict(file_offset=u['ovr_offset']+ENTRY,entry_slots=[i for i,a in enumerate(u['entries']) if a==ENTRY],queue_bytes=305,excluded_trailing_data_bytes=73)
    print('Caller',meta['root:05F0'],meta['target'],flush=True)
    return bodies,decoded,meta
def library(directory):
    path=directory/'u3b0entry.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u7d_entry_05f0_cpu.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    lib.pr_u7d_entry_05f0_cpu.restype=ctypes.c_int;lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies):
    for (k,a),b in bodies.items():p=physical(SEGMENTS[k],a);m[p:p+len(b)]=b
    for site,(_,kind,_) in FAR.items():put16(m,CS,site+3,SEGMENTS[kind])
    for site,(_,kind) in number.SITES.items():put16(m,CS,site+3,RES if kind=='res' else SYS)
    for site in number.RES_SITES:put16(m,RES,site+3,SYS)
    for site in (0x207,0x63C):put16(m,GLYPH,site+3,SYS)
def build(bodies,*,selection=2,values=(12,3,45,6),mode=1,ss=SS,sp=0x8000,flags=2,fontseg=FONT,fontoff=0x1000,
          pages=(0xA000,0xA200),pattern=0,title=None,separator=None,writes=()):
    gb={('caller' if k=='video' else k,a):b for (k,a),b in bodies.items() if k in ('sys','video')}
    m,c=glyph.build(gb,ss=ss,ds=DS,sp=sp,flags=flags,mode=mode,fontseg=fontseg,fontoff=fontoff,pages=pages,pattern=pattern)
    install(m,bodies);c.cs=CS;c.ip=ENTRY
    for n,v in enumerate(values):put16(m,DS,0x6DC4+2*n,v)
    put16(m,ss,sp+4,selection)
    if title is not None:
        check(len(title)<=10,'title fixture extent');m[physical(CS,0x5E3):physical(CS,0x5E4)+len(title)]=bytes([len(title)])+title
    if separator is not None:
        check(len(separator)<=1,'separator fixture extent');m[physical(CS,0x5EE):physical(CS,0x5EF)+len(separator)]=bytes([len(separator)])+separator
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,caller_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    d=Diagnostic();limits=Limits(caller_limit,printer_limit,glyph_limit)
    status=lib.pr_u7d_entry_05f0_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def owner(key):
    k,ip=key
    if k=='root':return 'format' if ip<ENTRY else 'root'
    if k=='video':return 'printer' if ip>=0x626 else 'glyph'
    if k=='sys' and 0x1254<=ip<0x13B1:return 'str'
    return k
def compare(name,state,bodies,decoded,lib,caller_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,previous=None,executing=None)
    d=[-1]*8;calls=[];seen=set();edges=set();events=[];writes=[];loops=dict(caller=0,printer=0,glyph=0)
    kinds={CS:"root",SYS:"sys",GLYPH:"video",RES:"res"}
    def stop(v):st['stop']=v;uc.emu_stop()
    def fail(o,v):
        if o=='root':return stop(v)
        if o=='glyph':d[5]=v;return fail('printer',3)
        if o=='str':return fail('res',3)
        if o=='res':d[3]=v;return fail('format',3)
        d[1]=v;return stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,caller=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),caller)
                return fail(caller,5 if caller in ('glyph','printer') else 4)
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key];o=owner(key);st['executing']=o
        if st['previous'] is not None:edges.add((st['previous'],key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        expected=None
        if o=='root' and ip in NEAR:
            target,_=NEAR[ip]
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return stop(2)
        if o=='root' and ip in FAR:expected=FAR[ip][0]
        elif o=='format' and ip in number.SITES:expected=number.SITES[ip][0]
        elif o=='res' and ip in number.RES_SITES:expected=int.from_bytes(number.RES_SITES[ip][1:],'little')
        elif o=='printer' and ip==0x63C:expected=0xB25
        elif o=='glyph' and ip==0x207:expected=0x182A
        if expected is not None and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,expected):return fail(o,2)
        if o=='str' and ip==0x1366 and struct.unpack('<10I',bytes(uc.mem_read(physical(cs,number.TABLE),40)))!=number.POWERS:return fail('str',1)
        if o=='printer' and ip==0x691 and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return fail(o,2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return fail('glyph',4)
        flag=uc.reg_read(R.UC_X86_REG_EFLAGS)
        limited=('caller' if o=='root' and ip in (0x611,0x6A7) and not flag&0x40 else
                 'printer' if o=='printer' and ip==0x69F and not flag&0x40 else
                 'glyph' if o=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not flag&0x40) else None)
        if limited:
            cap=dict(caller=caller_limit,printer=printer_limit,glyph=glyph_limit)[limited]
            if loops[limited]==cap:return fail(o,5 if o=='root' else 6)
            loops[limited]+=1
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):
            calls.append(((cs,ip+ins.size),o))
            if o=='root':
                d[0]=NEAR[ip][1] if ip in NEAR else FAR[ip][2];d[1]=0
                if d[0]==1:d[2:4]=[-1]*2
                if d[0]==2:d[4:8]=[-1]*4;loops['printer']=0
            elif o=='format':d[2]={0x1037:0,0xB25:1,0xB0B:2,0xB98:3,0xB57:4}[number.SITES[ip][0]];d[3]=0
            elif o=='printer':
                d[4]=0 if ip==0x63C else 1;d[5]=0
                if ip==0x691:d[6:8]=[-1]*2;loops['glyph']=0
            elif o=='glyph':d[6:8]=[0,0]
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
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,diag=run_c(lib,m,actual,caller_limit,printer_limit,glyph_limit,refuse)
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
    result=[('default',{}),('empty-labels',dict(title=b'',separator=b'')),
      ('synthetic-labels',dict(title=b'SYNTHETIC!',separator=b':')),('no-font',dict(fontseg=0,fontoff=0)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('stack-min',dict(sp=0x500)),('stack-max',dict(sp=0xFFF6)),
      ('caller-limit0',dict(caller_limit=0)),('caller-limit5',dict(caller_limit=5)),('caller-limit6',dict(caller_limit=6)),
      ('printer-limit0',dict(printer_limit=0)),('printer-limit1',dict(printer_limit=1)),('glyph-limit0',dict(glyph_limit=0)),
      ('glyph-limit1',dict(glyph_limit=1)),('port0',dict(refuse=0)),('port4',dict(refuse=4)),('port340',dict(refuse=340)),
      ('bad-power-table',dict(writes=((SYS,number.TABLE+1,b'\x00'),)))]
    for selection in (0,1,2,3,4,5,6,7,8,9,10,15,127,128,255,0xFF02):result.append((f'selection{selection}',dict(selection=selection)))
    for value in (0,1,9,10,99,100,127,128,255,256,0x1234,0x7FFF,0x8000,0xFFFF):
        result.append((f'values{value}',dict(values=(value,)*4,selection=value&7,mode=3)))
    for mode in range(4):
        for flags in FLAGS:result.append((f'mode{mode}-flags{flags:x}',dict(mode=mode,flags=flags)))
    for site in (*NEAR,*FAR):result.append((f'bad-root-{site:x}',dict(writes=((CS,site,b'\x90'),))))
    for site in number.SITES:
        result.append((f'bad-format-{site:x}',dict(values=(0,0,0,0),writes=((CS,site,b'\x90'),))))
    for site in number.RES_SITES:result.append((f'bad-res-{site:x}',dict(writes=((RES,site,b'\x90'),))))
    for site in (0x63C,0x691,0x207):result.append((f'bad-video-{site:x}',dict(writes=((GLYPH,site,b'\x90'),))))
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
            sys.modules.setdefault('u7d_entry_05f0_cpu_audit',sys.modules[__name__])
            from u7d_entry_05f0_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print({k:({kk:vv for kk,vv in v.items() if not kk.startswith('missing')} if k!='identity' else v) for k,v in report.items()})
if __name__=='__main__':main()
