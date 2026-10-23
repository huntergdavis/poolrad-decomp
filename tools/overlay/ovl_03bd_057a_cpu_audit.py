#!/usr/bin/env python3
"""OVL_03BD_057A (unit 0x3BD [057A,05F3), public entry 4, the cell lookup) against the original bytes, composed with its +031F range leaf: CPU14, all 1,048,576 RAM bytes and the stop,
unmasked after every case, plus every instruction start and every conditional outcome of the body.

The body calls the +031F leaf on its x ([BP+8]) and y ([BP+6]) bytes; on a 0 result with map DS:[84DC] = 0 or 0Ah it returns 0, otherwise it wraps x and y into 0..15 IN PLACE and
returns the byte at ES:[DI+200h], DI = DS:[6A5C] + sign(y)*16 + sign(x). The buffer byte goes through the bus (flat RAM here; no EGA in the window). The audit puts the map byte and the
buffer pointer on the frame (DS = SS), the buffer at offset wraps, and every x/y class across the wrap boundaries. Media stay under ignored resources/."""
import argparse, ctypes, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_0000_cpu_audit as A
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import Cpu, ROOT, SIZE, EXE_SHA, OVR_SHA, physical, put16

CHECKER_ROWS = ('OVL_03BD_057A',)
ENTRY, END = 0x057A, 0x05F3
LEAF, LEAF_END = 0x031F, 0x0350
W_CS = 0x6000
SP_MIN, SP_MAX = 0x0010, 0xFFF8
SOURCES = ['overlay_u3bd_cell_057a_cpu', 'overlay_u3bd_in_grid_031f', 'memory_bus', 'ega_hw']
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'TRANSFER'}
MAP, BUFPTR = 0x84DC, 0x6A5C
ENTRY_RET = A.ENTRY_RET
check = A.check


class Bus(ctypes.Structure):
    _fields_ = [('ram', ctypes.c_void_p), ('ega', ctypes.c_void_p), ('ports', ctypes.c_void_p), ('on_vram_write', ctypes.c_void_p), ('ctx', ctypes.c_void_p)]


def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x3BD)
    check(unit['entries'][4] == ENTRY, 'public entry 4 is +057A')
    code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
    body, leaf = code[ENTRY:END], code[LEAF:LEAF_END]
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    ins = list(md.disasm(body, ENTRY)); lins = list(md.disasm(leaf, LEAF))
    check(sum(i.size for i in ins) == len(body) and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '4', 'body decode covers [057A,05F3) and ends in RETF 4')
    check(sum(i.size for i in lins) == len(leaf) and lins[-1].mnemonic == 'retf' and lins[-1].op_str == '4', 'leaf decode covers [031F,0350) and ends in RETF 4')
    check([(i.address, int(i.op_str, 16)) for i in ins if i.mnemonic == 'call'] == [(0x0589, LEAF)], 'the one call is CALL +031F at 0589')
    check(not [i for i in ins + lins if i.mnemonic in ('lcall', 'int', 'in', 'out')], 'no far call, interrupt or port instruction')
    check(not [f for f in unit['fixups'] if LEAF <= f < LEAF_END or ENTRY <= f < END], 'no relocation inside either span')
    meta = dict(bytes=len(body), instructions=len(ins), sha256=sha256(body).hexdigest(), leaf_bytes=len(leaf), leaf_instructions=len(lins),
                leaf_sha256=sha256(leaf).hexdigest(), file_offset=unit['ovr_offset'] + ENTRY, memory_segment=unit['memory_segment'])
    return {('w', ENTRY): body, ('w', LEAF): leaf}, {('w', i.address): i for i in ins + lins}, {('w', i.address): i for i in ins}, meta


