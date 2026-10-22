#!/usr/bin/env python3
"""OVL_007F_0054 (unit 0x7F [0054,00A8)) composed with +0000 and the resident 00BA +13AB/+11EA closure, original CPU14/RAM oracle.

The body calls +0000 (near, after PUSH CS) for a record's class value V, reads the word W at record+0102h, and if W + extra (16-bit) is above V stores V - W through an out pointer and returns 1,
otherwise stores 0 and returns 0. Every callee runs as its ORIGINAL bytes in the oracle and as the merged C models in the C run. The body's own accesses through the record and out pointers go
through the bus in the model. Compared unmasked after every case: CPU14, ALL 1,048,576 RAM bytes, the stop and the remaining budget (one declared exception, shared with the +0000 audit: the SF/ZF/AF/PF
bits a MUL leaves undefined, at a LIMIT arrival on the instruction after a MUL). The out pointer and the record's +0102h word are placed ON the body's own frame (its local flag byte, its saved BP, its return
address and its argument words): the original's own store can clobber what it returns, restores or returns to. Independent statements pin the call frames, the stored word and the returned byte. Media stay under ignored resources/."""
import argparse, ctypes, json, random, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16
import subprocess

CHECKER_ROWS = ('OVL_007F_0054',)
S54_SHA = '9c986cb7eb942282b4b047ecc820a256b85f5f96ec9e7379ec9d56ce79811e3e'
SOURCES = A.SOURCES + ['overlay_u7f_0054_cpu', 'memory_bus', 'ega_hw']
SP_MIN, SP_MAX = 0x0050, 0xFFF2
ENTRY_RET = A.ENTRY_RET
STATUS = A.STATUS
check = A.check
ONLY = []


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.c_void_p), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p), ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


class Diag54(ctypes.Structure):
    _fields_ = [('callee_status', ctypes.c_int), ('site', ctypes.c_uint16)]


def source():
    bodies, decoded, meta = A.source()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x7F)
    body = ovr[unit['ovr_offset'] + 0x54:unit['ovr_offset'] + 0xA8]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x54))
    check(len(body) == 84 and sha256(body).hexdigest() == S54_SHA and len(ins) == 31 and sum(i.size for i in ins) == 84, 'unit 0x7F +0054 identity: 84 bytes / 31 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'call'] == [(0x61, '0'), (0x80, '0')] and not [i for i in ins if i.mnemonic in ('lcall', 'int', 'in', 'out')],
          'two near calls to +0000 at 0061 and 0080, no far call, interrupt or port')
    check([(i.address, i.mnemonic, int(i.op_str, 16)) for i in ins if i.mnemonic.startswith('j')] == [(0x73, 'jbe', 0x93), (0x91, 'jmp', 0x9F)], 'one conditional (JBE 0093 at 0073) and one JMP 009F')
    check([(i.address, int(i.op_str, 0)) for i in ins if i.mnemonic == 'retf'] == [(0xA5, 10)], 'the only return is RETF 000A at 00A5')
    check(not [f for f in unit['fixups'] if 0x54 <= f < 0xA8], 'no relocation inside the span')
    bodies = dict(bodies); bodies[('w', 0x54)] = body; decoded = dict(decoded); decoded.update({('w', i.address): i for i in ins})
    meta = dict(meta); meta['body54'] = dict(bytes=84, instructions=31, sha256=S54_SHA, file_offset=unit['ovr_offset'] + 0x54)
    return bodies, decoded, meta


def library(directory, override=None):
    path = Path(directory) / ('u7f_0054_mut.so' if override else 'u7f_0054.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u7f_0054_cpu.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t), ctypes.POINTER(Diag54)]
    lib.pr_u7f_0054_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state, budget):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget); diag = Diag54()
    bus = Bus(ctypes.addressof(ram), None, None, None, None)
    status = lib.pr_u7f_0054_cpu(ctypes.byref(bus), SIZE, ctypes.byref(out), ctypes.byref(left), ctypes.byref(diag))
    return STATUS[status], bytes(ram), out, left.value, diag


