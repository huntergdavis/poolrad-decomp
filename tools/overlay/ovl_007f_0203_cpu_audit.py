#!/usr/bin/env python3
"""OVL_007F_0203 (unit 0x7F [0203,0294)) composed with +0054 (+0000 and the resident 13AB/11EA closure), StrLoad, +003A and the resident 1693 display closure, against the original bytes.

The body asks +0054 whether adding AMOUNT to the record word at G+102h exceeds the class value; if it does it shows a message through 1693 (clear, print, delay, clear on the EGA), adds the excess to
the record word at G+90h, calls +003A and moves AMOUNT minus the excess into a 32-bit counter; otherwise it adds AMOUNT to G+90h and calls +003A. Every callee runs as its ORIGINAL bytes in the oracle
and as its merged C model in the C run. The record word at G+90h is read and written through the bus. Compared unmasked after every case: CPU14, ALL 1,048,576 RAM bytes, the EGA planes, latches and
registers, the ordered OUT events and VRAM operands, the stop and the remaining budget. Scope, declared: the budget counts this body, +0054, +0000 and the 13AB closure; the 1693 closure's own
loop-back limits and refusal stops stay with its own audit and are not exercised here. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from pathlib import Path
import ovl_007f_world1693 as WD
import ovl_007f_0054_cpu_audit as B54
import ovl_007f_0000_cpu_audit as A
from res0709_printer_0626_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

CHECKER_ROWS = ('OVL_007F_0203',)
ENTRY = 0x0203
SP_MIN, SP_MAX = 0x0200, 0xFFF8
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'LIMIT', 4: 'TRANSFER', 5: 'CALLEE'}
BUDGET = 100000
ALIAS_BUDGET = 30000
G0 = (0x3000, 0x0100)


class Diag(ctypes.Structure):
    _fields_ = [('raw', ctypes.c_uint8 * 4096)]


def library(directory, override=None):
    lib = WD.library(directory, override)
    f = lib.pr_u7f_0203_cpu
    f.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(Diag)]; f.restype = ctypes.c_int
    return lib


def call(fn, bus, size, cpu, left, lim):
    diag = Diag(); status = fn(bus, size, cpu, left, ctypes.byref(lim), ctypes.byref(diag))
    return STATUS[status], diag


def source():
    bodies, decoded, meta = WD.source()
    ins = [(k[1], i) for k, i in decoded.items() if k[0] == 'w' and 0x203 <= k[1] < 0x294]
    calls = sorted((a, i.mnemonic, i.op_str) for a, i in ins if i.mnemonic in ('call', 'lcall'))
    check(calls == [(0x21A, 'call', '0x54'), (0x22B, 'lcall', '0xaf8, 0xb0b'), (0x230, 'lcall', '0xba, 0x1693'), (0x24D, 'call', '0x3a'), (0x28B, 'call', '0x3a')], f'the five call sites of +0203: {calls}')
    check(sorted((a, i.mnemonic) for a, i in ins if i.mnemonic.startswith('j') or i.mnemonic == 'retf') == [(0x21F, 'je'), (0x271, 'jmp'), (0x291, 'retf')], 'one conditional (JE 0273), one JMP 028E and RETF 2')
    meta = dict(meta, body0203=dict(bytes=145, instructions=53, sha256=WD.SPANS[0x203][3]))
    return bodies, decoded, meta


def fixture(bodies, *, amount=0x0010, kind=0x12, minor=0x20, w=0x0100, acc=0x1000, ctr=(0, 0), g=G0, sp=0x8000, flags=0x0202, regs=None, world=None, rec_bytes=None):
    """D's world, then the record at G (class bytes at +10h/+16h, the accumulator word at +90h, W at +102h), the globals DS:[5D92] (G) and DS:[6804] (the long counter), the CPU and the entry frame."""
    m, c = WD.base_state(bodies, **(world or {}))
    ds, ss = c.ds, c.ss
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    rb = {0x10: kind & 0xFF, 0x16: minor & 0xFF, 0x90: acc & 0xFF, 0x91: (acc >> 8) & 0xFF, 0x102: w & 0xFF, 0x103: (w >> 8) & 0xFF}
    rb.update(rec_bytes or {})
    for k in range(0x110):
        m[physical(g[0], (g[1] + k) & 0xFFFF)] = (k * 5 + 1) & 0xFF
    for off, v in rb.items():
        m[physical(g[0], (g[1] + off) & 0xFFFF)] = v & 0xFF
    put16(m, ds, 0x5D92, g[1]); put16(m, ds, 0x5D94, g[0]); put16(m, ds, 0x6804, ctr[0]); put16(m, ds, 0x6806, ctr[1])
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, WD.W, ENTRY, ds, r[7], ss, flags | 0x0002)
    for k, v in enumerate((WD.ENTRY_RET[1], WD.ENTRY_RET[0], amount & 0xFFFF)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    return m, cpu, dict(amount=amount, kind=kind, minor=minor, w=w, acc=acc, ctr=ctr, g=g, sp=sp, ds=ds, ss=ss)


def run_pair(label, st, bodies, decoded, lib, budget):
    m, cpu, info = st
    expected = WD.run_original(m, cpu, decoded, budget)
    actual = WD.run_c(lib, lib.pr_u7f_0203_cpu, m, cpu, call, budget)
    seen = WD.compare_states(label, expected, actual, bodies, m, decoded)
    return expected, actual, seen


# ---------------------------------------------------------------------------------------------------- cases
def cases(bodies54, decoded54, quick=False):
    rng = random.Random(0x7F0203)
    out = []

    def add(name, budget=BUDGET, **kw):
        out.append((name, kw, budget))
    vcache = {}
    V = lambda k, mi=0x20: vcache.setdefault((k, mi), B54.original_value(bodies54, decoded54, k, mi, A.W_CS, 0x7000))
    for k in (0x0C, 0x10, 0x12, 0x13, 0x1D):
        v = V(k)
        for w in (0x0000, 0x0001, 0x1234, 0xFFFF):
            for delta, tag in ((-1, 'm1'), (0, 'eq'), (1, 'p1'), (0x8000, 'far')):
                add(f'b-{k:02x}-{w:04x}-{tag}', kind=k, w=w, amount=((v + delta) - w) & 0xFFFF)
    v = V(0x12)
    for ctr in ((0, 0), (0xFFFF, 0), (0xFFFF, 0xFFFF), (0x8000, 0x7FFF), (1, 0), (0, 0xFFFF)):
        for amount in (0x7000, 0x0001, 0xFFFF):
            add(f'ctr-{ctr[0]:04x}-{ctr[1]:04x}-{amount:04x}', ctr=ctr, amount=amount, w=0x0100, kind=0x12)
    for acc in (0x0000, 0xFFFF, 0x7FFF, 0x8000):
        for amount in (0x7000, 0x0000):
            add(f'acc-{acc:04x}-{amount:04x}', acc=acc, amount=amount)
    for m_ in (0, 1, 2, 3):
        add(f'mode{m_}-above', world=dict(mode=m_), amount=0x7000)
    for active in (0, 3, 7):
        add(f'page{active}-above', world=dict(active=active), amount=0x7000)
    for factor, count in ((1, 1), (2, 2), (0, 5)):
        add(f'delay-{factor}-{count}-above', world=dict(factor=factor, count=count), amount=0x7000)
    for off in (0x0000, 0xFE00, 0xFF70, 0xFF6F, 0xFEFD, 0xFFF0):
        add(f'g-off-{off:04x}-above', g=(0x3000, off), amount=0x7000); add(f'g-off-{off:04x}-not', g=(0x3000, off), amount=0)
    for seg in (0xFFFF, 0xFFF0, 0x0000):
        add(f'g-seg-{seg:04x}', g=(seg, 0x0200), amount=0x7000)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}-above', sp=sp, amount=0x7000); add(f'sp-{sp:04x}-not', sp=sp, amount=0)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, amount=0x7000)
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x34, 0x10):                                       # G+90h (the accumulator) on each byte of the body's frame: the 42-byte string temporary, the excess local, the saved BP, the return address, AMOUNT
        for branch, amount in (('above', 0x7000), ('not', 0)):
            add(f'g90-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, g=(0x2000, (BP + k - 0x90) & 0xFFFF), sp=S0, amount=amount)
    for k in range(-0x30, 0x10):                                       # W (G+102h) on those bytes: +0054 reads it twice, +003A adds to it
        for branch, amount in (('above', 0x7000), ('not', 0)):
            add(f'g102-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, g=(0x2000, (BP + k - 0x102) & 0xFFFF), sp=S0, amount=amount)
    for t in (0, 2, 4):                                                # G+90h over the globals G and the counter
        add(f'g90-over-g-{t}', budget=ALIAS_BUDGET, g=(0x5000, (0x5D92 + t - 0x90) & 0xFFFF), amount=0x7000)
        add(f'g90-over-ctr-{t}', budget=ALIAS_BUDGET, g=(0x5000, (0x6804 + t - 0x90) & 0xFFFF), amount=0x7000)
    for budget in list(range(0, 170)) + [200, 230, 260, 300, 400]:
        add(f'budget-above-{budget}', budget=budget, amount=0x7000)
    for budget in range(0, 150):
        add(f'budget-not-{budget}', budget=budget, amount=0)
    for k in range(10 if quick else 40):
        add(f'random{k}', kind=rng.randrange(256), minor=rng.randrange(256), w=rng.randrange(65536), amount=rng.randrange(65536), acc=rng.randrange(65536), ctr=(rng.randrange(65536), rng.randrange(65536)),
            regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), g=(rng.randrange(0x1000, 0x2000), rng.randrange(65536)),
            sp=rng.randrange(SP_MIN, SP_MAX + 1), world=dict(mode=rng.choice((0, 1, 2, 3)), active=rng.randrange(8)))
    return out


def expected_transfer(name):
    return name.startswith(('g90-at-bp', 'g102-at-bp'))


def statements(name, kw, st, expected, vcache, b54):
    """Independent of the C model, on cases whose record does not overlap the stack, the globals or the world's own data: the branch, the accumulator word, W, the counter, the message temporary and the stack."""
    status, ram, events, writes, ega, out, left, seen = expected[:8]
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    g = info['g']; recs = [physical(g[0], (g[1] + o) & 0xFFFF) for o in (0x10, 0x16, 0x90, 0x91, 0x102, 0x103)]
    lo = physical(info['ss'], (cpu.sp - 0x60) & 0xFFFF)
    if any(lo - 1 <= a < lo + 0x70 or physical(info['ds'], 0x5D00) <= a < physical(info['ds'], 0x6900) for a in recs) or g[0] in (0x2000, 0x5000, 0x4000, 0x6000, 0x7000, 0x8000, 0x9000):
        return 0
    if any(((g[1] + o) & 0xFFFF) == 0xFFFF for o in (0x90, 0x102)):                          # a word straddling offset FFFFh: the statement assumes a wrapped second byte, the machine reads it contiguous; differential only
        return 0
    key = (info['kind'], info['minor'])
    v = vcache.setdefault(key, B54.original_value(b54[0], b54[1], key[0], key[1], A.W_CS, 0x7000))
    w, amount, acc = info['w'], info['amount'], info['acc']
    above = ((w + amount) & 0xFFFF) > v
    excess = (v - w) & 0xFFFF if above else amount
    word = lambda a: ram[a] | (ram[(a + 1) & (SIZE - 1)] << 8)
    check(word(recs[2]) == (acc + excess) & 0xFFFF, f'{name}: accumulator {word(recs[2]):04x} is not {(acc + excess) & 0xFFFF:04x}')
    check(word(recs[4]) == (w + excess) & 0xFFFF, f'{name}: record word {word(recs[4]):04x} is not {(w + excess) & 0xFFFF:04x}')
    c0 = info['ctr'][0] | (info['ctr'][1] << 16); want = (c0 + amount - excess) & 0xFFFFFFFF if above else c0
    got = word(physical(info['ds'], 0x6804)) | (word(physical(info['ds'], 0x6806)) << 16)
    check(got == want, f'{name}: counter {got:08x} is not {want:08x}')
    shown = ram[physical(info['ss'], (cpu.sp - 2 - 0x2A) & 0xFFFF)] == 39                       # StrLoad left the 39-byte message's length byte at [BP-2Ah]; the not-above branch never wrote it
    check(shown == above, f'{name}: the message temporary says shown={shown} but the branch above is {above}')
    check(out.sp == (cpu.sp + 6) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == WD.ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, lib, b54, quick=False, only=None):
    t0 = time.time(); stops = {}; seen_all = set(); stated = 0; vcache = {}
    cs = [c for c in cases(b54[0], b54[1], quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 40 == 0 or name.startswith(('g90-at-bp+0', 'g102-at-bp+0', 'g90-over')) and index % 9 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual, seen = run_pair(name, st, bodies, decoded, lib, budget)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen_all |= seen
        if actual[0] in ('ARGUMENT', 'CODE', 'CALLEE') or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == BUDGET:
            stated += statements(name, kw, st, expected, vcache, b54)
    w0203 = {k for k in seen_all if k[0] == 'w' and 0x203 <= k[1] < 0x294}
    check(only is not None or len(w0203) == 53, f'every +0203 instruction start was reached ({len(w0203)}/53)')
    return dict(cases=len(cs), end_states=stops, starts_0203=f'{len(w0203)}/53', starts_total=len(seen_all), independent_statements_checked=stated, declared_mulw_split_states=WD.SPLIT_ARRIVALS[0], cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('b-12-1234-eq', 'b-12-1234-p1', 'b-12-0000-m1', 'b-0c-ffff-far', 'ctr-ffff-0000-7000', 'ctr-0001-0000-7000', 'ctr-ffff-ffff-ffff', 'acc-ffff-7000', 'acc-8000-0000', 'mode1-above', 'g90-at-bp-01-above',
                'g90-at-bp+00-not', 'g102-at-bp-01-above', 'g102-at-bp+00-not', 'g90-at-bp+02-above', 'g90-at-bp+04-not', 'sp-0200-above', 'flags-0a87') + tuple(f'budget-above-{b}' for b in (3, 12, 13, 14, 20, 40, 60, 120, 140)) + \
               tuple(f'budget-not-{b}' for b in (11, 12, 13, 14, 20, 40, 60, 90, 120))
MUTANTS = (
    ('JE sense inverted', [("above = (c->flags & 0x0040u) == 0u;", "above = (c->flags & 0x0040u) != 0u;")]),
    ('the message is at 01DCh', [("c->di = 0x01DBu;", "c->di = 0x01DCu;")]),
    ('the accumulator is at +92h', [("uint16_t at = (uint16_t)(c->di + 0x90u);", "uint16_t at = (uint16_t)(c->di + 0x92u);")]),
    ('ADC without the carry', [("arith16(c, c->dx, rd16(ram, c->ds, 0x6806u), 0, (c->flags & 1u))", "arith16(c, c->dx, rd16(ram, c->ds, 0x6806u), 0, 0)")]),
    ('SBB without the borrow', [("arith16(c, c->dx, c->bx, 1, (c->flags & 1u))", "arith16(c, c->dx, c->bx, 1, 0)")]),
    ('the counter low word stored at 6806', [("wr16(ram, c->ds, 0x6804u, c->ax);", "wr16(ram, c->ds, 0x6806u, c->ax);")]),
    ('+003A gets AMOUNT instead of the excess', [("STEP(); push(ram, c, bp16(ram, c, -2)); c->ip = 0x024Cu;", "STEP(); push(ram, c, bp16(ram, c, 6)); c->ip = 0x024Cu;")]),
    ('RETF pops 4 argument bytes', [("STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 2u);", "STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u);")]),
    ('SUB SP,2Ch', [("arith16(c, c->sp, 0x002Au, 1, 0)", "arith16(c, c->sp, 0x002Cu, 1, 0)")]),
    ('+0054 gets the word at [BP+8]', [("STEP(); push(ram, c, bp16(ram, c, 6)); c->ip = 0x0214u;", "STEP(); push(ram, c, bp16(ram, c, 8)); c->ip = 0x0214u;")]),
    ('the excess local is at BP-4', [("STEP(); c->di = lea_bp(c, -2); c->ip = 0x0217u;", "STEP(); c->di = lea_bp(c, -4); c->ip = 0x0217u;")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_0203_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the call-site displacement of +0054 expects FE38h', [("rd16(ram, cs, 0x021Bu) != 0xFE37u", "rd16(ram, cs, 0x021Bu) != 0xFE38u")]),
    ('the +0054 call is not charged to the budget', [("--*budget; push(ram, c, 0x021Du); c->ip = 0x0054u; d->callee = 0;", "push(ram, c, 0x021Du); c->ip = 0x0054u; d->callee = 0;")]),
    ('the 1693 call is not charged to the budget', [("--*budget; d->callee = 2;", "d->callee = 2;")]),
    ('OR AL,AL tests 16 bits', [("logic(c, (uint8_t)c->ax, 8u); c->ip = 0x021Fu;", "logic(c, (uint8_t)c->ax, 16u); c->ip = 0x021Fu;")]),
    ('the accumulator RMW bypasses the bus (both branches share one helper)', [("bwr16(bus, c->es, at, arith16(c, brd16(bus, c->es, at), c->ax, 0, 0));", "wr16(bus->ram, c->es, at, arith16(c, rd16(bus->ram, c->es, at), c->ax, 0, 0)); (void)brd16; (void)bwr16;")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u7f_0203_cpu' else ROOT / f'src/recovered/{s}.c') for s in WD.SOURCES if (ROOT / f'src/recovered/{s}.c').exists()]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u7f_0203_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded, b54):
    out = []; src = (ROOT / 'src/recovered/overlay_u7f_0203_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u7f-0203-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u7f_0203_cpu': path})
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
    with tempfile.TemporaryDirectory(prefix='u7f-0203-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); b54 = B54.source()[:2]; report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, b54, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded, b54)
        if a.native:
            from ovl_007f_0203_native import native
            report['native'] = native(bodies, decoded, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_007f_0203_cpu_audit'] = sys.modules[__name__]
    main()
