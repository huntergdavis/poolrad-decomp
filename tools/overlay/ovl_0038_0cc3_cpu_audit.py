#!/usr/bin/env python3
"""OVL_0038_0CC3 (unit 0x38 [0CC3,0FA2), the U9E dispatcher's slot-40 handler) composed with SYSTEM StrLoad, the resident 00BA:1693
display closure and SYSTEM FreeMem, against the original bytes.

The body finds, in the far-pointer list at DS:[6DBB], the node matching a 32-bit key and the type nibble of an object record; shows a
message; restores each of the node's four map cells (1Fh where an entry of the 7-byte table at DS:66D6 sits on the cell, otherwise the
node's saved byte); unlinks the node; frees it with FreeMem; and re-stamps 1Eh on every cell of every remaining node. Every callee runs
as its ORIGINAL bytes in the oracle and as its merged C model in the C run. The world is the unit-0x7F world's (the 1693 audit's EGA
device and display state, StrLoad) with this span at the unit segment, the FreeMem spans at the SYSTEM segment, and a Turbo Pascal heap
under the world's DS. Compared after every case: CPU14, ALL 1,048,576 RAM bytes, the EGA planes, latches and registers, the ordered OUT
events and VRAM operands, the stop and the remaining budget. FLAGS are compared exactly except under the shared declared splits
(pinned MULW, AF after SHR; tools/overlay/pinned_shift_flags.py), counted. FreeMem's RunError 204 is a named stop at SYSTEM:00E2 in
both runs. Scope, declared: the budget counts this body only; the 1693 closure's and FreeMem's own caps are generous and not
exercised here. Media stay under ignored resources/."""
import argparse, contextlib, ctypes, io, json, random, subprocess, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_007f_world1693 as WD
import res00ba_display_1693_cpu_audit as D
import system_freemem_0254_audit as FM
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack
from res0709_printer_0626_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

CHECKER_ROWS = ('OVL_0038_0CC3',)
ENTRY, END = 0x0CC3, 0x0FA2
SPAN_SHA = '21a1e6b4a6ce76ceb6fd1283552e752a5cf32c732cd104d50539e1f4e0f8ee89'
MSG = (0x0CA8, 27, '17fa53bb9131d10c69ad2371da75a9cebf83ae79e1ab9721f2df08426a15ad18')   # length byte 26 + text; plain data, no relocation
RELOC = {0x0D47: 'sys', 0x0D4C: 'display', 0x0EFA: 'sys'}
SP_MIN, SP_MAX = 0x0200, 0xFFF2
STATUS = {0: 'RETURNED', 1: 'ARGUMENT', 2: 'CODE', 3: 'LIMIT', 4: 'TRANSFER', 5: 'CALLEE', 6: 'RUNERROR'}
BUDGET, ALIAS_BUDGET = 200000, 60000
REC = (0x1000, 0x0100)                 # the object record the dispatcher hands over
GRID = (0xC000, 0x1000)                # the map behind DS:[6716] (50 bytes a row; byte +7 of a cell is written)
HEAP_ORG, HEAP_TOP, HEAP_END = (0x3000, 0), (0x3400, 0), (0x3800, 0)
SOURCES = list(dict.fromkeys(WD.SOURCES + ['overlay_u38_0cc3_cpu', 'system_freemem_0254_cpu']))


class Limits(ctypes.Structure):
    _fields_ = [('clear', ctypes.c_size_t), ('rect', ctypes.c_size_t), ('printer', ctypes.c_size_t), ('glyph', ctypes.c_size_t),
                ('delay', ctypes.c_uint64), ('freemem_steps', ctypes.c_uint)]


class Diag(ctypes.Structure):
    _fields_ = [('raw', ctypes.c_uint8 * 4096)]


