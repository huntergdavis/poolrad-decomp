#!/usr/bin/env python3
"""Actual far-hook ABI; controlled inputs, no fabricated hook return outcomes."""
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
import audit_system_getmem_23f_abi as prior
from audit_slot_a_string_0356_abi import CPU, State, NAMES, cpu_values, compare_cpu
mapped, Report = prior.mapped, prior.Report
GROWTH = ['growth-03b7', 'growth-thunk'] + ['growth-'+p for p in
    '0042 0043 0045 0048 004c 00a3 00a6 00a8 00a9 03bb 03bc 03bd 03be 0248'.split()]
EXHAUST = ['exhaust-03c3', 'exhaust-thunk'] + ['exhaust-'+p for p in
    '0042 0043 0045 0048 004c 004e 0052 0054 0055 0058 0059 005a 005d 005e 005f'.split()]
LABELS = ['natural-0308', 'growth-0308'] + GROWTH + ['exhaust-0308'] + EXHAUST
GROUPS = {'0299c91fc63f7904a6079b28a0bf800bf8d78d0a69266cb6ea1bec34f4129d79': 'growth-0308',
 '122c3317f753861fd4b0214704bc395bb035e5ff7f81179558d3ba24ee575b42': 'exhaust-03c3',
 '1c0e72e3ad1a1bfc3ac5b949835b3ef94c3eaeaf04e6da617c7fe24510c957ea': 'natural-0308',
 '228a2287ab2f2ff7cb9f43efea5d68a21b511199f55818a62fd93d40cfc634c3': 'exhaust-005a exhaust-005d',
 '26f9f645aa262b7959e88f8828ba570a4482699c4b1eb33324bbaf88a7d75d61': 'exhaust-0042 exhaust-thunk',
 '2aed36b3a4df09bd45ff742e3a41409b78c3a76fce4b08c9197deb79184a6543': 'exhaust-0052 exhaust-0054',
 '3ed24a60d20f51c9f3cbac7240d3bc7d409d08341be77b77e0f418f01eaaad9a': 'exhaust-0059',
 '52a1a627ee5e33d6e31ffcb41b149682014162d415ef7f7d0c8767f8b42ae63a': 'growth-03b7',
 '68b0be831679b1a60027928bf6db47df33df39bcb9b40c292d5b8fb61f0b3772': 'exhaust-0043 exhaust-0045 exhaust-0048 '
                                                                     'exhaust-004c exhaust-004e',
 '724d86e67ab03686fd714e0a8317c1d70caf7051bfdf186ba37a24cc058582c5': 'exhaust-005e',
 '7791f220b3daa1358eb243c4f740929e8dbd747972f2c5f5a17fb86707017d28': 'exhaust-0055 exhaust-0058',
 'ade63f99ff16a699235bb74e5618ad91b606db13f4aceeb226724056d2f9cf68': 'exhaust-0308',
 'cc3b016c50ba2dff7e32b12b9c44f7b29709436c9a17434200a49a8d6b056fd8': 'growth-0043 growth-0045 growth-0048 '
                                                                     'growth-004c growth-00a3 growth-00a6 '
                                                                     'growth-00a8 growth-00a9 growth-0248 '
                                                                     'growth-03bb growth-03bc growth-03bd '
                                                                     'growth-03be',
 'f5d7fb6c012e7c2ea41613320c1d6c56da1bbf8ecfd610222619e21e8d4882fb': 'growth-0042 growth-thunk',
 'fcf29e7bb1cdc7b8715197818d0c3a56144844b37a1dff22037a377c46663ffa': 'exhaust-005f'}


