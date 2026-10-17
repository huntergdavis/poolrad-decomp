#!/usr/bin/env python3
"""Full CPU/RAM original comparison for U33 +040F and its complete callees."""
import argparse
import ctypes
from hashlib import sha256
from itertools import product
from pathlib import Path
import random
import re
import subprocess
import tempfile
from types import SimpleNamespace

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from overlay_inventory import find_units, parse_mz
import u33_grid_pattern_0615_audit as peer

Cpu, NAMES, SIZE, MASK = peer.Cpu, peer.NAMES, peer.SIZE, peer.MASK
check, physical, put16 = peer.check, peer.physical, peer.put16
PINS = tuple(p for p in peer.PINS if p[:2] != (0x33, 0x615)) + (
    (0x33, 0x40F, 0x615, 209, '880652652d8a0222747e0f99b090418fc29d674e547b9809e0d5855a0f4bcc04'),)
SOURCES = ['overlay_u33_grid_pattern_040f'] + peer.SOURCES[1:]


def source(exe_path, ovr_path):
    exe, ovr = exe_path.read_bytes(), ovr_path.read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == peer.EXE_SHA, 'START.EXE mismatch')
    check(len(ovr) == 206105 and sha256(ovr).hexdigest() == peer.OVR_SHA, 'GAME.OVR mismatch')
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
    caller = decoded[0x33, 0x40F]
    calls = [(i.address, i.op_str) for i in caller if i.mnemonic == 'call']
    check(calls == [(0x42B, '0x1ad'), (0x448, '0x1ad'), (0x5D9, '0x221'),
                    (0x5EA, '0x221'), (0x5FB, '0x221'), (0x60C, '0x221')], 'callee mismatch')
    by_site = {i.address: i for i in caller}
    check(all(by_site[site - 1].mnemonic == 'push' and by_site[site - 1].op_str == 'cs'
              for site, _ in calls), 'near-call frame mismatch')
    base = units[0x33]['ovr_offset']
    near = []
    for f in units[0x33]['functions']:
        ins = list(decoder.disasm(ovr[base + f['rel']:base + f['rel'] + f['size_upper_bound']], f['rel']))
        for i, op in enumerate(ins):
            if op.mnemonic == 'call' and op.op_str == '0x40f':
                check(i > 0 and (ins[i - 1].mnemonic, ins[i - 1].op_str) == ('push', 'cs'),
                      'incoming frame mismatch')
                near.append((f['rel'], op.address))
    check(near == [(0x810, 0x888)], f'incoming callers differ: {near}')
    for site in (0x155, 0x170):
        check(site + 3 in units[0x33]['fixups'], 'lookup fixup missing')
    root = Path(__file__).resolve().parents[2]
    annotations = re.findall(r'/\* \+([0-9A-F]{4}) \*/',
                             (root / 'src/recovered/overlay_u33_grid_pattern_040f.c').read_text())
    check([int(a, 16) for a in annotations] == [i.address for i in caller], 'source sites differ')
    print('PASS: six PUSH-CS callee edges, incoming +0888 edge, two lookup fixups, 209 C sites')
    return bodies, decoded


def build(root, directory):
    path = directory / 'u33_040f.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-shared', '-fPIC', '-I', str(root / 'include')] +
                   [str(root / 'src/recovered' / (s + '.c')) for s in SOURCES] +
                   ['-o', str(path)], check=True)
    library = ctypes.CDLL(str(path))
    function = library.pr_u33_grid_pattern_040f
    function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    function.restype = ctypes.c_int
    # The shared oracle only uses this attribute to invoke its candidate.
    return SimpleNamespace(pr_u33_grid_pattern_0615=function, library=library)


