#!/usr/bin/env python3
"""Original CPU14/whole-RAM parity for U25 +0482 and real +047B/+031F."""
import argparse
import ctypes
from hashlib import sha256
from itertools import product
from pathlib import Path
import random
import subprocess
import tempfile
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as regs
from overlay_inventory import find_units, parse_mz
from u3c2_line_init_01a0_audit import Cpu, NAMES, REGS, SIZE, MASK, EXE_SHA, OVR_SHA, check, physical, put16
from u33_grid_pattern_0615_audit import PINS as LOOKUP_PINS

PINS = ((0x25, 0x482, 0x511, 52, 'bcfe56989d4733d260e0f85a7146d4cb56de3041cbf20b7de27851f0f513df2b'),) + tuple(
    p for p in LOOKUP_PINS if p[:2] in ((0x3BD, 0x31F), (0x3BD, 0x47B)))
SOURCES = ('overlay_u25_step_probe_0482', 'overlay_u3bd_lookup_fullstate', 'overlay_u3bd_in_grid_031f')


def source(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE mismatch')
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR mismatch')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    check(units[0x25]['ovr_offset'] == 0x74D3 and units[0x25]['memory_segment'] == 0x2B and
          units[0x25]['entries'][6] == 0x482, 'caller descriptor mismatch')
    check(units[0x3BD]['memory_segment'] == 0x3D5 and units[0x3BD]['entries'][2] == 0x47B,
          'callee descriptor mismatch')
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    bodies, decoded = {}, {}
    for unit, start, end, count, digest in PINS:
        data = ovr[units[unit]['ovr_offset'] + start:units[unit]['ovr_offset'] + end]
        ins = list(decoder.disasm(data, start))
        check(sha256(data).hexdigest() == digest and len(ins) == count and
              sum(i.size for i in ins) == end-start, f'body mismatch {unit:x}:{start:x}')
        bodies[unit,start], decoded[unit,start] = data, ins
        print(f'U{unit:X}+{start:04X}: {len(data)} bytes/{count} instructions; SHA256 {digest}')
    caller = decoded[0x25,0x482]
    check([(i.address,i.op_str) for i in caller if 'call' in i.mnemonic] ==
          [(0x4C9,'0x3d5, 0x2a')], 'call site mismatch')
    check(0x4CC in units[0x25]['fixups'], 'call relocation absent')
    u = units[0x1C]
    for site in (0x3F5,0x1803,0x1FF5):
        ins = list(decoder.disasm(ovr[u['ovr_offset']+site:u['ovr_offset']+site+5],site))
        check(len(ins) == 1 and ins[0].mnemonic == 'lcall' and
              ins[0].op_str == '0x2b, 0x3e' and site+3 in u['fixups'], 'incoming call mismatch')
    check(caller[-1].mnemonic == 'retf' and caller[-1].op_str == '6', 'return mismatch')
    print('PASS: caller/callee descriptors, one relocated callee call, three fixup-backed callers')
    return bodies, decoded


