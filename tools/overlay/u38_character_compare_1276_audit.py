#!/usr/bin/env python3
"""Original-byte full-state parity for raw-CPU U38 +1276, composing the SYSTEM string
compare 0AF8:0C10.

The differential loads unit 0x38 at its original offsets and the original START.EXE SYSTEM bytes 0AF8:0C10..0C3A into
one 1 MiB image, and compares the entire RAM and all 14 CPU words between the original under Unicorn and the C model,
with no mask. The original decides each stop: the fetch after its RETF 0Ah (return), the LCALL fetched with other bytes
(CODE), or a changed direct return (TRANSFER). Any fetch outside decoded instruction starts is an error. Fixtures include
separate records, live data/stack aliases, wrapping operands, and independently relocated comparator bodies. Loaded code stays immutable. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CALLER_CS, DS, SS = 0x3000, 0x1000, 0x2000
A_SEG, B_SEG, P_SEG = 0x4000, 0x5000, 0x6000
SYSTEM_CS, STRCMP, STRCMP_END = 0x0AF8, 0x0C10, 0x0C3B
ROUTINES = {"1276": (0x1276, 0, 0)}
RET_CS, RET_IP = 0x9001, 0x7000
ARGS = (0x1111, 0x2222, 0x3333)
OK, CODE, CALLEE, TRANSFER = 0, 2, 3, 4
NAMES = {OK: "return", CODE: "CODE", CALLEE: "CALLEE", TRANSFER: "TRANSFER"}
FLAGS = (0x0602, 0x0202, 0x0ED7, 0x0647, 0x72D7, 0x0403, 0x0213)


def check(condition, message):
    if not condition:
        raise ValueError(message)


def select(name):
    """Select the sole recovered caller."""
    global ROUTINE, ENTRY, END, BODY_RETF, SITE, LOOP_JNE, LOOP_HEAD, TABLE, LAST
    ROUTINE = name
    ENTRY, TABLE, LAST = ROUTINES[name]
    END, BODY_RETF, SITE, LOOP_JNE, LOOP_HEAD = 0x12B2, 0x12AF, 0x1290, 0, 0


select("1276")


def source():
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, "media provenance differs")
    u = next(u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr) if u["stub_para"] == 0x38)
    code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
    insns = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[ENTRY:END], ENTRY))
    calls = [(i.address, i.op_str) for i in insns if "call" in i.mnemonic]
    check(len(insns) == 24 and sum(i.size for i in insns) == END - ENTRY and insns[-1].address == BODY_RETF
          and (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "0xa") and calls == [(0x1290, "0xaf8, 0xc10"),(0x12A2, "0xaf8, 0xc10")]
          and all(a+3 in set(u["fixups"]) for a in (0x1290,0x12A2)), f"U38 +{ROUTINE} differs: {calls}")
    check(u['entries'][40]==ENTRY and min(v for v in u['entries'] if v>ENTRY)==0x12B2,'entry/next bound differs')
    check(sha256(code[ENTRY:END]).hexdigest()=='0598c5bb03bf995e3aeba41c11d749c629fa65cd273bd6a00892bf9a5eaca01a','caller hash differs')
    check(sha256(code[0x1268:ENTRY]).hexdigest()=='eb19eb8681d04b84bfe569f341ba3b2de215ff19ee7c51282198074a48c57ae4','code strings differ')
    expanded = bytes(unpack(exe)[0])
    system = expanded[physical(SYSTEM_CS, STRCMP):physical(SYSTEM_CS, STRCMP_END)]
    leaf = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(system, STRCMP))
    check(len(leaf) == 20 and sum(i.size for i in leaf) == len(system) and (leaf[-1].mnemonic, leaf[-1].op_str) == ("retf", "8"),
          "SYSTEM string compare differs")
    check(sha256(system).hexdigest()=='11e2860118cb471f9f1ecb1d0c44c060e0fbb83862f69c1cf39b5e0484298a58','SYSTEM hash differs')
    conditional = {i.address: (i.address + i.size, int(i.op_str, 16)) for i in insns if i.mnemonic.startswith("j") and i.mnemonic != "jmp"}
    sha = sha256(code[ENTRY:END]).hexdigest()
    print(f"U38 +{ROUTINE} (60/24, LCALL 0AF8:0C10, RETF 0Ah) GAME file {u['ovr_offset'] + ENTRY:#x} sha {sha}; "
          f"SYSTEM 0C10..0C3A sha {sha256(system).hexdigest()[:16]}...")
    return dict(leaf_starts=[i.address for i in leaf],leaf_conditional={i.address:(i.address+i.size,int(i.op_str,16)) for i in leaf if i.mnemonic.startswith('j')},conditional=conditional, starts=[i.address for i in insns], sha=sha, file_offset=u["ovr_offset"] + ENTRY), code, system


def library(directory):
    path = directory / "u38match.so"
    sources = ["overlay_u38_character_compare_1276_cpu.c", "system_string_move_leaves_cpu.c"]
    built = subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-O2", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                            *[str(ROOT / "src/recovered" / s) for s in sources], "-o", str(path)], capture_output=True, text=True)
    check(built.returncode == 0, built.stderr[-2000:])
    function = getattr(ctypes.CDLL(str(path)), f"pr_u38_character_compare_1276_cpu")
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_int)]
    function.restype = ctypes.c_int
    return function


def private_name(code,mode):
    first=bytes(code[0x1268:0x1268+1+code[0x1268]])
    second=bytes(code[0x1270:0x1270+1+code[0x1270]])
    if mode=='first':return first
    if mode=='second':return second
    if mode=='empty':return b'\x00'
    if mode=='prefix':return bytes([first[0]-1])+first[1:-1]
    if mode=='extension':return bytes([first[0]+1])+first[1:]+b'\xA5'
    if mode=='long':return bytes([255])+bytes(range(255))
    return first[:1]+bytes([first[1]^0xFF])+first[2:]


def build(code,system,rng,*,active=(P_SEG,0x300),marker=1,mode='first',name=None,
          count=0,sp=0x8000,ss=SS,flags=0x7202,loaded=(SYSTEM_CS,SYSTEM_CS),patch=(),background=None,writes=()):
    memory=bytearray([0xA5])*SIZE if background is None else bytearray(background)
    start=physical(CALLER_CS,0);memory[start:start+len(code)]=code
    for site,seg in zip((0x1290,0x12A2),loaded):
        at=physical(seg,STRCMP);memory[at:at+len(system)]=system
        put16(memory,CALLER_CS,site+3,seg)
    put16(memory,DS,0x5D92,active[1]);put16(memory,DS,0x5D94,active[0])
    name=private_name(code,mode) if name is None else name
    for i,v in enumerate(name):memory[physical(active[0],(active[1]+i)&65535)]=v
    memory[physical(active[0],(active[1]+0x9F)&65535)]=marker
    memory[physical(DS,0x6822)]=count
    for i,v in enumerate((RET_IP,RET_CS,*ARGS,0x4444,0x5555)):put16(memory,ss,(sp+2*i)&65535,v)
    for off,value in patch:memory[physical(CALLER_CS,off)]=value
    for seg,off,blob in writes:
        for i,v in enumerate(blob):memory[physical(seg,(off+i)&65535)]=v
    cpu=Cpu(*(rng.randrange(65536) for _ in range(7)),sp,CALLER_CS,ENTRY,DS,rng.randrange(65536),ss,flags)
    return memory,cpu,ss>=0xF000


def compare(label,memory,cpu,high,target,function):
    from unicorn import Uc,UC_ARCH_X86,UC_MODE_16,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS,UC_X86_REG_IP
    uc=Uc(UC_ARCH_X86,UC_MODE_16);uc.mem_map(0,2*SIZE)
    uc.mem_write(0,bytes(memory));uc.mem_write(SIZE,bytes(memory))
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):uc.reg_write(reg,getattr(cpu,name))
    segments={int.from_bytes(memory[physical(CALLER_CS,s+3):physical(CALLER_CS,s+3)+2],'little') for s in (0x1290,0x12A2)}
    state=dict(stop=None,returned=False,back=None,previous=None,calls=[])
    visited,edges=set(),set()
    def halt(status):state['stop']=status;uc.emu_stop()
    def hook(em,address,size,user):
        del em,size,user
        cs,ip=uc.reg_read(UC_X86_REG_CS),uc.reg_read(UC_X86_REG_IP)
        if state['returned']:return halt(OK)
        if state['back'] is not None:
            back=state['back'];state['back']=None
            if (cs,ip)!=(CALLER_CS,back):return halt(TRANSFER)
        kind='caller' if cs==CALLER_CS else 'leaf' if cs in segments else None
        starts=target['starts'] if kind=='caller' else target['leaf_starts'] if kind=='leaf' else ()
        check(ip in starts,f'unexpected original fetch {cs:04x}:{ip:04x}: {label}')
        key=(kind,ip)
        if kind=='caller' and ip in (0x1290,0x12A2):
            if bytes(uc.mem_read(address,3))!=bytes([0x9A,0x10,0x0C]):return halt(CODE)
            state['calls'].append(ip)
        visited.add(key)
        previous=state['previous']
        if previous:
            cond=target['conditional'] if previous[0]=='caller' else target['leaf_conditional']
            if previous[1] in cond:edges.add((previous,key))
        if kind=='caller' and ip==BODY_RETF:state['returned']=True
        elif kind=='leaf' and ip==0xC38:state['back']=state['calls'][-1]+5
        state['previous']=key
    def mirror(em,access,address,size,value,user):
        del access,user
        for j in range(size):
            raw=address+j;at=raw&(SIZE-1);em.mem_write(at+SIZE if raw<SIZE else at,bytes([(value>>(8*j))&255]))
    uc.hook_add(UC_HOOK_CODE,hook)
    if high:uc.hook_add(UC_HOOK_MEM_WRITE,mirror)
    uc.emu_start(physical(CALLER_CS,ENTRY),2*SIZE,count=200000)
    check(state['stop'] is not None,f'original timeout {label}')
    ram=(ctypes.c_uint8*SIZE).from_buffer_copy(memory);callee=ctypes.c_int(99)
    status=function(ram,SIZE,ctypes.byref(cpu),ctypes.byref(callee))
    check(status==state['stop'],f'status {status}/{state["stop"]}: {label}')
    want=bytes(uc.mem_read(0,SIZE));actual=bytes(ram)
    if actual!=want:
        diff=[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b]
        raise ValueError(f'RAM {label}: {len(diff)} differences {diff[:8]}')
    for (name,_),reg in zip(Cpu._fields_,REGISTERS):check(getattr(cpu,name)==(uc.reg_read(reg)&65535),f'{name} mismatch {label}: C={getattr(cpu,name):04x} original={uc.reg_read(reg)&65535:04x}')
    check(callee.value==(0 if state['calls'] else -1),f'callee status {label}')
    return status,visited,edges,tuple(state['calls'])


def cases(rng):
    modes=('first','second','mismatch','empty','prefix','extension','long')
    spec=[]
    for i in range(256):
        spec.append((f'count-{i}',dict(count=i,mode=modes[i%len(modes)],flags=FLAGS[i%len(FLAGS)])))
        spec.append((f'marker-{i}',dict(marker=i,mode='second',count=i)))
    for mode in modes:spec.append((mode,dict(mode=mode)))
    for sp in (0xE,0xF,0x8001,0xFFF1,0xFFF2):spec.append((f'sp-{sp:x}',dict(sp=sp,mode='second')))
    spec += [('relocated-first',dict(loaded=(0x7400,SYSTEM_CS))),('relocated-both',dict(mode='second',loaded=(0x7400,0x7500))),
        ('ss-ds',dict(ss=DS,mode='second')),('a20',dict(ss=0xFFFF,mode='second')),
        ('wrapped-name',dict(active=(P_SEG,0xFFFC),mode='first')),
        ('pointer-changes-before-second',dict(ss=DS,sp=0x5DA0,mode='second')),
        ('saved-bp-pointer-alias',dict(ss=DS,sp=0x5D94,mode='second')),
        ('result-return-alias',dict(ss=DS,sp=0x6822,mode='first')),
        ('result-saved-bp-alias',dict(ss=DS,sp=0x6824,mode='second')),
        ('name-call-frame-alias',dict(active=(SS,0x7FF6),mode='second')),
        ('random-background',dict(background=rng.randbytes(SIZE),mode='second')),
        ('unreached-first-code',dict(marker=0,patch=((0x1290,0x90),))),
        ('unreached-second-code',dict(mode='first',patch=((0x12A2,0x90),)))]
    for site in (0x1290,0x12A2):
        for delta in (0,1,2):spec.append((f'code-{site:x}-{delta}',dict(mode='second',patch=((site+delta,0x90),))))
    return spec


def differential(target,code,system,function):
    rng=random.Random(0x381276);visited,edges=set(),set();stops={};paths={}
    for label,options in cases(rng):
        m,c,h=build(code,system,rng,**options);status,v,e,calls=compare(label,m,c,h,target,function)
        visited|=v;edges|=e;stops[NAMES[status]]=stops.get(NAMES[status],0)+1
        path=','.join(f'{s:04X}' for s in calls) or 'no call';paths[path]=paths.get(path,0)+1
        if not label.startswith(('count-','marker-')):print(label,NAMES[status],path,flush=True)
    wanted={('caller',i) for i in target['starts']}|{('leaf',i) for i in target['leaf_starts']}
    outcomes={((kind,a),(kind,b)) for kind,cond in (('caller',target['conditional']),('leaf',target['leaf_conditional'])) for a,outs in cond.items() for b in outs}
    check(wanted<=visited and outcomes<=edges,f'coverage incomplete {wanted-visited} {outcomes-edges}')
    report=dict(cases=sum(stops.values()),stops=stops,paths=paths,caller_starts=len(target['starts']),caller_outcomes=2*len(target['conditional']),
        chain_starts=len(visited),chain_outcomes=len(outcomes),cpu_words=14,ram_bytes=SIZE)
    print('Original-chain:',report,flush=True);return report


# ---- DOSBox-X synthetic oracle ---------------------------------------------------------------------------
# The LCALL names paragraph 0AF8 directly: with the COM at segment 0814, 0AF8:0C10 is COM offset 3A50h, so the SYSTEM
# bytes are placed there in the COM image and nothing else may use it. The routine span is copied to its own offsets.
CALLER_N, DATA_N = 0x8800, 0x5000
MAIN_AT, SYSTEM_AT, UNIT_AT, CASES_AT = 0x0300, 0x3A50, 0x3B00, 0x3C00
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xD800, 0xF000
WINDOW_LOW, WINDOW_SIZE = STACK_TOP - 0x100, 0x100
NATIVE_CASES, CASE_SIZE = 64, 0x180
RECORD_BYTES = 28 + WINDOW_SIZE + 0x10000
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
ENTRY_REGS = dict(ax=0x1234, bx=0x3456, cx=0x5678, dx=0x789A, si=0x9ABC, di=0xBCDE, bp=0x7777)


def native_world(rng,code,index):
    data=bytearray([0xA5])*0x10000
    def w16(o,v):data[o]=v&255;data[o+1]=v>>8
    active=0xA000+(index*17)%0xF00
    w16(0x5D92,active);w16(0x5D94,DATA_N)
    mode=('first','second','mismatch','empty','prefix','extension','long')[index%7]
    name=private_name(code,mode)
    # Long strings exceed the sparse native-case encoding; original cases cover them.
    if mode=='long':name=private_name(code,'second')
    data[active:active+len(name)]=name;data[active+0x9F]=0 if index%11==0 else 1
    data[0x6822]=(index*19)&255
    return data,0


def native_memory(span, system, data):
    memory = bytearray(SIZE)
    at = physical(CALLER_N, 0x1268); memory[at:at + len(span)] = span
    at = physical(SYSTEM_CS, STRCMP); memory[at:at + len(system)] = system
    at = physical(DATA_N, 0); memory[at:at + 0x10000] = data
    return memory


def native_frame(memory, segment, returned, a):
    sp = STACK_TOP
    for value in (DATA_N, a, *reversed(ARGS), segment, returned):
        sp -= 2; put16(memory, segment, sp, value)
    return sp


def native_audit(code, system, function):
    import os
    directory = ROOT / f"resources/oracle-runs/u38-{ROUTINE}-synthetic"
    directory.mkdir(parents=True, exist_ok=True)
    span = bytes(code[0x1268:END])
    check(physical(0x0814, SYSTEM_AT) == physical(SYSTEM_CS, STRCMP), "the SYSTEM hole is not at 0AF8:0C10")
    rng = random.Random(int("38" + ROUTINE + "D", 16))
    cases = []
    while len(cases) < NATIVE_CASES:
        (data, a), flags = native_world(rng,code,len(cases)), len(cases) % len(NATIVE_FLAGS)
        writes = [(o, data[o] | (data[o + 1] << 8)) for o in range(0, 0x10000, 2) if data[o] != 0xA5 or data[o + 1] != 0xA5]
        check(6 + 4 * len(writes) <= CASE_SIZE, f"native case needs {len(writes)} writes")
        cases.append(dict(flags=flags, data=data, a=a, writes=writes))
    table = b"".join((struct.pack("<BBHH", c["flags"], 0, len(c["writes"]), c["a"]) + b"".join(struct.pack("<HH", o, v) for o, v in c["writes"]))
                     .ljust(CASE_SIZE, b"\0") for c in cases)
    check(MAIN_AT < SYSTEM_AT and SYSTEM_AT + len(system) <= UNIT_AT and UNIT_AT + len(span) <= CASES_AT
          and CASES_AT + len(table) <= OUTPUT, "COM layout overlaps")
    t = [".code16", ".intel_syntax noprefix", ".global _start", "_start:", "    jmp main", f".org {MAIN_AT - 0x100:#x}", "main:",
         "    mov ax,cs", "    mov ds,ax", "    mov es,ax", "    mov ss,ax", f"    mov sp,{STACK_TOP:#x}", "    cld",
         f"    mov ax,{CALLER_N:#x}", "    mov es,ax", "    mov si,offset unit", "    mov di,0x1268", f"    mov cx,{len(span):#x}", "    rep movsb",
         "    push cs", "    pop es", f"    mov word ptr [{HEADER:#x}],cs", f"    mov word ptr [{HEADER + 2:#x}],offset returned",
         f"    mov word ptr [{HEADER + 4:#x}],{len(cases)}", f"    mov word ptr [{HEADER + 6:#x}],0",
         "    mov dx,offset filename", "    xor cx,cx", "    mov ah,0x3c", "    int 0x21", "    jc failure",
         "    mov word ptr [handle],ax", "    mov bx,ax", f"    mov dx,{HEADER:#x}", "    mov cx,8", "    mov ah,0x40", "    int 0x21", "    jc failure",
         "again:", "    cli", f"    mov sp,{STACK_TOP:#x}", "    cld", "    push cs", "    pop ds", "    push cs", "    pop es",
         "    mov al,0xa5", f"    mov di,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep stosb",
         "    mov ax,word ptr [index]", f"    mov bx,{CASE_SIZE}", "    mul bx", "    add ax,offset cases", "    mov bx,ax",
         f"    mov ax,{DATA_N:#x}", "    mov es,ax", "    xor di,di", "    mov cx,0x8000", "    mov ax,0xa5a5", "    rep stosw",
         "    mov cx,word ptr [bx+2]", "    jcxz written", "    lea si,[bx+6]", "write_loop:", "    mov di,word ptr [si]", "    mov ax,word ptr [si+2]",
         "    mov word ptr es:[di],ax", "    add si,4", "    loop write_loop", "written:",
         f"    mov ax,{DATA_N:#x}", "    push ax", "    push word ptr [bx+4]"]
    for value in reversed(ARGS):
        t += [f"    mov ax,{value:#x}", "    push ax"]
    t += ["    xor ah,ah", "    mov al,byte ptr [bx]", "    shl ax,1", "    mov si,ax",
          "    push word ptr [flags_table+si]", "    push cs", "    pop es", f"    mov ax,{DATA_N:#x}", "    mov ds,ax"]
    for reg, value in ENTRY_REGS.items():
        t.append(f"    mov {reg},{value:#x}")
    t += ["    popf", "    call dword ptr cs:[target_far]", "returned:"]
    for i, reg in enumerate("ax bx cx dx si di bp sp cs".split()):
        t.append(f"    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}")
    t += [f"    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned", f"    mov word ptr cs:[{OUTPUT + 20:#x}],ds",
          f"    mov word ptr cs:[{OUTPUT + 22:#x}],es", f"    mov word ptr cs:[{OUTPUT + 24:#x}],ss",
          f"    mov sp,{STACK_TOP + 0x800:#x}", "    pushf", "    pop ax", f"    mov word ptr cs:[{OUTPUT + 26:#x}],ax",
          "    cld", "    push cs", "    pop ds", "    push cs", "    pop es", f"    mov di,{OUTPUT + 28:#x}",
          f"    mov si,{WINDOW_LOW:#x}", f"    mov cx,{WINDOW_SIZE:#x}", "    rep movsb", "    sti",
          "    mov bx,word ptr [handle]", f"    mov dx,{OUTPUT:#x}", f"    mov cx,{28 + WINDOW_SIZE}", "    mov ah,0x40", "    int 0x21", "    jc failure",
          f"    mov ax,{DATA_N:#x}", "    mov ds,ax", "    xor dx,dx", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure",
          "    mov dx,0x8000", "    mov cx,0x8000", "    mov ah,0x40", "    int 0x21", "    jc failure", "    push cs", "    pop ds",
          "    inc word ptr [index]", "    mov ax,word ptr [index]", f"    cmp ax,{len(cases)}", "    jb again",
          "    mov bx,word ptr [handle]", "    mov ah,0x3e", "    int 0x21", "    mov ax,0x4c00", "    int 0x21",
          "failure:", "    mov ax,0x4c01", "    int 0x21",
          'filename: .asciz "RESULT.BIN"', "handle: .word 0", "index: .word 0", f"target_far: .word {ENTRY:#x}, {CALLER_N:#x}",
          "flags_table: .word " + ",".join(f"{f:#x}" for f in NATIVE_FLAGS)]
    for label, blob, where in (("system", system, SYSTEM_AT), ("unit", span, UNIT_AT), ("cases", table, CASES_AT)):
        t += [f".org {where - 0x100:#x}", f"{label}:"]
        for i in range(0, len(blob), 16):
            t.append("    .byte " + ",".join(f"{x:#x}" for x in blob[i:i + 16]))
    asm, obj = directory / "oracle.S", directory / "oracle.o"
    asm.write_text("\n".join(t) + "\n")
    subprocess.run(["as", "--32", str(asm), "-o", str(obj)], check=True)
    subprocess.run(["ld", "-m", "elf_i386", "-Ttext", "0x100", "--oformat", "binary", str(obj), "-o", str(directory / "ORACLE.COM")], check=True)
    config = directory / "dosbox.conf"
    config.write_text("[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n"
                      "[mixer]\nnosound=true\n[midi]\nmididevice=none\n"
                      "[autoexec]\nmount c " + str(directory.resolve()) + "\nc:\nORACLE.COM\nexit\n")
    result_path = directory / "RESULT.BIN"
    if result_path.exists():
        result_path.unlink()
    environment = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    with (directory / "dosbox.log").open("w") as log:
        subprocess.run(["dosbox-x", "-conf", str(config), "-fastlaunch", "-exit", "-noconsole"],
                       env=environment, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(cases) * RECORD_BYTES, f"native output incomplete: {len(result)} bytes")
    segment, returned, count, _ = struct.unpack_from("<4H", result)
    check(count == len(cases) and segment == 0x0814, f"native header differs or COM segment {segment:04x} is not 0814")
    matched = 0
    for n, case in enumerate(cases):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = native_memory(span, system, case["data"])
        for i in range(WINDOW_SIZE):
            memory[physical(segment, WINDOW_LOW + i)] = 0xA5
        sp = native_frame(memory, segment, returned, case["a"])
        cpu = Cpu(*ENTRY_REGS.values(), sp, CALLER_N, ENTRY, DATA_N, segment, segment, NATIVE_FLAGS[case["flags"]])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        check(function(ram, SIZE, ctypes.byref(cpu), None) == OK, f"C status on native case {n}")
        for (reg, _), want in zip(Cpu._fields_, struct.unpack_from("<14H", record)):
            check(getattr(cpu, reg) == want, f"native case {n} {reg}: C={getattr(cpu, reg):04x}, DOS={want:04x}")
        got = bytes(ram[physical(segment, WINDOW_LOW):physical(segment, WINDOW_LOW) + WINDOW_SIZE]) + bytes(ram[physical(DATA_N, 0):physical(DATA_N, 0) + 0x10000])
        if got != record[28:]:
            differences = [i for i, (x, y) in enumerate(zip(got, record[28:])) if x != y]
            raise ValueError(f"native case {n} bytes differ at {[hex(d) for d in differences[:8]]} ({len(differences)} bytes)")
        matched += ram[physical(DATA_N, 0x6822)] != case["data"][0x6822]
    digest = sha256(result).hexdigest()
    print(f"DOSBox-X normal-core/386 synthetic cases passed: {len(cases)} ({matched} with a match); all 14 CPU words, the stack window and the "
          f"whole 64 KiB data segment; no mask")
    print(f"native result SHA-256 {digest}; COM segment {segment:04x}; SYSTEM at 0AF8:0C10 (COM 3A50h), U38 span at {CALLER_N:04X}:{ENTRY:04X}")
    return dict(cases=len(cases),matched=matched,result_sha256=digest,cpu_words=14,stack_bytes=WINDOW_SIZE,ds_bytes=65536)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--unicorn", action="store_true")
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--routine", choices=sorted(ROUTINES), default="1276")
    args = parser.parse_args()
    select(args.routine)
    target, code, system = source()
    report = dict(routine=ROUTINE, target=dict(sha256=target["sha"], bytes=END - ENTRY, instructions=len(target["starts"]), file_offset=target["file_offset"]))
    if args.unicorn or args.native:
        with tempfile.TemporaryDirectory(prefix="poolrad-u38match-") as temp:
            function = library(Path(temp))
            if args.unicorn:
                report["unicorn"] = differential(target, code, system, function)
            if args.native:
                report["native"] = native_audit(code, system, function)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    main()
