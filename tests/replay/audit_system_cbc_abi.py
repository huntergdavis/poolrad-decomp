#!/usr/bin/env python3
"""Complete SYSTEM CBC C/original/native parity, including signed counts."""
import ctypes as ct
from pathlib import Path
from types import SimpleNamespace
import sys
import unicorn
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/replay'))
import audit_system_b98_abi as b98
import audit_slot_a_delete_abi_map as mapped
from audit_slot_a_string_0356_abi import CPU,State,Report,NAMES,cpu_values,compare_cpu


def unified(lib,cpus,images):
    image=images['196a-controlled'];v=cpus['196a-controlled']
    memory=ct.create_string_buffer(image,len(image))
    state=State(CPU(**dict(zip(NAMES,v))),ct.cast(memory,ct.POINTER(ct.c_uint8)),len(image))
    report=Report();uc=mapped.emulator(image,v)
    pending=list(mapped.RAM_HASHES)[2:];seen=0
    for steps in range(5000):
        pc=(state.cpu.cs,state.cpu.ip)
        if pc==(0x1026,0x401):
            assert not pending
            print(f'unified resident +196A through Delete +0401: {steps} C instructions; {seen} post-controlled-entry native CPU/full-RAM checkpoints')
            return
        if pc==(0x1026,0x3FC):
            before_cpu,before_ram=cpu_values(state.cpu),memory.raw
            assert lib.pr_slot_a_string_0356_prefix_step(ct.byref(state),ct.byref(report))==2
            assert cpu_values(state.cpu)==before_cpu and memory.raw==before_ram
            assert (report.callee_segment,report.callee_pc,report.return_pc)==(0xAF8,0xCBC,0x401)
        assert lib.pr_slot_a_string_0356_step(ct.byref(state),ct.byref(report))==0,(pc,'normal adapter blocked')
        assert not report.dependency_valid
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu),mapped.values(uc),f'unified after {pc}')
        assert memory.raw==bytes(uc.mem_read(0,len(image))),pc
        if pending and cpu_values(state.cpu)[:2]==cpus[pending[0]][:2]:
            label=pending.pop(0)
            compare_cpu(cpu_values(state.cpu),cpus[label],label)
            assert memory.raw==images[label],label
            seen+=1
    raise AssertionError('unified CBC trace bound exhausted')


def main():
    if len(sys.argv)!=3:raise SystemExit('usage: audit_system_cbc_abi.py LIBRARY DELETE_NATIVE_ROOT')
    if unicorn.__version__!='2.1.4' or not __debug__:raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library,root=(Path(a).resolve(strict=True) for a in sys.argv[1:])
    if any(not p.is_relative_to((ROOT/'resources').resolve()) for p in (library,root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus,images=mapped.inputs(root)
    for image in images.values():mapped.code(root,image)
    lib=ct.CDLL(str(library))
    lib.pr_system_cbc_step.argtypes=[ct.POINTER(State)];lib.pr_system_cbc_step.restype=ct.c_int
    for name in ('pr_slot_a_string_0356_step','pr_slot_a_string_0356_prefix_step'):
        fn=getattr(lib,name);fn.argtypes=[ct.POINTER(State),ct.POINTER(Report)];fn.restype=ct.c_int
    unified(lib,cpus,images)
    view=SimpleNamespace(pr_system_b98_step=lib.pr_system_cbc_step)
    checkpoints=[(n,cpus[n],images[n]) for n in list(mapped.RAM_HASHES)[3:]]
    b98.differential(view,'complete controlled Delete C body',images['03fc'],cpus['03fc'],(0x1026,0x401),checkpoints=checkpoints)
    for name,count,index,length,alias in (
        ('count-zero',0,1,3,''),('count-minus-one',0xFFFF,1,3,''),('count-min-word',0x8000,1,3,''),
        ('middle-delete',1,2,3,''),('suffix-truncate',9,2,5,''),('index-past-end',1,10,3,''),
        ('index-zero',1,0,3,''),('index-signed-boundary',1,0x8000,3,''),('index-add-wrap',1,0xFFFF,3,''),
        ('delete-max-string',255,1,255,''),('last-max-character',1,255,255,''),
        ('source-prefix-alias',1,1,3,'prefix'),('source-suffix-alias',1,1,3,'suffix'),
    ):
        image=bytearray(images['0cbc']);v=cpus['0cbc'].copy();ss=v[6];sp=v[7]
        base=ss<<4
        source=0x3390 if alias=='prefix' else (0x3290 if alias=='suffix' else 0x36A2)
        if count&0x8000 or count==0:source=0xFFFF;source_seg=0xFFFF
        else:source_seg=ss;image[base+source:base+source+1+length]=bytes([length])+bytes(65+i%26 for i in range(length))
        for offset,value in ((4,count),(6,index),(8,source),(10,source_seg)):
            image[base+sp+offset:base+sp+offset+2]=value.to_bytes(2,'little')
        steps=b98.differential(view,'synthetic-CBC-'+name,bytes(image),v,(0x1026,0x401))
        if count&0x8000 or count==0:assert steps==8
    print('CBC complete-body gate passed: B57/B98/B25 compose; no identity/count credit')


if __name__=='__main__':main()
