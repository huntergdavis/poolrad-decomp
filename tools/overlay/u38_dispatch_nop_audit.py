#!/usr/bin/env python3
"""Four raw U38 dispatch leaves: original/C and DOSBox-X normal/386 parity."""
import argparse,ctypes,json,os,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from overlay_inventory import find_units,parse_mz
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,OVR_SHA,REGISTERS,physical,put16
ENTRIES=(0x647,0x1355,0x135C,0x319B)
SLOTS={0x647:[16],0x1355:[54],0x135C:[55],0x319B:[5,13,19,24,27,60,63,92,127]}
BODY_SHA='3d5d31028d8927cd81dffdf3a86f216fa4eb84d0874dc3daada06bd31188514d'
CS=0x8800
FLAGS=(0x0002,0x00D7,0x08D7,0x0402,0x0893,0x0003)
def check(v,msg):
    if not v:raise ValueError(msg)
def source():
    ex=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();ov=(ROOT/'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ex).hexdigest()==EXE_SHA and sha256(ov).hexdigest()==OVR_SHA,'media hashes')
    u=next(u for u in find_units(ex[parse_mz(ex)['header_bytes']:],ov) if u['stub_para']==0x38)
    code=ov[u['ovr_offset']:u['ovr_offset']+u['code_size']];md=Cs(CS_ARCH_X86,CS_MODE_16);bodies={};meta={}
    installer=list(md.disasm(code[0x31A2:0x38A9],0x31A2))
    check(sha256(code[0x31A2:0x38A9]).hexdigest()=='4a60eda9584cb62276f52b14e2ae7af2077857c521f3d5a835a344f2f50b2519','installer hash')
    for at in ENTRIES:
        body=code[at:at+7];ins=list(md.disasm(body,at));check(sha256(body).hexdigest()==BODY_SHA,'leaf hash')
        check([(i.mnemonic,i.op_str) for i in ins]==[('push','bp'),('mov','bp, sp'),('pop','bp'),('retf','0xa')],'leaf body')
        entry=u['entries'].index(at);stub=0x20+5*entry;slots=[]
        for n in range(138):
            a,b,c,d=installer[2+4*n:6+4*n]
            if a.op_str==f'ax, {stub:#x}' and b.op_str=='dx, 0x41':
                dest=int(c.op_str.split('[')[1].split(']')[0],0);slots.append((dest-0x6828)//4)
        check(slots==SLOTS[at],f'installer slots {at:x}: {slots}')
        bodies[at]=body;meta[f'{at:04X}']=dict(bytes=7,instructions=4,sha256=BODY_SHA,public_entry=entry,original_stub=f'0041:{stub:04X}',file_start=u['ovr_offset']+at,file_end=u['ovr_offset']+at+7,dispatch_slots=slots)
    return bodies,meta
def library(directory):
    p=directory/'nop.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),str(ROOT/'src/recovered/overlay_u38_dispatch_nop_cpu.c'),'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_u38_dispatch_nop_cpu;f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)];f.restype=ctypes.c_int;return f
def differential(bodies,function):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    rng=random.Random(0x38319B);counts={};stacks=(2,3,0x100,0x8000,0x8001,0xFFF0,0xFFFB,0xFFFC)
    for entry,body in bodies.items():
        seen=set()
        for n in range(96):
            ss=(0x2800,0xFFFF,0x1800)[n%3];sp=stacks[n%len(stacks)];m=bytearray(rng.randbytes(SIZE)) if n%24==0 else bytearray([n])*SIZE
            at=physical(CS,entry);m[at:at+7]=body;put16(m,ss,sp,0x7000+n);put16(m,ss,sp+2,0x6000+n)
            c=Cpu(*[rng.randrange(65536) for _ in range(7)],sp,CS,entry,rng.randrange(65536),rng.randrange(65536),ss,FLAGS[n%len(FLAGS)])
            uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE);uc.mem_write(0,bytes(m));uc.mem_write(SIZE,bytes(m))
            for (name,_),r in zip(Cpu._fields_,REGISTERS):uc.reg_write(r,getattr(c,name))
            done=[False]
            def hook(em,address,size,user):
                del address,size,user
                if done[0]:em.emu_stop();return
                pair=(em.reg_read(UC_X86_REG_CS),em.reg_read(UC_X86_REG_IP));check(pair[0]==CS and pair[1] in (entry,entry+1,entry+3,entry+4),'original escaped')
                seen.add(pair[1]);done[0]=pair[1]==entry+4
            def mirror(em,access,address,size,value,user):
                del access,user
                for j in range(size):
                    raw=address+j;p=raw&(SIZE-1);em.mem_write(p+SIZE if raw<SIZE else p,bytes([(value>>(8*j))&255]))
            uc.hook_add(UC_HOOK_CODE,hook);uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
            uc.emu_start(at,2*SIZE,count=8);check(done[0],'return not reached')
            ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);check(function(ram,SIZE,ctypes.byref(c))==0,'C status')
            check(bytes(ram)==bytes(uc.mem_read(0,SIZE)),f'RAM {entry:x}/{n}')
            for (name,_),r in zip(Cpu._fields_,REGISTERS):check(getattr(c,name)==(uc.reg_read(r)&65535),f'CPU {entry:x}/{n}/{name}')
        check(len(seen)==4,'instruction coverage');counts[f'{entry:04X}']=dict(cases=96,starts=4,conditional_outcomes=0)
    return dict(cases=384,identities=counts,cpu_words=14,ram_bytes=SIZE,flag_masks=0)

