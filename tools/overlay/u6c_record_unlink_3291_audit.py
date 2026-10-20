#!/usr/bin/env python3
"""U6C +3291 original CPU14/all1MiB differential, composed with the raw +0000
and U3CE +014E closures (resident 2595, SYSTEM FreeMem). Media and captures
stay in ignored resources/. Harness idioms follow u6c_record_release_0000_audit
and u3ce_bitmap_pair_release_014e_audit; no original code bytes are embedded.
"""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
import u6c_record_release_0000_audit as ub
import u3ce_bitmap_pair_release_014e_audit as ua
fm=ub.fm;r=ua.r;check=fm.check
CHECKER_ROWS=('OVL_006C_3291',)
CS,U3CE,STUB,RESIDENT,DS,SS,SYSTEM=0x5800,ua.CS,0x6000,ua.RESIDENT,r.DS,r.SS,r.SYSTEM
ENTRY,END,RETF=0x3291,0x33D7,0x33D4
FAR_SITES=(0x3321,0x337B)
NEAR_SITES=(0x334A,0x33B2)
OK,ARGUMENT,CODE,CALLEE,TRANSFER,LIMIT=0,1,2,3,4,5
NAMES={OK:'return',CODE:'CODE',CALLEE:'CALLEE',TRANSFER:'TRANSFER',LIMIT:'LIMIT'}
PAIR,REC=0,1
SOURCES=['overlay_u6c_record_unlink_3291_cpu.c','overlay_u6c_record_release_0000_cpu.c',*ua.SOURCES]
FLAGS=fm.FLAGS
class Limits(ctypes.Structure):
    _fields_=[('walk',ctypes.c_uint),('list',ctypes.c_uint),('heap',ctypes.c_uint)]
class Diagnostic(ctypes.Structure):
    _fields_=[('callee',ctypes.c_int),('callee_status',ctypes.c_int),('pair',ua.Diagnostic),('record',ub.Diagnostic),
              ('pair_calls',ctypes.c_uint),('record_calls',ctypes.c_uint),('call_site',ctypes.c_uint16)]
