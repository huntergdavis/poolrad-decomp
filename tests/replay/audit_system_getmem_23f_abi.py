#!/usr/bin/env python3
"""Separate GetMem ABI/store/split-tail parity; no allocator outcome bridge."""
import ctypes as ct
from hashlib import sha256
import json
from pathlib import Path
import re
import sys
import time
import unicorn
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/replay'))
import audit_slot_a_delete_abi_map as mapped
from audit_slot_a_string_0356_abi import CPU, State, NAMES, cpu_values, compare_cpu

class Report(ct.Structure):
    _fields_ = [('dependency_valid', ct.c_uint), ('call_is_near', ct.c_uint),
                ('call_pc', ct.c_uint16), ('callee_segment', ct.c_uint16),
                ('callee_pc', ct.c_uint16), ('return_pc', ct.c_uint16)]

LABELS = ('19b5 023f 0240 0245 0308 030c 030f 0491 0498 049e 0312 '
          '031d-first 0326-first 031d-second 0326-second 031d-third 0326-third '
          '0336 0343 0351 0363 0366 036a 036e 0372 0376 0378 037b 037f 0382 0248 024b 19ba').split()
GROUPS = {
    '1ca6a13b9b1e061f5b31e993f7a3717566d28393dc16e87c1f29328d59e4ce0e': '19b5',
    '2b70cdea067c68e5bbe503120b231e5d4ca5a9967102fb46e1d6c54e9032b940': '023f',
    '241195a41f1a31d1b75054eebd748d863b66a479512c77a0d0201a21f0375454': '0240 0245',
    '91646938bddec182973563d586d894daa53d9d878248fb67cd90f6de550c8a79': '0308 030c',
    'fc0cb0f744fbea45251ed4987451e5898afe27905c15627fce1e19c6d734cc62': '030f',
    '8eaa3ba6516c39712402c09e3d5630e1c68241a2b08f924bc0becbf62b4b7e87':
        '0491 0498 049e 0312 031d-first 0326-first 031d-second 0326-second 031d-third 0326-third 0336',
    '0d157aa55c64c74d313049dc1c8f58a5177f5a9d312aed946ce3591b190ccb7a': '0343',
    'b8e76cb40a3ad27a9397e66434499e0accac07dd82d60a8ce4f90e3c05e076d0': '0351 0363',
    'f041a6ef33e9e67903f7ee775090f81809d0d6597eb6b0ffb965d8923b8ea270': '0366',
    '868c7484eb5980311da1a995536713a96b7adaa9dc88db85093b05247299425b': '036a',
    'f131abcd8403db4f1192b36eb250e19e022a1a3056dd8e13bfb3206b298998f5': '036e',
    '0fc96f1d2ee8a56d5ed352630fca2dc95be8945131aa2e5582a60a779634fa0d': '0372 0376 0378 037b',
    'bbf5f620dd74e4bce0be4ac45eeacf819ee9b9aefd175ea0703771e0efda6f6f': '037f 0382 0248 024b 19ba',
}


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == 'f5fe196a15c9cf3ba8a9af361070d7a5a363b346eee234c8b5db1207f2d48997'
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", text)
    hashes = {n: h for h, names in GROUPS.items() for n in names.split()}
    assert len(rows) == len(hashes) == len(LABELS) == 33 and set(hashes) == set(LABELS)
    cpus = {n: [int(x, 16) for x in row.split()] for n, row in zip(LABELS, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, hashes[n]) for n in LABELS}
    start = mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    bodies = []
    for pc, size, digest in (
        (0x23F, 21, 'd4ceab56ec05612e8ba6c90dab11d9da62f1f38235503781f80d636daa63d346'),
        (0x308, 203, '03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0'),
        (0x491, 17, '440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d'),
    ):
        b = start[0xAC76+pc:0xAC76+pc+size]
        assert sha256(b).hexdigest() == digest
        bodies.append((pc, b))
    caller = game[0x1DFE6+0x19B1:0x1DFE6+0x19BA]
    assert sha256(caller).hexdigest() == '27678668b204ad2d84785e89897bf20454582a70e1f7e34bd363cab31373a5b2'
    caller = bytearray(caller); caller[7:9] = (0x131C).to_bytes(2, 'little')
    assert sha256(caller).hexdigest() == '963c38901cd3f9bc0c0473f7e06cb17e4f7559edff39d3d1827cfd9d489092f9'
    for n, v in cpus.items():
        assert len(v) == 16 and v[:2] == [0x2A8E if n in ('19b5', '19ba') else 0x131C, int(n.split('-')[0], 16)]
        assert v == [json.loads((root / f'cpu-{n}.json').read_text())[f] for f in NAMES]
        assert images[n][0x2A8E0+0x19B1:0x2A8E0+0x19BA] == caller
        for pc, body in bodies:
            assert images[n][0x131C0+pc:0x131C0+pc+len(body)] == body
    word = lambda n, off: int.from_bytes(images[n][0x1D160+off:0x1D160+off+2], 'little')
    for n, off, values in (
        ('023f', 0x37B2, [0x19BA, 0x2A8E, 0x11D]),
        ('0308', 0x37AE, [0x248, 0x3DCA, 0x19BA, 0x2A8E]),
        ('0491', 0x37AC, [0x312, 0x248, 0x3DCA]),
        ('0363', 0x37A2, [8, 0x44B2, 0, 0x42AB, 8, 0x449A, 0x248, 0x3DCA, 0x19BA, 0x2A8E, 0x11D]),
    ):
        assert [word(n, off+2*i) for i in range(len(values))] == values
    assert [cpus['0326-'+n][3] for n in ('first', 'second', 'third')] == [0x41DF, 0x42AB, 0x449A]
    assert cpus['19ba'][9] == 8 and cpus['19ba'][12] == 0x449A and cpus['19ba'][7] == 0x37B8
    return cpus, images


