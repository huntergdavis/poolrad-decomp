#!/usr/bin/env python3
"""U77 +0901 complete raw composition: original CPU/RAM/OUT/VRAM and native EGA.
All original bytes, string literals and generated guests remain in resources/.
"""
import argparse,ctypes,json,struct,subprocess,tempfile,sys
from pathlib import Path
from hashlib import sha256
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from resident_loader_map import unpack
import res03f1_clear_0137_cpu_audit as clear
import res0709_printer_0626_cpu_audit as printer
import res0709_glyph_01df_cpu_audit as glyph
import system_str_res1037_audit as decimal
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
from u86_procvars_3413_audit import OVR_SHA
from res03f1_clear_0137_cpu_audit import Ega,Bus,WRITE_FN
CS,DS,SS,SYS,GLYPH,CLEAR,DECIMAL,FONT,DATA=0x9000,0x1800,0x2800,0x4000,0x8800,0x8000,0x8200,0x7000,0x6000
ENTRY,END,RETF=0x901,0xA4C,0xA49
SEGMENTS=dict(root=CS,sys=SYS,video=GLYPH,clear=CLEAR,decimal=DECIMAL)
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
SITES={0x933:(0x137,'clear',0),0x971:(0x1037,'decimal',1),0x976:(0x626,'video',2),
 0x9AF:(0x1037,'decimal',1),0x9BD:(0xB25,'sys',4),0x9D8:(0xB0B,'sys',3),
 0x9E2:(0xB98,'sys',5),0x9F0:(0xB25,'sys',4),0xA0E:(0xB25,'sys',4),
 0xA28:(0xB0B,'sys',3),0xA32:(0xB98,'sys',5),0xA3C:(0xB98,'sys',5),0xA41:(0x626,'video',2)}
SOURCES=tuple(dict.fromkeys(('overlay_u77_party_0901_cpu',)+clear.SOURCES+printer.SOURCES+
 ('resident_00ba_byte_string_1037_cpu','system_str_1366_cpu','system_string_leaves_cpu')))
NAMES=('callee','status','clear_callee','clear_status','rect_callee','rect_status',
 'decimal_callee','decimal_status','printer_callee','printer_status','glyph_callee','glyph_status')
class Diagnostic(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in NAMES]
class Limits(ctypes.Structure):_fields_=[(n,ctypes.c_size_t) for n in ('clear','rect','printer','glyph')]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media')
    image=bytes(unpack(exe)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16);bodies={};decoded={}
    for module in (clear,printer):
        bb,dd,_=module.source()
        for (k,a),b in bb.items():bodies['video' if k=='caller' else k,a]=b
    for seg,k,a,b,n in ((0xBA,'decimal',0x1037,0x1073,28),(0xAF8,'sys',0x1366,0x13B1,39),
        (0xAF8,'sys',0x1254,0x12A6,37),(0xAF8,'sys',0xB0B,0xB25,12),(0xAF8,'sys',0xB98,0xBC4,19)):
        blob=image[physical(seg,a):physical(seg,b)];check(len(list(md.disasm(blob,a)))==n,'span');bodies[k,a]=blob
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)};u=units[0x77]
    body=ovr[u['ovr_offset']+ENTRY:u['ovr_offset']+END];ins=list(md.disasm(body,ENTRY))
    check(len(body)==331 and len(ins)==129 and ins[-1].address==RETF and ins[-1].op_str=='4','root extent')
    check(sha256(body).hexdigest()=='5c88f5a5a192ccfaa0d5df8d6d3b610e129845c73c049c6249b32c2bf9ce3a87','root hash')
    check([(i.address,int(i.op_str.split(',')[1],16)) for i in ins if i.mnemonic=='lcall']==[(s,v[0]) for s,v in SITES.items()],'calls')
    check(all(s+3 in u['fixups'] for s in SITES),'fixups')
    bodies['root',ENTRY]=body
    for (k,a),b in bodies.items():decoded.update({(k,i.address):i for i in md.disasm(b,a)})
    # Read-only source data is loaded with the code, but never decoded as code.
    bodies['root',0x8F8]=ovr[u['ovr_offset']+0x8F8:u['ovr_offset']+ENTRY]
    bodies['sys',0x12A6]=image[physical(0xAF8,0x12A6):physical(0xAF8,0x12CE)]
    check(struct.unpack('<10I',bodies['sys',0x12A6])==decimal.POWERS,'power table')
    meta={f'{k}:{a:04X}':dict(bytes=len(b),instructions=sum(1 for kk,ip in decoded if kk==k and a<=ip<a+len(b)),sha256=sha256(b).hexdigest()) for (k,a),b in bodies.items()}
    meta['target']=dict(file_offset=u['ovr_offset']+ENTRY,entry_slots=[i for i,a in enumerate(u['entries']) if a==ENTRY],excluded_trailing_bytes=0xAB3-END)
    print('Caller',meta['root:0901'],meta['target'],flush=True)
    return bodies,decoded,meta
