#!/usr/bin/env python3
"""Small original-byte/full-state audit; private originals are never emitted."""
import ctypes as C
from hashlib import sha256
from pathlib import Path
import sys

FIELDS = "ax bx cx dx si di bp sp cs ds es ss fs gs ip flags".split()
SIZE = 1 << 20


class Cpu(C.Structure):
    _fields_ = [(name, C.c_uint16) for name in FIELDS]


class State(C.Structure):
    _fields_ = [("cpu", Cpu), ("ram", C.POINTER(C.c_uint8)), ("ram_size", C.c_size_t)]


def checked(path, size, digest):
    data = Path(path).read_bytes()
    if len(data) != size or sha256(data).hexdigest() != digest:
        raise ValueError("original identity mismatch")
    return data


def spans(game, start):
    prefix = bytearray(game[0x1e1a9:0x1e1c9])
    b0b = start[0xac76+0xb0b:0xac76+0xb25]
    b98 = start[0xac76+0xb98:0xac76+0xbc4]
    expected = ["31a9993b6b58cee6e88c2ac0796d3b5dad94454e2f33a8e231bea06b439ae108",
                "4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f",
                "c51c23b93b560cbbd48e3002e63b6bee1d24387a95f33915616ec64bfcebbb64"]
    for data, digest in zip((prefix,b0b,b98),expected):
        if sha256(data).hexdigest() != digest: raise ValueError("source span mismatch")
    # Two actual far-CALL segment fixups; no branch or instruction replacement.
    for pc in (0x1d4,0x1de):
        p=pc-0x1c3+3
        if int.from_bytes(prefix[p:p+2],"little") != 0xaf8: raise ValueError("CALL fixup mismatch")
        prefix[p:p+2]=(0x131c).to_bytes(2,"little")
    return prefix,b0b,b98


def case(code,n,m,ds=0x1000):
    ram=bytearray([0x5a])*SIZE
    cpu=Cpu(0xa55a,0x1234,0x4567,0x789a,0x3333,0x4444,0x8000,0x7cfa,
            0x2a8e,ds,0x1100,0x2000,0x3456,0x789a,0x1c3,0x7606)
    p=(ds<<4)+0x537a;ram[p]=n;ram[p+1:p+n+1]=b'A'*n
    p=(cpu.cs<<4)+0x197;ram[p]=m;ram[p+1:p+m+1]=b'B'*m
    for (seg,off),data in zip(((0x2a8e,0x1c3),(0x131c,0xb0b),(0x131c,0xb98)),code):
        p=(seg<<4)+off;ram[p:p+len(data)]=data
    return ram,cpu


def compare(library,memory,cpu):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as regs
    mapping={name:getattr(regs,"UC_X86_REG_"+("EFLAGS" if name=="flags" else name.upper())) for name in FIELDS}
    original=Uc(UC_ARCH_X86,UC_MODE_16);original.mem_map(0,SIZE)
    original.mem_write(0,bytes(memory))
    for name in FIELDS: original.reg_write(mapping[name],getattr(cpu,name))
    reached=[False]
    def stop(emu,address,size,user):
        if address==(0x2a8e<<4)+0x1e3:
            reached[0]=True;emu.emu_stop()
    original.hook_add(UC_HOOK_CODE,stop)
    original.emu_start((0x2a8e<<4)+0x1c3,SIZE,count=4096)
    if not reached[0]: raise ValueError("original prefix did not reach04F7 boundary")
    ram=(C.c_uint8*SIZE).from_buffer_copy(memory)
    state=State(Cpu.from_buffer_copy(bytes(cpu)),ram,SIZE)
    for step in range(128):
        status=library.pr_u6f_01c3_text_record_step(C.byref(state))
        if status==1: break
        if status!=0: raise ValueError("C prefix unexpectedly blocked/transferred")
    else: raise ValueError("C test observation budget exceeded")
    if bytes(ram)!=bytes(original.mem_read(0,SIZE)): raise ValueError("full1MiB RAM mismatch")
    for name in FIELDS:
        expected=original.reg_read(mapping[name])&0xffff
        if getattr(state.cpu,name)!=expected:
            raise ValueError("CPU16 mismatch: "+name)


def main():
    if len(sys.argv)!=4: raise SystemExit("usage: audit.py GAME.OVR START.EXE C_LIBRARY")
    game=checked(sys.argv[1],206105,"872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614")
    start=checked(sys.argv[2],65231,"d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd")
    code=spans(game,start);lib=C.CDLL(sys.argv[3])
    lib.pr_u6f_01c3_text_record_step.argtypes=[C.POINTER(State)]
    lib.pr_u6f_01c3_text_record_step.restype=C.c_int
    count=0
    for n,m in [(0,0),(1,1),(3,2),(127,31),(128,8),(250,31),(254,2),(255,1)]:
        memory,cpu=case(code,n,m);compare(lib,memory,cpu);count+=1
    for ds in (0x22a7,0x22a8,0x22a9):
        memory,cpu=case(code,32,3,ds);compare(lib,memory,cpu);count+=1
    print(f"original32B14 + B0B/B98 audit PASS: {count} bounded cases, CPU16/full1MiB RAM")
    print("Source call segments relocated only; synthetic inputs, no native/whole/strict credit.")


if __name__=="__main__": main()
