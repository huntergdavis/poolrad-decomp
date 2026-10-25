"""Original +02FA groups under DOSBox-X normal/386; explicit pre-handler trap."""
# NOT_A_ROW_CHECKER: native helper imported by u9e_sidecar_groups_02fa_cpu_audit.py --native; this companion is not the row-checker entry point.
from native_emulator_provenance import describe_dosbox
from loaded_stub_fixtures import loaded_sites
import ctypes,os,struct,subprocess
from hashlib import sha256
from u9e_sidecar_groups_02fa_cpu_audit import ROOT,SIZE,CS,DS,NODES,FLAGS,ENTRY,Cpu,Limits,Diagnostic,DEFAULT,physical,put16,build,cases,segments,check,GROUPS
LOW,WINDOW,TOP=0xEC00,1024,0xF000
OUTPUT,HEADER=0xC000,0xD800
REGS=(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777)
RECORD=30+WINDOW+2*65536
def specifications():
    result=[]
    for group,types in enumerate(GROUPS):
        result += [(f'empty-{group}',dict(group=group)),
                   (f'first-{group}',dict(group=group,direct=(types[0],))),
                   (f'last-{group}',dict(group=group,direct=(types[-1],)))]
    for group,key in ((10,49),(11,45),(11,46),(15,21)):
        for combat in (0,5):
            result.append((f'scan-{group}-{key}-{combat}',dict(group=group,members=((key,),),combat=combat)))
    for flags in FLAGS:
        result.append((f'invalid-{flags}',dict(group=255,flags=flags)))
    result += [(f'inner-binding-{ip:x}',dict(group=10,members=((49,),),stop_site=ip)) for ip in (0x22A,0x250)]
    result += [('outer-binding',dict(group=0,stop_site=0x311))]
    return [(name,dict(options,flags=options.get('flags',FLAGS[n%6]))) for n,(name,options) in enumerate(result)]
def fixture(case,bodies,decoded,segment=0x0814,returned=0):
    options=dict(case);stop=options.pop('stop_site',0)
    if stop:options['writes']=((CS,stop,b'\x90'),)
    m,c,l=build(bodies,decoded,ss=segment,sp=TOP-10,**options)
    frame=bytes(m[physical(segment,TOP-10):physical(segment,TOP)])
    at=physical(segment,LOW);m[at:at+WINDOW]=bytes([0xA5])*WINDOW
    m[physical(segment,TOP-10):physical(segment,TOP)]=frame
    put16(m,segment,TOP-10,returned);put16(m,segment,TOP-8,segment)
    c=Cpu(*REGS,TOP-10,CS,ENTRY,DS,segment,segment,case['flags'])
    return m,c,l