# ---------------------------------------------------------------------------------------------------- state
def fixture(bodies, *, rec=(0x3000, 0x0100), kind=7, minor=0, w=0x1234, extra=0x0010, out=(0x4000, 0x0200), sp=0x8000, rec_extra=None, **kw):
    """Entry frame (SP+0): return IP, CS; out offset, out segment; extra; record offset, record segment. The record's +10h/+16h select the class value; +0102h holds W."""
    rb = {0x10: kind & 0xFF, 0x16: minor & 0xFF, 0x102: w & 0xFF, 0x103: (w >> 8) & 0xFF}
    rb.update(rec_extra or {})
    args = (ENTRY_RET[1], ENTRY_RET[0], out[1] & 0xFFFF, out[0], extra & 0xFFFF, rec[1] & 0xFFFF, rec[0])
    return A.fixture(bodies, rec=rec, rec_bytes=rb, sp=sp, call_entry=0x0054, args=args, **kw)


def original_value(bodies, decoded, kind, minor, w_cs, resident_cs):
    """V = +0000 on the same record, from the ORIGINAL bytes alone (the independent oracle for the boundary cases)."""
    rec = (0x3000, 0x0100)
    st = A.fixture(bodies, rec=rec, kind=kind, minor=minor, w_cs=w_cs, resident_cs=resident_cs)
    res = A.run_original(st, {k: v for k, v in decoded.items() if not (k[0] == 'w' and k[1] >= 0x54)}, 100000)
    check(res[0] == 'RETURNED', '+0000 alone returned')
    return res[2].ax