def word(image, ss, off):
    p = (ss << 4) + off
    return int.from_bytes(image[p:p+2], 'little')


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '63bbc99da636f5ea4ad2cf593937ee76c26a28d203eb7a51a3e2d189dc48113b'
    clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", clean)
    hashes = {n: h for h, names in GROUPS.items() for n in names.split()}
    assert len(rows) == len(hashes) == len(LABELS) == 36 and set(hashes) == set(LABELS)
    cpus = {n: [int(v, 16) for v in row.split()] for n, row in zip(LABELS, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, hashes[n]) for n in LABELS}
    start = mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    body = bytearray(game[0x4A:0x4A+106])
    assert sha256(body).hexdigest() == 'af7dd1cc3545d9c6e1b0cae16887cc5c47d2568912840a1fa7543c288ef38875'
    for absolute in (0x62, 0x67, 0x73, 0x91, 0x96, 0xA1):
        p = absolute - 0x42
        seg = int.from_bytes(body[p:p+2], 'little')
        body[p:p+2] = (seg+0x824).to_bytes(2, 'little')
    assert sha256(body).hexdigest() == 'b8e2acf3e162161244199147a586b0ed81882a0a30d2de4261a42acf5ef7c0f3'
    thunk = bytes.fromhex('ea42008a39')
    assert sha256(thunk).hexdigest() == 'a5378317872433ef1fc55344812aa7de6aeb930049cac07533a62dc14c044e79'
    for name, values in cpus.items():
        assert len(values) == 16 and values == [json.loads((root / f'cpu-{name}.json').read_text())[n] for n in NAMES]
        image = images[name]
        assert values[2] == 0x14A0 and image[0x18E14:0x18E18] == bytes.fromhex('25003a08')
        assert image[0x83C5:0x83CA] == thunk
        assert image[0x398A0+0x42:0x398A0+0xAC] == body
        for off, size, digest in (
            (0x308, 203, '03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0'),
            (0x491, 17, '440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d'),
        ):
            original = start[0xAC76+off:0xAC76+off+size]
            assert sha256(original).hexdigest() == digest
            assert image[0x131C0+off:0x131C0+off+size] == original
    assert cpus['natural-0308'] == cpus['growth-0308']
    a, b = images['natural-0308'], images['growth-0308']
    assert {i for i, (x, y) in enumerate(zip(a, b)) if x != y} == {0x18E0E, 0x18E0F}
    assert [word(images['growth-thunk'], 0x1D16, 0x37A4+2*i) for i in range(6)] == [0x3BB, 0x131C, 0, 8, 0x6652, 0x248]
    assert [word(images['exhaust-thunk'], 0x1D16, 0x37A8+2*i) for i in range(4)] == [0x3C7, 0x131C, 0x11D, 0x248]
    for n in ['natural-0308', 'growth-0308'] + GROWTH:
        assert word(images[n], 0x1D16, 0x37A0) == 0x196A
    assert cpus['growth-03bb'][9] == 0x196A and cpus['growth-03bb'][7] == 0x37AA
    assert (cpus['growth-0248'][9], cpus['growth-0248'][12], cpus['growth-0248'][7]) == (8, 0x6652, 0x37B0)
    assert cpus['exhaust-005f'][:2] == [0x398A, 0x5F]
    assert [word(images['exhaust-005f'], 0x1D16, 0x3774+2*i) for i in range(5)] == [0, 0x398A, 0x3786, 0x1D16, 0xE]
    return cpus, images


def original_prefixes(cpus, images):
    total = 0
    for name, terminal in (('growth', 0x3B7), ('exhaust', 0x3C3)):
        first, last = name+'-0308', name+f'-{terminal:04x}'
        uc = mapped.emulator(images[first], cpus[first]); started = time.monotonic()
        for steps in range(500):
            if time.monotonic() - started >= 10:
                raise TimeoutError('original-only core prefix deadline')
            if mapped.values(uc)[:2] == [0x131C, terminal]:
                compare_cpu(mapped.values(uc), cpus[last], 'original-only core prefix endpoint')
                assert bytes(uc.mem_read(0, 655360)) == images[last]
                print(f'original-only {name} prefix: {steps} original instructions; one native endpoint; no C bridge', flush=True)
                total += steps; break
            mapped.one(uc)
        else:
            raise AssertionError('original-only prefix instruction bound')
    return total