CHECKER_ROWS = ('OVL_0077_0901',)


def library(directory):
    path=directory/'u77party.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u77_party_0901_cpu.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    lib.pr_u77_party_0901_cpu.restype=ctypes.c_int;lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies):
    for (k,a),b in bodies.items():p=physical(SEGMENTS[k],a);m[p:p+len(b)]=b
    for site,(_,kind,_) in SITES.items():put16(m,CS,site+3,SEGMENTS[kind])
    for site in clear.FILL_SITES+clear.RECT_SITES:put16(m,CLEAR,site+3,GLYPH if site in clear.RECT_SITES else SYS)
    for site in (0x140,0x207,0x63C):put16(m,GLYPH,site+3,SYS)
    for site in (0x1054,0x1068):put16(m,DECIMAL,site+3,SYS)
def build(bodies,*,row=0,value=18,extension=1,color=0,mode=1,ss=SS,sp=0x8000,flags=2,
          fontseg=FONT,fontoff=0x1000,pages=(0xA000,0xA200),pattern=0,recordseg=DATA,recordoff=0x1000,writes=()):
    # Reuse the accepted glyph's synthetic font, masks and page setup.
    gb={('caller' if k=='video' else k,a):b for (k,a),b in bodies.items() if k=='sys' or k=='video'}
    m,c=glyph.build(gb,ss=ss,ds=DS,sp=sp,flags=flags,mode=mode,fontseg=fontseg,fontoff=fontoff,pages=pages,pattern=pattern)
    install(m,bodies);c.cs=CS;c.ip=ENTRY
    put16(m,ss,sp+4,0xA500|(color&255));put16(m,ss,sp+6,0xA500|(row&255))
    put16(m,DS,0x5D92,recordoff);put16(m,DS,0x5D94,recordseg)
    signed=row if row<128 else row-256
    m[physical(recordseg,(recordoff+0x10+signed)&65535)]=value
    m[physical(recordseg,(recordoff+0x16)&65535)]=extension
    m[physical(DS,0x4A1A)]=1
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    d=Diagnostic();limits=Limits(clear_limit,rect_limit,printer_limit,glyph_limit)
    status=lib.pr_u77_party_0901_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def owner(key):
    k,ip=key
    if k in ('root','clear','decimal'):return k
    if k=='video':
        for a,b,n in ((0,0x71,'reset'),(0xA0,0xB7,'page'),(0xB7,0x171,'rect'),(0x1DF,0x5EB,'glyph'),(0x626,0x6A7,'printer')):
            if a<=ip<b:return n
    if k=='sys':
        for a,b,n in ((0xB0B,0xB25,'load'),(0xB25,0xB49,'store'),(0xB98,0xBC4,'cat'),(0x1254,0x12A6,'core'),(0x1366,0x13B1,'str'),(0x16A8,0x16BC,'fill'),(0x182A,0x183D,'upcase')):
            if a<=ip<b:return n
    return '?'
