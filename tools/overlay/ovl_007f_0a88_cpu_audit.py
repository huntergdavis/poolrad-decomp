#!/usr/bin/env python3
"""OVL_007F_0A88 (unit 0x7F [0A88,0B5A)) composed with +0054 (+0000 and the resident 13AB/11EA closure), StrLoad, +003A and the resident 1693 display closure, against the original bytes.

The body asks +0054 whether adding AMOUNT to the record word at +102h exceeds the class value; if it does it shows a 10-byte message through 1693 (clear, print, delay, clear on the EGA) and returns.
Otherwise it clamps AMOUNT to the 32-bit pool at DS:[67F4h+4I] (signed 32-bit compare), subtracts it from the pool, adds it to the record word at +88h+2I and calls +003A, which adds it to +102h.
Every callee runs as its ORIGINAL bytes in the oracle and as its merged C model in the C run. The record word at +88h+2I is read and written through the bus. Compared after every case: CPU14,
ALL 1,048,576 RAM bytes, the EGA planes, latches and registers, the ordered OUT events and VRAM operands, the stop and the remaining budget. FLAGS are compared exactly except on a stop whose
last FLAGS writer is a non-zero SHL (the index scaling), where the declared pinned AF split applies (tools/overlay/pinned_shift_flags.py; counted). Scope, declared: the budget counts this body,
+0054, +0000 and the 13AB closure; the 1693 closure's own loop-back limits and refusal stops stay with its own audit and are not exercised here. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from pathlib import Path
import ovl_007f_world1693 as WD
import ovl_007f_0054_cpu_audit as B54
import ovl_007f_0000_cpu_audit as A
from res0709_printer_0626_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

CHECKER_ROWS = ('OVL_007F_0A88',)
ENTRY = 0x0A88
SP_MIN, SP_MAX = 0x0200, 0xFFF4
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'LIMIT', 4: 'TRANSFER', 5: 'CALLEE'}
BUDGET = 100000
ALIAS_BUDGET = 30000
G0 = (0x3000, 0x0100)
POOL = 0x67F4


class Diag(ctypes.Structure):
    _fields_ = [('raw', ctypes.c_uint8 * 4096)]


def library(directory, override=None):
    lib = WD.library(directory, override)
    f = lib.pr_u7f_0a88_cpu
    f.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(Diag)]; f.restype = ctypes.c_int
    return lib


def call(fn, bus, size, cpu, left, lim):
    diag = Diag(); status = fn(bus, size, cpu, left, ctypes.byref(lim), ctypes.byref(diag))
    return STATUS[status], diag


def source():
    bodies, decoded, meta = WD.source()
    ins = [(k[1], i) for k, i in decoded.items() if k[0] == 'w' and 0xA88 <= k[1] < 0xB5A]
    calls = sorted((a, i.mnemonic, i.op_str) for a, i in ins if i.mnemonic in ('call', 'lcall'))
    check(calls == [(0xA9D, 'call', '0x54'), (0xAAE, 'lcall', '0xaf8, 0xb0b'), (0xAB3, 'lcall', '0xba, 0x1693'), (0xB51, 'call', '0x3a')], f'the four call sites of +0A88: {calls}')
    check(sorted((a, i.mnemonic) for a, i in ins if i.mnemonic.startswith('j') or i.mnemonic == 'retf') ==
          [(0xAA2, 'je'), (0xAB8, 'jmp'), (0xAD4, 'jg'), (0xAD6, 'jl'), (0xADA, 'jbe'), (0xB57, 'retf')], 'JE 0ABB, JMP 0B54, JG/JL/JBE (the pool compare) and RETF 8')
    meta = dict(meta, body0a88=dict(bytes=210, instructions=87, sha256=WD.SPANS[0xA88][3]))
    return bodies, decoded, meta


def fixture(bodies, *, amount=0x0010, index=0x0002, kind=0x12, minor=0x20, w=0x0100, money=0x1000, pool=(0x2000, 0), g=G0, sp=0x8000, flags=0x0202, regs=None, world=None, rec_bytes=None):
    """D's world, then the record at G (class bytes at +10h/+16h, the money word at +88h+2I, W at +102h), the pool dword at DS:[67F4h+4I], the CPU and the entry frame (index word, AMOUNT, G)."""
    m, c = WD.base_state(bodies, **(world or {}))
    ds, ss = c.ds, c.ss
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    i = (index & 0xFF) - 0x100 if index & 0x80 else index & 0xFF
    mo = (0x88 + 2 * i) & 0xFFFF
    for k in range(0x110):
        m[physical(g[0], (g[1] + k) & 0xFFFF)] = (k * 5 + 1) & 0xFF
    rb = {0x10: kind & 0xFF, 0x16: minor & 0xFF, mo: money & 0xFF, (mo + 1) & 0xFFFF: (money >> 8) & 0xFF, 0x102: w & 0xFF, 0x103: (w >> 8) & 0xFF}
    rb.update(rec_bytes or {})
    for off, v in rb.items():
        m[physical(g[0], (g[1] + off) & 0xFFFF)] = v & 0xFF
    put16(m, ds, (POOL + 4 * i) & 0xFFFF, pool[0]); put16(m, ds, (POOL + 2 + 4 * i) & 0xFFFF, pool[1])
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, WD.W, ENTRY, ds, r[7], ss, flags | 0x0002)
    for k, v in enumerate((WD.ENTRY_RET[1], WD.ENTRY_RET[0], index & 0xFFFF, amount & 0xFFFF, g[1], g[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    return m, cpu, dict(amount=amount, index=i, kind=kind, minor=minor, w=w, money=money, pool=pool, g=g, sp=sp, ds=ds, ss=ss, mo=mo)


def run_pair(label, st, bodies, decoded, lib, budget):
    m, cpu, info = st
    expected = WD.run_original(m, cpu, decoded, budget)
    actual = WD.run_c(lib, lib.pr_u7f_0a88_cpu, m, cpu, call, budget)
    seen = WD.compare_states(label, expected, actual, bodies, m, decoded)
    return expected, actual, seen


# ---------------------------------------------------------------------------------------------------- cases
def cases(bodies54, decoded54, quick=False):
    rng = random.Random(0x7F0A88)
    out = []

    def add(name, budget=BUDGET, **kw):
        out.append((name, kw, budget))
    vcache = {}
    V = lambda k, mi=0x20: vcache.setdefault((k, mi), B54.original_value(bodies54, decoded54, k, mi, A.W_CS, 0x7000))
    for k in (0x0C, 0x10, 0x12, 0x13, 0x1D):                           # +0054's boundary: W + AMOUNT against the class value
        v = V(k)
        for w in (0x0000, 0x0001, 0x1234, 0xFFFF):
            for delta, tag in ((-1, 'm1'), (0, 'eq'), (1, 'p1'), (0x8000, 'far')):
                add(f'b-{k:02x}-{w:04x}-{tag}', kind=k, w=w, amount=((v + delta) - w) & 0xFFFF, pool=(0xFFFF, 0x0000))
    for hi in (0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF):                # the pool compare: JG / JL on the high word, JBE on the low word
        for lo in (0x0000, 0x0004, 0x0005, 0x0006, 0xFFFF):
            add(f'pool-{hi:04x}-{lo:04x}', pool=(lo, hi), amount=0x0005, w=0)
    for amount in (0x0000, 0x0001, 0x00FF):
        for pool in ((0, 0), (amount, 0), ((amount - 1) & 0xFFFF, 0)):
            add(f'amt-{amount:04x}-pool-{pool[0]:04x}', amount=amount, pool=pool, w=0)
    for index in list(range(0, 8)) + [0x7F, 0x80, 0xFF, 0xFE, 0x40]:   # the seven kinds, past them, and signed (CBW) indices
        add(f'index-{index:02x}', index=index, amount=0x0003, pool=(0x0010, 0), w=0)
        add(f'index-{index:02x}-hi', index=0xA500 | index, amount=0x0003, pool=(0x0001, 0), w=0)
    for money in (0x0000, 0xFFFF, 0x7FFF, 0xFFFE):
        add(f'money-{money:04x}', money=money, amount=0x0002, pool=(0x0100, 0), w=0)
    for m_ in (0, 1, 2, 3):
        add(f'mode{m_}-above', world=dict(mode=m_), amount=0x7000)
    for active in (0, 3, 7):
        add(f'page{active}-above', world=dict(active=active), amount=0x7000)
    for factor, count in ((1, 1), (2, 2), (0, 5)):
        add(f'delay-{factor}-{count}-above', world=dict(factor=factor, count=count), amount=0x7000)
    for off in (0x0000, 0xFE00, 0xFF70, 0xFF77, 0xFF76, 0xFEFD, 0xFFF0):
        add(f'g-off-{off:04x}-above', g=(0x3000, off), amount=0x7000); add(f'g-off-{off:04x}-not', g=(0x3000, off), amount=1, w=0)
    for seg in (0xFFFF, 0xFFF0, 0x0000):
        add(f'g-seg-{seg:04x}', g=(seg, 0x0200), amount=1, w=0)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}-above', sp=sp, amount=0x7000); add(f'sp-{sp:04x}-not', sp=sp, amount=1, w=0)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, amount=1, w=0); add(f'flags-{fl:04x}-above', flags=fl, amount=0x7000)
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x12, 0x10):                                       # the money word on each byte of the frame: the 0Eh locals, saved BP, return address, index, AMOUNT, G
        add(f'money-at-bp{k:+03x}', budget=ALIAS_BUDGET, g=(0x2000, (BP + k - 0x88 - 4) & 0xFFFF), sp=S0, amount=1, w=0)
    for k in range(-0x12, 0x10):                                       # W (G+102h) on those bytes: +0054 reads it twice, +003A adds to it
        for branch, amount in (('above', 0x7000), ('not', 1)):
            add(f'g102-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, g=(0x2000, (BP + k - 0x102) & 0xFFFF), sp=S0, amount=amount, w=0)
    for t in (0, 1, 2, 3):                                             # the money word over the pool dword itself (index 2: pool at 67FCh)
        add(f'money-over-pool-{t}', budget=ALIAS_BUDGET, g=(0x5000, (POOL + 8 + t - 0x88 - 4) & 0xFFFF), amount=3, w=0, pool=(0x0010, 0))
    for budget in list(range(0, 130)) + [150, 200, 260]:
        add(f'budget-above-{budget}', budget=budget, amount=0x7000)
    for budget in range(0, 200):
        add(f'budget-not-{budget}', budget=budget, amount=0x0005, pool=(0x0003, 0), w=0)
    for budget in range(150, 200):
        add(f'budget-noclamp-{budget}', budget=budget, amount=0x0005, pool=(0x0300, 0), w=0)
    for k in range(10 if quick else 40):
        add(f'random{k}', kind=rng.randrange(256), minor=rng.randrange(256), w=rng.randrange(65536), amount=rng.randrange(65536), money=rng.randrange(65536),
            index=rng.choice((rng.randrange(7), rng.randrange(65536))), pool=(rng.randrange(65536), rng.choice((0, 1, 0xFFFF, rng.randrange(65536)))),
            regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), g=(rng.randrange(0x1000, 0x2000), rng.randrange(65536)),
            sp=rng.randrange(SP_MIN, SP_MAX + 1), world=dict(mode=rng.choice((0, 1, 2, 3)), active=rng.randrange(8)))
    return out


def expected_transfer(name):
    return name.startswith(('money-at-bp', 'g102-at-bp'))


def statements(name, kw, st, expected, vcache, b54):
    """Independent of the C model, on cases whose record does not overlap the stack, the pool or the world's own data: the branch, the clamp, the pool, the money word, W, the message and the stack."""
    status, ram, events, writes, ega, out, left, seen = expected[:8]
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    g = info['g']; i = info['index']
    recs = [physical(g[0], (g[1] + o) & 0xFFFF) for o in (0x10, 0x16, info['mo'], (info['mo'] + 1) & 0xFFFF, 0x102, 0x103)]
    lo = physical(info['ss'], (cpu.sp - 0x60) & 0xFFFF); pool_at = physical(info['ds'], (POOL + 4 * i) & 0xFFFF)
    if any(lo - 1 <= a < lo + 0x70 or pool_at - 1 <= a < pool_at + 4 for a in recs) or g[0] in (0x2000, 0x5000, 0x4000, 0x6000, 0x7000, 0x8000, 0x9000):
        return 0
    if any(((g[1] + o) & 0xFFFF) == 0xFFFF for o in (info['mo'], 0x102)) or ((POOL + 4 * i) & 0xFFFF) > 0xFFFC:   # a word straddling offset FFFFh: differential only
        return 0
    key = (info['kind'], info['minor'])
    v = vcache.setdefault(key, B54.original_value(b54[0], b54[1], key[0], key[1], A.W_CS, 0x7000))
    w, amount, money = info['w'], info['amount'], info['money']
    pool = info['pool'][0] | (info['pool'][1] << 16)
    above = ((w + amount) & 0xFFFF) > v
    signed_pool = pool - (1 << 32) if pool & 0x80000000 else pool
    moved = 0 if above else (pool & 0xFFFF if signed_pool < amount else amount)
    word = lambda a: ram[a] | (ram[(a + 1) & (SIZE - 1)] << 8)
    check(word(recs[2]) == (money + moved) & 0xFFFF, f'{name}: money word {word(recs[2]):04x} is not {(money + moved) & 0xFFFF:04x}')
    check(word(recs[4]) == (w + moved) & 0xFFFF, f'{name}: record word {word(recs[4]):04x} is not {(w + moved) & 0xFFFF:04x}')
    got = word(pool_at) | (word((pool_at + 2) & (SIZE - 1)) << 16)
    check(got == (pool - moved) & 0xFFFFFFFF, f'{name}: pool {got:08x} is not {(pool - moved) & 0xFFFFFFFF:08x}')
    shown = ram[physical(info['ss'], (cpu.sp - 2 - 0x0D) & 0xFFFF)] == 10                       # StrLoad left the 10-byte message's length byte at [BP-0Dh]; the other branch never wrote it
    check(shown == above, f'{name}: the message temporary says shown={shown} but the branch above is {above}')
    check(out.sp == (cpu.sp + 12) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == WD.ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, lib, b54, quick=False, only=None):
    t0 = time.time(); stops = {}; seen_all = set(); stated = 0; vcache = {}; WD.SHIFT_SPLIT_ARRIVALS[0] = 0
    cs = [c for c in cases(b54[0], b54[1], quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 40 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual, seen = run_pair(name, st, bodies, decoded, lib, budget)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen_all |= seen
        if actual[0] in ('ARGUMENT', 'CODE', 'CALLEE') or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == BUDGET:
            stated += statements(name, kw, st, expected, vcache, b54)
    body = {k for k in seen_all if k[0] == 'w' and 0xA88 <= k[1] < 0xB5A}
    check(only is not None or len(body) == 87, f'every +0A88 instruction start was reached ({len(body)}/87)')
    return dict(cases=len(cs), end_states=stops, starts_0a88=f'{len(body)}/87', starts_total=len(seen_all), independent_statements_checked=stated, declared_mulw_split_states=WD.SPLIT_ARRIVALS[0],
                declared_shift_af_split_states=WD.SHIFT_SPLIT_ARRIVALS[0], cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('b-12-1234-eq', 'b-12-1234-p1', 'b-12-0000-m1', 'b-0c-ffff-far', 'pool-0000-0004', 'pool-0000-0005', 'pool-0000-0006', 'pool-0001-0000', 'pool-ffff-0005', 'pool-8000-0000',
                'pool-7fff-ffff', 'index-00', 'index-06', 'index-80', 'index-ff-hi', 'index-02-hi', 'money-ffff', 'mode1-above', 'money-at-bp+02', 'money-at-bp+00', 'g102-at-bp-01-above', 'g102-at-bp+00-not',
                'sp-0200-above', 'flags-0a87', 'flags-0a87-above', 'money-over-pool-0') + \
               tuple(f'budget-above-{b}' for b in (3, 10, 11, 12, 40, 100, 120)) + tuple(f'budget-not-{b}' for b in (11, 12, 13, 14, 15, 60, 120, 150, 170, 185)) + \
               tuple(f'budget-noclamp-{b}' for b in (160, 175, 190))
MUTANTS = (
    ('JE sense inverted', [("c->ip = (c->flags & 0x0040u) ? 0x0ABBu : 0x0AA4u;", "c->ip = (c->flags & 0x0040u) ? 0x0AA4u : 0x0ABBu;")]),
    ('the message is at 0A7Eh', [("c->di = 0x0A7Du;", "c->di = 0x0A7Eu;")]),
    ('the money word is at +8Ah', [("STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x88u), c->dx);", "STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x8Au), c->dx);")]),
    ('JG as unsigned above', [("clamp = !(c->flags & 0x0040u) && !(c->flags & 0x0080u) == !(c->flags & 0x0800u);", "clamp = !(c->flags & 0x0041u);")]),
    ('JBE as JB', [("clamp = !(c->flags & 0x0041u); c->ip = clamp", "clamp = !(c->flags & 0x0001u); c->ip = clamp")]),
    ('SBB without the borrow', [("arith16(c, c->dx, c->bx, 1, (c->flags & 1u))", "arith16(c, c->dx, c->bx, 1, 0)")]),
    ('the pool high word stored at 67F4', [("wr16(ram, c->ds, (uint16_t)(c->di + 0x67F6u), c->bx);", "wr16(ram, c->ds, (uint16_t)(c->di + 0x67F4u), c->bx);")]),
    ('the index zero-extended (no CBW)', [("static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }", "static void cbw(cpu_t *c) { c->ax = (uint16_t)(uint8_t)c->ax; }")]),
    ('the index byte read from [BP+7]', [("rd8(ram, c->ss, (uint16_t)(c->bp + 6))", "rd8(ram, c->ss, (uint16_t)(c->bp + 7))")]),
    ('the clamp keeps AMOUNT', [("STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + 8), c->ax); c->ip = 0x0AEDu;", "STEP(); c->ip = 0x0AEDu;")]),
    ('+003A gets the index word', [("STEP(); push(ram, c, bp16(ram, c, 8)); c->ip = 0x0B50u;", "STEP(); push(ram, c, bp16(ram, c, 6)); c->ip = 0x0B50u;")]),
    ('RETF pops 6 argument bytes', [("c->sp = (uint16_t)(c->sp + 8u);", "c->sp = (uint16_t)(c->sp + 6u);")]),
    ('SUB SP,10h', [("arith16(c, c->sp, 0x000Eu, 1, 0)", "arith16(c, c->sp, 0x0010u, 1, 0)")]),
    ('the excess local is at BP-4', [("STEP(); c->di = lea_bp(c, -2); c->ip = 0x0A9Au;", "STEP(); c->di = lea_bp(c, -4); c->ip = 0x0A9Au;")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_0A88_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the call-site displacement of +0054 expects F5B5h', [("rd16(ram, cs, 0x0A9Eu) != 0xF5B4u", "rd16(ram, cs, 0x0A9Eu) != 0xF5B5u")]),
    ('the +0054 call is not charged to the budget', [("--*budget; push(ram, c, 0x0AA0u); c->ip = 0x0054u; d->callee = 0;", "push(ram, c, 0x0AA0u); c->ip = 0x0054u; d->callee = 0;")]),
    ('the 1693 call is not charged to the budget', [("--*budget; d->callee = 2;", "d->callee = 2;")]),
    ('SHL leaves AF clear', [("f = (uint16_t)(szp(r) | 0x0010u);", "f = szp(r);")]),
    ('SHL without OF', [("if ((v ^ r) & 0x8000u) f |= 0x0800u;", "(void)0;")]),
    ('the money read bypasses the bus', [("STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x88u));", "STEP(); c->ax = rd16(bus->ram, c->es, (uint16_t)(c->di + 0x88u)); (void)brd16;")]),
    ('OR AL,AL tests 16 bits', [("logic(c, (uint8_t)c->ax, 8u); c->ip = 0x0AA2u;", "logic(c, (uint8_t)c->ax, 16u); c->ip = 0x0AA2u;")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u7f_0a88_cpu' else ROOT / f'src/recovered/{s}.c') for s in WD.SOURCES if (ROOT / f'src/recovered/{s}.c').exists()]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u7f_0a88_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded, b54):
    out = []; src = (ROOT / 'src/recovered/overlay_u7f_0a88_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u7f-0a88-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u7f_0a88_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, mlib, b54, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(1 for _, r in out if r.startswith('CAUGHT:')); c = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out,
                survivors=[n for n, r in out if (r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')) or r == 'COMPILE-ERROR'], cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u7f-0a88-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); b54 = B54.source()[:2]; report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, b54, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded, b54)
        if a.native:
            from ovl_007f_0a88_native import native
            report['native'] = native(bodies, decoded, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_007f_0a88_cpu_audit'] = sys.modules[__name__]
    main()