def differential(lib, oracle, name, image, values, terminal=None,
                 status_expected=None, checkpoints=(), single=False):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    oracle.mem_write(0, image)
    for register, value in zip(mapped.REGS, values):
        oracle.reg_write(register, value)
    pending = list(checkpoints); seen = 0; started = time.monotonic()
    for steps in range(100):
        if time.monotonic() - started >= 10:
            raise TimeoutError(f'{name}: deadline at step {steps}')
        pc = (state.cpu.cs, state.cpu.ip)
        if terminal == pc and status_expected is None:
            assert not pending
            print(f'{name}: {steps} C/original-byte comparisons; {seen} native checkpoints; boundary reached', flush=True)
            return steps, seen
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        report = Report()
        status = lib.pr_system_getmem_real_hook_step(ct.byref(state), ct.byref(report))
        if pc == terminal:
            assert status == status_expected and not pending, (name, pc, status)
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            if status == 1:
                assert (report.dependency_valid, report.call_is_near, report.call_pc, report.callee_segment,
                        report.callee_pc, report.return_pc) == (1, 0, 0x5F, 0xAF8, 0xB0B, 0x64)
            else:
                assert not report.dependency_valid
            print(f'{name}: {steps} C/original-byte comparisons; {seen} native checkpoints; gate={status}', flush=True)
            return steps, seen
        assert status == 0 and not report.dependency_valid, (name, pc, status)
        mapped.one(oracle)
        compare_cpu(cpu_values(state.cpu), mapped.values(oracle), f'{name} after {pc}')
        assert memory.raw == bytes(oracle.mem_read(0, len(image))), (name, pc, 'RAM')
        if pending and cpu_values(state.cpu)[:2] == pending[0][1][:2]:
            label, expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, label)
            assert memory.raw == expected_ram, label
            seen += 1
        if single:
            assert not pending
            return steps+1, seen
    raise AssertionError(f'{name}: instruction bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_system_getmem_real_hook_abi.py LIBRARY NATIVE_REAL_HOOK_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus, images = inputs(root); original_steps = original_prefixes(cpus, images)
    lib = ct.CDLL(str(library))
    lib.pr_system_getmem_real_hook_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_system_getmem_real_hook_step.restype = ct.c_int
    oracle = mapped.emulator(images['growth-03b7'], cpus['growth-03b7'])
    total = native = 0
    def run(*args, **kwargs):
        nonlocal total, native
        n, c = differential(lib, oracle, *args, **kwargs)
        total += n; native += c
    run('controlled native real growth hook', images[GROWTH[0]], cpus[GROWTH[0]], (0x131C, 0x248), 3,
        [(n, cpus[n], images[n]) for n in GROWTH[1:]])
    run('controlled native real exhaustion hook', images[EXHAUST[0]], cpus[EXHAUST[0]], (0x398A, 0x5F), 1,
        [(n, cpus[n], images[n]) for n in EXHAUST[1:]])
    # All following cases alter independent INPUTS, never post-hook outcomes.
    values = cpus['exhaust-03c3'].copy(); values[9] = 0xA55A
    run('synthetic incoming AX preserves AH', images['exhaust-03c3'], values, (0x398A, 0x5F), 1)
    for kind in ('wrapped SP', 'call pointer aliases far frame'):
        image = bytearray(images['growth-03b7']); values = cpus['growth-03b7'].copy()
        values[6], values[7] = (0x1D16, 0) if kind == 'wrapped SP' else (0x14A0, 0x4418)
        pos = (values[6] << 4) + values[7]; image[pos:pos+2] = b'\x00\x00'
        run('synthetic '+kind, bytes(image), values, (0x131C, 0x3BB))
    for size in (1, 0xFFFF):
        image = bytearray(images['exhaust-03c3'])
        image[0x1D160+0x37AC:0x1D160+0x37AE] = size.to_bytes(2, 'little')
        run(f'synthetic argument {size:04X}', bytes(image), cpus['exhaust-03c3'], (0x398A, 0x5F), 1)
    values = cpus['growth-03b7'].copy(); values[-1] = 0x7ED7
    run('synthetic stale incoming flags', images['growth-03b7'], values, (0x131C, 0x248), 3)
    for sp in (0, 0x27, 0x28, 0x7FFF, 0x8000, 0xFFFF):
        values = cpus['growth-0045'].copy(); values[7] = sp; values[-1] = 0x7ED7
        run(f'synthetic SUB SP boundary {sp:04X}', images['growth-0045'], values, single=True)
    for size in (0, 1, 0x8000, 0xFFFF):
        image = bytearray(images['exhaust-0048'])
        image[0x1D160+0x37AC:0x1D160+0x37AE] = size.to_bytes(2, 'little')
        run(f'synthetic argument CMP {size:04X}', bytes(image), cpus['exhaust-0048'], single=True)
    for value in (0, 0xFFFF, 0xA55A):
        image = bytearray(images['growth-00a3'])
        image[0x1D160+0x37A0:0x1D160+0x37A2] = value.to_bytes(2, 'little')
        run(f'synthetic single local-word read {value:04X}', bytes(image), cpus['growth-00a3'], single=True)
    print(f'Real-hook audit passed: {total} full-state C/original-byte instruction comparisons; '
          f'{native} post-input native C checkpoints; {original_steps} original-only core prefix instructions; '
          '36 pinned rows (one untouched baseline, 35 controlled); nonzero return/retry/error remain blocked', flush=True)


if __name__ == '__main__':
    main()