def library(directory, override=None):
    path = Path(directory) / ('u3bd_057a_mut.so' if override else 'u3bd_057a.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    lib.pr_u3bd_cell_057a_cpu.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu)]; lib.pr_u3bd_cell_057a_cpu.restype = ctypes.c_int
    return lib


def run_c(lib, state, budget=None):
    memory, cpu, _ = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(memory); out = Cpu.from_buffer_copy(cpu); bus = Bus(ctypes.addressof(ram), None, None, None, None)
    status = lib.pr_u3bd_cell_057a_cpu(ctypes.byref(bus), SIZE, ctypes.byref(out))
    return STATUS[status], bytes(ram), out, None, None


def fixture(bodies, *, x=5, y=3, xhi=0xCD, yhi=0xCD, mapid=1, buf=(0x3000, 0x0100), fill=None, sp=0x8000, ds=0x1000, ss=0x2000, w_cs=W_CS, flags=0x0202, regs=None,
            background=0xA5, seed=0):
    """Entry frame: return IP, CS; [SP+4] the y word slot, [SP+6] the x word slot. DS:[84DC] the map id; DS:[6A5C] the far buffer pointer (offset, segment)."""
    m = bytearray([background]) * SIZE
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, w_cs, ENTRY, ds, r[7], ss, flags | 0x0002)
    rng = random.Random(seed)
    base = physical(buf[0], 0)
    for k in range(0x400):                                                        # a recognisable buffer: every cell distinct
        m[physical(buf[0], (buf[1] + k) & 0xFFFF)] = fill if fill is not None else (k * 7 + 3 + rng.randrange(256)) & 0xFF
    m[physical(ds, MAP)] = mapid
    put16(m, ds, BUFPTR, buf[1]); put16(m, ds, BUFPTR + 2, buf[0])
    for (kind, start), blob in bodies.items():
        at = physical(w_cs, start); m[at:at + len(blob)] = blob
    for k, w in enumerate((ENTRY_RET[1], ENTRY_RET[0], (yhi << 8) | (y & 0xFF), (xhi << 8) | (x & 0xFF))):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, w)
    return m, cpu, dict(w_cs=w_cs, resident_cs=0, sp=sp, ss=ss, ds=ds, x=x, y=y, mapid=mapid, buf=buf)


def compare(label, state, bodies, decoded, lib):
    actual = run_c(lib, state); expected = A.run_original(state, decoded, 100000)
    check(actual[0] == expected[0], f'{label}: status C {actual[0]} original {expected[0]}')
    for name, _ in Cpu._fields_:
        check(getattr(actual[2], name) == getattr(expected[2], name), f'{label}: {name} C {getattr(actual[2], name):04x} original {getattr(expected[2], name):04x}')
    if actual[1] != expected[1]:
        diffs = [i for i in range(SIZE) if actual[1][i] != expected[1][i]]
        raise ValueError(f'{label}: RAM {[(hex(i), actual[1][i], expected[1][i]) for i in diffs[:12]]} ({len(diffs)} bytes)')
    return expected, actual


def cases(quick=False):
    rng = random.Random(0x3BD057A)
    out = []

    def add(name, **kw):
        out.append((name, kw))
    for v in range(256):                                               # every x, then every y, across both signed wrap boundaries
        add(f'x-{v:02x}', x=v, y=3); add(f'y-{v:02x}', x=5, y=v)
    for x, y in ((0, 0), (15, 15), (16, 16), (0x7F, 0x80), (0x80, 0x7F), (0xFF, 0xFF), (15, 16), (16, 15)):
        add(f'xy-{x:02x}-{y:02x}', x=x, y=y)
    for mapid in (0, 10, 1, 9, 11, 0xFF):                              # the gate: an out-of-grid pair on maps 0 and 10 returns 0; others wrap and look up
        add(f'map-{mapid:02x}-out', x=16, y=3, mapid=mapid); add(f'map-{mapid:02x}-in', x=4, y=3, mapid=mapid)
    for off in (0x0000, 0xFC00, 0xFDFF, 0xFE00, 0xFEFF, 0xFFFF):          # buffer offset wraps at 16 bits before +200h and the cell offset are added
        add(f'buf-off-{off:04x}', buf=(0x3000, off), x=15, y=15)
    for seg in (0xFFFF, 0xFFF0, 0x0000, 0xA000):
        add(f'buf-seg-{seg:04x}', buf=(seg, 0x0100), x=2, y=2)
    for h in (0x00, 0x80, 0xFF):
        add(f'highs-{h:02x}', xhi=h, yhi=h, x=0x8F, y=0x10)
    for sp in (SP_MIN, SP_MIN + 1, 0x0101, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp)
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202, 0x0ED7):
        add(f'flags-{fl:04x}', flags=fl); add(f'flags-{fl:04x}-zero', flags=fl, x=0x20, mapid=0)
    for w in (0x6000, 0x2345, 0xF000):
        add(f'at-{w:04x}', w_cs=w)
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x12, 0x0A):                                       # DS = SS with the map byte on each frame byte (below BP the leaf's own frame writes it)
        add(f'map-on-bp{k:+03x}', ds=0x2000, ss=0x2000, sp=(MAP - k + 2) & 0xFFFF, x=0x20, y=3)
    for k in range(-0x12, 0x0A):                                       # DS = SS with the buffer pointer on each frame byte
        add(f'bufptr-on-bp{k:+03x}', ds=0x2000, ss=0x2000, sp=(BUFPTR - k + 2) & 0xFFFF, x=0x11, y=0xF0)
    add('buffer-is-frame', buf=(0x2000, (0x8000 - 0x200 - 3 * 16 - 5) & 0xFFFF), x=5, y=3)       # the looked-up cell is the frame's own x slot after wrapping
    for k in range(40 if quick else 240):
        add(f'random{k}', x=rng.randrange(256), y=rng.randrange(256), xhi=rng.randrange(256), yhi=rng.randrange(256), mapid=rng.choice((0, 10, rng.randrange(256))),
            buf=(rng.randrange(0x1000, 0x9000), rng.randrange(65536)), regs=tuple(rng.randrange(65536) for _ in range(8)),
            flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1), w_cs=rng.choice((0x6000, 0x2345)), seed=k)
    return out


