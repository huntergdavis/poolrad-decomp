#!/usr/bin/env python3
"""Bounded saved-A allocation setup; allocator effects remain unsupported."""
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
    '19b1': 'c276543076b2e1d6d9710c2b0a908f7750e155556c1bafa800460c3707f83a05',
    '19b4': 'c276543076b2e1d6d9710c2b0a908f7750e155556c1bafa800460c3707f83a05',
    '19b5': 'ba06721343a36d5dc458c4846968884735741976e6c1d9237d38e53a3b312835',
}


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '94028d49d5d5a2d592e799d11ce256d89c2c3c06c65bcbc02876bb3561b71bf4'
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", text)
    assert len(rows) == len(RAM_HASHES) == 3
    cpus = {n: [int(x, 16) for x in row.split()] for n, row in zip(RAM_HASHES, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, h) for n, h in RAM_HASHES.items()}
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    original = game[0x1DFE6+0x19B1:0x1DFE6+0x19BA]
    assert sha256(original).hexdigest() == '27678668b204ad2d84785e89897bf20454582a70e1f7e34bd363cab31373a5b2'
    assert original[4:7] == b'\x9a\x3f\x02' and original[7:9] == b'\xf8\x0a'
    loaded = bytearray(original); loaded[7:9] = (0x131C).to_bytes(2, 'little')
    assert sha256(loaded).hexdigest() == '963c38901cd3f9bc0c0473f7e06cb17e4f7559edff39d3d1827cfd9d489092f9'
    for n, values in cpus.items():
        assert len(values) == 16 and values[:2] == [0x2A8E, int(n, 16)]
        assert values == [json.loads((root / f'cpu-{n}.json').read_text())[f] for f in NAMES]
        assert images[n][0x2A8E0+0x19B1:0x2A8E0+0x19BA] == loaded
        assert values[-1] == 0x7202
    assert cpus['19b1'][9] == 1 and cpus['19b4'][9] == cpus['19b5'][9] == 0x11D
    assert cpus['19b1'][7] == cpus['19b4'][7] == 0x37B8 and cpus['19b5'][7] == 0x37B6
    assert images['19b1'] == images['19b4']
    assert {i for i, (a, b) in enumerate(zip(images['19b1'], images['19b5'])) if a != b} == {0x20916, 0x20917}
    assert images['19b5'][0x20916:0x20918] == b'\x1d\x01'
    return cpus, images


def differential(lib, image, values, checkpoints=()):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    report = Report(); uc = mapped.emulator(image, values)
    pending = list(checkpoints); started = time.monotonic()
    for step in range(3):
        if time.monotonic() - started >= 10:
            raise TimeoutError(f'allocation setup deadline at C step {step}')
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_slot_a_allocation_19b1_step(ct.byref(state), ct.byref(report))
        if before_cpu[:2] == [0x2A8E, 0x19B5]:
            assert step == 2 and status == 1 and not pending
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            assert (report.dependency_valid, report.caller, report.call_pc,
                    report.callee_segment, report.callee_pc, report.return_pc,
                    report.caller_is_overlay) == (1, 0x6F, 0x19B5, 0xAF8, 0x23F, 0x19BA, 1)
            assert state.cpu.ax == 0x11D and state.cpu.flags == values[-1]
            return
        assert status == 0 and not report.dependency_valid
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu), mapped.values(uc), f'allocation setup step {step}')
        assert memory.raw == bytes(uc.mem_read(0, len(image)))
        if pending and cpu_values(state.cpu)[:2] == pending[0][0][:2]:
            expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, 'native allocation setup stop')
            assert memory.raw == expected_ram
    raise AssertionError('two-instruction setup bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_slot_a_allocation_19b1_abi.py LIBRARY NATIVE_ALLOCATION_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(x).resolve(strict=True) for x in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and native inputs must stay in this clone resources/')
    cpus, images = inputs(root)
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_allocation_19b1_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_slot_a_allocation_19b1_step.restype = ct.c_int
    differential(lib, images['19b1'], cpus['19b1'], [(cpus[n], images[n]) for n in ('19b4', '19b5')])
    print('Natural allocation setup: 2 C instructions; 2 post-entry native CPU/full-RAM checkpoints', flush=True)
    for label, ax, flags, sp in (
        ('zero-AX', 0, 0x7202, 0x37B8), ('all-bits-AX', 0xFFFF, 0x7ED7, 0x37B8),
        ('stack-word-wrap', 0xABCD, 0x7202, 0), ('stack-high-offset', 0xFF00, 0x7ED7, 0xFFFF),
        ('stack-data-alias', 0x1234, 0x7202, 0x3786),
    ):
        values = cpus['19b1'].copy(); values[9] = ax; values[-1] = flags; values[7] = sp
        differential(lib, images['19b1'], values)
        print(f'Synthetic allocation setup {label}: two full-state comparisons passed', flush=True)
    print('Allocation setup audit passed: 12 C/original-byte instruction comparisons; 3 pinned native rows; '
          'GetMem frame/effects remain blocked', flush=True)


if __name__ == '__main__':
    main()
