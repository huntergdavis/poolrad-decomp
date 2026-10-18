#!/usr/bin/env python3
"""Hash-pinned SYSTEM FreeMem original-byte CPU14/full1MiB parity and native oracle."""
import argparse,ctypes,json,random,struct,subprocess,tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_16
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu,ROOT,SIZE,EXE_SHA,REGISTERS,physical,put16
import system_getmem_023f_audit as gm
check=gm.check
SYSTEM_CS,ENTRY,ENTRY_RETF,RUNERROR_IP=0x0AF8,0x254,0x265,0xE2
SPANS=((0x254,0x26E),(0x3D3,0x432),(0x432,0x491),(0x491,0x4A2))
WALK_HEAD,WALK_JAS=0x40F,(0x41A,0x420)
DS,SS,HEAP=0x1800,0x2800,0x3000
OK,RUNERROR,TRANSFER,LIMIT=0,2,3,4
NAMES={0:'return',2:'RunError204',3:'TRANSFER',4:'LIMIT'}
FLAGS=gm.FLAGS

def source():
    exe=(ROOT/'resources/DOS/POOLRAD/START.EXE').read_bytes();check(sha256(exe).hexdigest()==EXE_SHA,'media hash')
    image=bytes(unpack(exe)[0]);md=Cs(CS_ARCH_X86,CS_MODE_16);decoded={};digests={}
    for a,b in SPANS:
        blob=image[physical(SYSTEM_CS,a):physical(SYSTEM_CS,b)];ins=list(md.disasm(blob,a));check(sum(i.size for i in ins)==len(blob),'decode span')
        decoded.update({i.address:i for i in ins});digests[f'{a:04X}-{b:04X}']=dict(bytes=len(blob),instructions=len(ins),sha256=sha256(blob).hexdigest())
    check([v['sha256'] for v in digests.values()]==['47b372b606692fcf3038cd6734cf3f3c600d9dff7298dce2a50d2d0a03196c96','828388f0c2e899b6aaaf00c9c294397c879075f524eff052e73d26cde7e79311','b839d3ecc1ecd5e593a51dbb7c22231ad96156638c4676c7a3753bccff5bb7c9','440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d'],'span hashes differ')
    check(decoded[ENTRY_RETF].mnemonic=='retf' and decoded[ENTRY_RETF].op_str=='6' and decoded[0x268].op_str=='ax, 0xcc','entry ABI')
    check([(a,i.op_str) for a,i in decoded.items() if i.mnemonic=='call']==[(0x25F,'0x3d3'),(0x3F9,'0x491'),(0x426,'0x432'),(0x42B,'0x432')],'near calls')
    conditional={a:(a+i.size,int(i.op_str,16)) for a,i in decoded.items() if i.mnemonic.startswith('j') and i.mnemonic!='jmp'}
    print('FreeMem spans',digests,flush=True);return image,decoded,conditional,digests

def library(directory):
    p=directory/'freemem.so';subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-shared','-fPIC','-I',str(ROOT/'include'),str(ROOT/'src/recovered/system_freemem_0254_cpu.c'),'-o',str(p)],check=True)
    f=ctypes.CDLL(str(p)).pr_system_freemem_0254;f.argtypes=[ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu),ctypes.c_uint];f.restype=ctypes.c_int;return f

def build(image,rng,*,size=16,pointer=(0x3100,0),blocks=(),top=(0x3400,0),org=(0x3000,0),sp=0x8000,ss=SS,flags=0x602,writes=(),background=None,bp=None,first=None):
    m=bytearray([0xA5])*SIZE if background is None else bytearray(background);m[:len(image)]=image
    gm.heap(m,blocks=blocks,top=top,end=(0x4000,0),first=first)
    put16(m,DS,0x4400,org[1]);put16(m,DS,0x4402,org[0]);put16(m,DS,0x4410,0);put16(m,DS,0x4412,0)
    for j,v in enumerate((0x7000,0x9001,size,pointer[1],pointer[0])):put16(m,ss,(sp+2*j)&65535,v)
    for seg,off,blob in writes:
        for j,v in enumerate(blob):m[physical(seg,(off+j)&65535)]=v
    regs=[rng.randrange(65536) for _ in range(7)]
    if bp is not None:regs[6]=bp
    return m,Cpu(*regs,sp,SYSTEM_CS,ENTRY,DS,rng.randrange(65536),ss,flags),ss>=0xF000
