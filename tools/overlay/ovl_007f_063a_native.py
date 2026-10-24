#!/usr/bin/env python3
"""Pinned DOSBox-X normal/386 corroboration of completed 063A calls.

Compare CPU14 (including every FLAGS bit) and five 64 KiB RAM regions.
IF is cleared for DOS capture. Divide errors and instruction-budget stops
are covered by the Unicorn checker, not this completion-only harness.
"""
# NOT_A_ROW_CHECKER: imported by ovl_007f_063a_cpu_audit.py --native
import os
import struct
import sys
from hashlib import sha256
from pathlib import Path
import ovl_007f_063a_cpu_audit as C
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT

PINNED = C.ROOT / 'resources/dosbox-x-heavy-src/src/dosbox-x'
PINNED_SHA = 'c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457'

def native(bodies, decoded, lib, quick=False):
    binary = Path(os.environ.get('POOLRAD_NATIVE_DOSBOX', str(PINNED))).resolve()
    C.check(binary.is_file(), 'native DOSBox-X binary missing')
    digest = sha256(binary.read_bytes()).hexdigest()
    C.check(digest == PINNED_SHA, 'native binary does not match pinned receipt')
    old_path = os.environ.get('PATH', '')
    os.environ['PATH'] = str(binary.parent) + os.pathsep + old_path
    try:
        return _native(bodies, lib, binary, digest, quick)
    finally:
        os.environ['PATH'] = old_path

def _native(bodies, lib, binary, digest, quick):
    out = OUT / 'u7f-063a'; out.mkdir(parents=True, exist_ok=True)
    base = find_layout(out)
    code, ds, ss, rec = (base + k * 0x1000 for k in (1,2,3,4))
    segments = [code,ds,ss,rec,base+0x5000,base+0x6000]
    C.check(all(C.physical(s,0)+65536<=0xA0000 for s in segments),'native loaded/captured regions must be below aperture')
    cases = [('shares-remainders',{}),('empty',dict(statuses=(),pools=(0,)*7)),
        ('cap-residual',dict(statuses=(0,),totals=(1599,),pools=(17,0,0,0,0,0,0))),
        ('status-mixed',dict(statuses=(1,0x7F,0x80,0xB3,0xFF,0),pools=(7,8,9,10,11,12,13))),
        ('negative-pools',dict(pools=(0xFFFFFFFF,0x80000000,0,0,0,0,0))),
        ('large-pools',dict(pools=(0x10001,0x7FFFFFFF,0,0,0,0,0))),
        ('above-cap',dict(totals=(1601,0xFFFF),pools=(19,7,0,0,0,0,0))),
        ('mixed-classes',dict(kinds=(0x12,0x1D))),
        ('sp-min',dict(sp=C.SP_MIN)),('sp-max',dict(sp=C.SP_MAX)),
        ('sp-odd',dict(sp=0x7FFF)),('offset-wrap',dict(statuses=(0,),rec_off=0xFF00)),
        ('word-cross-ffff',dict(statuses=(0,),rec_off=0xFF77))]
    cases += [(f'flags-{f:04x}',dict(flags=f)) for f in (2,0xAD7,0xE57)]
    report=[]
    for index,(name,kw) in enumerate(cases):
        directory=out/f'case-{index:02d}';directory.mkdir(exist_ok=True)
        opts=dict(w_cs=code,res_cs=code+0x200,sys_cs=code+0x400,ds=ds,ss=ss,rec_seg=rec,flags=2)
        opts.update(kw); opts['flags']=(opts['flags'] & ~0x200)|2
        state=C.fixture(bodies,**opts);m,c,_=state
        symbols=assemble(directory,assembly(c,segments,None))
        C.put16(m,c.ss,c.sp,symbols['returned']);C.put16(m,c.ss,(c.sp+2)&65535,base)
        status,ram,expected,left,diag=C.run_c(lib,state,100000)
        C.check(status=='RETURNED',f'native {name}: unexpected model stop {status}')
        for i,segment in enumerate(segments):
            at=C.physical(segment,0);(directory/f'I{i}.BIN').write_bytes(m[at:at+65536])
        (directory/'RESULT.BIN').unlink(missing_ok=True);launch(directory)
        result=(directory/'RESULT.BIN').read_bytes()
        C.check(len(result)==28+5*65536,f'native {name}: capture length')
        for (field,_),value in zip(C.Cpu._fields_,struct.unpack_from('<14H',result)):
            C.check(getattr(expected,field)==value,f'native {name}: {field} C={getattr(expected,field):04X} DOSBox={value:04X}')
        for i,segment in enumerate(segments[1:]):
            at=C.physical(segment,0);wanted=ram[at:at+65536];actual=result[28+i*65536:28+(i+1)*65536]
            if wanted!=actual:
                differences=[(hex(j),a,b) for j,(a,b) in enumerate(zip(wanted,actual)) if a!=b]
                raise AssertionError((name,'native RAM',hex(segment),differences[:12],len(differences)))
        report.append(dict(case=name,cpu_words=14,ram_bytes=5*65536,flags_masks=0,result_sha256=sha256(result).hexdigest()))
        print(f'PASS native {name}',file=sys.stderr,flush=True)
    return dict(profile='DOSBox-X normal/386',binary=str(binary),binary_sha256=digest,pinned_build_used=True,
        source_revision='5a9c7c53481c2e933948039c33256776653f4faf',cases=len(report),results=report,
        limitations='Completed calls only; IF cleared; five 64 KiB RAM regions, not all 1 MiB; flat-memory oracle, no EGA aperture claim.')
