#!/usr/bin/env python3
"""Directed original-byte native RETF wrap; synthetic frames, not game reach."""
import argparse
import ctypes
from hashlib import sha256
import os
import signal
import time
from pathlib import Path
import struct
import subprocess
import tempfile
import u3ce_gate_014e_audit as base
from u6f_prefix_0482_audit import emulator

def assembly(source,sp,filename):
    lines=['.code16','.intel_syntax noprefix','.global _start','_start:',
           'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
           'mov word ptr [header],ax','mov word ptr [header+2],offset returned',
           'mov word ptr [header+4],offset leaf',f'mov word ptr [header+6],{sp}',
           'cli','add ax,0x1000','mov ss,ax','add ax,0x1000','mov ds,ax',
           'mov sp,0xf000','mov ax,0xa55a']
    for seg,offsets in (('ss',range(0,8,2)),('ss',range(0xfff8,0x10000,2)),('ds',range(0,8,2))):
        for off in offsets:lines.append(f'mov word ptr {seg}:[{off}],ax')
    lines+=['mov word ptr ds:[0x5e5a],0','mov word ptr ds:[0x5e5c],0',
            'mov word ptr ds:[0x5e5e],0','mov word ptr ds:[0x5e60],0']
    if sp>=0xfffe:lines+=['mov ax,es','inc ax',f'mov word ptr ds:[{sp+2-0x10000}],ax']
    if sp==0xffff:lines+=['mov ax,offset returned','mov byte ptr ss:[0xffff],al','mov byte ptr ds:[0],ah']
    else:lines.append(f'mov word ptr ss:[{sp}],offset returned')
    lines.append('mov ax,es')
    if sp==0xfffd:lines+=['mov byte ptr ss:[0xffff],al','mov byte ptr ds:[0],ah']
    else:lines.append(f'mov word ptr ss:[{(sp+2)&65535}],ax')
    lines += [f'mov word ptr ss:[{(sp+4)&65535}],0',
              'push 0x402','popf','mov ax,0x1234','mov bx,0x3456','mov cx,0x5678',
              'mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',
              f'mov sp,{sp}','jmp leaf','.balign 16','returned:',
              'mov word ptr es:[output+28],1','jmp capture','.space 16-(.-returned)',
              'linear_return:','mov word ptr es:[output+28],2','jmp capture','capture:']
    for i,name in enumerate(base.FIELDS[:-1]):
        value='offset returned' if name=='ip' else name
        lines.append(f'mov word ptr es:[output+{2*i}],{value}')
    for seg,start,destination in (('ss',0,30),('ss',0xfff8,38),('ds',0,46)):
        for i in range(0,8,2):
            lines += [f'mov ax,word ptr {seg}:[{start+i}]',f'mov word ptr es:[output+{destination+i}],ax']
    lines += ['pushf','pop ax','mov word ptr es:[output+26],ax',
              'mov word ptr es:[output+54],0x402','mov ax,es','mov ss,ax','mov sp,0xf000',
              'push es','push offset normalized','retf','normalized:',
              'mov ax,cs','mov ds,ax','sti','mov dx,offset filename','xor cx,cx',
              'mov ah,0x3c','int 0x21','jc failure','mov bx,ax','mov dx,offset header',
              'mov cx,64','mov ah,0x40','int 0x21','jc failure','mov ah,0x3e','int 0x21',
              'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21',
              'header: .space 8','output: .space 56',f'filename: .asciz "{filename}"',
              '.org 0x1000','leaf:',f'.incbin "{source.resolve()}"']
    return '\n'.join(lines)+'\n'

def prepare(directory,body):
    entries=[]
    for kind,code in (('T',body[-3:]),('G',body)):
        for sp in (0xfffd,0xfffe,0xffff):
            stem=f'{kind}{sp:04X}'
            source=directory/(stem+'.BIN');source.write_bytes(code)
            asm=directory/(stem+'.S');asm.write_text(assembly(source,sp,stem+'.OUT'))
            obj=directory/(stem+'.o');com=directory/(stem+'.COM')
            subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
            subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
            base.check(com.read_bytes()[0x1000:]==code,'copied original bytes differ')
            result=directory/(stem+'.OUT')
            if result.exists():result.unlink()
            entries.append((kind,sp,result,code))
    config=directory/'dosbox.conf'
    config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\n'+'\n'.join(f'{k}{sp:04X}.COM' for k,sp,_,_ in entries)+'\nexit\n')
    return config,entries

def initial(segment,return_ip,leaf,sp,code):
    m=bytearray(base.RAM_SIZE);ss=(segment+0x1000)&65535;ds=(segment+0x2000)&65535
    for seg,offsets in ((ss,range(0,8,2)),(ss,range(0xfff8,0x10000,2)),(ds,range(0,8,2))):
        for off in offsets:base.put_word(m,seg,off,0xa55a)
    if sp>=0xfffe:base.put_word(m,ds,sp+2-0x10000,(segment+1)&65535)
    base.put_word(m,ss,sp,return_ip);base.put_word(m,ss,(sp+2)&65535,segment)
    base.put_word(m,ss,(sp+4)&65535,0)
    at=base.physical(segment,leaf);m[at:at+len(code)]=code
    c=base.Cpu(0x1234,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x7777,sp,segment,leaf,ds,segment,ss,0x402)
    return m,c

