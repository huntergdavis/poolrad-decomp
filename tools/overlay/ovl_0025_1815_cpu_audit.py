#!/usr/bin/env python3
"""OVL_0025_1815 (GAME.OVR unit 25h entry 15, compare two strings into six flag bytes) as a raw full-state model composed with SYSTEM StrStore 0B25,
FillChar 16A8 and the string compare 0C10, against the original bytes.

The world is u25_move_original_audit.media(): the relocated START.EXE at 1000h (SYSTEM at 1AF8h, the data segment at 1C7Ch) and unit 25h loaded at its own
segment with every fixup applied (the nine LCALL segment words point at SYSTEM). Each case's strings, registers and stack residue come from its own name.
Compared after every case, unmasked: CPU14, ALL 1,048,576 RAM bytes, the stop and the remaining budget (one unit per instruction of THIS body). FLAGS are
exact (no shift, DIV or MUL is executed anywhere in the closure). Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_printer_0626_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
import u25_move_original_audit as U
from hashlib import sha256

CHECKER_ROWS = ('OVL_0025_1815',)
ENTRY, END, INSNS = 0x1815, 0x18EB, 86
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'CODE', 4: 'TRANSFER'}
W, DS, SS, SYS = U.SEG[0x25], U.DS, 0x8000, U.BASE + 0x0AF8
RET = (0x9F00, 0x0000)
SP_MIN, SP_MAX = 0x0220, 0xFFF4
BUDGET = 1000
STR = 0x5000
FLAGS = 0x6F78
CALLS = [(0x182B, '0xb25'), (0x183F, '0xb25'), (0x1850, '0x16a8')] + [(a, '0xc10') for a in (0x1861, 0x1879, 0x1891, 0x18A9, 0x18C1, 0x18D9)]
SOURCES = ['overlay_u25_compare_strings_1815_cpu', 'system_strstore_fullstate_0b25', 'system_runtime_leaves_cpu', 'system_string_move_leaves_cpu']


def source():
    m, meta = U.media()
    body = bytes(m[physical(W, ENTRY):physical(W, END)])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == INSNS and sum(i.size for i in ins) == END - ENTRY, '1815: 214 bytes / 86 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(a, f'{hex(SYS)}, {o}') for a, o in CALLS], 'the nine SYSTEM far calls')
    check([(i.address, i.op_str) for i in ins if i.mnemonic in ('ret', 'retf')] == [(0x18E8, '8')], 'the only return is RETF 8 at 18E8')
    for off, n in ((0x0B25, 0x0B49), (0x16A8, 0x16BC), (0x0C10, 0x0C3B)):
        meta[f'system_{off:04X}'] = sha256(bytes(m[physical(SYS, off):physical(SYS, n)])).hexdigest()
    meta['body1815'] = dict(bytes=END - ENTRY, instructions=INSNS, sha256=sha256(body).hexdigest())
    return m, {i.address: i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('u1815_mut.so' if override else 'u1815.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_compare_strings_1815_cpu.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]
    lib.pr_u25_compare_strings_1815_cpu.restype = ctypes.c_int
    return lib


def fixture(name, base, *, a=b'ABC', b=b'ABC', at_a=(STR, 0x0100), at_b=(STR, 0x0400), sp=0xF000, ss=SS, flags=0x0202, regs=None, writes=(), budget=BUDGET, raw=None):
    """The loaded world, then the two Pascal strings (or raw bytes), stack residue, the six flag bytes' prior values, registers and the frame, from the name."""
    rng = random.Random(f'OVL_0025_1815/{name}')
    m = bytearray(base)
    for k in range(0x260):                                  # the stack residue first: with SS = DS it lies under the flag bytes
        m[physical(ss, (sp - 0x260 + k) & 0xFFFF)] = rng.randrange(256)
    for k in range(6):
        m[physical(DS, FLAGS + k)] = rng.randrange(256)
    for (seg, off), s in ((at_a, a), (at_b, b)):
        if s is None:
            continue
        blob = bytes([len(s) & 0xFF]) + s
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    for k, w in enumerate((RET[1], RET[0], at_b[1], at_b[0], at_a[1], at_a[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    for seg, off, data in writes:
        for j, v in enumerate(data):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    r = regs or tuple(rng.randrange(65536) for _ in range(8))
    c = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, W, ENTRY, DS, r[7], ss, (flags | 0x0002) & ~0x0100)
    return m, c, dict(name=name, a=a, b=b, at_a=at_a, at_b=at_b, sp=sp, budget=budget, plain=not writes)


def run_original(st, body):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    m, c, info = st
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, left=info['budget'], prev=None, steps=0); seen = set(); edges = set()

    def hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        if (cs, ip) == RET:
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        if cs not in (W, SYS) or (cs == W and ip not in body):
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        if cs == W:
            if s['left'] == 0:
                s['stop'] = 'LIMIT'; return uc.emu_stop()
            s['left'] -= 1; seen.add(ip)
            if s['prev'] is not None:
                edges.add((s['prev'], ip))
            s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 200000:
            s['stop'] = 'RUNAWAY'; return uc.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=400000)
    check(s['stop'] in ('RETURNED', 'LIMIT', 'TRANSFER'), f'{info["name"]}: original stop {s["stop"]}')
    cpu = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return s['stop'], bytes(backing), cpu, s['left'], seen, edges