def expected_transfer(name):
    return name.startswith(('map-on-bp', 'bufptr-on-bp'))


def statements(name, st, expected):
    """Independent of the C model, for cases whose globals and buffer do not overlap the stack: AL from the documented rule alone, SP/BP/CS:IP on return."""
    status, ram, out, _, _, _ = expected
    if status != 'RETURNED':
        return 0
    memory, cpu, info = st
    if info['ds'] == info['ss'] or info['buf'][0] == info['ss']:
        return 0
    sx, sy = (info['x'] ^ 0x80) - 0x80, (info['y'] ^ 0x80) - 0x80
    inside = 0 <= sx <= 15 and 0 <= sy <= 15
    if not inside and info['mapid'] in (0, 10):
        want = 0
    else:
        wx = 0 if sx > 15 else 15 if sx < 0 else sx; wy = 0 if sy > 15 else 15 if sy < 0 else sy
        want = memory[physical(info['buf'][0], (info['buf'][1] + wy * 16 + wx + 0x200) & 0xFFFF)]
    check(out.ax & 0xFF == want, f'{name}: AL {out.ax & 0xFF:02x} rule {want:02x}')
    check(out.sp == (cpu.sp + 8) & 0xFFFF and out.bp == cpu.bp and (out.cs, out.ip) == ENTRY_RET, f'{name}: SP/BP/CS:IP on return')
    return 1