def compare(label, state, bodies, decoded, lib, budget=100000):
    actual = run_c(lib, state, budget); expected = A.run_original(state, decoded, budget, frame_key=('w', 0))
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    check(actual[3] == expected[3], f'{label}: budget left C {actual[3]} original {expected[3]}')
    after_mul = {('res', a.address + a.size) for k, a in decoded.items() if k[0] == 'res' and a.mnemonic == 'mul'}
    undefined = 0x00D4 if expected[0] == 'LIMIT' and ('res', expected[2].ip) in after_mul and expected[2].cs == state[2]['resident_cs'] else 0
    for name, _ in Cpu._fields_:
        mask = ~undefined & 0xFFFF if name == 'flags' else 0xFFFF
        check(getattr(actual[2], name) & mask == getattr(expected[2], name) & mask, f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    for (kind, start), blob in bodies.items():
        seg = state[2]['w_cs'] if kind == 'w' else state[2]['resident_cs']
        at = physical(seg, start); check(expected[1][at:at + len(blob)] == state[0][at:at + len(blob)], f'{label}: code written')
    return expected, actual, bool(undefined)


# ---------------------------------------------------------------------------------------------------- cases
def cases(bodies, decoded, quick=False):
    rng = random.Random(0x7F0054)
    out = []

    def add(name, budget=100000, **kw):
        out.append((name, kw, budget))
    vcache = {}
    V = lambda k, m, wcs=A.W_CS, rcs=0x7000: vcache.setdefault((k, m, wcs, rcs), original_value(bodies, decoded, k, m, wcs, rcs))
    kinds = (0x00, 0x01, 0x03, 0x04, 0x06, 0x0C, 0x0E, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x20, 0x80, 0xFF)
    for k in kinds:
        v = V(k, 0x20)
        for w in (0x0000, 0x0001, 0x1234, 0xFFFF):
            for delta, tag in ((-1, 'm1'), (0, 'eq'), (1, 'p1'), (0x8000, 'far')):
                add(f'b-{k:02x}-{w:04x}-{tag}', kind=k, minor=0x20, w=w, extra=((v + delta) - w) & 0xFFFF)
    for minor in (0x00, 0x01, 0x32, 0x33, 0x4B, 0x4C, 0x5A, 0x5B, 0x63, 0x64, 0xFF):
        v = V(0x12, minor)
        for delta in (-1, 0, 1):
            add(f'minor-{minor:02x}-{delta:+d}', kind=0x12, minor=minor, w=0x0100, extra=((v + delta) - 0x0100) & 0xFFFF)
    v = V(0x13, 0x20)
    for resident_cs in (0x7000, 0x7345, 0xFE00):
        for wcs in (0x6000, 0x2345):
            vv = V(0x13, 0x20, wcs, resident_cs)
            for delta in (0, 1):
                add(f'place-{wcs:04x}-{resident_cs:04x}-{delta}', kind=0x13, w_cs=wcs, resident_cs=resident_cs, w=0x10, extra=(vv + delta - 0x10) & 0xFFFF)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        for delta in (0, 1):
            add(f'sp-{sp:04x}-{delta}', sp=sp, kind=0x12, minor=0x40, w=0x10, extra=(V(0x12, 0x40) + delta - 0x10) & 0xFFFF)
    add('ss-ds', ss=0x1000, sp=0xE000, kind=0x16, w=1, extra=0x7000)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, kind=0x15, w=0x20, extra=0x7000); add(f'flags-{fl:04x}-lo', flags=fl, kind=0x15, w=0x20, extra=0)
    # the record near the end of its segment: W at offset FEFFh..FFFFh wraps (and one straddles FFFFh)
    for off in (0xFEFC, 0xFEFD, 0xFEFE, 0xFEFF, 0xFF00, 0xFFF0, 0xFFFF):
        add(f'rec-top-{off:04x}', rec=(0x3000, off), kind=0x12, minor=0x20, w=0x0100, extra=0x7000)
    for off in (0xFFFE, 0xFFFF, 0xFFF0):
        add(f'out-top-{off:04x}-above', out=(0x4000, off), kind=0x12, minor=0x20, w=0x0100, extra=0x7000); add(f'out-top-{off:04x}-not', out=(0x4000, off), kind=0x12, minor=0x20, w=0x0100, extra=0)
    for seg, off in ((0xFFFF, 0x0010), (0x0000, 0x0000)):
        add(f'out-{seg:04x}-{off:04x}', out=(seg, off), kind=0x12, minor=0x20, w=0x0100, extra=0x7000)
    # the pointers on the body's own frame. S is the entry SP, BP after PUSH BP is S-2; the local flag byte is at BP-1.
    S0 = 0x8000; BP = S0 - 2
    for k in list(range(-0x20, 0x12)):
        for branch, extra in (('above', 0x7000), ('not', 0)):
            add(f'clobber-out-bp{k:+03x}-{branch}', out=(0x2000, (BP + k) & 0xFFFF), sp=S0, kind=0x12, minor=0x20, w=0x0100, extra=extra)
    for k in range(-0x20, 0x12):
        for branch, extra in (('above', 0x7000), ('not', 0)):
            add(f'rec102-at-bp{k:+03x}-{branch}', rec=(0x2000, (BP + k - 0x102) & 0xFFFF), sp=S0, extra=extra)
    for k in (-3, -1, 0, 2, 4, 6):
        add(f'out-and-rec102-same-bp{k:+d}', rec=(0x2000, (BP + k - 0x102) & 0xFFFF), out=(0x2000, (BP + k) & 0xFFFF), sp=S0, extra=0x7000)
    add('out-is-rec102', rec=(0x3000, 0x0100), out=(0x3000, 0x0202), kind=0x12, minor=0x20, w=0x0100, extra=0x7000)
    add('out-is-rec10', rec=(0x3000, 0x0100), out=(0x3000, 0x0110), kind=0x12, minor=0x20, w=0x0100, extra=0x7000)
    add('out-over-rec-pointer-words', out=(0x2000, (S0 + 10) & 0xFFFF), sp=S0, kind=0x12, minor=0x20, w=0x0100, extra=0x7000)
    add('rec-over-args', rec=(0x2000, (S0 + 4 - 0x102) & 0xFFFF), sp=S0, extra=0x7000)
    for budget in range(0, 140):
        add(f'budget-not-{budget}', budget=budget, kind=0x12, minor=0x20, w=0x0100, extra=0)
    for budget in range(0, 280):
        add(f'budget-above-{budget}', budget=budget, kind=0x12, minor=0x20, w=0x0100, extra=0x7000)
    for k in range(20 if quick else 80):
        add(f'random{k}', kind=rng.randrange(256), minor=rng.randrange(256), w=rng.randrange(65536), extra=rng.randrange(65536),
            regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)),
            rec=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)), out=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)),
            resident_cs=rng.choice((0x7000, rng.randrange(0x4000, 0x9000))), sp=rng.randrange(SP_MIN, SP_MAX + 1))
    return out


def expected_transfer(name):
    return name.startswith(('clobber-', 'out-and-rec102'))