def compare(label, memory, cpu, high, decoded, conditional, function, cap=64, count=2000000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory)); original.mem_write(SIZE, bytes(memory))
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, name))
    state = dict(stop=None, returning=False, returned=False, previous=None, last=None, steps=0, calls=[], unexpected=None, path=set())
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        del size, user
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if state["returned"]:
            state["stop"] = OK
            return emulator.emu_stop()
        if state["returning"]:
            state["returning"] = False
            if (cs, ip) != state["calls"].pop():
                state["stop"] = TRANSFER
                return emulator.emu_stop()
        if cs == SYSTEM_CS and ip == RUNERROR_IP:
            state["stop"] = RUNERROR
            return emulator.emu_stop()
        if cs != SYSTEM_CS or ip not in decoded:
            state["unexpected"] = (cs, ip)
            return emulator.emu_stop()
        if ip == WALK_HEAD and state["last"] in WALK_JAS:
            state["steps"] += 1
            if state["steps"] > cap:
                state["stop"] = LIMIT
                return emulator.emu_stop()
        ins = decoded[ip]
        visited.add(ip)
        state["path"].add(ip)
        if state["previous"] in conditional and state["previous"] != ip:
            edges.add((state["previous"], ip))
        state["previous"] = ip
        if ins.mnemonic in ("call", "lcall"):
            state["calls"].append((cs, (ip + ins.size) & 0xFFFF))
        elif ins.mnemonic in ("ret", "retf"):
            if ip == ENTRY_RETF:
                state["returned"] = True
            else:
                state["returning"] = True
        state["last"] = ip
        return None

    def mirror(emulator, access, address, size, value, user):
        del access, user
        for byte in range(size):
            raw = address + byte; at = raw & (SIZE - 1)
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    if high:
        original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(SYSTEM_CS, ENTRY), 2 * SIZE, count=count)
    check(state["unexpected"] is None, f"original left the decoded code at {state['unexpected']}: {label}")
    check(state["stop"] is not None, f"original did not stop: {label}")
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu), cap)
    check(status == state["stop"], f"C status {status}, original stopped with {NAMES[state['stop']]}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f"RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)")
    for (name, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, name), original.reg_read(register) & 0xFFFF
        check(mine == want, f"{name} mismatch {label}: C={mine:04x}, original={want:04x} ({NAMES[state['stop']]})")
    return state["stop"], visited, edges, state["path"]