def native_checks(cpus, images):
    changed = lambda a, b: {i for i, (x, y) in enumerate(zip(images[a], images[b])) if x != y}
    for a, b, expected in (
        ('030c', '030f', {0x1D154, 0x1D155}),
        ('0363', '0366', {0x44AC8}), ('0366', '036a', {0x44ACA, 0x44ACB}),
        ('036a', '036e', {0x44ACC, 0x44ACD}), ('036e', '0372', {0x44ACE, 0x44ACF}),
        ('0378', '037b', set()), ('037b', '037f', {0x42AB2}),
    ):
        assert changed(a, b) == expected, (a, b)
    assert len(changed('19b5', '0245')) == 6
    assert len(changed('0363', '19ba')) == 8
    assert len(changed('19b5', '19ba')) == 30
    for first, last in (('0245', '0308'), ('030f', '0491')):
        uc = mapped.emulator(images[first], cpus[first]); mapped.one(uc)
        compare_cpu(mapped.values(uc), cpus[last], 'original-only near-call frame')
        assert bytes(uc.mem_read(0, 655360)) == images[last]
    # Preserve the raw disagreement. No flag write/mask or C-core match claim.
    uc = mapped.emulator(images['0498'], cpus['0498'])
    for _ in range(3):
        mapped.one(uc)
    raw, native = mapped.values(uc), cpus['049e']
    assert raw[:-1] == native[:-1] and raw[-1] ^ native[-1] == 0x10
    assert bytes(uc.mem_read(0, 655360)) == images['049e']
    print(f'Normalizer outside C scope: raw Unicorn FLAGS={raw[-1]:04X}, '
          f'native={native[-1]:04X}; undefined AF disagreement retained, no masking', flush=True)


