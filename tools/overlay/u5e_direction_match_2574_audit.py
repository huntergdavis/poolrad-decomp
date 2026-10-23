#!/usr/bin/env python3
"""Compare complete U5E +2574 and its real +263D chain on synthetic CPU/RAM."""
import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import random
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as regs
import u5e_direction_263d_audit as dep
from pinned_shift_flags import LastFlagWriter, SPLIT_BY_RULE, declared_flags_split, split_used

ENTRY, END = 0x2574, 0x263D
SHA = '81c15df9fdd2f0693d2a4f1dd7e55ba75f049a8a785bba01b6c092ed98e3b101'
COMPASS = ([0, 1, 1, 1, 0, -1, -1, -1, 0],
           [-1, -1, 0, 1, 1, 1, 0, -1, 0])


def source(exe, ovr):
    bodies, decoded = dep.source_audit(exe, ovr)
    raw_exe, raw_ovr = exe.read_bytes(), ovr.read_bytes()
    units = {u['stub_para']: u for u in dep.find_units(
        raw_exe[dep.parse_mz(raw_exe)['header_bytes']:], raw_ovr)}
    base = units[0x5E]['ovr_offset']
    dep.check(base + ENTRY == 85359, 'caller file offset mismatch')
    data = raw_ovr[base + ENTRY:base + END]
    dep.check(sha256(data).hexdigest() == SHA, 'caller hash mismatch')
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, ENTRY))
    dep.check(len(instructions) == 64 and instructions[-1].address == 0x263A,
              'caller bounds changed')
    dep.check([(i.address, i.op_str) for i in instructions if i.mnemonic == 'call']
              == [(0x2619, '0x263d')], 'near-call changed')
    for start, end, sites, digest in (
        (0x0191, 0x02F9, (0x01EB,), '3944f2d5546d9b793c0284e5e9a9ca550490288cb810771860e5db964838841f'),
        (0x1406, 0x17F6, (0x15AD, 0x1650), '4d0a1fa91ee71fbc840153aa9737928bcca1479f222865d027dcf15c4e8c4235')):
        caller = raw_ovr[base + start:base + end]
        dep.check(sha256(caller).hexdigest() == digest, 'upstream caller hash mismatch')
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(caller, start))
        for site in sites:
            hit = next(i for i in ins if i.address == site)
            prior = next(i for i in ins if i.address + i.size == site)
            dep.check(hit.mnemonic == 'call' and hit.op_str == '0x2574' and
                      prior.mnemonic == 'push' and prior.op_str == 'cs', 'caller edge mismatch')
    bodies[(0x5E, ENTRY)], decoded[(0x5E, ENTRY)] = data, instructions
    print(f'U5E [{ENTRY:04X},{END:04X}): 201 bytes/64 instructions; SHA256 {SHA}')
    print('PASS: three aligned original caller edges, two caller-body hashes')
    return bodies, decoded


def build(root, directory):
    output = directory / 'u5e2574.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-shared', '-fPIC', '-I', str(root / 'include'),
                    *[str(root / 'src/recovered' / (n + '.c')) for n in (
                        'overlay_u5e_direction_match_2574', 'overlay_u5e_direction_263d',
                        'overlay_u3c2_entry4_057b')], '-o', str(output)], check=True)
    library = ctypes.CDLL(str(output))
    fn = library.pr_u5e_direction_match_2574
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                   ctypes.POINTER(dep.Cpu), ctypes.POINTER(dep.Layout), ctypes.c_size_t]
    fn.restype = ctypes.c_int
    return library


def fixture(*, signed_byte=1, type1=7, type2=50, counter=2, heading=0,
            ptr1=(0x7000, 0x300), ptr2=(0x7000, 0x400),
            first=(0x6000, 0x200), second=(0x6000, 0x100),
            link=(0x8000, 0x100), coords=(10, 10, 10, 10), **options):
    memory, cpu, layout = dep.fixture(coords, displacement=COMPASS, **options)
    cpu.ip = ENTRY
    for delta, pointer in ((4, first), (8, second)):
        dep.put16(memory, cpu.ss, cpu.sp + delta, pointer[1])
        dep.put16(memory, cpu.ss, cpu.sp + delta + 2, pointer[0])
    for offset, pointer, kind in ((0xCC, ptr1, type1), (0xD4, ptr2, type2)):
        segment, address = pointer if pointer is not None else (0, 0)
        dep.put16(memory, second[0], second[1] + offset, address)
        dep.put16(memory, second[0], second[1] + offset + 2, segment)
        if pointer is not None:
            memory[dep.physical(segment, address + 0x2E)] = kind
    memory[dep.physical(second[0], second[1] + 0x9C)] = signed_byte
    p = dep.physical(first[0], first[1] + 0x108)
    for i, value in enumerate((link[1] & 255, link[1] >> 8, link[0] & 255, link[0] >> 8)):
        memory[(p + i) & (dep.RAM_SIZE - 1)] = value
    memory[dep.physical(link[0], link[1] + 0xF)] = counter
    memory[dep.physical(link[0], link[1] + 9)] = heading
    return memory, cpu, layout


