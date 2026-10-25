#!/usr/bin/env python3
"""OVL_007F_0130 and OVL_007F_0185 (GAME.OVR unit 0x7F entries 10 and 11, the div 5 leaves) against the original bytes.

The unit's bytes [0130,01DB) come from GAME.OVR (ignored resources/) and run at a test code segment; there is no call and no relocation. Entry 10 writes the
record behind DS:[5D92] through the bus (the A0000..AFFFF aperture goes to the independent Python EGA in the oracle, pr_ega in the model); entry 11 writes
DS:[67F4..6807] flat. Compared after every case, unmasked: CPU14, ALL 1,048,576 RAM bytes, the EGA state and the ordered VRAM write operands, the stop and the
remaining budget (one unit per instruction). FLAGS are exact except where the last FLAGS writer is a word DIV or a non-zero SHL: there the declared pinned
DOSBox-X split applies (tools/overlay/pinned_div_flags.py, pinned_shift_flags.py; counted per rule), never a mask. Every case's world is built from its own
name (PM i22687). Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from pinned_shift_flags import LastFlagWriter, declared_flags_split, split_used, SPLIT_BY_RULE
from res00ba_display_1693_cpu_audit import PythonEga
from res0709_printer_0626_cpu_audit import Cpu, Ega, Bus, Env, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE, REGISTERS, physical, put16, check
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_007F_0130', 'OVL_007F_0185')
SPAN = (0x0130, 0x01DB)
ENTRIES = {0x0130: dict(end=0x0185, insns=31, size=85, fn='pr_u7f_div5_0130_cpu'), 0x0185: dict(end=0x01DB, insns=34, size=86, fn='pr_u7f_div5_0185_cpu')}
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'TRANSFER'}
W, DS, SS = 0x3000, 0x4000, 0x6000
RET = (0x9000, 0x8000)                      # (CS, IP) of the entry frame's return address
SP_MIN, SP_MAX = 0x0004, 0xFFFA
BUDGET = 1000
REC = (0x5000, 0x0100)
SOURCES = ['overlay_u7f_div5_leaves_cpu', 'memory_bus', 'ega_hw']


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x7F)
    base = unit['ovr_offset']; blob = ovr[base + SPAN[0]:base + SPAN[1]]
    check(not [f for f in unit['fixups'] if SPAN[0] <= f < SPAN[1]], 'no relocation in [0130,01DB)')
    decoded = {}; meta = {}
    for entry, e in ENTRIES.items():
        body = blob[entry - SPAN[0]:e['end'] - SPAN[0]]
        ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, entry))
        check(len(ins) == e['insns'] and sum(i.size for i in ins) == e['size'], f'+{entry:04X}: {e["size"]} bytes / {e["insns"]} instructions')
        check(not [i for i in ins if i.mnemonic in ('call', 'lcall', 'int', 'into', 'in', 'out')], f'+{entry:04X}: no call, interrupt or port')
        check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(e['end'] - 3, '2')], f'+{entry:04X}: the only return is RETF 2 at its end')
        check(sum(1 for i in ins if i.mnemonic == 'div') == 2, f'+{entry:04X}: two word DIVs')
        decoded[entry] = {i.address: i for i in ins}
        meta[f'{entry:04X}'] = dict(bytes=e['size'], instructions=e['insns'], sha256=sha256(body).hexdigest())
    return blob, decoded, meta


def library(directory, override=None):
    path = Path(directory) / ('u7fd5_mut.so' if override else 'u7fd5.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path))
    for e in ENTRIES.values():
        f = getattr(lib, e['fn']); f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]; f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def fixture(name, blob, entry, *, value=0x1234, rec=REC, sp=0x8000, flags=0x0202, ds=DS, ss=SS, regs=None, writes=(), w=W, ret=RET, fill=True, budget=BUDGET):
    """The case's world, built from its own name: background bytes, the code, the active-character pointer DS:[5D92], the 67F4 block, the frame."""
    rng = random.Random(f'OVL_007F_div5/{name}')
    m = bytearray(rng.randbytes(SIZE)) if fill else bytearray(SIZE)
    for k, b in enumerate(blob):
        m[physical(w, SPAN[0] + k)] = b
    put16(m, ds, 0x5D92, rec[1]); put16(m, ds, 0x5D94, rec[0])
    for off in (0x88, 0x8A, 0x8C, 0x8E, 0x90):
        put16(m, rec[0], (rec[1] + off) & 0xFFFF, rng.randrange(65536))
    for k, v in enumerate((ret[1], ret[0], value & 0xFFFF)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    for seg, off, data in writes:
        for j, v in enumerate(data):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    r = regs or tuple(rng.randrange(65536) for _ in range(8))
    c = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w, entry, ds, r[7], ss, (flags | 0x0002) & ~0x0100)
    return m, c, dict(name=name, entry=entry, value=value & 0xFFFF, rec=rec, sp=sp, ds=ds, ss=ss, budget=budget, plain=not writes)