def fixture(*, results=(0, 0), category=0, secondary=0, return_seam=None, **options):
    if return_seam is not None:
        sp = 0xFFFA if return_seam == 0xFFFE else 0xFFF9
        options.update(sp=sp, grid=(options.get('ss', 0x2000), sp - 0x259))
        category, secondary = 2, 2
    initial = peer.fixture(results=(0, 0), category=category, **options)
    memory, cpu, _, _ = initial
    cpu.ip = 0x40F
    frame = options.get('frame', 0x9000)
    memory[physical(cpu.ss, frame - 8)] = secondary & 255
    x, y = options.get('x', 3), options.get('y', 4)
    buffer = options.get('buffer', (0x5000, 0x100))
    if options.get('seed') is None:
        # Facing 6 at (x,y-1), then only if zero facing 0 at (x-1,y).
        # Zero direction deltas make each pair inspect the same cell twice.
        for cx, cy, result in ((x, y - 1, results[0]), (x - 1, y, results[1])):
            if 0 <= cx < 16 and 0 <= cy < 16:
                cell = 16 * cy + cx
                for plane in (0, 0x100):
                    memory[physical(buffer[0], buffer[1] + cell + plane)] = 0 if result == 0 else 0x11
                memory[physical(buffer[0], buffer[1] + cell + 0x300)] = 0 if result == 1 else 0x55
    if return_seam is not None:
        memory[physical(cpu.ss, cpu.sp - 5)] = (return_seam - 3) & 255
        put16(memory, cpu.ss, return_seam + 4, 0x9200)
        memory[physical(cpu.ss, 0x91FE)] = 0x80
        memory[physical(cpu.ss, 0x91FF)] = 0
        put16(memory, cpu.ss, return_seam + 2, 0x3333)
        p = physical(cpu.ss, return_seam)
        memory[(p + 2) & MASK], memory[(p + 3) & MASK] = 0x66, 0x77
    return initial


def run(bodies, decoded, library, random_cases):
    coverage, edges, counts = set(), set(), {}

    def case(label, **options):
        status = peer.compare(label, fixture(**options), bodies, decoded, library, coverage, edges)
        counts[status] = counts.get(status, 0) + 1
        if sum(counts.values()) % 128 == 0:
            print(f'Progress: {sum(counts.values())} full CPU/RAM cases passed', flush=True)

    for r1, r2, category, secondary in product((0, 1, 3), (0, 1, 3), (0, 1, 2, 3, 255), (0, 1, 2, 3, 255)):
        case(f'decisions-{r1}-{r2}-{category}-{secondary}', results=(r1, r2),
             category=category, secondary=secondary)
    for a, b in product((-128, -127, -5, -1, 0, 1, 4, 5, 127), repeat=2):
        case(f'projection-{a}-{b}', a=a, b=b, results=(0, 3))
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
    rng = random.Random(0x33040F)
    for i in range(random_cases):
        case(f'random-{i}', x=rng.randrange(256), y=rng.randrange(256),
             a=rng.randrange(256), b=rng.randrange(256), category=rng.randrange(4),
             secondary=rng.randrange(4), seed=i, map_id=rng.choice((0, 5, 10)))
    caller = decoded[0x33, 0x40F]
    expected = {(0x33, i.address) for i in caller}
    check(expected <= coverage, f'uncovered caller sites: {sorted(expected - coverage)}')
    for (unit, start), ins in decoded.items():
        expected = {(unit, i.address) for i in ins}
        print(f'U{unit:X}+{start:04X}: {len(expected & coverage)}/{len(expected)} instructions')
    conditional = [i for i in caller if i.mnemonic.startswith('j') and i.mnemonic != 'jmp']
    expected_edges = {((0x33, i.address), (0x33, dest)) for i in conditional
                      for dest in (i.address + i.size, int(i.op_str, 16))}
    # The four-pass loop's 1/2/3 arms leave only 4 at +0565.
    infeasible = {((0x33, 0x565), (0x33, 0x5C2))}
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
    with tempfile.TemporaryDirectory(prefix='poolrad-u33-040f-') as directory:
        library = build(Path(__file__).resolve().parents[2], Path(directory))
        run(bodies, decoded, library, args.random_cases)


if __name__ == '__main__':
    main()