SPLIT_ARRIVALS = [0]


def compare(label, memory, cpu, layout, bodies, decoded, library, coverage,
            *, base=0, budget=2048):
    dep.install(memory, cpu, layout, bodies, base)
    c_ram = (ctypes.c_uint8 * dep.RAM_SIZE).from_buffer_copy(memory)
    c_cpu = dep.Cpu.from_buffer_copy(cpu)
    backing = (ctypes.c_uint8 * dep.RAM_SIZE).from_buffer_copy(memory)
    emulator = Uc(UC_ARCH_X86, UC_MODE_16)
    emulator.mem_map_ptr(0, dep.RAM_SIZE, 7, ctypes.addressof(backing))
    emulator.mem_map_ptr(dep.RAM_SIZE, dep.RAM_SIZE, 7, ctypes.addressof(backing))
    for name, register in zip(dep.NAMES, dep.REGISTERS):
        emulator.reg_write(register, getattr(cpu, name))
    segments = {0x5E: cpu.cs, 0x3C6: layout.slots_cs, 0x3C2: layout.predicate_cs}
    instruction_map = {dep.physical(segments[unit], i.address): (unit, i)
                       for (unit, _), instructions in decoded.items() for i in instructions}
    stack, remaining, stop = [], [budget], [None]
    writer = LastFlagWriter()  # declared splits for the pinned flag rules, see pinned_shift_flags

    def instruction(uc, address, size, user):
        del size, user
        pair = instruction_map.get(address & (dep.RAM_SIZE - 1))
        if pair is None:
            writer.step(uc, address)
            return  # Loaded EA stubs are checked by source_audit/install.
        unit, ins = pair
        coverage.add((unit, ins.address))
        cs = uc.reg_read(regs.UC_X86_REG_CS)
        if (unit, ins.address) in ((0x5E, 0x267F), (0x3C6, 0xC71)):
            if remaining[0] == 0:
                stop[0] = 3
                uc.emu_stop()
                return
            remaining[0] -= 1
        writer.step(uc, address)
        if ins.mnemonic in ('call', 'lcall'):
            stack.append((cs, ins.address + ins.size))
        elif ins.mnemonic == 'retf':
            if not stack:
                stop[0] = 0
                uc.emu_stop()
                return
            expected = stack.pop()
            p = dep.physical(uc.reg_read(regs.UC_X86_REG_SS), uc.reg_read(regs.UC_X86_REG_SP))
            frame = bytes(backing[(p + i) & (dep.RAM_SIZE - 1)] for i in range(4))
            actual = (int.from_bytes(frame[2:], 'little'), int.from_bytes(frame[:2], 'little'))
            if actual != expected:
                stop[0] = 5
                uc.emu_stop()

    hook = emulator.hook_add(UC_HOOK_CODE, instruction)
    emulator.emu_start(dep.physical(cpu.cs, cpu.ip), 2 * dep.RAM_SIZE, count=3000000)
    emulator.hook_del(hook)
    writer.finish(emulator)
    dep.check(stop[0] is not None, f'{label}: original did not reach a boundary')
    if stop[0] in (0, 5):
        pc = dep.physical(emulator.reg_read(regs.UC_X86_REG_CS),
                          emulator.reg_read(regs.UC_X86_REG_IP))
        emulator.emu_start(pc, 2 * dep.RAM_SIZE, count=1)
    status = library.pr_u5e_direction_match_2574(c_ram, dep.RAM_SIZE,
                                              ctypes.byref(c_cpu), ctypes.byref(layout), budget)
    dep.check(status == stop[0], f'{label}: status C={status}, original={stop[0]}')
    for name, register in zip(dep.NAMES, dep.REGISTERS):
        actual, expected = getattr(c_cpu, name), emulator.reg_read(register) & 65535
        if name == 'flags':
            problem = declared_flags_split(actual, expected, writer)
            dep.check(problem is None, f'{label}: {problem}')
            SPLIT_ARRIVALS[0] += split_used(actual, expected, writer)
            continue
        dep.check(actual == expected, f'{label}: {name} C={actual:04x}, original={expected:04x}')
    if bytes(c_ram) != bytes(backing):
        bad = [i for i, (a, b) in enumerate(zip(c_ram, backing)) if a != b]
        raise AssertionError(f'{label}: RAM differs at {[hex(i) for i in bad[:12]]}')
    return status


