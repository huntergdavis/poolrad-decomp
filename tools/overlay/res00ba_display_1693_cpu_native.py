"""Synthetic DOSBox-X EGA corroboration; full CPU14, selected RAM and planes.
Original code is loaded at rebound segments; far-JMP capture traps are declared.
No successful callee stubs and no native full-1MiB claim.
"""
# NOT_A_ROW_CHECKER: native companion of res00ba_display_1693_cpu_audit.py.
from native_emulator_provenance import describe_dosbox
import os,struct,subprocess
from hashlib import sha256
from res00ba_display_1693_cpu_audit import ROOT,SIZE,CS,DS,SYS,ENTRY,Cpu,FLAGS,physical,put16,install,run_c,check,build,FONT,STRING,DISPLAY,CLEAR,DELAY,SEGMENTS,ROOT_CALLS,stop_name
TOP,LOW,WINDOW,OUTPUT=0xF000,0xEC00,1024,0xC000
RECORD=28+WINDOW+3*65536+4*65536
REGS=(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777)
def specifications():
    result=[dict(text=bytes((65+j*13)&255 for j in range(n)),factor=f) for n in (0,1,3,40,41,255) for f in (0,1)]
    result += [dict(flags=f) for f in FLAGS]
    result += [dict(active=a) for a in (0,2,7)]
    result += [dict(source_off=0xFFFE),dict(fontoff=0xFFFC),dict(fontseg=0xA000),dict(fontseg=0xA000,fontoff=0x1600),dict(pages=(0xA000,0xA000)),dict(writes=((DS,0x5E20,bytes(4)),)),dict(mode=3),dict(mode=255)]
    result += [dict(stop=(DISPLAY,site)) for site in ROOT_CALLS]
    result += [dict(stop=(CLEAR,0x1F1)),dict(stop=(CLEAR,0x20C)),dict(stop=(CS,0x63C)),dict(stop=(CS,0x207)),dict(stop=(CS,0xDCC)),dict(stop=(CS,0x8),refuse=0),dict(stop=(CS,0x108),refuse=18)]
    # Mode0/2 native only before their first clear; no CGA/Tandy device claim.
    result += [dict(mode=m,stop=(DISPLAY,0x16B8)) for m in (0,2)]
    for n,c in enumerate(result):c.setdefault('flags',FLAGS[n%len(FLAGS)])
    return result

def fixture(case,bodies,segment=0x0814,returned=0,bios=0x55):
    options=dict(case);stop=options.pop('stop',None);options.pop('refuse',None)
    writes=list(options.pop('writes',()))
    if stop and 'refuse' not in case:writes.append((*stop,b'\x90'))
    m,c=build(bodies,ss=segment,sp=TOP-8,bios=bios,writes=writes,**options);c.es=segment
    put16(m,segment,TOP-8,returned);put16(m,segment,TOP-6,segment)
    return m,c

