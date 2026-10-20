#!/usr/bin/env python3
"""DOSBox-X normal/386 synthetic original-chain captures; no CPU/flag masks.

Completed calls execute all original bodies with only the original loader segment
relocation. Budget witnesses replace the first unexecuted instruction with an
explicit far-jump capture trampoline; its CS:IP is reported as the intercepted
boundary, not mistaken for an original instruction. Checkpoints compare twelve
saved register words plus two declared boundary coordinates; completed calls
compare thirteen saved words plus the executed return-label IP. Five 64KiB synthetic
stack/data regions are captured after moving capture stack outside those regions.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import struct
import shutil
import subprocess
from u9e_character_pair_115c_audit import ROOT,SIZE,Cpu,physical,put16,fixture,source,install,build_model,run_c,original
OUT=ROOT/'resources/oracle-native'


def assemble(directory,text,name='ORACLE'):
    asm=directory/(name+'.S');obj=directory/(name+'.o');com=directory/(name+'.COM')
    asm.write_text(text)
    subprocess.run(['as','--32',str(asm),'-o',str(obj)],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext','0x100','--oformat','binary',str(obj),'-o',str(com)],check=True)
    symbols={}
    for line in subprocess.check_output(['nm',str(obj)],text=True).splitlines():
        fields=line.split()
        if len(fields)==3:symbols[fields[2]]=int(fields[0],16)+0x100
    assert len(com.read_bytes())+0x100<0xe000
    return symbols


def launch(directory,program='ORACLE.COM'):
    config=directory/'dosbox.conf'
    config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n[mixer]\nnosound=true\n[midi]\nmididevice=none\n[autoexec]\nmount c '+str(directory)+'\nc:\n'+program+'\nexit\n')
    with (directory/'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x','-conf',str(config),'-fastlaunch','-exit','-noconsole'],
                       env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),
                       stdout=log,stderr=subprocess.STDOUT,check=True,timeout=45)


def find_layout(directory):
    assemble(directory,'''.intel_syntax noprefix
.code16
.global _start
_start:
mov ax,cs
mov ds,ax
mov word ptr [layout],ax
mov dx,offset filename
xor cx,cx
mov ah,0x3c
int 0x21
mov bx,ax
mov dx,offset layout
mov cx,2
mov ah,0x40
int 0x21
mov ah,0x3e
int 0x21
mov ax,0x4c00
int 0x21
filename: .asciz "LAYOUT.BIN"
layout: .word 0
''','LAYOUT')
    launch(directory,'LAYOUT.COM')
    return int.from_bytes((directory/'LAYOUT.BIN').read_bytes(),'little')


def assembly(cpu,segments,checkpoint=None):
    a=['.intel_syntax noprefix','.code16','.global _start','_start:',
       'mov ax,cs','mov ds,ax','mov es,ax','mov ss,ax','mov sp,0xf000',
       'mov bx,0x7100','mov ah,0x4a','int 0x21','jc failure']
    for index,segment in enumerate(segments):
        a += ['mov ax,cs','mov ds,ax',f'mov dx,offset input{index}','mov ax,0x3d00','int 0x21','jc failure','mov bx,ax',
              f'mov ax,{segment}','mov ds,ax','xor dx,dx','mov cx,0xffff','mov ah,0x3f','int 0x21','jc failure','cmp ax,0xffff','jne failure',
              'mov dx,0xffff','mov cx,1','mov ah,0x3f','int 0x21','jc failure','cmp ax,1','jne failure','mov ah,0x3e','int 0x21']
    a += ['mov ax,cs','mov ds,ax','mov dx,offset filename','xor cx,cx','mov ah,0x3c','int 0x21','jc failure','mov word ptr cs:[handle],ax',
          'cli',f'push {cpu.flags}','popf']
    for name in ('ds','es','ss'):
        a += [f'mov ax,{getattr(cpu,name)}',f'mov {name},ax']
    for name in ('ax','bx','cx','dx','si','di','bp','sp'):a += [f'mov {name},{getattr(cpu,name)}']
    a += ['jmp far ptr cs:[dispatch]','returned:']
    for index,name in enumerate(('ax','bx','cx','dx','si','di','bp','sp')):a += [f'mov word ptr cs:[output+{index*2}],{name}']
    if checkpoint is None:
        a += ['mov word ptr cs:[output+16],cs','mov word ptr cs:[output+18],offset returned']
    else:
        a += [f'mov word ptr cs:[output+16],{checkpoint[0]}',f'mov word ptr cs:[output+18],{checkpoint[1]}']
    for index,name in enumerate(('ds','es','ss'),10):a += [f'mov word ptr cs:[output+{index*2}],{name}']
    # MOVs preserve FLAGS. Move SS:SP first so PUSHF cannot alter observed stack.
    a += ['mov ax,cs','mov ss,ax','mov sp,0xf000','pushf','pop word ptr cs:[output+26]',
          'mov ds,ax','mov bx,word ptr cs:[handle]','mov dx,offset output','mov cx,28','mov ah,0x40','int 0x21','jc failure','cmp ax,28','jne failure']
    for segment in segments[1:]:
        a += [f'mov ax,{segment}','mov ds,ax','mov bx,word ptr cs:[handle]','xor dx,dx','mov cx,0xffff','mov ah,0x40','int 0x21','jc failure','cmp ax,0xffff','jne failure',
              'mov dx,0xffff','mov cx,1','mov ah,0x40','int 0x21','jc failure','cmp ax,1','jne failure']
    a += ['mov bx,word ptr cs:[handle]','mov ah,0x3e','int 0x21','mov ax,0x4c00','int 0x21',
          'failure: mov ax,0x4c01','int 0x21','filename: .asciz "RESULT.BIN"','handle: .word 0',
          f'dispatch: .word {cpu.ip},{cpu.cs}','output: .space 28']
    for index in range(len(segments)):a += [f'input{index}: .asciz "I{index}.BIN"']
    return '\n'.join(a)+'\n'


def native_case(directory,base,options,bodies,decoded,lib,budget=None,checkpoint_ip=None):
    segments=[base+0x1000,base+0x2000,base+0x3000,base+0x4000,base+0x5000,base+0x6000]
    code,ds,ss,record,node,out=segments;resident=code+0x200
    opts=dict(cs=code,resident_cs=resident,ds=ds,ss=ss,record_seg=record,node_seg=node,out_seg=out)
    opts.update(options);state=fixture(**opts);memory,cpu,_,kind=state
    install(memory,cpu,resident,bodies)
    if kind=='res':cpu.cs=resident
    # install() used overlay CS before switching entry to the resident.
    if checkpoint_ip is not None:
        for budget in range(1,2000):
            trial=run_c(lib,state,budget)
            if trial[0]==2 and trial[2].ip==checkpoint_ip:break
        else:raise AssertionError(('checkpoint not reached',checkpoint_ip))
    limit=100000 if budget is None else budget
    checkpoint=None
    if budget is not None:
        status,_,boundary,_=run_c(lib,state,limit)
        assert status==2,('budget witness must stop before an instruction',status)
        checkpoint=(boundary.cs,boundary.ip)
        raw=original(state,decoded,limit)
        assert (raw[2].cs,raw[2].ip)==checkpoint,('original checkpoint location differs',checkpoint,(raw[2].cs,raw[2].ip))
        checkpoint_kind='res' if checkpoint[0]==resident else 'u9e'
        assert (checkpoint_kind,checkpoint[1]) not in raw[4],'checkpoint already executed: interception would stop too soon'
    symbols=assemble(directory,assembly(cpu,segments,checkpoint))
    put16(memory,cpu.ss,cpu.sp,symbols['returned']);put16(memory,cpu.ss,(cpu.sp+2)&65535,base)
    expected=run_c(lib,state,limit)
    assert expected[0]==(2 if budget is not None else 0)
    raw_differences={}
    if checkpoint or (kind=='res' and cpu.ip==0):
        raw=original(state,decoded,limit)
        assert raw[1]==expected[1],'raw original RAM differs before native corroboration'
        raw_differences={name:dict(model=getattr(expected[2],name),unicorn=getattr(raw[2],name)) for name,_ in Cpu._fields_ if getattr(expected[2],name)!=getattr(raw[2],name)}
    native_memory=bytearray(memory)
    if checkpoint:
        # Explicit synthetic interception, never presented as original code.
        address=physical(*checkpoint)
        native_memory[address:address+5]=b'\xea'+symbols['returned'].to_bytes(2,'little')+base.to_bytes(2,'little')
    for index,segment in enumerate(segments):
        at=physical(segment,0);(directory/f'I{index}.BIN').write_bytes(native_memory[at:at+65536])
    (directory/'RESULT.BIN').unlink(missing_ok=True);launch(directory)
    result=(directory/'RESULT.BIN').read_bytes();assert len(result)==28+5*65536,len(result)
    registers=struct.unpack_from('<14H',result)
    for (name,_),value in zip(Cpu._fields_,registers):
        assert getattr(expected[2],name)==value,(options,budget,name,hex(getattr(expected[2],name)),hex(value))
    for index,segment in enumerate(segments[1:]):
        at=physical(segment,0);wanted=expected[1][at:at+65536];actual=result[28+index*65536:28+(index+1)*65536]
        if wanted!=actual:
            diffs=[(hex(j),a,b) for j,(a,b) in enumerate(zip(wanted,actual)) if a!=b]
            raise AssertionError((options,budget,'RAM',hex(segment),diffs[:20]))
    return dict(options=options,budget=budget,checkpoint=list(checkpoint) if checkpoint else None,
                cpu_words=14,direct_register_words=12 if checkpoint else 13,known_control_coordinates=2 if checkpoint else 1,
                coordinate_provenance='declared original CS:IP at first unexecuted instruction' if checkpoint else 'IP is the executed return/capture label',
                data_bytes=5*65536,masked_flags=0,result_sha256=sha256(result).hexdigest(),
                entry_kind=kind,entry_offset=f'{cpu.ip:04X}',original_entry_complete=checkpoint is None,synthetic_interception=checkpoint is not None,
                raw_unicorn_cpu_differences=raw_differences)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--quick',action='store_true');parser.add_argument('--output-dir',type=Path,default=OUT);args=parser.parse_args()
    out=args.output_dir;out.mkdir(parents=True,exist_ok=True);base=find_layout(out);bodies,decoded=source();lib=build_model(out)
    cases=[dict(major=1),dict(nodes=()),dict(nodes=({'kind':38,'attribute':127},)),
           dict(items=({'id':0,'bonus':8,'weight':5000,'quantity':3},{'id':0x1c,'subtype':0xba,'weight':400},{'id':0x49,'bonus':128}),
                tables={0:{2:0,8:129,0x10:0x87},0x1c:{2:9,8:128},0x49:{2:9,8:255}}),
           dict(major=18,minor=100,old_major=18,old_minor=50),dict(major=25,minor=255)]
    if not args.quick:cases += [dict(sp=sp,major=major) for sp in (0xfffe,0xffff,0x20,0x21) for major in (1,18)]
    cases += [dict(kind='res',entry=0,items=({'id':0,'bonus':3},),tables={0:{2:0,0x10:0x87}},record_fields={0xaa:1}),
              dict(kind='res',entry=0xbb8,items=({'id':1,'bonus':5,'subtype':0xba,'weight':5000},),tables={1:{2:2,8:129}})]
    if not args.quick:cases += [dict(kind='res',entry=entry,sp=sp) for entry in (0x10e6,0x1168,0x11ea,0x1282,0x1325,0x13ab) for sp in (0xfffe,0xffff)]
    report=[]
    for index,options in enumerate(cases):
        directory=out/f'case-{index:02d}';directory.mkdir(exist_ok=True)
        report.append(native_case(directory,base,options,bodies,decoded,lib));print(f'PASS native {index}',flush=True)
    for value,checkpoint in ((17,0x1430),(19,0x144d),(23,0x1475)):
        directory=out/f'mul-{value}';directory.mkdir(exist_ok=True)
        report.append(native_case(directory,base,dict(kind='res',entry=0x13ab,old_major=value),bodies,decoded,lib,checkpoint_ip=checkpoint));print(f'PASS native MUL {value}',flush=True)
    for checkpoint in (0x0c51,0x0c82):
        directory=out/f'core-stop-{checkpoint:x}';directory.mkdir(exist_ok=True)
        report.append(native_case(directory,base,dict(kind='res',entry=0xbb8,items=({'id':1,'weight':65535,'quantity':255},),tables={1:{2:2}}),bodies,decoded,lib,checkpoint_ip=checkpoint));print(f'PASS native resident checkpoint {checkpoint:x}',flush=True)
    result=dict(target='OVL_009E_115C',profile='DOSBox-X normal/386',dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(),native_cases=len(report),complete_u9e_entries=sum(c['entry_kind']=='u9e' and c['original_entry_complete'] for c in report),complete_resident_entries=sum(c['entry_kind']=='res' and c['original_entry_complete'] for c in report),checkpoint_captures=sum(c['synthetic_interception'] for c in report),cpu_words=14,ram_bytes_per_case=327680,masked_flags=0,natural_game_route_measured=False,cases=report)
    (out/'report.json').write_text(json.dumps(result,indent=2,sort_keys=True)+'\n');print(json.dumps(result,sort_keys=True))
if __name__=='__main__':main()
