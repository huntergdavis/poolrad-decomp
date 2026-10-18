#!/usr/bin/env python3
"""Directed U6F original RETF 8 and complete-body wrap frames; not game reach."""
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
import u6f_vector_merge_032f_audit as base


REGIONS=(('ss',0,16),('ss',0xffe8,24),('gs',0,8),('ds',0x4000,55),('ds',0x4200,55),('ds',0x4400,32),('ds',0x4600,32),('ds',0x4a1e,1),('ds',0x5eb2,4),('ds',0x5011,2))
RECORD=38+sum(count for _,_,count in REGIONS)

def assembly(source,sp,filename,mode):
    lines=['.code16','.intel_syntax noprefix','.global _start','_start:',
           'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
           'mov word ptr [header],ax','mov word ptr [header+2],offset returned',
           'mov word ptr [header+4],offset leaf',f'mov word ptr [header+6],{sp}',
           'cli','add ax,0x1000','mov ss,ax','add ax,0x1000','mov ds,ax',
           'mov gs,ax','mov ax,es','mov ds,ax','mov sp,0xf000','mov ax,0xa55a']
    for seg,offsets in (('ss',range(0,16,2)),('ss',range(0xffe8,0x10000,2)),('gs',range(0,8,2))):
        for off in offsets:lines.append(f'mov word ptr {seg}:[{off}],ax')
    for off,count in ((0x4000,55),(0x4200,55),(0x4400,32),(0x4600,32)):
        for i in range(count):lines.append(f'mov byte ptr ds:[{off+i}],{(i*17+off//256+0x5a)&255}')
    for off,value in ((0x4013,0x4400),(0x4213,0x4600),(0x5eb2,0x5000),(0x4211,5),(0x5011,24)):
        lines.append(f'mov word ptr ds:[{off}],{value}')
    lines+=['mov ax,ds','mov word ptr ds:[0x4015],ax','mov word ptr ds:[0x4215],ax',
            'mov word ptr ds:[0x5eb4],ax',f'mov byte ptr ds:[0x4a1e],{mode}']
    if sp>=0xfffe:lines+=['mov ax,es','inc ax',f'mov word ptr gs:[{sp+2-0x10000}],ax']
    if sp==0xffff:lines+=['mov ax,offset returned','mov byte ptr ss:[0xffff],al','mov byte ptr gs:[0],ah']
    else:lines.append(f'mov word ptr ss:[{sp}],offset returned')
    lines.append('mov ax,es')
    if sp==0xfffd:lines+=['mov byte ptr ss:[0xffff],al','mov byte ptr gs:[0],ah']
    else:lines.append(f'mov word ptr ss:[{(sp+2)&65535}],ax')
    for off,value in (((sp+4)&65535,0x4000),((sp+6)&65535,None),((sp+8)&65535,0x4200),((sp+10)&65535,None)):
        lines.append('mov ax,ds' if value is None else f'mov ax,{value}')
        if off==0xffff:lines+=['mov byte ptr ss:[0xffff],al','mov byte ptr gs:[0],ah']
        else:lines.append(f'mov word ptr ss:[{off}],ax')
    lines += [
              'push 0x402','popf','mov ax,0x1234','mov bx,0x3456','mov cx,0x5678',
              'mov dx,0x789a','mov si,0x9abc','mov di,0xbcde','mov bp,0x7777',
              f'mov sp,{sp}','jmp leaf','.balign 16','returned:',
              'mov word ptr cs:[output+28],1','jmp capture','.space 16-(.-returned)',
              'linear_return:','mov word ptr cs:[output+28],2','jmp capture','capture:']
    for i,name in enumerate(base.FIELDS[:-1]):
        value='offset returned' if name=='ip' else name
        lines.append(f'mov word ptr cs:[output+{2*i}],{value}')
    destination=30
    for seg,start,count in REGIONS:
        for i in range(count):
            lines += [f'mov al,byte ptr {seg}:[{start+i}]',f'mov byte ptr cs:[output+{destination+i}],al']
        destination+=count
    lines += ['pushf','pop ax','mov word ptr cs:[output+26],ax',
              'mov ax,cs','mov ss,ax','mov sp,0xf000',
              'push cs','push offset normalized','retf','normalized:',
              'mov ax,cs','mov ds,ax','sti','mov dx,offset filename','xor cx,cx',
              'mov ah,0x3c','int 0x21','jc failure','mov bx,ax','mov dx,offset header',
              f'mov cx,{RECORD}','mov ah,0x40','int 0x21','jc failure','mov ah,0x3e','int 0x21',
              'mov ax,0x4c00','int 0x21','failure:','mov ax,0x4c01','int 0x21',
              'header: .space 8',f'output: .space {RECORD-8}',f'filename: .asciz "{filename}"',
              '.org 0x1000','leaf:',f'.incbin "{source.resolve()}"']
    return '\n'.join(lines)+'\n'

def prepare(directory,body):
    entries=[]
    for kind,mode,code in [('T',3,body[-3:])]+[('G',m,body) for m in range(4)]:
        for sp in (0xfffd,0xfffe,0xffff):
            stem=f'{kind}{mode}{sp:04X}'
            source=directory/(stem+'.BIN');source.write_bytes(code)
            asm=directory/(stem+'.S');asm.write_text(assembly(source,sp,stem+'.OUT',mode))
            obj=directory/(stem+'.o');com=directory/(stem+'.COM')
            subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
            subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
            base.check(com.read_bytes()[0x1000:]==code,'copied original bytes differ')
            result=directory/(stem+'.OUT')
            if result.exists():result.unlink()
            entries.append((kind,mode,sp,result,code))
    config=directory/'dosbox.conf'
    config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory.resolve())+'\nc:\n'+'\n'.join(f'{k}{mode}{sp:04X}.COM' for k,mode,sp,_,_ in entries)+'\nexit\n')
    return config,entries

