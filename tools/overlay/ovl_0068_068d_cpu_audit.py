#!/usr/bin/env python3
"""OVL_0068_068D (unit 0x68 [068D,073B), the Up preflight) against the original bytes, composed through its overlay stub with unit 0x3BD +0350 (which itself calls +047B and
+031F): CPU14, all 1,048,576 RAM bytes and the stop, unmasked after every case, plus every instruction start and conditional outcome of the 068D body.

The LCALL at 06C0 goes through the stub 03D5:002F (relocated at run time). The audit places the stub LOADED (EAh, 0350h, the unit's segment) or UNLOADED (INT 3Fh, as the overlay
manager leaves it): an unloaded stub stops both sides at the stub with OVERLAY, before the overlay manager. Unit 0x3BD's whole code image is placed at its segment so the original
really runs +0350, +047B and +031F. The 5AAh word stores go through the bus (flat RAM here). Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_0068_068D',)
ENTRY, END = 0x068D, 0x073B
W_CS, U3BD_CS, STUB_SEG, STUB_OFF = 0x6000, 0x7000, 0x0FD5, 0x002F
SP_MIN, SP_MAX = 0x0030, 0xFFFC
SOURCES = ['overlay_u68_up_preflight_068d_cpu', 'overlay_u3bd_lookup_fullstate', 'overlay_u3bd_in_grid_031f', 'memory_bus', 'ega_hw']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'OVERLAY', 3: 'CALLEE', 4: 'TRANSFER'}
ENTRY_RET = A.ENTRY_RET
check = A.check
REGISTERS = A.REGISTERS


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.c_void_p), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p), ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    u68, u3bd = units[0x68], units[0x3BD]
    code68 = ovr[u68['ovr_offset']:u68['ovr_offset'] + u68['code_size']]; code3bd = ovr[u3bd['ovr_offset']:u3bd['ovr_offset'] + u3bd['code_size']]
    body = code68[ENTRY:END]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(sum(i.size for i in ins) == len(body) and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '', 'decode covers [068D,073B) and ends in RETF')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x06C0, '0x3d5, 0x2f')], 'the one far call is LCALL 03D5:002F at 06C0')
    check([f for f in u68['fixups'] if ENTRY <= f < END] == [0x06C3], 'the only relocation in the span is the LCALL segment word at 06C3')
    check(u3bd['entries'][3] == 0x0350, 'unit 0x3BD public entry 3 is +0350')
    check(not [i for i in ins if i.mnemonic in ('call', 'int', 'in', 'out')], 'no near call, interrupt or port instruction in the body')
    meta = dict(bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), file_offset=u68['ovr_offset'] + ENTRY, memory_segment=u68['memory_segment'],
                callee_unit_sha256=sha256(code3bd).hexdigest())
    return body, code3bd, {i.address: i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('u68_068d_mut.so' if override else 'u68_068d.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u68_up_preflight_068d_cpu.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu)]; lib.pr_u68_up_preflight_068d_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); bus = Bus(ctypes.addressof(ram), None, None, None, None)
    status = lib.pr_u68_up_preflight_068d_cpu(ctypes.byref(bus), SIZE, ctypes.byref(out))
    return STATUS[status], bytes(ram), out


def run_original(state, body_ips, limit=200000):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_AX
    memory, cpu, info = state; backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory)
    uc = Uc(UC_ARCH_X86, UC_MODE_16); uc.mem_map_ptr(0, SIZE, 7, ctypes.addressof(backing)); uc.mem_map_ptr(SIZE, SIZE, 7, ctypes.addressof(backing))
    for (name, _), reg in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(reg, getattr(cpu, name))
    s = dict(stop=None, steps=0, prev=None, al=None); seen = set(); edges = set()

    def hook(em, address, size, user):
        cs = em.reg_read(UC_X86_REG_CS); ip = em.reg_read(UC_X86_REG_IP)
        if (cs, ip) == ENTRY_RET:
            s['stop'] = 'RETURNED'; return em.emu_stop()
        if bytes(em.mem_read(address, 2)) == b'\xcd\x3f':              # an unloaded overlay stub: stop before the overlay manager
            s['stop'] = 'OVERLAY'; return em.emu_stop()
        if cs not in (info['w_cs'], info['u3bd_cs'], info['stub_seg']):            # a clobbered return frame: the original leaves for another segment
            s['stop'] = 'TRANSFER'; return em.emu_stop()
        if cs == info['w_cs']:
            if ip == 0x06C5:
                s['al'] = em.reg_read(UC_X86_REG_AX) & 0xFF
            check(ip in body_ips, f'unexpected 068D PC {ip:04x}')
            seen.add(ip)
            if s['prev'] is not None:
                edges.add((s['prev'], ip))
            s['prev'] = ip
        else:
            s['prev'] = None
        s['steps'] += 1
        if s['steps'] > limit:
            s['stop'] = 'RUNAWAY'; em.emu_stop()
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(physical(cpu.cs, cpu.ip), 2 * SIZE, count=limit + 10)
    check(s['stop'] in ('RETURNED', 'OVERLAY', 'TRANSFER'), f'original stop {s["stop"]}')
    out = Cpu(*(uc.reg_read(r) & 0xFFFF for r in REGISTERS))
    return s['stop'], bytes(backing), out, seen, edges, s['al']


def fixture(body, code3bd, *, x=5, y=3, facing=2, dx=0, dy=-1, loaded=True, mapid=1, cellfill=None, ds=0x1000, ss=0x2000, sp=0x8000, w_cs=W_CS, u3bd_cs=U3BD_CS,
            window=(0x4000, 0x0100), flags=0x0202, regs=None, background=0xA5, seed=0, tables=None, stub_seg=STUB_SEG, buf_seg=0x5000):
    m = bytearray([background]) * SIZE
    rng = random.Random(seed)
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    at = physical(w_cs, ENTRY); m[at:at + len(body)] = body
    put16(m, w_cs, 0x06C3, stub_seg)                                    # the relocated LCALL segment word
    at = physical(u3bd_cs, 0); m[at:at + len(code3bd)] = code3bd       # unit 0x3BD, whole
    stub = physical(stub_seg, STUB_OFF)
    m[stub:stub + 5] = (bytes([0xEA]) + (0x0350).to_bytes(2, 'little') + u3bd_cs.to_bytes(2, 'little')) if loaded else bytes.fromhex('cd3f500300')
    m[physical(ds, 0x6AAD)] = x & 0xFF; m[physical(ds, 0x6AAE)] = y & 0xFF; m[physical(ds, 0x6AAF)] = facing & 0xFF
    t = tables or {}
    for k in range(0x2736, 0x273F + 256):                                # the facing-indexed step tables (signed bytes): dx at 2736, dy at 273F, 9 apart, so they
        m[physical(ds, k & 0xFFFF)] = rng.randrange(256)                 # overlap for a facing above 8; random first, then the entries the case names
    for (axis, f), v in t.items():
        m[physical(ds, ((0x2736 if axis == 'dx' else 0x273F) + f) & 0xFFFF)] = v & 0xFF
    m[physical(ds, (0x2736 + facing) & 0xFFFF)] = dx & 0xFF; m[physical(ds, (0x273F + facing) & 0xFFFF)] = dy & 0xFF
    put16(m, ds, 0x49D6, window[1]); put16(m, ds, 0x49D8, window[0]); put16(m, window[0], (window[1] + 0x5AA) & 0xFFFF, 0x5A5A)
    m[physical(ds, 0x84DC)] = mapid
    put16(m, ds, 0x6A5C, 0x0000); put16(m, ds, 0x6A5E, buf_seg)
    for k in range(0x402):                                               # the u3bd buffer: 1,024 bytes plus the block header
        m[physical(buf_seg, k)] = cellfill if cellfill is not None else rng.randrange(256)
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, u3bd_cs=u3bd_cs, stub_seg=stub_seg, ds=ds, ss=ss, sp=sp, loaded=loaded)


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
    rng = random.Random(0x68068D)
    out = []

    def add(name, **kw):
        out.append((name, kw))
    add('unloaded-stub', loaded=False); add('unloaded-stub-edge', loaded=False, x=15, y=0)
    for cell in (0x00, 0xFF, 0x55):                                      # buffer fills: the 0350 result is 1 or 3 here; only maps 0/0Ah out of the grid give 0
        for x, y, dx, dy in ((5, 3, 0, -1), (15, 3, 1, 0), (0, 3, -1, 0), (5, 15, 0, 1), (5, 0, 0, -1), (15, 15, 1, 1), (0, 0, -1, -1)):
            add(f'cell{cell:02x}-x{x}-y{y}-d{dx}{dy}', x=x, y=y, dx=dx, dy=dy, cellfill=cell)
    for mapid in (0, 10, 1):
        add(f'map-{mapid}-out', x=20, y=3, mapid=mapid, cellfill=0xFF)
    for facing in (0, 1, 2, 3, 7, 8, 0xFF):
        add(f'facing-{facing:02x}', facing=facing, cellfill=0xFF, dx=1, dy=1, x=15, y=15)
    for x in (0x7F, 0x80, 0xFF, 0x10):
        add(f'x-{x:02x}', x=x, cellfill=0x0F)
    for fl in (0x0002, 0x08D7, 0x0ED7, 0x0003):
        add(f'flags-{fl:04x}', flags=fl, cellfill=0xFF)
    for sp in (SP_MIN, 0x7FFF, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, cellfill=0xFF)
    add('window-wrap', window=(0x4000, 0xFA56), cellfill=0xFF, x=15, dx=1)
    add('window-a20', window=(0xFFFF, 0x0100), cellfill=0xFF, x=0, dx=-1)
    add('ds-eq-ss', ds=0x2000, ss=0x2000, sp=0x9000, cellfill=0xFF, x=15, dx=1)
    for k, name in ((2, 'bp'), (4, 'x'), (6, 'y'), (8, 'facing'), (-2, 'ret-cs')):       # the window word on the frame: [BP], the locals, the return CS
        add(f'window-on-{name}', window=(0x2000, (0x8000 - k - 0x5AA) & 0xFFFF), cellfill=0xFF, x=15, dx=1)
    add('window-on-x-negative', window=(0x2000, (0x8000 - 4 - 0x5AA) & 0xFFFF), cellfill=0xFF, x=0, dx=-1)   # the raised word rewrites the x local between the two compares
    for k in range(40 if quick else 200):
        add(f'random{k}', x=rng.randrange(256), y=rng.randrange(256), facing=rng.randrange(256), dx=rng.randrange(-2, 3), dy=rng.randrange(-2, 3),
            loaded=rng.random() > 0.1, mapid=rng.choice((0, 10, 1, rng.randrange(256))), regs=tuple(rng.randrange(65536) for _ in range(8)),
            flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1), seed=k)
    return out


def sx(v):
    return v - 256 if v & 0x80 else v


def independent(name, state, expected):
    """Without the C model: the party bytes and the window word from the stated clamp rule, given the +0350 result AL the original produced at 06C5,
    and SP/BP/CS:IP on return. Skipped when the window word or a party byte aliases the step tables, the frame or each other."""
    memory, cpu, info = state; ram = expected[1]; ds = info['ds']
    if expected[0] != 'RETURNED':
        return 0
    win = physical(u16(memory, physical(ds, 0x49D8)), (u16(memory, physical(ds, 0x49D6)) + 0x5AA) & 0xFFFF)
    party = [physical(ds, 0x6AAD + k) for k in range(3)]
    stack = {physical(info['ss'], (info['sp'] - k) & 0xFFFF) for k in range(-4, 0x31)}
    if {win, (win + 1) & 0xFFFFF} & (set(party) | stack) or set(party) & stack:
        return 0
    check(expected[2].sp == (info['sp'] + 4) & 0xFFFF and expected[2].bp == cpu.bp and (expected[2].cs, expected[2].ip) == ENTRY_RET, f'{name}: frame on return')
    x, y, f = (memory[a] for a in party); al = expected[5]
    want_x, want_y, flag = x, y, 0
    if al:
        cx = sx(x) + sx(memory[physical(ds, (0x2736 + f) & 0xFFFF)]); cy = sx(y) + sx(memory[physical(ds, (0x273F + f) & 0xFFFF)])
        if cx > 15: want_x, flag = 0x0F, 1
        if cx < 0: want_x, flag = 0, 1
        if cy > 15: want_y, flag = 0x0F, 1
        if cy < 0: want_y, flag = 0, 1
    check((ram[party[0]], ram[party[1]]) == (want_x, want_y), f'{name}: party x/y {ram[party[0]]},{ram[party[1]]} rule {want_x},{want_y}')
    check(ram[win] | ram[(win + 1) & 0xFFFFF] << 8 == flag, f'{name}: window word rule {flag}')
    return 1


def u16(m, p):
    return m[p] | m[(p + 1) & 0xFFFFF] << 8


def differential(body, code3bd, body_ips, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); edges = set(); statements = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw) in enumerate(cs):
        if index % 100 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(body, code3bd, **kw)
        expected, actual = compare(name, st, body_ips, lib)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[3]; edges |= expected[4]; statements += independent(name, st, expected)
        if actual[0] in ('ARGUMENT', 'CALLEE') or (actual[0] == 'TRANSFER') != (name == 'window-on-ret-cs'):
            raise ValueError(f'{name}: C stopped {actual[0]}')
    conds = {a: (int(i.op_str, 16), a + i.size) for a, i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(only is not None or seen == set(body_ips), f'starts not reached: {sorted(hex(a) for a in set(body_ips) - seen)}')
    check(only is not None or outcomes <= edges, f'outcomes not reached: {sorted(outcomes - edges)}')
    return dict(cases=len(cs), end_states=stops, starts=f'{len(seen)}/{len(body_ips)}', outcomes=f'{len(outcomes & edges)}/{len(outcomes)}',
                cpu_words=14, ram_bytes=SIZE, flag_masks=0, independent_statements=statements, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('unloaded-stub', 'cellff-x15-y3-d10', 'cellff-x0-y3-d-10', 'cellff-x5-y15-d01', 'cellff-x5-y0-d0-1', 'cell00-x5-y3-d0-1', 'cell55-x15-y15-d11',
                'facing-07', 'x-80', 'flags-08d7', 'window-wrap', 'map-0-out', 'random0', 'random1', 'random2', 'random3', 'window-on-x', 'window-on-x-negative')
MUTANTS = (
    ('the stub is not read (assumed loaded)', [("if (rd8(ram, stub_seg, stub_off) != 0xEAu || rd16(ram, stub_seg, (uint16_t)(stub_off + 1u)) != 0x0350u)\n        return PR_U68_068D_OVERLAY;",
                                               "(void)stub_off;")]),
    ('the dy table is at 2737', [("0x273Fu", "0x2737u")]),
    ('the clamp writes 0Eh', [("wr8(ram, c->ds, 0x6AADu, 0x0Fu)", "wr8(ram, c->ds, 0x6AADu, 0x0Eu)")]),
    ('unsigned clamp compare', [("if (v > 15) { wr8(ram, c->ds, 0x6AADu", "if ((uint16_t)v > 15) { wr8(ram, c->ds, 0x6AADu")]),
    ('the window flag is not raised on the x upper clamp', [("if (v > 15) { wr8(ram, c->ds, 0x6AADu, 0x0Fu); flag_window(bus, ram, c); }", "if (v > 15) { wr8(ram, c->ds, 0x6AADu, 0x0Fu); }")]),
    ('the window word is not cleared first', [("bus_wr16(bus, c->es, (uint16_t)(c->di + 0x05AAu), c->ax);                     /* 06AF", "(void)0;                     /* 06AF")]),
    ('the facing is sign-extended', [("c->ax &= 0x00FFu;", "cbw(c);")]),
    ('RETF pops 2 argument bytes', [("c->ip = pop(ram, c); c->cs = pop(ram, c);                                     /* 073A RETF */",
                                     "c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 2u);     /* 073A RETF */")]),
    ('the window stores bypass the bus', [("{ pr_bus_write8(bus, physical(seg, off), (uint8_t)v); pr_bus_write8(bus, physical(seg, (uint16_t)(off + 1u)), (uint8_t)(v >> 8u)); }",
                                           "{ (void)bus; wr16(bus->ram, seg, off, v); }")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u68_up_preflight_068d_cpu' else ROOT / f'src/recovered/{s}.c') for s in SOURCES]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u68_up_preflight_068d_cpu.c')] + srcs +
                       ['-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(body, code3bd, body_ips):
    out = []; src = (ROOT / 'src/recovered/overlay_u68_up_preflight_068d_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) >= 1, f'mutant text occurs: {name}: {old[:60]}'); text = text.replace(old, new, 1)
        with tempfile.TemporaryDirectory(prefix='u68-068d-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u68_up_preflight_068d_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(body, code3bd, body_ips, mlib, quick=True, only=MUTANT_CASES)
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
    with tempfile.TemporaryDirectory(prefix='u68-068d-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        body, code3bd, body_ips, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(body, code3bd, body_ips, lib, a.quick)
        if a.native:
            from ovl_0068_068d_native import native
            report['native'] = native(body, code3bd, lib)
        if a.mutate:
            report['mutation'] = mutation_check(body, code3bd, body_ips)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_0068_068d_cpu_audit'] = sys.modules[__name__]
    main()
