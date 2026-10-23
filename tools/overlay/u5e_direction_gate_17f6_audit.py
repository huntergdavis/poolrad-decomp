#!/usr/bin/env python3
"""Compare complete U5E +17F6 and its real +263D chain on synthetic CPU/RAM."""
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

ENTRY, END = 0x17F6, 0x1884
SHA = '0d16445d9e2c355857b7dc440d7f87f55001c9b6cbe79750c541a6cb76b430ef'
COMPASS = ([0, 1, 1, 1, 0, -1, -1, -1, 0],
           [-1, -1, 0, 1, 1, 1, 0, -1, 0])


def source(exe, ovr):
    bodies, decoded = dep.source_audit(exe, ovr)
    raw_exe, raw_ovr = exe.read_bytes(), ovr.read_bytes()
    units = {u['stub_para']: u for u in dep.find_units(
        raw_exe[dep.parse_mz(raw_exe)['header_bytes']:], raw_ovr)}
    dep.check(units[0x5E]['entries'][11] == ENTRY and
              units[0x5E]['ovr_offset'] + ENTRY == 81905, 'entry identity mismatch')
    for unit, start, end, site, digest in (
        (0x2F, 0x0D45, 0x0E45, 0x0DD4, 'e085b0a9e6a5dd12f42d3c5d7756fbeb2652a8357c0a2a435283f88d794ea302'),
        (0x31, 0x0B3C, 0x0F4F, 0x0EA2, '1b4b161741c25c8f2f8c7e2411619b4b43ccc5472863253d82ffeed135ea1b13'),
        (0x5E, 0x0627, 0x0712, 0x06E0, 'a5e1ad1fca36243e69a1cf5c1a33c508cbfb8a87d2fdb75ed828232d7c1c0c5d'),
        (0x5E, 0x0E82, 0x1085, 0x1040, 'fe5c0ec8c731f7bf5b3cbb5bfc4d3fc22d54f01bf145e3c28e517c9a918fd4a1'),
        (0x5E, 0x2C3D, 0x2DFA, 0x2D1E, '5043d2220ebb0159f337fce8ab20058cb067aa3d50c2fedbdc516a80e3b0e939')):
        base = units[unit]['ovr_offset']
        data = raw_ovr[base + start:base + end]
        dep.check(sha256(data).hexdigest() == digest, 'caller evidence hash mismatch')
        instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, start))
        hit = next(i for i in instructions if i.address == site)
        if unit == 0x5E:
            prior = next(i for i in instructions if i.address + i.size == site)
            dep.check(hit.mnemonic == 'call' and hit.op_str == '0x17f6' and
                      prior.mnemonic == 'push' and prior.op_str == 'cs', 'near caller mismatch')
        else:
            dep.check(hit.mnemonic == 'lcall' and hit.op_str == '0x68, 0x57' and
                      site + 3 in units[unit]['fixups'], 'far caller mismatch')
    print('PASS: public entry 11 identity and all five original call-site signals')
    data = ovr.read_bytes()[81905:82047]
    dep.check(sha256(data).hexdigest() == SHA, 'caller hash mismatch')
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(data, ENTRY))
    dep.check(len(instructions) == 53 and instructions[-1].address == 0x1881,
              'caller bounds changed')
    dep.check([(i.address, i.op_str) for i in instructions if i.mnemonic == 'call']
              == [(0x1815, '0x263d')], 'near-call changed')
    bodies[(0x5E, ENTRY)], decoded[(0x5E, ENTRY)] = data, instructions
    print(f'U5E [{ENTRY:04X},{END:04X}): 142 bytes/53 instructions; SHA256 {SHA}')
    return bodies, decoded


def build(root, directory):
    output = directory / 'u5e17f6.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-shared', '-fPIC', '-I', str(root / 'include'),
                    *[str(root / 'src/recovered' / (n + '.c')) for n in (
                        'overlay_u5e_direction_gate_17f6', 'overlay_u5e_direction_263d',
                        'overlay_u3c2_entry4_057b')], '-o', str(output)], check=True)
    library = ctypes.CDLL(str(output))
    fn = library.pr_u5e_direction_gate_17f6
    fn.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                   ctypes.POINTER(dep.Cpu), ctypes.POINTER(dep.Layout), ctypes.c_size_t]
    fn.restype = ctypes.c_int
    return library