def differential(bodies, decoded, body_ips, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen = set(); stated = 0; edges = set()
    conds = {a: (int(i.op_str, 16), a + i.size) for (k, a), i in body_ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw) in enumerate(cs):
        if index % 100 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual = compare(name, st, bodies, decoded, lib)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen |= expected[4]
        ips = sorted(a for (k, a) in expected[4] if ENTRY <= a < END)
        for a in conds:                                                 # outcome: which of the two successors was reached in this case
            if ('w', a) in expected[4]:
                for t in conds[a]:
                    if ('w', t) in expected[4] and t not in (a,):
                        edges.add((a, t))
        if actual[0] == 'ARGUMENT' or (actual[0] == 'TRANSFER' and not expected_transfer(name)):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        stated += statements(name, st, expected)
    body_seen = {k for k in seen if k in body_ips}
    check(only is not None or body_seen == set(body_ips), f'body starts not reached: {sorted(hex(a) for (_, a) in set(body_ips) - body_seen)}')
    outcomes = {(a, t) for a, ts in conds.items() for t in ts}
    check(only is not None or outcomes <= edges, f'outcomes not reached: {sorted(outcomes - edges)}')
    return dict(cases=len(cs), end_states=stops, body_starts=f'{len(body_seen)}/{len(body_ips)}', leaf_starts=f'{len(seen) - len(body_seen)}/{len(decoded) - len(body_ips)}',
                outcomes=f'{len(outcomes & edges)}/{len(outcomes)}', independent_statements_checked=stated, cpu_words=14, ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


MUTANT_CASES = ('x-10', 'x-0f', 'x-ff', 'x-80', 'y-10', 'y-ff', 'xy-7f-80', 'map-00-out', 'map-0a-out', 'map-09-out', 'map-01-in', 'buf-off-fdff', 'buf-off-ffff', 'highs-ff',
                'flags-08d7', 'flags-08d7-zero', 'flags-0003', 'sp-0010', 'map-on-bp-02', 'bufptr-on-bp+00', 'buffer-is-frame', 'random0', 'random1', 'random2')
MUTANTS = (
    ('the cell is at +1FFh', [("(uint16_t)(c->di + 0x0200u)", "(uint16_t)(c->di + 0x01FFu)")]),
    ('rows are 8 wide (SHL by 3)', [("shl16(c, 4u);", "shl16(c, 3u);")]),
    ('x and y swapped in the address', [("mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 8u))); cbw(c); c->dx = c->ax;", "mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); cbw(c); c->dx = c->ax;")]),
    ('the wrap uses unsigned compares', [("if ((int8_t)v > 15)", "if (v > 15)")]),
    ('map 0Ah does not gate', [("if (map == 0x0Au) goto zero;", "(void)0;")]),
    ('the map byte is read once', [("map = rd8(ram, c->ds, 0x84DCu); cmp8(c, map, 0x0Au);", "cmp8(c, map, 0x0Au);")]),
    ('SHL leaves AF clear (the u3bd fullstate convention)', [("f = parity((uint8_t)r) | 0x0010u;", "f = parity((uint8_t)r);")]),
    ('ADD DI,DX flags not computed', [("c->di = add16(c, c->di, c->dx);", "c->di = (uint16_t)(c->di + c->dx);")]),
    ('the result is not stored to [BP-1]', [("wr8(ram, c->ss, (uint16_t)(c->bp - 1u), (uint8_t)c->ax);                     /* 05E7 MOV [BP-1],AL */", "(void)0;")]),
    ('RETF 4 pops 2', [("c->sp = (uint16_t)(c->sp + 4u);     /* 05F0 RETF 4 */", "c->sp = (uint16_t)(c->sp + 2u);     /* 05F0 RETF 4 */")]),
    ('the buffer read bypasses the bus', [("mov_al(c, pr_bus_read8(bus, physical(c->es, (uint16_t)(c->di + 0x0200u))));", "mov_al(c, ram[physical(c->es, (uint16_t)(c->di + 0x0200u))]);")]),
)


def c_test_verdict(tmp, mutant_path):
    exe = Path(tmp) / 'ctest'
    srcs = [str(mutant_path if s == 'overlay_u3bd_cell_057a_cpu' else ROOT / f'src/recovered/{s}.c') for s in SOURCES]
    try:
        subprocess.run(['cc', '-std=c11', '-O1', '-fsanitize=address,undefined', '-I', str(ROOT / 'include'), str(ROOT / 'tests/recovered/test_overlay_u3bd_cell_057a_cpu.c')] + srcs +
                       ['-o', str(exe)], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        return ' (the C test did not build with it)'
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    return ': CAUGHT BY THE C TEST' if r.returncode != 0 else ': the C test also passes'


def mutation_check(bodies, decoded, body_ips):
    out = []; src = (ROOT / 'src/recovered/overlay_u3bd_cell_057a_cpu.c').read_text()
    for k, (name, edits) in enumerate(MUTANTS):
        t0 = time.time(); print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        text = src
        for old, new in edits:
            check(text.count(old) >= 1, f'mutant text occurs: {name}: {old[:60]}'); text = text.replace(old, new)
        with tempfile.TemporaryDirectory(prefix='u3bd-057a-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(text)
            try:
                mlib = library(tmp, override={'overlay_u3bd_cell_057a_cpu': path})
            except subprocess.CalledProcessError:
                out.append((name, 'COMPILE-ERROR')); continue
            try:
                differential(bodies, decoded, body_ips, mlib, quick=True, only=MUTANT_CASES)
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
    with tempfile.TemporaryDirectory(prefix='u3bd-057a-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, body_ips, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, body_ips, lib, a.quick)
        if a.native:
            from ovl_03bd_057a_native import native
            report['native'] = native(bodies, decoded, lib)
        if a.mutate:
            report['mutation'] = mutation_check(bodies, decoded, body_ips)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_03bd_057a_cpu_audit'] = sys.modules[__name__]
    main()
