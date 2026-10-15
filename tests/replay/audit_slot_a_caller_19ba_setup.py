#!/usr/bin/env python3
"""Actual allocation-return caller setup; the near record loader stays blocked."""
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
    _fields_ = [('dependency_valid', ct.c_uint)] + [(n, ct.c_uint16) for n in
        ('caller_unit', 'call_pc', 'callee_unit', 'callee_pc', 'return_pc')] + [('call_is_near', ct.c_uint)]
RAM_HASHES = {
    "19ba": "7efd3b6a696f7072f9e767beda9037dc1704ce64e6cd3103fdb9063a877cc9de",
    "19be": "bd0e356328cfee9bb26388be7ef12ac4dcbc80c791b5b27281686ee53b6f212a",
    "19c2": "b27119f2f92e72bde24aa1d0061e3a90e7f3b0829eb235e5c89d138547ca81a1",
    "19c6": "b27119f2f92e72bde24aa1d0061e3a90e7f3b0829eb235e5c89d138547ca81a1",
    "19c7": "90929e5e1f6870616ddb88baf9a82ce079513a82f506e9f6c330a5433e58f2a3",
    "19c8": "2d9ba43b34db8f8f0a7af9b826542b62f77eb08e4ac59573f91ae75a460c19e2",
    "19cc": "9c3f6676845db74bfd3bfc85249b4226d5b0bfdaefb745eb6d11b71a2b157320",
    "19d0": "9d6ba8241366332e60192eb36df301ef586a088ecd79759e1b2189e503084b49",
    "19d2": "9d6ba8241366332e60192eb36df301ef586a088ecd79759e1b2189e503084b49",
    "19d3": "8cee4f03d458ead70183281c14333d3e7d780a48262cc8bf2ce09c1067c486e4",
    "19d5": "8cee4f03d458ead70183281c14333d3e7d780a48262cc8bf2ce09c1067c486e4",
    "19d6": "f45b1a2af471dd032cb9c3471adbb7f53991d440ce13483a86208510dc4b7a9a",
    "19d7": "c5c02ac4b727ef2f0c5f0243ca6f18cb707fbf8a66bd8547e92e78da2dd512e4"
}

def word(image, off):
    return int.from_bytes(image[0x1D160+off:0x1D160+off+2], 'little')


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == '5829da0b43fd5e4a654190833b6063f390ba2b3fd1ed94dacd8e4405eb5f8948'
    clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", clean)
    assert len(rows) == len(RAM_HASHES) == 13
    cpus = {n: [int(v, 16) for v in row.split()] for n, row in zip(RAM_HASHES, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, h) for n, h in RAM_HASHES.items()}
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    original = game[0x1DFE6+0x19BA:0x1DFE6+0x19DA]
    assert len(original) == 32 and sha256(original).hexdigest() == 'de51fef7ceacba00f9619b52c0c06c2ae33c04d12f7f27a151c1cee23b4d0e2c'
    for n, values in cpus.items():
        assert len(values) == 16 and values[:2] == [0x2A8E, int(n, 16)]
        assert values == [json.loads((root / f'cpu-{n}.json').read_text())[f] for f in NAMES]
        assert images[n][0x2A8E0+0x19BA:0x2A8E0+0x19DA] == original
        assert values[-1] == 0x7202
    assert cpus['19ba'] == [0x2A8E, 0x19BA, 0x14A0, 0x42AB, 0, 0, 0x1D16, 0x37B8,
                            0x3DCA, 8, 0x44AC, 8, 0x449A, 0x449A, 0, 0x7202]
    assert [word(images['19c2'], p) for p in (0x3BFD, 0x3BFF)] == [8, 0x449A]
    assert [word(images['19d7'], 0x37AA+2*i) for i in range(7)] == [0x2A8E, 1, 1, 8, 0x449A, 0x3BD4, 0x1D16]
    assert cpus['19d7'][7:10] == [0x37AA, 0x3DCA, 1] and cpus['19d7'][14] == 0x3BD4
    # Only original stores and PUSH instructions may change the captured RAM.
    seq = list(RAM_HASHES)
    pushes = {0x19C6, 0x19C7, 0x19C8, 0x19CC, 0x19D2, 0x19D5, 0x19D6}
    for a, b in zip(seq, seq[1:]):
        pc = int(a, 16)
        off = (cpus[a][7]-2) & 65535 if pc in pushes else (0x3BFD if pc == 0x19BA else 0x3BFF)
        allowed = {0x1D160+off, 0x1D160+off+1} if pc in pushes | {0x19BA, 0x19BE} else set()
        changes = {i for i, (x, y) in enumerate(zip(images[a], images[b])) if x != y}
        assert changes <= allowed, (a, b, changes)
    return cpus, images


