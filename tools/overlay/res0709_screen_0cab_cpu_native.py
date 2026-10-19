"""Original clear execution on DOSBox-X EGA: plain state/planes plus OUT trace.
Guest trace mechanics reuse res0709_ega_reset_0000_cpu_audit.py."""
import os,struct,subprocess
from hashlib import sha256
from res0709_screen_0cab_cpu_audit import ROOT,SIZE,CS,DS,SYS,ENTRY,Cpu,FLAGS,physical,put16,install,run_c,check,build,CLEAR,FILL_SITES,RECT_SITES
TOP,LOW,WINDOW,OUTPUT,TRACE=0xF000,0xEF00,256,0xC000,0x8000
TRACE_BYTES=2+64*32
RECORD=28+WINDOW+65536+4*65536
REGS=(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777)
OUT_SITES=tuple(range(8,0x6F,6))+(0x108,0x10E,0x156,0x15C)
def specifications():
    cases=[dict(active=a) for a in range(8)]
    cases += [dict(flags=f) for f in FLAGS]
    cases += [dict(mode=3),dict(mode=255)]
    cases += [dict(stop_site=ip) for ip in (0xCF7,0xD0D,0x140)+OUT_SITES]
    cases += [dict(mode=0,stop_site=0xCC4),dict(mode=2,stop_site=0xD25)]
    for n,c in enumerate(cases):
        c.setdefault('flags',FLAGS[n%6]);c.setdefault('mode',1);c.setdefault('active',1)
    return cases
def stop_segment(ip):return CLEAR if ip in FILL_SITES+RECT_SITES else CS
def fixture(case,bodies,segment=0x0814,returned=0):
    options=dict(case);stop=options.pop('stop_site',0)
    if stop in FILL_SITES+RECT_SITES+(0x140,):options['writes']=((stop_segment(stop),stop,bytes([0x90])),)
    m,c=build(bodies,ss=segment,sp=TOP-4,**options);c.es=segment
    put16(m,segment,TOP-4,returned);put16(m,segment,TOP-2,segment)
    return m,c
def setup(traced,bodies):
    t=['cli',f'mov sp,{TOP}','cld','push cs','pop ds','push cs','pop es',
       'mov al,0xa5',f'mov di,{LOW}',f'mov cx,{WINDOW}','rep stosb',
       ]
    for seg in (DS,):
        for off in (0,0x8000):t += [f'mov ax,{seg}','mov ds,ax',f'mov dx,{off}','mov cx,0x8000','call read_input']
    t += [f'mov ax,{CS}','mov es,ax']
    originals={}
    for (kind,start),blob in bodies.items():
        if kind not in ('clear','caller'):continue
        body=bytearray(blob);segment=CLEAR if kind=='clear' else CS
        if kind=='caller' and start==0xB7:struct.pack_into('<H',body,0x143-start,SYS)
        if kind=='clear':
            for at in FILL_SITES:struct.pack_into('<H',body,at+3-start,SYS)
        originals.update({(segment,start+j):value for j,value in enumerate(body)})
    for at in (0xCF7,0xD0D,0xCC4,0xD25,0x140)+OUT_SITES:
        segment=stop_segment(at);t += [f'mov ax,{segment}','mov es,ax']
        for j in range(5):t += [f'mov byte ptr es:[{at+j}],{originals.get((segment,at+j),0xA5)}']
    label='traced_ready' if traced else 'plain_ready';returned='returned_traced' if traced else 'returned_plain'
    t += ['mov di,word ptr cs:[spec+14]','test di,di',f'jz {label}','mov ax,word ptr cs:[spec+16]','mov es,ax',
          'mov byte ptr es:[di],0xea',f'mov word ptr es:[di+1],offset {returned}','mov word ptr es:[di+3],cs',label+':']
    t += [
       'mov dx,0x3ce','mov ax,0x0000','out dx,ax','mov ax,0x0001','out dx,ax',
       'mov ax,0x0003','out dx,ax','mov ax,0x0004','out dx,ax','mov ax,0x0005','out dx,ax','mov ax,0xff08','out dx,ax',
       'mov ax,0xa000','mov es,ax']
    for plane in range(4):
        t += ['mov dx,0x3c4',f'mov ax,{((1<<plane)<<8)|2}','out dx,ax',
              'xor di,di',f'mov ax,{(0x31+plane*0x27)*257}','mov cx,0x8000','rep stosw']
    t += ['push cs','pop es','cld','mov al,0xa5',f'mov di,{LOW}',f'mov cx,{WINDOW}','rep stosb','mov ax,word ptr cs:[spec+12]']
    if traced:
        t += ['or ax,0x100','push ax',f'mov di,{TRACE}',f'mov cx,{TRACE_BYTES//2}','xor ax,ax','rep stosw','pop ax',
              f'mov word ptr cs:[trace_ptr],{TRACE+2}','mov word ptr cs:[trace_count],0']
    t += ['push ax',f'mov ax,{DS}','mov ds,ax']
    for r,v in zip('ax bx cx dx si di bp'.split(),REGS):t.append(f'mov {r},{v}')
    return t+['popf','call dword ptr cs:[target]']