def library(directory, override=None):
    path = Path(directory) / ('u38_mut.so' if override else 'u38.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES if (ROOT / f'src/recovered/{s}.c').exists() or s in (override or {})] +
                   ['-o', str(path)], check=True, capture_output=True)
    lib = ctypes.CDLL(str(path)); lib.pr_ega_reset.argtypes = [ctypes.POINTER(WD.Ega)]
    f = lib.pr_u38_0cc3_cpu
    f.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p, ctypes.POINTER(Diag)]; f.restype = ctypes.c_int
    return lib


def call(fn, bus, size, cpu, left, lim):
    mine = Limits(65536, 65536, 65536, 65536, 1000000, 65536)
    diag = Diag(); status = fn(bus, size, cpu, left, ctypes.byref(mine), ctypes.byref(diag))
    return STATUS[status], diag


def source():
    """D's closure (1693 and its world), StrLoad, the four FreeMem spans, the 0CC3 span and its message, all hash-checked."""
    bodies, decoded, meta = D.source()
    bodies = dict(bodies); decoded = dict(decoded)
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    with contextlib.redirect_stdout(io.StringIO()):
        image = bytes(unpack(exe)[0])
        _, fm_decoded, _, fm_digests = FM.source()
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    for a, b in ((0x0B0B, 0x0B25),) + FM.SPANS:
        blob = image[physical(0xAF8, a):physical(0xAF8, b)]; ins = list(md.disasm(blob, a))
        check(sum(i.size for i in ins) == len(blob), f'SYSTEM span {a:04X} decodes')
        bodies['sys', a] = blob; decoded.update({('sys', i.address): i for i in ins})
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x38)
    base = unit['ovr_offset']
    blob = ovr[base + ENTRY:base + END]; ins = list(md.disasm(blob, ENTRY))
    check(sha256(blob).hexdigest() == SPAN_SHA and len(ins) == 266 and sum(i.size for i in ins) == len(blob), 'unit 0x38 +0CC3 identity')
    check(ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa', 'the only return is RETF 0Ah')
    calls = [(i.address, i.op_str) for i in ins if i.mnemonic in ('call', 'lcall')]
    check(calls == [(0x0D44, '0xaf8, 0xb0b'), (0x0D49, '0xba, 0x1693'), (0x0EF7, '0xaf8, 0x254')], f'the three far calls {calls}')
    check(sorted(f for f in unit['fixups'] if ENTRY <= f < END) == sorted(RELOC), 'the three relocated segment words are the span fixups')
    msg = ovr[base + MSG[0]:base + MSG[0] + MSG[1]]
    check(sha256(msg).hexdigest() == MSG[2] and msg[0] == 26 and not [f for f in unit['fixups'] if MSG[0] <= f < MSG[0] + MSG[1]], 'the message at CS:0CA8')
    bodies['w', ENTRY] = blob; bodies['data', MSG[0]] = msg
    decoded.update({('w', i.address): i for i in ins})
    occupied = sorted((s, s + len(v)) for (k, s), v in bodies.items() if k == 'sys')
    check(all(a1 <= b0 for (a0, a1), (b0, b1) in zip(occupied, occupied[1:])), 'SYSTEM bodies do not overlap')
    meta = dict(meta, body0cc3=dict(bytes=len(blob), instructions=len(ins), sha256=SPAN_SHA), freemem=fm_digests)
    return bodies, decoded, meta


def heap(m, ds, *, top=HEAP_TOP, free=()):
    """HeapOrg/HeapPtr/HeapEnd/FreeList under DS (the GetMem audit's layout, at the world's DS); free blocks (seg, off, paragraphs, granule)."""
    put16(m, ds, 0x4400, HEAP_ORG[1]); put16(m, ds, 0x4402, HEAP_ORG[0]); put16(m, ds, 0x4404, top[1]); put16(m, ds, 0x4406, top[0])
    put16(m, ds, 0x4408, HEAP_END[1]); put16(m, ds, 0x440A, HEAP_END[0]); put16(m, ds, 0x4410, 0); put16(m, ds, 0x4412, 0)
    put16(m, ds, 0x4414, 0x00A9); put16(m, ds, 0x4416, 0x0AF8)
    chain = list(free) + [(top[0], top[1], None, None)]
    put16(m, ds, 0x440C, chain[0][1]); put16(m, ds, 0x440E, chain[0][0])
    for (seg, off, paragraphs, granule), nxt in zip(chain, chain[1:]):
        put16(m, seg, off, nxt[1]); put16(m, seg, (off + 2) & 0xFFFF, nxt[0]); put16(m, seg, (off + 4) & 0xFFFF, granule); put16(m, seg, (off + 6) & 0xFFFF, paragraphs)


def node_at(k):
    """The k-th node's heap address: 20h-byte slots from HeapOrg + 1 paragraph, offset 0 (a valid FreeMem block)."""
    return (HEAP_ORG[0] + 1 + 2 * k, 0x0000)


def fixture(bodies, *, nodes=(), key=0x00011234, kind=3, rec3_low=0x5, count=2, table=((1, 2, 3), (0, 0, 0)), steps=(1, 2, 3, 4),
            dxy=None, grid_fill=0x40, sp=0x8000, flags=0x0202, regs=None, world=None, nodes_at=None, writes=(), rec=REC, grid=GRID):
    """nodes: dicts with key, type, x, y, saved (4 bytes), live (4 flags), next (index or None, default the following node)."""
    m, c = WD.base_state(bodies, **(world or {}))
    ds, ss = c.ds, c.ss
    for off, seg in RELOC.items():
        put16(m, WD.W, off, WD.SEG[seg])
    heap(m, ds)
    r = regs or (0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, 0x1111)
    m[physical(rec[0], (rec[1] + 3) & 0xFFFF)] = ((kind & 0xF) << 4) | (rec3_low & 0xF)
    for y in range(-2, 12):
        for x in range(-2, 52):
            m[physical(grid[0], (grid[1] + 50 * y + x + 7) & 0xFFFF)] = (grid_fill + x + y) & 0xFF
    put16(m, ds, 0x6716, grid[1]); put16(m, ds, 0x6718, grid[0])
    for i, k in enumerate(steps, 1):
        m[physical(ds, 0x290D + i)] = k
    for k, (dx, dy) in enumerate(dxy or ((0, 0), (1, 0), (0, 1), (1, 1), (-1, 0), (0, -1), (-1, -1), (2, 0), (0, 2))):
        m[physical(ds, 0x2736 + k)] = dx & 0xFF; m[physical(ds, 0x273F + k)] = dy & 0xFF
    m[physical(ds, 0x6715)] = count
    for j, (live, x, y) in enumerate(table, 1):
        at = 0x66D6 + 7 * j
        put16(m, ds, at, 0x2222 if live else 0); put16(m, ds, at + 2, 0x0001 if live == 1 else 0)
        m[physical(ds, at + 4)] = x & 0xFF; m[physical(ds, at + 5)] = y & 0xFF
    places = nodes_at or [node_at(k) for k in range(len(nodes))]
    for k, n in enumerate(nodes):
        seg, off = places[k]
        nxt = n.get('next', k + 1 if k + 1 < len(nodes) else None)
        link = places[nxt] if nxt is not None else (0, 0)
        b = lambda o, v: m.__setitem__(physical(seg, (off + o) & 0xFFFF), v & 0xFF)
        for o in range(0x13):
            b(o, 0x60 + o + k)
        put16(m, seg, off, n['key'] & 0xFFFF); put16(m, seg, (off + 2) & 0xFFFF, n['key'] >> 16)
        put16(m, seg, (off + 4) & 0xFFFF, link[1]); put16(m, seg, (off + 6) & 0xFFFF, link[0])
        for i in range(4):
            b(8 + i, n.get('saved', (0x11, 0x22, 0x33, 0x44))[i]); b(0x0C + i, n.get('live', (1, 1, 1, 1))[i])
        b(0x10, n.get('x', 10)); b(0x11, n.get('y', 5)); b(0x12, n.get('type', kind))
    head = places[0] if nodes else (0, 0)
    put16(m, ds, 0x6DBB, head[1]); put16(m, ds, 0x6DBD, head[0])
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    cpu = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, WD.W, ENTRY, ds, r[7], ss, flags | 0x0002)
    for k, v in enumerate((WD.ENTRY_RET[1], WD.ENTRY_RET[0], 0x00AB, rec[1], rec[0], key & 0xFFFF, key >> 16)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    return m, cpu, dict(nodes=nodes, places=places, key=key, kind=kind, count=count, table=table, steps=steps, ds=ds, ss=ss, sp=sp,
                        dxy=dxy or ((0, 0), (1, 0), (0, 1), (1, 1), (-1, 0), (0, -1), (-1, -1), (2, 0), (0, 2)), grid=grid, plain=not writes and rec == REC and grid == GRID)


STOPS = {('sys', 0x00E2): 'RUNERROR'}


def run_pair(label, st, bodies, decoded, lib, budget):
    m, cpu, info = st
    expected = WD.run_original(m, cpu, decoded, budget, STOPS)
    actual = WD.run_c(lib, lib.pr_u38_0cc3_cpu, m, cpu, call, budget)
    seen = WD.compare_states(label, expected, actual, bodies, m, decoded)
    return expected, actual, seen


def s8(v):
    return v - 256 if v & 0x80 else v


def statement(name, st, expected):
    """Independent of the C model, on plain cases (no aliasing, a valid heap, an acyclic list): the found node, the four cells, the
    list after the unlink, the 1Eh stamps of the remaining nodes, the message temporary and the stack."""
    status, ram = expected[0], expected[1]
    m, cpu, info = st
    if status != 'RETURNED' or not info['plain']:
        return 0
    nodes, places, ds = info['nodes'], info['places'], info['ds']
    order, k = [], 0 if nodes else None
    while k is not None and k not in order and len(order) < 64:
        order.append(k); k = nodes[k].get('next', k + 1 if k + 1 < len(nodes) else None)
    if k is not None:
        return 0
    want = [k for k in order if nodes[k]['key'] == info['key'] and (nodes[k].get('type', info['kind']) & 0xFF) == info['kind']]
    g = lambda x, y: physical(info['grid'][0], (info['grid'][1] + ((50 * (y & 0xFF) + (x & 0xFF)) & 0xFFFF) + 7) & 0xFFFF)
    grid0 = {a: m[a] for a in range(physical(info['grid'][0], info['grid'][1]), physical(info['grid'][0], info['grid'][1]) + 0x1000)}

    def cell(n, i):
        kk = info['steps'][i]
        dx, dy = info['dxy'][kk] if kk < len(info['dxy']) else (s8(m[physical(ds, 0x2736 + kk)]), s8(m[physical(ds, 0x273F + kk)]))
        return (s8(n.get('x', 10)) + dx) & 0xFF, (s8(n.get('y', 5)) + dy) & 0xFF
    word = lambda a: ram[a] | (ram[(a + 1) & (SIZE - 1)] << 8)
    if not want:
        check(ram[physical(info['ss'], (info['sp'] - 2 - 0x28) & 0xFFFF)] != 26 or m[physical(info['ss'], (info['sp'] - 2 - 0x28) & 0xFFFF)] == 26,
              f'{name}: no node, but the message temporary was written')
        return 1
    t = want[0]; tn = nodes[t]
    expect = {}
    for i in range(4):
        if tn.get('live', (1, 1, 1, 1))[i]:
            x, y = cell(tn, i)
            hit = any(live and (s8(ex & 0xFF) & 0xFF) == x and (s8(ey & 0xFF) & 0xFF) == y for live, ex, ey in info['table'][:info['count']])
            expect[g(x, y)] = 0x1F if hit else tn.get('saved', (0x11, 0x22, 0x33, 0x44))[i]
    rest = [k for k in order if k != t]
    for k in rest:
        for i in range(4):
            if nodes[k].get('live', (1, 1, 1, 1))[i]:
                x, y = cell(nodes[k], i)
                expect[g(x, y)] = 0x1E
    for a, v in expect.items():
        check(ram[a] == v, f'{name}: map byte at {a:#x} is {ram[a]:#04x}, not {v:#04x}')
    for a, v in grid0.items():
        if a not in expect:
            check(ram[a] == v, f'{name}: map byte at {a:#x} changed')
    head = word(physical(ds, 0x6DBB)) | (word(physical(ds, 0x6DBD)) << 16)
    want_head = (places[rest[0]][0] << 16 | places[rest[0]][1]) if rest and order[0] == t else (places[order[0]][0] << 16 | places[order[0]][1]) if order[0] != t else 0
    check(head == want_head, f'{name}: list head {head:08x} is not {want_head:08x}')
    if order[0] != t:
        pred = places[order[order.index(t) - 1]]
        link = word(physical(pred[0], (pred[1] + 4) & 0xFFFF)) | (word(physical(pred[0], (pred[1] + 6) & 0xFFFF)) << 16)
        succ = order[order.index(t) + 1] if order.index(t) + 1 < len(order) else None
        want_link = (places[succ][0] << 16 | places[succ][1]) if succ is not None else 0
        check(link == want_link, f'{name}: the predecessor now links {link:08x}, not {want_link:08x}')
    check(ram[physical(info['ss'], (info['sp'] - 2 - 0x28) & 0xFFFF)] == 26, f'{name}: the 26-byte message was not loaded into [BP-28h]')
    out = expected[5]
    check(out.sp == (info['sp'] + 14) & 0xFFFF and (out.cs, out.ip) == WD.ENTRY_RET, f'{name}: SP/CS:IP on return')
    return 1


def N(key, **kw):
    return dict(key=key, **kw)


def cases(quick=False):
    rng = random.Random(0x380CC3)
    K = 0x00011234
    out = []

    def add(name, budget=BUDGET, **kw):
        out.append((name, kw, budget))
    add('empty-list', nodes=())
    add('no-key', nodes=(N(K + 1), N(K + 2)))
    add('key-wrong-type', nodes=(N(K, type=4), N(K + 1)))
    add('key-high-word-differs', nodes=(N(K ^ 0x10000),))
    for low in (0, 0xF):
        add(f'rec3-low-{low:x}', nodes=(N(K),), rec3_low=low)
    for kind in (0, 7, 0xF):
        add(f'kind-{kind:x}', nodes=(N(K, type=kind),), kind=kind)
    for pos, n in (('head', 0), ('middle', 1), ('tail', 2)):
        nodes = [N(K + 1, x=3, y=3), N(K + 2, x=20, y=6), N(K + 3, x=40, y=8)]
        nodes[n] = dict(nodes[n], key=K)
        add(f'unlink-{pos}', nodes=tuple(nodes))
    add('only-node', nodes=(N(K),))
    add('first-of-two-matches', nodes=(N(K, x=5), N(K, x=30)))
    for live in ((0, 0, 0, 0), (1, 0, 1, 0), (0, 0, 0, 1), (1, 1, 1, 1)):
        add(f'live-{"".join(map(str, live))}', nodes=(N(K, live=live), N(K + 1, x=30, live=(1, 0, 0, 1))))
    for count in (0, 1, 2, 5):
        add(f'table-count-{count}', count=count, table=((1, 10, 5), (1, 11, 5), (0, 10, 6), (2, 11, 6), (1, 99, 99)), nodes=(N(K, x=10, y=5),))
    add('table-zero-dword-on-cell', count=1, table=((0, 10, 5),), nodes=(N(K, x=10, y=5),))
    add('table-high-word-only', count=1, table=((2, 10, 5),), nodes=(N(K, x=10, y=5),))
    for x, y in ((0, 0), (-1, -1), (0x7F, 0x7F), (-0x80, -0x80), (49, 3)):
        add(f'coords-{x}-{y}', nodes=(N(K, x=x & 0xFF, y=y & 0xFF),), table=((1, x, y),), count=1)
    add('dxy-signed', dxy=((0, 0), (-3, 2), (4, -5), (-128, 127), (127, -128)) + ((0, 0),) * 4, steps=(1, 2, 3, 4), nodes=(N(K, x=20, y=6),))
    add('steps-high', steps=(8, 7, 0, 255), nodes=(N(K),))
    for m_ in (0, 1, 2, 3):
        add(f'mode{m_}', world=dict(mode=m_), nodes=(N(K),))
    for active in (0, 3, 7):
        add(f'page{active}', world=dict(active=active), nodes=(N(K),))
    for factor, cnt in ((1, 1), (2, 2), (0, 5)):
        add(f'delay-{factor}-{cnt}', world=dict(factor=factor, count=cnt), nodes=(N(K),))
    for sp in (SP_MIN, SP_MIN + 1, 0x7FFF, 0xF001, SP_MAX - 1, SP_MAX):
        add(f'sp-{sp:04x}', sp=sp, nodes=(N(K),)); add(f'sp-{sp:04x}-miss', sp=sp, nodes=(N(K + 1),))
    for fl in (0x0002, 0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57, 0x08D7, 0x0893, 0x0003, 0x7202):
        add(f'flags-{fl:04x}', flags=fl, nodes=(N(K), N(K + 1)))
    add('runerror-offset-4', nodes=(N(K),), nodes_at=[(HEAP_ORG[0] + 1, 0x0004)])
    add('runerror-outside-heap', nodes=(N(K),), nodes_at=[(0x3900, 0x0000)])
    add('freemem-offset-8', nodes=(N(K), N(K + 1)), nodes_at=[(HEAP_ORG[0] + 1, 0x0008), (HEAP_ORG[0] + 5, 0x0000)])
    add('cycle-no-key', budget=400, nodes=(N(K + 1, next=0),))
    add('cycle-two-no-key', budget=600, nodes=(N(K + 1, next=1), N(K + 2, next=0)))
    add('cycle-after-match', budget=3000, nodes=(N(K + 1), N(K, next=0)))
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x2C, 0x0E, 2):          # the map cell (+7) on each word of the frame: the 40-byte temporary, locals, BP, return, arguments
        add(f'grid-on-frame{k:+03x}', budget=ALIAS_BUDGET, sp=S0, nodes=(N(K, x=0, y=0, live=(1, 0, 0, 0)),), steps=(0, 0, 0, 0),
            grid=(0x2000, (BP + k - 7) & 0xFFFF))
    for k in (0, 2, 4, 6, 8, 0x10, 0x12):      # the map cell on the node itself (its key, link, saved bytes, coordinates, type)
        add(f'grid-on-node+{k:02x}', budget=ALIAS_BUDGET, nodes=(N(K, x=0, y=0, live=(1, 0, 0, 0)), N(K + 1, x=30)), steps=(0, 0, 0, 0),
            grid=(HEAP_ORG[0] + 1, (k - 7) & 0xFFFF))
    add('grid-on-list-head', budget=ALIAS_BUDGET, nodes=(N(K, x=0, y=0, live=(1, 0, 0, 0)),), steps=(0, 0, 0, 0), grid=(0x5000, (0x6DBB - 7) & 0xFFFF))
    add('record-on-frame', budget=ALIAS_BUDGET, sp=S0, nodes=(N(K),), rec=(0x2000, (BP - 0x0A - 3) & 0xFFFF))
    add('node-on-frame', budget=ALIAS_BUDGET, sp=S0, nodes=(N(K + 1),), nodes_at=[(0x2000, (BP - 0x0C) & 0xFFFF)])
    for budget in list(range(0, 260, 3 if quick else 1)) + [300, 400, 600, 800, 1000]:
        add(f'budget-{budget}', budget=budget, nodes=(N(K + 1, x=3), N(K, x=20), N(K + 2, x=40)))
    for budget in range(0, 40):
        add(f'budget-miss-{budget}', budget=budget, nodes=(N(K + 1), N(K + 2)))
    for k in range(8 if quick else 40):
        n = rng.randrange(1, 5)
        nodes = tuple(N(rng.choice((K, K + 1, rng.randrange(1 << 32))), type=rng.choice((3, 3, rng.randrange(256))), x=rng.randrange(256), y=rng.randrange(256),
                        saved=tuple(rng.randrange(256) for _ in range(4)), live=tuple(rng.randrange(2) for _ in range(4))) for _ in range(n))
        add(f'random{k}', nodes=nodes, count=rng.randrange(4), table=tuple((rng.randrange(3), rng.randrange(256), rng.randrange(256)) for _ in range(3)),
            steps=tuple(rng.randrange(9) for _ in range(4)), regs=tuple(rng.randrange(65536) for _ in range(8)),
            flags=rng.choice((2, 0x202, 0x08D7, rng.randrange(65536) & ~0x0100)), sp=rng.randrange(SP_MIN, SP_MAX + 1), world=dict(mode=rng.choice((0, 1, 2, 3))))
    return out


