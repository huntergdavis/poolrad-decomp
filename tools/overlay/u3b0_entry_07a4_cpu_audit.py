#!/usr/bin/env python3
"""Complete U3B0 +07A4 original CPU/RAM/OUT/VRAM and native EGA comparison.
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
import res03f1_clear_0137_cpu_audit as clear
import u3b0_leaf_0711_native as flagged
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
from u86_procvars_3413_audit import OVR_SHA
from res03f1_clear_0137_cpu_audit import Ega,Bus,WRITE_FN
from res00ba_display_1693_cpu_audit import PythonEga
CHECKER_ROWS=('OVL_03B0_07A4',)
CS,DS,SS,SYS,GLYPH,FONT,DATA=0x9000,0x1800,0x2800,0x4000,0x8800,0x7000,0x6000
ENTRY,END,RETF=0x7A4,0x8D7,0x8D4
CLEAR=0x8400
SEGMENTS=dict(root=CS,sys=SYS,video=GLYPH,clear=CLEAR)
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
NEAR={0x7DE:(0,1),0x7F9:(0x711,2)}
FAR={0x7CA:(0x137,'clear',0),0x849:(0x626,'video',3),0x869:(0x626,'video',3),0x8B5:(0x1DF,'video',4)}
SOURCES=tuple(dict.fromkeys(('overlay_u3b0_entry_07a4_cpu','overlay_u3b0_entry1_0000','overlay_u3b0_leaf_0711')+printer.SOURCES+clear.SOURCES))
NAMES=('callee','status','clear_callee','clear_status','rect_callee','rect_status','printer_callee','printer_status','printer_glyph_callee','printer_glyph_status','glyph_callee','glyph_status')
class Diagnostic(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in NAMES]
class Limits(ctypes.Structure):_fields_=[(n,ctypes.c_size_t) for n in ('row','clear','rect','printer','glyph')]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media')
    image=bytes(unpack(exe)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    bb,dd,_=printer.source();bodies={('video' if k=='caller' else k,a):b for (k,a),b in bb.items()}
    bb,_,_=clear.source()
    for (k,a),blob in bb.items():
        key=('video' if k=='caller' else k,a)
        check(key not in bodies or bodies[key]==blob,'shared dependency identity');bodies[key]=blob
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)};u=units[0x3B0]
    for a,b,n in ((0,0x67,38),(0x711,ENTRY,52),(ENTRY,END,115)):
        blob=ovr[u['ovr_offset']+a:u['ovr_offset']+b];ins=list(md.disasm(blob,a))
        check(len(ins)==n and sum(i.size for i in ins)==len(blob),'overlay extent')
        bodies['root',a]=blob
    check(sha256(bodies['root',0]).hexdigest()=='a7d25485edf131a638e320475f22544c82fe81967478369e17556db2509b2a3c','selector hash')
    check(sha256(bodies['root',ENTRY]).hexdigest()=='b4d0d09d692e50b31125fa64fbedb3316385f9dd60dc1efb5c74605b2e669eef','caller hash')
    check(bodies['root',0x711]==flagged.original_body(),'flagged traversal identity')
    ins=list(md.disasm(bodies['root',ENTRY],ENTRY))
    check(ins[-1].address==RETF and ins[-1].op_str=='4','caller return')
    check([(i.address,int(i.op_str,16)) for i in ins if i.mnemonic=='call']==[(s,v[0]) for s,v in NEAR.items()],'near sites')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(0x7CA,'0x3f1, 0x137'),(0x849,'0x709, 0x626'),(0x869,'0x709, 0x626'),(0x8B5,'0x709, 0x1df')],'far sites')
    check(all(s+3 in u['fixups'] for s in FAR),'caller fixups')
    decoded={(k,i.address):i for (k,a),b in bodies.items() for i in md.disasm(b,a)}
    meta={f'{k}:{a:04X}':dict(bytes=len(b),instructions=len(list(md.disasm(b,a))),sha256=sha256(b).hexdigest()) for (k,a),b in bodies.items()}
    meta['target']=dict(file_offset=u['ovr_offset']+ENTRY,entry_slots=[i for i,a in enumerate(u['entries']) if a==ENTRY])
    print('Caller',meta['root:07A4'],meta['target'],flush=True)
    return bodies,decoded,meta
def library(directory):
    path=directory/'u3b0entry.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3b0_entry_07a4_cpu.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    lib.pr_u3b0_entry_07a4_cpu.restype=ctypes.c_int;lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies):
    for (k,a),b in bodies.items():p=physical(SEGMENTS[k],a);m[p:p+len(b)]=b
    for site,(_,kind,_) in FAR.items():put16(m,CS,site+3,SEGMENTS[kind])
    for site in clear.FILL_SITES+clear.RECT_SITES:put16(m,CLEAR,site+3,GLYPH if site in clear.RECT_SITES else SYS)
    put16(m,GLYPH,0x143,SYS)
    for site in (0x207,0x63C):put16(m,GLYPH,site+3,SYS)
def build(bodies,*,text=b'Ab',index=0,node_count=3,cycle=False,null=False,context=0x2000,x=2,y=3,color=10,marked_color=5,node_flag=0,bottom=5,right=9,width=8,active=1,mode=1,
          ss=SS,sp=0x8000,flags=2,fontseg=FONT,fontoff=0x1000,pages=(0xA000,0xA200),pattern=0,
          node_seg=DATA,node_off=0x1000,node_flags=None,writes=()):
    gb={('caller' if k=='video' else k,a):b for (k,a),b in bodies.items() if k in ('sys','video')}
    m,c=glyph.build(gb,ss=ss,ds=DS,sp=sp,flags=flags,mode=mode,fontseg=fontseg,fontoff=fontoff,pages=pages,pattern=pattern)
    install(m,bodies);c.cs=CS;c.ip=ENTRY
    # For long strings, later bytes can include the link words; read them live.
    for n in range(node_count):
        off=(node_off+n*0x200)&65535;payload=bytes([len(text)])+text
        for j,v in enumerate(payload):m[physical(node_seg,(off+j)&65535)]=v
        m[physical(node_seg,(off+0x29)&65535)]=node_flags[n%len(node_flags)] if node_flags else node_flag
        nxt=(node_off+((n+1)%node_count)*0x200)&65535
        put16(m,node_seg,(off+0x2A)&65535,nxt if n+1<node_count or cycle else 0)
        put16(m,node_seg,(off+0x2C)&65535,node_seg if n+1<node_count or cycle else 0)
    # Null is ordinary address 0000:0000, not a host null pointer.
    m[0]=0
    put16(m,ss,context+0x14,0 if null else node_off);put16(m,ss,context+0x16,0 if null else node_seg)
    m[physical(ss,(context+0x1C)&65535)]=y&255;m[physical(ss,(context+0x1E)&65535)]=x&255;m[physical(ss,(context+0x22)&65535)]=color&255;m[physical(ss,(context+0x24)&65535)]=marked_color&255
    m[physical(ss,(context+0x18)&65535)]=bottom&255;m[physical(ss,(context+0x1A)&65535)]=right&255
    m[physical(ss,(context-0x54)&65535)]=width&255;m[physical(DS,0x4A1A)]=active&255
    put16(m,ss,sp+4,context);put16(m,ss,sp+6,index)
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,row_limit=65536,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    d=Diagnostic();limits=Limits(row_limit,clear_limit,rect_limit,printer_limit,glyph_limit)
    status=lib.pr_u3b0_entry_07a4_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def owner(key):
    k,ip=key
    if k=='root':return 'select' if ip<0x67 else 'flagged' if ip<ENTRY else 'root'
    if k=='video':return 'printer' if ip>=0x626 else 'glyph' if ip>=0x1DF else 'rect' if ip>=0xB7 else 'page' if ip>=0xA0 else 'reset'
    return k
def compare(name,state,bodies,decoded,lib,row_limit=65536,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);device=PythonEga()
    def vread(em,off,size,user):return sum(device.read((off+j)&65535)<<(8*j) for j in range(size))
    def vwrite(em,off,size,value,user):
        for j in range(size):device.write((off+j)&65535,(value>>(8*j))&255)
    for origin in (0,SIZE):  # the aperture is an independent device; all other backing RAM is flat, including the unused A0000..AFFFF backing bytes
        uc.mem_map_ptr(origin,0xA0000,7,ctypes.addressof(backing));uc.mmio_map(origin+0xA0000,65536,vread,None,vwrite,None)
        uc.mem_map_ptr(origin+0xB0000,0x50000,7,ctypes.addressof(backing)+0xB0000)
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,previous=None,executing=None)
    d=[-1]*12;calls=[];seen=set();edges=set();events=[];writes=[];loops=dict(row=0,clear=0,rect=0,printer=0,glyph=0)
    kinds={v:k for k,v in SEGMENTS.items()}
    def stop(v):st['stop']=v;uc.emu_stop()
    def fail(o,v):
        if o=='root':return stop(v)
        if o=='reset':d[5]=v;return fail('rect',3)
        if o=='rect':d[3]=v;return fail('clear',3)
        if o=='glyph' and st.get('glyph_parent')=='printer':d[7]=v;return fail('printer',3)
        d[1]=v;return stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,caller=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),caller)
                return fail(caller,4 if caller=='root' else 5)
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key];o=owner(key);st['executing']=o
        if st['previous'] is not None:edges.add((st['previous'],key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        expected=None
        if o=='root' and ip in NEAR:
            target,_=NEAR[ip]
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return stop(2)
        if o=='root' and ip in FAR:expected=FAR[ip][0]
        elif o=='clear' and ip in clear.FILL_SITES+clear.RECT_SITES:expected=0xB7 if ip in clear.RECT_SITES else 0x16A8
        elif o=='rect' and ip==0x140:expected=0x16A8
        elif o=='printer' and ip==0x63C:expected=0xB25
        elif o=='glyph' and ip==0x207:expected=0x182A
        if expected is not None and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,expected):return fail(o,2)
        if o=='printer' and ip==0x691 and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return fail(o,2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return fail(o,2 if o=='reset' else 4)
        flag=uc.reg_read(R.UC_X86_REG_EFLAGS)
        limited=('row' if o=='root' and ip==0x8CE else
                 'clear' if o=='clear' and ip in (0x1D2,0x2E0) and not flag&0x40 else
                 'rect' if o=='rect' and ip==0x14F and not flag&0x40 else
                 'printer' if o=='printer' and ip==0x69F and not flag&0x40 else
                 'glyph' if o=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not flag&0x40) else None)
        if limited:
            cap=dict(row=row_limit,clear=clear_limit,rect=rect_limit,printer=printer_limit,glyph=glyph_limit)[limited]
            if loops[limited]==cap:return fail(o,5 if o=='root' else 6)
            loops[limited]+=1
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):
            calls.append(((cs,ip+ins.size),o))
            if o=='root':
                d[0]=NEAR[ip][1] if ip in NEAR else FAR[ip][2];d[1]=0
                if d[0]==0:d[2:6]=[-1]*4;loops['clear']=0
                if d[0]==3:d[6:10]=[-1]*4;loops['printer']=0
                if d[0]==4:d[10:12]=[-1]*2;loops['glyph']=0;st['glyph_parent']='root'
            elif o=='clear':
                d[2]=1 if ip==0x1F1 else 2 if ip==0x20C else 0;d[3]=0
                if ip in clear.RECT_SITES:d[4:6]=[-1]*2;loops['rect']=0
            elif o=='rect':d[4]=2 if ip==0x140 else 1 if ip==0x100 else 0;d[5]=0
            elif o=='printer':
                d[6]=0 if ip==0x63C else 1;d[7]=0
                if ip==0x691:d[8:10]=[-1]*2;loops['glyph']=0;st['glyph_parent']='printer'
            elif o=='glyph':
                at=8 if st['glyph_parent']=='printer' else 10;d[at:at+2]=[0,0]
        if ins.mnemonic in ('ret','retf'):
            if key==('root',RETF):st['root_return']=True
            else:st['ret']=True
    def out(em,port,size,value,user):
        events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)));device.out(port,value,size)
    protected=[(physical(SEGMENTS[k],a),physical(SEGMENTS[k],a)+len(b)) for (k,a),b in bodies.items()]
    def write(em,access,address,size,value,user):
        for j in range(size):
            at=(address+j)&(SIZE-1)
            check(not any(a<=at<b for a,b in protected),f'code mutation {name}/{at:x}')
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT);uc.hook_add(UC_HOOK_MEM_WRITE,write)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=4000000)
    check(st['stop'] is not None,f'instruction ceiling {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,diag=run_c(lib,m,actual,row_limit,clear_limit,rect_limit,printer_limit,glyph_limit,refuse)
    check(status==st['stop'],f'status {name}: {status}/{st}')
    check([getattr(diag,n) for n in NAMES]==d,f'diagnostic {name}: {[getattr(diag,n) for n in NAMES]}/{d}')
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(reg)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n}: C {getattr(actual,n):04X}, original {want:04X}')
    check(mine==events,f'OUTs {name}: {len(mine)}/{len(events)}')
    check(stores==device.writes,f'VRAM operands {name}: {len(stores)}/{len(device.writes)}')
    if ram!=bytes(backing):  # the whole 1,048,576-byte backing array, A0000..AFFFF included
        diff=[(hex(i),x,y) for i,(x,y) in enumerate(zip(ram,backing)) if x!=y]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    if ega!=device.packed():
        want=device.packed();diff=[(hex(i),x,y) for i,(x,y) in enumerate(zip(ega,want)) if x!=y]
        per=[sum(1 for i in range(q*65536,(q+1)*65536) if ega[i]!=want[i]) for q in range(4)]
        raise ValueError(f'EGA {name}: {diff[:8]} ({len(diff)} bytes) planes={per} C-regs={list(ega[262144:])} model-regs={list(want[262144:])} reads={len(device.reads)}')
    return status,seen,edges,len(events),len(device.writes),len(device.reads)
def cases():
    result=[('default',{}),('unmarked',dict(node_flag=0)),('marked',dict(node_flag=255)),
      ('mixed',dict(node_flags=(0,1,255))),('empty',dict(text=b'')),('no-pad',dict(width=0)),
      ('equal-width',dict(width=2)),('narrow-width',dict(width=1)),('all-spaces',dict(text=b'   ')),
      ('select1',dict(index=1)),('select2',dict(index=2)),('past-end',dict(index=3)),('null',dict(null=True)),
      ('row-exit',dict(bottom=3)),('empty-viewport',dict(bottom=2,mode=3)),
      ('cycle',dict(cycle=True,bottom=5)),('cycle-limit',dict(cycle=True,bottom=127,y=127,mode=3,row_limit=3)),
      ('row-limit0',dict(row_limit=0)),('row-limit1',dict(row_limit=1)),
      ('clear-limit0',dict(mode=0,clear_limit=0)),('rect-limit0',dict(rect_limit=0)),
      ('printer-limit0',dict(printer_limit=0)),('glyph-limit0',dict(glyph_limit=0)),
      ('pad-glyph-limit0',dict(text=b'',glyph_limit=0)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('stack-min',dict(sp=0x300)),('stack-max',dict(sp=0xFFF4)),
      ('no-font',dict(fontseg=0,fontoff=0)),('node-wrap',dict(node_off=0xFFF0)),
      ('negative-index',dict(index=0xFFFF,mode=3)),('signed-bound',dict(index=0x8000,mode=3))]
    for mode in range(4):
        for flag in (0,1,128,255):result.append((f'mode{mode}-flag{flag}',dict(mode=mode,node_flag=flag)))
    for length in (0,1,39,40,41,42,127,255):
        result.append((f'length{length}',dict(text=b'A'*length,node_count=1,width=40,mode=3)))
    for active in (0,1,2,7):result.append((f'active{active}',dict(active=active)))
    for f in FLAGS:result.append((f'flags{f:x}',dict(flags=f)))
    for x,y,bottom in ((0,0,0),(39,24,24),(127,127,127),(128,128,128),(255,255,255),(0,127,127)):
        result.append((f'coordinates{x}-{y}',dict(x=x,right=x,y=y,bottom=bottom,node_count=1,width=2)))
    for count in (0,1,4,20,43,44,48,78,112):result.append((f'port{count}',dict(refuse=count)))
    for site in (*NEAR,*FAR):
        for flag in (0,1):result.append((f'bad-root-{site:x}-flag{flag}',dict(node_flag=flag,writes=((CS,site,b'\x90'),))))
    for site in clear.FILL_SITES+clear.RECT_SITES:
        mode=0 if site in clear.FILL_SITES[:2] else 2 if site in clear.FILL_SITES else 1
        result.append((f'bad-clear-{site:x}',dict(mode=mode,writes=((CLEAR,site,b'\x90'),))))
    for site in (0x140,0x63C,0x691,0x207):result.append((f'bad-video-{site:x}',dict(writes=((GLYPH,site,b'\x90'),))))
    return result
def differential(bodies,decoded,lib):
    seen=set();edges=set();stops={};outs=stores=reads=0;allcases=cases()
    for name,options in allcases:
        options=dict(options);limits={k:options.pop(k) for k in tuple(options) if k.endswith('_limit') or k=='refuse'}
        status,s,e,o,w,r=compare(name,build(bodies,**options),bodies,decoded,lib,**limits)
        seen|=s;edges|=e;outs+=o;stores+=w;reads+=r;stops[status]=stops.get(status,0)+1;print(name,status,flush=True)
    expected={((k,a),(k,v)) for (k,a),i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp' for v in (a+i.size,int(i.op_str,16))}
    caller={k for k in decoded if k[0]=='root' and k[1]>=ENTRY};ce={e for e in expected if e[0] in caller}
    return dict(cases=len(allcases),stops=stops,cpu_words=14,ram_bytes=SIZE,ram_excluded_ranges=[],ega_bytes=len(PythonEga().packed()),flag_masks=0,out_events=outs,vram_write_operands=stores,ega_reads=reads,
        caller_starts=len(seen&caller),caller_total=len(caller),caller_outcomes=len(edges&ce),caller_outcomes_total=len(ce),
        chain_starts=len(seen),chain_total=len(decoded),chain_outcomes=len(edges&expected),chain_outcomes_total=len(expected),
        missing_starts=sorted(set(decoded)-seen),missing_outcomes=sorted(expected-edges))
def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    if a.compile_only:  # same compiler command, shared library load and symbol binding as the audit; no media
        with tempfile.TemporaryDirectory() as tmp:library(Path(tmp))
        print('CHECKER_BUILD_OK');return
    bodies,decoded,meta=source();report=dict(identity=meta)
    with tempfile.TemporaryDirectory() as tmp:
        lib=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib)
        if a.native:
            sys.modules.setdefault('u3b0_entry_07a4_cpu_audit',sys.modules[__name__])
            from u3b0_entry_07a4_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print({k:({kk:vv for kk,vv in v.items() if not kk.startswith('missing')} if k!='identity' else v) for k,v in report.items()})
if __name__=='__main__':main()
