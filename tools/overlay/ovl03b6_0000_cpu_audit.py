#!/usr/bin/env python3
"""OVL_03B6_0000 (GAME.OVR unit 3B6h map window) and OVL_03CE_00F1 (unit 3CEh tile placement) against the original bytes,
as a composed closure: 03B6 +0000, overlay stub 03E9:002A in its loaded state (JMP FAR to unit 3CEh +00F1), 03CE +00F1,
0464:08D4 (the tile compositor) and SYSTEM Move 1685 (see res0464_compose_08d4_cpu_audit.py and ovl040c_015d_cpu_audit.py
for that closure's environment, reused here unchanged).

The loader-expanded START.EXE is loaded at a test base with the loader's relocation table applied; each unit's code is
loaded at its own segment with its fixups applied, and the stub entry holds what the overlay manager writes when unit 3CEh
is resident (EAh, offset 00F1h, the unit's segment). An unloaded entry (INT 3Fh) is a separate case: both sides stop at the
stub with OVERLAY. 00F1 is also entered directly. Unicorn: CPU14 + full 1 MiB + stop, unmasked; no interrupt, port or
environment event is allowed on either side."""
import argparse, ctypes, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, REGISTERS, physical, put16, check
from overlay_inventory import find_units, parse_mz
import ovl040c_015d_cpu_audit as O

Q, C8 = O.Q, O.C8
CHECKER_ROWS = ('OVL_03CE_00F1', 'OVL_03B6_0000')
STUBSEG, STUB_ENTRY = 0x03E9, 0x002A
B6, CE = (0x0000, 0x0206), (0x00F1, 0x014E)
BASE, SS = O.BASE, O.SS
OVSEG, B6SEG, WINDOW0 = O.OVSEG, 0x5200, 0x5400       # unit 3CEh code, unit 3B6h code, the far DS:49D2 block
RETURN_TO = O.RETURN_TO
FLAGS = O.FLAGS
SOURCES = ['memory_bus', 'ega_hw', 'system_string_move_leaves_cpu', 'resident_0464_compose_08d4_cpu',
           'overlay_u3ce_tile_00f1_cpu', 'overlay_u3b6_window_0000_cpu']
STATUS_CE = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'TRANSFER'}
STATUS_B6 = {0: 'RETURN', 1: 'ARGUMENT', 2: 'CALLEE', 3: 'TRANSFER', 4: 'LIMIT', 5: 'OVERLAY'}
STUB_UNLOADED = bytes.fromhex('cd3ff10000')
MAP, RECORDS, WFIELDS = 0x3648, 0x0C30, (0x2F8, 0x318, 0x33E, 0x340, 0x366, 0x3A4)
SET_LOW, SET_HIGH = (0x0100, O.TILES), (0x0100, O.TILES + 0x100)   # far DS:5E52 and DS:5E56: two of 015D's tile sets


class EnvCE(ctypes.Structure):
    _fields_ = [('max_loopbacks', ctypes.c_size_t)]


class DiagCE(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('status', ctypes.c_int), ('calls', ctypes.c_uint), ('compose', C8.Diagnostic)]


class EnvB6(ctypes.Structure):
    _fields_ = [('tile', EnvCE), ('max_loopbacks', ctypes.c_size_t)]


class DiagB6(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('status', ctypes.c_int), ('calls', ctypes.c_uint), ('tile', DiagCE)]


def units():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    found = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    out = {}
    for para, size, entries in ((0x3CE, 0x645, [0x640, 0x22, 0xF1, 0x14E, 0x1C8, 0x4FC]), (0x3B6, 0x288, [0x283, 0x0, 0x207])):
        u = found[para]; code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
        check(sha256(code).hexdigest() == u['code_sha256'] and len(code) == size, f'unit {para:X}h code identity')
        check(list(u['entries']) == entries, f'unit {para:X}h entries')
        out[para] = (code, list(u['fixups']))
    return out


UNITS = None


def unit(para):
    global UNITS
    if UNITS is None:
        UNITS = units()
    return UNITS[para]