def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ovr=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest()==EXE_SHA and sha256(ovr).hexdigest()==OVR_SHA,'media provenance')
    units=find_units(exe[parse_mz(exe)['header_bytes']:],ovr);u=next(u for u in units if u['stub_para']==0x6C)
    at=u['ovr_offset']+ENTRY;body=ovr[at:at+END-ENTRY];tail=ovr[at+END-ENTRY:at+342]
    dis=Cs(CS_ARCH_X86,CS_MODE_16);ins=list(dis.disasm(body,ENTRY))
    check(len(body)==326 and len(ins)==95 and sum(i.size for i in ins)==326,'3291 extent')
    check(sha256(body).hexdigest()==BODY_SHA,'3291 body hash')
    check((ins[-1].address,ins[-1].mnemonic,ins[-1].op_str)==(RETF,'retf','4'),'RETF4')
    check(u['entries'][2]==ENTRY and u['memory_segment']==0x78,'public entry 2 / stub 0078:002A')
    check(all(ENTRY<=t<END for i in ins if i.mnemonic.startswith('j') for t in [int(i.op_str,16)]),'all branch targets inside the extent')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='lcall']==[(a,'0x3e9, 0x2f') for a in FAR_SITES],'two stub calls')
    check(all(a+3 in u['fixups'] for a in FAR_SITES),'stub segment relocations')
    check([(i.address,i.op_str) for i in ins if i.mnemonic=='call']==[(a,'0') for a in NEAR_SITES],'two near calls to +0000')
    code=ovr[u['ovr_offset']:u['ovr_offset']+u['code_size']]
    check(all(code[a-1]==0x0E and a+3 not in u['fixups'] for a in NEAR_SITES),'push cs / near call, no relocation')
    incoming=[]
    for unit in units:
        for fix in unit['fixups']:
            b=ovr[unit['ovr_offset']+fix-3:unit['ovr_offset']+fix+2]
            if len(b)==5 and b[0]==0x9A and struct.unpack_from('<HH',b,1)==(0x2A,0x78):incoming.append((unit['stub_para'],fix-3))
    check(sorted(incoming)==INCOMING,f'incoming fixup-backed callers {incoming}')
    body0,image,d0,c0,leaf,leaf_edges,meta0=ub.source()
    body2,_,dd2,cc2,meta2=None,None,None,None,None
    pair_body,resident,image2,dd,cc,meta=ua.source()
    check(image==image2,'one loader-expanded image')
    decoded={i.address:i for i in ins}
    tables={'caller':decoded,'rec':d0,'pair':dd['caller'],'resident':dd['resident'],'freemem':dd['freemem']}
    cond={w:{a:(a+i.size,int(i.op_str,16)) for a,i in t.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'} for w,t in tables.items()}
    info=dict(target='OVL_006C_3291',span=[ENTRY,END],file_start=at,bytes=len(body),instructions=len(ins),sha256=sha256(body).hexdigest(),
        former_queue_bytes=342,trimmed_tail=len(tail),tail_sha256=sha256(tail).hexdigest(),incoming=incoming,
        record_release=meta0,pair_release=meta)
    return dict(body=body,body0=body0,pair_body=pair_body,resident=resident,image=image,tables=tables,cond=cond,info=info)
BODY_SHA='1e2161950d386c5df7394667e5d6079e692e551fc7610f1582ef06157528e74f'
INCOMING=[(0x1C,0x394),(0x20,0x6EA),(0x20,0x748),(0x20,0x1239),(0x33,0x1DCF),(0x6A,0x1935),(0x6A,0x19E5)]
# Stop each named fixture must reach (prefix of the measured name): proves the path was reached, not inferred from the label.
EXPECT={'bad-lcall-opcode-walk':'CODE_3321_stub','bad-lcall-offset-walk':'CODE_3321_stub','bad-lcall-opcode-head':'CODE_337B_stub','bad-lcall-offset-head':'CODE_337B_stub',
  'bad-near-walk':'CODE_334A_near','bad-pushcs-walk':'CODE_334A_near','bad-near-disp-walk':'CODE_334A_near','bad-near-head':'CODE_33B2_near','bad-pushcs-head':'CODE_33B2_near',
  'bad-0000-site':'CALLEE_record_2','stub-cold-int3f':'CODE_3321_stub','stub-wrong-offset':'CODE_3321_stub','stub-head-cold':'CODE_337B_stub',
  'walk-limit-0':'LIMIT','walk-cycle':'LIMIT','list-limit':'CALLEE_record_5','heap-limit':'CALLEE_record_3','heap-limit-pair':'CALLEE_pair_2',
  'invalid-record':'CALLEE_record_3','invalid-record-head':'CALLEE_record_3','invalid-bitmap-a':'CALLEE_pair_2','invalid-bitmap-b':'CALLEE_pair_2'}
def library(directory):
    p=directory/'unlink.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),*[str(ROOT/'src/recovered'/s) for s in SOURCES],'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u6c_record_unlink_3291_cpu
    f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint16,ctypes.c_uint16,ctypes.POINTER(Limits),ctypes.POINTER(Diagnostic)];f.restype=ctypes.c_int
    return f
def pair(m,s,o,p):
    put16(m,s,o&65535,p[1]);put16(m,s,(o+2)&65535,p[0])
REC0,REC_STEP=0x3100,0x20
def rec_ptr(i,off=0):return (REC0+REC_STEP*i,off)
def build(ctx,rng,*,records=3,head=0,active=0,order=None,slots=None,b6=1,b8=1,b6hi=0,b8hi=0,bitmaps=None,mode=0,side=None,list63=(),list9=(),
          blocks=(),top=(0x3800,0),org=(0x3000,0),sp=0x8000,ss=SS,flags=0x602,writes=(),background=None,system=SYSTEM,systems=None,
          window=(0x3A00,0),window_value=0x1234,ds=DS,stub=None,stubseg=STUB,active_ptr=None,head_ptr=None,nodes_off=None,hdrs=None,rec_at=None,rec_prev=None,rec_slot=0,nil_tables=True,**_):
    """records: list nodes 0..n-1 chained in `order`; head/active index into the chain (None = nil)."""
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background);m[:len(ctx['image'])]=ctx['image']
    fm.gm.heap(m,blocks=blocks,top=top,end=(0x4000,0),first=None);pair(m,DS,0x4400,org)
    chain=list(order) if order is not None else list(range(records))
    ptr=lambda i:rec_ptr(i,(nodes_off or {}).get(i,0))
    for k,i in enumerate(chain):
        p=ptr(i);nxt=ptr(chain[k+1]) if k+1<len(chain) else (0,0)
        pair(m,p[0],p[1]+0x104,nxt);m[physical(p[0],(p[1]+0xBF)&65535)]=(slots or {}).get(i,i&255)
        for off in (0x108,0xC8,0x7F):pair(m,p[0],p[1]+off,(0,0))
    act=ptr(chain[active]) if active_ptr is None and active is not None else (active_ptr or (0,0))
    hd=ptr(chain[head]) if head_ptr is None and head is not None else (head_ptr or (0,0))
    pair(m,DS,0x5D92,act);pair(m,DS,0x5D96,hd);pair(m,DS,0x49D6,window);put16(m,window[0],(window[1]+0x67C)&65535,window_value)
    if active is not None and active_ptr is None:
        ap=ptr(chain[active])
        if side is not None:pair(m,ap[0],ap[1]+0x108,side)
        for nodes,field,link in ((list63,0xC8,0x2A),(list9,0x7F,5)):
            if nodes:pair(m,ap[0],ap[1]+field,nodes[0])
            for p,nx in zip(nodes,list(nodes[1:])+[(0,0)]):pair(m,p[0],p[1]+link,nx)
    if rec_at is not None:
        ap=rec_at;pair(m,DS,0x5D92,ap);pair(m,DS,0x5D96,rec_prev or ap)
        pair(m,ap[0],ap[1]+0x104,(0,0));m[physical(ap[0],(ap[1]+0xBF)&65535)]=rec_slot
        for off in (0x108,0xC8,0x7F):pair(m,ap[0],ap[1]+off,(0,0))
        if side is not None:pair(m,ap[0],ap[1]+0x108,side)
        for nodes,field,link in ((list63,0xC8,0x2A),(list9,0x7F,5)):
            if nodes:pair(m,ap[0],ap[1]+field,nodes[0])
            for q,nx in zip(nodes,list(nodes[1:])+[(0,0)]):pair(m,q[0],q[1]+link,nx)
        if rec_prev:
            pair(m,rec_prev[0],rec_prev[1]+0x104,ap);m[physical(rec_prev[0],(rec_prev[1]+0xBF)&65535)]=rec_slot
    if nil_tables:
        for i in range(256):pair(m,DS,0x5E5A+8*i,(0,0));pair(m,DS,0x5E5E+8*i,(0,0))
    for idx,(a,b) in (bitmaps or {}).items():
        for which,rec in ((0,a),(1,b)):
            pair(m,DS,0x5E5A+8*idx+4*which,rec)
            if rec!=(0,0):
                pl,sd,ax=(hdrs or {}).get(rec,(1,16,(0,0)))
                m[physical(rec[0],(rec[1]+8)&65535)]=pl;put16(m,rec[0],(rec[1]+0x11)&65535,sd);pair(m,rec[0],(rec[1]+0x13)&65535,ax)
    m[physical(DS,0x4A1E)]=mode
    if ds!=DS:
        at=physical(ds,0);m[at:at+65536]=m[physical(DS,0):physical(DS,0)+65536]
        for a,b in fm.SPANS:
            at=physical(SYSTEM,a);m[at:at+b-a]=ctx['image'][physical(SYSTEM,a):physical(SYSTEM,b)]
    for j,v in enumerate((0x7000,0x9001,(b6hi<<8)|b6,(b8hi<<8)|b8)):put16(m,ss,(sp+2*j)&65535,v)
    for body,seg,off in ((ctx['body'],CS,ENTRY),(ctx['body0'],CS,0),(ctx['pair_body'],U3CE,ua.ENTRY),(ctx['resident'],RESIDENT,r.ENTRY)):
        at=physical(seg,off);m[at:at+len(body)]=body
    sysseg=system
    for site in r.SITES:put16(m,RESIDENT,site+3,sysseg)
    for site in ua.SITES:put16(m,U3CE,site+3,RESIDENT)
    for site in ub.SITES:put16(m,CS,site+3,(systems or {}).get(site,SYSTEM))
    for segs in {sysseg,*(systems or {}).values()}-{SYSTEM}:
        for a,b in fm.SPANS:
            at=physical(segs,a);m[at:at+b-a]=ctx['image'][physical(SYSTEM,a):physical(SYSTEM,b)]
    for site in FAR_SITES:put16(m,CS,site+3,stubseg)
    st=stub if stub is not None else bytes([0xEA])+struct.pack('<HH',ua.ENTRY,U3CE)
    at=physical(stubseg,0x2F);m[at:at+len(st)]=st
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[(physical(seg,off)+j)&(SIZE-1)]=v
    c=Cpu(*[rng.randrange(65536) for _ in range(7)],sp,CS,ENTRY,ds,rng.randrange(65536),ss,flags)
    return m,c

