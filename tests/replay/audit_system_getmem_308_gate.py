#!/usr/bin/env python3
"""Bounded allocator selection with explicit native and raw-Unicorn flag profiles."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import random
import re
import sys
import time
import unicorn
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/replay'))
import audit_system_getmem_23f_abi as prior
from audit_slot_a_string_0356_abi import CPU, State, NAMES, cpu_values, compare_cpu
mapped = prior.mapped
Report = prior.Report
NATURAL = '0308 0491 0494 0496 0498 049a 049c 049e 04a1 0312 0363 19ba'.split()
AF0 = ['af0-' + n for n in ('0498', '049a', '049c', '049e', '04a1')]
CONTROLLED = [n + '-' + p for n in ('exact', 'growth', 'exhaust', 'zero') for p in ('in', 'out')]
LABELS = NATURAL + AF0 + CONTROLLED
GROUPS = {
    '76c80e3cdb35f7d54c651822caa4a18583fa3ecd272b4b8b591c6976a639ba4c': '0308',
    'cd62f5fd85269dbcb4df140a2594a8cdc19f611d0dab3d55ce99cf5cc9bac2ed':
        '0491 0494 0496 0498 049a 049c 049e 04a1 0312',
    '7a377b1835c3560d94cbb706ff17133a7e926d6a0070c67f2bdf367c577276c6': '0363',
    'ff7ba445aed7574d62a164cb0fdd660c9dbc291c1a1de63dc929a38c248e5442':
        '19ba af0-0498 af0-049a af0-049c af0-049e af0-04a1',
    '700faf43647ef4cdf058c0ca9fbf55b95ba46af0d982bc2a9c3772e19eb485a9': 'exact-in',
    'ec53346f5359ff3c966e9dec624f3b57322c13b05dbb7731031b187285737a12': 'exact-out',
    '963bd0bdf21d31d5f9119585f3865c2345063500434c4458db5edba254f4df0e': 'growth-in',
    '281bac33cdd07204efa521d3d7e5cb77155d2693da63815e57bc24c574cc6148': 'growth-out',
    'bab1f83aa15f91e6bbafb5d324eb8b43320373e76a2fa7036ed8abd265ffdffa': 'exhaust-in',
    '8fb9b7a668c3fc1a4566e3394d819f2f8552b7d2acd5feffda87a6453640514f': 'exhaust-out',
    '06626309f406fbd5f4857d3b53f260e0b56e41301d09960c0251dc40fa1c7b70': 'zero-in zero-out',
}


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == 'b53f000fc0099b6e3bed113b7de83a14d83bb0f9d7cf755d510dd6a5c2e427e0'
    clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", clean)
    hashes = {n: h for h, names in GROUPS.items() for n in names.split()}
    assert len(rows) == len(hashes) == len(LABELS) == 25 and set(hashes) == set(LABELS)
    cpus = {n: [int(v, 16) for v in row.split()] for n, row in zip(LABELS, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, hashes[n]) for n in LABELS}
    start = mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    spans = ((0x308, 203, '03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0'),
             (0x491, 17, '440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d'))
    for name, values in cpus.items():
        assert len(values) == 16 and values == [json.loads((root / f'cpu-{name}.json').read_text())[n] for n in NAMES]
        assert values[0] == (0x2A8E if name == '19ba' else 0x131C)
        for off, size, digest in spans:
            body = start[0xAC76+off:0xAC76+off+size]
            assert sha256(body).hexdigest() == digest
            assert images[name][0x131C0+off:0x131C0+off+size] == body
    # Incoming AF=0 must become 1: preserving AF would fail this evidence.
    assert cpus['af0-0498'][-1] == 0x7206
    assert [cpus['af0-'+n][-1] for n in ('049a', '049c', '049e', '04a1')] == [0x7212, 0x7217, 0x7216, 0x7246]
    for name in ('exact', 'growth', 'exhaust', 'zero'):
        assert cpus[name+'-in'][2] == 0x6000 and cpus[name+'-in'][7] == 0x37AE
    assert cpus['exact-out'][:2] == [0x131C, 0x376]
    assert cpus['growth-out'][:2] == [0x131C, 0x3B7]
    assert cpus['exhaust-out'][:2] == [0x131C, 0x3C3]
    assert cpus['zero-out'][:2] == [0x131C, 0x248]
    assert cpus['19ba'][9] == 8 and cpus['19ba'][12] == 0x449A
    return cpus, images


def differential(lib, name, image, values, terminal=None, status_expected=None,
                 checkpoints=(), limit=500, single=False, quiet=False, oracle=None):
    memories = [ct.create_string_buffer(image, len(image)) for _ in range(2)]
    states = [State(CPU(**dict(zip(NAMES, values))), ct.cast(m, ct.POINTER(ct.c_uint8)), len(image))
              for m in memories]
    uc = oracle if oracle is not None else mapped.emulator(image, values)
    if oracle is not None:
        # Reset the entire input before this independent case, never mid-case.
        uc.mem_write(0, image)
        for register, value in zip(mapped.REGS, values):
            uc.reg_write(register, value)
    pending = list(checkpoints); seen = disagreements = 0; started = time.monotonic()
    for steps in range(limit):
        if time.monotonic() - started >= 10:
            raise TimeoutError(f'{name}: deadline at step {steps}')
        pc = (states[0].cpu.cs, states[0].cpu.ip)
        if terminal == pc and status_expected is None:
            assert not pending
            if not quiet:
                print(f'{name}: {steps} raw-profile full-state comparisons; {seen} native-profile '
                      f'checkpoints; {disagreements} explicit cross-profile AF differences; boundary reached', flush=True)
            return steps, seen, disagreements
        reports = [Report(), Report()]
        before = [(cpu_values(s.cpu), m.raw) for s, m in zip(states, memories)]
        statuses = [lib.pr_system_getmem_308_step(ct.byref(s), profile, ct.byref(r))
                    for profile, s, r in zip((1, 2), states, reports)]
        if pc == terminal:
            assert statuses == [status_expected] * 2 and not pending, (name, pc, statuses)
            for s, m, r, (old_cpu, old_ram) in zip(states, memories, reports, before):
                assert cpu_values(s.cpu) == old_cpu and m.raw == old_ram
                if status_expected == 3:
                    p = (s.cpu.ds << 4) + 0x4414
                    target = int.from_bytes(m.raw[p:p+2], 'little')
                    segment = int.from_bytes(m.raw[p+2:p+4], 'little')
                    assert (r.dependency_valid, r.call_is_near, r.call_pc, r.callee_segment,
                            r.callee_pc, r.return_pc) == (1, 0, pc[1], segment, target, pc[1]+4)
                else:
                    assert not r.dependency_valid
            if not quiet:
                print(f'{name}: {steps} raw-profile full-state comparisons; {seen} native-profile '
                      f'checkpoints; {disagreements} explicit cross-profile AF differences; gate={status_expected}', flush=True)
            return steps, seen, disagreements
        assert statuses == [0, 0], (name, pc, statuses)
        assert all(not r.dependency_valid for r in reports)
        mapped.one(uc)
        raw = mapped.values(uc)
        compare_cpu(cpu_values(states[1].cpu), raw, f'{name} raw profile after {pc}')
        native = cpu_values(states[0].cpu)
        assert native[:-1] == raw[:-1], (name, pc, 'native profile registers')
        if native[-1] != raw[-1]:
            # Exact expected negative evidence; neither flags value is masked or rewritten.
            assert native[:2] in ([0x131C, 0x49A], [0x131C, 0x49C], [0x131C, 0x49E])
            assert native[-1] == raw[-1] + 0x10 and not (raw[-1] & 0x10)
            disagreements += 1
        ram = bytes(uc.mem_read(0, len(image)))
        assert memories[0].raw == memories[1].raw == ram, (name, pc, 'RAM')
        if pending and native[:2] == pending[0][1][:2]:
            label, expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(native, expected_cpu, label)
            assert memories[0].raw == expected_ram, label
            seen += 1
        if single:
            assert not pending
            return steps+1, seen, disagreements
    raise AssertionError(f'{name}: instruction bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_system_getmem_308_gate.py LIBRARY NATIVE_CORE_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus, images = inputs(root)
    lib = ct.CDLL(str(library))
    lib.pr_system_getmem_308_step.argtypes = [ct.POINTER(State), ct.c_int, ct.POINTER(Report)]
    lib.pr_system_getmem_308_step.restype = ct.c_int
    oracle = mapped.emulator(images['0308'], cpus['0308'])
    total = native = differences = 0
    def run(*args, **kwargs):
        nonlocal total, native, differences
        n, c, d = differential(lib, *args, **kwargs, oracle=oracle)
        total += n; native += c; differences += d
    run('natural core to split-tail gate', images['0308'], cpus['0308'], (0x131C, 0x363), 1,
        [(n, cpus[n], images[n]) for n in NATURAL[1:-1]])
    run('controlled native incoming AF=0', images['af0-0498'], cpus['af0-0498'], (0x131C, 0x4A1),
        checkpoints=[(n, cpus[n], images[n]) for n in AF0[1:]])
    for name, terminal, status in (('exact', 0x376, 1), ('growth', 0x3B7, 3),
                                    ('exhaust', 0x3C3, 3), ('zero', 0x248, 2)):
        run('controlled native '+name, images[name+'-in'], cpus[name+'-in'], (0x131C, terminal), status,
            [(name+'-out', cpus[name+'-out'], images[name+'-out'])])
    # Synthetic cases are independent original-byte tests, never natural native claims.
    for size in (0, 1, 7, 8, 9, 15, 16, 17, 127, 128, 255, 256, 285,
                 0x7FFF, 0x8000, 0xFFF7, 0xFFF8, 0xFFF9, 0xFFFE, 0xFFFF):
        values = cpus['0491'].copy(); values[9] = size; values[-1] = 0x7ED7
        run(f'synthetic normalizer {size:04X}', images['0491'], values, (0x131C, 0x312), quiet=True)
    print('20 independent synthetic normalizer sizes passed', flush=True)
    for name, request, available, limit_words, terminal in (
        ('split equal high', (0, 0x12), (8, 0x12), None, 0x363),
        ('split borrow', (8, 0x12), (0, 0x13), None, 0x363),
        ('exact offset', (8, 0x12), (8, 0x12), None, 0x376),
        ('skip short offset', (8, 0x12), (0, 0x12), None, 0x3B7),
        ('skip short high', (0, 0x13), (0, 0x12), None, 0x3B7),
        ('growth equal limit', (0, 0x12), None, (0, 0x6212), 0x3B7),
        ('growth equal high smaller offset', (0, 0x12), None, (8, 0x6212), 0x3B7),
        ('exhaust offset', (8, 0x12), None, (0, 0x6212), 0x3C3),
    ):
        base = 'exact-in' if available else 'growth-in'
        image = bytearray(images[base]); values = cpus[base].copy()
        values[9], values[12] = request
        if available:
            image[0x61004:0x61008] = b''.join(v.to_bytes(2, 'little') for v in available)
        if limit_words:
            image[0x64408:0x6440C] = b''.join(v.to_bytes(2, 'little') for v in limit_words)
        run('synthetic '+name, bytes(image), values, (0x131C, terminal), 1 if terminal in (0x363, 0x376) else 3)
    rng = random.Random(0x3080491)
    pcs = (0x308, 0x34D, 0x34F, 0x351, 0x354, 0x356, 0x358, 0x35A, 0x35D,
           0x383, 0x385, 0x387, 0x389, 0x38B, 0x38D, 0x390, 0x3B4,
           0x491, 0x496, 0x498, 0x49A, 0x49C, 0x49E)
    boundaries = (0, 1, 7, 8, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF)
    for pc in pcs:
        for i in range(12):
            values = cpus['0308'].copy(); values[1] = pc
            for reg in (9, 10, 11, 12, 13, 14):
                values[reg] = rng.choice(boundaries) if i < 9 else rng.randrange(65536)
            values[-1] = rng.choice((0x7202, 0x7ED7))
            run(f'synthetic arithmetic {pc:04X}/{i}', images['0308'], values, single=True, quiet=True)
    print(f'{len(pcs)*12} independent synthetic arithmetic instructions passed', flush=True)
    image = bytearray(images['0308']); values = cpus['0308'].copy()
    values[1], values[3], values[14] = 0x31D, 0, 0x200
    image[0x200:0x204] = b'\xff\xff\x1c\x13'
    run('synthetic LES operand order', bytes(image), values, single=True)
    image = bytearray(images['exhaust-in']); values = cpus['exhaust-in'].copy()
    values[1], values[2], values[6], values[7] = 0x3BF, 0, 0, 0x8756
    image[0x8754:0x8756] = b'\xfe\xca'
    run('synthetic hook argument aliases push', bytes(image), values, (0x131C, 0x3C3), 3)
    values = cpus['growth-out'].copy()
    values[1], values[3], values[7], values[9], values[12], values[14] = 0x3A6, 0x6000, 0x1000, 8, 0x6212, 0x4406
    run('synthetic predecessor aliases heap globals', images['growth-out'], values, (0x131C, 0x3B7), 3)
    image = bytearray(images['exact-in'])
    image[0x61000:0x61008] = b'\x00\x00\x00\x61\x00\x00\x00\x00'
    try:
        differential(lib, 'synthetic cyclic free list', bytes(image), cpus['exact-in'], limit=40, oracle=oracle)
    except AssertionError as error:
        assert str(error) == 'synthetic cyclic free list: instruction bound exhausted'
    else:
        raise AssertionError('cyclic list escaped its instruction bound')
    print('cyclic free list rejected at 40-instruction bound (outside positive total)', flush=True)
    print(f'Allocator core gate audit passed: {total} raw-profile full-state instruction comparisons; '
          f'{native} native-profile checkpoints; {differences} retained cross-profile AF differences; '
          '25 pinned rows (12 natural, 13 controlled); unknown hooks remain blocked', flush=True)


if __name__ == '__main__':
    main()
