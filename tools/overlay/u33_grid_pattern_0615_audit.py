#!/usr/bin/env python3
"""Pinned original/full-CPU/full-RAM comparison for U33 +0615."""
import argparse
import ctypes
from hashlib import sha256
from itertools import product
from pathlib import Path
import random
import re
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
from unicorn import x86_const as regs
from overlay_inventory import find_units, parse_mz
from u3c2_line_init_01a0_audit import Cpu, NAMES, REGS, SIZE, MASK, EXE_SHA, OVR_SHA, check, physical, put16

PINS = (
    (0x33, 0x12B, 0x1AD, 49, '8a4956ef566e0c033171d771c19d0dc48aaf4b7d8e04e99be34c6fe3e3f276ae'),
    (0x33, 0x1AD, 0x221, 54, '6250ebfdce5344654824482798dfad959558058bf0dbb53d9de6e1852e4835db'),
    (0x33, 0x221, 0x2BA, 64, '9a97dca1163b4aabd6193cea3a81427143709390e7b4aa2dec1a2a727236cd06'),
    (0x33, 0x615, 0x810, 196, 'c973402de9f97dbf004ece46254273f351e9a0a8600bc1f442dbbd9b1b4371e9'),
    (0x3BD, 0x31F, 0x350, 18, 'ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b'),
    (0x3BD, 0x350, 0x47B, 120, '3b0a525351d0a12b3afeed4c9d9d39e28fdf3564166cdab0d16219fad1b60498'),
    (0x3BD, 0x47B, 0x57A, 103, '92d75878d963a7f5766b1ea49e2f6c1fb51287775d06a5b6ca485fee8bc43658'),
)
SOURCES = ['overlay_u33_grid_pattern_0615', 'overlay_u33_cell_pair_01ad',
           'overlay_u33_cell_lookup_012b', 'overlay_u33_clipped_writer_0221',
           'overlay_u3bd_lookup_fullstate', 'overlay_u3bd_in_grid_031f']


def source(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE mismatch')
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR mismatch')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    check(units[0x33]['ovr_offset'] == 0xC5D1 and units[0x3BD]['ovr_offset'] == 0x2E8F7,
          'unit placement mismatch')
    check(units[0x3BD]['memory_segment'] == 0x3D5 and
          units[0x3BD]['entries'][2:4] == [0x47B, 0x350], 'lookup stub mismatch')
    bodies, decoded = {}, {}
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    for unit, start, end, count, digest in PINS:
        data = ovr[units[unit]['ovr_offset'] + start:units[unit]['ovr_offset'] + end]
        ins = list(decoder.disasm(data, start))
        check(sha256(data).hexdigest() == digest and len(ins) == count and
              sum(i.size for i in ins) == end - start, f'body mismatch {unit:x}:{start:x}')
        bodies[unit, start], decoded[unit, start] = data, ins
        print(f'U{unit:X}+{start:04X}: {len(data)} bytes/{count} instructions; SHA256 {digest}')
    caller = decoded[0x33, 0x615]
    calls = [(i.address, i.op_str) for i in caller if i.mnemonic == 'call']
    check(calls == [(0x631, '0x1ad'), (0x64D, '0x1ad'), (0x7D4, '0x221'),
                    (0x7E5, '0x221'), (0x7F6, '0x221'), (0x807, '0x221')], 'callee mismatch')
    by_site = {i.address: i for i in caller}
    check(all(by_site[site - 1].mnemonic == 'push' and by_site[site - 1].op_str == 'cs'
              for site, _ in calls), 'near-call frame mismatch')
    base = units[0x33]['ovr_offset']
    near = list(decoder.disasm(ovr[base + 0x88C:base + 0x890], 0x88C))
    check([(i.mnemonic, i.op_str) for i in near] == [('push', 'cs'), ('call', '0x615')],
          'incoming caller mismatch')
    for site in (0x155, 0x170):
        check(site + 3 in units[0x33]['fixups'], 'lookup fixup missing')
    root = Path(__file__).resolve().parents[2]
    annotations = re.findall(r'/\* \+([0-9A-F]{4}) \*/',
                             (root / 'src/recovered/overlay_u33_grid_pattern_0615.c').read_text())
    check([int(a, 16) for a in annotations] == [i.address for i in caller], 'source sites differ')
    print('PASS: six PUSH-CS callee edges, incoming +088D edge, two lookup fixups, 196 C sites')
    return bodies, decoded