def compare(name,state,bodies,decoded,lib,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,previous=None,executing=None)
    d=[-1]*12;calls=[];seen=set();edges=set();events=[];writes=[];loops=dict(clear=0,rect=0,printer=0,glyph=0)
    kinds={v:k for k,v in SEGMENTS.items()}
    def stop(v):st['stop']=v;uc.emu_stop()
    def fail(o,v):
        if o=='root':return stop(v)
        if o=='reset':d[5]=v;return fail('rect',3)
        if o=='rect':d[3]=v;return fail('clear',3)
        if o=='clear':d[1]=v;return stop(3)
        if o=='glyph':d[9]=v;return fail('printer',3)
        if o=='printer':d[1]=v;return stop(3)
        if o=='str':d[7]=v;return fail('decimal',3)
        if o=='decimal':d[1]=v;return stop(3)
        raise ValueError(f'unhandled stop {o}/{v}')
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,caller=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),caller)
                return fail(caller,4 if caller in ('root','decimal') else 5)
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key];o=owner(key);st['executing']=o
        if st['previous'] is not None:edges.add((st['previous'],key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        expected=None
        if o=='root' and ip in SITES:expected=SITES[ip][0]
        elif o=='clear' and ip in clear.FILL_SITES+clear.RECT_SITES:expected=0xB7 if ip in clear.RECT_SITES else 0x16A8
        elif o=='rect' and ip==0x140:expected=0x16A8
        elif o=='printer' and ip==0x63C:expected=0xB25
        elif o=='glyph' and ip==0x207:expected=0x182A
        elif o=='decimal' and ip in (0x1054,0x1068):expected=0x1366 if ip==0x1054 else 0xB25
        if expected is not None and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,expected):return fail(o,2)
        if o=='printer' and ip==0x691 and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return fail(o,2)
        if key==('sys',0x1366) and bytes(uc.mem_read(physical(SYS,0x12A6),40))!=struct.pack('<10I',*decimal.POWERS):return fail('str',1)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return fail(o,2 if o=='reset' else 4)
        flag=uc.reg_read(R.UC_X86_REG_EFLAGS)
        limited=(o=='clear' and ip in (0x1D2,0x2E0) and not flag&0x40 or
                 o=='rect' and ip==0x14F and not flag&0x40 or
                 o=='printer' and ip==0x69F and not flag&0x40 or
                 o=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not flag&0x40))
        if limited:
            cap=dict(clear=clear_limit,rect=rect_limit,printer=printer_limit,glyph=glyph_limit)[o]
            if loops[o]==cap:return fail(o,6)
            loops[o]+=1
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):
            calls.append(((cs,ip+ins.size),o))
            if o=='root':
                d[0]=SITES[ip][2];d[1]=0
                if d[0]==0:d[2:6]=[-1]*4;loops['clear']=0
                if d[0]==1:d[6:8]=[-1]*2
                if d[0]==2:d[8:12]=[-1]*4;loops['printer']=0
            elif o=='clear':
                d[2]=1 if ip==0x1F1 else 2 if ip==0x20C else 0;d[3]=0
                if ip in clear.RECT_SITES:d[4:6]=[-1]*2;loops['rect']=0
            elif o=='rect':d[4]=2 if ip==0x140 else 1 if ip==0x100 else 0;d[5]=0
            elif o=='decimal':d[6]=0 if ip==0x1054 else 1;d[7]=0
            elif o=='printer':
                d[8]=0 if ip==0x63C else 1;d[9]=0
                if ip==0x691:d[10:12]=[-1]*2;loops['glyph']=0
            elif o=='glyph':d[10:12]=[0,0]
        if ins.mnemonic in ('ret','retf'):
            if key==('root',RETF):st['root_return']=True
            else:st['ret']=True
    def out(em,port,size,value,user):events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)))
    protected=[(physical(SEGMENTS[k],a),physical(SEGMENTS[k],a)+len(b)) for (k,a),b in bodies.items()]
    def write(em,access,address,size,value,user):
        for j in range(size):
            at=(address+j)&(SIZE-1)
            check(not any(a<=at<b for a,b in protected),f'code/data mutation {name}/{at:x}')
            if 0xA0000<=at<0xB0000:
                check(st['executing'] in ('glyph','fill'),f'flat-RAM aperture write {name}/{st["executing"]}')
                writes.append((at-0xA0000,(value>>(8*j))&255))
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT);uc.hook_add(UC_HOOK_MEM_WRITE,write)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=4000000)
    check(st['stop'] is not None,f'instruction ceiling {name}')
    actual=Cpu.from_buffer_copy(c)
    status,ram,mine,stores,ega,diag=run_c(lib,m,actual,clear_limit,rect_limit,printer_limit,glyph_limit,refuse)
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
    result=[('default',{}),('no-extension',dict(extension=0)),('not18',dict(value=17)),('row1',dict(row=1)),
      ('extension9',dict(extension=9)),('extension10',dict(extension=10)),('extension99',dict(extension=99)),
      ('extension100',dict(extension=100)),('extension101',dict(extension=101)),('extension255',dict(extension=255)),
      ('no-font',dict(fontseg=0,fontoff=0)),('ss-ds',dict(ss=DS)),('high-stack',dict(ss=0xFFFF)),
      ('stack-min',dict(sp=0x300)),('stack-max',dict(sp=0xFFF4)),
      ('clear-limit',dict(mode=0,clear_limit=0)),('rect-limit',dict(rect_limit=0)),
      ('printer-limit',dict(printer_limit=0)),('glyph-limit',dict(glyph_limit=0)),
      ('port0',dict(refuse=0)),('port3',dict(refuse=3)),('port45',dict(refuse=45)),
      ('bad-table',dict(writes=((SYS,0x12A6,b'\x01'),)))]
    for mode in range(4):
        for value in (0,1,9,10,17,18,99,100,127,128,255):
            result.append((f'mode{mode}-value{value}',dict(mode=mode,value=value,flags=FLAGS[value%6],color=value%2)))
    for row in (2,6,10,18,127,128,249,255):result.append((f'row{row}',dict(row=row,value=18)))
    for site in SITES:
        result.append((f'bad-root-{site:x}',dict(writes=((CS,site,b'\x90'),),extension=100 if site==0xA0E else 1)))
    for seg,site in ((CLEAR,0x1F1),(GLYPH,0x140),(GLYPH,0x207),(GLYPH,0x63C),(GLYPH,0x691),(DECIMAL,0x1054),(DECIMAL,0x1068)):
        result.append((f'bad-nested-{seg:x}-{site:x}',dict(writes=((seg,site,b'\x90'),))))
    return result