def initial(segment,return_ip,leaf,sp,code,mode):
    m=bytearray(base.RAM_SIZE);ss=(segment+0x1000)&65535;extension=(segment+0x2000)&65535
    for seg,offsets in ((ss,range(0,16,2)),(ss,range(0xffe8,0x10000,2)),(extension,range(0,8,2))):
        for off in offsets:base.put_word(m,seg,off,0xa55a)
    for off,count in ((0x4000,55),(0x4200,55),(0x4400,32),(0x4600,32)):
        for i in range(count):m[base.physical(segment,off+i)]=(i*17+off//256+0x5a)&255
    for off,value in ((0x4013,0x4400),(0x4015,segment),(0x4213,0x4600),(0x4215,segment),(0x5eb2,0x5000),(0x5eb4,segment),(0x4211,5),(0x5011,24)):
        base.put_word(m,segment,off,value)
    m[base.physical(segment,0x4a1e)]=mode
    if sp>=0xfffe:base.put_word(m,extension,sp+2-0x10000,(segment+1)&65535)
    for offset,value in ((sp,return_ip),((sp+2)&65535,segment),((sp+4)&65535,0x4000),((sp+6)&65535,segment),((sp+8)&65535,0x4200),((sp+10)&65535,segment)):
        base.put_word(m,ss,offset,value)
    at=base.physical(segment,leaf);m[at:at+len(code)]=code
    c=base.Cpu(0x1234,0x3456,0x5678,0x789a,0x9abc,0xbcde,0x7777,sp,segment,leaf,segment,segment,ss,0x402)
    return m,c

def emulator(m,c):
    back=(ctypes.c_uint8*base.RAM_SIZE).from_buffer_copy(m)
    uc=base.Uc(base.UC_ARCH_X86,base.UC_MODE_16);ptr=ctypes.addressof(back)
    uc.mem_map_ptr(0,base.RAM_SIZE,7,ptr);uc.mem_map_ptr(base.RAM_SIZE,0x10000,7,ptr)
    for name,r in zip(base.FIELDS,base.REGISTERS):uc.reg_write(r,getattr(c,name))
    return uc,back

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
    native.sp=(before.sp+12)&65535
    uc.emu_start(ret,0x110000,count=1)
    unicorn=base.Cpu(*(uc.reg_read(r)&65535 for r in base.REGISTERS))
    return native,unicorn,bytes(back)

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--prepare-only',action='store_true');parser.add_argument('--diagnose',action='store_true');base.add_media_arguments(parser);args=parser.parse_args()
    body,_=base.source_audit(args.exe,args.ovr)
    directory=base.ROOT/'resources/oracle-runs/u6f-032f-retf-wrap-native'
    directory.mkdir(parents=True,exist_ok=True);config,entries=prepare(directory,body)
    if args.prepare_only:
        print('prepared fifteen original-byte native wrap programs');return
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
        lib=base.build_c(Path(temporary))
        for kind,mode,sp,path,code in entries:
            data=path.read_bytes();base.check(len(data)==RECORD,'incomplete native record')
            segment,ip,leaf,actual_sp=struct.unpack_from('<4H',data)
            base.check(leaf==0x1100 and actual_sp==sp,'native header differs')
            native=base.Cpu.from_buffer_copy(data[8:36]);marker=struct.unpack_from('<H',data,36)[0]
            m,c=initial(segment,ip,leaf,sp,code,mode)
            expected,unicorn,ram=original_states(m,c,code)
            base.check(bytes(native)==bytes(expected),'native differs from two word pops')
            base.check(marker==1,'native took contiguous-CS landing')
            def captured(memory):
                return b''.join(bytes(memory[base.physical(c.ss if seg=='ss' else ((segment+0x2000)&65535) if seg=='gs' else segment,start):base.physical(c.ss if seg=='ss' else ((segment+0x2000)&65535) if seg=='gs' else segment,start)+count]) for seg,start,count in REGIONS)
            base.check(data[38:]==captured(ram),'native frame/data differs')
            differences=[name for name in base.FIELDS if getattr(native,name)!=getattr(unicorn,name)]
            if differences:unicorn_divergences.append((kind,mode,f'{sp:04X}',differences))
            if kind=='G':
                c.ip=base.ENTRY;buffer=(ctypes.c_uint8*base.RAM_SIZE).from_buffer_copy(m)
                status=lib.pr_u6f_vector_merge_032f(buffer,base.RAM_SIZE,ctypes.byref(c))
                diff=[name for name in base.FIELDS if getattr(native,name)!=getattr(c,name)]
                if status!=0 or diff:problems.append((mode,f'{sp:04X}',status,diff))
                base.check(data[38:]==captured(buffer),'C/native stack/data differs')
            results.append((path.name,sha256(data).hexdigest()))
    print('native: fifteen original-byte frames; all 14 CPU words and 229 stack/data bytes checked')
    print('Unicorn divergences',unicorn_divergences)
    print('C gate mismatches',problems)
    for name,digest in results:print(name,digest)
    print('DOSBox SHA256',sha256(Path('/usr/bin/dosbox-x').read_bytes()).hexdigest())
    if not args.diagnose:base.check(not problems,'C/native wrap regression')
    if args.report:
        args.report.write_text(base.json.dumps(dict(original_frames=15,
            complete_body_cases=12, retf_only_cases=3, cpu_words=14,
            ram_bytes_per_case=229, unicorn_divergences=unicorn_divergences,
            c_mismatches=problems, capture_sha256=dict(results)), indent=2)+"\n")
    print('synthetic instruction/all-four-mode evidence; no natural game reach or external calls')

if __name__=='__main__':main()