def cases():
    result=[('zero-invalid',dict(size=0,pointer=(0,0xFFFF))),('bad-offset',dict(pointer=(0x3100,1))),
      ('below-org-seg',dict(pointer=(0x2FFF,0))),('below-org-off',dict(pointer=(0x3000,0),org=(0x3000,8))),
      ('above-top',dict(pointer=(0x3401,0))),('at-top',dict(pointer=(0x3400,0))),('top-same-seg',dict(pointer=(0x3400,0),top=(0x3400,8),size=8)),
      ('at-org',dict(pointer=(0x3000,0))),('duplicate',dict(blocks=((0x3100,0,2,0),))),
      ('same-seg-lower',dict(pointer=(0x3100,0),blocks=((0x3100,8,2,0),),size=8)),
      ('same-seg-higher',dict(pointer=(0x3100,8),blocks=((0x3100,0,0,8),),size=8)),
      ('merge-next',dict(blocks=((0x3101,0,4,8),))),('merge-prev',dict(blocks=((0x30FF,0,1,0),))),
      ('merge-both',dict(blocks=((0x30FF,0,1,0),(0x3101,0,4,8)))),('shrink-top',dict(pointer=(0x33FF,0))),
      ('merge-prev-top',dict(pointer=(0x33FF,0),blocks=((0x33FD,0,2,0),))),
      ('next-top-offset',dict(pointer=(0x33FF,8),top=(0x3400,8))),
      ('invalid-size-overrun',dict(size=0xFFFF)),
      ('next-offset-gap',dict(blocks=((0x3101,8,1,0),))),
      ('next-same-top-seg',dict(blocks=((0x3101,0,0,8),),top=(0x3101,8))),
      ('list-limit',dict(blocks=((0x3001,0,1,0),(0x3003,0,1,0)),cap=0)),
      ('list-cycle',dict(blocks=((0x3001,0,1,0),),writes=((0x3001,0,struct.pack('<HH',0,0x3001)),),cap=3)),
      ('ss-ds',dict(ss=DS)),('a20',dict(ss=0xFFFF)),
      ('alias-size-return',dict(ss=HEAP,sp=0x1008,pointer=(0x3100,0))),
      ('alias-merge-stack',dict(ss=HEAP,sp=0x1010,pointer=(0x3100,0),blocks=((0x3101,0,4,0),))),
      ('globals-node',dict(pointer=(DS+0x4400//16,0),org=(DS,0))),
    ]
    rng=random.Random(0x2543D3)
    for n in range(320):
        pointer=(0x3100+rng.randrange(0,0x100),rng.choice((0,8)));size=rng.choice((0,1,7,8,9,15,16,17,0xFFFF,rng.randrange(1,1024)))
        blocks=[(0x3000+j*0x10, rng.choice((0,8)),rng.randrange(1,8),rng.choice((0,8))) for j in range(1,n%8+1)]
        if n%3==0:blocks.append((pointer[0]+(pointer[1]+size+7)//16,(pointer[1]+size+7)&8,rng.randrange(1,16),rng.choice((0,8))))
        result.append((f'random-{n}',dict(pointer=pointer,size=size,blocks=blocks,flags=FLAGS[n%len(FLAGS)])))
    for sp in (0x20,0x21,0x8001,0xFFF4,0xFFF6):result.append((f'sp-{sp:x}',dict(sp=sp)))
    result.append(('random-ram',dict(background=rng.randbytes(SIZE))))
    return result

def differential(image,decoded,conditional,function):
    rng=random.Random(0x254);visited,edges,stops=set(),set(),{}
    for label,options in cases():
        cap=options.pop('cap',64);m,c,h=build(image,rng,**options)
        status,v,e,path=compare(label,m,c,h,decoded,conditional,function,cap=cap)
        visited|=v;edges|=e;stops[NAMES[status]]=stops.get(NAMES[status],0)+1
        if not label.startswith('random-'):print(label,NAMES[status],flush=True)
    missing=set(decoded)-visited;missing_edges={(a,b) for a,pair in conditional.items() for b in pair}-edges
    check(not missing and not missing_edges,f'coverage missing {sorted(missing)} edges {sorted(missing_edges)}')
    report=dict(cases=sum(stops.values()),stops=stops,starts=len(visited),conditional_outcomes=len(edges),cpu_words=14,ram_bytes=SIZE)
    print('Original-chain',report,flush=True);return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--unicorn',action='store_true');p.add_argument('--native',action='store_true');p.add_argument('--output',type=Path);a=p.parse_args()
    image,decoded,conditional,digests=source();report=dict(target=dict(spans=digests))
    with tempfile.TemporaryDirectory(prefix='freemem-0254-') as tmp:
        f=library(Path(tmp))
        if a.unicorn:report['unicorn']=differential(image,decoded,conditional,f)
        if a.native:
            from system_freemem_0254_native import native_audit
            report['native']=native_audit(image,f)
    if a.output:a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(report,sort_keys=True))
if __name__=='__main__':main()