def decode(code, span, count, retf):
    body = code[span[0]:span[1]]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, span[0]))
    check(len(ins) == count and sum(i.size for i in ins) == len(body), f'decode covers {span[0]:04X}')
    check(ins[-1].mnemonic == 'retf' and ins[-1].op_str == retf and [i.mnemonic for i in ins].count('retf') == 1, 'single RETF at the end')
    check(not [i for i in ins if i.mnemonic in ('call', 'ret', 'int', 'out', 'in')], 'no near call, RET, INT or port access')
    return body, {i.address: i for i in ins}


def source(image):
    code_ce, fix_ce = unit(0x3CE); code_b6, fix_b6 = unit(0x3B6)
    body_ce, ips_ce = decode(code_ce, CE, 38, '8')
    body_b6, ips_b6 = decode(code_b6, B6, 211, '4')
    check([(a, i.op_str) for a, i in ips_ce.items() if i.mnemonic == 'lcall'] == [(0x011D, '0x464, 0x8d4'), (0x0145, '0x464, 0x8d4')], '00F1 far calls')
    check([(a, i.op_str) for a, i in ips_b6.items() if i.mnemonic == 'lcall'] == [(0x01E1, '0x3e9, 0x2a')], '0000 far call')
    check(0x0120 in fix_ce and 0x0148 in fix_ce and 0x01E4 in fix_b6, 'every far-call segment word is a fixup')
    check(image[STUBSEG * 16 + STUB_ENTRY:STUBSEG * 16 + STUB_ENTRY + 5] == STUB_UNLOADED, 'stub entry 2 is INT 3Fh +00F1 (unit 3CEh)')
    return dict(ce=(body_ce, ips_ce), b6=(body_b6, ips_b6))