def statements(name, kw, st, bodies, decoded, expected, vcache):
    """Independent of the C model, for cases whose pointers do not alias the body's own frame or each other: (1) the frames +0000 sees; (2) the stored word and the returned byte, from V
    computed by the ORIGINAL +0000 alone; (3) the flags of the last flag-setting instruction; (4) SP, BP and CS:IP on return."""
    status, ram, out, left, seen, frames = expected
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    w_cs, rcs = info['w_cs'], info['resident_cs']
    kind, minor = kw.get('kind', 7), kw.get('minor', 0)
    w, extra = kw.get('w', 0x1234), kw.get('extra', 0x0010)
    rec = kw.get('rec', (0x3000, 0x0100)); outp = kw.get('out', (0x4000, 0x0200))
    lo_s = physical(info['ss'], (cpu.sp - 0x60) & 0xFFFF); hi_s = lo_s + 0x70                    # a random placement that happens to overlap the stack words is a differential-only case
    for base in (physical(rec[0], rec[1] & 0xFFFF), physical(rec[0], (rec[1] + 0x102) & 0xFFFF), physical(outp[0], outp[1] & 0xFFFF)):
        if lo_s - 0x40 < base < hi_s:
            return 0
    key = (kind, minor, w_cs, rcs)
    v = vcache.setdefault(key, original_value(bodies, decoded, kind, minor, w_cs, rcs))
    above = ((w + extra) & 0xFFFF) > v
    stored = ((v - w) & 0xFFFF) if above else 0
    at = physical(outp[0], outp[1] & 0xFFFF)
    check(ram[at] | (ram[(at + 1) & (SIZE - 1)] << 8) == stored, f'{name}: stored word {ram[at] | (ram[(at + 1) & (SIZE - 1)] << 8):04x} is not {stored:04x}')
    check(out.ax & 0xFF == (1 if above else 0), f'{name}: AL is not {int(above)}')
    check(len(frames) == (2 if above else 1) and all(f == (0x0064 if i == 0 else 0x0083, w_cs, rec[1] & 0xFFFF, rec[0]) for i, f in enumerate(frames)), f'{name}: +0000 frames {frames}')
    if above:
        a, b = v, w; r = (a - b) & 0xFFFF
        want = (1 if a < b else 0) | (4 if bin(r & 0xFF).count('1') % 2 == 0 else 0) | (0x10 if (a ^ b ^ r) & 0x10 else 0) | (0x40 if r == 0 else 0) | (0x80 if r & 0x8000 else 0) | (0x800 if ((a ^ b) & (a ^ r)) & 0x8000 else 0)
    else:
        want = 0x44
    check(out.flags & 0x08D5 == want, f'{name}: flags {out.flags & 0x08D5:04x} not {want:04x}')
    check(out.sp == (cpu.sp + 14) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); stated = 0; masked_n = 0; vcache = {}
    cs = [c for c in cases(bodies, decoded, quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 100 == 0 or name.startswith(('clobber-out-bp+0', 'rec102-at-bp+0', 'out-', 'random')) and index % 7 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual, masked = compare(name, st, bodies, decoded, lib, budget)
        masked_n += int(masked); stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[4]
        if actual[0] in ('ARGUMENT', 'CODE', 'CALLEE') or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        alias = name.startswith(('clobber', 'rec102', 'out-', 'rec-over', 'rec-top')) and not name.startswith(('out-top', 'out-ffff', 'out-0000'))
        if budget == 100000 and not alias:
            stated += statements(name, kw, st, bodies, decoded, expected, vcache)
    check(only is not None or len({k for k in seen if k[0] == 'w' and k[1] >= 0x54}) == 31, 'every instruction start of +0054 was reached')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(decoded)}', body54_starts=f'{len([k for k in seen if k[0] == "w" and k[1] >= 0x54])}/31',
                independent_statements_checked=stated, mul_undefined_flag_arrivals=masked_n, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('b-12-1234-eq', 'b-12-1234-p1', 'b-01-0000-m1', 'b-13-1234-far', 'place-2345-fe00-1', 'sp-0050-0', 'flags-0a87', 'flags-0a87-lo', 'budget-above-12', 'budget-above-60', 'budget-not-9',
                'clobber-out-bp-01-above', 'clobber-out-bp-01-not', 'clobber-out-bp+02-above', 'clobber-out-bp+00-not', 'clobber-out-bp+04-above', 'rec102-at-bp-01-above', 'rec102-at-bp+00-not',
                'rec-top-fefd', 'out-top-ffff-above')
MUTANTS = (
    ('JBE ignores ZF', [("if (c->flags & 0x0041u) {", "if (c->flags & 0x0001u) {")]),
    ('CMP operands swapped', [("(void)arith16(c, c->ax, dx, 1);", "(void)arith16(c, dx, c->ax, 1);")]),
    ('the record word read at +0100', [("brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)); c->ip = 0x006Eu;", "brd16(bus, c->es, (uint16_t)(c->di + 0x0100u)); c->ip = 0x006Eu;")]),
    ('extra read from [BP+8]', [("arith16(c, c->ax, bp16(ram, c, 0xA), 0)", "arith16(c, c->ax, bp16(ram, c, 0x8), 0)")]),
    ('the stored word is an ADD', [("arith16(c, c->ax, brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)), 1)", "arith16(c, c->ax, brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)), 0)")]),
    ('the flag byte values swapped', [("ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 0u;", "ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 2u;")]),
    ('the out pointer words swapped', [("STEP(); c->di = bp16(ram, c, 6); c->es = bp16(ram, c, 8); c->ip = 0x009Au;", "STEP(); c->di = bp16(ram, c, 8); c->es = bp16(ram, c, 6); c->ip = 0x009Au;")]),
    ('RETF pops 8 argument bytes', [("c->sp = (uint16_t)(c->sp + 10u);", "c->sp = (uint16_t)(c->sp + 8u);")]),
    ('the RETF return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_0054_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('XOR AX,AX leaves PF clear', [("parity((uint8_t)r) | (r ? 0u : 0x0040u)", "(r ? 0u : 0x0040u)")]),
    ('the JBE is not charged to the budget', [("    STEP();                                                                                   /* 0073 JBE 0093 */\n", "")]),
    ('the call-site displacement check expects 9Dh', [("0x9Cu, 0xFFu, cs", "0x9Du, 0xFFu, cs")]),
    ('MOV DX,AX dropped', [("c->dx = c->ax; dx = c->dx;", "dx = c->dx;")]),
    ('the second read of W reuses the first', [("STEP(); c->ax = arith16(c, c->ax, brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)), 1); c->ip = 0x008Bu;",
                                                  "STEP(); c->ax = arith16(c, c->ax, w_first, 1); c->ip = 0x008Bu;"),
                                                 ("    STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 0xA), 0); c->ip = 0x0071u;", "    w_first = c->ax; STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 0xA), 0); c->ip = 0x0071u;"),
                                                 ("uint8_t *ram; uint16_t cs, ret_ip, ret_cs, dx; int r;", "uint8_t *ram; uint16_t cs, ret_ip, ret_cs, dx, w_first = 0; int r;")]),
    ('the out store bypasses the bus', [("STEP(); bwr16(bus, c->es, c->di, c->ax); c->ip = 0x0091u;", "STEP(); wr16(ram, c->es, c->di, c->ax); c->ip = 0x0091u;")]),
)
EQUIVALENT_IN_DIFFERENTIAL = ('the out store bypasses the bus',)


def c_test_verdict(tmp, mutant_path):
    """A mutant the (EGA-less) differential cannot see is built into the C test, which attaches an EGA and hand-checks the bus claims."""
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u7f_0054_cpu' else ROOT / f'src/recovered/{s}.c') for s in SOURCES]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u7f_0054_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded):
    out = []; src = (ROOT / 'src/recovered/overlay_u7f_0054_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u7f-0054-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u7f_0054_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    caught = sum(1 for _, r in out if r.startswith('CAUGHT'))
    c_caught = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(total=len(out), caught_by_differential=caught, caught_by_c_test=c_caught, caught=caught + c_caught, results=out, survivors=[n for n, r in out if r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')], cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u7f-0054-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, a.quick)
        if a.native:
            from ovl_007f_0054_native import native
            report['native'] = native(bodies, decoded, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_007f_0054_cpu_audit'] = sys.modules[__name__]
    main()
