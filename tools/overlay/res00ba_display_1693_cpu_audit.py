#!/usr/bin/env python3
"""Raw +1693: CPU14, all 1MiB backing RAM, independent Python EGA bus.
No game bytes are embedded. Synthetic fixtures and generated guests stay private.
"""
import argparse, contextlib, ctypes, io, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
import res0709_printer_0626_cpu_audit as printer
import res03f1_clear_0137_cpu_audit as clear
from res0709_printer_0626_cpu_audit import Cpu, Ega, Bus, Env, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE, EXE_SHA, REGISTERS, physical, put16, check, FLAGS, FONT, STRING
from resident_loader_map import unpack
CHECKER_ROWS=('RES_00BA_1693',)
CS,DS,SS,SYS,CLEAR,DISPLAY,DELAY=0x8800,0x5000,0x2000,0x6000,0x8000,0x8200,0x8400
ENTRY,END=0x1693,0x16EC
SEGMENTS={'caller':CS,'sys':SYS,'clear':CLEAR,'display':DISPLAY,'delay':DELAY}
ROOT_CALLS={0x16A7:(0,0xB25,SYS,'store'),0x16B8:(1,0x137,CLEAR,'clear'),0x16CB:(2,0x626,CS,'printer'),0x16D0:(3,0xDBE,CS,'wrapper'),0x16E1:(4,0x137,CLEAR,'clear')}
SOURCES=tuple(dict.fromkeys(('resident_00ba_display_1693_cpu','resident_0709_delay_wrapper_0dbe_cpu','resident_a40_delay_029c_cpu')+printer.SOURCES+clear.SOURCES))
class Diagnostic(ctypes.Structure):
    _fields_=[(n,ctypes.c_int) for n in ('callee','status','delay_status','clear_callee','clear_status','rect_callee','rect_status','printer_callee','printer_status','glyph_callee','glyph_status')]
class PythonEga:
    """Independent byte bus, using the documented EGA planar truth tables.
    Full planes, latches and indexed registers are compared, not just stores.
    Display timing, address decoding and VGA-only write mode3 are out of scope.
    """
    def __init__(self):
        self.plane=[bytearray([0x31+p*0x27])*65536 for p in range(4)]
        self.latch=[0]*4;self.seq_index=0;self.seq=[0,0,15,0,0]
        self.gc_index=0;self.gc=[0,0,0,0,0,0,0,15,255]
        self.reads=[];self.writes=[]
    def out(self,port,value,size):
        for j in range(size):
            p=port+j;v=(value>>(8*j))&255
            if p==0x3C4:self.seq_index=v
            elif p==0x3CE:self.gc_index=v
            elif p==0x3C5 and self.seq_index<5:self.seq[self.seq_index]=v
            elif p==0x3CF and self.gc_index<9:self.gc[self.gc_index]=v
    def read(self,off):
        self.latch=[p[off] for p in self.plane]
        if self.gc[5]&8:
            v=255
            for p in range(4):
                if self.gc[7]&(1<<p):v &= self.latch[p] if self.gc[2]&(1<<p) else self.latch[p]^255
        else:v=self.latch[self.gc[4]&3]
        self.reads.append((off,v));return v
    def write(self,off,value):
        self.writes.append((off,value));mode=self.gc[5]&3
        check(mode!=3,'VGA write mode3 is outside EGA contract')
        for p in range(4):
            if not self.seq[2]&(1<<p):continue
            if mode==1:self.plane[p][off]=self.latch[p];continue
            if mode==2:source=255 if value&(1<<p) else 0
            elif self.gc[1]&(1<<p):source=255 if self.gc[0]&(1<<p) else 0
            else:
                bits=[(value>>(b+(self.gc[3]&7))%8)&1 for b in range(8)]
                source=sum(v<<b for b,v in enumerate(bits))
            fn=(self.gc[3]>>3)&3
            mixed=(source,source&self.latch[p],source|self.latch[p],source^self.latch[p])[fn]
            self.plane[p][off]=(mixed&self.gc[8])|(self.latch[p]&(255^self.gc[8]))
    def packed(self):
        return b''.join(self.plane)+bytes(self.latch+[self.seq_index]+self.seq+[self.gc_index]+self.gc)