def run_original(st, decoded):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    m, c, info = st
    body = decoded[info['entry']]
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); device = PythonEga()

    def vread(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def vwrite(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for base in (0, SIZE):
        uc.mem_map_ptr(base, 0xA0000, 7, ctypes.addressof(backing)); uc.mmio_map(base + 0xA0000, 65536, vread, None, vwrite, None)
        uc.mem_map_ptr(base + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, left=info['budget'], prev=None); seen = set(); edges = set(); writer = LastFlagWriter()

    def hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        writer.finish(uc)
        if cs != c.cs or ip not in body:
            s['stop'] = 'RETURNED' if (cs, ip) == RET else 'TRANSFER'; return uc.emu_stop()
        if s['left'] == 0:
            s['stop'] = 'LIMIT'; return uc.emu_stop()
        s['left'] -= 1; seen.add(ip)
        if s['prev'] is not None:
            edges.add((s['prev'], ip))
        s['prev'] = ip
        writer.step(uc, address)
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=10 * info['budget'] + 100)
    writer.finish(uc)
    check(s['stop'] is not None, f'{info["name"]}: the original did not stop')
    cpu = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return s['stop'], bytes(backing), device.writes, device.packed(), cpu, s['left'], seen, edges, writer


def run_c(lib, st):
    m, c, info = st
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); ega = Ega(); lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), 0x31 + p * 0x27, 65536)
    writes = []
    env = Env(None, OUT_FN(lambda *a: 0), IN_FN()); bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(lambda ctx, off, v: writes.append((off, v))), None)
    cpu = Cpu.from_buffer_copy(c); left = ctypes.c_size_t(info['budget'])
    status = getattr(lib, ENTRIES[info['entry']]['fn'])(ctypes.byref(bus), SIZE, ctypes.byref(cpu), ctypes.byref(left))
    return STATUS[status], bytes(ram), writes, bytes(ega), cpu, left.value


def compare(st, decoded, lib):
    name = st[2]['name']
    estop, eram, ewr, eega, ecpu, eleft, seen, edges, writer = run_original(st, decoded)
    astop, aram, awr, aega, acpu, aleft = run_c(lib, st)
    check(astop == estop, f'{name}: stop C {astop} original {estop}')
    check(aleft == eleft, f'{name}: budget left C {aleft} original {eleft}')
    for n, _ in Cpu._fields_:
        if n == 'flags':
            problem = declared_flags_split(acpu.flags, ecpu.flags, writer)
            check(problem is None, f'{name}: {problem}')
            split_used(acpu.flags, ecpu.flags, writer)
            continue
        check(getattr(acpu, n) == getattr(ecpu, n), f'{name}: {n} C {getattr(acpu, n):04x} original {getattr(ecpu, n):04x}')
    if aram != eram:
        diffs = [i for i in range(SIZE) if aram[i] != eram[i]]
        raise ValueError(f'{name}: RAM {[(hex(i), aram[i], eram[i]) for i in diffs[:8]]} ({len(diffs)} bytes)')
    check(awr == ewr, f'{name}: VRAM operands ({len(awr)}/{len(ewr)})')
    check(aega == eega, f'{name}: EGA state')
    return estop, eram, ecpu, seen, edges


