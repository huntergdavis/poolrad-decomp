#!/usr/bin/env python3
"""Complete U3B0 +0C33 original CPU/RAM/OUT/VRAM and native EGA comparison.
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
import u3b0_entry_07a4_cpu_audit as display
import u3b0_step_scan_0b03_audit as step
from res0709_ega_reset_0000_cpu_audit import Cpu,Env,OUT_FN,IN_FN,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16,check
from u86_procvars_3413_audit import OVR_SHA
from res03f1_clear_0137_cpu_audit import Ega,Bus,WRITE_FN
CS,DS,SS,SYS,GLYPH,FONT,DATA=0x9000,0x1800,0x2800,0x4000,0x8800,0x7000,0x6000
ENTRY,END,RETF=0xC33,0xCC5,0xCC2
CLEAR=display.CLEAR
SEGMENTS=display.SEGMENTS
FLAGS=display.FLAGS
NEAR={0xCB1:(0xB03,0),0xCBC:(0x7A4,1)}
SOURCES=('overlay_u3b0_entry_0c33_cpu','overlay_u3b0_step_scan_0b03_cpu')+display.SOURCES
NAMES=('callee','status','step_status')+tuple('display_'+n for n in display.NAMES)
class Diagnostic(ctypes.Structure):_fields_=[(n,ctypes.c_int) for n in NAMES]
class Limits(ctypes.Structure):_fields_=[('step',ctypes.c_uint),('display',display.Limits)]
def source():
    bodies,_,_=display.source();target,code=step.source();md=Cs(CS_ARCH_X86,CS_MODE_16)
    bodies['root',0xB03]=target['body'];bodies['root',ENTRY]=code[ENTRY:END]
    check(sha256(bodies['root',ENTRY]).hexdigest()=='c7cb3ab8625088a43a7c7d35f9fa21dde2252bdfa9132e2fc5592070489fe86a','caller hash')
    ins=list(md.disasm(bodies['root',ENTRY],ENTRY));check(len(ins)==49 and sum(i.size for i in ins)==146,'caller extent')
    check(ins[-1].address==RETF and ins[-1].op_str=='4','caller return')
    check([(i.address,int(i.op_str,16)) for i in ins if i.mnemonic=='call']==[(s,v[0]) for s,v in NEAR.items()],'near sites')
    decoded={(k,i.address):i for (k,a),b in bodies.items() for i in md.disasm(b,a)}
    meta={f'{k}:{a:04X}':dict(bytes=len(b),instructions=len(list(md.disasm(b,a))),sha256=sha256(b).hexdigest()) for (k,a),b in bodies.items()}
    meta['target']=dict(file_offset=185602,entry_slots=[])
    print('Caller',meta['root:0C33'],meta['target'],flush=True)
    return bodies,decoded,meta
def library(directory):
    path=directory/'u3b0entry.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));lib.pr_u3b0_entry_0c33_cpu.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)]
    lib.pr_u3b0_entry_0c33_cpu.restype=ctypes.c_int;lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib
def install(m,bodies):display.install(m,bodies)
def build(bodies,*,direction=1,top=0,cursor=0,page_size=2,total=6,index_seg=DATA,index_off=0x8000,context=0x2000,
          node_count=6,bottom=4,width=3,writes=(),**options):
    m,c=display.build(bodies,context=context,node_count=node_count,bottom=bottom,width=width,**options)
    c.ip=ENTRY
    put16(m,c.ss,(context-0x59)&65535,page_size);put16(m,c.ss,(context-0x56)&65535,total)
    put16(m,c.ss,(context+10)&65535,index_off);put16(m,c.ss,(context+12)&65535,index_seg)
    # Parent fixture's head writes use nonwrapping offsets; install wrapped operands explicitly.
    put16(m,c.ss,(context+0x14)&65535,0 if options.get('null',False) else options.get('node_off',0x1000))
    put16(m,c.ss,(context+0x16)&65535,0 if options.get('null',False) else options.get('node_seg',DATA))
    put16(m,DS,0x6B29,top);put16(m,index_seg,index_off,cursor)
    put16(m,c.ss,c.sp+6,direction)
    for seg,off,blob in writes:m[physical(seg,off):physical(seg,off)+len(blob)]=blob
    return m,c
def run_c(lib,m,c,step_limit=65536,row_limit=65536,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    def write(ctx,off,value):writes.append((off,value))
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(write),None)
    d=Diagnostic();limits=Limits(step_limit,display.Limits(row_limit,clear_limit,rect_limit,printer_limit,glyph_limit))
    status=lib.pr_u3b0_entry_0c33_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),ctypes.byref(limits),ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d
def owner(key):
    k,ip=key
    if k=='root':
        if ip>=ENTRY:return 'root'
        if ip>=0xB03:return 'step'
        if ip>=0x7A4:return 'display'
    return display.owner(key)
def compare(name,state,bodies,decoded,lib,step_limit=65536,row_limit=65536,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0,SIZE,7,ctypes.addressof(backing));uc.mem_map_ptr(SIZE,SIZE,7,ctypes.addressof(backing))
    for (n,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(c,n))
    st=dict(stop=None,ret=False,root_return=False,previous=None,executing=None)
    d=[-1]*15;calls=[];seen=set();edges=set();events=[];writes=[];loops=dict(step=0,row=0,clear=0,rect=0,printer=0,glyph=0)
    kinds={v:k for k,v in SEGMENTS.items()}
    def stop(v):st['stop']=v;uc.emu_stop()
    def fail(o,v):
        if o=='root':return stop(v)
        if o in ('display','step'):d[1]=v;return stop(3)
        if o=='reset':d[8]=v;return fail('rect',3)
        if o=='rect':d[6]=v;return fail('clear',3)
        if o=='glyph' and st.get('glyph_parent')=='printer':d[10]=v;return fail('printer',3)
        d[4]=v;return fail('display',3)
    def hook(em,address,size,user):
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);key=(kinds.get(cs,'?'),ip)
        if st['root_return']:return stop(0)
        if st['ret']:
            expected,caller=calls.pop();st['ret']=False
            if (cs,ip)!=expected:
                st['transfer']=(expected,(cs,ip),caller)
                return fail(caller,3 if caller=='step' else 4 if caller in ('root','display') else 5)
        if st.pop('step_jump',False):
            if loops['step']==step_limit:return fail('step',4)
            loops['step']+=1
        check(key in decoded,f'escape {name} {cs:04X}:{ip:04X}')
        ins=decoded[key];o=owner(key);st['executing']=o
        if st['previous'] is not None:edges.add((st['previous'],key))
        st['previous']=key if ins.mnemonic.startswith('j') and ins.mnemonic!='jmp' else None
        if o=='step' and ip in (0xB99,0xC2A):st['step_jump']=True
        expected=None
        if o=='root' and ip in NEAR:
            target,_=NEAR[ip]
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return stop(2)
        if o=='display' and ip in display.NEAR:
            target,_=display.NEAR[ip]
            if bytes(uc.mem_read(address,3))!=struct.pack('<BH',0xE8,(target-ip-3)&65535):return fail('display',2)
        if o=='display' and ip in display.FAR:expected=display.FAR[ip][0]
        elif o=='clear' and ip in clear.FILL_SITES+clear.RECT_SITES:expected=0xB7 if ip in clear.RECT_SITES else 0x16A8
        elif o=='rect' and ip==0x140:expected=0x16A8
        elif o=='printer' and ip==0x63C:expected=0xB25
        elif o=='glyph' and ip==0x207:expected=0x182A
        if expected is not None and bytes(uc.mem_read(address,3))!=struct.pack('<BH',0x9A,expected):return fail(o,2)
        if o=='printer' and ip==0x691 and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return fail(o,2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return fail(o,2 if o=='reset' else 4)
        flag=uc.reg_read(R.UC_X86_REG_EFLAGS)
        limited=('row' if o=='display' and ip==0x8CE else
                 'clear' if o=='clear' and ip in (0x1D2,0x2E0) and not flag&0x40 else
                 'rect' if o=='rect' and ip==0x14F and not flag&0x40 else
                 'printer' if o=='printer' and ip==0x69F and not flag&0x40 else
                 'glyph' if o=='glyph' and ip in glyph.LOOPS and (ins.mnemonic=='jmp' or not flag&0x40) else None)
        if limited:
            cap=dict(row=row_limit,clear=clear_limit,rect=rect_limit,printer=printer_limit,glyph=glyph_limit)[limited]
            if loops[limited]==cap:return fail(o,5 if o=='display' else 6)
            loops[limited]+=1
        seen.add(key)
        if ins.mnemonic in ('call','lcall'):
            calls.append(((cs,ip+ins.size),o))
            if o=='root':
                d[0]=NEAR[ip][1];d[1]=0
                if d[0]==0:d[2]=-1;loops['step']=0
                else:d[3:15]=[-1]*12;loops['row']=0
            elif o=='step':d[2]=0
            elif o=='display':
                d[3]=display.NEAR[ip][1] if ip in display.NEAR else display.FAR[ip][2];d[4]=0
                if d[3]==0:d[5:9]=[-1]*4;loops['clear']=0
                if d[3]==3:d[9:13]=[-1]*4;loops['printer']=0
                if d[3]==4:d[13:15]=[-1]*2;loops['glyph']=0;st['glyph_parent']='root'
            elif o=='clear':
                d[5]=1 if ip==0x1F1 else 2 if ip==0x20C else 0;d[6]=0
                if ip in clear.RECT_SITES:d[7:9]=[-1]*2;loops['rect']=0
            elif o=='rect':d[7]=2 if ip==0x140 else 1 if ip==0x100 else 0;d[8]=0
            elif o=='printer':
                d[9]=0 if ip==0x63C else 1;d[10]=0
                if ip==0x691:d[11:13]=[-1]*2;loops['glyph']=0;st['glyph_parent']='printer'
            elif o=='glyph':
                at=11 if st['glyph_parent']=='printer' else 13;d[at:at+2]=[0,0]
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
                check(st['executing'] in ('glyph','rect','sys'),f'flat-RAM aperture write {name}/{st["executing"]}')
                writes.append((at-0xA0000,(value>>(8*j))&255))
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT);uc.hook_add(UC_HOOK_MEM_WRITE,write)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=4000000)
    check(st['stop'] is not None,f'instruction ceiling {name}')
    actual=Cpu.from_buffer_copy(c);status,ram,mine,stores,ega,diag=run_c(lib,m,actual,step_limit,row_limit,clear_limit,rect_limit,printer_limit,glyph_limit,refuse)
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
    result=[('default',{}),('backward',dict(direction=0,top=3,cursor=4)),
      ('back-clamp',dict(direction=0,top=1,cursor=2)),('forward-clamp',dict(top=5,cursor=5)),
      ('marked',dict(node_flag=1)),('mixed',dict(node_flags=(1,0,1,1,0,1))),
      ('marked-back',dict(direction=0,top=3,cursor=4,node_flag=1)),
      ('empty',dict(text=b'')),('no-pad',dict(width=0)),('null',dict(null=True)),
      ('step-limit0',dict(node_flag=1,step_limit=0)),('step-limit1',dict(node_flag=1,step_limit=1)),
      ('step-limit-back',dict(direction=0,top=3,cursor=4,node_flag=1,step_limit=0)),
      ('row-limit0',dict(row_limit=0)),('row-limit1',dict(row_limit=1)),
      ('clear-limit0',dict(mode=0,clear_limit=0)),('rect-limit0',dict(rect_limit=0)),
      ('printer-limit0',dict(printer_limit=0)),('glyph-limit0',dict(glyph_limit=0)),
      ('pad-glyph-limit0',dict(text=b'',glyph_limit=0)),
      ('ss-ds',dict(ss=DS)),('a20-stack',dict(ss=0xFFFF)),('stack-min',dict(sp=0x320)),('stack-max',dict(sp=0xFFF4)),
      ('no-font',dict(fontseg=0,fontoff=0)),('index-wrap',dict(index_off=0xFFFF)),('index-a20',dict(index_seg=0xFFFF,index_off=0x8000)),
      ('index-global',dict(index_seg=DS,index_off=0x6B29)),
      ('index-page-size',dict(index_seg=SS,index_off=0x2000-0x59)),
      ('index-local',dict(index_seg=SS,index_off=0x7FFC)),
      ('context-wrap',dict(context=0xFFF8)),('node-wrap',dict(node_off=0xFFF0))]
    for direction in (0,1,128,255,0xFF00,0xFF01):
        result.append((f'direction{direction}',dict(direction=direction,top=3,cursor=4)))
    for top,page,total,cursor in ((0,0,0,0),(0,1,1,0),(0,3,2,0),(0x7FFF,1,0x7FFF,0x7FFF),(0x8000,1,0x8000,0x8000),(0xFFFF,2,6,0),(1,0xFFFF,6,0),(0,0x8000,0x7FFF,0),(0xFFFF,0x7FFF,0xFFFF,0xFFFF)):
        for direction in (0,1):result.append((f'word-{top}-{page}-{total}-{direction}',dict(top=top,page_size=page,total=total,cursor=cursor,direction=direction,mode=3,step_limit=2)))
    for mode in range(4):
        for f in FLAGS:result.append((f'mode{mode}-flags{f:x}',dict(mode=mode,flags=f)))
    for active in (0,2,7):result.append((f'active{active}',dict(active=active)))
    for count in (0,1,4,20,43,44,48,78,112):result.append((f'port{count}',dict(refuse=count)))
    for site in NEAR:result.append((f'bad-root-{site:x}',dict(writes=((CS,site,b'\x90'),))))
    for site in (*display.NEAR,*display.FAR):
        for flag in (0,1):result.append((f'bad-display-{site:x}-{flag}',dict(node_flag=flag,writes=((CS,site,b'\x90'),))))
    for site in clear.FILL_SITES+clear.RECT_SITES:
        mode=0 if site in clear.FILL_SITES[:2] else 2 if site in clear.FILL_SITES else 1
        result.append((f'bad-clear-{site:x}',dict(mode=mode,writes=((CLEAR,site,b'\x90'),))))
    for site in (0x140,0x63C,0x691,0x207):result.append((f'bad-video-{site:x}',dict(writes=((GLYPH,site,b'\x90'),))))
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
            sys.modules.setdefault('u3b0_entry_0c33_cpu_audit',sys.modules[__name__])
            from u3b0_entry_0c33_cpu_native import native
            report['native']=native(bodies,lib)
    if a.output:a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print({k:({kk:vv for kk,vv in v.items() if not kk.startswith('missing')} if k!='identity' else v) for k,v in report.items()})
if __name__=='__main__':main()