def build(root, directory):
    path = directory / 'u33_0615.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-shared', '-fPIC', '-I', str(root / 'include')] +
                   [str(root / 'src/recovered' / (s + '.c')) for s in SOURCES] +
                   ['-o', str(path)], check=True)
    library = ctypes.CDLL(str(path))
    library.pr_u33_grid_pattern_0615.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                                                ctypes.POINTER(Cpu)]
    library.pr_u33_grid_pattern_0615.restype = ctypes.c_int
    return library


def fixture(*, results=(0, 0), category=0, other=0, x=3, y=4, a=0, b=0,
            frame=0x9000, ss=0x2000, sp=0x8000, ds=0x1000, cs=0x3000,
            lookup_cs=0x4000, stub_cs=0x3D5, grid=(0x6000, 0x100),
            buffer=(0x5000, 0x100), flags=0x7202, seed=None, map_id=5, party_y=3, return_seam=None):
    if return_seam is not None:
        sp = 0xFFFA if return_seam == 0xFFFE else 0xFFF9
        grid, category = (ss, sp - 0x25F), 2
    memory = bytearray([0xA5]) * SIZE
    rng = random.Random(seed)
    cpu = Cpu(0x1234, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777,
              sp, cs, 0x615, ds, 0x1111, ss, flags)
    memory[physical(ds, 0x84DC)] = map_id
    memory[physical(ds, 0x6AAE)] = party_y
    for offset, value in ((0x6A5C, buffer[1]), (0x6A5E, buffer[0]),
                          (0x6716, grid[1]), (0x6718, grid[0])):
        put16(memory, ds, offset, value)
    for i in range(0x10A):
        memory[physical(ds, 0x2736 + i)] = rng.randrange(256) if seed is not None else 0
    for i in range(0x400):
        memory[physical(buffer[0], buffer[1] + i)] = rng.randrange(256) if seed is not None else 0
    if seed is None:
        # Zero direction deltas make each real pair inspect one cell with
        # opposite selectors; encode results through genuine nibble/field data.
        for cx, cy, result in ((x, y - 1, results[0]), (x + 1, y, results[1])):
            if 0 <= cx < 16 and 0 <= cy < 16:
                cell = 16 * cy + cx
                for plane in (0, 0x100):
                    memory[physical(buffer[0], buffer[1] + cell + plane)] = 0 if result == 0 else 0x11
                memory[physical(buffer[0], buffer[1] + cell + 0x300)] = 0 if result == 1 else 0x55
    for off, value in ((-9, other), (-7, category), (-4, y), (-3, x), (-2, a), (-1, b)):
        memory[physical(ss, frame + off)] = value & 255
    for off, value in ((0, 0x7000), (2, cs), (4, frame)):
        put16(memory, ss, sp + off, value)
    if return_seam is not None:
        # Third glyph remains unassigned for category 2. Its write changes
        # the writer's saved BP; the fourth writer clips at its new frame.
        memory[physical(ss, sp - 5)] = (return_seam - 3) & 255
        put16(memory, ss, return_seam + 4, 0x9200)
        memory[physical(ss, 0x91FE)] = 0x80
        memory[physical(ss, 0x91FF)] = 0
        put16(memory, ss, return_seam + 2, 0x3333)
        p = physical(ss, return_seam)
        memory[(p + 2) & MASK], memory[(p + 3) & MASK] = 0x66, 0x77
    return memory, cpu, lookup_cs, stub_cs


def compare(label, initial, bodies, decoded, library, coverage, edges):
    memory, cpu, lookup_cs, stub_cs = initial
    instruction_map = {}
    for (unit, start), data in bodies.items():
        segment = cpu.cs if unit == 0x33 else lookup_cs
        for i, value in enumerate(data):
            memory[(physical(segment, start) + i) & MASK] = value
        for ins in decoded[unit, start]:
            instruction_map[physical(segment, ins.address)] = (unit, ins)
    # Relocated original call segment words and loaded far-jump stubs.
    for site, stub, entry in ((0x155, 0x2F, 0x350), (0x170, 0x2A, 0x47B)):
        put16(memory, cpu.cs, site + 3, stub_cs)
        memory[physical(stub_cs, stub)] = 0xEA
        put16(memory, stub_cs, stub + 1, entry)
        put16(memory, stub_cs, stub + 3, lookup_cs)
    stubs = {physical(stub_cs, 0x2A), physical(stub_cs, 0x2F)}
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
    status = library.pr_u33_grid_pattern_0615(c_ram, SIZE, ctypes.byref(c_cpu))
    check(status == stop[0], f'{label}: C status {status}, original {stop[0]}')
    for name, reg in zip(NAMES, REGS):
        actual, expected = getattr(c_cpu, name), original.reg_read(reg) & 65535
        check(actual == expected, f'{label}: {name} C={actual:04x}, original={expected:04x}')
    if bytes(c_ram) != bytes(backing):
        bad = [i for i, (a, b) in enumerate(zip(c_ram, backing)) if a != b]
        raise AssertionError(f'{label}: RAM differs at {[hex(i) for i in bad[:12]]}')
    return status


