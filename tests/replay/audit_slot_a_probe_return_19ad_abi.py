#!/usr/bin/env python3
"""Original/native parity for the two-instruction saved-A probe return test."""
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
RAM_HASH = 'e8356e1107adcd130ea03a79033ca764cebd67f91375485efe2632c1bddbae77'
CODE_HASH = 'cb1c7828f77549fd0e0620fbb5a261abb5966a8188dfd3a50e242ef4e08376ca'


def inputs(root):
    raw = (root / 'native.log').read_bytes()
    assert sha256(raw).hexdigest() == 'aae3f921ce34bcc4279b68db74a6a3f2aad3188caf80a0529af2769ba73c3a51'
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', raw.decode(errors='replace'))
    rows = re.findall(r"LOG: EV of 'CS IP DS ES FS GS SS SP BP AX BX CX DX SI DI FLAGS' is:\s*LOG: ([0-9a-f ]+)", text)
    assert len(rows) == 3
    names = ('19ad', '19af', '19b1')
    cpus = {n: [int(x, 16) for x in row.split()] for n, row in zip(names, rows)}
    images = {n: mapped.pins.checked(root / f'ram-{n}.bin', 655360, RAM_HASH) for n in names}
    game = mapped.pins.checked(root / 'POOLRAD/GAME.OVR', 206105, mapped.pins.GAME_HASH)
    mapped.pins.checked(root / 'POOLRAD/START.EXE', 65231, mapped.pins.START_HASH)
    mapped.pins.checked(root / 'POOLRAD/SAVGAMA.DAT', 13137, mapped.pins.SAVE_HASH)
    code = game[0x1DFE6+0x19AD:0x1DFE6+0x19B1]
    assert len(code) == 4 and sha256(code).hexdigest() == CODE_HASH
    for n, values in cpus.items():
        assert len(values) == 16 and values[:2] == [0x2A8E, int(n, 16)]
        assert values == [json.loads((root / f'cpu-{n}.json').read_text())[f] for f in NAMES]
        assert images[n][0x2A8E0+0x19AD:0x2A8E0+0x19B1] == code
        assert values[7] == 0x37B8 and values[9] == 1
    assert cpus['19ad'][-1] == 0x7206
    assert cpus['19af'][-1] == cpus['19b1'][-1] == 0x7202
    return cpus, images


def differential(lib, image, values, checkpoints=()):
    memory = ct.create_string_buffer(image, len(image))
    state = State(CPU(**dict(zip(NAMES, values))), ct.cast(memory, ct.POINTER(ct.c_uint8)), len(image))
    uc = mapped.emulator(image, values); pending = list(checkpoints)
    present = bool(values[9] & 255); terminal = 0x19B1 if present else 0x19EA
    started = time.monotonic()
    for step in range(3):
        if time.monotonic() - started >= 10:
            raise TimeoutError(f'caller test deadline at C step {step}')
        before_cpu, before_ram = cpu_values(state.cpu), memory.raw
        status = lib.pr_slot_a_probe_return_19ad_step(ct.byref(state))
        if before_cpu[:2] == [0x2A8E, terminal]:
            assert step == 2 and status == (1 if present else 2)
            assert cpu_values(state.cpu) == before_cpu and memory.raw == before_ram == image
            assert not pending
            assert state.cpu.ax == values[9]  # AH and AL are preserved by OR
            return
        assert status == 0
        mapped.one(uc)
        compare_cpu(cpu_values(state.cpu), mapped.values(uc), f'caller step {step}')
        assert memory.raw == bytes(uc.mem_read(0, len(image))) == image
        if pending and cpu_values(state.cpu)[:2] == pending[0][0][:2]:
            expected_cpu, expected_ram = pending.pop(0)
            compare_cpu(cpu_values(state.cpu), expected_cpu, 'native caller stop')
            assert memory.raw == expected_ram
    raise AssertionError('two-instruction caller bound exhausted')


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_slot_a_probe_return_19ad_abi.py LIBRARY NATIVE_CALLER_ROOT')
    if unicorn.__version__ != '2.1.4' or not __debug__:
        raise SystemExit('requires Unicorn 2.1.4 and non-optimized assertions')
    library, root = (Path(x).resolve(strict=True) for x in sys.argv[1:])
    if any(not p.is_relative_to((ROOT / 'resources').resolve()) for p in (library, root)):
        raise SystemExit('library and native inputs must stay in this clone resources/')
    cpus, images = inputs(root)
    lib = ct.CDLL(str(library))
    lib.pr_slot_a_probe_return_19ad_step.argtypes = [ct.POINTER(State)]
    lib.pr_slot_a_probe_return_19ad_step.restype = ct.c_int
    differential(lib, images['19ad'], cpus['19ad'], [(cpus[n], images[n]) for n in ('19af', '19b1')])
    print('Natural +19AD -> +19B1: 2 C instructions, 2 post-entry native CPU/full-RAM checkpoints', flush=True)
    for al in range(256):
        values = cpus['19ad'].copy(); values[9] = 0xA500 | al
        values[-1] |= 0x8D1  # stale CF/OF/AF/ZF/SF must be replaced by OR
        if al & 1:
            values[-1] |= 0x400  # DF is unrelated and must survive
        differential(lib, images['19ad'], values)
        if al % 64 == 63:
            print(f'Synthetic caller AL coverage: {al+1}/256 complete', flush=True)
    print('Caller audit passed: 514 C/original-byte instruction comparisons; all 256 AL values; '
          '3 pinned native rows; present continuation and loop gate remain blocked', flush=True)


if __name__ == '__main__':
    main()