def source():
    with contextlib.redirect_stdout(io.StringIO()):
        bodies,decoded,meta=printer.source();cb,cd,cm=clear.source()
    bodies.update(cb);decoded.update(cd);meta.update(cm)
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA,'START identity')
    with contextlib.redirect_stdout(io.StringIO()):image=bytes(unpack(exe)[0])
    from capstone import Cs,CS_ARCH_X86,CS_MODE_16
    md=Cs(CS_ARCH_X86,CS_MODE_16)
    for seg,a,b,kind,n,digest in (
        (0xBA,ENTRY,END,'display',44,'6db9222ad42386393622a6614d2ab37cc1f2eec0a7d65b2e943bbc2e9593581a'),
        (0x709,0xDBE,0xDD3,'caller',10,None),(0xA40,0x29C,0x2C9,'delay',19,None)):
        blob=image[physical(seg,a):physical(seg,b)];ins=list(md.disasm(blob,a))
        check(len(ins)==n and sum(i.size for i in ins)==len(blob),'decoded extent')
        if digest:check(sha256(blob).hexdigest()==digest,'root identity')
        bodies[kind,a]=blob;decoded.update({(kind,i.address):i for i in ins})
        meta[f'{seg:04X}:{a:04X}']=dict(bytes=len(blob),instructions=n,sha256=sha256(blob).hexdigest())
    check(decoded['display',0x16E9].mnemonic=='retf' and decoded['display',0x16E9].op_str=='4','root return')
    meta['expanded_image']=dict(bytes=len(image),sha256=sha256(image).hexdigest())
    from resident_loader_map import relocations
    from overlay_inventory import find_units,parse_mz
    rels,_=relocations(exe,image)
    original_segments={SYS:0xAF8,CLEAR:0x3F1,CS:0x709}
    outgoing=[]
    for site,(_,dest,segment,_) in ROOT_CALLS.items():
        at=physical(0xBA,site)
        check(image[at]==0x9A and struct.unpack_from('<HH',image,at+1)==(dest,original_segments[segment]) and at+3 in rels,'outgoing relocation')
        outgoing.append(dict(site=f'{site:04X}',target=f'{original_segments[segment]:04X}:{dest:04X}',expanded_relocation=f'{at+3:05X}'))
    ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest()=='872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614','GAME.OVR identity')
    incoming=[]
    for unit in find_units(exe[parse_mz(exe)['header_bytes']:],ovr):
        for fix in unit['fixups']:
            at=unit['ovr_offset']+fix-3;call=ovr[at:at+5]
            if len(call)==5 and call[0]==0x9A and struct.unpack_from('<HH',call,1)==(ENTRY,0xBA):
                incoming.append(dict(unit=f'{unit["stub_para"]:04X}',site=f'{fix-3:04X}',segment_fixup=f'{fix:04X}'))
    check(len(incoming)==42,'incoming ownership count')
    meta['ownership']=dict(incoming=incoming,outgoing=outgoing,game_ovr_sha256=sha256(ovr).hexdigest())
    return bodies,decoded,meta

