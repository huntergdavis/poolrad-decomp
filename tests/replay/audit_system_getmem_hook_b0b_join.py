#!/usr/bin/env python3
"""Real callback B0B join through the printer gate; no hook outcome injection."""
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
import audit_system_getmem_real_hook_abi as prior
from audit_slot_a_string_0356_abi import CPU, State, NAMES, cpu_values, compare_cpu
mapped, Report = prior.mapped, prior.Report
LABELS = ('natural-0308 controlled-0308 03c3 005f 0b0b 0b0c 0b0e 0b10 '
          '0b14 0b18 0b19 0b1a 0b1c 0b1e 0b20 0b22 0064').split()
GROUPS = {'92cc0dc353e1d6e1a559820028ba7701860c3422bd0ea5bf7ccfdba65c19d3d7': '0b1a 0b1c 0b1e',
 'a4c72d02bcec29773f83156db24c9c17242f3c11af5ae83e3762ab051cf083d1': 'controlled-0308',
 'c62903974d60c56f82db9dc53752a4cc66e3fd8e85200cb601cfd1ab5a5c6e9f': '0b0b 0b0c 0b0e 0b10 0b14 0b18 0b19',
 'c65998a7e7e7102c753220a2964762b6b0bfc48170328bbe63979085c60def94': 'natural-0308',
 'dbbdbd4fcb862d6afdc4902138e7e083c758bb5c4e023569a2047ed19bc4cfdc': '03c3',
 'ea8e77683ce659a5acaaf38579c5464cf3d31eb9c701395dc1a787646b9f1f05': '0064 0b20 0b22',
 'ff101e2055cfe8c7dc149e1bbca408591a5fbc3b41ddb4ed89c2329df733b293': '005f'}


def word(image, off):
    return int.from_bytes(image[0x1D160+off:0x1D160+off+2], 'little')


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '39ef8adb748c91ff5e0e960cb0703229a9194cede407764f17e4e092acee4cc8'
    clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", clean)
    hashes = {n: h for h, names in GROUPS.items() for n in names.split()}
    assert len(rows) == len(hashes) == len(LABELS) == 17 and set(hashes) == set(LABELS)
    cpus = {n: [int(v, 16) for v in row.split()] for n, row in zip(LABELS, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, hashes[n]) for n in LABELS}
    start = mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    body = bytearray(game[0x4A:0x4A+106])
    assert sha256(body).hexdigest() == 'af7dd1cc3545d9c6e1b0cae16887cc5c47d2568912840a1fa7543c288ef38875'
    for absolute in (0x62, 0x67, 0x73, 0x91, 0x96, 0xA1):
        p = absolute-0x42
        body[p:p+2] = (int.from_bytes(body[p:p+2], 'little')+0x824).to_bytes(2, 'little')
    assert sha256(body).hexdigest() == 'b8e2acf3e162161244199147a586b0ed81882a0a30d2de4261a42acf5ef7c0f3'
    source = game[8:37]
    assert len(source) == 29 and source[0] == 28
    assert sha256(source).hexdigest() == 'ad7768f38fbce2c9c16fad3b116ed210b41a094354d6275085a12548d6272fc2'
    for name, values in cpus.items():
        assert len(values) == 16 and values == [json.loads((root / f'cpu-{name}.json').read_text())[n] for n in NAMES]
        cs = 0x398A if name in ('005f', '0064') else 0x131C
        assert values[:2] == [cs, int(name.split('-')[-1], 16)]
        image = images[name]
        assert image[0x18E14:0x18E18] == bytes.fromhex('25003a08')
        assert image[0x83C5:0x83CA] == bytes.fromhex('ea42008a39')
        assert image[0x398A0+0x42:0x398A0+0xAC] == body
        assert image[0x398A0:0x398A0+29] == source
        for off, size, digest in (
            (0x308, 203, '03e08d678db922d962aafbd79b301ca90556775acef11b832fecbeb29fe0dff0'),
            (0x491, 17, '440dc8e6e579ef895d168051f9a82a7def5562006d3777c191f15b88bf5bce1d'),
            (0xB0B, 26, '4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f'),
        ):
            original = start[0xAC76+off:0xAC76+off+size]
            assert sha256(original).hexdigest() == digest
            assert image[0x131C0+off:0x131C0+off+size] == original
    assert cpus['natural-0308'] == cpus['controlled-0308']
    changed = lambda a, b: {i for i, (x, y) in enumerate(zip(images[a], images[b])) if x != y}
    assert changed('natural-0308', 'controlled-0308') == {0x18E08, 0x18E0A, 0x18E0B, 0x18E0E, 0x18E0F}
    assert [word(images['0b0b'], 0x3770+2*i) for i in range(7)] == [0x64, 0x398A, 0, 0x398A, 0x3786, 0x1D16, 0xE]
    assert changed('005f', '0b0b') == set(range(0x208D0, 0x208D4))
    assert changed('0b19', '0b1a') == {0x208E6}
    assert changed('0b1e', '0b20') == set(range(0x208E7, 0x20903))
    assert images['0064'][0x208E6:0x20903] == source
    assert images['0064'][0x20903] == 0  # adjacent callback flag remains intact
    assert cpus['0b1c'][11] == 0x441C and cpus['0b1e'][11] == 28
    assert cpus['0064'] == [0x398A, 0x64, 0x14A0, 0x1D16, 0, 0, 0x1D16, 0x3778,
                            0x37A6, 0x1C, 0x3770, 0, 0x14A0, 0x1D, 0x37A3, 0x7246]
    assert [word(images['0064'], 0x3778+2*i) for i in range(3)] == [0x3786, 0x1D16, 0xE]
    return cpus, images