def native(bodies,lib):
    dosbox_x=describe_dosbox('dosbox-x')
    directory=ROOT/'resources/display1693/native';directory.mkdir(parents=True,exist_ok=True)
    specs=specifications();payload=bytearray()
    for case in specs:
        m,c=fixture(case,bodies)
        seg,off=case.get('stop',(0,0))
        payload+=struct.pack('<5H',case.get('source_off',0x800),case.get('source_seg',STRING),case['flags'],off,seg)
        payload+=b''.join(m[physical(s,0):physical(s,0)+65536] for s in (DS,FONT,STRING))
    (directory/'INPUT.BIN').write_bytes(payload)
    memory=bytearray(SIZE);install(memory,bodies);blobs=[]
    for n,((kind,start),body) in enumerate(bodies.items()):
        seg=SEGMENTS[kind];at=physical(seg,start);path=directory/f'body{n}.bin';path.write_bytes(memory[at:at+len(body)])
        blobs.append((f'body{n}',seg,start,len(body),path))
    t=['.code16','.intel_syntax noprefix','.global _start','_start: jmp main','.org 0x200','main:',
       'mov ax,cs','mov ds,ax','mov ss,ax',f'mov sp,{TOP}','mov ax,0x000d','int 0x10','cld',
       'mov dx,offset inputname','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [inputhandle],ax',
       'mov dx,offset outputname','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [outputhandle],ax',
       'mov word ptr [0xd800],cs','mov word ptr [0xd802],offset returned',f'mov word ptr [0xd804],{len(specs)}',
       'mov dx,0xd800','mov cx,6','call write_output','again:',
       'push cs','pop ds','mov dx,offset spec','mov cx,10','call read_input']
    for seg in (DS,FONT,STRING):
        for off in (0,0x8000):t += [f'mov ax,{seg}','mov ds,ax',f'mov dx,{off}','mov cx,0x8000','call read_input']
    # Restore all original bytes and loader-equivalent segment bindings per case.
    t+=['push cs','pop ds','cld']
    for label,seg,off,size,path in blobs:t += [f'mov ax,{seg}','mov es,ax',f'mov di,{off}',f'mov si,offset {label}',f'mov cx,{size}','rep movsb']
    t += ['mov di,word ptr cs:[spec+6]','test di,di','jz trap_ready','mov ax,word ptr cs:[spec+8]','mov es,ax',
          'mov byte ptr es:[di],0xea','mov word ptr es:[di+1],offset returned','mov word ptr es:[di+3],cs','trap_ready:',
          'cli','mov dx,0x3ce','mov ax,0x0000','out dx,ax','mov ax,0x0001','out dx,ax',
          'mov ax,0x0003','out dx,ax','mov ax,0x0004','out dx,ax','mov ax,0x0005','out dx,ax','mov ax,0xff08','out dx,ax',
          'mov ax,0xa000','mov es,ax']
    for p in range(4):t += ['mov dx,0x3c4',f'mov ax,{((1<<p)<<8)|2}','out dx,ax','xor di,di',f'mov ax,{(0x31+p*0x27)*257}','mov cx,0x8000','rep stosw']
    t += ['push cs','pop es','cld',f'mov sp,{TOP}','mov al,0xa5',f'mov di,{LOW}',f'mov cx,{WINDOW}','rep stosb',
          'mov ax,0x40','mov ds,ax','mov al,byte ptr [0]','mov byte ptr cs:[bios],al',
          'push word ptr cs:[spec+2]','push word ptr cs:[spec]','push word ptr cs:[spec+4]',f'mov ax,{DS}','mov ds,ax']
    for r,v in zip('ax bx cx dx si di bp'.split(),REGS):t.append(f'mov {r},{v}')
    t+=['popf','call dword ptr cs:[target]','returned:']
    for n,r in enumerate('ax bx cx dx si di bp sp cs'.split()):t.append(f'mov word ptr cs:[{OUTPUT+2*n}],{r}')
    t += [f'mov word ptr cs:[{OUTPUT+18}],offset returned']
    for off,r in ((20,'ds'),(22,'es'),(24,'ss')):t.append(f'mov word ptr cs:[{OUTPUT+off}],{r}')
    t += [f'mov sp,{TOP+0x800}','pushf','pop ax',f'mov word ptr cs:[{OUTPUT+26}],ax',
          'mov ax,word ptr cs:[spec+6]','test ax,ax','jz pc_ready',f'mov word ptr cs:[{OUTPUT+18}],ax',
          'mov ax,word ptr cs:[spec+8]',f'mov word ptr cs:[{OUTPUT+16}],ax','pc_ready:','cld','push cs','pop ds','push cs','pop es',
          f'mov si,{LOW}',f'mov di,{OUTPUT+28}',f'mov cx,{WINDOW}','rep movsb','sti',
          'mov dx,offset bios','mov cx,1','call write_output',f'mov dx,{OUTPUT}',f'mov cx,{28+WINDOW}','call write_output']
    for seg in (DS,FONT,STRING):
        for off in (0,0x8000):t += [f'mov ax,{seg}','mov ds,ax',f'mov dx,{off}','mov cx,0x8000','call write_output']
    for p in range(4):
        t += ['mov dx,0x3ce','mov ax,0x0005','out dx,ax',f'mov ax,{(p<<8)|4}','out dx,ax']
        for off in (0,0x8000):t += ['mov ax,0xa000','mov ds,ax','mov ax,0x7000','mov es,ax',f'mov si,{off}','xor di,di','mov cx,0x8000','cld','rep movsb','mov ax,0x7000','mov ds,ax','xor dx,dx','mov cx,0x8000','call write_output']
    t += ['push cs','pop ds','inc word ptr [index]',f'cmp word ptr [index],{len(specs)}','jb again',
          'mov bx,word ptr [outputhandle]','mov ah,0x3e','int 0x21','mov ax,3','int 0x10','mov ax,0x4c00','int 0x21',
          'write_output:','mov bx,word ptr cs:[outputhandle]','mov ah,0x40','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'read_input:','mov bx,word ptr cs:[inputhandle]','mov ah,0x3f','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'failure:','mov ax,0x4c01','int 0x21',
          'inputname: .asciz "INPUT.BIN"','outputname: .asciz "RESULT.BIN"',
          'inputhandle: .word 0','outputhandle: .word 0','index: .word 0','spec: .space 10','bios: .byte 0',f'target: .word {ENTRY},{DISPLAY}',
          '.org 0x3000']
    for label,seg,off,size,path in blobs:t += [label+':',f'.incbin "{path}"']
    asm=directory/'oracle.S';obj=directory/'oracle.o';asm.write_text('\n'.join(t)+'\n')
    subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
    com=directory/'ORACLE.COM';subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    check(com.stat().st_size+0x100<OUTPUT,'COM/output overlap')
    cfg=directory/'dosbox.conf';cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run([dosbox_x['path'],'-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
    result=output.read_bytes();check(len(result)==6+len(specs)*(RECORD+1),f'native output length {len(result)}')
    segment,returned,count=struct.unpack_from('<3H',result);check(count==len(specs),'native header count')
    stops={}
    for n,case in enumerate(specs):
        at=6+n*(RECORD+1);bios=result[at];record=result[at+1:at+1+RECORD]
        m,c=fixture(case,bodies,segment,returned,bios)
        status,ram,events,writes,ega,d=run_c(lib,m,c,refuse=case.get('refuse'))
        expected=2 if case.get('stop',(None,None))[0]==DISPLAY else 3 if 'stop' in case else 0
        check(status==expected,f'native status {n}: {status}/{expected}')
        for (reg,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(c,reg)==want,f'native CPU {n}/{reg}: C {getattr(c,reg):04X}, DOS {want:04X}')
        mine=ram[physical(segment,LOW):physical(segment,LOW)+WINDOW]+b''.join(ram[physical(s,0):physical(s,0)+65536] for s in (DS,FONT,STRING))+ega[:4*65536]
        if mine!=record[28:]:
            differences=[(hex(i),a,b) for i,(a,b) in enumerate(zip(mine,record[28:])) if a!=b]
            raise ValueError(f'native memory {n}: {differences[:8]} ({len(differences)} bytes)')
        label=stop_name(status,d,c)
        stops[label]=stops.get(label,0)+1
    captured=sorted([(physical(segment,LOW),physical(segment,LOW)+WINDOW)]+[(physical(s,0),physical(s,0)+65536) for s in (DS,FONT,STRING)])
    masked=[];at=0
    for lo,hi in captured:
        if at<lo:masked.append((at,lo))
        at=hi
    if at<SIZE:masked.append((at,SIZE))
    ranges=lambda spans:[dict(start=f'{lo:05X}',end_exclusive=f'{hi:05X}',bytes=hi-lo) for lo,hi in spans]
    return dict(dosbox_x=dosbox_x,cases=len(specs),stops=stops,native_captured_ranges=ranges(captured),native_masked_ranges=ranges(masked),native_masked_bytes=sum(hi-lo for lo,hi in masked),cpu_words=14,ram_bytes=WINDOW+3*65536,ram_comparison='partial; all other backing RAM is masked in this native snapshot',stack_bytes=WINDOW,ds_bytes=65536,font_bytes=65536,string_bytes=65536,plane_bytes=4*65536,flag_masks=0,controller_state='not captured natively; independently compared in Python EGA differential',machine='ega',video_mode='0Dh',core='normal',cpu='386',result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}',synthetic_callee_stubs=0,capture='far JMP replaces reached instruction before CALL/OUT; snapshot reports that original boundary')