def native(bodies,decoded,function):
    directory=ROOT/'resources/u9e-02fa/native';directory.mkdir(parents=True,exist_ok=True)
    specs=specifications();inputs=bytearray()
    for _,case in specs:
        m,c,l=fixture(case,bodies,decoded);inputs+=struct.pack('<H',c.flags)+m[physical(c.ss,c.sp+4):physical(c.ss,c.sp+10)]+struct.pack('<H',case.get('stop_site',0))
        for seg in (DS,NODES):inputs+=m[physical(seg,0):physical(seg,0)+65536]
    (directory/'INPUT.BIN').write_bytes(inputs)
    m,c,l=fixture(specs[0][1],bodies,decoded);segs=segments(c,l);blobs=[]
    for j,((kind,start),body) in enumerate(bodies.items()):
        p=directory/f'body{j}.bin';p.write_bytes(m[physical(segs[kind],start):physical(segs[kind],start)+len(body)])
        blobs.append((f'body{j}',segs[kind],start,p,len(body)))
    for j,(seg,off) in enumerate(sorted(loaded_sites(m,segs,decoded))):
        p=directory/f'stub{j}.bin';p.write_bytes(bytes(m[physical(seg,off+i)] for i in range(5)))
        blobs.append((f'stub{j}',seg,off,p,5))
    t=['.code16','.intel_syntax noprefix','.global _start','_start: jmp main','.org 0x200','main:',
       'mov ax,cs','mov ds,ax','mov ss,ax',f'mov sp,{TOP}','cld']
    for label,seg,off,path,length in blobs:
        t += [f'mov ax,{seg}','mov es,ax',f'mov di,{off}',f'mov si,offset {label}',f'mov cx,{length}','rep movsb']
    # Far JMP replaces the unresolved call only. It changes no general register,
    # flag, or stack byte. Captured CS:IP identifies the replaced boundary.
    t += [f'mov ax,{CS}','mov es,ax','mov byte ptr es:[0x1e],0xea','mov word ptr es:[0x1f],offset pending','mov word ptr es:[0x21],cs',
          'mov dx,offset inputname','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [inputhandle],ax',
          'mov dx,offset outputname','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [outputhandle],ax',
          f'mov word ptr [{HEADER}],cs',f'mov word ptr [{HEADER+2}],offset returned',f'mov word ptr [{HEADER+4}],{len(specs)}',
          f'mov dx,{HEADER}','mov cx,6','call write_output','again:','push cs','pop ds',f'mov sp,{TOP}',
          'mov dx,offset spec','mov cx,10','call read_input']
    for seg in (DS,NODES):
        for off in (0,0x8000):t += [f'mov ax,{seg}','mov ds,ax',f'mov dx,{off}','mov cx,0x8000','call read_input']
    t += [f'mov ax,{CS}','mov es,ax','mov byte ptr es:[0x22a],0x9a','mov word ptr es:[0x22b],0xc0f',f'mov word ptr es:[0x22d],{l.slots_cs}',
          'mov byte ptr es:[0x250],0x9a','mov word ptr es:[0x251],0x909',f'mov word ptr es:[0x253],{l.chain_cs}',
          'mov di,word ptr cs:[spec+8]','test di,di','jz ready','mov byte ptr es:[di],0xea','mov word ptr es:[di+1],offset bad_binding','mov word ptr es:[di+3],cs','ready:']
    # Restore the three-byte outer CALL plus its following bytes before any
    # case can replace that boundary with the five-byte far-JMP capture trap.
    original=m[physical(CS,0x311):physical(CS,0x316)]
    for j,value in enumerate(original):t += [f'mov byte ptr es:[{0x311+j}],{value}']
    t += ['mov di,word ptr cs:[spec+8]','cmp di,0x311','jne outer_ready',
          'mov byte ptr es:[di],0xea','mov word ptr es:[di+1],offset bad_binding','mov word ptr es:[di+3],cs','outer_ready:']
    t += ['cli','cld','push cs','pop ds','push cs','pop es','mov al,0xa5',f'mov di,{LOW}',f'mov cx,{WINDOW}','rep stosb',
          'push word ptr [spec+6]','push word ptr [spec+4]','push word ptr [spec+2]','push word ptr [spec]',
          f'mov ax,{DS}','mov ds,ax']
    for reg,value in zip('ax bx cx dx si di bp'.split(),REGS):t.append(f'mov {reg},{value}')
    t += ['popf','call dword ptr cs:[target]','returned:','mov word ptr cs:[status],0','jmp record','pending:','mov word ptr cs:[status],5','jmp record','bad_binding:','mov word ptr cs:[status],2','record:']
    for j,reg in enumerate('ax bx cx dx si di bp sp'.split()):t.append(f'mov word ptr cs:[{OUTPUT+2*j}],{reg}')
    for off,reg in ((20,'ds'),(22,'es'),(24,'ss')):t.append(f'mov word ptr cs:[{OUTPUT+off}],{reg}')
    t += [f'mov sp,{TOP+0x800}','pushf','pop ax',f'mov word ptr cs:[{OUTPUT+26}],ax',
          'cmp word ptr cs:[status],0','jne trapped',f'mov word ptr cs:[{OUTPUT+16}],cs',f'mov word ptr cs:[{OUTPUT+18}],offset returned',
          'jmp captured_pc','trapped:',f'mov word ptr cs:[{OUTPUT+16}],{CS}','cmp word ptr cs:[status],2','jne was_pending','mov ax,word ptr cs:[spec+8]',f'mov word ptr cs:[{OUTPUT+18}],ax','jmp captured_pc','was_pending:',f'mov word ptr cs:[{OUTPUT+18}],0x1e','captured_pc:',
          'mov ax,word ptr cs:[status]',f'mov word ptr cs:[{OUTPUT+28}],ax','cld','push cs','pop ds','push cs','pop es',
          f'mov si,{LOW}',f'mov di,{OUTPUT+30}',f'mov cx,{WINDOW}','rep movsb','sti',
          f'mov dx,{OUTPUT}',f'mov cx,{30+WINDOW}','call write_output']
    for seg in (DS,NODES):
        for off in (0,0x8000):t += [f'mov ax,{seg}','mov ds,ax',f'mov dx,{off}','mov cx,0x8000','call write_output']
    t += ['push cs','pop ds','inc word ptr [index]',f'cmp word ptr [index],{len(specs)}','jb again',
          'mov bx,word ptr [outputhandle]','mov ah,0x3e','int 0x21','mov ax,0x4c00','int 0x21',
          'read_input:','mov bx,word ptr cs:[inputhandle]','mov ah,0x3f','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'write_output:','mov bx,word ptr cs:[outputhandle]','mov ah,0x40','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'failure:','mov ax,0x4c01','int 0x21','inputname: .asciz "INPUT.BIN"','outputname: .asciz "RESULT.BIN"',
          'inputhandle: .word 0','outputhandle: .word 0','spec: .space 10','status: .word 0','index: .word 0',f'target: .word {ENTRY},{CS}',
          '.org 0x4000']
    for label,seg,off,path,length in blobs:t += [label+':',f'.incbin "{path}"']
    asm=directory/'oracle.S';obj=directory/'oracle.o';asm.write_text('\n'.join(t)+'\n')
    subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
    com=directory/'ORACLE.COM';subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    check(com.stat().st_size<OUTPUT-0x100,'COM overlaps capture')
    cfg=directory/'dosbox.conf';cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    emulator = describe_dosbox('dosbox-x')
    with (directory/'dosbox.log').open('w') as log:subprocess.run([emulator['path'],'-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
    result=output.read_bytes();check(len(result)==6+len(specs)*RECORD,f'native result length {len(result)}')
    segment,returned,count=struct.unpack_from('<3H',result);check(count==len(specs),'native count');stops={}
    for j,(name,case) in enumerate(specs):
        record=result[6+j*RECORD:6+(j+1)*RECORD];m,c,l=fixture(case,bodies,decoded,segment,returned)
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);l=function.argtypes[3]._type_.from_buffer_copy(l);limits=function.argtypes[4]._type_(*DEFAULT);d=function.argtypes[5]._type_()
        status=function(ram,SIZE,ctypes.byref(c),ctypes.byref(l),ctypes.byref(limits),ctypes.byref(d))
        check(status in (0,2,5) and struct.unpack_from('<H',record,28)[0]==status,f'native status {name}: {status}')
        for (field,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(c,field)==want,f'native CPU {name}/{field}: {getattr(c,field):04x}/{want:04x}')
        actual=b''.join(bytes(ram[physical(seg,off):physical(seg,off)+length]) for seg,off,length in ((segment,LOW,WINDOW),(DS,0,65536),(NODES,0,65536)))
        if actual!=record[30:]:
            diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,record[30:])) if a!=b];raise ValueError(f'native RAM {name}: {diff[:8]} ({len(diff)} bytes)')
        stops[str(status)]=stops.get(str(status),0)+1
    return dict(dosbox_x=emulator,cases=len(specs),stops=stops,cpu_words=14,stack_bytes=WINDOW,ds_bytes=65536,nodes_bytes=65536,
        flag_masks=0,boundary_traps='far JMP at U9E001E or inner or outer bad-binding checkpoints; captured CS:IP names the replaced instruction',
        result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
