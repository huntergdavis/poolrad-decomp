#!/usr/bin/env python3
"""Compare complete U3C2 +0422 with original instructions on synthetic RAM."""
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
import u3c2_line_init_01a0_audit as init
from overlay_inventory import find_units, parse_mz

SIZE, MASK, Cpu, NAMES, REGS = init.SIZE, init.MASK, init.Cpu, init.NAMES, init.REGS
check, physical, put16 = init.check, init.physical, init.put16
PINS = ((0x2CC, 0x422, 111, '818132a151e78bff884848a874203a09a50b6d459e0ad9f5e7278de692d28337'),
        (0x422, 0x57B, 141, 'f075c6dba0b31e12aea19318958a72f4caee6fe7954c53f3583dd5dd9fa8114d'))


def source(exe_path, ovr_path):
    bodies, instructions = init.source(exe_path, ovr_path)
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    check(units[0x3C2]['entries'][5] == 0x422, 'entry5 mismatch')
    for start, end, count, digest in PINS:
        data = ovr[0x2FB52 + start:0x2FB52 + end]
        check(sha256(data).hexdigest() == digest, 'body hash mismatch')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, start))
        check(len(ins) == count and sum(i.size for i in ins) == end - start, 'decode mismatch')
        bodies[start], instructions[start] = data, ins
        print(f'U3C2 [{start:04X},{end:04X}): {len(data)} bytes/{count} instructions; SHA256 {digest}')
    calls = [(i.address, i.op_str) for i in instructions[0x422] if i.mnemonic == 'call']
    check(calls == [(0x45C, '0x1a0'), (0x4D6, '0x1a0'), (0x541, '0x2cc'), (0x54D, '0x2cc')],
          'callee mismatch')
    for unit, site in ((0x31, 0xD1F), (0x38, 0x1EFD), (0x5E, 0x2F1A)):
        base = units[unit]['ovr_offset']
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(ovr[base + site:base + site + 5], site))
        check(len(ins) == 1 and ins[0].mnemonic == 'lcall' and ins[0].op_str == '0x3db, 0x39'
              and site + 3 in units[unit]['fixups'], 'far caller mismatch')
    base, site = 0x2FB52, 0xAF5
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(ovr[base + site - 1:base + site + 3], site - 1))
    check([(i.mnemonic, i.op_str) for i in ins] == [('push', 'cs'), ('call', '0x422')],
          'near caller mismatch')
    print('PASS: entry5, four near callee sites, one near and three fixup-backed far callers')
    return bodies, instructions


def build(root, directory):
    output = directory / 'u3c2_0422.so'
    sources = ['overlay_u3c2_entry5_0422', 'overlay_u3c2_line_init_01a0',
               'overlay_u3c2_sign_entry1_0000', 'overlay_u3c2_entry3_02cc']
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-shared', '-fPIC', '-I', str(root / 'include')] +
                   [str(root / 'src/recovered' / (s + '.c')) for s in sources] +
                   ['-o', str(output)], check=True)
    library = ctypes.CDLL(str(output))
    library.pr_u3c2_line_check_0422.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                              ctypes.POINTER(Cpu), ctypes.c_size_t]
    library.pr_u3c2_line_check_0422.restype = ctypes.c_int
    return library


def fixture(points=(0, 0, 7, 3), *, radius=100, mode=1, seed=0, ss=0x2000,
            sp=0x8000, cs=0x3000, ds=0x1000, flags=0x7202, map_at=(0x6000, 0x100),
            outputs=((0x5000, 0x100), (0x5000, 0x102), (0x5000, 0x104)),
            heights=None):
    rng = random.Random(seed)
    memory = bytearray([0xA5]) * SIZE
    cpu = Cpu(*(rng.randrange(65536) for _ in NAMES))
    cpu.cs, cpu.ip, cpu.ss, cpu.sp, cpu.bp = cs, 0x422, ss, sp, 0x7777
    cpu.ds, cpu.flags = ds, flags
    for i in range(65536):
        memory[physical(map_at[0], i)] = (i * 17 + seed) & 255
    memory[physical(map_at[0], map_at[1] + 6)] = mode
    for i in range(256):
        memory[physical(ds, 0x2745 + i * 4)] = heights[0] if heights else (i * 13 + seed) & 255
        memory[physical(ds, 0x2746 + i * 4)] = heights[1] if heights else (i * 7 + seed) & 255
    for i in range(9):
        memory[physical(ds, 0x25C0 + i)] = (i * 23 + seed) & 255
    wordptr, yptr, xptr = outputs
    put16(memory, *wordptr, radius)
    memory[physical(*xptr)] = points[2] & 255
    memory[physical(*yptr)] = points[3] & 255
    frame = [0x7000, cs, map_at[1], map_at[0], wordptr[1], wordptr[0],
             yptr[1], yptr[0], xptr[1], xptr[0], points[1] & 255, points[0] & 255]
    for i, value in enumerate(frame):
        put16(memory, ss, sp + 2 * i, value)
    return memory, cpu


