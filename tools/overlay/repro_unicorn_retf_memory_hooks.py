#!/usr/bin/env python3
"""Media-free reproduction of RETF interacting with Unicorn memory hooks.

Synthetic code: CB (RETF) at 6000:0639; F4 (HLT) at 6000:0655.
The far return frame at 2000:7FD0 contains 0655,6000. A code hook stops
before the second instruction, so an erroneous second RETF never executes.
Memory callbacks only record accesses; they never read or write CPU state.
No C model, game bytes, library compilation, or DOSBox is involved.

Run: python3 tools/overlay/repro_unicorn_retf_memory_hooks.py
A fixed Unicorn may report normal_return for every mode. Unexpected states
or a failure of the no-hook/aperture-only controls produce a nonzero exit.
"""
import json
import unicorn
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_SS, UC_X86_REG_SP

def run(mode):
    uc=Uc(UC_ARCH_X86,UC_MODE_16)
    uc.mem_map(0,1<<20)
    uc.mem_write(0x60639,b'\xcb')
    uc.mem_write(0x60655,b'\xf4')
    uc.mem_write(0x27FD0,b'\x55\x06\x00\x60')
    for reg,value in ((UC_X86_REG_CS,0x6000),(UC_X86_REG_IP,0x0639),
                      (UC_X86_REG_SS,0x2000),(UC_X86_REG_SP,0x7FD0)):
        uc.reg_write(reg,value)
    trace=[];accesses=[]
    def code(em,address,size,user):
        trace.append(dict(address=address,cs=em.reg_read(UC_X86_REG_CS),
                          ip=em.reg_read(UC_X86_REG_IP),sp=em.reg_read(UC_X86_REG_SP)))
        if len(trace)==2:em.emu_stop()
    def memory(em,access,address,size,value,user):
        accesses.append(dict(access=access,address=address,size=size))
    uc.hook_add(UC_HOOK_CODE,code)
    if mode!='none':
        mask=UC_HOOK_MEM_WRITE if mode=='full_write' else UC_HOOK_MEM_READ
        if mode=='full_read_write':mask|=UC_HOOK_MEM_WRITE
        if mode=='aperture_read':uc.hook_add(mask,memory,begin=0x9FFF0,end=0xBFFFF)
        else:uc.hook_add(mask,memory)
    uc.emu_start(0x60639,1<<20,count=2)
    end=trace[-1] if trace else {}
    classification='unexpected'
    if len(trace)==2 and end.get('cs')==0x6000 and end.get('sp')==0x7FD4:
        if end['ip']==0x0655:classification='normal_return'
        elif end['ip']==0x0639:classification='repeated_retf'
    return dict(mode=mode,classification=classification,trace=trace,memory_accesses=accesses)

def main():
    results=[run(mode) for mode in ('none','full_read','full_write','full_read_write','aperture_read')]
    report=dict(unicorn_version=unicorn.__version__,synthetic_code_hex='cb',
                expected_return=dict(cs=0x6000,ip=0x0655,sp=0x7FD4),results=results)
    print(json.dumps(report,indent=2))
    return int(any(r['classification']=='unexpected' or
                   (r['mode'] in ('none','aperture_read') and r['classification']!='normal_return')
                   for r in results))

if __name__=='__main__':raise SystemExit(main())