def statement(st, stop, ram, cpu):
    """Without the C model, on plain RETURNED cases whose written words alias neither the frame nor DS:[5D92]: the five words and the return."""
    m, c, info = st
    if stop != 'RETURNED' or not info['plain']:
        return 0
    v = info['value']; word = lambda seg, off: ram[physical(seg, off & 0xFFFF)] | (ram[physical(seg, (off + 1) & 0xFFFF)] << 8)
    if info['entry'] == 0x0130:
        seg, off = info['rec']
        targets = [physical(seg, (off + k) & 0xFFFF) for k in range(0x88, 0x92)]
        frame = {physical(info['ss'], (info['sp'] + k) & 0xFFFF) for k in range(-4, 6)} | {physical(info['ds'], 0x5D92 + k) for k in range(4)}
        if frame & set(targets) or 0xA0000 <= min(targets) < 0xB0000 or 0xA0000 <= max(targets) < 0xB0000:
            return 0
        if any(((off + k) & 0xFFFF) == 0xFFFF for k in (0x88, 0x8A, 0x8C, 0x8E, 0x90)):     # a word straddling offset FFFFh: differential only
            return 0
        want = {0x88: 0, 0x8A: 0, 0x8C: 0, 0x8E: v % 5, 0x90: v // 5}
        for k, x in want.items():
            check(word(seg, off + k) == x, f'{info["name"]}: record +{k:02X} = {word(seg, off + k):04x}, rule {x:04x}')
    else:
        frame = {physical(info['ss'], (info['sp'] + k) & 0xFFFF) for k in range(-4, 6)}
        if frame & {physical(info['ds'], 0x67F4 + k) for k in range(0x14)}:
            return 0
        want = [0] * 6 + [v % 5, 0, v // 5, 0]
        for k, x in enumerate(want):
            check(word(info['ds'], 0x67F4 + 2 * k) == x, f'{info["name"]}: DS:{0x67F4 + 2 * k:04X} = {word(info["ds"], 0x67F4 + 2 * k):04x}, rule {x:04x}')
    check((cpu.cs, cpu.ip) == RET and cpu.sp == (info['sp'] + 6) & 0xFFFF and cpu.bp == c.bp, f'{info["name"]}: RETF 2 back with SP+6 and BP restored')
    return 1


def cases(quick=False):
    out = []

    def add(name, entry, **kw):
        out.append((name, entry, kw))
    for entry in ENTRIES:
        t = f'{entry:04x}'
        for v in (0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF, 12345, 0x0F0F):
            add(f'{t}-value-{v:04x}', entry, value=v)
        for fl in (0x0002, 0x0202, 0x0ED7, 0x08D7, 0x0003, 0x0A87, 0x0656, 0x0001):
            add(f'{t}-flags-{fl:04x}', entry, flags=fl, value=7)
        for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
            add(f'{t}-sp-{sp:04x}', entry, sp=sp, value=0x0103)
        add(f'{t}-zero-background', entry, fill=False, value=0x2222)
        for b in range(0, 140 if entry == 0x0130 else 150, 1 if not quick else 3):
            add(f'{t}-budget-{b}', entry, budget=b, value=0x0233)
        for k in range(8 if quick else 30):
            rng = random.Random(f'OVL_007F_div5/{t}-random{k}')
            add(f'{t}-random{k}', entry, value=rng.randrange(65536), flags=rng.randrange(65536) & 0x0ED7, sp=rng.randrange(SP_MIN, SP_MAX + 1),
                rec=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)), regs=tuple(rng.randrange(65536) for _ in range(8)))
    t = '0130'
    for rec in ((0x5000, 0xFF70), (0x5000, 0xFF6F), (0x5000, 0xFF77), (0xFFFF, 0x0100), (0xFFF8, 0x0F70), (0x0000, 0x0000)):
        add(f'{t}-rec-{rec[0]:04x}-{rec[1]:04x}', 0x0130, rec=rec, value=0x4321)
    for rec in ((0xA000, 0x0100), (0xA000, 0xFF70), (0x9FFF, 0x0080), (0xAFFF, 0x0078), (0x9FF8, 0x0070)):
        add(f'{t}-ega-{rec[0]:04x}-{rec[1]:04x}', 0x0130, rec=rec, value=0x4321)
    for k in range(-0x0E, 0x08):                     # the record's words over each byte of the frame: [BP-2..BP-1] (the counter), saved BP, return address, the value
        add(f'{t}-rec-on-frame{k:+03x}', 0x0130, rec=(SS, (0x8000 + k - 0x88) & 0xFFFF), sp=0x8000, value=0x0E0E)
    for k in range(-0x0A, 0x04):                     # the record's words over DS:[5D92] itself: a later LES reads the rewritten pointer
        add(f'{t}-rec-on-pointer{k:+03x}', 0x0130, rec=(DS, (0x5D92 + k - 0x88) & 0xFFFF), value=0x0A0A)
    t = '0185'
    for k in range(-0x18, 0x08, 1 if not quick else 2):     # DS = SS and the 67F4 block over each byte of the frame
        add(f'{t}-block-on-frame{k:+03x}', 0x0185, ds=SS, sp=(0x67F4 + 0x14 + k) & 0xFFFF, value=0x0B0B)
    for ds in (0xFFFF, 0xF980, 0x0000):                     # the block across the A20 wrap and at the bottom of memory
        add(f'{t}-ds-{ds:04x}', 0x0185, ds=ds, value=0x5151)
    return out