def library(directory):
    path=directory/'display1693.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-fPIC','-shared','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/(s+'.c')) for s in SOURCES],'-o',str(path)],check=True)
    lib=ctypes.CDLL(str(path));f=lib.pr_r00ba_display_1693_cpu
    f.argtypes=[ctypes.POINTER(Bus),ctypes.c_size_t,ctypes.POINTER(Cpu),*[ctypes.c_size_t]*4,ctypes.c_uint64,ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    lib.pr_ega_reset.argtypes=[ctypes.POINTER(Ega)]
    return lib

def install(m,bodies):
    for (kind,start),blob in bodies.items():
        at=physical(SEGMENTS[kind],start);m[at:at+len(blob)]=blob
    for site,(_,dest,seg,_) in ROOT_CALLS.items():put16(m,DISPLAY,site+3,seg)
    for site in clear.FILL_SITES+clear.RECT_SITES:put16(m,CLEAR,site+3,CS if site in clear.RECT_SITES else SYS)
    for site,seg in ((0x20A,SYS),(0x63F,SYS),(0x143,SYS),(0xDCF,DELAY)):put16(m,CS,site,seg)

def build(bodies,*,text=b'AbZ',source_seg=STRING,source_off=0x800,active=1,factor=0,count=1,bios=0x55,writes=(),**options):
    # Reuse only fixture construction; the independent device oracle is below.
    subset={k:v for k,v in bodies.items() if k[0] in ('caller','sys')}
    m,c=printer.build(subset,text=text,source_seg=source_seg,source_off=source_off,**options)
    install(m,bodies);c.cs=DISPLAY;c.ip=ENTRY
    put16(m,c.ss,(c.sp+4)&65535,source_off);put16(m,c.ss,(c.sp+6)&65535,source_seg)
    # Remove unused printer argument words from the synthetic entry frame.
    for off in range(8,16):m[physical(c.ss,(c.sp+off)&65535)]=0xA5
    m[physical(c.ds,0x4A1A)]=active;m[physical(c.ds,0x49E2)]=factor;put16(m,c.ds,0x84FC,count);m[0x400]=bios
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    return m,c

def run_c(lib,m,c,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,delay_limit=1000000,refuse=None):
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);ega=Ega();lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):ctypes.memset(ctypes.addressof(ega.plane[p]),0x31+p*0x27,65536)
    events=[];writes=[]
    def out(ctx,port,value,size,cs,ip):
        if refuse is not None and len(events)==refuse:return 1
        events.append((port,value,size,cs,ip));return 0
    env=Env(None,OUT_FN(out),IN_FN());bus=Bus(ram,ctypes.pointer(ega),ctypes.pointer(env),WRITE_FN(lambda ctx,off,v:writes.append((off,v))),None);d=Diagnostic()
    status=lib.pr_r00ba_display_1693_cpu(ctypes.byref(bus),SIZE,ctypes.byref(c),clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,ctypes.byref(d))
    return status,bytes(ram),events,writes,bytes(ega),d

def stop_name(status,d,c):
    if status==0:return 'RETURN'
    root={1:'ARGUMENT',2:'CODE',3:'CALLEE',5:'TRANSFER'}.get(status,str(status))
    calls={-1:'none',0:'StrStore',1:'first-clear',2:'printer',3:'delay-wrapper',4:'second-clear'}
    chain=[root,calls[d.callee]]
    if status==3:
        raw={1:'ARGUMENT',2:'CODE',3:'CALLEE',4:'PORT',5:'TRANSFER',6:'LIMIT'}
        if d.callee==3:raw[4]='TRANSFER'
        chain.append(raw.get(d.status,str(d.status)))
        if d.callee in (1,4) and d.status==3:
            chain.append('rectangle:'+raw.get(d.clear_status,str(d.clear_status)))
            if d.clear_status==3:chain.append('nested:'+str(d.rect_status))
        if d.callee==2 and d.status==3:chain.append('glyph:'+raw.get(d.printer_status,str(d.printer_status)))
        if d.callee==3 and d.status==3:chain.append('CRT:'+{0:'RETURN',1:'ARGUMENT',2:'LIMIT'}.get(d.delay_status,str(d.delay_status)))
    return '/'.join(chain)+f'@{c.cs:04X}:{c.ip:04X}'

# Parent/child diagnostic routing is based on actual original CALL/RET execution.
# Each entry names the called C contract, not an instruction-emulation shortcut.
CALLS={}
for site,(index,dest,seg,name) in ROOT_CALLS.items():CALLS[DISPLAY,site]=(name,('callee','status'),index,dest)
for site in clear.FILL_SITES+clear.RECT_SITES:
    CALLS[CLEAR,site]=('rect' if site in clear.RECT_SITES else 'fill',('clear_callee','clear_status'),1 if site==0x1F1 else 2 if site==0x20C else 0,0xB7 if site in clear.RECT_SITES else 0x16A8)
CALLS.update({(CS,0x140):('fill',('rect_callee','rect_status'),2,0x16A8),
    (CS,0xFC):('page',('rect_callee','rect_status'),0,0xA0),
    (CS,0x168):('page',('rect_callee','rect_status'),0,0xA0),
    (CS,0x100):('reset',('rect_callee','rect_status'),1,0),
    (CS,0x63C):('store',('printer_callee','printer_status'),0,0xB25),
    (CS,0x691):('glyph',('printer_callee','printer_status'),1,0x1DF),
    (CS,0x207):('upcase',('glyph_callee','glyph_status'),0,0x182A),
    (CS,0xDCC):('delay',(None,'delay_status'),None,0x29C)})