def word(m,s,o):
    at=base.physical(s,o);return m[at] | (m[(at+1)&0xfffff]<<8)

def original_states(m,c,code):
    uc,back=emulator(m,c);ret=base.physical(c.cs,c.ip)+len(code)-3;stopped=[False]
    def stop(u,address,size,data):
        if address==ret:stopped[0]=True;u.emu_stop()
    h=uc.hook_add(base.UC_HOOK_CODE,stop)
    uc.emu_start(base.physical(c.cs,c.ip),0x110000,count=1000)
    base.check(stopped[0],'original instruction return not reached');uc.hook_del(h)
    before=base.Cpu(*(uc.reg_read(r)&65535 for r in base.REGISTERS))
    native=base.Cpu.from_buffer_copy(before)
    native.ip=word(back,before.ss,before.sp)
    native.cs=word(back,before.ss,(before.sp+2)&65535)
    native.sp=(before.sp+6)&65535
    uc.emu_start(ret,0x110000,count=1)
    unicorn=base.Cpu(*(uc.reg_read(r)&65535 for r in base.REGISTERS))
    return native,unicorn,bytes(back)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--prepare-only',action='store_true');parser.add_argument('--diagnose',action='store_true');args=parser.parse_args()
    body,_,_=base.source()
    directory=base.ROOT/'resources/oracle-runs/u3ce-retf-wrap-native'
    directory.mkdir(parents=True,exist_ok=True);config,entries=prepare(directory,body)
    if args.prepare_only:
        print('prepared six original-byte native wrap programs');return
    env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',
             XDG_CONFIG_HOME=str(directory/'.config'),XDG_CACHE_HOME=str(directory/'.cache'),XDG_DATA_HOME=str(directory/'.local'))
    command=['bwrap','--die-with-parent','--unshare-net','--ro-bind','/','/','--dev-bind','/dev','/dev','--bind',str(directory),str(directory),'--chdir',str(directory),'/usr/bin/dosbox-x','-conf',str(config),'-fastlaunch','-exit','-noconsole']
    with (directory/'dosbox.log').open('w') as log:
        process=subprocess.Popen(command,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        started=time.monotonic()
        try:
            while process.poll() is None:
                if time.monotonic()-started>30 or (directory/'dosbox.log').stat().st_size>2097152:
                    raise RuntimeError('native time/log bound exceeded')
                time.sleep(.1)
            base.check(process.returncode==0,'native process failed')
        finally:
            if process.poll() is None:
                os.killpg(process.pid,signal.SIGTERM)
                process.wait(timeout=5)
    problems=[];unicorn_divergences=[];results=[]
    with tempfile.TemporaryDirectory() as temporary:
        lib=base.build(Path(temporary))
        for kind,sp,path,code in entries:
            data=path.read_bytes();base.check(len(data)==64,'incomplete native record')
            segment,ip,leaf,actual_sp=struct.unpack_from('<4H',data)
            base.check(leaf==0x1100 and actual_sp==sp,'native header differs')
            native=base.Cpu.from_buffer_copy(data[8:36]);marker=struct.unpack_from('<H',data,36)[0]
            m,c=initial(segment,ip,leaf,sp,code)
            expected,unicorn,ram=original_states(m,c,code)
            base.check(bytes(native)==bytes(expected),'native differs from two word pops')
            base.check(marker==1,'native took contiguous-CS landing')
            for seg,start,offset in ((c.ss,0,38),(c.ss,0xfff8,46),(c.ds,0,54)):
                at=base.physical(seg,start)
                base.check(data[offset:offset+8]==ram[at:at+8],'native frame/canary differs')
            differences=[name for name in base.FIELDS if getattr(native,name)!=getattr(unicorn,name)]
            if differences:unicorn_divergences.append((kind,f'{sp:04X}',differences))
            if kind=='G':
                c.ip=0x14e;buffer=(ctypes.c_uint8*base.RAM_SIZE).from_buffer_copy(m)
                pending=base.Pending(1,2,3,4)
                status=lib.pr_u3ce_gate_014e(buffer,base.RAM_SIZE,ctypes.byref(c),ctypes.byref(pending))
                diff=[name for name in base.FIELDS if getattr(native,name)!=getattr(c,name)]
                if status!=0 or diff:problems.append((f'{sp:04X}',status,diff))
                for seg,start,offset in ((c.ss,0,38),(c.ss,0xfff8,46),(c.ds,0,54)):
                    at=base.physical(seg,start)
                    base.check(data[offset:offset+8]==bytes(buffer[at:at+8]),'C/native stack/canary differs')
            results.append((path.name,sha256(data).hexdigest()))
    print('native: six original-byte frames use two 16-bit stack word pops; all 14 CPU words and 24 frame/canary bytes checked')
    print('Unicorn divergences',unicorn_divergences)
    print('C gate mismatches',problems)
    for name,digest in results:print(name,digest)
    print('DOSBox SHA256',sha256(Path('/usr/bin/dosbox-x').read_bytes()).hexdigest())
    if not args.diagnose:base.check(not problems,'C/native wrap regression')
    print('synthetic instruction/zero-path evidence; no natural game reach or heap calls')

if __name__=='__main__':main()
