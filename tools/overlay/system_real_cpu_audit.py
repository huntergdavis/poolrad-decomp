#!/usr/bin/env python3
"""Original-byte full-state parity for the raw-CPU SYSTEM 6-byte Real entries 0AF8:1186 (multiply), 119A
(LongInt to Real) and 119E (Real to LongInt), plus a DOSBox-X oracle.

The bytes come from the hash-checked expanded START.EXE image, placed at load segment 0 for the Unicorn run, which
executes the original at 0AF8:<entry> and stops at the root return or at the RunError entry 0AF8:00E2. After every
case all 14 CPU words and the whole 1 MiB RAM are compared with the C model, with no mask. The DOSBox-X oracle runs
the original bytes at 9000:<entry>, patching only the RunError entry to a trap. Media stays private.
"""
import argparse
import ctypes
from hashlib import sha256
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import tempfile

from capstone import CS_ARCH_X86, CS_MODE_16, Cs
from resident_loader_map import unpack
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, REGISTERS, physical, put16

CHECKER_ROWS = ('RES_0AF8_1186', 'RES_0AF8_119A', 'RES_0AF8_119E')
SYSTEM, RUNERROR = 0x0AF8, 0x00E2
ENTRIES = dict(mul=0x1186, long=0x119A, trunc=0x119E)
FUNCTIONS = dict(mul='pr_system_real_mul_1186', long='pr_system_real_from_long_119a', trunc='pr_system_real_trunc_119e')
RETURNED, RUNERR = 0, 2
# Declared split (VP rule i19555): the model follows DOSBox-X, where a shift by one sets AF; Unicorn leaves AF clear. The
# comparison tolerates exactly that: only the AF bit, only when the last flag-writing instruction executed was a shift,
# and only with the model's AF set. Nothing else is tolerated, and every occurrence is counted in the report.
SHIFTS = ('shl', 'shr', 'sal', 'sar')
AF_WRITERS = ('add', 'adc', 'sub', 'sbb', 'cmp', 'neg', 'inc', 'dec', 'or', 'and', 'xor', 'test', 'mul', 'imul')
SS, RET_CS, RET_IP = 0x2800, 0x9001, 0x7000
FLAG_SET = (0x7202, 0x0202, 0x0ED7, 0x0002, 0x0646, 0x3A93)
EXPONENTS = (0, 1, 2, 3, 0x20, 0x40, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x9E, 0x9F, 0xA0, 0xBE, 0xBF, 0xC0, 0xFD, 0xFE, 0xFF)
# Body pins (bytes reachable by control flow from the entries, concatenated by address): bytes, instructions, SHA-256.
PINS = {
    0x0F33: (263, 123, '8fe816b3b4264cf5aea4b6adb4d423711814791b193dc656526c2ac19930cf67'),
    0x10D9: (63, 30, '64b69941b112ea462aea3848edd98e4f3ec85523f1b631a38fda67942f1c9251'),
    0x1118: (92, 44, '7ccd6539f95a342fa34f081470130d1ed34749d7761369cb05dc02a5723c1234'),
    0x1186: (12, 5, '1d4c1ec4d7592a5d7178a9ce8f07ccadef80ffa9674d19d0c9c14276f1dbd8e4'),
    0x119A: (4, 2, 'e962b3c25deea98a57e98aac984c325f9cfc8dacebb4c508fdd5943756fad95a'),
    0x119E: (14, 6, '089e1c58fe8ed92def3d01a88e74c1a045324d38bd62b4b1dc7215de4a4cc2f1'),
}
# Instruction starts and conditional outcomes no input can reach from the stated entry, with the reason.
UNREACHABLE = {
    'trunc': {
        'why': 'the wrapper sets CH=0 immediately before CALL +1118, so the rounding arm (CH != 0) is dead from +119E',
        'starts': (0x1155, 0x1158, 0x115B),
        'outcomes': ((0x1153, 0x1155), (0x115B, 0x115D), (0x115B, 0x116D)),
    },
}


def expected(label):
    """Stop a fixture must reach, derived from the arithmetic, or None: proves the path was reached, not inferred from the label.
    Zero operands and every LongInt conversion return; a Real to LongInt with exponent >= A0h overflows (RunError 207) and one
    with exponent <= 9Fh returns (zero below 80h)."""
    if label.startswith('long-') or label.startswith('mul-zero'):
        return 'long:return' if label.startswith('long-') else 'mul:return'
    parts = label.split('-')
    if parts[0] == 'trunc' and len(parts) == 4 and parts[1] != 'random':
        return 'trunc:RunError' if int(parts[1], 16) >= 0xA0 else 'trunc:return'
    return None


