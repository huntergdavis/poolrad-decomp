#!/usr/bin/env python3
"""OVL_006A_0824 (unit 0x6A [0824,08BF)) against the original bytes: CPU14, all 1,048,576 RAM bytes, the stop and the remaining budget, unmasked after every case.

The body sorts the 21 bytes at G+17h..G+2Bh (G is the far pointer at DS:[5D92]) into ascending order of their low seven bits with a selection-exchange loop. It is a leaf with no arguments. The
array accesses go through a caller-supplied pointer, so the model sends them through the bus; G is RE-READ before every access. The loop counters i, j and the swap temporary live in the body's own
frame (BP-1, BP-2, BP-3): the audit places the array ON that frame, on the saved BP and the return address, and over the global pointer G itself, so the sort changes its own loop, its own
return and its own pointer. Those runs may not terminate; they run under a budget and the original and the model are compared at the same LIMIT arrival. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_006A_0824',)
ENTRY, END = 0x0824, 0x08BF
SHA = 'c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7'
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0008, 0xFFFC
SOURCES = ['overlay_u6a_0824_raw_cpu', 'memory_bus', 'ega_hw']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'TRANSFER'}
ENTRY_RET = A.ENTRY_RET
BUDGET = 100000
ALIAS_BUDGET = 30000
check = A.check


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.c_void_p), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p), ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


def reference(arr):
    """The selection-exchange loop of the listing, in Python: i = 0..13h, j = i..14h, swap when (a[i] & 7F) > (a[j] & 7F)."""
    a = list(arr)
    for i in range(0x14):
        for j in range(i, 0x15):
            if (a[i] & 0x7F) > (a[j] & 0x7F):
                a[i], a[j] = a[j], a[i]
    return a


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x6A)
    body = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 155 and sha256(body).hexdigest() == SHA and len(ins) == 57 and sum(i.size for i in ins) == 155, 'unit 0x6A +0824 identity: 155 bytes / 57 instructions')
    check(not [i for i in ins if i.mnemonic in ('call', 'lcall', 'int', 'in', 'out', 'mul', 'imul', 'div', 'idiv', 'shl', 'shr')], 'a leaf: no call, interrupt, port, multiply, divide or shift')
    check([(i.address, i.mnemonic, int(i.op_str, 16)) for i in ins if i.mnemonic.startswith('j')] == [(0x082E, 'jmp', 0x0833), (0x0838, 'ja', 0x08B2), (0x083D, 'jmp', 0x0842), (0x0868, 'jbe', 0x08AC),
          (0x08B0, 'jne', 0x083F), (0x08B6, 'je', 0x08BB), (0x08B8, 'jmp', 0x0830)], 'the seven jumps of the two loops')
    check([i.address for i in ins if i.mnemonic == 'retf'] == [0x08BE] and ins[-1].op_str == '', 'the only return is a plain RETF at 08BE')
    check(not [f for f in unit['fixups'] if ENTRY <= f < END], 'no relocation inside the span')
    return {('w', ENTRY): body}, {('w', i.address): i for i in ins}, dict(bytes=155, instructions=57, sha256=SHA, file_offset=unit['ovr_offset'] + ENTRY, memory_segment=unit['memory_segment'])


def library(directory, override=None):
    path = Path(directory) / ('u6a_0824_mut.so' if override else 'u6a_0824.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u6a_0824_raw_cpu.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]; lib.pr_u6a_0824_raw_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state, budget):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget); bus = Bus(ctypes.addressof(ram), None, None, None, None)
    status = lib.pr_u6a_0824_raw_cpu(ctypes.byref(bus), SIZE, ctypes.byref(out), ctypes.byref(left))
    return STATUS[status], bytes(ram), out, left.value, None


def fixture(bodies, *, arr=None, g=(0x3000, 0x0100), sp=0x8000, ds=0x1000, ss=0x2000, w_cs=W_CS, flags=0x0202, regs=None, background=0xA5, arr_seed=0):
    """Entry frame: return IP, CS. DS:[5D92]/[5D94] hold G; the 21 array bytes are at G+17h. The array is written first, so anything placed over code, the frame or G reads what the later writes leave."""
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    arr = list(arr) if arr is not None else [random.Random(arr_seed).randrange(256) for _ in range(21)]
    for k in range(21):
        m[physical(g[0], (g[1] + 0x17 + k) & 0xFFFF)] = arr[k]
    put16(m, ds, 0x5D92, g[1]); put16(m, ds, 0x5D94, g[0])
    for (kind, start), blob in bodies.items():
        at = physical(w_cs, start); m[at:at + len(blob)] = blob
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, resident_cs=0, sp=sp, ss=ss, ds=ds, g=g, arr=arr)


def compare(label, state, bodies, decoded, lib, budget=BUDGET):
    actual = run_c(lib, state, budget); expected = A.run_original(state, decoded, budget)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    check(actual[3] == expected[3], f'{label}: budget left C {actual[3]} original {expected[3]}')
    for name, _ in Cpu._fields_:
        check(getattr(actual[2], name) == getattr(expected[2], name), f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    for (kind, start), blob in bodies.items():
        at = physical(state[2]['w_cs'], start); check(expected[1][at:at + len(blob)] == state[0][at:at + len(blob)], f'{label}: code written')
    return expected, actual


# ---------------------------------------------------------------------------------------------------- cases
def cases(quick=False):
    rng = random.Random(0x6A0824)
    out = []

    def add(name, budget=BUDGET, **kw):
        out.append((name, kw, budget))
    rb = lambda: [rng.randrange(256) for _ in range(21)]
    specials = {'asc': list(range(21)), 'desc': list(range(20, -1, -1)), 'equal': [7] * 21, 'equal-hi': [0x87] * 21, 'zeros': [0] * 21, 'ff': [0xFF] * 21, 'hi-lo-mix': [0x80 | (k % 5) if k % 2 else k % 5 for k in range(21)],
                'ties': [3, 0x83, 3, 0x83, 1, 0x81, 1, 0x81, 2, 2, 0x82, 0x82, 3, 3, 3, 0x83, 0x83, 0x83, 1, 2, 3], 'one-swap': list(range(19)) + [20, 19], 'last-small': [5] * 20 + [1], 'first-big': [0x7F] + [1] * 20,
                'all-hi-desc': [0x80 | k for k in range(20, -1, -1)]}
    for name, arr in specials.items():
        add(f'arr-{name}', arr=arr)
    for k in range(30 if quick else 150):
        add(f'rand-{k}', arr=rb())
    for off in (0x0000, 0xFFE0, 0xFFE9, 0xFFEA, 0xFFEB, 0xFFF0, 0xFFFF, 0x8000):
        add(f'g-off-{off:04x}', g=(0x3000, off), arr=rb())
    for seg in (0xFFFF, 0xFFF0, 0x0000, 0xA000, 0x9FFF):
        add(f'g-seg-{seg:04x}', g=(seg, 0x0200), arr=rb())
    for sp in (SP_MIN, SP_MIN + 1, 0x0101, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, arr=rb())
    add('ss-ds', ss=0x1000, sp=0xE000, arr=rb()); add('ss-grid', ss=0x3000, sp=0x8000, arr=rb())
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, arr=rb())
    for w in (0x6000, 0x2345, 0xF000):
        add(f'at-{w:04x}', w_cs=w, arr=rb())
    fixed = [9, 3, 0x85, 1, 0x80, 4, 4, 0x7F, 2, 0xFF, 6, 0, 8, 7, 5, 0x81, 12, 11, 10, 14, 13]
    for budget in list(range(0, 80)) + [100, 199, 200, 201, 500, 1000, 2000, 3000, 5000]:
        add(f'budget-{budget}', budget=budget, arr=fixed)
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x30, 0x14):                                       # the array start on each byte from BP-30h to BP+13h: the locals, the saved BP, the return address, the caller's words
        add(f'array-at-bp{k:+03x}', budget=ALIAS_BUDGET, g=(0x2000, (BP + k - 0x17) & 0xFFFF), sp=S0, arr=fixed)
    for t in range(0, 21):                                             # the array over the global pointer G itself: the sort rewrites G, and G is re-read before every access
        add(f'array-over-g-{t:02d}', budget=ALIAS_BUDGET, g=(0x1000, (0x5D92 - t - 0x17) & 0xFFFF), arr=fixed)
    for t in (0, 4, 10, 20):
        add(f'array-over-g-{t:02d}-rand', budget=ALIAS_BUDGET, g=(0x1000, (0x5D92 - t - 0x17) & 0xFFFF), arr=rb())
    for k in (-3, -2, -1):                                             # values that make the loop counters hit the bounds: i = 14h (JA equal), j = 14h
        for v in (0x13, 0x14, 0x15, 0x00, 0xFF):
            add(f'counter-val-bp{k:+d}-{v:02x}', budget=ALIAS_BUDGET, g=(0x2000, (BP + k - 0x17) & 0xFFFF), sp=S0, arr=[v] + [3] * 20)
    # fixtures FOUND by searching random frame placements for states that separate mutants from the original (a surviving mutant is a missing fixture): the loop counter i left above 13h by
    # the sort's own stores (INC with CF clear), and states whose budget stops between a flag-setting instruction and the next
    FOUND = {'found-ja-1': (-20, [127, 22, 22, 18, 99, 22, 21, 173, 22, 160, 1, 128, 148, 134, 148, 16, 21, 22, 239, 109, 147], 3000),
             'found-inc-cf-1': (-11, [196, 21, 105, 18, 0, 22, 147, 1, 128, 22, 142, 19, 191, 175, 24, 66, 186, 178, 19, 63, 18], 3000),
             'found-inc-cf-2': (-8, [147, 51, 104, 81, 169, 106, 150, 33, 127, 227, 128, 243, 72, 20, 148, 18, 234, 43, 199, 148, 223], 500),
             'found-af-1': (-12, [147, 193, 128, 1, 186, 58, 20, 0, 6, 7, 1, 20, 147, 1, 148, 255, 222, 193, 150, 244, 19], 200)}
    for name, (k, arr, budget) in FOUND.items():
        add(name, budget=budget, g=(0x2000, (BP + k - 0x17) & 0xFFFF), sp=S0, arr=arr)
    for k in range(20 if quick else 60):
        add(f'random{k}', arr=rb(), regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)),
            g=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)), sp=rng.randrange(SP_MIN, SP_MAX + 1), w_cs=rng.choice((0x6000, 0x2345)))
    return out


def expected_transfer(name):
    return name.startswith(('array-at-bp', 'array-over-g', 'counter-val'))


def statements(name, kw, st, expected):
    """Independent of the C model, for cases whose array does not overlap the stack window, the globals or the code: the final array is the Python selection-exchange of the initial one, it is a
    permutation of it, it is ordered by the low seven bits, and SP, BP and CS:IP are restored."""
    status, ram, out, left, seen, frames = expected
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    g, arr = info['g'], info['arr']
    addrs = [physical(g[0], (g[1] + 0x17 + k) & 0xFFFF) for k in range(21)]
    lo = physical(info['ss'], (cpu.sp - 0x20) & 0xFFFF); hi = lo + 0x30
    if any(lo - 1 <= a < hi or abs(a - physical(info['ds'], 0x5D92)) < 4 or abs(a - physical(info['w_cs'], 0x0824)) < 0x9F for a in addrs):
        return 0
    final = [ram[a] for a in addrs]
    check(final == reference(arr), f'{name}: array {final} is not the selection-exchange of {arr}')
    check(sorted(final) == sorted(arr), f'{name}: not a permutation')
    check(all((final[k] & 0x7F) <= (final[k + 1] & 0x7F) for k in range(20)), f'{name}: not ordered by the low seven bits')
    check(out.sp == (cpu.sp + 4) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); stated = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 100 == 0 or name.startswith(('array-at-bp+0', 'array-over', 'counter-val')) and index % 7 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual = compare(name, st, bodies, decoded, lib, budget)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[4]
        if actual[0] == 'ARGUMENT' or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == BUDGET and not name.startswith(('g-seg-a000',)):
            stated += statements(name, kw, st, expected)
    check(only is not None or len(seen) == 57, 'every instruction start was reached')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(decoded)}', independent_statements_checked=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = tuple(f'budget-{b}' for b in range(0, 80)) + ('found-ja-1', 'found-inc-cf-1', 'found-inc-cf-2', 'found-af-1', 'arr-ties', 'arr-desc', 'arr-hi-lo-mix', 'arr-equal-hi', 'rand-0', 'rand-1', 'g-off-ffe9', 'flags-0a87', 'sp-0008', 'budget-33', 'budget-500', 'array-at-bp-01', 'array-at-bp+00', 'array-at-bp+02', 'array-at-bp+04',
                'array-at-bp-03', 'array-over-g-04', 'array-over-g-10', 'counter-val-bp-1-14', 'counter-val-bp-2-14', 'counter-val-bp-1-13')
MUTANTS = (
    ('JBE ignores ZF (equal elements are swapped)', [("c->ip = (c->flags & 0x0041u) != 0u ? 0x08ACu : 0x086Au;", "c->ip = (c->flags & 0x0001u) != 0u ? 0x08ACu : 0x086Au;")]),
    ('the first comparison value is not masked to seven bits', [("case 0x0851u: set_al(c, logic8(c, (uint8_t)(c->ax & 0x7Fu)));", "case 0x0851u: set_al(c, logic8(c, (uint8_t)(c->ax & 0xFFu)));")]),
    ('the array is read at +16h', [("physical(c->es, (uint16_t)(c->di + 0x17u))); }\nstatic void elem_wr", "physical(c->es, (uint16_t)(c->di + 0x16u))); }\nstatic void elem_wr")]),
    ('the outer loop ends at 14h', [("arith(c, local(ram, c, -1), 0x13u, 8u, 1)", "arith(c, local(ram, c, -1), 0x14u, 8u, 1)")]),
    ('the inner loop ends at 13h', [("arith(c, local(ram, c, -2), 0x14u, 8u, 1)", "arith(c, local(ram, c, -2), 0x13u, 8u, 1)")]),
    ('the JA bound is 13h', [("(void)arith(c, (uint8_t)c->ax, 0x14u, 8u, 1);", "(void)arith(c, (uint8_t)c->ax, 0x13u, 8u, 1);")]),
    ('the swap temporary is not reloaded', [("case 0x089Au: set_dl(c, local(ram, c, -3)); c->ip = 0x089Du; break;", "case 0x089Au: c->ip = 0x089Du; break;")]),
    ('G is read from 5D94/5D96', [("c->di = rd16(ram, c->ds, 0x5D92u); c->es = rd16(ram, c->ds, 0x5D94u);", "c->di = rd16(ram, c->ds, 0x5D94u); c->es = rd16(ram, c->ds, 0x5D96u);")]),
    ('the first loop counter starts at 1', [("set_local(ram, c, -1, 0u);", "set_local(ram, c, -1, 1u);")]),
    ('SUB SP,6', [("c->sp = arith(c, c->sp, 4u, 16u, 1);", "c->sp = arith(c, c->sp, 6u, 16u, 1);")]),
    ('INC BYTE also sets CF', [("c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);\n    return r;\n}\nstatic void xor_ah", "c->flags = (uint16_t)((c->flags & ~0x08D5u) | f | 1u);\n    return r;\n}\nstatic void xor_ah")]),
    ('XOR AH,AH leaves AH', [("static void xor_ah(cpu_t *c) { (void)logic8(c, 0u); c->ax &= 0x00FFu; }", "static void xor_ah(cpu_t *c) { (void)logic8(c, 0u); }")]),
    ('AND/XOR set AF', [("c->flags = (uint16_t)((c->flags & ~0x08D5u) | parity(r) | (r ? 0u : 0x0040u) | ((r & 0x80u) ? 0x0080u : 0u)); return r;", "c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x0010u | parity(r) | (r ? 0u : 0x0040u) | ((r & 0x80u) ? 0x0080u : 0u)); return r;")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U6A_0824_RAW_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the array read bypasses the bus', [("return pr_bus_read8(bus, physical(c->es, (uint16_t)(c->di + 0x17u)));", "return bus->ram[physical(c->es, (uint16_t)(c->di + 0x17u))];")]),
    ('the array write bypasses the bus', [("pr_bus_write8(bus, physical(c->es, (uint16_t)(c->di + 0x17u)), v);", "bus->ram[physical(c->es, (uint16_t)(c->di + 0x17u))] = v;")]),
)


EQUIVALENT_MUTANTS = (('the JA ignores ZF', [("c->ip = (c->flags & 0x0041u) == 0u ? 0x08B2u : 0x083Au;", "c->ip = (c->flags & 0x0001u) == 0u ? 0x08B2u : 0x083Au;")],
                       'JA at 0838 follows CMP AL,14h, so the two differ only when AL = 14h at 0836. AL is the loop counter i, which is written only by the MOV at 082A (0) and the INC at 0830, and 0830 is reached only '
                       'through 08B8, after CMP [BP-1],13h / JE 08BB fell through (i != 13h). So i + 1 != 14h at 0836, whatever the array stores do, and the original never reaches AL = 14h there.'),)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u6a_0824_raw_cpu' else ROOT / f'src/recovered/{s}.c') for s in SOURCES]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u6a_0824_raw_cpu.c')] + srcs + ['-o', str(exe)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded):
    out = []; src = (ROOT / 'src/recovered/overlay_u6a_0824_raw_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u6a-0824-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u6a_0824_raw_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    equivalent = []
    for name, edits, proof in EQUIVALENT_MUTANTS:
        text = src
        for old_, new_ in edits:
            check(text.count(old_) == 1, f'equivalent mutant text occurs once: {name}'); text = text.replace(old_, new_)
        with tempfile.TemporaryDirectory(prefix='u6a-0824-eq-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text); mlib = library(tmp, override={'overlay_u6a_0824_raw_cpu': path})
            try:
                r = differential(bodies, decoded, mlib, quick=False)
                equivalent.append(dict(name=name, survived_all_cases=r['cases'], proof=proof))
            except (ValueError, AssertionError) as e:
                equivalent.append(dict(name=name, NOT_EQUIVALENT=str(e)[:120], proof=proof))
    d = sum(1 for _, r in out if r.startswith('CAUGHT:')); c = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(equivalent=equivalent, total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out,
                survivors=[n for n, r in out if (r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')) or r == 'COMPILE-ERROR'], cases=list(MUTANT_CASES))


def crosscheck_existing(bodies, decoded, lib, quick=False):
    """The ALREADY MERGED full-C translation of the same function (overlay_u6a_sort_0824, flat RAM, no budget) against the original bytes on this audit's cases, wherever the raw model terminates.
    Reports every case where its final CPU14 or RAM differs from the original; it does not raise, because it is a finding about another model, not a claim of this one."""
    with tempfile.TemporaryDirectory(prefix='u6a-0824-old-') as tmp:
        path = Path(tmp) / 'old.so'
        subprocess.run(['cc', '-std=c11', '-Wall', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'), str(ROOT / 'src/recovered/overlay_u6a_sort_0824.c'), '-o', str(path)], check=True)
        old = ctypes.CDLL(str(path)); old.pr_u6a_sort_0824.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(Cpu)]; old.pr_u6a_sort_0824.restype = ctypes.c_int
        compared = 0; mismatches = []; skipped = 0
        for name, kw, budget in cases(quick):
            st = fixture(bodies, **kw)
            mine = run_c(lib, st, budget)
            if mine[0] not in ('RETURNED', 'TRANSFER'):
                skipped += 1; continue
            expected = A.run_original(st, decoded, budget)
            ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(st[0]); cpu = Cpu.from_buffer_copy(st[1])
            status = old.pr_u6a_sort_0824(ram, SIZE, ctypes.byref(cpu)); compared += 1
            diffs = [n for n, _ in Cpu._fields_ if getattr(cpu, n) != getattr(expected[2], n)]
            ramdiff = [i for i in range(SIZE) if ram[i] != expected[1][i]] if bytes(ram) != expected[1] else []
            if diffs or ramdiff:
                mismatches.append(dict(case=name, status=status, original=expected[0], cpu_words=diffs, ram_bytes_differing=len(ramdiff), first=[hex(i) for i in ramdiff[:4]]))
    return dict(cases_compared=compared, skipped_not_terminating=skipped, mismatching_cases=len(mismatches), mismatches=mismatches[:60])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native', '--crosscheck-existing'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u6a-0824-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, a.quick)
        if a.native:
            from ovl_006a_0824_native import native
            report['native'] = native(bodies, decoded, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded)
        if a.crosscheck_existing:
            report['existing_model_crosscheck'] = crosscheck_existing(bodies, decoded, lib, a.quick)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_006a_0824_cpu_audit'] = sys.modules[__name__]
    main()
