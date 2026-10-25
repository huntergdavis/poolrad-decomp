#!/usr/bin/env python3
"""OVL_0025_18EB (GAME.OVR unit 0x25 [18EB,1951), the ECL word compare) against the original bytes, composed with the original SYSTEM
FillChar 0AF8:16A8 it far-calls: CPU14, all 1,048,576 RAM bytes and the stop, flags UNMASKED at the RETF, plus every instruction start and
conditional outcome. The body clears the six compare-flag bytes DS:[6F78..6F7D] through FillChar, then sets each one whose unsigned relation
of a ([BP+8]) to b ([BP+6]) holds. The body bytes come from GAME.OVR (hash-checked, the +18FD segment fixup confirmed) and FillChar from the
loader-expanded START.EXE; the fixture writes the FillChar segment it uses into +18FD, as the loader's fixup does. Media stay under ignored
resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_0025_18EB',)
ENTRY, END, FILL_SITE = 0x18EB, 0x1951, 0x18FA
W_CS, SYS_CS = 0x6000, 0x0AF8
FILL, FILL_END, FILL_SHA = 0x16A8, 0x16BC, 'f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7'
FLAGS_AT = 0x6F78
SP_MIN, SP_MAX = 0x000E, 0xFFF8
SOURCES = ['overlay_u25_compare_words_18eb_cpu', 'system_runtime_leaves_cpu']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'TRANSFER'}
ENTRY_RET = A.ENTRY_RET
check = A.check
REGISTERS = A.REGISTERS


def source():
    from overlay_inventory import find_units, parse_mz
    from resident_loader_map import unpack
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'START.EXE / GAME.OVR provenance differs')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    u = units[0x25]; code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    body = bytes(code[ENTRY:END]); md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = list(md.disasm(body, ENTRY))
    check(sum(i.size for i in ins) == len(body) == 102 and len(ins) == 36, 'decode covers [18EB,1951): 102 bytes / 36 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(0x194E, '4')], 'the only return is RETF 4 at 194E')
    check([(i.address, i.mnemonic, i.op_str) for i in ins if i.mnemonic in ('call', 'lcall', 'int', 'in', 'out', 'loop')]
          == [(FILL_SITE, 'lcall', '0xaf8, 0x16a8')], 'the one call is LCALL 0AF8:16A8 at +18FA')
    check([f for f in u['fixups'] if ENTRY <= f < END] == [FILL_SITE + 3], 'the only fixup in the span is the LCALL segment word at +18FD')
    image = bytes(unpack(exe)[0]); fill = image[physical(SYS_CS, FILL):physical(SYS_CS, FILL_END)]
    check(sha256(fill).hexdigest() == FILL_SHA, 'SYSTEM FillChar differs')
    fins = list(md.disasm(fill, FILL))
    check(len(fins) == 7 and fins[-1].mnemonic == 'retf' and fins[-1].op_str == '8', 'FillChar: 7 instructions ending RETF 8')
    meta = dict(bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), file_offset=u['ovr_offset'] + ENTRY,
                fillchar_sha256=FILL_SHA, fillchar_instructions=len(fins))
    ips = {('body', i.address): i for i in ins}; ips.update({('sys', i.address): i for i in fins})
    return (body, fill), ips, meta


def library(directory, override=None):
    path = Path(directory) / ('u25_18eb_mut.so' if override else 'u25_18eb.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_compare_words_18eb_cpu.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_u25_compare_words_18eb_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu)
    status = lib.pr_u25_compare_words_18eb_cpu(ctypes.addressof(ram), SIZE, ctypes.byref(out))
    return STATUS[status], bytes(ram), out


def run_original(state, ips, limit=10000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    memory, cpu, info = state; backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    s = dict(stop=None, steps=0, prev=None); seen = set(); edges = set()
    kinds = {info['w_cs']: 'body', info['sys']: 'sys'}

    def hook(em, address, size, user):
        cs = em.reg_read(UC_X86_REG_CS); ip = em.reg_read(UC_X86_REG_IP); key = (kinds.get(cs), ip)
        if key not in ips:                                       # left the two original spans: the entry frame's return, or a transfer
            s['stop'] = 'RETURNED' if (cs, ip) == ENTRY_RET else 'TRANSFER'; return em.emu_stop()
        seen.add(key)
        if s['prev'] is not None:
            edges.add((s['prev'], key))
        s['prev'] = key; s['steps'] += 1
        if s['steps'] > limit:
            s['stop'] = 'RUNAWAY'; em.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=limit + 10)
    check(s['stop'] in ('RETURNED', 'TRANSFER'), f'original stop {s["stop"]}')
    out = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return s['stop'], bytes(backing), out, seen, edges


def fixture(src, *, a=0x1234, b=0x1234, prior=None, ss=0x2000, sp=0x8000, ds=0x5000, w_cs=W_CS, sys=SYS_CS, flags=0x0202, regs=None,
            background=0xA5):
    body, fill = src
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    at = physical(w_cs, ENTRY); m[at:at + len(body)] = body; put16(m, w_cs, FILL_SITE + 3, sys)
    at = physical(sys, FILL); m[at:at + len(fill)] = fill
    for k, v in enumerate(prior or ()):
        m[physical(ds, FLAGS_AT + k)] = v & 0xFF
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0], b, a)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, sys=sys, ds=ds, ss=ss, sp=sp, a=a, b=b)


def compare(label, state, ips, lib):
    actual = run_c(lib, state); expected = run_original(state, ips)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    for name, _ in Cpu._fields_:
        check(getattr(actual[2], name) == getattr(expected[2], name), f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    return expected, actual


def touched(info):
    """Physical bytes the run reads or writes outside code: the flag bytes and the stack from entry SP-14 to SP+7."""
    return ({physical(info['ds'], FLAGS_AT + k) for k in range(6)} |
            {physical(info['ss'], (info['sp'] + k) & 0xFFFF) for k in range(-14, 8)})


def code_bytes(info):
    return ({physical(info['w_cs'], o) for o in range(ENTRY, END)} | {physical(info['sys'], o) for o in range(FILL, FILL_END)})


PAIRS = ((0x0000, 0x0000), (0x0000, 0x0001), (0x0001, 0x0000), (0xFFFF, 0x0000), (0x0000, 0xFFFF), (0x8000, 0x7FFF), (0x7FFF, 0x8000),
         (0x1234, 0x1234), (0xFFFF, 0xFFFF), (0x8000, 0x8000), (0x0010, 0x0001), (0x0001, 0x0010), (0x00FF, 0x0100), (0x0100, 0x00FF))


def cases(quick=False):
    rng = random.Random(0x002518EB)
    out = []

    def add(name, **kw):
        out.append((name, kw))
    for a, b in PAIRS:                                                                         # every relation, each CMP's flags at the edges
        add(f'pair-{a:04x}-{b:04x}', a=a, b=b, prior=(0xFF, 0x00, 0x5A, 0x01, 0xA5, 0x7F))
    for fl in (0x0002, 0x0202, 0x0602, 0x08D7, 0x0ED7, 0x0003, 0x7602):                         # DF set on entry: FillChar's CLD
        add(f'flags-{fl:04x}', flags=fl, a=0x4000, b=0x3FFF); add(f'flags-{fl:04x}-eq', flags=fl, a=0x55AA, b=0x55AA)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, a=3, b=9)
    for s in range(0x6F68, 0x6F8C):                                                            # DS = SS: the clear and the stores reach the frame
        add(f'alias-{s:04x}', ds=0x2000, ss=0x2000, sp=s, a=0x0101, b=0x0001, prior=(9, 9, 9, 9, 9, 9))
    for s in (0x6F70, 0x6F72, 0x6F74, 0x6F76, 0x6F78):
        add(f'alias-eq-{s:04x}', ds=0x2000, ss=0x2000, sp=s, a=0x0000, b=0x0100)
    add('a20-flags', ds=0xFFFF, a=1, b=2); add('a20-stack', ss=0xFFFF, sp=0x8000, a=2, b=1)
    add('code-seg-ds', ds=0x6000, a=7, b=7)                                                  # DS = CS: the flag bytes sit in the unit, away from the body
    for k in range(40 if quick else 300):
        while True:
            kw = dict(a=rng.randrange(65536), regs=tuple(rng.randrange(65536) for _ in range(8)), prior=tuple(rng.randrange(256) for _ in range(6)),
                      flags=rng.choice((2, 0x202, 0x08D7, 0x0602, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1),
                      background=rng.randrange(256), ds=rng.randrange(65536), ss=rng.randrange(65536))
            kw['b'] = rng.choice((kw['a'], rng.randrange(65536), (kw['a'] + 1) & 0xFFFF, (kw['a'] - 1) & 0xFFFF))
            info = dict(w_cs=W_CS, sys=SYS_CS, ds=kw['ds'], ss=kw['ss'], sp=kw['sp'])
            if not touched(info) & code_bytes(info):                                           # code is immutable (header precondition)
                break
        add(f'random{k}', **kw)
    return out


def relations(a, b):
    return (a == b, a != b, a < b, a > b, a <= b, a >= b)


def independent(name, state, expected):
    """Without the C model: the six bytes from the stated relations, AX = a, CX = 0, DI = 6F7E, ES = DS, DF clear, SP+8, BP back;
    skipped when the flag bytes share a byte with the stack range the call uses."""
    memory, cpu, info = state; out = expected[2]; ram = expected[1]
    if expected[0] != 'RETURNED':
        return 0
    flags = {physical(info['ds'], FLAGS_AT + k) for k in range(6)}
    if flags & {physical(info['ss'], (info['sp'] + k) & 0xFFFF) for k in range(-14, 8)}:
        return 0
    want = [1 if r else 0 for r in relations(info['a'], info['b'])]
    check([ram[physical(info['ds'], FLAGS_AT + k)] for k in range(6)] == want, f'{name}: flag bytes {want}')
    check(out.ax == info['a'] and out.cx == 0 and out.di == 0x6F7E and out.es == info['ds'] and not out.flags & 0x0400, f'{name}: AX/CX/DI/ES/DF')
    check(out.sp == (info['sp'] + 8) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: frame on return')
    return 1


def differential(src, ips, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); edges = set(); stated = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw) in enumerate(cs):
        if index % 100 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(src, **kw)
        expected, actual = compare(name, st, ips, lib)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[3]; edges |= expected[4]
        if actual[0] in ('ARGUMENT', 'CODE'):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        stated += independent(name, st, expected)
    conds = {k: ((k[0], int(i.op_str, 16)), (k[0], k[1] + i.size)) for k, i in ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(k, t) for k, ts in conds.items() for t in ts}
    check(only is not None or seen == set(ips), f'starts not reached: {sorted(set(ips) - seen)}')
    check(only is not None or outcomes <= edges, f'outcomes not reached: {sorted(outcomes - edges)}')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('pair-0000-0000', 'pair-0000-0001', 'pair-0001-0000', 'pair-8000-7fff', 'pair-7fff-8000', 'pair-0010-0001', 'pair-00ff-0100',
                'flags-0602', 'flags-08d7-eq', 'alias-6f6e', 'alias-6f72', 'alias-6f74', 'alias-6f7a', 'alias-6f80', 'alias-6f84', 'random0', 'random1', 'random2')
MUTANTS = (
    ('JBE taken on CF only (above when equal)', [("case 3: return cf || zf;", "case 3: return cf;")]),
    ('JAE read as signed', [("case 2: return !cf;", "case 2: return ((f & 0x0080u) != 0) == ((f & 0x0800u) != 0);")]),
    ('the stores start at 6F79', [("(uint16_t)(0x6F78u + (unsigned)k), 1u)", "(uint16_t)(0x6F79u + (unsigned)k), 1u)")]),
    ('FillChar clears 5 bytes', [("c->ax = 0x0006u; push(ram, c, c->ax);", "c->ax = 0x0005u; push(ram, c, c->ax);")]),
    ('FillChar not called (no clear)', [("(void)pr_system_fillchar_16a8(ram, ram_size, &state);",
                                         "(void)ram_size; state.ip = 0x18FFu; state.cs = caller_cs; state.sp = (uint16_t)(state.sp + 12u);")]),
    ('the operands read once, before the loop', [("c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));", "if (k == 0) c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));"),
                                                 ("cmp16(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + 6u)));", "{ static uint16_t bb; if (k == 0) bb = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); cmp16(c, c->ax, bb); }")]),
    ('CMP leaves AF clear', [("    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;\n", "")]),
    ('CMP OF from the result sign only', [("if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;", "if ((a ^ r) & 0x8000u) f |= 0x0800u;")]),
    ('RETF pops 2 argument bytes', [("c->sp = (uint16_t)(c->sp + 4u);               /* 194E RETF 4 */", "c->sp = (uint16_t)(c->sp + 2u);               /* 194E RETF 4 */")]),
    ('MOV AL,0 clears AH too', [("c->ax &= 0xFF00u; push(ram, c, c->ax);", "c->ax = 0u; push(ram, c, c->ax);")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-UNDEBUG', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'),
                        str(ROOT / 'tests/recovered/test_overlay_u25_compare_words_18eb_cpu.c'), str(mutant_path),
                        str(ROOT / 'src/recovered/system_runtime_leaves_cpu.c'), '-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(src, ips):
    out = []; text0 = (ROOT / 'src/recovered/overlay_u25_compare_words_18eb_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = text0
        for old, new in edits:
            check(text.count(old) >= 1, f'mutant text occurs: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u25-18eb-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u25_compare_words_18eb_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(src, ips, mlib, quick=True, only=MUTANT_CASES)
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
    with tempfile.TemporaryDirectory(prefix='u25-18eb-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        src, ips, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(src, ips, lib, a.quick)
        if a.native:
            from u25_compare_words_18eb_native import native
            report['native'] = native(src, lib)
        if a.mutate:
            report['mutation'] = mutation_check(src, ips)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['u25_compare_words_18eb_cpu_audit'] = sys.modules[__name__]
    main()
