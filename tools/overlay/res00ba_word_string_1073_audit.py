#!/usr/bin/env python3
"""Compare the bounded 00BA:1073 formatter against its original START.EXE bytes.

--compile-only builds and binds the real provider without private media.
Normal execution requires the owner START.EXE and Unicorn. No native claim.
"""
import argparse
import ctypes
import random
import struct
import subprocess
import tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from resident_loader_map import unpack
import system_str_res1037_audit as audit

CHECKER_ROWS = ('RES_00BA_1073',)
ROOT = Path(__file__).resolve().parents[2]
SOURCES = ('resident_00ba_word_string_1073_cpu.c', 'system_str_1366_cpu.c',
           'system_strstore_fullstate_0b25.c', 'memory_bus.c', 'ega_hw.c')

class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.POINTER(ctypes.c_uint8)), ('ega', ctypes.c_void_p),
                ('ports', ctypes.c_void_p), ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


def library(directory):
    path = Path(directory) / 'word1073.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), *[str(ROOT / 'src/recovered' / s) for s in SOURCES],
                    '-o', str(path)], check=True)
    f = ctypes.CDLL(str(path)).pr_res_00ba_1073_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(audit.Cpu),
                  ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    audit.check(len(exe) == 65231 and sha256(exe).hexdigest() == audit.EXE_SHA, 'START.EXE provenance')
    image = bytes(unpack(exe)[0]); md = Cs(CS_ARCH_X86, CS_MODE_16); spans = {}
    for name, (start, end, count) in audit.SPANS.items():
        seg = audit.RES_CS if name == 'res' else audit.SYSTEM_CS
        body = image[audit.physical(seg,start):audit.physical(seg,end)]
        ins = list(md.disasm(body,start))
        audit.check(sum(i.size for i in ins) == len(body) and (count is None or len(ins)==count), f'{name} decode')
        conditional = {i.address:(i.address+i.size,int(i.op_str,16)) for i in ins if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
        spans[name] = dict(decoded={i.address:i for i in ins},conditional=conditional)
        if name == 'res':
            audit.check(sha256(body).hexdigest() == '189b8c5536f60fd2c20e4c087348c50648140cce18d772087470e3ec1f11bd4c', '1073 hash')
            audit.check(ins[-1].address == 0x10aa and ins[-1].op_str == '2', '1073 return')
    audit.check(struct.unpack('<10I', image[audit.physical(audit.SYSTEM_CS,audit.TABLE):audit.physical(audit.SYSTEM_CS,audit.TABLE+40)]) == audit.POWERS, 'power table')
    return image, spans


def main():
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--compile-only',action='store_true');args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='poolrad-word1073-') as directory:
        f=library(directory)
        if args.compile_only:
            print('CHECKER_BUILD_OK');return
        audit.SPANS['res']=(0x1073,0x10ad,None);audit.RES_RETF=0x10aa
        audit.SITES={0x108e:b'\x9a\x66\x13',0x10a2:b'\x9a\x25\x0b'}
        image,spans=source();rng=random.Random(0x1073);seen=set();stops={}
        def adapter(ram,n,cpu,callee,status):
            bus=Bus(ram,None,None,None,None)
            return f(ctypes.byref(bus),n,cpu,callee,status)
        values=[0,1,9,10,255,256,999,1000,32767,32768,65535]+[rng.randrange(65536) for _ in range(80)]
        cases=[(f'word-{v}',v,(),None) for v in values]
        cases += [('bad-str',7,((audit.RES_CS,0x108e,0x90),),None),('bad-store',7,((audit.RES_CS,0x10a2,0x90),),None),
                  ('bad-table',7,(),struct.pack('<10I',*audit.POWERS[:9],2))]
        for index,(name,value,patch,table) in enumerate(cases):
            memory,cpu,high=audit.build(image,rng,target='res',words=(value,0x300,0x5000),result=(0x5000,0x300),
                                        flags=audit.FLAGS[index%len(audit.FLAGS)],patch=patch,table=table)
            cpu.ip=0x1073
            stop,visited,_=audit.compare(name,'res',memory,cpu,high,spans,{'res':adapter})
            seen|=visited;stops[stop]=stops.get(stop,0)+1
        body={('res',x) for x in spans['res']['decoded']}
        audit.check(body<=seen,'uncovered 1073 instruction')
        audit.check(stops.get(audit.OK) and stops.get(audit.CODE) and stops.get(audit.CALLEE),'missing stop')
        print(f'1073 Unicorn: {len(cases)} cases; {len(body)}/{len(body)} body starts; CPU14 and full RAM match; stops {stops}')

if __name__=='__main__':
    main()
