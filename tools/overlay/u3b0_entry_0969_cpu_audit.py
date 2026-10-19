#!/usr/bin/env python3
"""Complete U3B0 +0969 original CPU/RAM/OUT/VRAM and native EGA comparison.
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
import u3b0_space_scan_audit as scans
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
from u86_procvars_3413_audit import OVR_SHA
from res03f1_clear_0137_cpu_audit import Ega,Bus,WRITE_FN
CS,DS,SS,SYS,GLYPH,FONT,DATA=0x9000,0x1800,0x2800,0x4000,0x8800,0x7000,0x6000
ENTRY,END,RETF=0x969,0xA08,0xA05
SEGMENTS=dict(root=CS,sys=SYS,video=GLYPH)
FLAGS=(2,0xD7,0x8D7,0x402,0x893,3)
NEAR={0x97F:(0,0),0x991:(0x8D7,1),0x9A6:(0x923,2)}
FAR={0x9F8:(0xB57,'sys',3),0x9FD:(0x626,'video',4)}
SOURCES=tuple(dict.fromkeys(('overlay_u3b0_entry_0969_cpu','overlay_u3b0_entry1_0000','overlay_u3b0_space_scan_cpu','system_string_move_leaves_cpu')+printer.SOURCES))
NAMES=('callee','status','scan_store_status','printer_callee','printer_status','glyph_callee','glyph_status')
class Diagnostic(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in NAMES]
class Limits(ctypes.Structure):_fields_=[(n,ctypes.c_size_t) for n in ('printer','glyph')]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media')
    image=bytes(unpack(exe)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16)
    bb,dd,_=printer.source();bodies={('video' if k=='caller' else k,a):b for (k,a),b in bb.items()}
    units={u['stub_para']:u for u in find_units(exe[parse_mz(exe)['header_bytes']:],ovr)};u=units[0x3B0]
    for a,b,n in ((0,0x67,38),(0x8D7,0x923,31),(0x923,ENTRY,29),(ENTRY,END,69)):
        blob=ovr[u['ovr_offset']+a:u['ovr_offset']+b];ins=list(md.disasm(blob,a))
        check(len(ins)==n and sum(i.size for i in ins)==len(blob),'overlay extent')
        bodies['root',a]=blob
    check(sha256(bodies['root',0]).hexdigest()=='a7d25485edf131a638e320475f22544c82fe81967478369e17556db2509b2a3c','selector hash')
    check(sha256(bodies['root',ENTRY]).hexdigest()=='7938dda6f949e94aba973ac2a5267c265f8a87fbe176ed0021668cd468b9ac6a','caller hash')
    targets,_,_,_=scans.source()
    for a in (0x8D7,0x923):check(bodies['root',a]==targets[a]['body'],'scan identity')
    blob=image[physical(0xAF8,0xB57):physical(0xAF8,0xB98)]
    check(len(list(md.disasm(blob,0xB57)))==29,'Copy extent');bodies['sys',0xB57]=blob
    ins=list(md.disasm(bodies['root',ENTRY],ENTRY))
    check(ins[-1].address==RETF and ins[-1].op_str=='4','caller return')
    check([(i.address,int(i.op_str,16)) for i in ins if i.mnemonic=='call']==[(s,v[0]) for s,v in NEAR.items()],'near sites')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(0x9F8,'0xaf8, 0xb57'),(0x9FD,'0x709, 0x626')],'far sites')
    check(all(s+3 in u['fixups'] for s in FAR),'caller fixups')
    decoded={(k,i.address):i for (k,a),b in bodies.items() for i in md.disasm(b,a)}
    meta={f'{k}:{a:04X}':dict(bytes=len(b),instructions=len(list(md.disasm(b,a))),sha256=sha256(b).hexdigest()) for (k,a),b in bodies.items()}
    meta['target']=dict(file_offset=u['ovr_offset']+ENTRY,entry_slots=[i for i,a in enumerate(u['entries']) if a==ENTRY])
    print('Caller',meta['root:0969'],meta['target'],flush=True)
    return bodies,decoded,meta
def library(directory):
    path=directory/'u3b0entry.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3b0_entry_0969_cpu.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    lib.pr_u3b0_entry_0969_cpu.restype=ctypes.c_int;lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies):
    for (k,a),b in bodies.items():p=physical(SEGMENTS[k],a);m[p:p+len(b)]=b
    for site,(_,kind,_) in FAR.items():put16(m,CS,site+3,SEGMENTS[kind])
    for site in (0x8EB,0x937):put16(m,CS,site+3,SYS)
    for site in (0x207,0x63C):put16(m,GLYPH,site+3,SYS)
def build(bodies,*,text=b'  Ab  ',index=0,node_count=3,cycle=False,null=False,context=0x2000,x=2,y=3,color=10,base=0,mode=1,
          ss=SS,sp=0x8000,flags=2,fontseg=FONT,fontoff=0x1000,pages=(0xA000,0xA200),pattern=0,
          node_seg=DATA,node_off=0x1000,writes=()):
    gb={('caller' if k=='video' else k,a):b for (k,a),b in bodies.items() if k in ('sys','video')}
    m,c=glyph.build(gb,ss=ss,ds=DS,sp=sp,flags=flags,mode=mode,fontseg=fontseg,fontoff=fontoff,pages=pages,pattern=pattern)
    install(m,bodies);c.cs=CS;c.ip=ENTRY
    # For long strings, later bytes can include the link words; read them live.
    for n in range(node_count):
        off=(node_off+n*0x200)&65535;payload=bytes([len(text)])+text
        for j,v in enumerate(payload):m[physical(node_seg,(off+j)&65535)]=v
        nxt=(node_off+((n+1)%node_count)*0x200)&65535
        put16(m,node_seg,(off+0x2A)&65535,nxt if n+1<node_count or cycle else 0)
        put16(m,node_seg,(off+0x2C)&65535,node_seg if n+1<node_count or cycle else 0)
    # Null is ordinary address 0000:0000, not a host null pointer.
    m[0]=0
    put16(m,ss,context+0x14,0 if null else node_off);put16(m,ss,context+0x16,0 if null else node_seg)
    m[physical(ss,(context+0x1C)&65535)]=y&255;m[physical(ss,(context+0x1E)&65535)]=x&255;m[physical(ss,(context+0x20)&65535)]=color&255
    put16(m,DS,0x6B29,base)
    put16(m,ss,sp+4,context);put16(m,ss,sp+6,index)
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,printer_limit=65536,glyph_limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    d=Diagnostic();limits=Limits(printer_limit,glyph_limit)
    status=lib.pr_u3b0_entry_0969_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def owner(key):
    k,ip=key
    if k=='root':
        if ip<0x67:return 'select'
        if ip<0x923:return 'lead'
        if ip<ENTRY:return 'trail'
        return 'root'
    if k=='video':return 'printer' if ip>=0x626 else 'glyph'
    return 'sys'
def compare(name,state,bodies,decoded,lib,printer_limit=65536,glyph_limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,previous=None,executing=None)
    d=[-1]*7;calls=[];seen=set();edges=set();events=[];writes=[];loops=dict(printer=0,glyph=0)
    kinds={v:k for k,v in SEGMENTS.items()}
    def stop(v):st['stop']=v;uc.emu_stop()
    def fail(o,v):
        if o=='root':return stop(v)
        if o=='glyph':d[4]=v;return fail('printer',3)
        d[1]=v;return stop(3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,caller=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),caller)
                return fail(caller,4 if caller=='root' else 3 if caller in ('lead','trail') else 5)
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key];o=owner(key);st['executing']=o
        if st['previous'] is not None:edges.add((st['previous'],key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        expected=None
        if o=='root' and ip in NEAR:
            target,_=NEAR[ip]
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return stop(2)
        if o=='root' and ip in FAR:expected=FAR[ip][0]
        elif o in ('lead','trail') and ip in (0x8EB,0x937):expected=0xB25
        elif o=='printer' and ip==0x63C:expected=0xB25
        elif o=='glyph' and ip==0x207:expected=0x182A
        if expected is not None and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,expected):return fail(o,2)
        if o=='printer' and ip==0x691 and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return fail(o,2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return fail('glyph',4)
        flag=uc.reg_read(R.UC_X86_REG_EFLAGS)
        limited=(o=='printer' and ip==0x69F and not flag&0x40 or
                 o=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not flag&0x40))
        if limited:
            cap=printer_limit if o=='printer' else glyph_limit
            if loops[o]==cap:return fail(o,6)
            loops[o]+=1
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):
            calls.append(((cs,ip+ins.size),o))
            if o=='root':
                d[0]=NEAR[ip][1] if ip in NEAR else FAR[ip][2];d[1]=0
                if d[0] in (1,2):d[2]=-1
                if d[0]==4:d[3:7]=[-1]*4;loops['printer']=0
            elif o in ('lead','trail'):d[2]=0
            elif o=='printer':
                d[3]=0 if ip==0x63C else 1;d[4]=0
                if ip==0x691:d[5:7]=[-1]*2;loops['glyph']=0
            elif o=='glyph':d[5:7]=[0,0]
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
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,diag=run_c(lib,m,actual,printer_limit,glyph_limit,refuse)
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
    result=[('default',{}),('empty',dict(text=b'')),('one-space',dict(text=b' ')),('all-spaces',dict(text=b' '*40)),
      ('single',dict(text=b'A')),('select1',dict(index=1)),('select2',dict(index=2)),('past-end',dict(index=3)),
      ('null0',dict(null=True)),('null1',dict(null=True,index=1)),('cycle32',dict(cycle=True,index=32,base=32)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('stack-min',dict(sp=0x300)),('stack-max',dict(sp=0xFFF4)),
      ('no-font',dict(fontseg=0,fontoff=0)),('node-offset-wrap',dict(node_off=0xFFF0)),
      ('printer-limit0',dict(printer_limit=0)),('printer-limit1',dict(printer_limit=1)),('glyph-limit0',dict(glyph_limit=0)),
      ('glyph-limit1',dict(glyph_limit=1)),('port0',dict(refuse=0)),('port4',dict(refuse=4))]
    for length in (0,1,2,3,39,40,41,42,127,255):
        for style in range(4):
            text=(b' '*length if style==0 else b'A'*length if style==1 else b' '*(length//2)+b'a'*(length-length//2) if style==2 else b'a'*(length//2)+b' '*(length-length//2))
            result.append((f'length{length}-style{style}',dict(text=text,mode=3 if length>40 else 1)))
    for mode in range(4):
        for x,y in ((0,0),(39,24),(40,25),(127,127),(128,128),(255,255)):
            result.append((f'mode{mode}-xy{x}-{y}',dict(mode=mode,x=x,y=y,flags=FLAGS[(x+y)%6])))
    for site in (*NEAR,*FAR):result.append((f'bad-root-{site:x}',dict(writes=((CS,site,b'\x90'),))))
    for seg,site in ((CS,0x8EB),(CS,0x937),(GLYPH,0x63C),(GLYPH,0x691),(GLYPH,0x207)):
        result.append((f'bad-nested-{seg:x}-{site:x}',dict(writes=((seg,site,b'\x90'),))))
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
            sys.modules.setdefault('u3b0_entry_0969_cpu_audit',sys.modules[__name__])
            from u3b0_entry_0969_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print({k:({kk:vv for kk,vv in v.items() if not kk.startswith('missing')} if k!='identity' else v) for k,v in report.items()})
if __name__=='__main__':main()
