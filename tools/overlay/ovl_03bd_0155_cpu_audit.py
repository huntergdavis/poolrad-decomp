#!/usr/bin/env python3
"""OVL_03BD_0155 (unit 0x3BD [0155,0174), public entry 1) against the original bytes: CPU14, all 1,048,576 RAM bytes, the stop and the remaining budget, unmasked after every case.

The body copies the low bytes of its four argument slots to DS:[6A60..6A63] (first pushed to 6A60) through AL, and returns with RETF 8. It is a leaf (no call) and writes no flags.
Its four stores go to fixed DS globals and its loads come from its own frame, so everything is flat RAM. The audit places DS:6A60 ON every byte of the body's frame (DS = SS), so a
store can rewrite an argument a later load rereads, the saved BP, or the return address (TRANSFER). Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_03BD_0155',)
ENTRY, END = 0x0155, 0x0174
SHA = 'f5fb6f0833aa34a68327cc0cb2ae1576f743a3f0fdb27fe7b8a101f4de8ca402'
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0002, 0xFFF4
SOURCES = ['overlay_u3bd_entry1_0155_cpu']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'TRANSFER'}
GLOBALS = 0x6A60
ENTRY_RET = A.ENTRY_RET
check = A.check


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x3BD)
    check(unit['entries'][1] == ENTRY, 'public entry 1 is +0155')
    body = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(body) == 31 and sha256(body).hexdigest() == SHA and len(ins) == 12 and sum(i.size for i in ins) == 31, 'unit 0x3BD +0155 identity: 31 bytes / 12 instructions')
    check(not [i for i in ins if i.mnemonic.startswith('j') or i.mnemonic in ('call', 'lcall', 'int', 'in', 'out')], 'a leaf: straight line, no jump, call, interrupt or port instruction')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(0x0171, '8')], 'the only return is RETF 8 at 0171')
    check([int(i.op_str.split('[')[1].split(']')[0], 16) for i in ins if i.mnemonic == 'mov' and i.op_str.startswith('byte ptr [0x')] == [0x6A60, 0x6A61, 0x6A62, 0x6A63],
          'the four stores go to DS:6A60..6A63 in order')
    check(not [f for f in unit['fixups'] if ENTRY <= f < END], 'no relocation inside the span')
    return {('w', ENTRY): body}, {('w', i.address): i for i in ins}, dict(bytes=31, instructions=12, sha256=SHA, file_offset=unit['ovr_offset'] + ENTRY,
                                                                          memory_segment=unit['memory_segment'])


def library(directory, override=None):
    path = Path(directory) / ('u3bd_0155_mut.so' if override else 'u3bd_0155.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u3bd_entry1_0155_cpu.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]
    lib.pr_u3bd_entry1_0155_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state, budget):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget)
    status = lib.pr_u3bd_entry1_0155_cpu(ctypes.addressof(ram), SIZE, ctypes.byref(out), ctypes.byref(left))
    return STATUS[status], bytes(ram), out, left.value, None


def fixture(bodies, *, args=(0x0B, 0x00, 0x07, 0x06), highs=(0xCD, 0xCD, 0xCD, 0xCD), sp=0x8000, ds=0x1000, ss=0x2000, w_cs=W_CS, flags=0x0202, regs=None, background=0xA5):
    """Entry frame (SP+0): return IP, CS; then the four argument slots, the LAST pushed (args[3]) nearest. args[0] is the first pushed (at SP+0A, read by [BP+0C])."""
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    for (kind, start), blob in bodies.items():
        at = physical(w_cs, start); m[at:at + len(blob)] = blob
    slots = [(highs[3 - k] << 8) | args[3 - k] for k in range(4)]                   # SP+4 holds the last pushed
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0]) + tuple(slots)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, resident_cs=0, sp=sp, ss=ss, ds=ds, args=args)


def compare(label, state, bodies, decoded, lib, budget=100000):
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


def cases(quick=False):
    rng = random.Random(0x3BD0155)
    out = []

    def add(name, budget=100000, **kw):
        out.append((name, kw, budget))
    for v in range(256):                                               # every byte value through every slot
        add(f'v-{v:02x}', args=(v, (v + 1) & 0xFF, (v + 2) & 0xFF, (v + 3) & 0xFF))
    add('program-body-call', args=(0x0B, 0x00, 0x07, 0x06))
    for h in (0x00, 0x80, 0xFF):
        add(f'highs-{h:02x}', highs=(h, h, h, h))
    for ds in (0x0000, 0x1000, 0xFFFF, 0xF96A, 0x9000):                # FFFF and F96A put 6A60.. above 1 MiB (A20 off: they wrap)
        add(f'ds-{ds:04x}', ds=ds)
    for sp in (SP_MIN, SP_MIN + 1, 0x0101, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202, 0x0ED7):
        add(f'flags-{fl:04x}', flags=fl)
    for w in (0x6000, 0x2345, 0xF000):
        add(f'at-{w:04x}', w_cs=w)
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x10, 0x14):                                       # DS = SS with DS:6A60 on each byte of the frame: saved BP, return address, the four slots, the caller's words
        add(f'globals-at-bp{k:+03x}', ds=0x2000, ss=0x2000, sp=(GLOBALS - k + 2) & 0xFFFF)
    for budget in range(0, 14):
        add(f'budget-{budget}', budget=budget, args=(0x90, 0xF0, 0x0F, 0x01))
    for k in range(30 if quick else 200):
        add(f'random{k}', args=tuple(rng.randrange(256) for _ in range(4)), highs=tuple(rng.randrange(256) for _ in range(4)), ds=rng.randrange(65536),
            regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1),
            w_cs=rng.choice((0x6000, 0x2345)))
    return out


def expected_transfer(name):
    return name.startswith('globals-at-bp')


def statements(name, st, expected):
    """Independent of the C model, for cases whose globals do not overlap the stack frame: the four DS bytes from the arguments alone, AL, SP and BP."""
    status, ram, out, left, seen, frames = expected
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    g = [physical(info['ds'], GLOBALS + k) for k in range(4)]
    lo = physical(info['ss'], (cpu.sp - 4) & 0xFFFF); hi = physical(info['ss'], (cpu.sp + 0x0C) & 0xFFFF)
    if any(lo <= a < hi for a in g) or any(physical(info['w_cs'], ENTRY) <= a < physical(info['w_cs'], END) for a in g):
        return 0
    check([ram[a] for a in g] == list(info['args']), f'{name}: globals {[hex(ram[a]) for a in g]} args {info["args"]}')
    check(out.ax & 0xFF == info['args'][3] and out.ax >> 8 == cpu.ax >> 8, f'{name}: AL is the last copied byte, AH unchanged')
    check(out.sp == (cpu.sp + 12) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET and out.flags == cpu.flags, f'{name}: SP/BP/CS:IP/flags on return')
    return 1


def differential(bodies, decoded, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); stated = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 100 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual = compare(name, st, bodies, decoded, lib, budget)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[4]
        if actual[0] == 'ARGUMENT' or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == 100000:
            stated += statements(name, st, expected)
    check(only is not None or len(seen) == 12, 'every instruction start was reached')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(decoded)}', independent_statements_checked=stated, cpu_words=14, ram_bytes=SIZE,
                flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('v-00', 'v-7f', 'v-fe', 'program-body-call', 'highs-ff', 'ds-ffff', 'sp-0002', 'sp-fff4', 'flags-08d7', 'budget-0', 'budget-3', 'budget-11', 'budget-12',
                'globals-at-bp+00', 'globals-at-bp+02', 'globals-at-bp+04', 'globals-at-bp+0a', 'globals-at-bp+0c', 'random0', 'random1')
MUTANTS = (
    ('the first load reads [BP+0A]', [("c->bp + 0x0Cu))); AT(0x015Bu);", "c->bp + 0x0Au))); AT(0x015Bu);")]),
    ('the stores go to 6A61..6A64', [("0x6A60u, (uint8_t)c->ax", "0x6A64u, (uint8_t)c->ax")]),
    ('the last store writes AH', [("wr8(ram, c->ds, 0x6A63u, (uint8_t)c->ax)", "wr8(ram, c->ds, 0x6A63u, (uint8_t)(c->ax >> 8u))")]),
    ('the loads clear AH', [("static void mov_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }", "static void mov_al(cpu_t *c, uint8_t v) { c->ax = v; }")]),
    ('POP BP is skipped', [("STEP(); c->bp = pop(ram, c); AT(0x0171u);", "STEP(); c->sp = (uint16_t)(c->sp + 2u); AT(0x0171u);")]),
    ('RETF pops 6 argument bytes', [("c->sp = (uint16_t)(c->sp + 8u);", "c->sp = (uint16_t)(c->sp + 6u);")]),
    ('the return-address (TRANSFER) check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U3BD_0155_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the loads are taken from the entry SP, not BP', [("c->bp + 0x0Cu", "c->sp + 0x0Cu")]),
    ('the stores are not charged to the budget', [("STEP(); wr8(ram, c->ds, 0x6A61u", "wr8(ram, c->ds, 0x6A61u")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u3bd_entry1_0155_cpu.c'),
                        str(mutant_path), '-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded):
    out = []; src = (ROOT / 'src/recovered/overlay_u3bd_entry1_0155_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) >= 1, f'mutant text occurs: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u3bd-0155-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u3bd_entry1_0155_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential' + c_test_verdict(tmp, path)))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:90]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:70]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    d = sum(1 for _, r in out if r.startswith('CAUGHT:')); c = sum(1 for _, r in out if r.endswith('CAUGHT BY THE C TEST'))
    return dict(total=len(out), caught=d + c, caught_by_differential=d, caught_by_c_test=c, results=out,
                survivors=[n for n, r in out if r.startswith('SURVIVED') and not r.endswith('CAUGHT BY THE C TEST')] + [n for n, r in out if r == 'COMPILE-ERROR'],
                cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u3bd-0155-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, a.quick)
        if a.native:
            from ovl_03bd_0155_native import native
            report['native'] = native(bodies, decoded, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_03bd_0155_cpu_audit'] = sys.modules[__name__]
    main()
