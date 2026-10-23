#!/usr/bin/env python3
"""RES_00BA_2E03 (START.EXE resident 00BA [2E03,2E46), the character link check) against the original bytes: CPU14, all 1,048,576 RAM bytes and the
stop, unmasked after every case, plus every instruction start and conditional outcome.

The body reads the character far pointer from its frame, ORs the two words of the character's link (+0CCh/+0CEh); a zero link gives 0. Otherwise it
loads the link, takes the byte at link+2Eh as an index, and returns 1 when DS:[558Eh + index*16] is above 1 (unsigned), else 0, RETF 4. All
accesses are flat RAM. The audit places the character, the link target and the table entry on the frame (DS = SS), wraps every 16-bit offset sum,
and puts segments at the A20 wrap. The original bytes come from the relocated image of res0899_quit_0000_cpu_audit.load_image (no relocation in the
span). Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, physical, put16

CHECKER_ROWS = ('RES_00BA_2E03',)
ENTRY, END, RES = 0x2E03, 0x2E46, 0x00BA
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0004, 0xFFF8
SOURCES = ['resident_00ba_link_check_2e03_cpu']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'TRANSFER'}
TABLE = 0x558E
ENTRY_RET = A.ENTRY_RET
check = A.check
REGISTERS = A.REGISTERS


def source():
    import res0899_quit_0000_cpu_audit as Q
    image, sites = Q.load_image()
    body = bytes(image[RES * 16 + ENTRY:RES * 16 + END])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(sum(i.size for i in ins) == len(body) == 67 and len(ins) == 23, 'decode covers [2E03,2E46): 67 bytes / 23 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(0x2E43, '4')], 'the only return is RETF 4 at 2E43')
    check(not [i for i in ins if i.mnemonic in ('call', 'lcall', 'int', 'in', 'out', 'loop')], 'a leaf: no call, interrupt, port or loop')
    check(not [s for s in sites if RES * 16 + ENTRY <= s < RES * 16 + END], 'no relocation inside the span')
    meta = dict(bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), image_offset=RES * 16 + ENTRY, image_sha256=Q.IMAGE_DIGEST)
    return body, {i.address: i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('res_2e03_mut.so' if override else 'res_2e03.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_res_00ba_link_check_2e03_cpu.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(Cpu)]
    lib.pr_res_00ba_link_check_2e03_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu)
    status = lib.pr_res_00ba_link_check_2e03_cpu(ctypes.addressof(ram), SIZE, ctypes.byref(out))
    return STATUS[status], bytes(ram), out


def run_original(state, body_ips, limit=10000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP
    memory, cpu, info = state; backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    s = dict(stop=None, steps=0, prev=None); seen = set(); edges = set()

    def hook(em, address, size, user):
        cs = em.reg_read(UC_X86_REG_CS); ip = em.reg_read(UC_X86_REG_IP)
        if cs != info['w_cs']:
            s['stop'] = 'RETURNED' if (cs, ip) == ENTRY_RET else 'TRANSFER'; return em.emu_stop()
        check(ip in body_ips, f'unexpected PC {ip:04x}')
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


def fixture(body, *, char=(0x3000, 0x0100), link=(0x4000, 0x0200), idx=0x05, tval=0x02, ds=0x1000, ss=0x2000, sp=0x8000, w_cs=W_CS, flags=0x0202,
            regs=None, background=0xA5, nolink=False, les_mirror=True):
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    at = physical(w_cs, ENTRY); m[at:at + len(body)] = body
    lseg, loff = (0, 0) if nolink else link
    put16(m, char[0], (char[1] + 0xCC) & 0xFFFF, loff); put16(m, char[0], (char[1] + 0xCE) & 0xFFFF, lseg)
    if les_mirror and ((char[1] + 0xCC) & 0xFFFF) > 0xFFFC:                                   # declared split (VP i19555): pinned DOSBox-X reads LES's segment
        q = ((char[0] << 4) + ((char[1] + 0xCC) & 0xFFFF) + 2) & (SIZE - 1); m[q] = lseg & 0xFF; m[(q + 1) & (SIZE - 1)] = lseg >> 8   # word at (off+2) mod 64K, Unicorn at off+2 contiguous; the Unicorn
    if not nolink:                                                                             # side gets the same word at both places (native checks wrapped-only)
        m[physical(lseg, (loff + 0x2E) & 0xFFFF)] = idx & 0xFF
        m[physical(ds, (TABLE + ((idx & 0xFF) << 4)) & 0xFFFF)] = tval & 0xFF
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0], char[1], char[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, ds=ds, ss=ss, sp=sp, char=char, link=(lseg, loff))


def compare(label, state, body_ips, lib):
    actual = run_c(lib, state); expected = run_original(state, body_ips)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    for name, _ in Cpu._fields_:
        check(getattr(actual[2], name) == getattr(expected[2], name), f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    return expected, actual


def cases(quick=False):
    rng = random.Random(0x00BA2E03)
    out = []

    def add(name, **kw):
        out.append((name, kw))
    add('no-link', nolink=True)
    for lo, ls in ((0x0001, 0x0000), (0x0000, 0x0001), (0x8000, 0x0000), (0x0000, 0x8000)):      # one link word non-zero: the OR is non-zero
        add(f'half-link-{lo:04x}-{ls:04x}', link=(ls, lo), idx=1, tval=2)
    for tv in (0x00, 0x01, 0x02, 0x7F, 0x80, 0xFF):                                              # JBE: 0/1 give 0, the rest 1 (unsigned)
        add(f'table-{tv:02x}', tval=tv)
    for idx in (0x00, 0x01, 0x0F, 0x10, 0x7F, 0x80, 0xAA, 0xFF):                                  # SHL by 4: DI up to 0FF0h
        add(f'idx-{idx:02x}', idx=idx, tval=idx & 3)
    for co in (0xFF33, 0xFF34, 0xFF35, 0xFFFF):                                                  # +0CCh/+0CEh wrap at 16 bits
        add(f'char-wrap-{co:04x}', char=(0x3000, co))
    add('link-wrap', link=(0x4000, 0xFFD2)); add('link-wrap2', link=(0x4000, 0xFFFF))
    add('a20-char', char=(0xFFFF, 0x0100)); add('a20-link', link=(0xFFFF, 0x0200)); add('a20-ds', ds=0xFFFF)
    add('ds-table-wrap', ds=0x1000, idx=0xFF, tval=5)                                            # 558E + FF0 = 657E
    for fl in (0x0002, 0x0202, 0x08D7, 0x0ED7, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl); add(f'flags-{fl:04x}-nolink', flags=fl, nolink=True)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp)
    for k in range(-4, 8):                                                                        # DS = SS: the table entry (558E + 10h*16) on each frame byte
        add(f'table-on-frame{k:+d}', ds=0x2000, ss=0x2000, sp=(TABLE + 0x100 - k) & 0xFFFF, idx=0x10, tval=2)
    for k in (-4, -3, -2, -1, 0, 2, 4, 6):                                                        # the link's index byte on the frame (link -> stack)
        add(f'linkbyte-on-frame{k:+d}', ss=0x2000, sp=0x8000, link=(0x2000, (0x8000 + k - 0x2E) & 0xFFFF), tval=3)
    for k in (-2, 0, 4, 6):                                                                       # the character's link words on the frame (character -> stack)
        add(f'char-on-frame{k:+d}', ss=0x2000, sp=0x8000, char=(0x2000, (0x8000 + k - 0xCC) & 0xFFFF))
    for k in range(40 if quick else 200):
        add(f'random{k}', char=(rng.randrange(65536), rng.randrange(65536)), link=(rng.randrange(65536), rng.randrange(65536)), idx=rng.randrange(256),
            tval=rng.choice((0, 1, 2, rng.randrange(256))), ds=rng.randrange(65536), nolink=rng.random() < 0.15,
            regs=tuple(rng.randrange(65536) for _ in range(8)), flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)),
            sp=rng.randrange(SP_MIN, SP_MAX + 1), background=rng.randrange(256))
    return out


def independent(name, state, expected):
    """Without the C model: AL from the stated rule, AH, and SP/BP/CS:IP on return; skipped when the inputs alias the frame or each other."""
    memory, cpu, info = state; out = expected[2]
    if expected[0] != 'RETURNED':
        return 0
    frame = {physical(info['ss'], (info['sp'] - k) & 0xFFFF) for k in range(-8, 5)}
    cseg, coff = info['char']
    words = [physical(cseg, (coff + k) & 0xFFFF) for k in range(0xCC, 0xD0)]
    if set(words) & frame:
        return 0
    loff = memory[words[0]] | memory[words[1]] << 8; lseg = memory[words[2]] | memory[words[3]] << 8
    if loff | lseg:
        b = physical(lseg, (loff + 0x2E) & 0xFFFF)
        if b in frame:
            return 0
        t = physical(info['ds'], (TABLE + (memory[b] << 4)) & 0xFFFF)
        if t in frame:
            return 0
        want = 1 if memory[t] > 1 else 0
    else:
        want = 0
    check(out.ax & 0xFF == want, f'{name}: AL {out.ax & 0xFF} rule {want}')
    check(out.sp == (info['sp'] + 8) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: frame on return')
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


MUTANT_CASES = ('no-link', 'half-link-0001-0000', 'half-link-0000-0001', 'table-00', 'table-01', 'table-02', 'table-80', 'idx-10', 'idx-ff',
                'char-wrap-ff33', 'flags-08d7', 'flags-08d7-nolink', 'flags-0ed7', 'linkbyte-on-frame-1', 'char-on-frame+0', 'random0', 'random1', 'random2')
MUTANTS = (
    ('only the low link word is tested', [("c->ax = (uint16_t)(c->ax | rd16(ram, c->es, (uint16_t)(c->di + 0x00CEu)));", "(void)0;")]),
    ('the index byte is at link+2Fh', [("(uint16_t)(c->di + 0x2Eu)", "(uint16_t)(c->di + 0x2Fu)")]),
    ('the table stride is 8 (SHL 3)', [("c->cx = (uint16_t)((c->cx & 0xFF00u) | 4u);", "c->cx = (uint16_t)((c->cx & 0xFF00u) | 3u);")]),
    ('signed compare (JLE) instead of JBE', [("if ((c->flags & 0x0041u) != 0u)", "if ((c->flags & 0x0040u) != 0u || (((c->flags >> 7u) ^ (c->flags >> 11u)) & 1u))")]),
    ('OR leaves AF set', [("static void logic16(cpu_t *c, uint16_t r)                                   /* OR/XOR word: CF=OF=AF=0 (DOSBox-X), ZF/SF of the word, PF low byte */\n{ uint16_t f = parity((uint8_t)r);",
                           "static void logic16(cpu_t *c, uint16_t r)                                   /* OR/XOR word: CF=OF=AF=0 (DOSBox-X), ZF/SF of the word, PF low byte */\n{ uint16_t f = (uint16_t)(parity((uint8_t)r) | 0x10u);")]),
    ('CMP PF from the whole word', [("uint8_t r = (uint8_t)(a - b); uint16_t f = parity(r);", "uint8_t r = (uint8_t)(a - b); uint16_t f = parity((uint8_t)(r ^ 1u));")]),
    ('the local is not cleared on the zero path', [("        wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 0u);                                        /* 2E39 MOV BYTE [BP-1],0 */\n    } else {",
                                                    "        (void)0;\n    } else {")]),
    ('RETF pops 2 argument bytes', [("c->sp = (uint16_t)(c->sp + 4u);               /* 2E43 RETF 4 */", "c->sp = (uint16_t)(c->sp + 2u);               /* 2E43 RETF 4 */")]),
    ('LES reads its segment word contiguously at EA+2 (the Unicorn behaviour, not pinned DOSBox-X)',
     [("{ uint16_t o = rd16(ram, seg, off); c->es = rd16(ram, seg, (uint16_t)(off + 2u)); c->di = o; }",
       "{ size_t p = physical(seg, off); uint16_t o = rd16(ram, seg, off); c->es = (uint16_t)(ram[(p + 2u) & MASK] | ((uint16_t)ram[(p + 3u) & MASK] << 8u)); c->di = o; }")]),
    ('the character pointer is not re-read (LES reuses ES:DI)', [("        les(ram, c, c->ss, (uint16_t)(c->bp + 6u));                                         /* 2E18 LES DI,[BP+6] */\n", "")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'),
                        str(ROOT / 'tests/recovered/test_resident_00ba_link_check_2e03_cpu.c'), str(mutant_path), '-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(body, body_ips):
    out = []; src = (ROOT / 'src/recovered/resident_00ba_link_check_2e03_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) >= 1, f'mutant text occurs: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='res-2e03-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'resident_00ba_link_check_2e03_cpu': path})
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
    with tempfile.TemporaryDirectory(prefix='res-2e03-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        body, body_ips, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(body, body_ips, lib, a.quick)
        if a.native:
            from res00ba_link_check_2e03_native import native
            report['native'] = native(body, lib)
        if a.mutate:
            report['mutation'] = mutation_check(body, body_ips)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['res00ba_link_check_2e03_cpu_audit'] = sys.modules[__name__]
    main()