def native(bodies,function):
    directory=ROOT/'resources/u38-dispatch-nop/native';directory.mkdir(parents=True,exist_ok=True)
    count=24;low=0xEFC0;window=64;record_size=28+window
    asm=['.code16','.intel_syntax noprefix','.global _start','_start: jmp main','.org 0x200','main:',
         'mov ax,cs','mov ds,ax','mov ss,ax','mov sp,0xf000','cld',f'mov ax,{CS:#x}','mov es,ax']
    for entry,body in bodies.items():
        asm += [f'mov di,{entry:#x}',f'mov si,offset body_{entry:x}','mov cx,7','rep movsb']
    asm += ['mov word ptr [header],cs','mov word ptr [header+2],offset returned',f'mov word ptr [header+4],{count}',
            'mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr [handle],ax','mov bx,ax',
            'mov dx,offset header','mov cx,6','mov ah,0x40','int 0x21','jc failure','again:','cli','cld','mov sp,0xf000',
            'push cs','pop es',f'mov di,{low:#x}',f'mov cx,{window}','mov al,0xa5','rep stosb',
            'mov si,word ptr [index]','shl si,1','mov ax,word ptr [entries+si]','mov word ptr [target],ax',
            'push 0x5555','push 0x4444','push 0x3333','push 0x2222','push 0x1111','push word ptr [flag_values+si]',
            'mov ax,0x1234','mov bx,0x3456','mov cx,0x5678','mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',
            'popf','call dword ptr [target]','returned:']
    for j,reg in enumerate('ax bx cx dx si di bp sp cs'.split()):asm.append(f'mov word ptr [output+{2*j}],{reg}')
    asm += ['mov word ptr [output+18],offset returned','mov word ptr [output+20],ds','mov word ptr [output+22],es','mov word ptr [output+24],ss']
    # Capture the target's stack footprint before PUSHF reuses it.
    for j in range(0,window,2):asm += [f'mov ax,word ptr [{low+j}]',f'mov word ptr [output+{28+j}],ax']
    asm += ['pushf','pop ax','mov word ptr [output+26],ax','cld','sti','mov bx,word ptr [handle]','mov dx,offset output',f'mov cx,{record_size}',
            'mov ah,0x40','int 0x21','jc failure',f'cmp ax,{record_size}','jne failure','inc word ptr [index]',f'cmp word ptr [index],{count}','jb again',
            'mov bx,word ptr [handle]','mov ah,0x3e','int 0x21','mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21',
            'filename: .asciz "RESULT.BIN"','header: .space 6','handle: .word 0','index: .word 0',f'target: .word 0,{CS}',f'output: .space {record_size}',
            'entries: .word '+','.join(str(ENTRIES[n%4]) for n in range(count)),
            'flag_values: .word '+','.join(str(FLAGS[n//4]) for n in range(count))]
    for entry,body in bodies.items():
        blob=directory/f'{entry:04x}.bin';blob.write_bytes(body);asm += [f'body_{entry:x}:',f'.incbin "{blob}"']
    path=directory/'oracle.S';obj=directory/'oracle.o';path.write_text('\n'.join(asm)+'\n')
    subprocess.run(['as','--32',str(path),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(directory/'ORACLE.COM')],check=True)
    config=directory/'dosbox.conf';config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\nORACLE.COM\nexit\n')
    output=directory/'RESULT.BIN'
    if output.exists():output.unlink()
    with (directory/'dosbox.log').open('w') as log:subprocess.run(['dosbox-x','-conf',str(config),'-fastlaunch','-exit','-noconsole'],env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
    result=output.read_bytes();check(len(result)==6+count*record_size,'native length');segment,returned,got=struct.unpack_from('<3H',result);check(got==count,'native count')
    for n in range(count):
        m=bytearray(SIZE);p=physical(segment,low);m[p:p+window]=bytes([0xA5])*window;sp=0xF000-14
        for j,v in enumerate((returned,segment,0x1111,0x2222,0x3333,0x4444,0x5555)):put16(m,segment,sp+2*j,v)
        c=Cpu(0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,CS,ENTRIES[n%4],segment,segment,segment,FLAGS[n//4])
        ram=(ctypes.c_uint8*SIZE).from_buffer_copy(m);check(function(ram,SIZE,ctypes.byref(c))==0,'native C status')
        record=result[6+n*record_size:6+(n+1)*record_size]
        for (name,_),want in zip(Cpu._fields_,struct.unpack_from('<14H',record)):check(getattr(c,name)==want,f'native {n}/{name}: {getattr(c,name):04x}/{want:04x}')
        check(bytes(ram[p:p+window])==record[28:],f'native stack {n}')
    return dict(cases=count,cases_per_identity=6,cpu_words=14,stack_bytes=window,flag_masks=0,result_sha256=sha256(result).hexdigest(),com_segment=f'{segment:04X}')
def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    bodies,meta=source();report=dict(targets=meta)
    with tempfile.TemporaryDirectory(prefix='u38-nop-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(bodies,f)
        if a.native:report['native']=native(bodies,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