def compare(name,state,bodies,decoded,lib,clear_limit=65536,rect_limit=65536,printer_limit=65536,glyph_limit=65536,delay_limit=1000000,refuse=None):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_INSN
    from unicorn import x86_const as R
    m,c=state;uc=Uc(UC_ARCH_X86,UC_MODE_16);backing=(ctypes.c_uint8*SIZE).from_buffer_copy(m);device=PythonEga()
    def read(em,off,size,user):return sum(device.read((off+j)&65535)<<(8*j) for j in range(size))
    def write(em,off,size,value,user):
        for j in range(size):device.write((off+j)&65535,(value>>(8*j))&255)
    for base in (0,SIZE):
        uc.mem_map_ptr(base,0xA0000,7,ctypes.addressof(backing));uc.mmio_map(base+0xA0000,65536,read,None,write,None)
        uc.mem_map_ptr(base+0xB0000,0x50000,7,ctypes.addressof(backing)+0xB0000)
    for (n,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,n))
    diag={n:-1 for n,_ in Diagnostic._fields_};frames=[];active='root';returning=False;stop=None;last=None;seen=set();edges=set();events=[]
    budgets={'clear':clear_limit,'rect':rect_limit,'printer':printer_limit,'glyph':glyph_limit,'delay':delay_limit};used={k:0 for k in budgets}
    def halt(status):
        nonlocal stop
        # Unwind only model boundaries; a near helper is inside its contract.
        for frame in reversed(frames):
            if frame['spec'] is not None:
                diag[frame['spec'][1][1]]=status;status=3
        stop=status;uc.emu_stop()
    def hook(em,address,size,user):
        nonlocal active,returning,stop,last
        cs=uc.reg_read(R.UC_X86_REG_CS);ip=uc.reg_read(R.UC_X86_REG_IP);here=(cs,ip)
        if returning:
            returning=False
            if not frames:stop=0;return uc.emu_stop()
            f=frames.pop();active=f['parent']
            if here!=f['return']:
                if f['spec'] is not None:
                    diag[f['spec'][1][1]]=0
                    return halt(4 if active=='wrapper' else 5)
                raise ValueError(f'unexpected internal delay RET: {name} {here}')
        key=(next((k for k,v in SEGMENTS.items() if v==cs),'?'),ip)
        check(key in decoded,f'escape {name}: {here} active={active}')
        ins=decoded[key]
        # Delay budget stops after a taken transfer, matching its raw API.
        if active=='delay' and last is not None and last[0]==DELAY and {0x2C1:0x2C0,0x2C6:0x2BD,0x2B8:0x2B0}.get(last[1])==ip:
            if used['delay']==delay_limit:return halt(2)
            used['delay']+=1
        spec=CALLS.get(here)
        if spec and ins.mnemonic=='lcall':
            if bytes(uc.mem_read(address,3))!=b'\x9a'+struct.pack('<H',spec[3]):return halt(2)
        if here==(CS,0x691) and bytes(uc.mem_read(address,3))!=bytes.fromhex('e84bfb'):return halt(2)
        if ins.mnemonic=='out' and refuse is not None and len(events)==refuse:return halt(2 if active=='reset' else 4)
        take=not bool(uc.reg_read(R.UC_X86_REG_EFLAGS)&0x40)
        loop=(active=='clear' and ip in (0x1D2,0x2E0) and take) or (active=='rect' and ip==0x14F and take) or (active=='printer' and ip==0x69F and take) or (active=='glyph' and ip in printer.LOOPS and (ins.mnemonic=='jmp' or take))
        if loop:
            if used[active]==budgets[active]:return halt(6)
            used[active]+=1
        if last is not None:
            lk=(next((k for k,v in SEGMENTS.items() if v==last[0]),'?'),last[1]);prev=decoded[lk]
            if (prev.mnemonic.startswith('j') and prev.mnemonic!='jmp') or prev.mnemonic.startswith('loop'):edges.add((last,here))
        seen.add(here);last=here
        if ins.mnemonic in ('call','lcall'):
            frames.append(dict(parent=active,return_ignored=None,**{'return':(cs,ip+ins.size),'spec':spec}))
            if spec:
                child,(field,status),index,dest=spec
                if field:diag[field]=index
                diag[status]=0;active=child
                if child in used:used[child]=0
                reset={'clear':('clear_callee','clear_status','rect_callee','rect_status'),'rect':('rect_callee','rect_status'),'printer':('printer_callee','printer_status','glyph_callee','glyph_status'),'glyph':('glyph_callee','glyph_status'),'wrapper':('delay_status',)}.get(child,())
                for n in reset:diag[n]=-1
            else:check(active=='delay',f'unclassified CALL {name} {here} {ins.op_str}')
        if ins.mnemonic in ('ret','retf'):returning=True
    def out(em,port,size,value,user):
        events.append((port,value,size,uc.reg_read(R.UC_X86_REG_CS),uc.reg_read(R.UC_X86_REG_IP)));device.out(port,value,size)
    uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_INSN,out,None,1,0,R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs,c.ip),2*SIZE,count=10000000)
    check(stop is not None,f'instruction ceiling {name}')
    for (kind,start),blob in bodies.items():
        at=physical(SEGMENTS[kind],start);check(bytes(backing[at:at+len(blob)])==m[at:at+len(blob)],f'code mutation {name}')
    actual=Cpu.from_buffer_copy(c)
    status,ram,mine,stores,ega,d=run_c(lib,m,actual,clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,refuse)
    check(status==stop,f'status {name}: {status}/{stop}, {diag}')
    check([getattr(d,n) for n in diag]==list(diag.values()),f'diagnostic {name}: {[getattr(d,n) for n in diag]}/{diag}')
    for (n,_),r in zip(Cpu._fields_,REGISTERS):
        want=uc.reg_read(r)&65535;check(getattr(actual,n)==want,f'CPU {name}/{n}: {getattr(actual,n):04X}/{want:04X}')
    if ram!=bytes(backing):
        differences=[(hex(i),a,b) for i,(a,b) in enumerate(zip(ram,backing)) if a!=b];raise ValueError(f'RAM {name}: {differences[:8]} ({len(differences)})')
    check(events==mine,f'OUTs {name}');check(stores==device.writes,f'VRAM operands {name}')
    if ega!=device.packed():
        differences=[(hex(i),a,b) for i,(a,b) in enumerate(zip(ega,device.packed())) if a!=b];raise ValueError(f'EGA {name}: {differences[:8]} ({len(differences)})')
    label=stop_name(status,d,actual)
    return label,seen,edges,len(events),len(stores),len(device.reads)