def differential(blob, decoded, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; stated = 0; seen = {e: set() for e in ENTRIES}; edges = {e: set() for e in ENTRIES}
    for k in SPLIT_BY_RULE:
        SPLIT_BY_RULE[k] = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, entry, kw) in enumerate(cs):
        if index % 50 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(name, blob, entry, **kw)
        stop, ram, cpu, s, e = compare(st, decoded, lib)
        stops[stop] = stops.get(stop, 0) + 1; seen[entry] |= s; edges[entry] |= e
        stated += statement(st, stop, ram, cpu)
    report = dict(cases=len(cs), stops=dict(sorted(stops.items())), independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0,
                  declared_splits=dict(SPLIT_BY_RULE), seconds=round(time.time() - t0, 1))
    for entry, body in decoded.items():
        conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
        outcomes = {(a, t) for a, ts in conds.items() for t in ts}
        check(only is not None or seen[entry] == set(body), f'+{entry:04X}: starts not reached {sorted(hex(a) for a in set(body) - seen[entry])}')
        check(only is not None or outcomes <= edges[entry], f'+{entry:04X}: outcomes not reached {sorted(outcomes - edges[entry])}')
        report[f'starts_{entry:04X}'] = f'{len(seen[entry] & set(body))}/{len(body)}'; report[f'outcomes_{entry:04X}'] = f'{len(outcomes & edges[entry])}/{len(outcomes)}'
    return report


MUTANT_CASES = tuple(f'{t}-{s}' for t in ('0130', '0185') for s in ('value-0000', 'value-0005', 'value-0006', 'value-0007', 'value-ffff', 'value-3039', 'flags-0ed7',
                                                                    'sp-0004', 'budget-0', 'budget-6', 'budget-9', 'budget-12', 'budget-21', 'budget-84', 'budget-87', 'budget-96')) + \
               ('0130-rec-on-frame-02', '0130-rec-on-frame+02', '0130-rec-on-pointer+00', '0130-rec-on-pointer-08', '0130-budget-15', '0185-budget-18', '0130-ega-a000-0100', '0130-rec-5000-ff77', '0185-block-on-frame-16', '0185-block-on-frame-04')
