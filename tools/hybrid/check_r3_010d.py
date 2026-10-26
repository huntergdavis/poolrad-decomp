#!/usr/bin/env python3
"""Check t73's direct entry against original U38:010D, without new credit.

Run through poolrad-heavy.sh. Original bytes and build products stay in resources/.
The existing dispatcher adapter is reused; this tests its new direct-call boundary.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/overlay'))
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, REGISTERS, SIZE, EXE_SHA, OVR_SHA, physical, put16
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE

SOURCE = ROOT / 'src/recovered/overlay_u9e_dispatch_0000_cpu.c'
SEMANTIC = ROOT / 'src/recovered/overlay_unit38_effects.c'
SYMBOL = 'pr_u38_entry2_010d_cpu'


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    assert hashlib.sha256(exe).hexdigest() == EXE_SHA
    assert hashlib.sha256(ovr).hexdigest() == OVR_SHA
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)
                if u['stub_para'] == 0x38)
    assert unit['entries'][2] == 0x010D
    code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
    body = code[0x010D:0x011D]
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x010D))
    assert [(i.mnemonic, i.op_str) for i in instructions] == [
        ('push', 'bp'), ('mov', 'bp, sp'), ('add', 'byte ptr [0x6825], 5'),
        ('inc', 'byte ptr [0x6822]'), ('pop', 'bp'), ('retf', '0xa')]
    assert sum(i.size for i in instructions) == len(body) == 16
    return code, hashlib.sha256(body).hexdigest()


def library(directory, replacements=()):
    directory.mkdir(parents=True, exist_ok=True)
    sources = []
    for path in (SOURCE, SEMANTIC):
        text = path.read_text()
        for filename, before, after in replacements:
            if filename == path.name:
                assert before in text, (filename, before)
                text = text.replace(before, after, 1)
        copy = directory / path.name
        copy.write_text(text)
        sources.append(str(copy))
    version = directory / 'exports.map'
    version.write_text('{ global: ' + SYMBOL + '; local: *; };\n')
    so = directory / 'entry.so'
    subprocess.run(['cc', '-std=c11', '-O2', '-fPIC', '-ffunction-sections',
                    '-fdata-sections', '-shared', '-I', str(ROOT / 'include'),
                    *sources, '-Wl,--gc-sections', '-Wl,--no-undefined',
                    '-Wl,--version-script=' + str(version), '-o', str(so)], check=True)
    lib = ctypes.CDLL(str(so))
    func = getattr(lib, SYMBOL)
    func.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
    func.restype = ctypes.c_int
    return func


def fixtures(code):
    rng = random.Random(0x73010D)
    pairs = [(n, (n * 73) & 255) for n in range(256)]
    edges = (0, 1, 4, 5, 15, 16, 0x7A, 0x7B, 0x7F, 0x80, 0xFA, 0xFB, 0xFE, 0xFF)
    pairs += [(a, b) for a in edges for b in edges]
    pairs += [(rng.randrange(256), rng.randrange(256)) for _ in range(64)]
    layouts = [(0x1000, 0x2000, 0x8000)] * len(pairs)
    aliases = [(ds, ds, sp) for ds in (0x1000, 0xFFFF)
               for sp in range(0x681E, 0x682B)]
    aliases += [(0xFFFF, 0xFFFF, sp) for sp in (0x0200, 0xFFED, 0xFFEE)]
    pairs += [(rng.randrange(256), rng.randrange(256)) for _ in aliases]
    layouts += aliases
    for n, ((a, b), (ds, ss, sp)) in enumerate(zip(pairs, layouts)):
        memory = bytearray(range(256)) * (SIZE // 256)
        memory[0x30000:0x30000 + len(code)] = code
        memory[physical(ds, 0x6825)] = a
        memory[physical(ds, 0x6822)] = b
        put16(memory, ss, sp, 0x7000)
        put16(memory, ss, sp + 2, 0x7001)
        cpu = Cpu(*(rng.randrange(65536) for _ in range(7)), sp, 0x3000, 0x010D,
                  ds, rng.randrange(65536), ss, 0x202 | (rng.randrange(65536) & 0xCD5))
        yield f'case-{n}-ds{ds:04x}-ss{ss:04x}-sp{sp:04x}', memory, cpu


def oracle(memory, cpu):
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    uc.mem_map(0, 2 * SIZE)
    uc.mem_write(0, bytes(memory)); uc.mem_write(SIZE, bytes(memory))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    count = [0]

    def step(emulator, address, size, _):
        count[0] += 1
        if count[0] == 7:
            emulator.emu_stop()

    def mirror(emulator, access, address, size, value, _):
        for i in range(size):
            at = address + i
            emulator.mem_write(at + SIZE if at < SIZE else at - SIZE,
                               bytes([(value >> (8 * i)) & 255]))

    uc.hook_add(UC_HOOK_CODE, step)
    uc.hook_add(UC_HOOK_MEM_WRITE, mirror)
    uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=8)
    assert count[0] == 7, count
    state = [uc.reg_read(reg) & 65535 for reg in REGISTERS]
    return state, hashlib.sha256(uc.mem_read(0, SIZE)).hexdigest()


def actual(func, memory, cpu):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    state = Cpu.from_buffer_copy(cpu)
    status = func(ram, SIZE, ctypes.byref(state))
    return status, [getattr(state, n) for n, _ in Cpu._fields_], hashlib.sha256(ram).hexdigest()


def refusals(func):
    for name, changes, size, null_ram, null_cpu in [
        ('wrong-ip', {'ip': 0x011D}, SIZE, False, False),
        ('low-sp', {'sp': 0x01FF}, SIZE, False, False),
        ('high-sp', {'sp': 0xFFEF}, SIZE, False, False),
        ('short-ram', {}, SIZE - 1, False, False),
        ('large-ram', {}, SIZE + 1, False, False),
        ('null-ram', {}, SIZE, True, False),
        ('null-cpu', {}, SIZE, False, True),
    ]:
        ram = (ctypes.c_uint8 * SIZE)()
        cpu = Cpu(sp=0x8000, ip=0x010D)
        for k, v in changes.items(): setattr(cpu, k, v)
        before = bytes(cpu), hashlib.sha256(ram).digest()
        status = func(None if null_ram else ram, size, None if null_cpu else ctypes.byref(cpu))
        assert status == 1 and before == (bytes(cpu), hashlib.sha256(ram).digest()), name
    return 7


MUTANTS = [
    ('wrong-argument-pop', SOURCE.name, 'frame_leave(m, 0x0Au);', 'frame_leave(m, 0x08u);'),
    ('lost-add-carry', SOURCE.name, 'add8_flags(m->cpu, a6825, 5u);',
     'add8_flags(m->cpu, a6825, 5u); m->cpu->flags ^= 1u;'),
    ('wrong-inc-zero', SOURCE.name, 'inc8_flags(m->cpu, a6822);',
     'inc8_flags(m->cpu, a6822); m->cpu->flags ^= 0x40u;'),
    ('wrong-state-add', SEMANTIC.name, 'state->b6825 + 5u', 'state->b6825 + 4u'),
    ('missing-bp-write', SOURCE.name, 'push(m->ram, m->cpu, m->cpu->bp);',
     'm->cpu->sp = (uint16_t)(m->cpu->sp - 2u);'),
    ('wrong-entry-gate', SOURCE.name, 'cpu->ip != 0x010Du', 'cpu->ip != 0x011Du'),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    code, body_sha = source()
    func = library(args.out / 'correct')
    expected = []
    for label, memory, cpu in fixtures(code):
        want = oracle(memory, cpu)
        status, regs, digest = actual(func, memory, cpu)
        assert status == 0 and (regs, digest) == want, (label, status, regs, want[0])
        expected.append(want)
    refused = refusals(func)
    controls = {}
    for name, filename, before, after in MUTANTS:
        bad = library(args.out / name, [(filename, before, after)])
        for (label, memory, cpu), want in zip(fixtures(code), expected):
            status, regs, digest = actual(bad, memory, cpu)
            if status != 0 or (regs, digest) != want:
                controls[name] = label
                break
        assert name in controls, ('mutation survived', name)
    report = dict(body_sha256=body_sha, body_bytes=16, instructions=6,
                  cases=len(expected), argument_refusals=refused,
                  cpu_words=14, ram_bytes=SIZE, masks=0, negative_controls=controls,
                  original_media_sha256={'START.EXE': EXE_SHA, 'GAME.OVR': OVR_SHA},
                  source_sha256={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in (SOURCE, SEMANTIC, Path(__file__),
                                           ROOT / 'include/poolrad/recovered/overlay_u9e_dispatch_0000_cpu.h')},
                  head_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  scope='Direct-entry adapter only; no native route or recovery credit')
    (args.out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