def cases(quick=False):
    specs=[(f'mode{m}-len{n}',dict(mode=m,text=bytes((65+j*13)&255 for j in range(n)))) for m in (0,1,2,3,255) for n in (0,1,3,40,41,255)]
    specs += [('delay-zero',dict(factor=0)),('delay-one',dict(factor=1)),('delay-two',dict(factor=2,count=2)),('delay-limit',dict(factor=255,delay_limit=17)),('printer-limit',dict(printer_limit=0)),('rect-limit',dict(rect_limit=0))]
    if quick:return specs
    for flags in FLAGS:specs.append((f'flags{flags:x}',dict(flags=flags)))
    for active in range(8):specs.append((f'page{active}',dict(active=active)))
    for sp in (0x16A,0x16B,0x8001,0xFFF7,0xFFF8):specs.append((f'sp{sp:x}',dict(sp=sp)))
    specs += [('a20',dict(ss=0xFFFF)),('ds-ss',dict(ss=DS)),('source-wrap',dict(source_off=0xFFFE)),('font-wrap',dict(fontoff=0xFFFC)),('font-ega',dict(fontseg=0xA000)),('font-output-alias',dict(fontseg=0xA000,fontoff=0x1600)),('no-font',dict(writes=((DS,0x5E20,bytes(4)),))),('same-pages',dict(pages=(0xA000,0xA000))),('ordinary-pages',dict(pages=(0xB000,0xC000)))]
    for site in ROOT_CALLS:specs.append((f'root-code{site:x}',dict(writes=((DISPLAY,site,b'\x90'),))))
    for seg,site in ((CLEAR,0x1F1),(CLEAR,0x20C),(CS,0x140),(CS,0x63C),(CS,0x691),(CS,0x207),(CS,0xDCC)):
        specs.append((f'nested-code{seg:x}-{site:x}',dict(writes=((seg,site,b'\x90'),))))
    for n in (0,1,2,17,21,22,23,43,44,45,65,66,67,88,100,120,150):specs.append((f'port{n}',dict(refuse=n)))
    specs += [('clear-limit',dict(mode=0,clear_limit=0)),('glyph-limit',dict(glyph_limit=0)),
      ('delay-inner-limit',dict(factor=1,count=1,delay_limit=0)),
      ('delay-pass-limit',dict(factor=1,count=2,delay_limit=3)),
      ('delay-ms-limit',dict(factor=1,count=1,delay_limit=3)),
      ('delay-count-zero',dict(factor=1,count=0,delay_limit=16)),
      ('bios-stack-alias',dict(mode=3,text=b'',ss=0,sp=0x43A,factor=1,count=2)),
      ('source-forward-overlap',dict(source_seg=SS,source_off=0x7FD4,text=b'AbCd')),
      ('source-back-overlap',dict(source_seg=SS,source_off=0x7FD6,text=b'AbCd'))]
    for sp in (0x1E00,0x1E40,0x1E60,0x1E80,0x1EC0,0x1F00):
        specs.append((f'screen-stack-{sp:x}',dict(mode=0,text=b'',ss=0xB800,sp=sp,clear_limit=4,rect_limit=4,printer_limit=4,glyph_limit=4,delay_limit=4)))
    rng=random.Random(0x1693)
    for n in range(64):specs.append((f'random{n}',dict(mode=rng.choice((0,1,2,3)),text=rng.randbytes(rng.randrange(256)),active=rng.randrange(8),flags=rng.choice(FLAGS),sp=rng.randrange(0x200,0xF000),factor=n%2)))
    return specs