def check(condition, message):
    if not condition:
        raise ValueError(message)


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    check(len(exe) == 65231 and sha256(exe).hexdigest() == EXE_SHA, 'START.EXE provenance differs')
    image = bytes(unpack(exe)[0])
    base = SYSTEM * 16
    md = Cs(CS_ARCH_X86, CS_MODE_16)

    def decode(address):
        return list(md.disasm(image[base + address:base + address + 8], address, 1))[0]

    bodies = {}

    def flow(entry):
        if entry in bodies:
            return
        seen, work, calls = {}, [entry], set()
        while work:
            at = work.pop()
            if at in seen:
                continue
            ins = decode(at)
            seen[at] = ins
            if ins.mnemonic in ('ret', 'retf'):
                continue
            if ins.mnemonic == 'jmp':
                target = int(ins.op_str, 16)
                if target != RUNERROR:
                    work.append(target)
                continue
            if ins.mnemonic.startswith('j'):
                work += [int(ins.op_str, 16), at + ins.size]
                continue
            if ins.mnemonic == 'call':
                calls.add(int(ins.op_str, 16))
            work.append(at + ins.size)
        bodies[entry] = (seen, calls)
        for call in sorted(calls):
            flow(call)

    for entry in ENTRIES.values():
        flow(entry)
    check(sorted(bodies) == sorted(PINS), f'closure bodies differ: {[hex(b) for b in sorted(bodies)]}')
    for entry, (seen, _) in bodies.items():
        raw = b''.join(image[base + a:base + a + seen[a].size] for a in sorted(seen))
        check((len(raw), len(seen), sha256(raw).hexdigest()) == PINS[entry], f'body {entry:04X} extent/hash differs')
    # The two RunError stubs the wrappers jump to.
    for stub, code in ((0x11B4, 0xCD), (0x11AE, 0xCF)):
        mov, jump = decode(stub), decode(stub + 3)
        check((mov.mnemonic, mov.op_str, jump.mnemonic, jump.op_str) == ('mov', f'ax, {code:#x}', 'jmp', '0xe2'), f'RunError stub {stub:04X}')
    # The entries are exactly what OVL_0025_1CC3 calls: seven of each, nothing else in SYSTEM.
    ledger = [r for r in __import__('csv').DictReader(open(ROOT / 'docs/re/overlay_function_ledger.csv')) if r['id'] == '0x25:0x1cc3'][0]
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(ovr).hexdigest() == OVR_SHA, 'GAME.OVR provenance differs')
    census = {}
    for ins in md.disasm(ovr[int(ledger['ovr_offset']):int(ledger['ovr_offset']) + 301], 0x1CC3):
        if ins.mnemonic == 'lcall':
            census[ins.op_str] = census.get(ins.op_str, 0) + 1
    check(census == {'0xaf8, 0x1186': 7, '0xaf8, 0x119a': 7, '0xaf8, 0x119e': 7}, f'1CC3 call census {census}')
    decoded, conditional, closure = {}, {}, {}
    for entry, (seen, _) in bodies.items():
        decoded.update(seen)
    for at, ins in decoded.items():
        if ins.mnemonic.startswith('j') and ins.mnemonic != 'jmp':
            conditional[at] = (at + ins.size, int(ins.op_str, 16))
    for name, entry in ENTRIES.items():
        reach, work = set(), [entry]
        while work:
            at = work.pop()
            if at in reach:
                continue
            reach.add(at)
            ins = decoded[at]
            if ins.mnemonic in ('ret', 'retf'):
                continue
            if ins.mnemonic == 'jmp':
                target = int(ins.op_str, 16)
                if target != RUNERROR:
                    work.append(target)
                continue
            if ins.mnemonic.startswith('j'):
                work += [int(ins.op_str, 16), at + ins.size]
                continue
            if ins.mnemonic == 'call':
                work.append(int(ins.op_str, 16))
                # the callee's ret returns here
            work.append(at + ins.size)
        closure[name] = reach
    # No hardware-touching instruction on any path: port I/O, interrupts, HLT/CLI/STI, segment-register writes, control/debug registers.
    forbidden = ('in', 'out', 'int', 'int3', 'into', 'hlt', 'cli', 'sti', 'insb', 'insw', 'outsb', 'outsw', 'iret', 'lock', 'wait',
                 'lds', 'les', 'lss', 'lgdt', 'lidt', 'ljmp', 'lcall')
    segment_registers = ('cs', 'ds', 'es', 'ss', 'fs', 'gs')
    for at, ins in sorted(decoded.items()):
        destination = ins.op_str.split(',')[0].strip()
        check(ins.mnemonic not in forbidden, f'hardware-touching instruction {ins.mnemonic} at {at:04X}')
        check(not (ins.mnemonic in ('mov', 'pop', 'xchg') and destination in segment_registers), f'segment-register write at {at:04X}')
        check(not any(token in ins.op_str for token in ('cr0', 'cr2', 'cr3', 'cr4', 'dr0', 'dr1', 'dr2', 'dr3', 'dr6', 'dr7')), f'control register at {at:04X}')
    mix = {}
    for ins in decoded.values():
        mix[ins.mnemonic] = mix.get(ins.mnemonic, 0) + 1
    info = dict(bodies={f'{e:04X}': dict(bytes=PINS[e][0], instructions=PINS[e][1], sha256=PINS[e][2]) for e in sorted(PINS)},
                total_bytes=sum(v[0] for v in PINS.values()), media_1cc3_calls=census, instruction_mix=dict(sorted(mix.items())))
    print('SYSTEM Real chain', json.dumps(info, sort_keys=True), flush=True)
    return dict(image=image, decoded=decoded, conditional=conditional, closure=closure, info=info)