def original_prefix(cpus, images):
    uc = mapped.emulator(images['controlled-0308'], cpus['controlled-0308'])
    started = time.monotonic()
    for steps in range(500):
        if time.monotonic() - started >= 10:
            raise TimeoutError('original-only core prefix deadline')
        if mapped.values(uc)[:2] == [0x131C, 0x3C3]:
            compare_cpu(mapped.values(uc), cpus['03c3'], 'original-only pre-hook endpoint')
            assert bytes(uc.mem_read(0, 655360)) == images['03c3']
            print(f'original-only core prefix: {steps} instructions; one native endpoint; excluded from C total', flush=True)
            return steps
        mapped.one(uc)
    raise AssertionError('original-only core prefix instruction bound')


def differential(lib, oracle, name, image, values, checkpoints=()):
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
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        report = Report()
        status = lib.pr_system_getmem_hook_b0b_step(ct.byref(state), ct.byref(report))
        if pc == (0x398A, 0x64):
            assert status == 1 and not pending, (name, status)
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            assert (report.dependency_valid, report.call_is_near, report.call_pc, report.callee_segment,
                    report.callee_pc, report.return_pc) == (1, 0, 0x64, 0x709, 0xBF5, 0x69)
            print(f'{name}: {steps} full-state C/original-byte comparisons; {seen} native checkpoints; printer blocked', flush=True)
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
    raise AssertionError(f'{name}: instruction bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_system_getmem_hook_b0b_join.py LIBRARY NATIVE_CALLBACK_B0B_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus, images = inputs(root); original_steps = original_prefix(cpus, images)
    lib = ct.CDLL(str(library))
    lib.pr_system_getmem_hook_b0b_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_system_getmem_hook_b0b_step.restype = ct.c_int
    oracle = mapped.emulator(images['03c3'], cpus['03c3'])
    total = native = 0
    def run(*args, **kwargs):
        nonlocal total, native
        n, c = differential(lib, oracle, *args, **kwargs)
        total += n; native += c
    run('controlled native callback B0B join', images['03c3'], cpus['03c3'],
        [(n, cpus[n], images[n]) for n in LABELS[3:]])
    # Independent inputs only; no mid-case snapshot or returned-AL injection.
    values = cpus['005f'].copy(); values[-1] = 0x7ED7
    run('synthetic incoming DF/stale flags', images['005f'], values)
    values = cpus['005f'].copy(); values[9] = 0xA55A
    run('synthetic incoming AH', images['005f'], values)
    image = bytearray(images['005f']); values = cpus['005f'].copy(); values[7] = 0
    image[0x1D160:0x1D16A] = b''.join(v.to_bytes(2, 'little') for v in (0, 0x398A, 0x3786, 0x1D16, 0xE))
    run('synthetic wrapped far frame', bytes(image), values)
    for length in (0, 1, 255):
        image = bytearray(images['005f'])
        payload = bytes([length]) + bytes((17*i+3) & 255 for i in range(length))
        image[0x22160:0x22160+len(payload)] = payload
        image[0x208D4:0x208DC] = b''.join(v.to_bytes(2, 'little') for v in (0x5000, 0x1D16, 0x5200, 0x1D16))
        run(f'synthetic own string length {length}', bytes(image), cpus['005f'])
    image = bytearray(images['005f'])
    image[0x22160:0x22165] = b'\x04\x11\x22\x33\x44'
    image[0x208D4:0x208DC] = b''.join(v.to_bytes(2, 'little') for v in (0x5000, 0x1D16, 0x5001, 0x1D16))
    run('synthetic forward overlap', bytes(image), cpus['005f'])
    print(f'Callback B0B join audit passed: {total} full-state joined C/original-byte instruction comparisons; '
          f'{native} distinct post-input native checkpoints; {original_steps} original-only pre-hook instructions; '
          '17 pinned rows (one untouched baseline, 16 controlled); printer and retry/error remain blocked', flush=True)


if __name__ == '__main__':
    main()
