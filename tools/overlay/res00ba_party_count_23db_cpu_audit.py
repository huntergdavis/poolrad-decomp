#!/usr/bin/env python3
"""RES_00BA_23DB (START.EXE resident 00BA [23DB,2433), the party count) against the original bytes: CPU14, all 1,048,576 RAM bytes, the stop and
the remaining budget, unmasked after every case, plus every instruction start and conditional outcome.

The body zeroes DS:[6814] and DS:[6815], walks the party list from DS:[5D96] through each character's link +104h/+106h, and for each character whose
byte +10Dh is non-zero increments DS:[6814h + CBW(byte +10Eh)]; RETF. A cyclic list never ends: the budget (one unit per instruction, in both runs)
stops it at the same unexecuted instruction. All accesses are flat RAM. The audit places characters, the head and the counters on the frame (DS = SS),
wraps every 16-bit offset sum, and puts segments at the A20 wrap. The original bytes come from the relocated image of
res0899_quit_0000_cpu_audit.load_image (no relocation in the span). Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CHECKER_ROWS = ('RES_00BA_23DB',)
ENTRY, END, RES = 0x23DB, 0x2433, 0x00BA
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0006, 0xFFFC
BUDGET = 100000
SOURCES = ['resident_00ba_party_count_23db_cpu']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'TRANSFER'}
COUNT = 0x6814
HEAD = 0x5D96
ENTRY_RET = A.ENTRY_RET
check = A.check
REGISTERS = A.REGISTERS


def source():
    import res0899_quit_0000_cpu_audit as Q
    image, sites = Q.load_image()
    body = bytes(image[RES * 16 + ENTRY:RES * 16 + END])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(sum(i.size for i in ins) == len(body) == 88 and len(ins) == 29, 'decode covers [23DB,2433): 88 bytes / 29 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(0x2432, '')], 'the only return is RETF at 2432')
    check(not [i for i in ins if i.mnemonic in ('call', 'lcall', 'int', 'in', 'out')], 'no call, interrupt or port')
    check(not [s_ for s_ in sites if RES * 16 + ENTRY <= s_ < RES * 16 + END], 'no relocation inside the span')
    meta = dict(bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), image_offset=RES * 16 + ENTRY, image_sha256=Q.IMAGE_DIGEST)
    return body, {i.address: i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('res_23db_mut.so' if override else 'res_23db.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_res_00ba_party_count_23db_cpu.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]
    lib.pr_res_00ba_party_count_23db_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state):
    memory, cpu, info = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(info['budget'])
    status = lib.pr_res_00ba_party_count_23db_cpu(ctypes.addressof(ram), SIZE, ctypes.byref(out), ctypes.byref(left))
    return STATUS[status], bytes(ram), out, left.value


def run_original(state, body_ips):
    """One budget unit per executed instruction: LIMIT stops before the next instruction with the CPU at it."""
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    memory, cpu, info = state; backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    s = dict(stop=None, left=info['budget'], prev=None); seen = set(); edges = set()

    def hook(em, address, size, user):
        cs = em.reg_read(UC_X86_REG_CS); ip = em.reg_read(UC_X86_REG_IP)
        if cs != info['w_cs']:
            s['stop'] = 'RETURNED' if (cs, ip) == ENTRY_RET else 'TRANSFER'; return em.emu_stop()
        check(ip in body_ips, f'unexpected PC {ip:04x}')
        if s['left'] == 0:
            s['stop'] = 'LIMIT'; return em.emu_stop()
        s['left'] -= 1; seen.add(ip)
        if s['prev'] is not None:
            edges.add((s['prev'], ip))
        s['prev'] = ip
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=info['budget'] + 10)
    check(s['stop'] in ('RETURNED', 'TRANSFER', 'LIMIT'), f'original stop {s["stop"]}')
    out = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return s['stop'], bytes(backing), out, seen, edges, s['left']


def fixture(body, *, chars=((0x3000, 0x0100, 1, 0), (0x3100, 0x0100, 1, 1)), cycle=None, ds=0x1000, ss=0x2000, sp=0x8000, w_cs=W_CS, flags=0x0202,
            regs=None, background=0xA5, counts=(0x37, 0x48), budget=BUDGET, head=None, writes=()):
    """chars: (seg, off, flag byte +10Dh, index byte +10Eh), linked in order through +104h/+106h, the last null unless `cycle` names the index the
    last links back to. The counters DS:[6814]/[6815] start at `counts` (the body zeroes them)."""
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    at = physical(w_cs, ENTRY); m[at:at + len(body)] = body
    m[physical(ds, COUNT)] = counts[0]; m[physical(ds, COUNT + 1)] = counts[1]
    for k, (seg, off, flag, idx) in enumerate(chars):
        nxt = chars[k + 1][:2] if k + 1 < len(chars) else (chars[cycle][:2] if cycle is not None else (0, 0))
        put16(m, seg, (off + 0x104) & 0xFFFF, nxt[1]); put16(m, seg, (off + 0x106) & 0xFFFF, nxt[0])
        m[physical(seg, (off + 0x10D) & 0xFFFF)] = flag & 0xFF; m[physical(seg, (off + 0x10E) & 0xFFFF)] = idx & 0xFF
    h = head if head is not None else (chars[0][:2] if chars else (0, 0))
    put16(m, ds, HEAD, h[1]); put16(m, ds, (HEAD + 2) & 0xFFFF, h[0])
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, ds=ds, ss=ss, sp=sp, chars=chars, cycle=cycle, budget=budget, plain=not writes and cycle is None and head is None)


def compare(label, state, body_ips, lib):
    actual = run_c(lib, state); expected = run_original(state, body_ips)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    check(actual[3] == expected[5], f'{label}: budget left C {actual[3]} original {expected[5]}')
    for name, _ in Cpu._fields_:
        check(getattr(actual[2], name) == getattr(expected[2], name), f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    return expected, actual


def cases(quick=False):
    rng = random.Random(0x00BA23DB)
    out = []

    def add(name, **kw):
        out.append((name, kw))
    C = lambda k, flag=1, idx=0: (0x3000 + 0x40 * k, 0x0100, flag, idx)
    add('empty', chars=())
    add('one', chars=(C(0),)); add('one-flag-0', chars=(C(0, 0),))
    add('three-mixed', chars=(C(0, 1, 0), C(1, 0, 1), C(2, 5, 1)))
    for idx in (0x00, 0x01, 0x02, 0x7F, 0x80, 0xFF, 0xFE):                                         # CBW: the counter address 6814h + signed byte
        add(f'idx-{idx:02x}', chars=(C(0, 1, idx), C(1, 1, idx)))
    for flag in (0x01, 0x80, 0xFF):
        add(f'flag-{flag:02x}', chars=(C(0, flag, 1),))
    add('count-wrap', chars=tuple(C(k, 1, 0) for k in range(260)))                                # INC from FFh to 00h (OF/AF/ZF), 260 nodes
    add('count-to-80', chars=tuple(C(k, 1, 1) for k in range(128)))                               # INC 7Fh -> 80h sets OF
    add('stop-after-inc-80', chars=tuple(C(k, 1, 1) for k in range(128)), budget=2179)      # LIMIT at 241A right after the INC 7Fh -> 80h (OF set; the OR at 23FB overwrites it)
    add('head-null-seg', head=(0x0000, 0x0010), chars=(C(0),))                                    # a null-segment head with a non-zero offset: not null
    for co in (0xFEF2, 0xFEF3, 0xFEFC, 0xFFFF):                                                  # +104h/+106h/+10Dh/+10Eh wrap at 16 bits
        add(f'char-wrap-{co:04x}', chars=((0x3000, co, 1, 0), (0x3100, 0x0100, 1, 1)))
    add('a20-char', chars=((0xFFFF, 0x0100, 1, 0),)); add('a20-ds', ds=0xFFFF, chars=(C(0),))
    for fl in (0x0002, 0x0202, 0x08D7, 0x0ED7, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl); add(f'flags-{fl:04x}-empty', flags=fl, chars=())
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp)
    for k in range(-6, 4):                                                                         # DS = SS: a counter on each frame byte (6814h + idx)
        add(f'counter-on-frame{k:+d}', ds=0x2000, ss=0x2000, sp=(COUNT + 6 - k) & 0xFFFF, chars=(C(0, 1, 0), C(1, 1, 0)), budget=5000)
    for k in range(-6, 4):                                                                         # DS = SS: the party head on each frame byte
        add(f'head-on-frame{k:+d}', ds=0x2000, ss=0x2000, sp=(HEAD + 6 - k) & 0xFFFF, chars=(C(0),), budget=5000)
    for k in (-4, -2, 0, 2):                                                                       # a character's link words on the frame
        add(f'link-on-frame{k:+d}', ss=0x2000, sp=0x8000, chars=((0x2000, (0x8000 + k - 0x104) & 0xFFFF, 1, 0),), budget=5000)
    add('counter-on-link', chars=((0x1000, (COUNT - 0x104) & 0xFFFF, 1, 0),), ds=0x1000, budget=5000)  # the counter byte is the character's link
    add('cycle-1', chars=(C(0),), cycle=0, budget=300); add('cycle-3', chars=(C(0), C(1, 0), C(2)), cycle=1, budget=600)
    for b in list(range(0, 60 if quick else 80, 1 if not quick else 3)) + [100, 120]:
        add(f'budget-{b}', budget=b, chars=(C(0, 1, 0), C(1, 0, 1), C(2, 1, 1)))
    for k in range(40 if quick else 200):
        n = rng.randrange(0, 6)
        add(f'random{k}', chars=tuple((rng.randrange(0x1000, 0x9000), rng.randrange(65536), rng.choice((0, 1, rng.randrange(256))), rng.randrange(256)) for _ in range(n)),
            ds=rng.randrange(0x9000, 0xA000), counts=(rng.randrange(256), rng.randrange(256)), regs=tuple(rng.randrange(65536) for _ in range(8)),
            flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1), background=rng.randrange(256), budget=20000)
    return out


def independent(name, state, expected):
    """Without the C model: the counters (DS:6814h + idx bytes) from the stated rule, and SP/BP/CS:IP on return; plain cases whose characters, head
    and touched counters alias neither the frame nor each other."""
    memory, cpu, info = state; out = expected[2]
    if expected[0] != 'RETURNED' or not info['plain']:
        return 0
    frame = {physical(info['ss'], (info['sp'] - k) & 0xFFFF) for k in range(-4, 7)}
    want = {physical(info['ds'], COUNT): 0, physical(info['ds'], COUNT + 1): 0}
    fields = set()
    for seg, off, flag, idx in info['chars']:
        fields |= {physical(seg, (off + k) & 0xFFFF) for k in (0x104, 0x105, 0x106, 0x107, 0x10D, 0x10E)}
    if fields & (frame | set(want) | {physical(info['ds'], (COUNT + i) & 0xFFFF) for i in range(-128, 128)}):
        return 0
    if any(((off + k) & 0xFFFF) < ((off + 0x104) & 0xFFFF) for seg, off, _, _ in info['chars'] for k in (0x106, 0x10E)):
        return 0
    for seg, off, flag, idx in info['chars']:
        if flag:
            a = physical(info['ds'], (COUNT + (idx - 256 if idx & 0x80 else idx)) & 0xFFFF)
            want[a] = (want.get(a, memory[a]) + 1) & 0xFF
    for a, v in want.items():
        check(expected[1][a] == v, f'{name}: counter at {a:#x} is {expected[1][a]}, rule {v}')
    check(out.sp == (info['sp'] + 4) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: frame on return')
    return 1


def differential(body, body_ips, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); edges = set(); stated = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw) in enumerate(cs):
        if index % 100 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(body, **kw)
        expected, actual = compare(name, st, body_ips, lib)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[3]; edges |= expected[4]
        if actual[0] == 'ARGUMENT':
            raise ValueError(f'{name}: C stopped ARGUMENT')
        stated += independent(name, st, expected)
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(only is not None or seen == set(body_ips), f'starts not reached: {sorted(hex(a) for a in set(body_ips) - seen)}')
    check(only is not None or outcomes <= edges, f'outcomes not reached: {sorted(outcomes - edges)}')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(body_ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('empty', 'one', 'one-flag-0', 'three-mixed', 'idx-80', 'idx-ff', 'flag-80', 'count-wrap', 'count-to-80', 'stop-after-inc-80', 'head-null-seg',
                'char-wrap-fef2', 'flags-08d7', 'flags-08d7-empty', 'counter-on-frame-1', 'cycle-1', 'budget-9', 'budget-30', 'random0', 'random1')
MUTANTS = (
    ('the counters are not zeroed', [("    STEP(0x23E1u); wr8(ram, c->ds, 0x6814u, 0u);", "    STEP(0x23E1u);")]),
    ('the index is zero-extended (no CBW)', [("c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax;", "c->ax = (uint16_t)(uint8_t)c->ax;")]),
    ('the flag byte is +10Ch', [("(uint16_t)(c->di + 0x010Du)", "(uint16_t)(c->di + 0x010Cu)")]),
    ('the index byte is +10Fh', [("(uint16_t)(c->di + 0x010Eu)", "(uint16_t)(c->di + 0x010Fu)")]),
    ('the next link is +100h', [("c->ax = rd16(ram, c->es, (uint16_t)(c->di + 0x0104u));", "c->ax = rd16(ram, c->es, (uint16_t)(c->di + 0x0100u));")]),
    ('the head is DS:5D92', [("c->ax = rd16(ram, c->ds, 0x5D96u);", "c->ax = rd16(ram, c->ds, 0x5D92u);")]),
    ('only the offset word tests null', [("c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp - 2u))); logic16(c, c->ax);", "logic16(c, c->ax);")]),
    ('JE as JNE on the flag', [("        if (!(c->flags & 0x0040u)) {", "        if (c->flags & 0x0040u) {")]),
    ('INC without OF', [("    if (r == 0x80u) f |= 0x0800u;", "    (void)0;")]),
    ('the budget is not charged in the loop body', [("        STEP(0x241Au); les(ram, c, c->ss, (uint16_t)(c->bp - 4u));", "        les(ram, c, c->ss, (uint16_t)(c->bp - 4u));")]),
    ('RETF pops an argument word', [("    STEP(0x2432u); c->ip = pop(ram, c); c->cs = pop(ram, c);", "    STEP(0x2432u); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 2u);")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'),
                        str(ROOT / 'tests/recovered/test_resident_00ba_party_count_23db_cpu.c'), str(mutant_path), '-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(body, body_ips):
    out = []; src = (ROOT / 'src/recovered/resident_00ba_party_count_23db_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='res-23db-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'resident_00ba_party_count_23db_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(body, body_ips, mlib, quick=True, only=MUTANT_CASES)
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
    with tempfile.TemporaryDirectory(prefix='res-23db-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        body, body_ips, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(body, body_ips, lib, a.quick)
        if a.native:
            from res00ba_party_count_23db_native import native
            report['native'] = native(body, lib)
        if a.mutate:
            report['mutation'] = mutation_check(body, body_ips)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['res00ba_party_count_23db_cpu_audit'] = sys.modules[__name__]
    main()