def run_c(lib, st):
    m, c, info = st
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    cpu = Cpu.from_buffer_copy(c); left = ctypes.c_size_t(info['budget'])
    status = lib.pr_u25_compare_strings_1815_cpu(ctypes.addressof(ram), SIZE, ctypes.byref(cpu), ctypes.byref(left))
    return STATUS[status], bytes(ram), cpu, left.value


def compare(st, body, lib):
    name = st[2]['name']
    estop, eram, ecpu, eleft, seen, edges = run_original(st, body)
    astop, aram, acpu, aleft = run_c(lib, st)
    check(astop == estop, f'{name}: stop C {astop} original {estop}')
    check(aleft == eleft, f'{name}: budget left C {aleft} original {eleft}')
    for n, _ in Cpu._fields_:
        check(getattr(acpu, n) == getattr(ecpu, n), f'{name}: {n} C {getattr(acpu, n):04x} original {getattr(ecpu, n):04x}')
    if aram != eram:
        diffs = [i for i in range(SIZE) if aram[i] != eram[i]]
        raise ValueError(f'{name}: RAM {[(hex(i), aram[i], eram[i]) for i in diffs[:8]]} ({len(diffs)} bytes)')
    return estop, eram, ecpu, seen, edges


def statement(st, stop, ram):
    """Without the C model, on plain RETURNED cases whose strings alias neither each other's text, the flag bytes nor the frame: the six flag bytes
    from Pascal's string order (unsigned bytes, then length) and the return."""
    m, c, info = st
    if stop != 'RETURNED' or not info['plain'] or info['a'] is None or info['b'] is None:
        return 0
    spans = [{physical(seg, (off + j) & 0xFFFF) for j in range(1 + len(s))} for (seg, off), s in ((info['at_a'], info['a']), (info['at_b'], info['b']))]
    guard = {physical(DS, FLAGS + k) for k in range(6)} | {physical(SS, (info['sp'] - 0x230 + k) & 0xFFFF) for k in range(0x240)}
    if spans[0] & spans[1] or (spans[0] | spans[1]) & guard:
        return 0
    a, b = info['a'], info['b']
    got = bytes(ram[physical(DS, FLAGS + k)] for k in range(6))
    want = bytes([a == b, a != b, a < b, a > b, a <= b, a >= b])
    check(got == want, f'{info["name"]}: flag bytes {got.hex()}, rule {want.hex()}')
    return 1