def build(root, directory):
    path = directory / 'u25_0482.so'
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-shared','-fPIC',
                    '-I',str(root/'include')] + [str(root/'src/recovered'/(s+'.c')) for s in SOURCES] +
                   ['-o',str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_step_probe_0482_fullstate.argtypes = [ctypes.POINTER(ctypes.c_uint8),ctypes.c_size_t,ctypes.POINTER(Cpu)]
    lib.pr_u25_step_probe_0482_fullstate.restype = ctypes.c_int
    return lib


def fixture(*, window=1, direction=0, x=3, y=4, map_id=5, pattern=0, stop_second=False,
            cs=0x3000, ss=0x2000, ds=0x1000, sp=0x8000, flags=0x202,
            lookup_cs=0x4000, stub_cs=0x3D5, buffer=(0x5000,0x100),
            window0=(0x6000,0x100), window1=(0x7000,0x100), seed=None, stale=0):
    memory = bytearray([0xA5])*SIZE if seed is None else bytearray(random.Random(seed).randbytes(SIZE))
    cpu = Cpu(0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,sp,cs,0x482,ds,0x1111,ss,flags)
    for i in range(0x400): memory[physical(buffer[0],buffer[1]+i)] = pattern
    for off,val in ((0,0x7000),(2,cs),(4,0xAA00 | direction),(6,0xBB00 | (y&255)),(8,0xCC00 | (x&255))):
        put16(memory,ss,sp+off,val)
    for off,ptr in ((0x49D2,window0),(0x49D6,window1),(0x6A5C,buffer)):
        put16(memory,ds,off,ptr[1]); put16(memory,ds,off+2,ptr[0])
    put16(memory,window0[0],window0[1]+0x1CC,window)
    memory[physical(ds,0x84DC)] = map_id
    memory[physical(ss,sp-19)] = stale
    if stop_second:
        dx,dy = {0:(0,-1),2:(1,0),4:(0,1),6:(-1,0)}.get(direction,(0,0))
        nx,ny = (x+dx)&15,(y+dy)&15
        for plane in (0,0x100):memory[physical(buffer[0],buffer[1]+16*ny+nx+plane)] = 0xFF
    return memory,cpu,lookup_cs,stub_cs


# Adapted from the accepted U33 +0615 oracle: actual nested and outer RETFs.
def compare(label, initial, bodies, decoded, library, coverage, edges):
    memory, cpu, lookup_cs, stub_cs = initial
    instruction_map = {}
    for (unit, start), data in bodies.items():
        segment = cpu.cs if unit == 0x25 else lookup_cs
        for i, value in enumerate(data):
            memory[(physical(segment, start) + i) & MASK] = value
        for ins in decoded[unit, start]:
            instruction_map[physical(segment, ins.address)] = (unit, ins)
    # Relocated original call segment words and loaded far-jump stubs.
    for site, stub, entry in ((0x4C9, 0x2A, 0x47B),):
        put16(memory, cpu.cs, site + 3, stub_cs)
        memory[physical(stub_cs, stub)] = 0xEA
        put16(memory, stub_cs, stub + 1, entry)
        put16(memory, stub_cs, stub + 3, lookup_cs)
    stubs = {physical(stub_cs, 0x2A)}
    immutable = {at:memory[at] for at in instruction_map}
    for (unit,start),data in bodies.items():
        segment = cpu.cs if unit == 0x25 else lookup_cs
        for i in range(len(data)):
            at = (physical(segment,start)+i)&MASK
            immutable[at] = memory[at]
    for i in range(5):
        at = (physical(stub_cs,0x2A)+i)&MASK
        immutable[at] = memory[at]
    c_ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    original.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for name, reg in zip(NAMES, REGS):
        original.reg_write(reg, getattr(cpu, name))
    stack, stop, previous = [], [None], [None]

    def instruction(uc, address, size, user):
        del size, user
        address &= MASK
        if address in stubs:
            previous[0] = None
            return
        check(address in instruction_map, f'{label}: unexpected PC {address:x}')
        unit, ins = instruction_map[address]
        if previous[0] is not None:
            edges.add((previous[0], (unit, ins.address)))
        previous[0] = (unit, ins.address) if ins.mnemonic.startswith('j') and ins.mnemonic != 'jmp' else None
        coverage.add((unit, ins.address))
        if ins.mnemonic in ('call', 'lcall'):
            stack.append((uc.reg_read(regs.UC_X86_REG_CS), ins.address + ins.size))
        elif ins.mnemonic == 'retf':
            if not stack:
                stop[0] = 0
                uc.emu_stop()
                return
            expected = stack.pop()
            p = physical(uc.reg_read(regs.UC_X86_REG_SS), uc.reg_read(regs.UC_X86_REG_SP))
            frame = bytes(backing[(p + i) & MASK] for i in range(4))
            actual = (int.from_bytes(frame[2:], 'little'), int.from_bytes(frame[:2], 'little'))
            if actual != expected:
                stop[0] = 4
                uc.emu_stop()

    hook = original.hook_add(UC_HOOK_CODE, instruction)
    original.emu_start(physical(cpu.cs, cpu.ip), SIZE * 2, count=20000)
    original.hook_del(hook)
    check(stop[0] is not None, f'{label}: original did not return')
    pc = physical(original.reg_read(regs.UC_X86_REG_CS), original.reg_read(regs.UC_X86_REG_IP))
    original.emu_start(pc, SIZE * 2, count=1)
    check(all(backing[at] == value for at,value in immutable.items()), f'{label}: immutable code modified')
    status = library.pr_u25_step_probe_0482_fullstate(c_ram, SIZE, ctypes.byref(c_cpu))
    check(status == stop[0], f'{label}: C status {status}, original {stop[0]}')
    for name, reg in zip(NAMES, REGS):
        actual, expected = getattr(c_cpu, name), original.reg_read(reg) & 65535
        check(actual == expected, f'{label}: {name} C={actual:04x}, original={expected:04x}')
    if bytes(c_ram) != bytes(backing):
        bad = [i for i, (a, b) in enumerate(zip(c_ram, backing)) if a != b]
        raise AssertionError(f'{label}: RAM differs at {[hex(i) for i in bad[:12]]}')
    return status


def run(bodies, decoded, library):
    coverage, edges, counts = set(), set(), {}
    def case(label, **options):
        status = compare(label,fixture(**options),bodies,decoded,library,coverage,edges)
        counts[status] = counts.get(status,0)+1
        if sum(counts.values()) % 128 == 0:print(f'Progress: {sum(counts.values())} cases passed',flush=True)
    for window,direction,pattern in product((0,1,0x8000),(0,2,4,6,1,7,255),(0,0xFF,0xA5)):
        case(f'basic-{window}-{direction}-{pattern}',window=window,direction=direction,pattern=pattern)
    for direction in (0,2,4,6):case(f'stop-second-{direction}',direction=direction,stop_second=True)

    for x,y,direction,map_id in product((0,1,14,15,16,127,128,255),(0,1,14,15,16,127,128,255),
                                       (0,2,4,6,1,255),(0,5,10)):
        case(f'grid-{x}-{y}-{direction}-{map_id}',x=x,y=y,direction=direction,map_id=map_id)
    for direction in range(256):case(f'direction-{direction}',direction=direction)
    for sp in (0x80,0x81,0x82,0xFFF8,0xFFF9,0xFFFA):case(f'stack-{sp:x}',sp=sp)
    for at in range(0x7FC0,0x800C):
        case(f'window-write-stack-{at:x}',window=0,window1=(0x2000,(at-0x582)&65535))
        case(f'window-read-stack-{at:x}',window0=(0x2000,(at-0x1CC)&65535))
    for sp in range(0x49D0,0x49F0):case(f'window-pointer-stack-{sp:x}',ss=0x1000,sp=sp)
    for sp in range(0x6A58,0x6A78):case(f'map-pointer-stack-{sp:x}',ss=0x1000,sp=sp)
    for off in range(0x7FC0,0x800C):case(f'map-stack-{off:x}',buffer=(0x2000,off))
    case('A20-stack',ss=0xFFFF,sp=0x80)
    case('A20-window-write',window=0,window1=(0xFFFF,0xFA8D))
    case('wrapped-map',buffer=(0x5000,0xFFF0))
    case('relocated-caller',cs=0x3333)
    case('relocated-callee',lookup_cs=0x5555,stub_cs=0x888)
    for flags in (0x202,0x203,0x602,0xA02,0xE03):case(f'flags-{flags:x}',flags=flags)
    rng = random.Random(0x250482)
    for i in range(300):
        case(f'random-{i}',direction=rng.randrange(256),x=rng.randrange(256),y=rng.randrange(256),
             window=rng.choice((0,1,0x8000)),map_id=rng.choice((0,5,10)),pattern=rng.randrange(256),
             stale=rng.randrange(256),seed=i)
    for (unit,start),ins in decoded.items():
        expected = {(unit,i.address) for i in ins}
        print(f'U{unit:X}+{start:04X}: {len(expected&coverage)}/{len(expected)} instructions')
        check(expected <= coverage,f'uncovered sites {sorted(expected-coverage)}')
    caller = decoded[0x25,0x482]
    conditional = [i for i in caller if i.mnemonic.startswith('j') and i.mnemonic != 'jmp']
    expected = {((0x25,i.address),(0x25,dest)) for i in conditional
                for dest in (i.address+i.size,int(i.op_str,16))}
    check(expected <= edges,f'uncovered caller branch edges {expected-edges}')
    print(f'Caller branch edges: {len(expected & edges)}/{len(expected)}')
    print(f'PASS: {sum(counts.values())} whole CPU/RAM cases; statuses {counts}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe',required=True,type=Path)
    parser.add_argument('--ovr',required=True,type=Path)
    args = parser.parse_args()
    bodies,decoded = source(args.exe,args.ovr)
    with tempfile.TemporaryDirectory(prefix='poolrad-u25-0482-') as directory:
        library = build(Path(__file__).resolve().parents[2],Path(directory))
        run(bodies,decoded,library)


if __name__ == '__main__':main()