def differential(bodies,decoded,lib):
    seen=set();edges=set();stops={};outs=stores=0;allcases=cases()
    for name,options in allcases:
        options=dict(options);limits={k:options.pop(k) for k in tuple(options) if k.endswith('_limit') or k=='refuse'}
        status,s,e,o,w=compare(name,build(bodies,**options),bodies,decoded,lib,**limits)
        seen|=s;edges|=e;outs+=o;stores+=w;stops[status]=stops.get(status,0)+1
        print(name,status,flush=True)
    expected={((k,a),(k,v)) for (k,a),i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for v in (a+i.size,int(i.op_str,16))}
    caller={k for k in decoded if k[0]=='root'};ce={e for e in expected if e[0][0]=='root'}
    return dict(cases=len(allcases),stops=stops,cpu_words=14,ordinary_ram_bytes=SIZE-65536,flag_masks=0,out_events=outs,vram_write_operands=stores,
        caller_starts=len(seen&caller),caller_total=len(caller),caller_outcomes=len(edges&ce),caller_outcomes_total=len(ce),
        chain_starts=len(seen),chain_total=len(decoded),chain_outcomes=len(edges&expected),chain_outcomes_total=len(expected),
        missing_starts=sorted(set(decoded)-seen),missing_outcomes=sorted(expected-edges))
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);p.add_argument('--compile-only',action='store_true');a=p.parse_args()
    if a.compile_only:  # same compiler command and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory(prefix='poolrad-checker-') as _t:
            library(Path(_t))
        print('CHECKER_BUILD_OK')
        return
    bodies,decoded,meta=source();report=dict(identity=meta)
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib)
        if a.native:
            sys.modules.setdefault('u77_party_0901_cpu_audit',sys.modules[__name__])
            from u77_party_0901_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print({k:({kk:vv for kk,vv in v.items() if not kk.startswith('missing')} if k!='identity' else v) for k,v in report.items()})
if __name__=='__main__':main()