def cases(quick=False):
    out = []

    def add(name, **kw):
        out.append((name, kw))
    pairs = [(b'', b''), (b'', b'A'), (b'A', b''), (b'ABC', b'ABC'), (b'ABC', b'ABD'), (b'ABD', b'ABC'), (b'AB', b'ABC'), (b'ABC', b'AB'),
             (b'a', b'A'), (b'\x80', b'\x7f'), (b'\x7f', b'\x80'), (b'\xff' * 255, b'\xff' * 255), (b'\xff' * 255, b'\xff' * 254), (b'x' * 200 + b'a', b'x' * 200 + b'b'),
             (b'Z', b'AAAA'), (b'\x00', b''), (b'\x00\x00', b'\x00')]
    for k, (a, b) in enumerate(pairs):
        add(f'pair{k:02d}', a=a, b=b)
    for fl in (0x0002, 0x0202, 0x0ED7, 0x08D7, 0x0003, 0x0A87, 0x0656):
        add(f'flags-{fl:04x}-eq', flags=fl); add(f'flags-{fl:04x}-lt', flags=fl, a=b'AB', b=b'AC')
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, a=b'HELLO', b=b'HELP')
    for off in (0xFFF0, 0xFFFF, 0xFFFE):                                    # a string wrapping at 16 bits (StrStore's LODSB/MOVSB wrap SI)
        add(f'wrap-a-{off:04x}', at_a=(STR, off), a=b'WRAPPED-TEXT', b=b'WRAPPED-TEXT')
        add(f'wrap-b-{off:04x}', at_b=(STR, off), a=b'WRAP', b=b'WRAPS')
    add('a20-a', at_a=(0xFFFF, 0x0020), a=b'A20', b=b'A20')
    add('same-string', at_b=(STR, 0x0100), a=b'SAME', b=None)
    add('overlap', at_b=(STR, 0x0102), a=b'OVERLAP', b=None)
    for k in range(6):                                                       # a string over the flag bytes: compared as on entry (the copies come first)
        add(f'a-over-flags{k}', at_a=(DS, FLAGS - 1 + k), a=b'\x05FLAGS', b=b'FLAGS')
        add(f'b-over-flags{k}', at_b=(DS, FLAGS + k), a=b'XY', b=b'XY')
    for k in (0x0200, 0x0100, 0x00FF, 0x0002):                               # a string inside the frame's locals or under them: the copy writes over its own source
        add(f'a-in-locals-{k:04x}', at_a=(SS, (0xF000 - 2 - k) & 0xFFFF), a=b'LOCALS', b=b'LOCALS')
        add(f'b-in-locals-{k:04x}', at_b=(SS, (0xF000 - 2 - k) & 0xFFFF), a=b'LOCAL', b=b'LOCALS')
    for k in (0x00, 0x04, 0x06, 0x08):                                       # a string over the frame's own argument words
        add(f'a-on-frame-{k:02x}', at_a=(SS, (0xF000 + k) & 0xFFFF), a=None, b=b'FRAME')
    for sp in (0x6F78, 0x6F7A):                                              # SS = DS: FillChar's six zeros land on this body's return address (TRANSFER)
        add(f'ret-under-flags-{sp:04x}', ss=DS, sp=sp, a=b'AB', b=b'AB')
    for b in range(0, 90, 1 if not quick else 3):
        add(f'budget-{b}', budget=b, a=b'AB', b=b'AC')
    for k in range(8 if quick else 80):
        rng = random.Random(f'OVL_0025_1815/random{k}')
        la, lb = rng.choice((0, 1, 2, 5, 255, rng.randrange(256))), rng.choice((0, 1, 2, 5, 255, rng.randrange(256)))
        alpha = rng.choice((b'AB', bytes(range(256))))
        a = bytes(rng.choice(alpha) for _ in range(la)); b = a[:lb] if rng.random() < 0.4 else bytes(rng.choice(alpha) for _ in range(lb))
        add(f'random{k}', a=a, b=b, at_a=(rng.randrange(0x2000, 0x7000), rng.randrange(65536)), at_b=(rng.randrange(0x2000, 0x7000), rng.randrange(65536)),
            flags=rng.randrange(65536) & 0x0ED7, sp=rng.randrange(0x0400, SP_MAX + 1), regs=tuple(rng.randrange(65536) for _ in range(8)))
    return out


def differential(base, body, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; stated = 0; seen = set(); edges = set()
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw) in enumerate(cs):
        if index % 50 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(name, base, **kw)
        stop, ram, cpu, s, e = compare(st, body, lib)
        stops[stop] = stops.get(stop, 0) + 1; seen |= s; edges |= e
        stated += statement(st, stop, ram)
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(only is not None or seen == set(body), f'starts not reached {sorted(hex(a) for a in set(body) - seen)}')
    check(only is not None or outcomes <= edges, f'outcomes not reached {sorted(outcomes - edges)}')
    return dict(cases=len(cs), stops=dict(sorted(stops.items())), starts=f'{len(seen & set(body))}/{len(body)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = tuple(f'pair{k:02d}' for k in (0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 12)) + ('flags-0ed7-eq', 'flags-0ed7-lt', 'sp-0220', 'wrap-a-ffff', 'a-over-flags1', 'b-over-flags3',
                                                                                    'a-in-locals-0100', 'a-on-frame-04', 'budget-0', 'budget-12', 'budget-27', 'budget-45', 'budget-75', 'budget-87', 'ret-under-flags-6f78', 'ret-under-flags-6f7a')
