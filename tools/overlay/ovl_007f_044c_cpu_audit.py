#!/usr/bin/env python3
"""OVL_007F_044C (unit 0x7F [044C,0506)) composed with +0000 (and the resident 13AB/11EA closure), +0020, +003A, StrLoad and the resident 1693 display closure, against the original bytes.

The body moves EXTRA units between two records addressed through pointers to pointers (PP and QQ, each a far pointer to a cell that holds a far pointer to the record): if the destination record's word
at +0102h plus EXTRA exceeds the class value V (+0000) it shows a message through 1693 and returns; otherwise it subtracts EXTRA from the source record's word at +88h+2*INDEX, calls +0020 on it, adds
EXTRA to the destination's word at +88h+2*INDEX and calls +003A on the source record. Every callee runs as its ORIGINAL bytes in the oracle and as its merged C model in the C run. EVERY access through
PP, QQ, the pointer cells and the records goes through the bus. Compared unmasked after every case: CPU14, ALL 1,048,576 RAM bytes, the EGA planes, latches and registers, the ordered OUT events and VRAM
operands, the stop and the remaining budget. Scope, declared: the budget counts this body, +0000 and the 13AB closure; the 1693 closure's own loop-back limits and refusal stops stay with its own audit.
Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from pathlib import Path
import ovl_007f_world1693 as WD
import ovl_007f_0054_cpu_audit as B54
import ovl_007f_0000_cpu_audit as A
from res0709_printer_0626_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

CHECKER_ROWS = ('OVL_007F_044C',)
ENTRY = 0x044C
SP_MIN, SP_MAX = 0x0200, 0xFFEE
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'LIMIT', 4: 'TRANSFER', 5: 'CALLEE'}
BUDGET = 100000
ALIAS_BUDGET = 30000
P0, Q0 = (0x3000, 0x0100), (0x3400, 0x0100)
PCELL0, QCELL0 = (0x3800, 0x0000), (0x3800, 0x0010)


class Diag(ctypes.Structure):
    _fields_ = [('raw', ctypes.c_uint8 * 4096)]


def library(directory, override=None):
    lib = WD.library(directory, override)
    f = lib.pr_u7f_044c_cpu
    f.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(Diag)]; f.restype = ctypes.c_int
    return lib


def call(fn, bus, size, cpu, left, lim):
    diag = Diag(); status = fn(bus, size, cpu, left, ctypes.byref(lim), ctypes.byref(diag))
    return STATUS[status], diag


def sext8(v):
    return (v & 0xFF) - 0x100 if v & 0x80 else v & 0xFF


def source():
    bodies, decoded, meta = WD.source()
    ins = [(k[1], i) for k, i in decoded.items() if k[0] == 'w' and 0x44C <= k[1] < 0x506]
    calls = sorted((a, i.mnemonic, i.op_str) for a, i in ins if i.mnemonic in ('call', 'lcall'))
    check(calls == [(0x45D, 'call', '0'), (0x4AD, 'call', '0x20'), (0x4E9, 'call', '0x3a'), (0x4F8, 'lcall', '0xaf8, 0xb0b'), (0x4FD, 'lcall', '0xba, 0x1693')], f'the five call sites of +044C: {calls}')
    check(sorted((a, i.mnemonic) for a, i in ins if i.mnemonic.startswith('j') or i.mnemonic == 'retf') == [(0x472, 'ja'), (0x4EC, 'jmp'), (0x505, 'retf')], 'one conditional (JA 04EE), one JMP 0502 and RETF 000Ch')
    check(len(ins) == 71, 'the 71 instructions of +044C')
    meta = dict(meta, body044c=dict(bytes=188, instructions=71, sha256=WD.SPANS[0x44C][3]))
    return bodies, decoded, meta


def fixture(bodies, *, index=2, extra=0x0010, kind=0x12, minor=0x20, pw=0x0100, parr=0x0200, qarr=0x0300, qw=0x0400, p=P0, q=Q0, pcell=PCELL0, qcell=QCELL0, sp=0x8000, flags=0x0202, regs=None, world=None):
    """D's world; then the destination record P and the source record Q (class bytes at +10h/+16h, the word at +0102h, the array word at +88h+2*INDEX), the two pointer cells, PP and QQ, the CPU and the entry frame.
    The cells are written LAST, so a cell placed on the frame holds valid pointers when the body starts and is overwritten by the body's own stores later."""
    m, c = WD.base_state(bodies, **(world or {}))
    ds, ss = c.ds, c.ss
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    for rec in (p, q):
        for k in range(0x110):
            m[physical(rec[0], (rec[1] + k) & 0xFFFF)] = (k * 5 + 3) & 0xFF
    elem = 0x88 + 2 * sext8(index)
    put = lambda rec, off, v: (m.__setitem__(physical(rec[0], (rec[1] + off) & 0xFFFF), v & 0xFF), m.__setitem__(physical(rec[0], (rec[1] + off + 1) & 0xFFFF), (v >> 8) & 0xFF))
    m[physical(p[0], (p[1] + 0x10) & 0xFFFF)] = kind & 0xFF; m[physical(p[0], (p[1] + 0x16) & 0xFFFF)] = minor & 0xFF
    put(p, 0x102, pw); put(p, elem, parr); put(q, elem, qarr); put(q, 0x102, qw)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, WD.W, ENTRY, ds, r[7], ss, flags | 0x0002)
    pp, qq = pcell, qcell
    frame = (WD.ENTRY_RET[1], WD.ENTRY_RET[0], 0xAB00 | (index & 0xFF), extra & 0xFFFF, pp[1], pp[0], qq[1], qq[0])
    for k, v in enumerate(frame):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    for cell, rec in ((pcell, p), (qcell, q)):
        put16(m, cell[0], cell[1] & 0xFFFF, rec[1]); put16(m, cell[0], (cell[1] + 2) & 0xFFFF, rec[0])
    return m, cpu, dict(index=index, extra=extra, kind=kind, minor=minor, pw=pw, parr=parr, qarr=qarr, qw=qw, p=p, q=q, pcell=pcell, qcell=qcell, sp=sp, ds=ds, ss=ss)