def differential(bodies,decoded,lib,quick=False):
    seen=set();edges=set();stops={};outs=writes=reads=0
    for name,options in cases(quick):
        options=dict(options);budgets={k:options.pop(k) for k in tuple(options) if k.endswith('_limit') or k=='refuse'}
        label,v,e,o,w,r=compare(name,build(bodies,**options),bodies,decoded,lib,**budgets)
        seen|=v;edges|=e;outs+=o;writes+=w;reads+=r;stops[label]=stops.get(label,0)+1
        print(name,label,flush=True)
    starts={(SEGMENTS[k],a) for k,a in decoded};root={(DISPLAY,a) for k,a in decoded if k=='display'}
    expected={( (SEGMENTS[k],a),(SEGMENTS[k],target)) for (k,a),i in decoded.items() if (i.mnemonic.startswith('j') and i.mnemonic!='jmp') or i.mnemonic.startswith('loop') for target in (a+i.size,int(i.op_str,16))}
    check(root<=seen,'root starts missing')
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,ram_excluded_ranges=[],flag_masks=0,ega_bytes=len(PythonEga().packed()),root_starts=len(root&seen),root_total=len(root),closure_starts=len(starts&seen),closure_total=len(starts),outcomes=len(expected&edges),outcomes_total=len(expected),missing_starts=[f'{s:04X}:{a:04X}' for s,a in sorted(starts-seen)],missing_outcomes=[[f'{s:04X}:{a:04X}' for s,a in pair] for pair in sorted(expected-edges)],outs=outs,ega_writes=writes,ega_reads=reads)
    print(json.dumps(report,sort_keys=True));return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--quick',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='display1693-') as temp:
        lib=library(Path(temp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        bodies,decoded,meta=source();report=dict(target=meta)
        if a.unicorn:report['unicorn']=differential(bodies,decoded,lib,a.quick)
        if a.native:
            from res00ba_display_1693_cpu_native import native
            report['native']=native(bodies,lib)
        if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':
    sys.modules['res00ba_display_1693_cpu_audit']=sys.modules[__name__];main()
