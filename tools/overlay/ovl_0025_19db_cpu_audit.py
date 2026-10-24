#!/usr/bin/env python3
"""OVL_0025_19DB (GAME.OVR unit 25h entry 19, the party step) as a raw full-state model composed with unit 3BDh's raw +057A (cell) and +047B (nibble), against
the original bytes.

The world is u25_move_original_audit.media(): the relocated START.EXE at 1000h, units 25h and 3BDh loaded at their own segments with every fixup applied, and
the overlay stub 03D5 in its LOADED state (slots 2 and 4 hold JMP FAR to unit 3BDh), plus a 1,024-byte map behind DS:[6A5C]. A case may put a slot back to
the unloaded INT 3Fh: the model then stops STUB at the LCALL, and the oracle stops at the same LCALL before executing it. Compared after every case, unmasked:
CPU14, ALL 1,048,576 RAM bytes, the EGA state and the ordered VRAM write operands, the stop and the remaining budget (one unit per instruction of THIS body).
FLAGS are exact except where the last FLAGS writer is a non-zero shift or a word DIV/MUL in a callee, where the declared pinned split applies (counted).
Declared exclusion: +047B reads the map FLAT, so a map inside A0000..AFFFF is only exercised where +047B does not read it (an unmatched facing). Every case's
world is built from its own name (PM i22687). Media stay under ignored resources/."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from pinned_shift_flags import LastFlagWriter, declared_flags_split, split_used, SPLIT_BY_RULE
from res00ba_display_1693_cpu_audit import PythonEga
from res0709_printer_0626_cpu_audit import Cpu, Ega, Bus, Env, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE, REGISTERS, physical, put16, check
import u25_move_original_audit as U

CHECKER_ROWS = ('OVL_0025_19DB',)
ENTRY, END, INSNS = U.ENTRY, U.END, U.INSNS
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'LIMIT', 3: 'STUB', 4: 'TRANSFER', 5: 'CALLEE'}
W, DS, SS, STUBSEG = U.SEG[0x25], U.DS, U.SS, U.BASE + U.STUB
RET = (0x8F00, 0x0000)                    # every rewritten return target stays below A000 (8F00:FFFF = 9EFFFh)
SP_MIN, SP_MAX = 0x0030, 0xFFFC
BUDGET = 1000
CALLS = {0x1A47: 0x0034, 0x1A5B: 0x002A}
SOURCES = ['overlay_u25_move_19db_cpu', 'overlay_u3bd_cell_057a_cpu', 'overlay_u3bd_in_grid_031f', 'overlay_u3bd_lookup_fullstate', 'memory_bus', 'ega_hw']


def source():
    m, meta = U.media()
    body = bytes(m[physical(W, ENTRY):physical(W, END)])
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == INSNS and sum(i.size for i in ins) == END - ENTRY, '19DB: 138 bytes / 50 instructions')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x1A47, f'{hex(STUBSEG)}, 0x34'), (0x1A5B, f'{hex(STUBSEG)}, 0x2a')], 'the two far calls through stub 03D5')
    check([i.address for i in ins if i.mnemonic in ('ret', 'retf')] == [0x1A64], 'the only return is RETF at 1A64')
    return m, {i.address: i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('u25m_mut.so' if override else 'u25m.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u25_move_19db_cpu.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(ctypes.c_size_t)]; lib.pr_u25_move_19db_cpu.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def fixture(name, base, *, x=3, y=4, facing=0, map_id=1, mp=U.MAP, sp=0xF000, ss=SS, flags=0x0202, regs=None, unload=(), writes=(), budget=BUDGET):
    """The loaded world, then this case's state bytes, map, stack residue, registers and frame, all from the case's own name."""
    rng = random.Random(f'OVL_0025_19DB/{name}')
    m = bytearray(base)
    for k in range(0x60):                                   # the caller's stack residue first: with SS = DS it lies under the state bytes
        m[physical(ss, (sp - 0x60 + k) & 0xFFFF)] = rng.randrange(256)
    for a, v in zip(U.STATE, (x, y, facing, rng.randrange(256), rng.randrange(256))):
        m[physical(DS, a)] = v & 0xFF
    m[physical(DS, 0x84DC)] = map_id & 0xFF
    put16(m, DS, 0x6A5C, mp[1]); put16(m, DS, 0x6A5E, mp[0])
    for k in range(1024):
        m[physical(mp[0], (mp[1] + k) & 0xFFFF)] = rng.randrange(256)
    for slot in unload:                                     # back to the unloaded stub: INT 3Fh and its operand bytes
        at = physical(STUBSEG, slot); m[at:at + 5] = b'\xcd\x3f\x00\x00\x00'
    put16(m, ss, sp, RET[1]); put16(m, ss, (sp + 2) & 0xFFFF, RET[0])
    for seg, off, data in writes:
        for j, v in enumerate(data):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    r = regs or tuple(rng.randrange(65536) for _ in range(8))
    c = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, W, ENTRY, DS, r[7], ss, (flags | 0x0002) & ~0x0100)
    return m, c, dict(name=name, x=x & 0xFF, y=y & 0xFF, facing=facing & 0xFF, map_id=map_id & 0xFF, sp=sp, budget=budget, plain=not writes and not unload, mp=mp)


def run_original(st, body):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn import x86_const as R
    m, c, info = st
    uc = Uc(UC_ARCH_X86, UC_MODE_16); backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); device = PythonEga()

    def vread(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def vwrite(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for b in (0, SIZE):
        uc.mem_map_ptr(b, 0xA0000, 7, ctypes.addressof(backing)); uc.mmio_map(b + 0xA0000, 65536, vread, None, vwrite, None)
        uc.mem_map_ptr(b + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(c, n))
    s = dict(stop=None, left=info['budget'], prev=None, steps=0); seen = set(); edges = set(); writer = LastFlagWriter()
    known = {W, U.SEG[0x3BD], STUBSEG}

    def hook(em, address, size, user):
        cs = uc.reg_read(R.UC_X86_REG_CS); ip = uc.reg_read(R.UC_X86_REG_IP)
        writer.finish(uc)
        if (cs, ip) == RET:
            s['stop'] = 'RETURNED'; return uc.emu_stop()
        if cs not in known or (cs == W and ip not in body):
            s['stop'] = 'TRANSFER'; return uc.emu_stop()
        if cs == W:
            if s['left'] == 0:
                s['stop'] = 'LIMIT'; return uc.emu_stop()
            if ip in CALLS:
                slot = bytes(uc.mem_read(physical(STUBSEG, CALLS[ip]), 3))
                if slot[0] != 0xEA:
                    s['stop'] = 'STUB'; return uc.emu_stop()
            s['left'] -= 1; seen.add(ip)
            if s['prev'] is not None:
                edges.add((s['prev'], ip))
            s['prev'] = ip
        s['steps'] += 1
        if s['steps'] > 100000:
            s['stop'] = 'RUNAWAY'; return uc.emu_stop()
        writer.step(uc, address)
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=200000)
    writer.finish(uc)
    check(s['stop'] in ('RETURNED', 'LIMIT', 'STUB', 'TRANSFER'), f'{info["name"]}: original stop {s["stop"]}')
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
    status = lib.pr_u25_move_19db_cpu(ctypes.byref(bus), SIZE, ctypes.byref(cpu), ctypes.byref(left))
    return STATUS[status], bytes(ram), writes, bytes(ega), cpu, left.value


def compare(st, body, lib):
    name = st[2]['name']
    estop, eram, ewr, eega, ecpu, eleft, seen, edges, writer = run_original(st, body)
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


def statement(st, stop, ram):
    """Without the C model, on plain RETURNED cases: the position after the step (the stated wrap rule) and the return."""
    m, c, info = st
    if stop != 'RETURNED' or not info['plain']:
        return 0
    s8 = lambda v: v - 256 if v & 0x80 else v
    x, y, f = info['x'], info['y'], info['facing']
    if f == 0:
        y = y - 1 if s8(y) > 0 else 0x0F
    elif f == 2:
        x = x + 1 if s8(x) < 0x0F else 0
    elif f == 4:
        y = y + 1 if s8(y) < 0x0F else 0
    elif f == 6:
        x = x - 1 if s8(x) > 0 else 0x0F
    got = (ram[physical(DS, 0x6AAD)], ram[physical(DS, 0x6AAE)], ram[physical(DS, 0x6AAF)])
    check(got == (x & 0xFF, y & 0xFF, f), f'{info["name"]}: position {got}, rule {(x & 0xFF, y & 0xFF, f)}')
    return 1


def cases(quick=False):
    out = []

    def add(name, **kw):
        out.append((name, kw))
    vals = (0x00, 0x01, 0x07, 0x0E, 0x0F, 0x10, 0x7F, 0x80, 0xFF)
    for f in (0, 2, 4, 6, 1, 3, 5, 7, 8, 0xFF):
        for v in vals:
            add(f'f{f:02x}-x{v:02x}', facing=f, x=v, y=5)
            add(f'f{f:02x}-y{v:02x}', facing=f, x=5, y=v)
    for mid in (0x00, 0x0A, 0x01, 0x02, 0xFF):
        for f in (0, 2, 4, 6, 3):
            add(f'map{mid:02x}-f{f:02x}', map_id=mid, facing=f, x=0x0F, y=0x00)
            add(f'map{mid:02x}-f{f:02x}-out', map_id=mid, facing=f, x=0x20, y=0xF0)
    for fl in (0x0002, 0x0202, 0x0ED7, 0x08D7, 0x0003, 0x0A87, 0x0656):
        add(f'flags-{fl:04x}', flags=fl, facing=2); add(f'flags-{fl:04x}-f05', flags=fl, facing=5)
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, facing=4)
    for mp in ((0x5000, 0xFF00), (0x5000, 0xFE01), (0xFFFF, 0x0010), (0xFFF0, 0x0000)):        # offset wraps below A000 (a wrap into A000 is the +047B exclusion)
        for f in (0, 2):
            add(f'mp-{mp[0]:04x}-{mp[1]:04x}-f{f}', mp=mp, facing=f, x=0x0F, y=0x0F)
    for mp in ((0xA000, 0x0000), (0x9FC0, 0x0000), (0xAFF0, 0x0200)):        # the map in the EGA aperture: +057A reads it through the bus; facing 1 keeps +047B off the map
        add(f'ega-{mp[0]:04x}-{mp[1]:04x}', mp=mp, facing=1, x=3, y=4, map_id=1)
    for slots, tag in (((0x0034,), 'cell'), ((0x002A,), 'nibble'), ((0x0034, 0x002A), 'both')):
        add(f'unloaded-{tag}', unload=slots, facing=2)
    for k in range(-0x08, 0x08):                                             # DS = SS would need a moved DS; instead the map over the frame: the cell read sees the frame
        add(f'map-on-frame{k:+03x}', mp=(SS, (0xF000 + k - 0x200 - 0x50) & 0xFFFF), facing=1, x=0, y=5)
    for sp in (0x6AB0, 0x6AAF):                                              # SS = DS: MOV [6AB0]/[6AB1],AL writes into this body's own return address (TRANSFER)
        add(f'ret-under-{sp:04x}', ss=DS, sp=sp, facing=1, x=3, y=4)
    for b in range(0, 34, 1 if not quick else 3):
        add(f'budget-f00-{b}', budget=b, facing=0, x=3, y=4)
        add(f'budget-f06-{b}', budget=b, facing=6, x=0, y=4)
    for k in range(8 if quick else 60):
        rng = random.Random(f'OVL_0025_19DB/random{k}')
        add(f'random{k}', x=rng.choice(vals + (rng.randrange(256),)), y=rng.choice(vals + (rng.randrange(256),)), facing=rng.choice((0, 2, 4, 6, 0, 2, 4, 6, 1, 9)),
            map_id=rng.choice((0, 1, 0x0A, rng.randrange(256))), flags=rng.randrange(65536) & 0x0ED7, sp=rng.randrange(0x0100, SP_MAX + 1),
            regs=tuple(rng.randrange(65536) for _ in range(8)))
    return out


def differential(base, body, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; stated = 0; seen = set(); edges = set()
    for k in SPLIT_BY_RULE:
        SPLIT_BY_RULE[k] = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw) in enumerate(cs):
        if index % 50 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(name, base, **kw)
        stop, ram, cpu, s, e = compare(st, body, lib)
        stops[stop] = stops.get(stop, 0) + 1; seen |= s; edges |= e
        stated += statement(st, stop, ram)
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(only is not None or seen == set(body), f'starts not reached {sorted(hex(a) for a in set(body) - seen)}')
    check(only is not None or outcomes <= edges, f'outcomes not reached {sorted(outcomes - edges)}')
    return dict(cases=len(cs), stops=dict(sorted(stops.items())), starts=f'{len(seen & set(body))}/{len(body)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                independent_statements=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, declared_splits=dict(SPLIT_BY_RULE), seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('f00-y00', 'f00-y01', 'f00-y80', 'f02-x0e', 'f02-x0f', 'f02-x7f', 'f02-x80', 'f04-y0f', 'f04-yff', 'f06-x00', 'f06-x01', 'f06-xff', 'f05-x07',
                'map00-f00', 'map0a-f02-out', 'flags-0ed7', 'flags-0ed7-f05', 'sp-0030', 'ega-a000-0000', 'unloaded-cell', 'unloaded-nibble', 'map-on-frame+00',
                'budget-f00-3', 'budget-f00-9', 'budget-f00-18', 'budget-f06-12', 'budget-f06-21', 'budget-f06-30', 'ret-under-6ab0', 'ret-under-6aaf')
MUTANTS = (
    ('facing 0 compared with JL (no equal case)', [("static int jle(const cpu_t *c) { return (c->flags & 0x0040u) || (!(c->flags & 0x0080u) != !(c->flags & 0x0800u)); }",
                                                    "static int jle(const cpu_t *c) { return (!(c->flags & 0x0080u) != !(c->flags & 0x0800u)); }")]),
    ('JGE as unsigned JAE', [("static int jge(const cpu_t *c) { return !(c->flags & 0x0080u) == !(c->flags & 0x0800u); }", "static int jge(const cpu_t *c) { return !(c->flags & 0x0001u); }")]),
    ('the y wrap value is 0Eh', [("STEP(0x19F2u); wr8(ram, c->ds, 0x6AAEu, 0x0Fu);", "STEP(0x19F2u); wr8(ram, c->ds, 0x6AAEu, 0x0Eu);")]),
    ('facing 6 increments', [("wr8(ram, c->ds, 0x6AADu, incdec8(c, rd8(ram, c->ds, 0x6AADu), 1));  /* 1A34", "wr8(ram, c->ds, 0x6AADu, incdec8(c, rd8(ram, c->ds, 0x6AADu), 0));  /* 1A34")]),
    ('the cell result stored at 6AB0', [("STEP(0x1A4Cu); wr8(ram, c->ds, 0x6AB1u, (uint8_t)c->ax);", "STEP(0x1A4Cu); wr8(ram, c->ds, 0x6AB0u, (uint8_t)c->ax);")]),
    ('the nibble call pushes y twice', [("STEP(0x1A57u); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AAFu));", "STEP(0x1A57u); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AAEu));")]),
    ('DEC clears CF', [("c->flags = (uint16_t)((c->flags & ~0x08D4u) | f); return r;", "c->flags = (uint16_t)((c->flags & ~0x08D5u) | f); return r;")]),
    ('the stub is not checked (always followed)', [("if (rd16(ram, cs, (uint16_t)((site_) + 1u)) != (slot_) || rd8(ram, stub, (slot_)) != 0xEAu || rd16(ram, stub, (uint16_t)((slot_) + 1u)) != (entry_)) \\",
                                                    "if (rd16(ram, cs, (uint16_t)((site_) + 1u)) != (slot_)) \\")]),
    ('RETF pops an argument', [("STEP(0x1A64u); c->ip = pop(ram, c); c->cs = pop(ram, c);", "STEP(0x1A64u); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 2u);")]),
    ('the far call not charged to the budget', [("    --*budget; push(ram, c, cs); push(ram, c, (next_));", "    push(ram, c, cs); push(ram, c, (next_));")]),
    ('the return-address check dropped', [("if (c->ip != ret_ip || c->cs != ret_cs) return PR_U25_19DB_TRANSFER;\n    return PR_U25_19DB_RETURNED;", "(void)ret_ip; (void)ret_cs;\n    return PR_U25_19DB_RETURNED;")]),
    ('the PUSH AX before the cell call pushes AL zero-extended', [("STEP(0x1A46u); push(ram, c, c->ax);", "STEP(0x1A46u); push(ram, c, (uint16_t)(c->ax & 0x00FFu));")]),
)


def mutation_check(base, body):
    src = (ROOT / 'src/recovered/overlay_u25_move_19db_cpu.c').read_text()
    with tempfile.TemporaryDirectory(prefix='u25m-base-') as tmp:                         # the unmutated model must pass the same subset first (PM i22687)
        baseline = differential(base, body, library(tmp), quick=True, only=MUTANT_CASES)
    check(baseline['cases'] == len(MUTANT_CASES), f'baseline ran {baseline["cases"]} of {len(MUTANT_CASES)} mutation cases')
    out = []
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) == 1, f'mutant text occurs once: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u25m-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u25_move_19db_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR (not counted)')); continue
            try:
                differential(base, body, mlib, quick=True, only=MUTANT_CASES)
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
    with tempfile.TemporaryDirectory(prefix='u25m-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        base, body, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(base, body, lib, a.quick)
        if a.mutate:
            report['mutation'] = mutation_check(base, body)
        if a.native:
            from ovl_0025_19db_native import native
            report['native'] = native(base, body, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_0025_19db_cpu_audit'] = sys.modules[__name__]
    main()