def compare(label, memory, cpu, bodies, instructions, library, coverage, budget=512):
    for start, data in bodies.items():
        for i, value in enumerate(data):
            memory[(physical(cpu.cs, start) + i) & MASK] = value
    c_ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    c_cpu = Cpu.from_buffer_copy(cpu)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing))
    emulator.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for name, register in zip(NAMES, REGS):
        emulator.reg_write(register, getattr(cpu, name))
    instruction_map = {physical(cpu.cs, i.address): i for ins in instructions.values() for i in ins}
    stack, stop, remaining = [], [None], [budget]

    def instruction(uc, address, size, user):
        del size, user
        ins = instruction_map.get(address & MASK)
        check(ins is not None, f'{label}: unexpected execution at {address:x}')
        if ins.address == 0x4DD:
            if remaining[0] == 0:
                stop[0] = 3
                uc.emu_stop()
                return
            remaining[0] -= 1
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
                stop[0] = 2
                uc.emu_stop()

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    emulator.emu_start(physical(cpu.cs, cpu.ip), SIZE * 2, count=500000)
    emulator.hook_del(hook)
    check(stop[0] is not None, f'{label}: original did not reach boundary')
    if stop[0] != 3:
        pc = physical(emulator.reg_read(regs.UC_X86_REG_CS), emulator.reg_read(regs.UC_X86_REG_IP))
        emulator.emu_start(pc, SIZE * 2, count=1)
    status = library.pr_u3c2_line_check_0422(c_ram, SIZE, ctypes.byref(c_cpu), budget)
    check(status == stop[0], f'{label}: status C={status}, original={stop[0]}')
    for name, register in zip(NAMES, REGS):
        actual, expected = getattr(c_cpu, name), emulator.reg_read(register) & 65535
        check(actual == expected, f'{label}: {name} C={actual:04x}, original={expected:04x}')
    if bytes(c_ram) != bytes(backing):
        bad = [i for i, (a, b) in enumerate(zip(c_ram, backing)) if a != b]
        raise AssertionError(f'{label}: RAM differs at {[hex(i) for i in bad[:12]]}')
    return status


def run(bodies, instructions, library, random_cases):
    coverage, counts = set(), {}

    def case(label, points=(0, 0, 7, 3), budget=512, **options):
        status = compare(label, *fixture(points, **options), bodies, instructions, library, coverage, budget)
        counts[status] = counts.get(status, 0) + 1
        if sum(counts.values()) % 128 == 0:
            print(f'Progress: {sum(counts.values())} full CPU/RAM cases passed', flush=True)

    for i, points in enumerate(product((-2, -1, 0, 1, 2), repeat=4)):
        case(f'short-lines-{i}', points, seed=i)
    for end in product((-128, -127, -1, 0, 1, 126, 127), repeat=2):
        for radius in (0, 1, 127, 0x7FFF, 0x8000, 0xFFFF):
            case(f'signed-range-{end}-{radius}', (0, 0, *end), radius=radius)
    for mode, heights in product((0, 1, 255), ((0, 0), (0, 255), (255, 0), (128, 128))):
        case(f'height-{mode}-{heights}', mode=mode, heights=heights)
    for budget in (0, 1, 2, 3, 7):
        case(f'limit-{budget}', budget=budget)
    for sp in (0x54, 0x55, 0x56, 0xFFE7, 0xFFE8):
        for ss in (0x2000, 0xFFFF):
            case(f'stack-{ss:x}-{sp:x}', sp=sp, ss=ss)
    for off in (0, 1, 0xFFF9, 0xFFFA, 0xFFFF):
        case(f'map-wrap-{off:x}', map_at=(0x6000, off), mode=0)
    for off in (0xF, 0x10, 0xFFFF):
        case(f'output-A20-{off:x}', outputs=((0xFFFF, off), (0x5000, 0x102), (0x5000, 0x104)))
    # Final output may change saved BP, actual far return, or a later pointer.
    for off in range(0x7FC4, 0x8018):
        case(f'output-frame-{off:x}', radius=0,
             outputs=((0x2000, off), (0x5000, 0x102), (0x5000, 0x104)))
    for off in range(0x7FC4, 0x8018):
        case(f'x-output-frame-{off:x}', radius=0,
             outputs=((0x5000, 0x100), (0x5000, 0x102), (0x2000, off)))
    case('same-ss-ds', ds=0x2000)
    case('relocated-code', cs=0x1234)
    case('A20-code', cs=0xFFFF)
    for flags in (0x202, 0x203, 0x602, 0xA02, 0xE03):
        case(f'flags-{flags:x}', flags=flags)
    rng = random.Random(0x3C20422)
    for i in range(random_cases):
        case(f'random-{i}', tuple(rng.randrange(-128, 128) for _ in range(4)),
             seed=i, radius=rng.randrange(128), mode=rng.choice((0, 1)))
    for start, ins in instructions.items():
        expected = {i.address for i in ins}
        check(expected <= coverage, f'uncovered {start:04X}: {sorted(expected - coverage)}')
        print(f'U3C2+{start:04X}: {len(expected & coverage)}/{len(expected)} instructions')
    print(f'PASS: {sum(counts.values())} full CPU/RAM cases; statuses {counts}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--ovr', type=Path, required=True)
    parser.add_argument('--random-cases', type=int, default=300)
    args = parser.parse_args()
    bodies, instructions = source(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix='poolrad-u3c2-0422-') as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, instructions, library, args.random_cases)


if __name__ == '__main__':
    main()