def run(bodies, decoded, library, random_cases):
    coverage, edges, counts = set(), set(), {}

    def case(label, **options):
        status = compare(label, fixture(**options), bodies, decoded, library, coverage, edges)
        counts[status] = counts.get(status, 0) + 1
        if sum(counts.values()) % 128 == 0:
            print(f'Progress: {sum(counts.values())} full CPU/RAM cases passed', flush=True)

    for r1, r2, category, other in product((0, 1, 3), (0, 1, 3), (0, 1, 2, 3, 255), (0, 1, 255)):
        case(f'decisions-{r1}-{r2}-{category}-{other}', results=(r1, r2), category=category, other=other)
    for a, b in product((-128, -127, -5, -1, 0, 1, 4, 5, 127), repeat=2):
        case(f'projection-{a}-{b}', a=a, b=b, results=(1, 3))
    for x, y in product((-128, -1, 0, 1, 14, 15, 16, 127), repeat=2):
        for map_id in (0, 5, 10):
            case(f'coordinate-{x}-{y}-{map_id}', x=x, y=y, map_id=map_id, party_y=y & 255)
    for sp in (0x80, 0x81, 0x82, 0xFFF9, 0xFFFA):
        case(f'stack-{sp:x}', sp=sp)
    for frame in (0, 1, 7, 8, 9, 0xFFFF):
        case(f'parent-wrap-{frame:x}', frame=frame)
    for frame in range(0x7F80, 0x8010):
        case(f'parent-stack-{frame:x}', frame=frame)
    for off in range(0x7DB0, 0x7E00):
        case(f'grid-stack-{off:x}', grid=(0x2000, off), category=1)
    for seam in (0xFFFE, 0xFFFF):
        case(f'actual-retf-seam-{seam:x}', return_seam=seam)
    case('A20-stack', ss=0xFFFF, sp=0x80)
    case('A20-parent', ss=0xFFFF, frame=0x10)
    case('A20-grid', grid=(0xFFFF, 0xFFE0))
    case('wrapped-buffer', buffer=(0x5000, 0xFFF0))
    case('relocated-caller', cs=0x3333)
    case('relocated-lookup', lookup_cs=0x5555, stub_cs=0x888)
    for flags in (0x202, 0x203, 0x602, 0xA02, 0xE03):
        case(f'flags-{flags:x}', flags=flags)
    rng = random.Random(0x330615)
    for i in range(random_cases):
        case(f'random-{i}', x=rng.randrange(256), y=rng.randrange(256),
             a=rng.randrange(256), b=rng.randrange(256), category=rng.randrange(4),
             other=rng.randrange(256), seed=i, map_id=rng.choice((0, 5, 10)))
    caller = decoded[0x33, 0x615]
    expected = {(0x33, i.address) for i in caller}
    check(expected <= coverage, f'uncovered caller sites: {sorted(expected - coverage)}')
    for (unit, start), ins in decoded.items():
        expected = {(unit, i.address) for i in ins}
        print(f'U{unit:X}+{start:04X}: {len(expected & coverage)}/{len(expected)} instructions')
    conditional = [i for i in caller if i.mnemonic.startswith('j') and i.mnemonic != 'jmp']
    expected_edges = {((0x33, i.address), (0x33, dest)) for i in conditional
                      for dest in (i.address + i.size, int(i.op_str, 16))}
    # +01AD returns only 0/1/3 before the loop; 0 and 3 were handled
    # before +06EB, leaving 1. At +0763, prior arms excluded 1/2/3
    # from the loop's fixed four values, leaving 4.
    infeasible = {((0x33, 0x6EB), (0x33, 0x707)), ((0x33, 0x763), (0x33, 0x7BD))}
    check(expected_edges - edges == infeasible, 'unexpected caller branch coverage gap')
    print('Caller missing branch edges:', [(hex(a[1]), hex(b[1])) for a, b in sorted(expected_edges - edges)])
    print(f'Caller branch edges: {len(expected_edges & edges)}/{len(expected_edges)}')
    print(f'PASS: {sum(counts.values())} full CPU/RAM cases; statuses {counts}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--ovr', required=True, type=Path)
    parser.add_argument('--random-cases', type=int, default=300)
    args = parser.parse_args()
    bodies, decoded = source(args.exe, args.ovr)
    with tempfile.TemporaryDirectory(prefix='poolrad-u33-0615-') as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == '__main__':
    main()