def differential(lib, uc, name, image, values, checkpoints=()):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    uc.mem_write(0, image)
    for reg, value in zip(mapped.REGS, values): uc.reg_write(reg, value)
    pending = list(checkpoints); seen = 0; started = time.monotonic()
    for step in range(13):
        if time.monotonic() - started >= 10: raise TimeoutError(f'{name}: caller deadline at {step}')
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        report = Report()
        status = lib.pr_slot_a_caller_19ba_step(ct.byref(state), ct.byref(report))
        if before_cpu[:2] == [0x2A8E, 0x19D7]:
            assert step == 12 and status == 1 and not pending
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram
            assert (report.dependency_valid, report.caller_unit, report.call_pc, report.callee_unit,
                    report.callee_pc, report.return_pc, report.call_is_near) == (1, 0x6F, 0x19D7, 0x6F, 0xAEC, 0x19DA, 1)
            print(f'{name}: {step} full-state C/original-byte comparisons; {seen} native checkpoints; near loader blocked', flush=True)
            return step, seen
        assert status == 0 and not report.dependency_valid
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu), mapped.values(uc), f'{name} step {step}')
        assert memory.raw == bytes(uc.mem_read(0, len(image))), (name, step, 'RAM')
        assert state.cpu.flags == values[-1], (name, step, 'flags')
        if pending and cpu_values(state.cpu)[:2] == pending[0][0][:2]:
            expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, 'native caller setup')
            assert memory.raw == expected_ram
            seen += 1
    raise AssertionError(f'{name}: caller instruction bound')


def main():
    if len(sys.argv) != 3: raise SystemExit('usage: audit_slot_a_caller_19ba_setup.py LIBRARY NATIVE_CALLER_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__: raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(p).resolve(strict=True) for p in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and captures must stay in this clone resources/')
    cpus, images = inputs(root)
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_caller_19ba_step.argtypes = [ct.POINTER(State), ct.POINTER(Report)]
    lib.pr_slot_a_caller_19ba_step.restype = ct.c_int
    uc = mapped.emulator(images['19ba'], cpus['19ba'])
    total, native = differential(lib, uc, 'natural caller setup', images['19ba'], cpus['19ba'],
        [(cpus[n], images[n]) for n in list(RAM_HASHES)[1:]])
    # These are independent synthetic entry states, never an allocator result
    # injected into a continuous replay or a mid-case snapshot replacement.
    for label, edits in (
        ('zero offset', {'ax': 0}), ('all-bits offset', {'ax': 0xFFFF}),
        ('AH and stale flags/DF', {'ax': 0xABCD, 'flags': 0x7ED7}),
        ('SP wrap', {'sp': 0}), ('odd high SP', {'sp': 0xFFFF}),
        ('BP address wrap', {'bp': 0}),
        ('stack overwrites low pointer local', {'sp': 0x3BFF}),
        ('stack overwrites high pointer local', {'sp': 0x3C01}),
    ):
        values = cpus['19ba'].copy()
        for field, value in edits.items(): values[NAMES.index(field)] = value
        steps, seen = differential(lib, uc, 'synthetic '+label, images['19ba'], values)
        total += steps; native += seen
    print(f'Caller setup audit passed: {total} full-state C/original-byte instruction comparisons; '
          f'{native} distinct post-input native checkpoints; 13 untouched native rows; '
          'allocator and record-loader effects remain outside this adapter', flush=True)


if __name__ == '__main__': main()