def native(bodies,lib):
    directory=ROOT/'resources/screen0cab/native';directory.mkdir(parents=True,exist_ok=True)
    specs=specifications()
    data=bytearray()
    for case in specs:
        m,c=fixture(case,bodies)
        stop=case.get('stop_site',0)
        data+=struct.pack('<9H',0,0,0,0,case['active'],case['mode'],case['flags'],stop,stop_segment(stop))
        payload=m[physical(DS,0):physical(DS,0)+65536]
        data+=payload+payload
    (directory/'INPUT.BIN').write_bytes(data)
    blobs=[]
    for n,((kind,start),body) in enumerate(bodies.items()):
        blob=bytearray(body)
        if kind=='caller' and start==0xB7:struct.pack_into('<H',blob,0x143-start,SYS)
        if kind=='clear':
            for at in FILL_SITES:struct.pack_into('<H',blob,at+3-start,SYS)
        path=directory/f'body{n}.bin';path.write_bytes(blob)
        blobs.append((f'body{n}',CLEAR if kind=='clear' else CS if kind=='caller' else SYS,start,len(blob),path))
    t=['.code16','.intel_syntax noprefix','.global _start','_start: jmp main','.org 0x200','main:',
       'mov ax,cs','mov ds,ax','mov ss,ax',f'mov sp,{TOP}','mov ax,0x000d','int 0x10','cld']
    for label,seg,off,size,path in blobs:
        t += [f'mov ax,{seg}','mov es,ax',f'mov di,{off}',f'mov si,offset {label}',f'mov cx,{size}','rep movsb']
    t += ['xor ax,ax','mov es,ax','mov ax,word ptr es:[4]','mov word ptr [old_int1],ax',
          'mov ax,word ptr es:[6]','mov word ptr [old_int1+2],ax','cli','mov word ptr es:[4],offset int1','mov word ptr es:[6],cs','sti',
          'mov dx,offset inputname','mov ax,0x3d00','int 0x21','jc failure','mov word ptr [inputhandle],ax',
          'mov dx,offset outputname','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [outputhandle],ax',
          'mov word ptr [0xd800],cs','mov word ptr [0xd802],offset returned_plain',f'mov word ptr [0xd804],{len(specs)}',
          'mov dx,0xd800','mov cx,6','call write_output','again:',
          'push cs','pop ds','mov bx,word ptr [inputhandle]','mov dx,offset spec','mov cx,18','mov ah,0x3f','int 0x21',
          'jc failure','cmp ax,18','jne failure']
    t += setup(False,bodies)+['returned_plain:']
    for n,r in enumerate('ax bx cx dx si di bp sp cs'.split()):t.append(f'mov word ptr cs:[{OUTPUT+2*n}],{r}')
    t += [f'mov word ptr cs:[{OUTPUT+18}],offset returned_plain']
    for off,r in ((20,'ds'),(22,'es'),(24,'ss')):t.append(f'mov word ptr cs:[{OUTPUT+off}],{r}')
    t += [f'mov sp,{TOP+0x800}','pushf','pop ax',f'mov word ptr cs:[{OUTPUT+26}],ax','mov ax,word ptr cs:[spec+14]','test ax,ax','jz pc_ready',f'mov word ptr cs:[{OUTPUT+18}],ax','mov ax,word ptr cs:[spec+16]',f'mov word ptr cs:[{OUTPUT+16}],ax','pc_ready:','cld','push cs','pop ds','push cs','pop es',
          f'mov si,{LOW}',f'mov di,{OUTPUT+28}',f'mov cx,{WINDOW}','rep movsb','sti',f'mov dx,{OUTPUT}',f'mov cx,{28+WINDOW}','call write_output',
          f'mov ax,{DS}','mov ds,ax','xor dx,dx','mov cx,0x8000','call write_output','mov dx,0x8000','mov cx,0x8000','call write_output']
    for plane in range(4):
        t += ['mov dx,0x3ce','mov ax,0x0005','out dx,ax',f'mov ax,{(plane<<8)|4}','out dx,ax']
        for off in (0,0x8000):
            t += ['mov ax,0xa000','mov ds,ax','mov ax,0x7000','mov es,ax',f'mov si,{off}','xor di,di','mov cx,0x8000','cld','rep movsb',
                  'mov ax,0x7000','mov ds,ax','xor dx,dx','mov cx,0x8000','call write_output']
    t += setup(True,bodies)+['returned_traced:','pushf','pop ax','and ax,0xfeff','push ax','popf','sti','push cs','pop ds',
          'mov ax,word ptr [trace_count]',f'mov word ptr [{TRACE}],ax',f'mov dx,{TRACE}',f'mov cx,{TRACE_BYTES}','call write_output',
          'inc word ptr [index]',f'cmp word ptr [index],{len(specs)}','jb again',
          'xor ax,ax','mov es,ax','cli','mov ax,word ptr [old_int1]','mov word ptr es:[4],ax',
          'mov ax,word ptr [old_int1+2]','mov word ptr es:[6],ax','sti','mov bx,word ptr [outputhandle]','mov ah,0x3e','int 0x21',
          'mov ax,3','int 0x10','mov ax,0x4c00','int 0x21',
          'write_output:','mov bx,word ptr cs:[outputhandle]','mov ah,0x40','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'read_input:','mov bx,word ptr cs:[inputhandle]','mov ah,0x3f','int 0x21','jc failure','cmp ax,cx','jne failure','ret',
          'failure:','mov ax,0x4c01','int 0x21']
    # Original guest INT1 observer, adapted from the accepted +0000 harness.
    t += ['int1:','push bp','mov bp,sp','push ax','push bx','push si','push di','push ds','push es',
          'mov ax,word ptr [bp+4]',f'cmp ax,{CS}','jne int1_done',
          'mov si,word ptr [bp+2]','mov es,ax','cmp byte ptr es:[si],0xee','jne int1_done',
          'mov si,dx','mov bx,word ptr [bp-2]','xor bh,bh','push cs','pop ds',
          'cmp word ptr [trace_count],64','jae int1_done','mov di,word ptr [trace_ptr]',
          'mov ax,word ptr [trace_count]','mov word ptr [di],ax','mov word ptr [di+2],0x0101',
          'mov ax,word ptr [bp+2]','mov word ptr [di+4],ax','mov ax,word ptr [bp+4]','mov word ptr [di+6],ax',
          'mov word ptr [di+8],si','mov word ptr [di+10],bx','add word ptr [trace_ptr],32','inc word ptr [trace_count]',
          'int1_done:','pop es','pop ds','pop di','pop si','pop bx','pop ax','pop bp','iret',
          'inputname: .asciz "INPUT.BIN"','outputname: .asciz "RESULT.BIN"',
          'inputhandle: .word 0','outputhandle: .word 0','index: .word 0','spec: .space 18',
          'old_int1: .word 0,0','trace_ptr: .word 0','trace_count: .word 0',f'target: .word {ENTRY},{CLEAR}',
          '.org 0x3000']
    for label,seg,off,size,path in blobs:t += [label+':',f'.incbin "{path}"']
    asm=directory/'oracle.S';obj=directory/'oracle.o';asm.write_text('\n'.join(t)+'\n')
    subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
    com=directory/'ORACLE.COM';subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    check(com.stat().st_size+0x100<TRACE,'COM/trace overlap')
    cfg=directory/'dosbox.conf'
    cfg.write_text('[sdl]\noutput=surface\n[dosbox]\nmachine=ega\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x','-conf',str(cfg),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
    result=output.read_bytes()
    check(len(result)==6+len(specs)*(RECORD+TRACE_BYTES),f'native output length {len(result)}')
    segment,returned,count=struct.unpack_from('<3H',result);check(count==len(specs),'native header count')
    outs=0
    for n,case in enumerate(specs):
        at=6+n*(RECORD+TRACE_BYTES);record=result[at:at+RECORD];trace=result[at+RECORD:at+RECORD+TRACE_BYTES]
        m,c=fixture(case,bodies,segment,returned)
        stop=case.get('stop_site',0);refuse=OUT_SITES.index(stop) if stop in OUT_SITES else None
        status,ram,events,writes,ega,d=run_c(lib,m,c,refuse=refuse)
        check(status==(2 if stop in FILL_SITES+RECT_SITES else 3 if stop else 0),f'native status {n}: {status}')
        for (reg,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):
            check(getattr(c,reg)==want,f'native CPU {n}/{reg}: C {getattr(c,reg):04x}, DOS {want:04x}')
        mine=ram[physical(segment,LOW):physical(segment,LOW)+WINDOW]+ram[physical(DS,0):physical(DS,0)+65536]+ega[:4*65536]
        if mine!=record[28:]:
            differences=[(hex(i),a,b) for i,(a,b) in enumerate(zip(mine,record[28:])) if a!=b]
            raise ValueError(f'native memory {n}: {differences[:8]} ({len(differences)} bytes)')
        count=struct.unpack_from('<H',trace)[0];logged=[]
        for j in range(count):
            seq,kind,size,ip,cs,port,value=struct.unpack_from('<HBBHHHH',trace,2+32*j)
            check(seq==j and kind==size==1 and trace[2+32*j+12:2+32*(j+1)]==bytes(20),f'bad OUT trace {n}/{j}')
            logged.append((port,value,size,cs,ip))
        check(logged==events,f'native OUT trace {n}: {logged}/{events}')
        outs+=count
    return dict(cases=len(specs),cpu_words=14,stack_bytes=WINDOW,ds_bytes=65536,plane_bytes=4*65536,
        traced_out_events=outs,pre_call_code_cases=5,pre_out_port_cases=22,packed_pre_write_cases=2,flag_masks=0,machine='ega',video_mode='0Dh',core='normal',cpu='386',
        result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
