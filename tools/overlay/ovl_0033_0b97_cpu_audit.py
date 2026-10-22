#!/usr/bin/env python3
"""OVL_0033_0B97 (unit 0x33 [0B97,0BDE)) against the original bytes: CPU14, all 1,048,576 RAM bytes, the stop and the remaining budget, unmasked after every case.

The body reads the far pointer G at DS:[6716] and stores 34h at G + Y*50 + X + 7 and 35h at G + Y*50 + X + 8 (X and Y are signed bytes), then increments the byte at SS:[P-2] for a near pointer
P taken from its frame. It is a leaf (no call). The three stores go through caller-supplied pointers, so the model sends them through the bus; the audit places each of them ON the body's own frame
(its saved BP, its return address, its own argument bytes, the pointer P itself): the second half of the body re-reads X and Y after the first store, and reads P after both. One declared exception,
shared with the +0000 audit: the SF/ZF/AF/PF bits a MUL leaves undefined are not compared at a LIMIT arrival on the instruction after a MUL. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_0033_0B97',)
ENTRY, END = 0x0B97, 0x0BDE
SHA = '75c464f3df7839fc6a44ab167138b0addb4a329758a775b3255c50a35108b52a'
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0010, 0xFFF6
SOURCES = ['overlay_u33_0b97_cpu', 'memory_bus', 'ega_hw']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'TRANSFER'}
ENTRY_RET = A.ENTRY_RET
check = A.check


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.c_void_p), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p), ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


def sext(v):
    return (v & 0xFF) - 0x100 if v & 0x80 else v & 0xFF


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x33)
    body = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 71 and sha256(body).hexdigest() == SHA and len(ins) == 29 and sum(i.size for i in ins) == 71, 'unit 0x33 +0B97 identity: 71 bytes / 29 instructions')
    check(not [i for i in ins if i.mnemonic.startswith('j') or i.mnemonic in ('call', 'lcall', 'int', 'in', 'out')], 'a leaf: straight line, no jump, call, interrupt or port instruction')
    check([i.address for i in ins if i.mnemonic == 'mul'] == [0x0BA7, 0x0BC4] and [(i.address, int(i.op_str, 0)) for i in ins if i.mnemonic == 'retf'] == [(0x0BDB, 6)], 'two MULs and the only return RETF 6 at 0BDB')
    check(not [f for f in unit['fixups'] if ENTRY <= f < END], 'no relocation inside the span')
    return {('w', ENTRY): body}, {('w', i.address): i for i in ins}, dict(bytes=71, instructions=29, sha256=SHA, file_offset=unit['ovr_offset'] + ENTRY, memory_segment=unit['memory_segment'])


def library(directory, override=None):
    path = Path(directory) / ('u33_0b97_mut.so' if override else 'u33_0b97.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u33_0b97_cpu.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]; lib.pr_u33_0b97_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state, budget):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget); bus = Bus(ctypes.addressof(ram), None, None, None, None)
    status = lib.pr_u33_0b97_cpu(ctypes.byref(bus), SIZE, ctypes.byref(out), ctypes.byref(left))
    return STATUS[status], bytes(ram), out, left.value, None


def fixture(bodies, *, x=5, y=3, g=(0x3000, 0x0100), p=None, counter=0x10, sp=0x8000, ds=0x1000, ss=0x2000, w_cs=W_CS, flags=0x0202, regs=None, background=0xA5, xhi=0xCD, yhi=0xCD):
    """Entry frame (SP+0): return IP, CS; P (the near pointer); X and Y (each a word slot whose high byte is junk). DS:[6716]/[6718] hold the far pointer G."""
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    p = (sp + 0x100) & 0xFFFF if p is None else p                 # the counter lives in the caller's frame, well above the entry frame
    m[physical(ss, (p - 2) & 0xFFFF)] = counter
    put16(m, ds, 0x6716, g[1]); put16(m, ds, 0x6718, g[0])
    for (kind, start), blob in bodies.items():
        at = physical(w_cs, start); m[at:at + len(blob)] = blob
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0], p, (xhi << 8) | (x & 0xFF), (yhi << 8) | (y & 0xFF))):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, resident_cs=0, sp=sp, ss=ss, ds=ds, x=x, y=y, g=g, p=p, counter=counter)


FLAG_WRITERS = ('add', 'sub', 'inc', 'dec', 'cmp', 'and', 'or', 'xor', 'adc', 'sbb', 'neg', 'test', 'mul', 'imul', 'shl', 'shr', 'sar', 'rol', 'ror')


def undefined_arrivals(decoded):
    """MUL leaves SF, ZF, AF and PF undefined until the next instruction that writes the flags. The arrivals on every instruction from the one after a MUL up to and including that next writer (an
    arrival is BEFORE the instruction runs) hold the stale value: Unicorn keeps its own, the model computes one. Those bits are the only ones not compared, and only at those arrivals."""
    by_ip = sorted(k[1] for k in decoded)
    arrivals = set()
    for ip in by_ip:
        if decoded[('w', ip)].mnemonic != 'mul':
            continue
        nxt = ip + decoded[('w', ip)].size
        while ('w', nxt) in decoded:
            arrivals.add(('w', nxt))
            if decoded[('w', nxt)].mnemonic in FLAG_WRITERS:
                break
            nxt += decoded[('w', nxt)].size
    return arrivals


def compare(label, state, bodies, decoded, lib, budget=100000):
    actual = run_c(lib, state, budget); expected = A.run_original(state, decoded, budget)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    check(actual[3] == expected[3], f'{label}: budget left C {actual[3]} original {expected[3]}')
    after_mul = undefined_arrivals(decoded)
    undefined = 0x00D4 if expected[0] == 'LIMIT' and ('w', expected[2].ip) in after_mul else 0
    for name, _ in Cpu._fields_:
        mask = ~undefined & 0xFFFF if name == 'flags' else 0xFFFF
        check(getattr(actual[2], name) & mask == getattr(expected[2], name) & mask, f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    for (kind, start), blob in bodies.items():
        at = physical(state[2]['w_cs'], start); check(expected[1][at:at + len(blob)] == state[0][at:at + len(blob)], f'{label}: code written')
    return expected, actual, bool(undefined)


# ---------------------------------------------------------------------------------------------------- cases
def cases(quick=False):
    rng = random.Random(0x330B97)
    out = []

    def add(name, budget=100000, **kw):
        out.append((name, kw, budget))
    for x in range(256):
        add(f'x-{x:02x}', x=x, y=3)
    for y in range(256):
        add(f'y-{y:02x}', x=5, y=y)
    for x, y in ((0x7F, 0x7F), (0x80, 0x80), (0xFF, 0xFF), (0x00, 0x00), (0x7F, 0x80), (0x80, 0x7F), (0xFF, 0x01), (0x01, 0xFF)):
        add(f'xy-{x:02x}-{y:02x}', x=x, y=y)
    for off in (0x0000, 0x0007, 0xFFF0, 0xFFF9, 0xFFFA, 0xFFFB, 0xFFFE, 0xFFFF, 0xFF00, 0x8000):
        add(f'g-off-{off:04x}', g=(0x3000, off), x=0x0A, y=0)
    for seg in (0xFFFF, 0xFFF0, 0x0000, 0xA000, 0x9FFF):
        add(f'g-seg-{seg:04x}', g=(seg, 0x0200), x=2, y=2)
    for counter in (0x00, 0x7F, 0x80, 0xFE, 0xFF, 0x0F):
        add(f'counter-{counter:02x}', counter=counter)
    for sp in (SP_MIN, SP_MIN + 1, 0x0101, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, p=(sp + 0x100) & 0xFFFF)
    add('ss-ds', ss=0x1000, sp=0xE000, p=0xE100); add('ss-grid', ss=0x3000, g=(0x3000, 0x0100), sp=0x8000)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, x=0x90, y=0xF0)
    for w in (0x6000, 0x2345, 0xF000):
        add(f'at-{w:04x}', w_cs=w)
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x10, 0x14):                                       # the counter byte at SS:[P-2] on each byte of the body's frame: saved BP, return address, P itself, X, Y, the caller's words above
        add(f'near-ptr-at-bp{k:+03x}', p=(BP + k + 2) & 0xFFFF, sp=S0)
    for k in range(-0x10, 0x14):                                       # the first store (34h) on each byte; the second store (35h) lands on the next byte
        add(f'grid-at-bp{k:+03x}', g=(0x2000, (BP + k - 7) & 0xFFFF), x=0, y=0, sp=S0)
    for k in (0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B):                     # the first store on P, X or Y themselves: the second half re-reads them
        for (x, y) in ((0, 0), (3, 2)):
            add(f'grid-args-bp{k:+03x}-{x}-{y}', g=(0x2000, (BP + k - 7 - (y * 50 + x)) & 0xFFFF), x=x, y=y, sp=S0)
    for k in (-1, 0, 1, 2, 4, 6, 8):
        add(f'both-bp{k:+03x}', g=(0x2000, (BP + k - 7) & 0xFFFF), x=0, y=0, p=(BP + k + 2) & 0xFFFF, sp=S0)
        add(f'both-bp{k:+03x}-next', g=(0x2000, (BP + k - 8) & 0xFFFF), x=0, y=0, p=(BP + k + 2) & 0xFFFF, sp=S0)
    add('grid-is-counter', g=(0x2000, (0x8000 + 0x2E - 7) & 0xFFFF), x=0, y=0)
    add('grid-over-globals', g=(0x1000, (0x6716 - 7) & 0xFFFF), x=0, y=0)               # the first store lands on G's own offset word
    add('grid-over-globals-seg', g=(0x1000, (0x6718 - 7) & 0xFFFF), x=0, y=0)
    for budget in range(0, 36):
        add(f'budget-{budget}', budget=budget, x=0x90, y=0xF0)
    for k in range(30 if quick else 120):
        add(f'random{k}', x=rng.randrange(256), y=rng.randrange(256), g=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)), p=rng.randrange(65536), counter=rng.randrange(256),
            regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1),
            xhi=rng.randrange(256), yhi=rng.randrange(256), w_cs=rng.choice((0x6000, 0x2345)))
    return out


def expected_transfer(name):
    return name.startswith(('near-ptr-at-bp', 'grid-at-bp', 'both-', 'grid-args'))


def statements(name, kw, st, expected):
    """Independent of the C model, for cases whose three targets do not overlap each other or the stack: the two grid cells and the counter, from the arguments alone."""
    status, ram, out, left, seen, frames = expected
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    x, y, g, p = info['x'], info['y'], info['g'], info['p']
    prod = ((sext(y) & 0xFFFF) * 0x32) & 0xFFFF
    a1 = physical(g[0], (g[1] + prod + sext(x) + 7) & 0xFFFF); a2 = physical(g[0], (g[1] + prod + sext(x) + 1 + 7) & 0xFFFF); a3 = physical(info['ss'], (p - 2) & 0xFFFF)
    lo = physical(info['ss'], (cpu.sp - 0x20) & 0xFFFF); hi = lo + 0x60
    if len({a1, a2, a3}) < 3 or any(lo - 1 <= a < hi for a in (a1, a2, a3)) or any(abs(a - physical(info['ds'], 0x6716)) < 4 for a in (a1, a2, a3)):
        return 0
    check(ram[a1] == 0x34 and ram[a2] == 0x35, f'{name}: grid cells {ram[a1]:02x} {ram[a2]:02x}')
    check(ram[a3] == (info['counter'] + 1) & 0xFF, f'{name}: counter {ram[a3]:02x}')
    check(out.sp == (cpu.sp + 10) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); stated = 0; masked_n = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 100 == 0 or name.startswith(('grid-at-bp+0', 'near-ptr-at-bp+0', 'both-', 'random')) and index % 9 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual, masked = compare(name, st, bodies, decoded, lib, budget)
        masked_n += int(masked); stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[4]
        if actual[0] == 'ARGUMENT' or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == 100000 and not name.startswith(('grid-over', 'ss-grid')):
            stated += statements(name, kw, st, expected)
    check(only is not None or len(seen) == 29, 'every instruction start was reached')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(decoded)}', independent_statements_checked=stated, mul_undefined_flag_arrivals=masked_n, cpu_words=14, ram_bytes=SIZE,
                flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('x-80', 'x-7f', 'y-80', 'y-02', 'xy-ff-01', 'g-off-fffb', 'counter-7f', 'sp-0010', 'flags-0a87', 'budget-9', 'budget-25', 'budget-34', 'near-ptr-at-bp+00', 'near-ptr-at-bp+02', 'near-ptr-at-bp+04', 'near-ptr-at-bp+06',
                'grid-at-bp+00', 'grid-at-bp+02', 'grid-at-bp+04', 'grid-args-bp+08-0-0', 'grid-args-bp+0a-3-2', 'both-bp+02', 'grid-over-globals')
MUTANTS = (
    ('the multiplier is 33h', [("c->dx = 0x0032u; AT(0x0BA7u);", "c->dx = 0x0033u; AT(0x0BA7u);")]),
    ('the first store is at +6', [("c->di + 7u)), 0x34u)", "c->di + 6u)), 0x34u)")]),
    ('the values are swapped', [("0x34u); AT(0x0BB6u);", "0x35u); AT(0x0BB6u);"), ("0x35u); AT(0x0BD3u);", "0x34u); AT(0x0BD3u);")]),
    ('X is zero-extended, not sign-extended', [("STEP(); cbw(c); AT(0x0B9Eu);", "STEP(); c->ax = (uint16_t)(uint8_t)c->ax; AT(0x0B9Eu);")]),
    ('Y is zero-extended, not sign-extended', [("STEP(); cbw(c); AT(0x0BA4u);", "STEP(); c->ax = (uint16_t)(uint8_t)c->ax; AT(0x0BA4u);")]),
    ('the second cell is not advanced (INC AX dropped)', [("STEP(); c->ax = inc16(c, c->ax); AT(0x0BBBu);", "STEP(); (void)inc16; AT(0x0BBBu);")]),
    ('the counter is decremented', [("pr_bus_write8(bus, at, inc8(c, v));", "pr_bus_write8(bus, at, (uint8_t)(inc8(c, v) - 2u));")]),
    ('the counter is at P-1', [("physical(c->ss, (uint16_t)(c->di - 2u))", "physical(c->ss, (uint16_t)(c->di - 1u))")]),
    ('INC BYTE also sets CF', [("c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);\n    return r;\n}\nstatic void mul_dx", "c->flags = (uint16_t)((c->flags & ~0x08D5u) | f | 1u);\n    return r;\n}\nstatic void mul_dx")]),
    ('RETF pops 4 argument bytes', [("c->sp = (uint16_t)(c->sp + 6u);", "c->sp = (uint16_t)(c->sp + 4u);")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U33_0B97_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the globals are read at 6718/671A', [("rd16(ram, c->ds, 0x6716u); c->es = rd16(ram, c->ds, 0x6718u); AT(0x0BADu);", "rd16(ram, c->ds, 0x6718u); c->es = rd16(ram, c->ds, 0x671Au); AT(0x0BADu);")]),
    ('the MULs are not charged to the budget', [("STEP(); mul_dx(c); AT(0x0BA9u);", "mul_dx(c); AT(0x0BA9u);")]),
    ('MUL leaves CF and OF clear', [("c->flags = (uint16_t)((c->flags & ~0x0801u) | (c->dx ? 0x0801u : 0u));", "c->flags = (uint16_t)(c->flags & ~0x0801u);")]),
    ('the first grid store bypasses the bus', [("pr_bus_write8(bus, physical(c->es, (uint16_t)(c->di + 7u)), 0x34u);", "ram[physical(c->es, (uint16_t)(c->di + 7u))] = 0x34u;")]),
    ('the second grid store bypasses the bus', [("pr_bus_write8(bus, physical(c->es, (uint16_t)(c->di + 7u)), 0x35u);", "ram[physical(c->es, (uint16_t)(c->di + 7u))] = 0x35u;")]),
    ('the counter increment bypasses the bus', [("{ size_t at = physical(c->ss, (uint16_t)(c->di - 2u)); uint8_t v = pr_bus_read8(bus, at); pr_bus_write8(bus, at, inc8(c, v)); }",
                                                "{ size_t at = physical(c->ss, (uint16_t)(c->di - 2u)); ram[at] = inc8(c, ram[at]); }")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u33_0b97_cpu' else ROOT / f'src/recovered/{s}.c') for s in SOURCES]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u33_0b97_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded):
    out = []; src = (ROOT / 'src/recovered/overlay_u33_0b97_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u33-0b97-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u33_0b97_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(1 for _, r in out if r.startswith('CAUGHT:')); c = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out, survivors=[n for n, r in out if r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')] +
                [n for n, r in out if r == 'COMPILE-ERROR'], cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u33-0b97-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, a.quick)
        if a.native:
            from ovl_0033_0b97_native import native
            report['native'] = native(bodies, decoded, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_0033_0b97_cpu_audit'] = sys.modules[__name__]
    main()