def differential(bodies, decoded, lib, quick=False, only=None):
    t0 = time.time(); stops = {}; seen_all = set(); stated = 0
    WD.SPLIT_ARRIVALS[0] = WD.SHIFT_SPLIT_ARRIVALS[0] = 0
    cs = [c for c in cases(quick) if only is None or c[0] in only]
    for index, (name, kw, budget) in enumerate(cs):
        if index % 40 == 0:
            print(f'[{index + 1}/{len(cs)}] {name}: START', file=sys.stderr, flush=True)
        st = fixture(bodies, **kw)
        expected, actual, seen = run_pair(name, st, bodies, decoded, lib, budget)
        stops[expected[0]] = stops.get(expected[0], 0) + 1; seen_all |= seen
        if actual[0] in ('ARGUMENT', 'CODE', 'CALLEE'):
            raise ValueError(f'{name}: C stopped {actual[0]}')
        if budget == BUDGET:
            stated += statement(name, st, expected)
    body = {k for k in seen_all if k[0] == 'w' and ENTRY <= k[1] < END}
    check(only is not None or len(body) == 266, f'every +0CC3 instruction start was reached ({len(body)}/266)')
    return dict(cases=len(cs), end_states=stops, starts_0cc3=f'{len(body)}/266', starts_total=len(seen_all), independent_statements_checked=stated,
                declared_mulw_split_states=WD.SPLIT_ARRIVALS[0], declared_shift_af_split_states=WD.SHIFT_SPLIT_ARRIVALS[0], cpu_words=14,
                ram_bytes=SIZE, flag_masks=0, seconds=round(time.time() - t0, 1))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for flag in ('--compile-only', '--unicorn', '--quick', '--mutate', '--native'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='u38-0cc3-') as tmp:
        lib = library(tmp)
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        bodies, decoded, meta = source(); report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, lib, a.quick)
        if a.mutate:
            from ovl_0038_0cc3_mutation import mutation_check
            report['mutation'] = mutation_check(bodies, decoded)
        if a.native:
            from ovl_0038_0cc3_native import native
            report['native'] = native(bodies, decoded, lib)
    if a.output:
        a.output.parent.mkdir(parents=True, exist_ok=True); a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    sys.modules['ovl_0038_0cc3_cpu_audit'] = sys.modules[__name__]
    main()