def fresh_pair():return dict(rs=-1,inner=-1,site=0,ic=0,calls=0,sroot=0)
def compare(label,ctx,m,c,f,lim):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    T,C=ctx['tables'],ctx['cond'];walk_cap,list_cap,heap_cap=lim
    rw=lambda seg,off:struct.unpack_from('<H',m,physical(seg,off))[0]
    stub_segs={rw(CS,s+3) for s in FAR_SITES}
    system_res=rw(RESIDENT,r.SITES[0]+3)
    systems={rw(CS,s+3) for s in ub.SITES}|{system_res}
    u=Uc(UC_ARCH_X86,UC_MODE_16);u.mem_map(0,2*SIZE);u.mem_write(0,bytes(m));u.mem_write(SIZE,bytes(m))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):u.reg_write(reg,getattr(c,name))
    st=dict(stop=None,callee=-1,cstatus=-1,call_site=0,pair_calls=0,rec_calls=0,active=None,pair=fresh_pair(),rec=dict(inner=-1,site=0),
        stack=[],returning=None,root_return=False,previous=None,last=None,walk=0,lists=0,heap=0,unexpected=None)
    visited,edges=set(),set()
    def stop(status):st['stop']=status;u.emu_stop()
    def fail_callee(callee,cstatus):
        st['callee']=callee;st['cstatus']=cstatus;stop(CALLEE)
    def nested(inner,rs=None):
        """The active callee stops with FreeMem state `inner` (and resident status rs for +014E)."""
        if st['active']==REC:st['rec']['inner']=inner;fail_callee(REC,ub.CALLEE)
        else:st['pair']['rs']=2 if rs is None else rs;st['pair']['inner']=inner;fail_callee(PAIR,2)
    def returned(owner,far,ok):
        if owner in('caller_stub','caller_rec'):
            st['callee']=PAIR if owner=='caller_stub' else REC;st['cstatus']=0
            return None if ok else stop(TRANSFER)
        if owner=='rec':
            st['rec']['inner']=0
            if not ok:st['rec']['inner']=0;fail_callee(REC,ub.TRANSFER)
        elif owner=='pair':
            st['pair']['rs']=0
            if not ok:fail_callee(PAIR,3)
        elif owner=='resident':
            st['pair']['inner']=0
            if not ok:st['pair']['rs']=3;fail_callee(PAIR,2)
        elif not ok:nested(fm.TRANSFER)
    def hook(emu,address,size,user):
        cs,ip=emu.reg_read(UC_X86_REG_CS),emu.reg_read(UC_X86_REG_IP)
        if st['root_return']:return stop(OK)
        if st['returning']:
            exp,owner,far=st['returning'];st['returning']=None
            returned(owner,far,(cs,ip)==exp)
            if st['stop'] is not None:return
        if cs in systems and ip==fm.RUNERROR_IP:return nested(fm.RUNERROR)
        if cs in stub_segs and ip==0x2F:
            b=bytes(emu.mem_read(physical(cs,ip),3))
            if b[0]!=0xEA or struct.unpack_from('<H',b,1)[0]!=ua.ENTRY:return stop(CODE)
            return
        who=('caller' if ENTRY<=ip<END else 'rec' if ip<ub.END else None) if cs==CS else 'pair' if cs==U3CE else 'resident' if cs==RESIDENT else 'freemem' if cs in systems else None
        if who is None or ip not in T[who]:
            st['unexpected']=(cs,ip);return emu.emu_stop()
        here=(who,ip);prev=st['previous']
        if prev and prev[1] in C[prev[0]]:edges.add((*prev,ip))
        if who=='caller':
            if ip in FAR_SITES:
                st['call_site']=ip
                if bytes(emu.mem_read(physical(cs,ip),3))!=b'\x9a\x2f\x00':return stop(CODE)
            if ip+1 in NEAR_SITES:
                st['call_site']=ip+1
                b=bytes(emu.mem_read(physical(cs,ip),4))
                if b[:2]!=b'\x0e\xe8' or struct.unpack_from('<H',b,2)[0]!=(-(ip+4))&65535:return stop(CODE)
            if ip==ENTRY+0x34-ENTRY+0x3291-0x3291 and False:pass
            if ip==0x3291+0x34 and st['last']==('caller',0x3291+0x6F):
                if st['walk']==walk_cap:return stop(LIMIT)
                st['walk']+=1
        elif who=='rec':
            if ip==0:st['active']=REC;st['callee']=REC;st['rec_calls']+=1;st['rec']=dict(inner=-1,site=0);st['lists']=0
            if ip in ub.SITES:
                st['rec']['site']=ip
                if bytes(emu.mem_read(physical(cs,ip),3))!=b'\x9a\x54\x02':return fail_callee(REC,ub.CODE)
                st['heap']=0
            if (ip==0x47 and st['last']==('rec',0x7B)) or (ip==0x8F and st['last']==('rec',0xC3)):
                if st['lists']==list_cap:return fail_callee(REC,ub.LIMIT)
                st['lists']+=1
        elif who=='pair':
            if ip==ua.ENTRY:st['active']=PAIR;st['callee']=PAIR;st['pair_calls']+=1;st['pair']=fresh_pair()
            if ip in ua.SITES:p=st['pair'];p['sroot']=ip;p['calls']+=1;p['inner']=-1;p['site']=0;p['ic']=0
        elif who=='resident':
            if ip in r.SITES:p=st['pair'];p['site']=ip;p['ic']+=1;st['heap']=0
        elif who=='freemem':
            if ip==fm.WALK_HEAD and st['last'] in {('freemem',a) for a in fm.WALK_JAS}:
                if st['heap']==heap_cap:return nested(fm.LIMIT)
                st['heap']+=1
        visited.add(here);ins=T[who][ip]
        if ins.mnemonic in('call','lcall'):
            owner=('caller_stub' if ins.mnemonic=='lcall' else 'caller_rec') if who=='caller' else who
            st['stack'].append(((cs,ip+ins.size),owner,ins.mnemonic=='lcall'))
        elif ins.mnemonic in('ret','retf'):
            if who=='caller' and ip==RETF:st['root_return']=True
            else:
                exp,owner,far=st['stack'].pop();st['returning']=(exp,owner,far)
        st['previous']=st['last']=here
    protected=[(physical(CS,ENTRY),physical(CS,END)),(physical(CS,0),physical(CS,ub.END)),(physical(U3CE,ua.ENTRY),physical(U3CE,ua.END)),(physical(RESIDENT,r.ENTRY),physical(RESIDENT,r.END))]
    protected+=[(physical(s,a),physical(s,b)) for s in systems for a,b in fm.SPANS]
    def mirror(emu,access,address,size,value,user):
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);check(not any(a<=at<b for a,b in protected),f'immutable code written: {label} {at:x}')
            emu.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    u.hook_add(UC_HOOK_CODE,hook);u.hook_add(UC_HOOK_MEM_WRITE,mirror)
    u.emu_start(physical(CS,ENTRY),2*SIZE,count=4000000)
    check(st['unexpected'] is None and st['stop'] is not None,f'original escaped/stalled {label}: {st["unexpected"]} {st["stop"]}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);d=Diagnostic();limits=Limits(walk_cap,list_cap,heap_cap)
    status=f(ram,SIZE,ctypes.byref(c),RESIDENT,system_res,ctypes.byref(limits),ctypes.byref(d))
    check(status==st['stop'],f'status {label}: {status} != {st["stop"]}')
    p=st['pair']
    actual=(d.callee,d.callee_status,d.call_site,d.pair_calls,d.record_calls,d.pair.resident_status,d.pair.resident.freemem_status,d.pair.resident.call_site,d.pair.resident.calls,d.pair.calls,d.pair.call_site,d.record.freemem_status,d.record.call_site)
    want=(st['callee'],st['cstatus'],st['call_site'],st['pair_calls'],st['rec_calls'],p['rs'],p['inner'],p['site'],p['ic'],p['calls'],p['sroot'],st['rec']['inner'],st['rec']['site'])
    check(actual==want,f'diagnostic {label}: {actual} != {want}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):
        w=u.reg_read(reg)&65535;check(getattr(c,name)==w,f'{label} {name}: C {getattr(c,name):04X}, original {w:04X}')
    a=bytes(ram);w=bytes(u.mem_read(0,SIZE))
    if a!=w:
        diff=[i for i,(x,y) in enumerate(zip(a,w)) if x!=y];raise ValueError(f'{label}: {len(diff)} RAM differences {[(hex(q),a[q],w[q]) for q in diff[:8]]}')
    name=NAMES[status]
    if status==CALLEE:
        name+=f"_{'pair' if d.callee==PAIR else 'record'}_{d.callee_status}"
        name+=f"_rs{d.pair.resident_status}_in{d.pair.resident.freemem_status}" if d.callee==PAIR else f"_in{d.record.freemem_status}"
    elif status in (CODE,TRANSFER):name+=f"_{d.call_site:04X}_{'stub' if d.call_site in FAR_SITES else 'near'}"
    return name,visited,edges
V1,V2=(0x3600,0),(0x3640,0)
def cases():
    a,b=(0x3300,0),(0x3500,0)
    bm={0:(V1,V2),1:(V1,(0,0)),2:((0,0),V2),3:((0,0),(0,0))}
    res=[('head-nil',dict(head=None,active=0)),('single-head-active',dict(records=1,head=0,active=0)),
      ('head-active-next',dict(head=0,active=0)),('head-active-bitmaps',dict(head=0,active=0,bitmaps=bm,slots={0:0})),
      ('walk-1',dict(head=0,active=1)),('walk-2',dict(head=0,active=2)),('walk-1-bitmaps',dict(head=0,active=1,bitmaps=bm,slots={1:1})),
      ('walk-slot2',dict(head=0,active=1,bitmaps=bm,slots={1:2})),('walk-slot3',dict(head=0,active=1,bitmaps=bm,slots={1:3})),
      ('active-not-in-list',dict(head=0,active=None,active_ptr=(0x3180,0))),
      ('active-nil-head-set',dict(head=0,active=None)),
      ('head-offset-differs',dict(head=0,active=0,head_ptr=(0x3100,8),active_ptr=(0x3100,0))),
      ('head-segment-differs',dict(head=0,active=0,head_ptr=(0x3101,0),active_ptr=(0x3100,0))),
      ('walk-next-segment-differs',dict(head=0,active=1,writes=((0x3100,0x104,struct.pack('<HH',0,0x3121)),))),
      ('walk-next-offset-differs',dict(head=0,active=1,writes=((0x3100,0x104,struct.pack('<HH',8,0x3120)),))),
      ('head-ends-before-active',dict(head=0,active=None,active_ptr=(0x3140,0),records=2)),
      ]
    for b6 in (0,1,2,255):
        for b8 in (0,1,255):
            res.append((f'head-b6-{b6}-b8-{b8}',dict(head=0,active=0,b6=b6,b8=b8,b6hi=0xFF if b6 else 0,b8hi=0x5A,bitmaps=bm,slots={0:b6&3})))
            res.append((f'walk-b6-{b6}-b8-{b8}',dict(head=0,active=1,b6=b6,b8=b8,b6hi=0x5A,b8hi=0xFF if b8 else 0,bitmaps=bm,slots={1:b8&3})))
    res+=[('side',dict(head=0,active=1,side=(0x3200,0))),('list63',dict(head=0,active=1,list63=(a,))),('list9',dict(head=0,active=0,list9=(b,))),
      ('all-children',dict(head=0,active=1,side=(0x3200,8),list63=(a,(0x3304,0)),list9=(b,(0x3501,0)),bitmaps=bm,slots={1:0})),
      ('merge-prev',dict(head=0,active=1,blocks=((0x30FE,0,2,0),))),('merge-next',dict(head=0,active=0,blocks=((0x3112,0,3,0),))),
      ('shrink-top',dict(head=0,active=1,top=(0x3132,0))),
      ('invalid-record',dict(head=0,active=1,nodes_off={1:1})),('invalid-record-head',dict(head=0,active=0,nodes_off={0:1})),
      ('invalid-bitmap-a',dict(head=0,active=1,bitmaps={1:((0x3600,1),V2)},slots={1:1})),
      ('invalid-bitmap-b',dict(head=0,active=1,bitmaps={1:(V1,(0x3640,1))},slots={1:1})),
      ('same-bitmap-twice',dict(head=0,active=1,bitmaps={1:(V1,V1)},slots={1:1})),
      ('walk-limit-0',dict(head=0,active=None,active_ptr=(0x3180,0),records=2,writes=((0x3120,0x104,struct.pack('<HH',0,0x3100)),),walk_cap=0)),
      ('walk-cycle',dict(head=0,active=None,active_ptr=(0x3180,0),records=2,writes=((0x3120,0x104,struct.pack('<HH',0,0x3100)),),walk_cap=5)),
      ('list-limit',dict(head=0,active=1,list63=(a,),list_cap=0)),('heap-limit',dict(head=0,active=1,blocks=((0x3001,0,1,0),),heap_cap=0)),
      ('heap-limit-pair',dict(head=0,active=1,blocks=((0x3001,0,1,0),),bitmaps=bm,slots={1:0},heap_cap=0)),
      ('stub-cold-int3f',dict(head=0,active=1,stub=bytes([0xCD,0x3F,0,0,0]))),('stub-wrong-offset',dict(head=0,active=1,stub=bytes([0xEA,0x50,0x01,0,0x40]))),
      ('stub-head-cold',dict(head=0,active=0,stub=bytes([0xCD,0x3F,0,0,0]))),
      ('bad-lcall-opcode-walk',dict(head=0,active=1,writes=((CS,0x3321,b'\x90'),))),('bad-lcall-offset-walk',dict(head=0,active=1,writes=((CS,0x3322,b'\x30'),))),
      ('bad-lcall-opcode-head',dict(head=0,active=0,writes=((CS,0x337B,b'\x90'),))),('bad-lcall-offset-head',dict(head=0,active=0,writes=((CS,0x337C,b'\x30'),))),
      ('bad-near-walk',dict(head=0,active=1,b6=0,writes=((CS,0x334A,b'\xE9'),))),('bad-pushcs-walk',dict(head=0,active=1,b6=0,writes=((CS,0x3349,b'\x90'),))),
      ('bad-near-disp-walk',dict(head=0,active=1,b6=0,writes=((CS,0x334B,b'\x00'),))),
      ('bad-near-head',dict(head=0,active=0,b6=0,writes=((CS,0x33B2,b'\xE9'),))),('bad-pushcs-head',dict(head=0,active=0,b6=0,writes=((CS,0x33B1,b'\x90'),))),
      ('bad-0000-site',dict(head=0,active=1,b6=0,side=(0x3200,0),writes=((CS,0x2F,b'\x90'),))),
      ('relocated-system',dict(head=0,active=1,system=0x6100,bitmaps=bm,slots={1:0})),
      ('relocated-0000-systems',dict(head=0,active=1,side=(0x3200,0),list63=(a,),list9=(b,),systems={0x2F:0x6200,0x76:0x6300,0xBE:SYSTEM,0xCF:0x6200})),
      ('window-is-record',dict(head=0,active=0,window=(0x3100,0))),('window-below-top',dict(head=0,active=1,window=(0x3000,8))),
      ('ss-ds',dict(head=0,active=1,ss=DS,bitmaps=bm,slots={1:0})),('a20-stack',dict(head=0,active=1,ss=0xFFFF)),
      ('ds-is-ss',dict(head=0,active=1,ds=SS)),
    ]
    for sp in range(0x5E60,0x5E82,2):
        for walk in (0,1):
            res.append((f'push-ds-clears-slot-{walk}-{sp:x}',dict(ds=0,ss=0,sp=sp,head=0,active=1 if walk else 0,bitmaps={0:((0,0x1000),(0,0))},slots={0:0,1:0})))
    for sp in (0x8C,0x8D,0x8001,0xFFF7,0xFFF8):res.append((f'sp-{sp:x}',dict(head=0,active=1,sp=sp,bitmaps=bm,slots={1:0},side=(0x3200,0))))
    for sp in (0x0FF0,0x1000,0x1008,0x1010,0x1018,0x1020,0x1028,0x1030,0x1040,0x1060,0x1080,0x10A0):
        res.append((f'heap-stack-{sp:x}',dict(ss=0x3000,sp=sp,head=0,active=1,bitmaps=bm,slots={1:0},side=(0x3200,0),list63=(a,),list9=(b,))))
        res.append((f'heap-stack-head-{sp:x}',dict(ss=0x3000,sp=sp,head=0,active=0,bitmaps=bm,slots={0:0},side=(0x3200,0),list9=(b,))))
    rng=random.Random(0x6C3291)
    pool=[(0,0),V1,V2,(0x3600,1),(0x3680,0)]
    for n in range(360):
        k=rng.randrange(1,6);order=list(range(k));rng.shuffle(order)
        act=rng.choice([None]+list(range(k)));hd=rng.choice([0,0,0,None])
        slot=rng.randrange(256);bitmaps={slot:(rng.choice(pool),rng.choice(pool))}
        o=dict(records=k,order=order,head=hd,active=act,b6=rng.choice((0,1,2,255)),b8=rng.choice((0,1,255)),b6hi=rng.randrange(256),b8hi=rng.randrange(256),
            slots={i:slot for i in range(k)},bitmaps=bitmaps,mode=rng.randrange(4),flags=FLAGS[n%len(FLAGS)],
            side=(0x3200,rng.choice((0,8))) if n%3==0 else None,list63=tuple((0x3300+j*8,rng.choice((0,8))) for j in range(n%4)),
            list9=tuple((0x3500+j*4,rng.choice((0,8))) for j in range(n%3)),blocks=tuple((0x3001+j*8,0,1,0) for j in range(n%5)))
        if act is None:o['active']=None;o['active_ptr']=rng.choice(((0x3180,0),(0,0),(0x3100,8)))
        res.append((f'random-{n}',o))
    res.append(('random-ram',dict(head=0,active=1,background=rng.randbytes(SIZE),bitmaps=bm,slots={1:0},side=(0x3200,0),list63=(a,),list9=(b,))))
    res+=translated()
    return res
def translated():
    """The resident +2595 and +0000 audits' targeted cases, reached through the unlink: the original bytes
    run the same heap/bitmap/list states, the active record placed explicitly at a free address."""
    out=[];REC=(0x3400,0)
    for label,o in r.cases():
        if 'slot' in o or label in('callee-return-alias',) or 'background' in o or label.startswith('stack-slot'):continue
        o=dict(o);sp=o.get('sp',0x8000)
        if sp<0x100:o['sp']=sp+0xC0
        rec,aux=o.get('record',(0x3100,0)),o.get('aux',(0,0));hd={rec:(o.get('planes',1),o.get('stride',16),aux)}
        for which in (0,1):
            x=dict(rec_at=REC,bitmaps={5:(rec,(0,0)) if which==0 else ((0,0),rec)},hdrs=hd,rec_slot=5,mode=o.get('mode',0),blocks=o.get('blocks',()),
                top=o.get('top',(0x3800,0)),writes=o.get('writes',()),system=o.get('system',SYSTEM),sp=o.get('sp',0x8000),ss=o.get('ss',SS),
                flags=o.get('flags',0x602),heap_cap=o.get('cap',64),org=o.get('org',(0x3000,0)))
            out.append((f'res-{which}-{label}',x))
    for k,(label,o) in enumerate(ub.cases()):
        if 'background' in o or (label.startswith('random-') and k%3):continue
        o=dict(o);sp=o.get('sp',0x8000)
        if sp<0x100:continue
        for walk in (0,1):
            x=dict(rec_at=o.get('record',(0x3100,0)),side=o.get('side'),list63=o.get('list63',()),list9=o.get('list9',()),blocks=o.get('blocks',()),
                top=o.get('top',(0x3800,0)),org=o.get('org',(0x3000,0)),writes=o.get('writes',()),systems=o.get('systems'),ss=o.get('ss',SS),sp=sp,
                flags=o.get('flags',0x602),list_cap=o.get('list_cap',64),heap_cap=o.get('heap_cap',64),b6=0,rec_prev=(0x3300,0x0) if walk and not o.get('list63') else None)
            if walk and x['rec_prev']==(0x3300,0):x['rec_prev']=(0x3380,0)
            out.append((f'rel-{walk}-{label}',x))
    return out
def differential(ctx,f):
    visited,edges,stops,named=set(),set(),{},{};rng=random.Random(0x3291)
    for label,opts in cases():
        opts=dict(opts);lim=(opts.pop('walk_cap',64),opts.pop('list_cap',64),opts.pop('heap_cap',64))
        m,c=build(ctx,rng,**opts)
        name,v,e=compare(label,ctx,m,c,f,lim);visited|=v;edges|=e;stops[name]=stops.get(name,0)+1
        if label in EXPECT:check(name.startswith(EXPECT[label]),f'fixture {label} reached {name}, expected {EXPECT[label]}')
        if not label.startswith(('random-','res-','rel-','heap-stack','push-ds','sp-')):named[label]=name
        if not label.startswith('random-'):print(label,name,flush=True)
    T,C=ctx['tables'],ctx['cond']
    wanted={(w,a) for w,t in T.items() for a in t};wanted_edges={(w,a,b) for w,t in C.items() for a,ends in t.items() for b in ends}
    missing=sorted(wanted-visited);missing_edges=sorted(wanted_edges-edges)
    cm=[x for x in missing if x[0]=='caller'];cme=[x for x in missing_edges if x[0]=='caller']
    report=dict(cases=sum(stops.values()),stops=stops,cpu_words=14,ram_bytes=SIZE,masked_bits=0,
        caller_starts=sum(w=='caller' for w,a in visited),caller_total=len(T['caller']),
        caller_outcomes=sum(w=='caller' for w,a,b in edges),caller_outcomes_total=2*len(C['caller']),
        chain_starts=len(visited),chain_total=len(wanted),chain_outcomes=len(edges),chain_outcomes_total=len(wanted_edges),
        missing_caller_starts=[hex(a) for w,a in cm],missing_caller_outcomes=[(hex(a),hex(b)) for w,a,b in cme],
        missing_starts=[(w,hex(a)) for w,a in missing],missing_outcomes=[(w,hex(a),hex(b)) for w,a,b in missing_edges])
    print('Original parity',json.dumps(report,sort_keys=True),flush=True)
    check(not cm and not cme,'caller coverage incomplete')
    check(not missing and not missing_edges,f'closure coverage incomplete: {missing} {missing_edges}')
    report['named_stops']=named;return report
def main():
    p=argparse.ArgumentParser();p.add_argument('--compile-only',action='store_true');p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u6c-3291-') as tmp:
        f=library(Path(tmp))
        if a.compile_only:print('CHECKER_BUILD_OK');return
        ctx=source();report=dict(target=ctx['info']);print('Target',json.dumps(ctx['info'],sort_keys=True),flush=True)
        if a.unicorn:report['unicorn']=differential(ctx,f)
        if a.native:
            from u6c_record_unlink_3291_native import native_audit
            report['native']=native_audit(ctx,f)
    if a.output:a.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':
    import sys
    sys.modules['u6c_record_unlink_3291_audit']=sys.modules[__name__]
    main()