def fixture(*, direction=0, heading=0, orientation=0, counter=0,
            record=None, link=None, coords=None, displacement=COMPASS, **options):
    dx, dy = (values[direction] for values in COMPASS)
    memory, cpu, layout = dep.fixture(coords or (20, 10, 20 + dx, 10 + dy),
                                      displacement=displacement, **options)
    cpu.ip = ENTRY
    # Existing fixture's first argument is 6000:0200, second 6000:0100.
    record = record or (0x6000, 0x200)
    link = link or (0x7000, 0x100)
    dep.put16(memory, cpu.ss, cpu.sp + 4, record[1])
    dep.put16(memory, cpu.ss, cpu.sp + 6, record[0])
    # Two physical words of the LES operand are contiguous, even at FFFE.
    p = dep.physical(record[0], record[1] + 0x108)
    for i, value in enumerate((link[1] & 255, link[1] >> 8,
                               link[0] & 255, link[0] >> 8)):
        memory[(p + i) & (dep.RAM_SIZE - 1)] = value
    for offset, value in ((9, heading), (15, counter), (18, orientation)):
        memory[dep.physical(link[0], link[1] + offset)] = value
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
    status = library.pr_u5e_direction_gate_17f6(c_ram, dep.RAM_SIZE,
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

    for direction in range(9):
        for heading in range(256):
            case(f'direction-{direction}-heading-{heading}', direction=direction,
                 heading=heading, orientation=(heading * 73) & 255, counter=heading)
    for orientation in range(256):
        case(f'orientation-{orientation}', orientation=orientation, heading=255)
    for sp in (50, 51, 0xFFF3, 0xFFF4):
        case(f'stack-bound-{sp}', sp=sp)
    case('search-limit', budget=1, count=0, slots=(255, 254))
    case('predicate-limit', budget=6, direction=7)
    case('direction-eight', coords=(10, 10, 11, 11), displacement=(
        [0, 100, 100, 100, 0, -100, -100, -100, 0],
        [-100, -100, 0, 100, 100, 100, 0, -100, 0]))
    case('count-zero', count=0, slots=(255, 254))
    case('missing-slots', slots=(None, None))
    case('equivalent-pointer', equivalent_pointer=True)
    case('relocated', base=0x1234, direction=5)
    case('same-ds-ss', ds=0x2000)
    for sp in (0x5F32, 0x5F40, 0x65E0, 0x6600):
        case(f'stack-table-{sp:04x}', ss=0x1000, sp=sp, budget=1100)
    for offset in (0xFEF6, 0xFEF7, 0xFEF8, 0xFFF0, 0xFFFF):
        case(f'link-operand-wrap-{offset:04x}', record=(0x6000, offset))
    for offset in (0xFFEE, 0xFFF0, 0xFFF1, 0xFFF7, 0xFFFF):
        case(f'record-fields-wrap-{offset:04x}', link=(0x7000, offset), heading=255)
    case('counter-alias-link-offset', link=(0x6000, 0x2F9), counter=0xF9)
    case('counter-alias-link-segment', link=(0x6000, 0x2FB), counter=0)
    case('link-A20', record=(0xFFFF, 0xFEF7), link=(0xFFFF, 0x0007))
    for offset in (0x7FE0, 0x7FE8, 0x7FF0, 0x7FF2, 0x7FF5):
        case(f'record-stack-{offset:04x}', link=(0x2000, offset), heading=5)
    rng = random.Random(0x5E17F6)
    for i in range(random_cases):
        case(f'random-{i}', direction=rng.randrange(9), heading=rng.randrange(256),
             orientation=rng.randrange(256), counter=rng.randrange(256), seed=i)
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
    with tempfile.TemporaryDirectory(prefix='poolrad-u5e-17f6-') as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == '__main__':
    main()