def run(bodies, decoded, library, random_cases):
    coverage, counts = set(), {}

    def case(label, *, budget=2048, **options):
        status = compare(label, *fixture(**options), bodies, decoded, library,
                         coverage, base=options.get('base', 0), budget=budget)
        counts[status] = counts.get(status, 0) + 1
        if sum(counts.values()) % 256 == 0:
            print(f'Progress: {sum(counts.values())} CPU/RAM cases passed', flush=True)

    for field in ('signed_byte', 'type1', 'type2', 'counter', 'heading'):
        for value in range(256):
            case(f'{field}-{value}', **{field: value})
    for signed_byte in (0, 1, 127, 128, 255):
        for type1 in (0, 7, 8, 34, 35, 36, 37, 38, 255):
            for type2 in (0, 49, 50, 51, 255):
                for counter in (0, 1, 2):
                    case(f'cross-{signed_byte}-{type1}-{type2}-{counter}',
                         signed_byte=signed_byte, type1=type1, type2=type2, counter=counter)
    for p1 in (None, (0, 0), (0, 0x300), (0x7000, 0), (0xFFFF, 0x10)):
        for p2 in (None, (0, 0), (0, 0x400), (0x7100, 0)):
            case(f'nullable-{p1}-{p2}', ptr1=p1, ptr2=p2)
    for direction in range(9):
        dx, dy = (a[direction] for a in COMPASS)
        for heading in range(9):
            case(f'direction-{direction}-{heading}', coords=(20, 10, 20 + dx, 10 + dy),
                 heading=heading)
    case('search-limit', budget=1, count=0, slots=(255, 254))
    case('predicate-limit', budget=6, coords=(10, 10, 11, 11))
    case('invalid-coordinate-limit', budget=270, coords=(255, 10, 10, 10))
    case('count-zero', count=0, slots=(255, 254))
    case('missing-slots', slots=(None, None))
    case('relocated', base=0x1234)
    case('same-ds-ss', ds=0x2000)
    for sp in (60, 61, 0xFFF3, 0xFFF4):
        case(f'stack-bound-{sp}', sp=sp)
    for sp in (0x5F32, 0x5F40, 0x65E0, 0x6600):
        case(f'stack-table-{sp:04x}', ss=0x1000, sp=sp, budget=1100)
    for offset in (0xFEF6, 0xFEF7, 0xFEF8, 0xFFFF):
        case(f'first-pointer-wrap-{offset:04x}', first=(0x6000, offset))
    for offset in (0xFF32, 0xFF33, 0xFF34, 0xFF2A, 0xFF2B, 0xFF2C, 0xFFFF):
        case(f'second-fields-wrap-{offset:04x}', second=(0x6000, offset))
    for offset in (0xFFEE, 0xFFF0, 0xFFF7, 0xFFFF):
        case(f'link-fields-wrap-{offset:04x}', link=(0x8000, offset))
    case('A20-link', first=(0xFFFF, 0xFEF7), link=(0xFFFF, 0x0007))
    case('A20-ptr1', ptr1=(0xFFFF, 0xFFE2))
    case('A20-ptr2', ptr2=(0xFFFF, 0xFFE2))
    for offset in (0x7FC0, 0x7FD0, 0x7FE0, 0x7FF0):
        case(f'link-stack-{offset:04x}', link=(0x2000, offset))
        case(f'ptr1-stack-{offset:04x}', ptr1=(0x2000, offset))
        case(f'ptr2-stack-{offset:04x}', ptr2=(0x2000, offset))
    rng = random.Random(0x5E2574)
    for i in range(random_cases):
        case(f'random-{i}', signed_byte=rng.randrange(256), type1=rng.randrange(256),
             type2=rng.choice((50, rng.randrange(256))), counter=rng.randrange(256),
             heading=rng.randrange(256), seed=i)
    starts = {(0x5E, i.address) for i in decoded[(0x5E, ENTRY)]}
    dep.check(starts <= coverage, f'uncovered caller instructions: {sorted(starts - coverage)}')
    print(f'PASS: {sum(counts.values())} full CPU/RAM cases; statuses {counts}; FLAGS differ from raw Unicorn only by '
          f'the declared pinned splits at {SPLIT_ARRIVALS[0]} arrivals ({SPLIT_BY_RULE["div"]} after a word DIV/IDIV: pinned six bits; '
          f'{SPLIT_BY_RULE["shift"]} after a non-zero shift: AF set)')
    for key, instructions in decoded.items():
        expected = {(key[0], i.address) for i in instructions}
        print(f'U{key[0]:X}+{key[1]:04X}: {len(expected & coverage)}/{len(expected)} instructions')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--ovr', type=Path, required=True)
    parser.add_argument('--random-cases', type=int, default=200)
    args = parser.parse_args()
    bodies, decoded = source(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix='poolrad-u5e-2574-') as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == '__main__':
    main()