MUTANTS = (
    ('a copied to [BP-200h] (the locals swapped)', [("STEP(0x1821u); c->di = (uint16_t)(c->bp - 0x0100u);", "STEP(0x1821u); c->di = (uint16_t)(c->bp - 0x0200u);")]),
    ('StrStore limit FEh', [("STEP(0x1827u); c->ax = 0x00FFu;", "STEP(0x1827u); c->ax = 0x00FEu;")]),
    ('the clear is five bytes', [("STEP(0x1849u); c->ax = 0x0006u;", "STEP(0x1849u); c->ax = 0x0005u;")]),
    ('the clear starts at 6F79h', [("STEP(0x1844u); c->di = 0x6F78u;", "STEP(0x1844u); c->di = 0x6F79u;")]),
    ('JAE as JA ([6F7A] on a <= b)', [("COMPARE(0x1885u, 0x6F7Au, CF);", "COMPARE(0x1885u, 0x6F7Au, CF || ZF);")]),
    ('JB as JBE ([6F7D] on a > b only)', [("COMPARE(0x18CDu, 0x6F7Du, !CF);", "COMPARE(0x18CDu, 0x6F7Du, !CF && !ZF);")]),
    ('the a <> b flag at 6F78', [("COMPARE(0x186Du, 0x6F79u, !ZF);", "COMPARE(0x186Du, 0x6F78u, !ZF);")]),
    ('the compare arguments swapped', [("STEP((at_)); c->di = (uint16_t)(c->bp - 0x0100u); \\", "STEP((at_)); c->di = (uint16_t)(c->bp - 0x0200u); \\"),
                                       ("STEP((uint16_t)((at_) + 6u)); c->di = (uint16_t)(c->bp - 0x0200u); \\", "STEP((uint16_t)((at_) + 6u)); c->di = (uint16_t)(c->bp - 0x0100u); \\")]),
    ('RETF 6', [("c->sp = (uint16_t)(c->sp + 8u);             /* 18E8 RETF 8 */", "c->sp = (uint16_t)(c->sp + 6u);             /* 18E8 RETF 8 */")]),
    ('SUB SP,1FEh', [("c->sp = sub16(c, c->sp, 0x0200u);", "c->sp = sub16(c, c->sp, 0x01FEu);")]),
    ('the far call not charged to the budget', [("    --*budget; push(ram, c, cs); push(ram, c, (next_));", "    push(ram, c, cs); push(ram, c, (next_));")]),
    ('the return-address check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U25_1815_TRANSFER;\n    return PR_U25_1815_RETURNED;", "(void)ret_ip; (void)ret_cs;\n    return PR_U25_1815_RETURNED;")]),
    ('LES segment from [BP+0Ch] ignored (DS used)', [("STEP(0x181Fu); push(ram, c, c->es);", "STEP(0x181Fu); push(ram, c, c->ds);")]),
)


def mutation_check(base, body):
    src = (ROOT / 'src/recovered/overlay_u25_compare_strings_1815_cpu.c').read_text()
    with tempfile.TemporaryDirectory(prefix='u1815-base-') as tmp:                       # the unmutated model must pass the same subset first (PM i22687)
        baseline = differential(base, body, library(tmp), quick=True, only=MUTANT_CASES)
    check(baseline['cases'] == len(MUTANT_CASES), f'baseline ran {baseline["cases"]} of {len(MUTANT_CASES)} mutation cases')
    out = []
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u1815-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u25_compare_strings_1815_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR (not counted)')); continue
            try:
                differential(base, body, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:110]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:80]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    built = [r for r in out if not r[1].startswith('COMPILE-ERROR')]
    return dict(baseline=dict(cases=baseline['cases'], stops=baseline['stops'], mismatches=0), total=len(built), caught=sum(1 for _, r in built if r.startswith('CAUGHT:')),
                not_built=[n for n, r in out if r.startswith('COMPILE-ERROR')], survivors=[n for n, r in built if not r.startswith('CAUGHT:')], results=out, cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u1815-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        base, body, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(base, body, lib, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(base, body)
        if a.native:
            from ovl_0025_1815_native import native
            report['native'] = native(base, body, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_0025_1815_cpu_audit'] = sys.modules[__name__]
    main()