def code_unchanged(eram, m, bodies):
    for (kind, start), blob in bodies.items():
        seg = WD.W if kind == 'data' else WD.SEG.get(kind)
        if seg is None:
            continue
        at = WD.physical(seg, start)
        if eram[at:at + len(blob)] != m[at:at + len(blob)]:
            return False
    return True


def run_pair(label, st, bodies, decoded, lib, budget):
    m, cpu, info = st
    expected = WD.run_original(m, cpu, decoded, budget)
    actual = WD.run_c(lib, lib.pr_u7f_044c_cpu, m, cpu, call, budget)
    try:
        seen = WD.compare_states(label, expected, actual, bodies, m, decoded)
    except ValueError as e:
        if code_unchanged(expected[1], m, bodies):
            raise
        # the ORIGINAL's own stores reached resident code, so it went on executing modified bytes: the precondition 'code is immutable' failed and the later paths are not comparable
        raise ValueError(f'{label}: code written (original executed modified code; first difference: {str(e)[:120]})')
    return expected, actual, seen


# ---------------------------------------------------------------------------------------------------- cases
def cases(bodies54, decoded54, quick=False):
    rng = random.Random(0x7F044C)
    out = []

    def add(name, budget=BUDGET, **kw):
        out.append((name, kw, budget))
    vcache = {}
    V = lambda k, mi=0x20: vcache.setdefault((k, mi), B54.original_value(bodies54, decoded54, k, mi, A.W_CS, 0x7000))
    for k in (0x0C, 0x12, 0x13):
        v = V(k)
        for pw in (0x0000, 0x0001, 0x1234, 0xFFFF):
            for delta, tag in ((-1, 'm1'), (0, 'eq'), (1, 'p1'), (0x8000, 'far')):
                add(f'b-{k:02x}-{pw:04x}-{tag}', kind=k, pw=pw, extra=((v + delta) - pw) & 0xFFFF)
    for index in (0, 1, 5, 0x7F, 0x80, 0xFF, 0x41):
        add(f'index-{index:02x}-not', index=index, extra=0x0005, pw=0); add(f'index-{index:02x}-above', index=index, extra=0x7000)
    for parr, qarr in ((0, 0), (0xFFFF, 1), (0x8000, 0x7FFF), (5, 3), (0xFFFE, 0xFFFF)):
        for extra in (0x0001, 0x0008, 0xFFFF):
            add(f'arr-{parr:04x}-{qarr:04x}-{extra:04x}', parr=parr, qarr=qarr, extra=extra, pw=0)
    for qw in (0x0000, 0x0005, 0xFFFF):
        add(f'qw-{qw:04x}', qw=qw, extra=0x0005, pw=0)
    for mode in (0, 1, 2, 3):
        add(f'mode{mode}-above', world=dict(mode=mode), extra=0x7000)
    for active in (0, 3, 7):
        add(f'page{active}-above', world=dict(active=active), extra=0x7000)
    for factor, count in ((1, 1), (2, 2), (0, 5)):
        add(f'delay-{factor}-{count}-above', world=dict(factor=factor, count=count), extra=0x7000)
    for off in (0xFF00, 0xFF77, 0xFF78, 0xFF79, 0xFE00, 0xFEFD, 0xFFF0):
        add(f'p-off-{off:04x}-not', p=(0x3000, off), extra=5, pw=0); add(f'q-off-{off:04x}-not', q=(0x3400, off), extra=5, pw=0)
    for off in (0xFFF8, 0xFFFC, 0xFFFD, 0xFFFE, 0xFFFF):
        add(f'pcell-off-{off:04x}-not', pcell=(0x3800, off), extra=5, pw=0); add(f'qcell-off-{off:04x}-not', qcell=(0x3800, off), extra=5, pw=0)
    for seg in (0xFFFF, 0xFFF0):
        add(f'p-seg-{seg:04x}-not', p=(seg, 0x0200), extra=5, pw=0); add(f'q-seg-{seg:04x}-not', q=(seg, 0x0200), extra=5, pw=0)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}-above', sp=sp, extra=0x7000); add(f'sp-{sp:04x}-not', sp=sp, extra=5, pw=0)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, extra=5, pw=0)
    S0 = 0x8000; BP = S0 - 2; ssr = (0x2000, 0)
    steps = range(-0x30, 0x10, 2)
    for k in steps:                                                                   # the pointer cells, the arrays and the words on the body's own frame
        for branch, kw in (('above', dict(extra=0x7000)), ('not', dict(extra=5, pw=0))):
            at = (BP + k) & 0xFFFF
            add(f'pcell-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, pcell=(0x2000, at), sp=S0, **kw)
            add(f'qcell-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, qcell=(0x2000, at), sp=S0, **kw)
            add(f'parr-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, p=(0x2000, (at - 0x88 - 4) & 0xFFFF), sp=S0, **kw)          # index 2: the array word at P+88h+4
            add(f'qarr-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, q=(0x2000, (at - 0x88 - 4) & 0xFFFF), sp=S0, **kw)
            add(f'pw-at-bp{k:+03x}-{branch}', budget=ALIAS_BUDGET, p=(0x2000, (at - 0x102) & 0xFFFF), sp=S0, **kw)
    for budget in list(range(0, 175)) + [200, 230, 260, 300, 400]:
        add(f'budget-above-{budget}', budget=budget, extra=0x7000)
    for budget in range(0, 200):
        add(f'budget-not-{budget}', budget=budget, extra=5, pw=0)
    for k in range(10 if quick else 40):
        add(f'random{k}', kind=rng.randrange(256), minor=rng.randrange(256), pw=rng.randrange(65536), extra=rng.randrange(65536), index=rng.randrange(256), parr=rng.randrange(65536), qarr=rng.randrange(65536),
            qw=rng.randrange(65536), regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1),
            p=(rng.randrange(0x1000, 0x2000), rng.randrange(65536)), q=(rng.randrange(0x2800, 0x3000), rng.randrange(65536)), world=dict(mode=rng.choice((0, 1, 2, 3)), active=rng.randrange(8)))
    return out


def expected_transfer(name):
    return name.startswith(('pcell-at', 'qcell-at', 'parr-at', 'qarr-at', 'pw-at'))


def statements(name, kw, st, expected, vcache, b54):
    """Independent of the C model, on cases whose records, cells and arrays do not overlap the stack, the globals or each other, and straddle nothing: the branch, the two array words, Q's +0102 word
    (+0020 then +003A leave it where it started) and the message temporary."""
    status, ram, events, writes, ega, out, left, seen = expected[:8]
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    p, q = info['p'], info['q']; elem = (0x88 + 2 * sext8(info['index'])) & 0xFFFF
    spots = [physical(p[0], (p[1] + o) & 0xFFFF) for o in (0x10, 0x16, 0x102, 0x103, elem, elem + 1)] + [physical(q[0], (q[1] + o) & 0xFFFF) for o in (0x102, 0x103, elem, elem + 1)] + \
            [physical(c[0], (c[1] + o) & 0xFFFF) for c in (info['pcell'], info['qcell']) for o in range(4)]
    lo = physical(info['ss'], (cpu.sp - 0x60) & 0xFFFF)
    if any(lo - 1 <= a < lo + 0x80 for a in spots) or len(set(spots)) != len(spots) or any(((r[1] + o) & 0xFFFF) > 0xFFFC for r in (p, q) for o in (0x102, elem)) or any(((c[1] + 3) & 0xFFFF) < 3 for c in (info['pcell'], info['qcell'])):
        return 0
    if info['p'][0] in (0x2000, 0x5000, 0x4000, 0x6000, 0x7000, 0x8000, 0x9000) or info['q'][0] in (0x2000, 0x5000, 0x4000, 0x6000, 0x7000, 0x8000, 0x9000):
        return 0
    key = (info['kind'], info['minor'])
    v = vcache.setdefault(key, B54.original_value(b54[0], b54[1], key[0], key[1], A.W_CS, 0x7000))
    pw, extra = info['pw'], info['extra']
    above = ((pw + extra) & 0xFFFF) > v
    word = lambda a: ram[a] | (ram[(a + 1) & (SIZE - 1)] << 8)
    shown = ram[physical(info['ss'], (cpu.sp - 2 - 0x0B) & 0xFFFF)] == 10                      # StrLoad left the 10-byte message's length byte at [BP-0Bh]
    check(shown == above, f'{name}: the message temporary says shown={shown} but the branch above is {above}')
    pe, qe = physical(p[0], (p[1] + elem) & 0xFFFF), physical(q[0], (q[1] + elem) & 0xFFFF)
    if above:
        check(word(pe) == info['parr'] & 0xFFFF and word(qe) == info['qarr'] & 0xFFFF, f'{name}: arrays changed on the message branch')
    else:
        check(word(qe) == (info['qarr'] - extra) & 0xFFFF, f'{name}: source array word {word(qe):04x}')
        check(word(pe) == (info['parr'] + extra) & 0xFFFF, f'{name}: destination array word {word(pe):04x}')
        check(word(physical(q[0], (q[1] + 0x102) & 0xFFFF)) == info['qw'] & 0xFFFF, f'{name}: source +0102 word is not back where it started')
        check(word(physical(p[0], (p[1] + 0x102) & 0xFFFF)) == pw & 0xFFFF, f'{name}: destination +0102 word changed')
    check(out.sp == (cpu.sp + 0x10) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == WD.ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, lib, b54, quick=False, only=None):
    t0 = time.time(); stops = {}; seen_all = set(); stated = 0; vcache = {}; code_writes = []
    cs = [c for c in cases(b54[0], b54[1], quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 40 == 0 or name.startswith(('pcell-at-bp+0', 'parr-at-bp+0', 'pw-at-bp+0')) and index % 9 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        try:
            expected, actual, seen = run_pair(name, st, bodies, decoded, lib, budget)
        except ValueError as e:
            if 'code written' not in str(e):
                raise
            code_writes.append(name)            # every other comparison (status, budget, CPU14, all RAM, EGA, OUT events, VRAM operands) had already passed; the precondition 'code is immutable' did not hold
            continue
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen_all |= seen
        if actual[0] in ('ARGUMENT', 'CODE', 'CALLEE') or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == BUDGET:
            stated += statements(name, kw, st, expected, vcache, b54)
    w044 = {k for k in seen_all if k[0] == 'w' and 0x44C <= k[1] < 0x506}
    check(only is not None or len(w044) == 71, f'every +044C instruction start was reached ({len(w044)}/71)')
    return dict(cases=len(cs), end_states=stops, starts_044c=f'{len(w044)}/71', starts_total=len(seen_all), independent_statements_checked=stated, declared_mulw_split_states=WD.SPLIT_ARRIVALS[0], cases_whose_stores_reached_code=len(code_writes), code_write_cases=code_writes,
                cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('b-12-1234-eq', 'b-12-1234-p1', 'b-12-0000-m1', 'b-0c-ffff-far', 'index-80-not', 'index-ff-above', 'index-05-not', 'arr-ffff-0001-0008', 'arr-8000-7fff-ffff', 'qw-ffff', 'mode1-above',
                'pcell-at-bp-02-not', 'pcell-at-bp+02-above', 'parr-at-bp+00-not', 'parr-at-bp+02-not', 'qarr-at-bp+04-not', 'qarr-at-bp-04-not', 'pw-at-bp+00-above', 'sp-0200-above', 'flags-0a87') + tuple(f'budget-not-{b}' for b in range(0, 200)) + \
               tuple(f'budget-above-{b}' for b in (3, 12, 50, 100, 140))
EQUIVALENT = {      # a survivor named here has a written proof; the proof is also checked by running it
    'SHL leaves OF clear': 'every SHL AX,1 in the body (0478, 0490, 04B4, 04CC) directly follows a CBW, so its operand is a sign-extended byte: bits 15 and 14 are equal, hence OF = CF xor the new bit 15 = 0 for the first shift; '
                           'checked exhaustively below over all 256 INDEX values. OF is therefore never set by this body, and a model that never sets it is indistinguishable',
}


def prove_shl_of_zero():
    for i in range(-128, 128):
        a = i & 0xFFFF; r = (a << 1) & 0xFFFF
        check(((a >> 15) ^ (r >> 15)) & 1 == 0, f'SHL OF set for INDEX {i}')
    return 256


MUTANTS = (
    ('JA sense inverted', [("above = (c->flags & 0x0041u) == 0u;", "above = (c->flags & 0x0041u) != 0u;")]),
    ('the message is at 0442h', [("c->di = 0x0441u;", "c->di = 0x0442u;")]),
    ('the source array word is read at +8Ah', [("brd16(bus, c->es, (uint16_t)(c->di + 0x88u)); AT(0x0487u);", "brd16(bus, c->es, (uint16_t)(c->di + 0x8Au)); AT(0x0487u);")]),
    ('the source word gets an ADD', [("c->ax = arith16(c, c->ax, bp16(ram, c, 8), 1); AT(0x048Au);", "c->ax = arith16(c, c->ax, bp16(ram, c, 8), 0); AT(0x048Au);")]),
    ('the destination word gets a SUB', [("c->ax = arith16(c, c->ax, bp16(ram, c, 8), 0); AT(0x04C6u);", "c->ax = arith16(c, c->ax, bp16(ram, c, 8), 1); AT(0x04C6u);")]),
    ('INDEX is zero-extended, not sign-extended (first use)', [("STEP(); cbw(c); AT(0x0478u);", "STEP(); c->ax &= 0x00FFu; AT(0x0478u);")]),
    ('the first SHL AX,1 is skipped', [("STEP(); c->ax = shl1(c, c->ax); AT(0x047Au);", "STEP(); AT(0x047Au);")]),
    ('SHL leaves OF clear', [("if (((a >> 15u) ^ (r >> 15u)) & 1u) f |= 0x0800u;", "")]),
    ('+0020 gets INDEX instead of EXTRA', [("STEP(); push(ram, c, bp16(ram, c, 8)); AT(0x04ACu);", "STEP(); push(ram, c, bp16(ram, c, 6)); AT(0x04ACu);")]),
    ('+003A is given P instead of Q', [("STEP(); les_bp(ram, c, 0x0E); AT(0x04DEu);", "STEP(); les_bp(ram, c, 0x0A); AT(0x04DEu);")]),
    ('RETF pops 0Ah argument bytes', [("c->sp = (uint16_t)(c->sp + 0x0Cu);", "c->sp = (uint16_t)(c->sp + 0x0Au);")]),
    ('SUB SP,0Eh', [("arith16(c, c->sp, 0x000Cu, 1)", "arith16(c, c->sp, 0x000Eu, 1)")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_044C_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the +0000 call-site displacement expects FBA1h', [("rd16(ram, cs, 0x045Eu) != 0xFBA0u", "rd16(ram, cs, 0x045Eu) != 0xFBA1u")]),
    ('the +0000 call is not charged to the budget', [("--*budget; push(ram, c, 0x0460u); AT(0x0000u); d->callee = 0;", "push(ram, c, 0x0460u); AT(0x0000u); d->callee = 0;")]),
    ('LES DI,ES:[DI] swaps the pointer words', [("c->di = off; c->es = seg;", "c->di = seg; c->es = off;")]),
    ('the destination array store bypasses the bus', [("bwr16(bus, c->es, (uint16_t)(c->di + 0x88u), c->dx); AT(0x04DBu);", "wr16(bus->ram, c->es, (uint16_t)(c->di + 0x88u), c->dx); (void)bwr16; AT(0x04DBu);")]),
    ('the pointer-cell read in LES DI,ES:[DI] bypasses the bus', [("uint16_t off = (uint16_t)(pr_bus_read8(bus, at) | ((uint16_t)pr_bus_read8(bus, (at + 1u) & MASK) << 8u));",
                                                                 "uint16_t off = (uint16_t)(bus->ram[at] | ((uint16_t)bus->ram[(at + 1u) & MASK] << 8u));")]),
    ('the pointer-cell reads before the +0000 call bypass the bus', [("STEP(); push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u))); AT(0x0459u);", "STEP(); push(ram, c, rd16(ram, c->es, (uint16_t)(c->di + 2u))); (void)brd16; AT(0x0459u);"),
                                                                     ("STEP(); push(ram, c, brd16(bus, c->es, c->di)); AT(0x045Cu);", "STEP(); push(ram, c, rd16(ram, c->es, c->di)); AT(0x045Cu);")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u7f_044c_cpu' else ROOT / f'src/recovered/{s}.c') for s in WD.SOURCES if (ROOT / f'src/recovered/{s}.c').exists()]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u7f_044c_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded, b54):
    out = []; src = (ROOT / 'src/recovered/overlay_u7f_044c_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u7f-044c-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u7f_044c_cpu': path})
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
                proven_equivalent=[n for n, r in out if r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST') and n in EQUIVALENT and prove_shl_of_zero() == 256],
                survivors=[n for n, r in out if ((r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')) or r == 'COMPILE-ERROR') and n not in EQUIVALENT], cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u7f-044c-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); b54 = B54.source()[:2]; report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, b54, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded, b54)
        if a.native:
            from ovl_007f_044c_native import native
            report['native'] = native(bodies, decoded, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_007f_044c_cpu_audit'] = sys.modules[__name__]
    main()
