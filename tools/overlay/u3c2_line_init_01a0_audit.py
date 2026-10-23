#!/usr/bin/env python3
"""Compare U3C2 +01A0 and its real sign helper against C on synthetic RAM."""
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
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used

SIZE, MASK = 1 << 20, (1 << 20) - 1
NAMES = 'ax bx cx dx si di bp sp cs ip ds es ss flags'.split()
REGS = [getattr(regs, 'UC_X86_REG_' + ('EFLAGS' if n == 'flags' else n.upper())) for n in NAMES]
EXE_SHA = 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd'
OVR_SHA = '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614'
PINS = ((0, 0x2E, 17, '5e8e5fc8a5907fee2efd922fc4f96682fd1f2e8e58cbebcd058d3f025948e5bf'),
        (0x1A0, 0x2CC, 99, '7fbacb8d9d298c72bd886321a58a8d842f2ec6dc350fc83fde6064fe78f27f40'))


class Cpu(ctypes.Structure):
    _fields_ = [(n, ctypes.c_uint16) for n in NAMES]


def check(value, message):
    if not value:
        raise AssertionError(message)


def physical(segment, offset):
    return ((segment << 4) + (offset & 65535)) & MASK


def put16(memory, segment, offset, value):
    p = physical(segment, offset)
    memory[p] = value & 255
    memory[(p + 1) & MASK] = (value >> 8) & 255


def source(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'unsupported START.EXE')
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, 'unsupported GAME.OVR')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    unit = units[0x3C2]
    check(unit['entries'][2] == 0x1A0 and unit['ovr_offset'] == 0x2FB52 and
          unit['memory_segment'] == 0x3DB, 'descriptor mismatch')
    bodies, instructions = {}, {}
    for start, end, count, digest in PINS:
        data = ovr[unit['ovr_offset'] + start:unit['ovr_offset'] + end]
        check(sha256(data).hexdigest() == digest, 'body hash mismatch')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, start))
        check(len(ins) == count and sum(i.size for i in ins) == end - start, 'decode mismatch')
        bodies[start], instructions[start] = data, ins
        print(f'U3C2 [{start:04X},{end:04X}): {len(data)} bytes/{len(ins)} instructions; SHA256 {digest}')
    calls = [i for i in instructions[0x1A0] if i.mnemonic == 'call']
    check([(i.address, i.op_str) for i in calls] == [(0x1F9, '0'), (0x213, '0')], 'callee mismatch')
    for which, site in ((0x3C2, 0x45C), (0x3C2, 0x4D6), (0x86, 0x2A4B), (0x86, 0x2B4C)):
        base = units[which]['ovr_offset']
        if which == 0x3C2:
            ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(ovr[base + site - 1:base + site + 3], site - 1))
            check([(i.mnemonic, i.op_str) for i in ins] == [('push', 'cs'), ('call', '0x1a0')],
                  'near caller mismatch')
        else:
            ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(ovr[base + site:base + site + 5], site))
            check(len(ins) == 1 and ins[0].mnemonic == 'lcall' and ins[0].op_str == '0x3db, 0x2a' and
                  site + 3 in units[which]['fixups'], 'far caller mismatch')
    print('PASS: public entry 2, two near caller edges, two fixup-backed far caller edges')
    return bodies, instructions


def build(root, directory):
    output = directory / 'u3c2_01a0.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-shared', '-fPIC', '-I', str(root / 'include'),
                    str(root / 'src/recovered/overlay_u3c2_line_init_01a0.c'),
                    str(root / 'src/recovered/overlay_u3c2_sign_entry1_0000.c'),
                    '-o', str(output)], check=True)
    library = ctypes.CDLL(str(output))
    library.pr_u3c2_line_init_01a0.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                             ctypes.POINTER(Cpu)]
    library.pr_u3c2_line_init_01a0.restype = ctypes.c_int
    return library


def fixture(points=(0, 0, 7, 3), *, record=(0x6000, 0x100), ss=0x2000,
            sp=0x8000, cs=0x3000, seed=0, flags=0x7202):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE
    cpu = Cpu(*(rng.randrange(65536) for _ in NAMES))
    cpu.cs, cpu.ip, cpu.ss, cpu.sp, cpu.bp = cs, 0x1A0, ss, sp, 0x7777
    cpu.ds, cpu.flags = 0x1000, flags
    for i, value in enumerate(points):
        put16(memory, record[0], record[1] + i * 2, value)
    # Establish the ABI frame last; alias fixtures may change coordinate values.
    for offset, value in ((0, 0x7000), (2, cs), (4, record[1]), (6, record[0])):
        put16(memory, ss, sp + offset, value)
    return memory, cpu


