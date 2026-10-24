#!/usr/bin/env python3
"""OVL_0025_06D3 (the text-code remap) and OVL_0025_06FB (the ECL address classifier), the two shared raw entries for the full-state bodies
inside overlay_u25_string_cores.c, against the original bytes of GAME.OVR unit 0x25 (hash-checked, no fixup in either span): CPU14, all
1,048,576 RAM bytes and the stop, flags UNMASKED at the RETF, plus every instruction start and conditional outcome. Both are leaves called
as PUSH CS; CALL near, with one argument word, ending RETF 2. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_0025_06D3', 'OVL_0025_06FB')
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0004, 0xFFFA
SOURCES = ['overlay_u25_string_cores']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'TRANSFER'}
BODIES = {   # entry: (end, bytes, instructions, C symbol)
    0x06D3: (0x06FB, 40, 16, 'pr_u25_char_remap_06d3_cpu'),
    0x06FB: (0x0756, 91, 28, 'pr_u25_address_class_06fb_cpu'),
}
RANGES = ((0x4900, 0x4CFF), (0x6B00, 0x6EFF), (0x9700, 0x98FF), (0x9900, 0xB6FF))
ENTRY_RET = A.ENTRY_RET
check = A.check
REGISTERS = A.REGISTERS


def source():
    from overlay_inventory import find_units, parse_mz
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'START.EXE / GAME.OVR provenance differs')
    u = {x['stub_para']: x for x in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}[0x25]
    code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]; md = Cs(CS_ARCH_X86, CS_MODE_16)
    out = {}; meta = {}
    for entry, (end, size, count, _) in BODIES.items():
        body = bytes(code[entry:end]); ins = list(md.disasm(body, entry))
        check(sum(i.size for i in ins) == len(body) == size and len(ins) == count, f'decode covers [{entry:04X},{end:04X}): {size} bytes / {count} instructions')
        check([i.op_str for i in ins if i.mnemonic == 'retf'] == ['2'] and ins[-1].mnemonic == 'retf', f'{entry:04X}: the only return is the last, RETF 2')
        check(not [i for i in ins if i.mnemonic in ('call', 'lcall', 'int', 'in', 'out', 'loop')], f'{entry:04X}: a leaf')
        check(not [f for f in u['fixups'] if entry <= f < end], f'{entry:04X}: no fixup in the span')
        out[entry] = (body, {i.address: i for i in ins})
        meta[f'{entry:04X}'] = dict(bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), file_offset=u['ovr_offset'] + entry)
    return out, meta


def library(directory, override=None):
    path = Path(directory) / ('u25_leaf_mut.so' if override else 'u25_leaf.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    for _, (_, _, _, symbol) in BODIES.items():
        f = getattr(lib, symbol); f.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Cpu)]; f.restype = ctypes.c_int
    return lib


def run_c(lib, state):
    memory, cpu, info = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu)
    status = getattr(lib, BODIES[info['entry']][3])(ctypes.addressof(ram), SIZE, ctypes.byref(out))
    return STATUS[status], bytes(ram), out


def run_original(state, ips, limit=10000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    memory, cpu, info = state; backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    s = dict(stop=None, steps=0, prev=None); seen = set(); edges = set()

    def hook(em, address, size, user):
        cs = em.reg_read(UC_X86_REG_CS); ip = em.reg_read(UC_X86_REG_IP)
        if cs != info['w_cs'] or ip not in ips:
            s['stop'] = 'RETURNED' if (cs, ip) == ENTRY_RET else 'TRANSFER'; return em.emu_stop()
        seen.add(ip)
        if s['prev'] is not None:
            edges.add((s['prev'], ip))
        s['prev'] = ip; s['steps'] += 1
        if s['steps'] > limit:
            s['stop'] = 'RUNAWAY'; em.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=limit + 10)
    check(s['stop'] in ('RETURNED', 'TRANSFER'), f'original stop {s["stop"]}')
    out = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return s['stop'], bytes(backing), out, seen, edges


def fixture(bodies, entry, *, arg=0x0041, ss=0x2000, sp=0x8000, ds=0x5000, w_cs=W_CS, flags=0x0202, regs=None, background=0xA5):
    body = bodies[entry][0]
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, entry, ds, r[7], ss, flags | 0x0002)
    at = physical(w_cs, entry); m[at:at + len(body)] = body
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0], arg)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(entry=entry, w_cs=w_cs, ss=ss, sp=sp, arg=arg)


def compare(label, state, ips, lib):
    actual = run_c(lib, state); expected = run_original(state, ips)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    for name, _ in Cpu._fields_:
        check(getattr(actual[2], name) == getattr(expected[2], name), f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    return expected, actual


def remap(code):
    return code if code > 0x1F else (code + 0x40) & 0xFF


def address_class(a):
    c = 4
    for k, (lo, hi) in enumerate(RANGES):
        if lo <= a <= hi:
            c = k
    return c


def cases(quick=False):
    rng = random.Random(0x002506D3)
    out = []

    def add(name, entry, **kw):
        out.append((name, entry, kw))
    for code in range(256):                                                                   # every code byte; a random high byte
        add(f'06d3-code-{code:02x}', 0x06D3, arg=(rng.randrange(256) << 8) | code)
    edges = sorted({(v + d) & 0xFFFF for lo, hi in RANGES for v in (lo, hi) for d in (-1, 0, 1)} | {0x0000, 0xFFFF, 0x8000, 0x7FFF})
    for a in edges:                                                                           # every range edge +-1
        add(f'06fb-addr-{a:04x}', 0x06FB, arg=a)
    for entry, arg in ((0x06D3, 0x001F), (0x06D3, 0x0020), (0x06FB, 0x4A00), (0x06FB, 0xC000)):
        for fl in (0x0002, 0x0202, 0x08D7, 0x0ED7, 0x0003, 0x7202):
            add(f'{entry:04x}-flags-{fl:04x}-{arg:04x}', entry, arg=arg, flags=fl)
        for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, SP_MAX - 1, SP_MAX):
            add(f'{entry:04x}-sp-{sp:04x}-{arg:04x}', entry, arg=arg, sp=sp)
        add(f'{entry:04x}-a20-{arg:04x}', entry, arg=arg, ss=0xFFFF, sp=0x8000)
    for k in range(40 if quick else 300):
        entry = 0x06D3 if k % 2 else 0x06FB
        arg = rng.randrange(65536) if entry == 0x06D3 or k % 3 else rng.choice(RANGES)[rng.randrange(2)] + rng.choice((-1, 0, 1))
        add(f'random{k}', entry, arg=arg & 0xFFFF, regs=tuple(rng.randrange(65536) for _ in range(8)),
            flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1),
            background=rng.randrange(256), ds=rng.randrange(65536), ss=rng.choice((rng.randrange(65536), 0x2000)))
    return out


def independent(name, state, expected):
    """Without the C model: AL from the stated rule (06D3: code > 1Fh kept, else code + 40h with AH cleared; 06FB: the class 0..4),
    SP + 6, BP and the return CS:IP back."""
    memory, cpu, info = state; out = expected[2]
    if expected[0] != 'RETURNED':
        return 0
    if info['entry'] == 0x06D3:
        code = info['arg'] & 0xFF
        ah = (cpu.ax >> 8) if code > 0x1F else ((code + 0x40) >> 8)
        check(out.ax == ((ah << 8) | remap(code)), f'{name}: AX rule')
    else:
        check(out.ax & 0xFF == address_class(info['arg']) and out.ax >> 8 == cpu.ax >> 8, f'{name}: AL class rule')
    check(out.sp == (info['sp'] + 6) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: frame on return')
    return 1


def differential(bodies, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = {e: set() for e in BODIES}; edges = {e: set() for e in BODIES}; stated = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, entry, kw) in enumerate(cs):
        if index % 100 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, entry, **kw)
        expected, actual = compare(name, st, bodies[entry][1], lib)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen[entry] |= expected[3]; edges[entry] |= expected[4]
        if actual[0] == 'ARGUMENT':
            raise ValueError(f'{name}: C stopped ARGUMENT')
        stated += independent(name, st, expected)
    report = dict(cases=len(cs), end_states=stops, independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0)
    for entry, (_, ips) in bodies.items():
        conds = {a: (int(i.op_str, 16), a + i.size) for a, i in ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
        outcomes = {(a, t) for a, ts in conds.items() for t in ts}
        check(only is not None or seen[entry] == set(ips), f'{entry:04X} starts not reached: {sorted(hex(a) for a in set(ips) - seen[entry])}')
        check(only is not None or outcomes <= edges[entry], f'{entry:04X} outcomes not reached: {sorted(outcomes - edges[entry])}')
        report[f'{entry:04X}'] = dict(starts=f'{len(seen[entry])}/{len(ips)}', outcomes=f'{len(outcomes & edges[entry])}/{len(outcomes)}')
    report['seconds'] = round(time.time() - t0, 1)
    return report


MUTANT_CASES = ('06d3-code-00', '06d3-code-1f', '06d3-code-20', '06d3-code-c0', '06d3-code-ff', '06d3-flags-08d7-001f', '06d3-sp-fffa-0020',
                '06fb-addr-48ff', '06fb-addr-4900', '06fb-addr-4cff', '06fb-addr-4d00', '06fb-addr-6eff', '06fb-addr-6f00', '06fb-addr-96ff', '06fb-addr-98ff',
                '06fb-addr-9900', '06fb-addr-b6ff', '06fb-addr-b700', '06fb-flags-0ed7-4a00', 'random0', 'random1', 'random2', 'random3')
MUTANTS = (
    ('06D3: JBE read as JB', [("if ((cpu->flags & 0x41u) != 0u) goto L_06e7; /* +06DD */", "if ((cpu->flags & 0x01u) != 0u) goto L_06e7; /* +06DD */")]),
    ('06D3: CMP against 20h', [("read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x1fu); /* +06D9 */", "read8(ram, cpu->ss, (uint16_t)(cpu->bp + 6)), 0x20u); /* +06D9 */")]),
    ('06D3: ADD 41h', [("arithmetic(cpu, cpu->ax, 0x40u, 0x8000u, 0)); /* +06EC */", "arithmetic(cpu, cpu->ax, 0x41u, 0x8000u, 0)); /* +06EC */")]),
    ('06D3: XOR AH,AH missing', [("    cpu->ax &= 0x00FFu;\n    logical(cpu, 0u, 8u); /* +06EA */", "    logical(cpu, 0u, 8u); /* +06EA */")]),
    ('06D3: ADD as a byte add (flags from AL)', [("arithmetic(cpu, cpu->ax, 0x40u, 0x8000u, 0)); /* +06EC */", "arithmetic(cpu, cpu->ax, 0x40u, 0x80u, 0)); /* +06EC */")]),
    ('06FB: default class 5', [("(uint8_t)(0x4u)); /* +0701 */", "(uint8_t)(0x5u)); /* +0701 */")]),
    ('06FB: first JA read as JAE', [("if ((cpu->flags & 0x41u) == 0u) goto L_0717; /* +0711 */", "if ((cpu->flags & 0x01u) == 0u) goto L_0717; /* +0711 */")]),
    ('06FB: last range ends B700', [("0xb6ffu); /* +0742 */", "0xb700u); /* +0742 */")]),
    ('06FB: ranges 3 and 4 swap classes', [("(uint8_t)(0x2u)); /* +0737 */", "(uint8_t)(0x3u)); /* +0737 */")]),
    ('CMP AF dropped (shared arithmetic)', [("    if (((left ^ right ^ result) & 0x10u) != 0u) cpu->flags |= 0x10u;\n", "")]),
    ('06D3: RETF 2 pops 0', [("    cpu->sp = (uint16_t)(cpu->sp + 2u);\n    return PR_U25_STRINGS_OK; /* +06F8 */", "    return PR_U25_STRINGS_OK; /* +06F8 */")]),
    ('06FB: RETF 2 pops 0', [("    cpu->sp = (uint16_t)(cpu->sp + 2u);\n    return PR_U25_STRINGS_OK; /* +0753 */", "    return PR_U25_STRINGS_OK; /* +0753 */")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-UNDEBUG', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'),
                        str(ROOT / 'tests/recovered/test_overlay_u25_06d3_06fb_cpu.c'), str(mutant_path), '-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies):
    out = []; text0 = (ROOT / 'src/recovered/overlay_u25_string_cores.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = text0
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u25-leaf-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u25_string_cores': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, mlib, quick=True, only=MUTANT_CASES)
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
    with tempfile.TemporaryDirectory(prefix='u25-leaf-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, lib, a.quick)
        if a.native:
            from u25_06d3_06fb_native import native
            report['native'] = native(bodies, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['u25_06d3_06fb_cpu_audit'] = sys.modules[__name__]
    main()
