#!/usr/bin/env python3
"""Bounded file-probe instruction parity; unknown runtime effects stay blocked."""
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
from audit_slot_a_string_0356_abi import CPU, State, Report, NAMES, cpu_values, compare_cpu
RAM_HASHES = {
    '19a8': '9f246341a8e2a1ad5b6492b0669d6cdea2e599f698e4f448a76f9639971310e5',
    '02eb': 'c97808d3ef3d6c4f837954f2fcb09c32305c3e802bd2689eb20c1f6bdbe6ca3f',
    '02ec': '63bdbf8bf856ea73dfadf329b75d95bd3542c29d9eff7b0fbc9ccdac217c446b',
    '02f2': '63bdbf8bf856ea73dfadf329b75d95bd3542c29d9eff7b0fbc9ccdac217c446b',
    '0300': 'd75a06bf875cf3098c0ac198255294e004bf707a56a6e810926a9029db76ee7a',
    '0305': 'bd210da0a0d671416b034194e968e485fd34139d8f07d0faa29b888a0c8649eb',
    '0310': 'f4c5a354693735b9e00beec62abb659331cf5ede2ec582c6997bb56313befa07',
    '13e2': '79c6f94d0481ebba6d1a502cd83e0e082de20dac069cd11c3905e917f959b7b7',
    '0315': '2e8203b19909b4a5550de08b91a4bcc8b85a5fd3b7cd59e92c274120875c5567',
    '031f': '9d0a30343b0effb06d6bb8b16499e9367ca5fa7dfce4af5195e241531e61cc65',
    '1410': '6759553b678a1e698d8535bad8f324cf209ede8278fa1e73824103cebeddf2fa',
    '0324': 'af840c1398eb601429a00523cbcb63ec620440ab7daae58c440de3f89ccafe9e',
    '032a': 'af840c1398eb601429a00523cbcb63ec620440ab7daae58c440de3f89ccafe9e',
    '1491': 'fee6436b981235460db085a5c1a529f60d584989cea7df83ea386c9ef0d81f68',
    '032f': '29848453cca2d3f356ea0608d0f121dab28230905cd724f5a17bc4cca5ef93c3',
    '04a2': 'a4e467a32cd09cd8bda85ed1d7999ea4206386e4c059b22ce093bcd5a619340a',
    '0334': 'a4e467a32cd09cd8bda85ed1d7999ea4206386e4c059b22ce093bcd5a619340a',
    '0337': 'a4e467a32cd09cd8bda85ed1d7999ea4206386e4c059b22ce093bcd5a619340a',
    '0348': 'a4e467a32cd09cd8bda85ed1d7999ea4206386e4c059b22ce093bcd5a619340a',
    '0353': 'e8356e1107adcd130ea03a79033ca764cebd67f91375485efe2632c1bddbae77',
    '19ad': 'e8356e1107adcd130ea03a79033ca764cebd67f91375485efe2632c1bddbae77',
}
GATES = {0x310: 0x13E2, 0x31F: 0x1410, 0x32A: 0x1491, 0x32F: 0x4A2}


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '1a2a7c30d75c61af2c53ca05d957861eb37297ba0c7284c6e7c0f2d9ff6110dd'
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", text)
    assert len(rows) == len(RAM_HASHES) == 21
    cpus = {n: [int(x, 16) for x in row.split()] for n, row in zip(RAM_HASHES, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, h) for n, h in RAM_HASHES.items()}
    start = mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    original = start[0x8342:0x83AD]
    assert sha256(original).hexdigest() == '141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474'
    loaded = bytearray(original)
    for pc in (0x300, *GATES):
        i = pc - 0x2EB
        assert original[i] == 0x9A and original[i+3:i+5] == b'\xf8\x0a'
        loaded[i+3:i+5] = (0x131C).to_bytes(2, 'little')
    assert sha256(loaded).hexdigest() == 'a1bd874454a477dc56eb74bee8be51f5e77b17c782081440306e0ffb5213d2e2'
    caller = game[0x1F98E:0x1F997]
    assert sha256(caller).hexdigest() == '34225ed53f01e5483c1e290c0958129055a15cbed08fbd34cc1007f28c2a37cb'
    caller = bytearray(caller); caller[3:5] = (0x1026).to_bytes(2, 'little')
    assert sha256(caller).hexdigest() == '09eb6c96435627d600410ae10b43548304ec9f27e62070481443ab6bc639c6fe'
    copy = start[0xB79B:0xB7BF]
    assert sha256(copy).hexdigest() == mapped.pins.COPY_HASH
    for n, v in cpus.items():
        assert len(v) == 16
        cs = 0x2A8E if n in ('19a8', '19ad') else (0x131C if n in ('13e2', '1410', '1491', '04a2') else 0x1026)
        assert v[:2] == [cs, int(n, 16)]
        assert v == [json.loads((root / f'cpu-{n}.json').read_text())[f] for f in NAMES]
        ram = images[n]
        assert ram[0x10260+0x2EB:0x10260+0x356] == loaded
        assert ram[0x2A8E0+0x19A8:0x2A8E0+0x19B1] == caller
        assert ram[0x131C0+0xB25:0x131C0+0xB49] == copy
        for target in GATES.values():
            assert ram[0x131C0+target:0x131C0+target+16] == start[0xAC76+target:0xAC76+target+16]
    word = lambda n, off: int.from_bytes(images[n][0x1D160+off:0x1D160+off+2], 'little')
    for n, off, values in (
        ('19a8', 0x37B4, [0x3AB8, 0x1D16]),
        ('02eb', 0x37B0, [0x19AD, 0x2A8E, 0x3AB8, 0x1D16]),
        ('0300', 0x36FA, [0x28, 0x3784, 0x1D16, 0x3AB8, 0x1D16]),
        ('13e2', 0x36F8, [0x315, 0x1026, 0x3784, 0x1D16, 0x3704, 0x1D16]),
        ('1410', 0x36FA, [0x324, 0x1026, 0x80, 0x3704, 0x1D16]),
        ('1491', 0x36FC, [0x32F, 0x1026, 0x3704, 0x1D16]),
        ('04a2', 0x3700, [0x334, 0x1026]),
    ):
        assert [word(n, off+2*i) for i in range(len(values))] == values
    changed = lambda a, b: {i for i, (x, y) in enumerate(zip(images[a], images[b])) if x != y}
    assert changed('19a8', '02eb') == {0x20910, 0x20911}
    assert len(changed('19a8', '0310')) == 42
    assert len(changed('19a8', '19ad')) == 81
    assert not changed('0334', '0337')  # store of the same zero still executes
    assert cpus['19ad'][7] == 0x37B8 and cpus['19ad'][9] == 1
    return cpus, images