SPLIT_ARRIVALS = [0]


def compare(label, memory, cpu, bodies, instructions, library, coverage):
    for start, data in bodies.items():
        p = physical(cpu.cs, start)
        memory[p:p + len(data)] = data
    c_ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    emulator.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for name, register in zip(NAMES, REGS):
        emulator.reg_write(register, getattr(cpu, name))
    instruction_map = {physical(cpu.cs, i.address): i for ins in instructions.values() for i in ins}
    stack, stop = [], [None]
    writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

    def instruction(uc, address, size, user):
        del size, user
        ins = instruction_map.get(address & MASK)
        check(ins is not None, f'{label}: unexpected execution at {address:x}')
        writer.step(uc, address)
        coverage.add(ins.address)
        if ins.mnemonic == 'call':
            stack.append((cpu.cs, ins.address + ins.size))
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

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    emulator.emu_start(physical(cpu.cs, cpu.ip), SIZE * 2, count=1000)
    emulator.hook_del(hook)
    writer.finish(emulator)
    check(stop[0] is not None, f'{label}: original did not return')
    pc = physical(emulator.reg_read(regs.UC_X86_REG_CS), emulator.reg_read(regs.UC_X86_REG_IP))
    emulator.emu_start(pc, SIZE * 2, count=1)
    status = library.pr_u3c2_line_init_01a0(c_ram, SIZE, ctypes.byref(c_cpu))
    check(status == stop[0], f'{label}: status C={status}, original={stop[0]}')
    for name, register in zip(NAMES, REGS):
        actual, expected = getattr(c_cpu, name), emulator.reg_read(register) & 65535
        if name == 'flags':
            problem = declared_flags_split(actual, expected, writer)
            check(problem is None, f'{label}: {problem}')
            SPLIT_ARRIVALS[0] += split_used(actual, expected, writer)
            continue
        check(actual == expected, f'{label}: {name} C={actual:04x}, original={expected:04x}')
    if bytes(c_ram) != bytes(backing):
        bad = [i for i, (a, b) in enumerate(zip(c_ram, backing)) if a != b]
        raise AssertionError(f'{label}: RAM differs at {[hex(i) for i in bad[:12]]}')
    return status


def run(bodies, instructions, library, random_cases):
    coverage, counts = set(), {}

    def case(label, points=(0, 0, 7, 3), **options):
        status = compare(label, *fixture(points, **options), bodies, instructions, library, coverage)
        counts[status] = counts.get(status, 0) + 1
        if sum(counts.values()) % 256 == 0:
            print(f'Progress: {sum(counts.values())} full CPU/RAM cases passed', flush=True)

    edges = (0, 1, 0x3FFF, 0x4000, 0x7FFF, 0x8000, 0xFFFF)
    for i, points in enumerate(product(edges, repeat=4)):
        case(f'coordinates-{i}', points)
    for sp in (16, 17, 18, 0xFFF7, 0xFFF8):
        case(f'stack-bound-{sp:04x}', sp=sp)
    for off in range(0xFFE0, 0x10000):
        case(f'record-wrap-{off:04x}', record=(0x6000, off))
    for off in range(0, 32):
        case(f'A20-record-{off:04x}', record=(0xFFFF, off))
    for off in range(0x7FC0, 0x8010):
        case(f'record-stack-{off:04x}', record=(0x2000, off))
    for off in range(0xFFE0, 0x10000):
        case(f'record-low-stack-{off:04x}', record=(0x2000, off), sp=16)
    case('same-ss-ds', ss=0x1000)
    case('relocated-code', cs=0x1234)
    for flags in (0x202, 0x203, 0x602, 0xA02, 0xE03):
        case(f'flags-{flags:x}', flags=flags)
    rng = random.Random(0x3C201A0)
    for i in range(random_cases):
        case(f'random-{i}', tuple(rng.randrange(65536) for _ in range(4)), seed=i)
    for start, ins in instructions.items():
        expected = {i.address for i in ins}
        check(expected <= coverage, f'uncovered {start:04X}: {sorted(expected - coverage)}')
        print(f'U3C2+{start:04X}: {len(expected & coverage)}/{len(expected)} instructions')
    print(f'PASS: {sum(counts.values())} full CPU/RAM cases; statuses {counts}; FLAGS differ from raw Unicorn only by '
          f'the declared pinned splits at {SPLIT_ARRIVALS[0]} stops {SPLIT_BY_RULE}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--ovr', type=Path, required=True)
    parser.add_argument('--random-cases', type=int, default=300)
    args = parser.parse_args()
    bodies, instructions = source(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix='poolrad-u3c2-01a0-') as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, instructions, library, args.random_cases)


if __name__ == '__main__':
    main()