def library(directory):
    path = directory / 'ovl03b6.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str(ROOT / f'src/recovered/{s}.c') for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    fce, fb6 = lib.pr_ovl03ce_00f1_cpu, lib.pr_ovl03b6_0000_cpu
    fce.argtypes = [ctypes.POINTER(O.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(EnvCE), ctypes.POINTER(DiagCE)]
    fb6.argtypes = [ctypes.POINTER(O.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(EnvB6), ctypes.POINTER(DiagB6)]
    fce.restype = fb6.restype = ctypes.c_int
    return fce, fb6


def build(image_reloc, code, fixups, *, entry='b6', x0=3, y0=2, records=(), window0=None, tiles=None, tile_seed=1,
          flag=0x0001, tile=0x10, col=6, row=3, loaded=True, set_low=SET_LOW, set_high=SET_HIGH, set_rows=2, set_width=2,
          set_tile=0x10, sp=0xF000, flags=2,
          cpu=None, ovseg=OVSEG, b6seg=B6SEG, **o):
    """code/fixups: unit 3CEh (the native driver passes them through). entry 'b6' calls 03B6 +0000 with (y0, x0);
    'ce' calls 03CE +00F1 with (flag, tile, col, row). records: (i, x, y, tile) for DS:0C30 + 6i; the rest stay at 0200h
    (never a byte coordinate). window0: field -> word at far DS:49D2. tiles: map bytes (default from tile_seed)."""
    m, c, env, keys, stubs = O.build(image_reloc, code, fixups, sp=sp, flags=flags, cpu=cpu, ovseg=ovseg, **o)
    ds = BASE + O.SEG['data']
    put16(m, ds, 0x5E52, set_low[0]); put16(m, ds, 0x5E54, set_low[1])
    put16(m, ds, 0x5E56, set_high[0]); put16(m, ds, 0x5E58, set_high[1])
    # Both tile sets are written here, as 015D's fixture writes a band's (it leaves the band numbered `fill` unwritten,
    # which as a source here would be a zero row count, 65,536 rows): rows, width, tile size, aux far pointer, data.
    for n, (off, seg) in enumerate((set_low, set_high)):
        put16(m, seg, off, set_rows); put16(m, seg, off + 2, set_width); put16(m, seg, off + 0x11, set_tile)
        put16(m, seg, off + 0x13, O.AUX[0]); put16(m, seg, off + 0x15, O.AUX[1])
        for k in range(0x700):
            m[physical(seg, off + 0x17 + k)] = (k * 29 + 7 + n) & 255
    rng = random.Random(tile_seed)
    for k in range(0x2C * 0x30):
        m[physical(ds, MAP + k)] = tiles[k % len(tiles)] if tiles else rng.choice((1, 2, 0x20, 0x41, 0x7F, 0x80, 0x81, 0xC0, 0xFF, 0x00, rng.randrange(256)))
    for i in range(1, 10):
        put16(m, ds, RECORDS + 6 * i, 0x0200); put16(m, ds, RECORDS + 6 * i + 2, 0x0200); put16(m, ds, RECORDS + 6 * i + 4, 0x30 + i)
    for i, x, y, t in records:
        put16(m, ds, RECORDS + 6 * i, x); put16(m, ds, RECORDS + 6 * i + 2, y); put16(m, ds, RECORDS + 6 * i + 4, t)
    put16(m, ds, 0x49D2, 0x0000); put16(m, ds, 0x49D4, WINDOW0)
    w = dict(zip(WFIELDS, (0, 0, 0, 0, 0, 0)))
    w.update(window0 or {})
    for f, v in w.items():
        put16(m, WINDOW0, f, v)
    code_b6, fix_b6 = unit(0x3B6)
    b6 = bytearray(code_b6)
    for f in fix_b6:
        struct.pack_into('<H', b6, f, (struct.unpack_from('<H', b6, f)[0] + BASE) & 0xFFFF)
    at = b6seg * 16; m[at:at + len(b6)] = b6
    at = physical(BASE + STUBSEG, STUB_ENTRY)
    m[at:at + 5] = (bytes([0xEA]) + struct.pack('<HH', 0x00F1, ovseg)) if loaded else STUB_UNLOADED
    args = (y0, x0) if entry == 'b6' else (flag, tile, col, row)
    for k, v in enumerate((RETURN_TO[1], RETURN_TO[0]) + args):
        put16(m, SS, (sp + 2 * k) & 0xFFFF, v & 0xFFFF)
    c.cs, c.ip = (b6seg, B6[0]) if entry == 'b6' else (ovseg, CE[0])
    return m, c, env, keys, stubs


def run_c(f, m, c, envspec, keys, stubs, max_loopbacks=1 << 20):
    fce, fb6 = f
    actual = Cpu.from_buffer_copy(c); ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    bus = O.Bus(ram, None, None, C8.WRITE_FN(), None)
    if c.ip == CE[0]:
        d = DiagCE(); e = EnvCE(1 << 20)
        status = STATUS_CE.get(fce(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d)))
    else:
        d = DiagB6(); e = EnvB6(EnvCE(1 << 20), max_loopbacks)
        status = STATUS_B6.get(fb6(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(e), ctypes.byref(d)))
    return status, actual, bytes(ram), Q.Env(**envspec), d


def compare(name, state, ips, f, o):
    m, c, envspec, keys, stubs = state
    ovseg, b6seg = o.get('ovseg', OVSEG), o.get('b6seg', B6SEG)
    span = (ovseg, *CE) if c.ip == CE[0] else (b6seg, *B6)
    uc, backing, uenv, ustop, seen, edges = O.run_unicorn(name, m, c, envspec, keys, ips, body=span, extra=(BASE + STUBSEG, ovseg, b6seg))
    status, actual, ram, cenv, d = run_c(f, m, c, envspec, keys, stubs)
    check(status == ustop, f'C status {name}: {status} original {ustop} (site {d.site:04x} {d.status})')
    check(uenv.log == [], f'environment events {name}: {uenv.log[:6]}')
    if ram != bytes(backing):
        diff = [hex(i) for i, (a, b) in enumerate(zip(ram, bytes(backing))) if a != b]
        raise ValueError(f'RAM {name}: {diff[:8]} ({len(diff)} bytes)')
    for (n, _), reg in zip(Cpu._fields_, REGISTERS):
        w = uc.reg_read(reg) & 0xFFFF
        check(getattr(actual, n) == w, f'CPU {name}/{n} C {getattr(actual, n):04x} original {w:04x}')
    return ustop, seen, edges, d


# The nine records' conditions on the window-0 words (record 6 always replaces; 1 and 2 never do).
GATES = {3: [({0x318: 0x00FF}, False), ({0x318: 0x00FE}, True)],
         4: [({0x3A4: 1}, True), ({0x2F8: 8}, True), ({0x33E: 1}, False), ({}, True)],
         5: [({0x340: 2}, False), ({0x340: 1}, True)],
         7: [({0x340: 1}, False), ({0x340: 2}, True)],
         8: [({0x366: 0xFD}, False), ({0x366: 0xFE}, True)],
         9: [({0x366: 0xFE}, True), ({0x366: 0x8000}, True), ({0x366: 0x0000}, False)]}


def cases():
    out = [('ce-low', dict(entry='ce', tile=0x10)), ('ce-7f', dict(entry='ce', tile=0x7F)), ('ce-80', dict(entry='ce', tile=0x80)),
           ('ce-ff', dict(entry='ce', tile=0xFF)), ('ce-high-byte', dict(entry='ce', tile=0xA512, flag=0x5A00)),
           ('ce-neg-xy', dict(entry='ce', tile=0x22, col=0x00FF, row=0x0081)), ('ce-mode1', dict(entry='ce', mode=1)),
           ('ce-mode2', dict(entry='ce', tile=0x85, mode=2)), ('ce-mode3', dict(entry='ce', mode=3))]
    out += [(f'ce-flags{f:x}', dict(entry='ce', tile=0x40 + k * 0x11, flags=f)) for k, f in enumerate(FLAGS)]
    out += [('origin', dict()), ('origin-zero', dict(x0=0, y0=0)), ('origin-far', dict(x0=36, y0=40)),
            ('wrap-7d', dict(x0=0x7D, y0=0x7E)), ('negative', dict(x0=0xFFFE, y0=0x00FD)), ('high-words', dict(x0=0xA503, y0=0x5A02)),
            ('unloaded', dict(loaded=False)), ('mode1', dict(mode=1)), ('mode2', dict(mode=2)), ('mode3', dict(mode=3)),
            ('all-high-tiles', dict(tiles=(0x00, 0x81, 0xC0, 0xFF))), ('all-low-tiles', dict(tiles=(0x01, 0x02, 0x7F, 0x80))),
            ('ovseg-high', dict(ovseg=0x9800, b6seg=0x9A00)), ('wide-tiles', dict(set_rows=3, set_width=4, set_tile=0x20)),
            ('ce-wide-tiles', dict(entry='ce', tile=0x93, set_rows=4, set_width=3, set_tile=0x18))]
    for i in (1, 2, 6):
        out.append((f'record{i}', dict(records=((i, 4, 3, 0x91),))))
    for i, gates in GATES.items():
        for k, (w, _) in enumerate(gates):
            out.append((f'record{i}-gate{k}', dict(records=((i, 5, 4, 0x20 + i),), window0=w)))
    out += [('record-x-only', dict(records=((6, 4, 9, 0x77),))), ('record-y-only', dict(records=((6, 9, 3, 0x77),))),
            ('record-neg-x', dict(x0=0xFFFE, records=((6, 0xFFFF, 2, 0x55),))), ('record-neg-y', dict(y0=0xFFFE, records=((6, 3, 0xFFFE, 0x55),))),
            ('record-tile-zero', dict(records=((6, 4, 3, 0),))), ('record-tile-high', dict(records=((6, 4, 3, 0x1234),))),
            ('records-override', dict(records=((3, 4, 3, 0x10), (6, 4, 3, 0x20), (9, 4, 3, 0x30)), window0={0x366: 0xFF})),
            ('records-all', dict(records=tuple((i, 3 + (i % 5), 2 + (i // 5), 0x40 + i) for i in range(1, 10)),
                                 window0={0x318: 1, 0x3A4: 1, 0x366: 0xFFFF}))]
    out += [(f'flags{f:x}', dict(flags=f)) for f in FLAGS]
    rng = random.Random(0x03B6)
    for k in range(30):
        x0, y0 = rng.randrange(0, 40), rng.randrange(0, 44)
        recs = tuple((i, x0 + rng.randrange(5), y0 + rng.randrange(5), rng.randrange(0x100)) for i in range(1, 10) if rng.random() < 0.4)
        out.append((f'random{k}', dict(x0=x0, y0=y0, records=recs, window0={f: rng.choice((0, 1, 2, 8, 0xFD, 0xFE, 0xFF, rng.randrange(65536)))
                                                                            for f in WFIELDS},
                                       mode=rng.choice((0, 0, 2, 3)), flags=rng.choice(FLAGS), cpu=tuple(rng.randrange(65536) for _ in range(7)),
                                       fill=rng.choice((0x5A, 0x00, rng.randrange(256))), tile_seed=k, seed=k)))
    return out


def native_cases():
    """Every case except three groups. The unloaded stub executes INT 3Fh, the overlay manager's interrupt, which the bare oracle does not install.
    The relocated-segment case puts both units outside the oracle's loaded regions. The random cases follow the 015D and
    03F1 precedent: their writes are not shown to stay inside the loaded regions, and a stray store hangs DOSBox.
    All four video modes run natively: 08D4 writes only its destination object and the DS metadata (mode 1 is the
    nibble path, values other than 0, 1 and 2 return at once)."""
    return [(n, o) for n, o in cases() if o.get('loaded', True) and 'ovseg' not in o and not n.startswith('random')]


SMOKE = ('ce-low', 'ce-7f', 'ce-80', 'ce-ff', 'ce-neg-xy', 'origin', 'negative', 'record-neg-x', 'record-neg-y', 'record6',
         'record3-gate0', 'record3-gate1', 'record8-gate0', 'record8-gate1', 'records-all', 'flags8d7')


def differential(image_reloc, code, fixups, srcs, f, only=None):
    """only: case names to run (the mutation smoke subset); coverage is then not required."""
    stops = {}; seen = {'ce': set(), 'b6': set()}; edges = {'ce': set(), 'b6': set()}; calls = 0
    for name, o in cases():
        if only is not None and name not in only:
            continue
        which = o.get('entry', 'b6')
        state = build(image_reloc, code, fixups, **o)
        ustop, s, e, d = compare(name, state, srcs[which][1], f, o)
        stops[ustop] = stops.get(ustop, 0) + 1; seen[which] |= s; edges[which] |= e
        calls += d.calls
    report = dict(cases=sum(stops.values()), stops=stops, tile_calls=calls, cpu_words=14, ram_bytes=SIZE, flag_masks=0)
    if only is not None:
        return report
    for which, row in (('ce', 'OVL_03CE_00F1'), ('b6', 'OVL_03B6_0000')):
        ips = srcs[which][1]
        conds = {a: (int(i.op_str, 16), a + i.size) for a, i in ips.items() if i.mnemonic.startswith('j') and i.mnemonic != 'jmp'}
        outcomes = {(a, t) for a, ts in conds.items() for t in ts}
        check(set(ips) <= seen[which], f'{row} starts not reached: {[hex(a) for a in sorted(set(ips) - seen[which])]}')
        check(outcomes <= edges[which], f'{row} outcomes not reached: {sorted((hex(a), hex(t)) for a, t in outcomes - edges[which])}')
        report[row] = dict(starts=f'{len(seen[which])}/{len(ips)}', outcomes=f'{len(outcomes & edges[which])}/{len(outcomes)}')
    return report


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        f = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = Q.load_image()
        image_reloc = Q.relocate(image, sites, BASE)
        srcs = source(image)
        code, fixups = unit(0x3CE)
        report = dict(image_sha256=Q.IMAGE_DIGEST, load_base=f'{BASE:04X}', relocations=len(sites),
                      bodies={row: dict(span=f'unit {u}h +[{s[0]:04X},{s[1]:04X})', bytes=len(srcs[k][0]), instructions=len(srcs[k][1]),
                                        sha256=sha256(srcs[k][0]).hexdigest())
                              for row, u, s, k in (('OVL_03CE_00F1', '3CE', CE, 'ce'), ('OVL_03B6_0000', '3B6', B6, 'b6'))})
        if a.unicorn:
            report['unicorn'] = differential(image_reloc, code, fixups, srcs, f)
        if a.native:
            import ovl040c_015d_cpu_native as N
            regions = N.REGIONS + (((BASE + STUBSEG) * 16, 0x40),)
            report['native'] = N.native(sys.modules[__name__], code, fixups, None, f, regions=regions,
                                        directory=ROOT / 'resources/ovl03b6-0000/native')
    text = json.dumps(report, indent=1, sort_keys=True)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(text + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
    for _row in ('OVL_03B6_0000', 'OVL_03CE_00F1'):
        print(f"ROW_EXECUTED {_row} PASS")