def differential(lib, label, image, values, terminal, checkpoints=()):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    report = Report(); uc = mapped.emulator(image, values)
    pending = list(checkpoints); seen = 0; started = time.monotonic()
    print(f'{label}: starting; limits=1000 instructions/30s', flush=True)
    for steps in range(1000):
        pc = (state.cpu.cs, state.cpu.ip)
        if time.monotonic() - started >= 30:
            raise TimeoutError(f'{label}: deadline at C step {steps}, PC={pc}')
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_slot_a_file_probe_02eb_step(ct.byref(state), ct.byref(report))
        if pc == terminal:
            assert status == (2 if terminal == (0x2A8E, 0x19AD) else 1)
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            assert not pending
            if status == 1:
                assert (report.dependency_valid, report.caller, report.call_pc,
                        report.callee_segment, report.callee_pc, report.return_pc,
                        report.caller_is_overlay) == (1, 0x802, pc[1], 0xAF8, GATES[pc[1]], pc[1]+5, 0)
            else:
                assert not report.dependency_valid
            print(f'{label}: {steps} C instructions, full CPU/RAM parity; '
                  f'{seen} native checkpoints; blocked={status}', flush=True)
            return steps, seen
        assert status == 0, (label, pc, status)
        assert not report.dependency_valid
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu), mapped.values(uc), f'{label} after {pc}')
        assert memory.raw == bytes(uc.mem_read(0, len(image))), (label, pc, 'RAM')
        if pending and cpu_values(state.cpu)[:2] == pending[0][1][:2]:
            name, expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, name)
            assert memory.raw == expected_ram, name
            seen += 1
    raise AssertionError(f'{label}: instruction bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_slot_a_file_probe_02eb_abi.py LIBRARY NATIVE_PROBE_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(x).resolve(strict=True) for x in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and native inputs must stay in this clone resources/')
    cpus, images = inputs(root)
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_file_probe_02eb_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_slot_a_file_probe_02eb_step.restype = ct.c_int
    total = native = 0
    # Separate initial states; never bridge unknown calls by injecting snapshots.
    for label, first, last, stops in (
        ('natural prefix', '19a8', '0310', ('02eb', '02ec', '02f2', '0300', '0305', '0310')),
        ('independent after Assign', '0315', '031f', ('031f',)),
        ('independent after Reset', '0324', '032a', ('032a',)),
        ('independent after Close', '032f', '032f', ()),
        ('independent after IOResult', '0334', '19ad', ('0337', '0348', '0353', '19ad')),
    ):
        checkpoints = [(n, cpus[n], images[n]) for n in stops]
        steps, count = differential(lib, label, images[first], cpus[first], tuple(cpus[last][:2]), checkpoints)
        total += steps; native += count
    # Emulator-only call/frame measurements, not C effects or runtime-body parity.
    for first, entry in (('0310', '13e2'), ('031f', '1410'), ('032a', '1491'), ('032f', '04a2')):
        uc = mapped.emulator(images[first], cpus[first]); mapped.one(uc)
        compare_cpu(mapped.values(uc), cpus[entry], f'original-only call {first}')
        assert bytes(uc.mem_read(0, 655360)) == images[entry]
    print('Four unknown far-call frames match original execution/native entries; no C runtime effects supplied', flush=True)
    for label, length, source, segment in (
        ('empty', 0, 0x5000, 0x1D16), ('exact-cap', 40, 0x5000, 0x1D16),
        ('over-cap', 41, 0x5000, 0x1D16), ('max-string', 255, 0x5000, 0x1D16),
        ('same-source-destination', 3, 0x3784, 0x1D16),
        ('forward-overlap', 3, 0x3783, 0x1D16),
        ('backward-overlap', 3, 0x3785, 0x1D16),
        ('distinct-segment-alias', 3, 0x3774, 0x1D17),
    ):
        image = bytearray(images['19a8']); start = (segment << 4) + source
        image[start:start+length+1] = bytes([length]) + bytes(65+i%26 for i in range(length))
        image[0x20914:0x20918] = source.to_bytes(2, 'little') + segment.to_bytes(2, 'little')
        steps, _ = differential(lib, 'synthetic-prefix-'+label, bytes(image), cpus['19a8'], (0x1026, 0x310))
        total += steps
    for label, error, length in (
        ('success', 0, 3), ('empty-name', 0, 0), ('error-one', 1, 3),
        ('error-high-byte', 0x100, 3), ('error-sign', 0x8000, 3), ('error-all-bits', 0xFFFF, 3),
    ):
        image = bytearray(images['0334']); values = cpus['0334'].copy()
        values[9] = error; image[0x208E4] = length
        steps, _ = differential(lib, 'synthetic-return-'+label, bytes(image), values, (0x2A8E, 0x19AD))
        total += steps
    print(f'Probe audit complete: {total} C instruction comparisons; {native} native C checkpoints '
          'across independent fragments; 21 pinned native rows; unknown effects remain blocked', flush=True)


if __name__ == '__main__':
    main()