MUTANTS = (
    ('0130 clears from +86h', [("bwr16(bus, c->es, (uint16_t)(c->di + 0x0088u), c->ax);", "bwr16(bus, c->es, (uint16_t)(c->di + 0x0086u), c->ax);")]),
    ('0130 quotient and remainder swapped', [("bwr16(bus, c->es, (uint16_t)(c->di + 0x0090u), c->ax);", "bwr16(bus, c->es, (uint16_t)(c->di + 0x008Eu), c->ax);"),
                                            ("bwr16(bus, c->es, (uint16_t)(c->di + 0x008Eu), c->ax);                                /* 017A", "bwr16(bus, c->es, (uint16_t)(c->di + 0x0090u), c->ax);                                /* 017A")]),
    ('0130 loop runs to 5', [("cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u)), 4u);                             /* 0152", "cmp8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u)), 5u);                             /* 0152")]),
    ('0130 LES once (pointer cached)', [("STEP(0x0176u); les(ram, c, c->ds, 0x5D92u);", "STEP(0x0176u);")]),
    ('0130 record written flat', [("static void bwr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)\n{ size_t p = physical(seg, off); pr_bus_write8(bus, p, (uint8_t)v); pr_bus_write8(bus, (p + 1u) & MASK, (uint8_t)(v >> 8u)); }",
                                   "static void bwr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)\n{ wr16(bus->ram, seg, off, v); }")]),
    ('0185 block at 67F0', [("wr16(ram, c->ds, (uint16_t)(c->di + 0x67F4u), c->ax);", "wr16(ram, c->ds, (uint16_t)(c->di + 0x67F0u), c->ax);")]),
    ('0185 high words not zeroed (no XOR DX after DIV)', [("STEP(0x01B8u); c->dx = xor_zero(c);", "STEP(0x01B8u);")]),
    ('DIV flags left unchanged', [("    set_arith(c, f);\n}\nstatic void xchg_dx_ax", "    (void)f;\n}\nstatic void xchg_dx_ax")]),
    ('DIV CF from remainder bit 0', [("if ((rem & 3u) == 1u || (rem & 3u) == 2u) f |= 0x0001u;", "if (rem & 1u) f |= 0x0001u;")]),
    ('SHL without AF', [("f = (uint16_t)(parity((uint8_t)r) | 0x0010u);", "f = parity((uint8_t)r);")]),
    ('INC clears CF', [("c->flags = (uint16_t)((c->flags & ~0x08D4u) | f); return r;", "c->flags = (uint16_t)((c->flags & ~0x08D5u) | f); return r;")]),
    ('RETF 4', [("c->sp = (uint16_t)(c->sp + 2u);   /* RETF 2 */", "c->sp = (uint16_t)(c->sp + 4u);   /* RETF 2 */")]),
    ('the return-address check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_DIV5_CPU_TRANSFER;", "(void)ret_ip; (void)ret_cs;")]),
    ('the loop INC not charged to the budget', [("STEP(0x013Cu); wr8(", "wr8(")]),
)


def mutation_check(blob, decoded):
    src = (ROOT / 'src/recovered/overlay_u7f_div5_leaves_cpu.c').read_text()
    with tempfile.TemporaryDirectory(prefix='u7fd5-base-') as tmp:                       # the unmutated model must pass the same subset first (PM i22687)
        baseline = differential(blob, decoded, library(tmp), quick=True, only=MUTANT_CASES)
    check(baseline['cases'] == len(MUTANT_CASES), f'baseline ran {baseline["cases"]} of {len(MUTANT_CASES)} mutation cases')
    out = []
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u7fd5-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u7f_div5_leaves_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR (not counted)')); continue
            try:
                differential(blob, decoded, mlib, quick=True, only=MUTANT_CASES)
                out.append((name, 'SURVIVED the differential'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:110]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:80]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    built = [r for r in out if not r[1].startswith('COMPILE-ERROR')]
    return dict(baseline=dict(cases=baseline['cases'], stops=baseline['stops'], mismatches=0), total=len(built), caught=sum(1 for _, r in built if r.startswith('CAUGHT:')),
                not_built=[n for n, r in out if r.startswith('COMPILE-ERROR')], survivors=[n for n, r in built if not r.startswith('CAUGHT:')], results=out, cases=list(MUTANT_CASES))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u7fd5-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        blob, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(blob, decoded, lib, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(blob, decoded)
        if a.native:
            from ovl_007f_div5_leaves_native import native
            report['native'] = native(blob, decoded, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_007f_div5_leaves_cpu_audit'] = sys.modules[__name__]
    main()
