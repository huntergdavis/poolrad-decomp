"""DOSBox-X EGA execution of original U1C +2B14 and its complete closure.
Normal cases execute unchanged bodies except loaded segment relocations.
Stop cases use a flags-neutral far jump immediately before the measured
call/OUT boundary. Captures stay under resources/. No native OUT-trace claim.
"""
import os,struct,subprocess
from hashlib import sha256
from u1c_clear_box_2b14_cpu_audit import ROOT,SIZE,CS,DS,SYS,OVERLAY,ENTRY,Cpu,FLAGS,physical,put16,run_c,check,build,FILL_SITES,RECT_SITES
FONT=0x7000 # ordinary scratch segment used only when exporting planes
TOP,LOW,WINDOW,OUTPUT=0xF000,0xEC00,1024,0xC000
RECORD=28+WINDOW+65536+4*65536
REGS=(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777)
CALL_STOPS=(0x2B27,0xE8E,0xEA8,0x140,0xE41,0xF18)
PORT_STOPS=(8,0x108,0x156,0x15C)
def specifications():
    cases=[dict(active=a) for a in range(8)]
    cases += [dict(flags=f) for f in FLAGS]
    cases += [dict(pc=pc,mode=3) for pc in (0,15,0x7FFF,0x8000,0xFFFF)]
    cases += [dict(mode=3),dict(mode=255)]
    cases += [dict(stop_site=s,mode=0 if s==0xE41 else 2 if s==0xF18 else 1) for s in CALL_STOPS+PORT_STOPS]
    cases += [dict(stop_site=0x2B27,pc=pc) for pc in (0,15,0x7FFF,0xFFFF)]
    for n,c in enumerate(cases):c.setdefault('flags',FLAGS[n%6]);c.setdefault('mode',1)
    return cases
def fixture(case,bodies,segment=0x0814,returned=0):
    options=dict(case);stop=options.pop('stop_site',0)
    if stop in CALL_STOPS:
        options['writes']=((OVERLAY if stop==0x2B27 else CS,stop,b'\x90'),)
    m,c=build(bodies,ss=segment,sp=TOP-4,**options);c.es=segment
    put16(m,segment,TOP-4,returned);put16(m,segment,TOP-2,segment)
    return m,c