def differential(lib, label, image, values, terminal, status_expected, checkpoints=()):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    report = Report(); uc = mapped.emulator(image, values)
    pending = list(checkpoints); seen = 0; started = time.monotonic()
    for steps in range(100):
        if time.monotonic() - started >= 10:
            raise TimeoutError(f'{label}: deadline at C step {steps}')
        pc = (state.cpu.cs, state.cpu.ip)
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_system_getmem_23f_step(ct.byref(state), ct.byref(report))
        if pc == terminal:
            assert status == status_expected and not pending, (label, pc, status)
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            if status in (1, 2):
                target, ret = (0x308, 0x248) if status == 1 else (0x491, 0x312)
                assert (report.dependency_valid, report.call_is_near, report.call_pc,
                        report.callee_segment, report.callee_pc, report.return_pc) == (1, 1, pc[1], 0xAF8, target, ret)
            else:
                assert not report.dependency_valid
            print(f'{label}: {steps} C/original-byte full-state comparisons; '
                  f'{seen} native checkpoints; blocked={status}', flush=True)
            return steps, seen, cpu_values(state.cpu)
        assert status == 0 and not report.dependency_valid, (label, pc, status)
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu), mapped.values(uc), f'{label} after {pc}')
        assert memory.raw == bytes(uc.mem_read(0, len(image))), (label, pc, 'RAM')
        if pending and cpu_values(state.cpu)[:2] == pending[0][1][:2]:
            name, expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, name)
            assert memory.raw == expected_ram, name
            seen += 1
    raise AssertionError(f'{label}: C instruction bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_system_getmem_23f_abi.py LIBRARY NATIVE_SPLIT_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(x).resolve(strict=True) for x in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus, images = inputs(root); native_checks(cpus, images)
    lib = ct.CDLL(str(library))
    lib.pr_system_getmem_23f_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_system_getmem_23f_step.restype = ct.c_int
    total = native = 0
    # Three independent inputs. Never replace a blocked state with a snapshot.
    for label, first, last, status, stops in (
        ('natural ABI prefix', '19b5', '0245', 1, ('023f', '0240', '0245')),
        ('independent requested-size store', '030c', '030f', 2, ('030f',)),
        ('independent split tail', '0363', '19ba', 4,
         ('0366', '036a', '036e', '0372', '0376', '0378', '037b', '037f', '0382', '0248', '024b', '19ba')),
    ):
        steps, count, result = differential(lib, label, images[first], cpus[first], tuple(cpus[last][:2]), status,
                                             [(n, cpus[n], images[n]) for n in stops])
        total += steps; native += count
        if first == '0363':
            assert (result[12], result[9]) == (0x449A, 8)
    # Synthetic cases appended below; all remain independent fragment inputs.
    for size, sp in ((0, 0x37B6), (0xFFFF, 0x37B6), (0x11D, 0)):
        image = bytearray(images['19b5']); values = cpus['19b5'].copy()
        values[7] = sp; values[9] = 0xA55A; values[-1] = 0x7ED7
        pos = (values[6] << 4) + sp; image[pos:pos+2] = size.to_bytes(2, 'little')
        steps, _, _ = differential(lib, f'synthetic wrapper size={size:04X}/SP={sp:04X}', bytes(image), values,
                                   (0x131C, 0x245), 1)
        total += steps
    for value in (0, 0xFFFF):
        values = cpus['030c'].copy(); values[9] = value; values[-1] = 0x7ED7
        steps, _, _ = differential(lib, f'synthetic requested-size {value:04X}', images['030c'], values,
                                   (0x131C, 0x30F), 2)
        total += steps
    for label in ('alternate-live-return', 'stale-flags', 'header-next-stack-alias',
                  'header-predecessor-alias', 'header-high-offset'):
        image = bytearray(images['0363']); values = cpus['0363'].copy()
        base = values[6] << 4; sp = values[7]
        if label == 'alternate-live-return':
            image[base+sp+8:base+sp+12] = b'\x0c\x00\x00\x50'
        elif label == 'stale-flags':
            values[-1] = 0x7ED7
        elif label == 'header-next-stack-alias':
            values[3], values[14] = values[6], sp+2
        elif label == 'header-predecessor-alias':
            values[14] = int.from_bytes(image[base+sp+4:base+sp+6], 'little')
            values[3] = int.from_bytes(image[base+sp+6:base+sp+8], 'little')
        else:
            values[3], values[14] = 0x3000, 0xFFF8
        steps, _, result = differential(lib, 'synthetic split tail '+label, bytes(image), values,
                                        (0x2A8E, 0x19BA), 4)
        if label == 'alternate-live-return':
            assert (result[12], result[9]) == (0x5000, 0x000C)
        total += steps
    values = cpus['0248'].copy(); values[-1] |= 1
    steps, _, result = differential(lib, 'synthetic carry-error wrapper', images['0248'], values,
                                    (0x131C, 0xE2), 3)
    assert result[9] == 0xCB
    total += steps
    print(f'GetMem ABI audit passed: {total} C instruction comparisons; {native} native C checkpoints '
          'across three independent fragments; 33 pinned native rows; core/normalizer remain blocked', flush=True)


if __name__ == '__main__':
    main()