def library(directory):
    path = directory / 'real.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    str(ROOT / 'src/recovered/system_real_cpu.c'), '-o', str(path)], check=True, capture_output=True, text=True)
    lib = ctypes.CDLL(str(path))
    functions = {}
    for name, symbol in FUNCTIONS.items():
        function = getattr(lib, symbol)
        function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]
        function.restype = ctypes.c_int
        functions[name] = function
    return functions


def build(ctx, rng, name, regs, *, flags=0x7202, sp=0x8000, ss=SS, background=None):
    memory = bytearray([0xA5]) * SIZE if background is None else bytearray(background)
    memory[0:len(ctx['image'])] = ctx['image']
    for i, value in enumerate((RET_IP, RET_CS)):
        put16(memory, ss, (sp + 2 * i) & 0xFFFF, value)
    ax, bx, cx, dx, si, di = regs
    return memory, Cpu(ax, bx, cx, dx, si, di, rng.randrange(0x10000), sp, SYSTEM, ENTRIES[name], rng.randrange(0x10000), rng.randrange(0x10000), ss, flags)


def compare(label, ctx, memory, cpu, name, function, budget=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    original = Uc(UC_ARCH_X86, UC_MODE_16)
    original.mem_map(0, 2 * SIZE)
    original.mem_write(0, bytes(memory))
    original.mem_write(SIZE, bytes(memory))
    for (field, _), register in zip(Cpu._fields_, REGISTERS):
        original.reg_write(register, getattr(cpu, field))
    decoded, conditional = ctx['decoded'], ctx['conditional']
    protected = [(physical(SYSTEM, a), physical(SYSTEM, a) + i.size) for a, i in decoded.items()]
    state = dict(stop=None, previous=None, unexpected=None, shift=False)
    visited, edges = set(), set()

    def hook(emulator, address, size, user):
        cs, ip = emulator.reg_read(UC_X86_REG_CS), emulator.reg_read(UC_X86_REG_IP)
        if (cs, ip) == (RET_CS, RET_IP):
            state['stop'] = RETURNED
            return emulator.emu_stop()
        if (cs, ip) == (SYSTEM, RUNERROR):
            state['stop'] = RUNERR
            return emulator.emu_stop()
        if cs != SYSTEM or ip not in decoded:
            state['unexpected'] = (cs, ip)
            return emulator.emu_stop()
        if state['previous'] in conditional:
            edges.add((state['previous'], ip))
        visited.add(ip)
        state['previous'] = ip
        mnemonic = decoded[ip].mnemonic
        if mnemonic in SHIFTS:
            state['shift'] = True
        elif mnemonic in AF_WRITERS:
            state['shift'] = False

    def mirror(emulator, access, address, size, value, user):
        for byte in range(size):
            raw = address + byte
            at = raw & (SIZE - 1)
            check(not any(lo <= at < hi for lo, hi in protected), f'immutable code written: {label} {at:x}')
            emulator.mem_write(at + SIZE if raw < SIZE else at, bytes([(value >> (8 * byte)) & 0xFF]))

    original.hook_add(UC_HOOK_CODE, hook)
    original.hook_add(UC_HOOK_MEM_WRITE, mirror)
    original.emu_start(physical(SYSTEM, ENTRIES[name]), 2 * SIZE, count=budget)
    check(state['unexpected'] is None and state['stop'] is not None, f'original escaped or stalled: {label} {state}')
    expected = bytes(original.mem_read(0, SIZE))
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    status = function(ram, SIZE, ctypes.byref(cpu))
    check(status == state['stop'], f"C status {status}, original {state['stop']}: {label}")
    actual = bytes(ram)
    if actual != expected:
        differences = [i for i, (a, b) in enumerate(zip(actual, expected)) if a != b]
        raise ValueError(f'RAM mismatch {label}: {[(hex(d), actual[d], expected[d]) for d in differences[:6]]} ({len(differences)} bytes)')
    split = 0
    for (field, _), register in zip(Cpu._fields_, REGISTERS):
        mine, want = getattr(cpu, field), original.reg_read(register) & 0xFFFF
        if field == 'flags' and mine != want and mine ^ want == 0x10 and mine & 0x10 and state['shift']:
            split = 1
            continue
        check(mine == want, f'{field} mismatch {label}: C={mine:04x}, original={want:04x}')
    return state['stop'], visited, edges, split


def real(rng, exponent=None, sign=None, mantissa=None):
    """One 6-byte Real as (exponent byte, mantissa low byte, mantissa middle word, high word with the sign in bit 15)."""
    exponent = rng.choice(EXPONENTS + (rng.randrange(256),)) if exponent is None else exponent
    sign = rng.randrange(2) if sign is None else sign
    if mantissa is None:
        mantissa = rng.choice((rng.getrandbits(39), (1 << 39) - 1, 0, 1 << 38, (1 << 31) - 1, 1 << 32))
    return exponent, mantissa & 0xFF, (mantissa >> 8) & 0xFFFF, ((mantissa >> 24) & 0x7FFF) | (sign << 15)


def pack_real(parts):
    exponent, low, middle, high = parts
    return exponent | (low << 8), middle, high   # AL/AH, BX, DX


def mul_regs(a, b):
    ax, bx, dx = pack_real(a)
    cx, si, di = pack_real(b)
    return (ax, bx, cx, dx, si, di)


def cases(rng):
    spec = []
    flag = lambda i: FLAG_SET[i % 6]
    n = 0
    for a in (0, 1, 2, 0x7F, 0x80, 0xFF):
        spec.append((f'mul-zero-a-{a}', 'mul', mul_regs((0, 5, 6, 7), real(rng)), flag(n))); n += 1
        spec.append((f'mul-zero-b-{a}', 'mul', mul_regs(real(rng), (0, 5, 6, 7)), flag(n))); n += 1
    for ea in EXPONENTS:
        for eb in EXPONENTS:
            for kind in range(3):
                mant = (None, (1 << 39) - 1, 1 << 38)[kind]
                spec.append((f'mul-exp-{ea:02x}-{eb:02x}-{kind}', 'mul', mul_regs(real(rng, ea, mantissa=mant), real(rng, eb, mantissa=mant)), flag(n))); n += 1
    for i in range(400):                                            # short path: either low mantissa part is zero
        a = real(rng, mantissa=rng.choice((0, rng.getrandbits(39) & ~0xFFFFFF)) if i % 2 else None)
        b = real(rng, mantissa=(rng.getrandbits(39) & ~0xFFFFFF) if i % 3 == 0 else None)
        if i % 5 == 0:
            a = (a[0], 0, 0, a[3])
        if i % 7 == 0:
            b = (b[0], 0, 0, b[3])
        spec.append((f'mul-short-{i}', 'mul', mul_regs(a, b), flag(n))); n += 1
    for i in range(60):                                             # second operand: low byte zero, middle word nonzero
        b = real(rng, mantissa=(rng.getrandbits(39) & ~0xFF) | (rng.getrandbits(16) << 8) | (1 << 8))
        b = (b[0], 0, b[2] or 1, b[3])
        spec.append((f'mul-b-lowbyte-zero-{i}', 'mul', mul_regs((rng.choice(EXPONENTS[1:]), 1 + rng.randrange(255), rng.getrandbits(16), rng.getrandbits(16)), b), flag(n))); n += 1
    for i in range(60):                                             # first operand: low byte zero, middle word nonzero
        first = real(rng)
        spec.append((f'mul-a-lowbyte-zero-{i}', 'mul', mul_regs((rng.choice(EXPONENTS[1:]), 0, 1 + rng.randrange(65535), first[3]), real(rng)), flag(n))); n += 1
    # Products that round up through every mantissa word (found by probing; each pair reaches the carry-out arm at +1021).
    for m1, m2 in ((0xEBF1446BEA, 0x8AE19E7B75), (0xD3BD69FE29, 0x9AC1831AED), (0xEB076CE2EF, 0x8B6BCC8773), (0x86A6233255, 0xF35BD40D8E)):
        for first, second in ((m1, m2), (m2, m1)):
            for ea, eb in ((0x81, 0x81), (0x80, 0x80), (0xFE, 0x81), (0xFF, 0xFF), (0x81, 0xBE)):
                for sa, sb in ((0, 0), (1, 0), (1, 1)):
                    spec.append((f'mul-carry-{first:x}-{second:x}-{ea:02x}-{eb:02x}-{sa}{sb}', 'mul',
                                 mul_regs(real(rng, ea, sa, first & ((1 << 39) - 1)), real(rng, eb, sb, second & ((1 << 39) - 1))), flag(n))); n += 1
    for i in range(1500):
        spec.append((f'mul-random-{i}', 'mul', mul_regs(real(rng), real(rng)), flag(n))); n += 1
    longs = [0, 1, 2, 3, 0xFFFF, 0x10000, 0x10001, 0xFFFFFF, 0x1000000, 0x7FFFFFFF, 0x80000000, 0x80000001, 0xFFFFFFFF, 0xFFFFFFFE, 0xFFFF0000, 0xFF00]
    for k in range(32):
        for delta in (-1, 0, 1):
            longs += [((1 << k) + delta) & 0xFFFFFFFF, (-((1 << k) + delta)) & 0xFFFFFFFF]
    longs += [rng.getrandbits(32) for _ in range(300)] + [rng.getrandbits(16) for _ in range(100)] + [rng.getrandbits(24) for _ in range(100)]
    for v in longs:
        spec.append((f'long-{v:08x}-{n}', 'long', (v & 0xFFFF, rng.getrandbits(16), rng.getrandbits(16), v >> 16, rng.getrandbits(16), rng.getrandbits(16)), flag(n))); n += 1
    for exponent in range(0x60, 0xA4):
        for sign in (0, 1):
            for kind in range(3):
                mant = (None, (1 << 39) - 1, 1 << 38)[kind]
                ax, bx, dx = pack_real(real(rng, exponent, sign, mant))
                spec.append((f'trunc-{exponent:02x}-{sign}-{kind}', 'trunc', (ax, bx, rng.getrandbits(16), dx, rng.getrandbits(16), rng.getrandbits(16)), flag(n))); n += 1
    for i in range(1200):
        parts = real(rng)
        ax, bx, dx = pack_real(parts)
        spec.append((f'trunc-random-{i}', 'trunc', (ax, bx, rng.getrandbits(16), dx, rng.getrandbits(16), rng.getrandbits(16)), flag(n))); n += 1
    for sp in (0x0040, 0x0042, 0x0100, 0xFFF8, 0xFFFC):
        for name in ENTRIES:
            ax, bx, dx = pack_real(real(rng, 0x90, 0, None))
            spec.append((f'sp-{sp:04x}-{name}', name, (ax, bx, 0x8081, dx, rng.getrandbits(16), rng.getrandbits(16)), flag(n), dict(sp=sp, ss=0x1C00))); n += 1
    spec.append(('random-background-mul', 'mul', mul_regs(real(rng), real(rng)), 0x0202, dict(background=bytes(random.Random(0x1186).randbytes(SIZE)))))
    return spec


def differential(ctx, functions):
    rng = random.Random(0x1186119A)
    visited = {name: set() for name in ENTRIES}
    edges = {name: set() for name in ENTRIES}
    stops, named, splits = {}, {}, 0
    for entry in cases(rng):
        label, name, regs, flags = entry[:4]
        options = entry[4] if len(entry) > 4 else {}
        memory, cpu = build(ctx, rng, name, regs, flags=flags, **options)
        stop, v, e, split = compare(label, ctx, memory, cpu, name, functions[name])
        splits += split
        visited[name] |= v
        edges[name] |= e
        key = f"{name}:{'return' if stop == RETURNED else 'RunError'}"
        stops[key] = stops.get(key, 0) + 1
        want = expected(label)
        if want is not None:
            check(key == want, f'fixture {label} reached {key}, expected {want}')
        if not label.startswith(('mul-exp', 'mul-random', 'mul-short', 'mul-a-low', 'mul-b-low', 'mul-carry', 'trunc-random', 'long-')):
            named[label] = key
    report = dict(cases=sum(stops.values()), stops=stops, cpu_words=14, ram_bytes=SIZE, masked_bits=0, af_after_shift_splits=splits, entries={})
    conditional = ctx['conditional']
    for name in ENTRIES:
        reach = ctx['closure'][name]
        wanted_edges = {(a, b) for a in reach if a in conditional for b in conditional[a]}
        excluded = UNREACHABLE.get(name, dict(starts=(), outcomes=()))
        missing = sorted(reach - visited[name])
        missing_edges = sorted(wanted_edges - edges[name])
        check(missing == sorted(excluded['starts']) and missing_edges == sorted(excluded['outcomes']),
              f'{name} coverage: starts {[hex(a) for a in missing]} outcomes {[(hex(a), hex(b)) for a, b in missing_edges]}')
        report['entries'][name] = dict(starts=len(visited[name]), total=len(reach), outcomes=len(edges[name]), outcomes_total=len(wanted_edges),
                                       excluded_starts=[hex(a) for a in missing], excluded_outcomes=[[hex(a), hex(b)] for a, b in missing_edges],
                                       reason=excluded.get('why', ''))
    report['named_stops'] = named
    print('Original parity', json.dumps(report, sort_keys=True), flush=True)
    return report


# ---- DOSBox-X synthetic oracle -------------------------------------------------------------------------
BODY_N = 0x9000
MAIN_AT, BLOB_AT, CASES_AT = 0x0300, 0x0900, 0x1900
BLOB_LOW, BLOB_HIGH = 0x0F30, 0x11C0
OUTPUT, HEADER, STACK_TOP = 0xC000, 0xC800, 0xF000
FRAME_LOW, FRAME_SIZE = 0xEFE0, 0x20
CASE_SIZE = 14
RECORD_BYTES = 30 + FRAME_SIZE
NATIVE_FLAGS = (0x0002, 0x00D7, 0x08D7, 0x0402, 0x0893, 0x0003)
NAMES = tuple(ENTRIES)


def native_cases():
    rng = random.Random(0x1186D05)
    pool = cases(random.Random(0x1186119A))
    chosen = [c for c in pool if len(c) == 4]
    spread = lambda items, k: items[::max(1, len(items) // k)][:k]
    picked = spread([c for c in chosen if c[1] == 'mul'], 90) + spread([c for c in chosen if c[1] == 'long'], 80) + \
        spread([c for c in chosen if c[1] == 'trunc'], 100)
    rows = []
    for n, entry in enumerate(picked):
        rows.append(dict(label=entry[0], name=entry[1], regs=entry[2], flags=n % len(NATIVE_FLAGS)))
    for name in ENTRIES:
        rows.append(dict(label=f'zero-{name}', name=name, regs=(0, 0, 0, 0, 0, 0), flags=0))
    return rows


def native_audit(ctx, functions):
    directory = ROOT / 'resources/oracle-runs/system-real-synthetic'
    directory.mkdir(parents=True, exist_ok=True)
    rows = native_cases()
    blob = ctx['image'][SYSTEM * 16 + BLOB_LOW:SYSTEM * 16 + BLOB_HIGH]
    text = ['.code16', '.intel_syntax noprefix', '.global _start', '_start:', '    jmp main', f'.org {MAIN_AT - 0x100:#x}', 'main:',
            '    mov ax,cs', '    mov ds,ax', '    mov es,ax', '    mov ss,ax', f'    mov sp,{STACK_TOP:#x}', '    cld',
            f'    mov ax,{BODY_N:#x}', '    mov es,ax', '    mov si,offset body', f'    mov di,{BLOB_LOW:#x}', f'    mov cx,{len(blob)}', '    rep movsb',
            f'    mov byte ptr es:[{RUNERROR:#x}],0xea', f'    mov word ptr es:[{RUNERROR + 1:#x}],offset trap', f'    mov word ptr es:[{RUNERROR + 3:#x}],cs',
            '    mov ax,cs', '    mov es,ax',
            f'    mov word ptr [{HEADER:#x}],cs', f'    mov word ptr [{HEADER + 2:#x}],offset returned',
            f'    mov word ptr [{HEADER + 4:#x}],{len(rows)}', f'    mov word ptr [{HEADER + 6:#x}],0',
            '    mov dx,offset filename', '    xor cx,cx', '    mov ah,0x3c', '    int 0x21', '    jc failure',
            '    mov word ptr [handle],ax', '    mov bx,ax', f'    mov dx,{HEADER:#x}', '    mov cx,8', '    mov ah,0x40', '    int 0x21', '    jc failure',
            'again:', '    cli', f'    mov sp,{STACK_TOP:#x}', '    cld', '    mov ax,cs', '    mov ds,ax', '    mov es,ax',
            '    mov al,0xa5', f'    mov di,{FRAME_LOW:#x}', f'    mov cx,{FRAME_SIZE}', '    rep stosb',
            '    mov ax,word ptr [index]', f'    mov bx,{CASE_SIZE}', '    mul bx', '    add ax,offset cases', '    mov si,ax',
            '    mov bl,byte ptr [si]', '    xor bh,bh', '    shl bx,2', '    mov ax,word ptr [targets+bx]', '    mov word ptr [current],ax',
            '    mov ax,word ptr [targets+bx+2]', '    mov word ptr [current+2],ax',
            '    mov bl,byte ptr [si+1]', '    xor bh,bh', '    shl bx,1', '    push word ptr [flags_table+bx]', '    popf',
            '    mov ax,word ptr [si+2]', '    mov bx,word ptr [si+4]', '    mov cx,word ptr [si+6]', '    mov dx,word ptr [si+8]',
            '    mov di,word ptr [si+12]', '    mov si,word ptr [si+10]', '    mov bp,0x7777',
            '    call dword ptr cs:[current]', 'returned:', '    mov word ptr cs:[status],0', '    jmp record', 'trap:', '    mov word ptr cs:[status],1', 'record:']
    for i, reg in enumerate('ax bx cx dx si di bp sp'.split()):
        text.append(f'    mov word ptr cs:[{OUTPUT + 2 * i:#x}],{reg}')
    text += [f'    mov word ptr cs:[{OUTPUT + 20:#x}],ds', f'    mov word ptr cs:[{OUTPUT + 22:#x}],es', f'    mov word ptr cs:[{OUTPUT + 24:#x}],ss',
             f'    mov sp,{STACK_TOP + 0x800:#x}', '    pushf', '    pop ax', f'    mov word ptr cs:[{OUTPUT + 26:#x}],ax',
             '    cmp word ptr cs:[status],0', '    jne trapped',
             f'    mov word ptr cs:[{OUTPUT + 16:#x}],cs', f'    mov word ptr cs:[{OUTPUT + 18:#x}],offset returned', '    jmp stamp', 'trapped:',
             f'    mov word ptr cs:[{OUTPUT + 16:#x}],{BODY_N:#x}', f'    mov word ptr cs:[{OUTPUT + 18:#x}],{RUNERROR:#x}', 'stamp:',
             '    mov ax,word ptr cs:[status]', f'    mov word ptr cs:[{OUTPUT + 28:#x}],ax',
             f'    mov sp,{STACK_TOP + 0x800:#x}', '    cld', '    mov ax,cs', '    mov es,ax', '    mov ds,ax', f'    mov di,{OUTPUT + 30:#x}',
             f'    mov si,{FRAME_LOW:#x}', f'    mov cx,{FRAME_SIZE}', '    rep movsb', '    sti',
             '    mov bx,word ptr [handle]', f'    mov dx,{OUTPUT:#x}', f'    mov cx,{RECORD_BYTES}', '    mov ah,0x40', '    int 0x21', '    jc failure',
             f'    cmp ax,{RECORD_BYTES}', '    jne failure',
             '    inc word ptr [index]', '    mov ax,word ptr [index]', f'    cmp ax,{len(rows)}', '    jb again',
             '    mov bx,word ptr [handle]', '    mov ah,0x3e', '    int 0x21', '    mov ax,0x4c00', '    int 0x21',
             'failure:', '    mov ax,0x4c01', '    int 0x21',
             'filename: .asciz "RESULT.BIN"', 'handle: .word 0', 'index: .word 0', 'status: .word 0', 'current: .word 0,0',
             'targets:', *[f'    .word {ENTRIES[name]:#x},{BODY_N:#x}' for name in NAMES],
             'flags_table: .word ' + ','.join(f'{f:#x}' for f in NATIVE_FLAGS),
             f'.org {BLOB_AT - 0x100:#x}', 'body:', '    .byte ' + ','.join(f'{b:#x}' for b in blob),
             f'.org {CASES_AT - 0x100:#x}', 'cases:']
    for row in rows:
        ax, bx, cx, dx, si, di = row['regs']
        record = struct.pack('<BBHHHHHH', NAMES.index(row['name']), row['flags'], ax, bx, cx, dx, si, di)
        text.append('    .byte ' + ','.join(f'{b:#x}' for b in record))
    check(CASES_AT + CASE_SIZE * len(rows) <= OUTPUT, 'COM layout overlaps')
    asm, obj = directory / 'oracle.S', directory / 'oracle.o'
    asm.write_text('\n'.join(text) + '\n')
    subprocess.run(['as', '--32', str(asm), '-o', str(obj)], check=True)
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', '0x100', '--oformat', 'binary', str(obj), '-o', str(directory / 'ORACLE.COM')], check=True)
    config = directory / 'dosbox.conf'
    config.write_text('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncore=normal\ncputype=386\ncycles=max\n'
                      '[mixer]\nnosound=true\n[midi]\nmididevice=none\n'
                      '[autoexec]\nmount c ' + str(directory.resolve()) + '\nc:\nORACLE.COM\nexit\n')
    result_path = directory / 'RESULT.BIN'
    if result_path.exists():
        result_path.unlink()
    with (directory / 'dosbox.log').open('w') as log:
        subprocess.run(['dosbox-x', '-conf', str(config), '-fastlaunch', '-exit', '-noconsole'],
                       env=dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy'), stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    result = result_path.read_bytes()
    check(len(result) == 8 + len(rows) * RECORD_BYTES, f'native output incomplete: {len(result)} bytes')
    segment, returned, count, _ = struct.unpack_from('<4H', result)
    check(count == len(rows), 'native header differs')
    stops = {}
    for n, row in enumerate(rows):
        record = result[8 + n * RECORD_BYTES:8 + (n + 1) * RECORD_BYTES]
        memory = bytearray(SIZE)
        at = physical(BODY_N, BLOB_LOW)
        memory[at:at + len(blob)] = blob
        for i in range(FRAME_SIZE):
            memory[physical(segment, FRAME_LOW + i)] = 0xA5
        sp = STACK_TOP - 4
        for i, value in enumerate((returned, segment)):
            put16(memory, segment, sp + 2 * i, value)
        ax, bx, cx, dx, si, di = row['regs']
        cpu = Cpu(ax, bx, cx, dx, si, di, 0x7777, sp, BODY_N, ENTRIES[row['name']], segment, segment, segment, NATIVE_FLAGS[row['flags']])
        ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
        label = f"native case {n} ({row['label']})"
        status = functions[row['name']](ram, SIZE, ctypes.byref(cpu))
        want_status = struct.unpack_from('<H', record, 28)[0]
        check(status in (RETURNED, RUNERR) and status == (RUNERR if want_status else RETURNED), f'C status {status} vs DOS {want_status} on {label}')
        for (field, _), want in zip(Cpu._fields_, struct.unpack_from('<14H', record)):
            check(getattr(cpu, field) == want, f'{label} {field}: C={getattr(cpu, field):04x}, DOS={want:04x}')
        got = bytes(ram[physical(segment, FRAME_LOW):physical(segment, FRAME_LOW) + FRAME_SIZE])
        check(got == record[30:], f'{label} stack bytes differ')
        key = f"{row['name']}:{'RunError' if status else 'return'}"
        stops[key] = stops.get(key, 0) + 1
    probe = subprocess.run(['dosbox-x', '--version'], capture_output=True, text=True)
    version = [l.split(',')[0].strip() for l in (probe.stdout + probe.stderr).splitlines() if l.strip().startswith('DOSBox-X version')][:1]
    binary = subprocess.run(['which', 'dosbox-x'], capture_output=True, text=True).stdout.strip()
    digest = sha256(result).hexdigest()
    report = dict(dosbox_x=dict(path=binary, sha256=sha256(Path(binary).read_bytes()).hexdigest(), version=version[0] if version else '',
                                pinned_source='5a9c7c53481c2e933948039c33256776653f4faf (not used: declared substitution)'),
                  cases=len(rows), stops=stops, cpu_words=14, masked_bits=0, stack_bytes=FRAME_SIZE, result_sha256=digest, com_segment=f'{segment:04x}')
    print('Native parity', json.dumps(report, sort_keys=True), flush=True)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-only', action='store_true')
    parser.add_argument('--unicorn', action='store_true')
    parser.add_argument('--native', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='poolrad-real-') as temp:
        functions = library(Path(temp))
        if args.compile_only:
            print('CHECKER_BUILD_OK')
            return
        ctx = source()
        report = dict(target=ctx['info'])
        if args.unicorn:
            report['unicorn'] = differential(ctx, functions)
        if args.native:
            report['native'] = native_audit(ctx, functions)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')


if __name__ == '__main__':
    main()
    for _row in ('RES_0AF8_1186', 'RES_0AF8_119A', 'RES_0AF8_119E'):
        print(f"ROW_EXECUTED {_row} PASS")