def native(bodies,lib):
    directory=ROOT/'resources/u1c-2b14/native';directory.mkdir(parents=True,exist_ok=True)
    specs=specifications();data=bytearray()
    for case in specs:
        m,c=fixture(case,bodies)
        data+=struct.pack('<HH',case['flags'],case.get('stop_site',0))
        data+=b''.join(m[physical(seg,0):physical(seg,0)+65536] for seg in (DS,))
    (directory/'INPUT.BIN').write_bytes(data)
    blobs=[];originals={}
    for n,((kind,start),body) in enumerate(bodies.items()):
        blob=bytearray(body);seg=OVERLAY if kind=='overlay' else CS if kind in ('caller','clear') else SYS
        if kind=='caller' and start==0xB7:struct.pack_into('<H',blob,0x143-start,SYS)
        if kind=='clear':
            for at in FILL_SITES:struct.pack_into('<H',blob,at+3-start,SYS)
        if kind=='overlay':struct.pack_into('<H',blob,0x2B2A-start,CS)
        path=directory/f'body{n}.bin';path.write_bytes(blob)
        blobs.append((f'body{n}',seg,start,len(blob),path))
        originals.update({(seg,start+j):value for j,value in enumerate(blob)})
    t=['.code16','.intel_syntax noprefix','.global _start','_start: jmp main','.org 0x200','main:',
       'mov ax,cs','mov ds,ax','mov ss,ax',f'mov sp,{TOP}','mov ax,0x000d','int 0x10','cld']
    for label,seg,off,size,path in blobs:
        t += [f'mov ax,{seg}','mov es,ax',f'mov di,{off}',f'mov si,offset {label}',f'mov cx,{size}','rep movsb']
    t += ['mov dx,offset inputname','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [inputhandle],ax',
          'mov dx,offset outputname','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [outputhandle],ax',
          'mov word ptr [0xd800],cs','mov word ptr [0xd802],offset returned',f'mov word ptr [0xd804],{len(specs)}',
          'mov dx,0xd800','mov cx,6','call write_output','again:',
          'push cs','pop ds','mov bx,word ptr [inputhandle]','mov dx,offset spec','mov cx,4','mov ah,0x3f','int 0x21',
          'jc failure','cmp ax,4','jne failure','cli',f'mov sp,{TOP}','cld']
    for seg in (DS,):
        for off in (0,0x8000):
            t += [f'mov ax,{seg}','mov ds,ax',f'mov dx,{off}','mov cx,0x8000','call read_input']
    for at in CALL_STOPS+PORT_STOPS:
        seg=OVERLAY if at==0x2B27 else CS
        t += [f'mov ax,{seg}','mov es,ax']
        for j in range(5):t += [f'mov byte ptr es:[{at+j}],{originals[seg,at+j]}']
    t += ['mov di,word ptr cs:[spec+2]','test di,di','jz code_ready',f'mov ax,{CS}',
          'cmp di,0x2b27','jne stop_resident',f'mov ax,{OVERLAY}','stop_resident:','mov es,ax',
          'mov byte ptr es:[di],0xea','mov word ptr es:[di+1],offset returned','mov word ptr es:[di+3],cs','code_ready:',
          'mov dx,0x3ce','mov ax,0x0000','out dx,ax','mov ax,0x0001','out dx,ax',
          'mov ax,0x0003','out dx,ax','mov ax,0x0004','out dx,ax','mov ax,0x0005','out dx,ax','mov ax,0xff08','out dx,ax',
          'mov ax,0xa000','mov es,ax']
    for plane in range(4):
        t += ['mov dx,0x3c4',f'mov ax,{((1<<plane)<<8)|2}','out dx,ax',
              'xor di,di',f'mov ax,{(0x31+plane*0x27)*257}','mov cx,0x8000','rep stosw']
    t += ['push cs','pop es','cld','mov al,0xa5',f'mov di,{LOW}',f'mov cx,{WINDOW}','rep stosb',
          'push word ptr cs:[spec]',f'mov ax,{DS}','mov ds,ax']
    for r,v in zip('ax bx cx dx si di bp'.split(),REGS):t.append(f'mov {r},{v}')
    t += ['popf','call dword ptr cs:[target]','returned:']
    for n,r in enumerate('ax bx cx dx si di bp sp cs'.split()):t.append(f'mov word ptr cs:[{OUTPUT+2*n}],{r}')
    t += [f'mov word ptr cs:[{OUTPUT+18}],offset returned']
    for off,r in ((20,'ds'),(22,'es'),(24,'ss')):t.append(f'mov word ptr cs:[{OUTPUT+off}],{r}')
    t += [f'mov sp,{TOP+0x800}','pushf','pop ax',f'mov word ptr cs:[{OUTPUT+26}],ax',
          'mov ax,word ptr cs:[spec+2]','test ax,ax','jz pc_ready',
          f'mov word ptr cs:[{OUTPUT+18}],ax',f'mov word ptr cs:[{OUTPUT+16}],{CS}',
          'cmp ax,0x2b27','jne pc_ready',f'mov word ptr cs:[{OUTPUT+16}],{OVERLAY}','pc_ready:',
          'cld','push cs','pop ds','push cs','pop es',f'mov si,{LOW}',f'mov di,{OUTPUT+28}',f'mov cx,{WINDOW}','rep movsb',
          'sti',f'mov dx,{OUTPUT}',f'mov cx,{28+WINDOW}','call write_output']
    for seg in (DS,):
        t += [f'mov ax,{seg}','mov ds,ax','xor dx,dx','mov cx,0x8000','call write_output',
              'mov dx,0x8000','mov cx,0x8000','call write_output']
    for plane in range(4):
        t += ['mov dx,0x3ce','mov ax,0x0005','out dx,ax',f'mov ax,{(plane<<8)|4}','out dx,ax']
        for off in (0,0x8000):
            t += ['mov ax,0xa000','mov ds,ax',f'mov ax,{FONT}','mov es,ax',f'mov si,{off}','xor di,di',
                  'mov cx,0x8000','cld','rep movsb',f'mov ax,{FONT}','mov ds,ax','xor dx,dx','mov cx,0x8000','call write_output']
    t += ['push cs','pop ds','inc word ptr [index]',f'cmp word ptr [index],{len(specs)}','jb again',
          'mov bx,word ptr [outputhandle]','mov ah,0x3e','int 0x21','mov ax,3','int 0x10','mov ax,0x4c00','int 0x21',
          'write_output:','mov bx,word ptr cs:[outputhandle]','mov ah,0x40','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'read_input:','mov bx,word ptr cs:[inputhandle]','mov ah,0x3f','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'failure:','mov ax,0x4c01','int 0x21','inputname: .asciz "INPUT.BIN"','outputname: .asciz "RESULT.BIN"',
          'inputhandle: .word 0','outputhandle: .word 0','index: .word 0','spec: .space 4',
          f'target: .word {ENTRY},{OVERLAY}','.org 0x3000']
    for label,seg,off,size,path in blobs:t += [label+':',f'.incbin "{path}"']
    asm=directory/'oracle.S';obj=directory/'oracle.o';asm.write_text('\n'.join(t)+'\n')
    subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
    com=directory/'ORACLE.COM';subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    check(com.stat().st_size+0x100<OUTPUT,'COM/output overlap')
    cfg=directory/'dosbox.conf'
    cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x','-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],
          env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),
          stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
    result=output.read_bytes()
    check(len(result)==6+len(specs)*RECORD,f'native output length {len(result)}')
    segment,returned,count=struct.unpack_from('<3H',result);check(count==len(specs),'native header count')
    for n,case in enumerate(specs):
        record=result[6+n*RECORD:6+(n+1)*RECORD];m,c=fixture(case,bodies,segment,returned)
        stop=case.get('stop_site',0);refuse={8:0,0x108:18,0x156:20,0x15C:21}.get(stop)
        status,ram,events,writes,ega,d=run_c(lib,m,c,refuse=refuse)
        check(status==(2 if stop==0x2B27 else 3 if stop else 0),f'native status {n}: {status}')
        for (reg,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(c,reg)==want,f'native CPU {n}/{reg}: C {getattr(c,reg):04x}, DOS {want:04x}')
        mine=ram[physical(segment,LOW):physical(segment,LOW)+WINDOW]+ram[physical(DS,0):physical(DS,0)+65536]+ega[:4*65536]
        if mine!=record[28:]:
            differences=[(hex(i),a,b) for i,(a,b) in enumerate(zip(mine,record[28:])) if a!=b]
            raise ValueError(f'native memory {n}: {differences[:8]} ({len(differences)} bytes)')
    return dict(cases=len(specs),returns=sum(not c.get('stop_site') for c in specs),pre_call_stops=sum(c.get('stop_site') in CALL_STOPS for c in specs),pre_out_stops=sum(c.get('stop_site') in PORT_STOPS for c in specs),
        cpu_words=14,stack_bytes=WINDOW,ds_bytes=65536,plane_bytes=4*65536,
        native_out_trace=False,flag_masks=0,machine='ega',video_mode='0Dh',core='normal',cpu='386',
        result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
