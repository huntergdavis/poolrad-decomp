#!/usr/bin/env python3
"""U77 +17ED split/message caller: CPU14, all 1 MiB backing RAM, independent Python EGA bus.

The original 176-byte body, the original GetMem, Move and 0AF8:0B0B string load and the original
00BA:1693 closure run under Unicorn; the C composition is compared against them.
No flag masks, no excluded RAM range, no callback substitutes for a callee.
The string literal and game media stay under ignored resources/; only counts,
lengths and hashes are reported. Synthetic inputs assert no natural game route.
"""
import argparse, contextlib, ctypes, io, json, struct, subprocess, sys, tempfile, types
from hashlib import sha256
from pathlib import Path
import res00ba_display_1693_cpu_audit as base
from res00ba_display_1693_cpu_audit import (Cpu, Env, Bus, Ega, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE,
    REGISTERS, physical, put16, check, FLAGS, PythonEga, SEGMENTS, ROOT_CALLS)
from resident_loader_map import unpack
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0077_17ED',)
OVERLAY = 0x3000
SEGMENTS['overlay'] = OVERLAY
ENTRY, END = 0x17ED, 0x189D
LITERAL = 0x17DC
BODY_SHA = '46c07cbe1120a95c2830f87199a1f5141a272b9dfba7df3f534e4168079e82e9'  # prefix, full digest pinned in source()
SOURCES = tuple(dict.fromkeys(('overlay_u77_split_17ed_cpu', 'system_getmem_023f_cpu', 'system_string_move_leaves_cpu', 'system_string_leaves_cpu') + base.SOURCES))
SS, SP = base.SS, 0x8000


class Diag(ctypes.Structure):
    _fields_ = [('callee', ctypes.c_int), ('status', ctypes.c_int), ('display', base.Diagnostic)]


class Limits(ctypes.Structure):
    _fields_ = [(n, ctypes.c_size_t) for n in ('clear', 'rect', 'printer', 'glyph')] + [('delay', ctypes.c_uint64), ('heap', ctypes.c_uint)]


FIELDS = [n for n, _ in base.Diagnostic._fields_]
KEYS = ['r_callee', 'r_status'] + ['d_' + n for n in FIELDS]


def source():
    with contextlib.redirect_stdout(io.StringIO()):
        bodies, decoded, meta = base.source()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x77)
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    # Discover the first return BEFORE assigning a size to the target.
    ins = []
    for i in md.disasm(ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + unit['code_size']], ENTRY):
        ins.append(i)
        if i.mnemonic == 'retf':
            break
    end = ins[-1].address + ins[-1].size
    check(end == END and ins[-1].op_str == '4', 'first return extent')
    blob = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + end]
    digest = sha256(blob).hexdigest()
    check(len(blob) == 176 and len(ins) == 63 and sum(i.size for i in ins) == 176, '17ED extent')
    check(digest == BODY_SHA, '17ED identity')
    starts = {i.address for i in ins}
    check(all(int(i.op_str, 16) in starts for i in ins if i.mnemonic.startswith('j')), 'aligned internal branches')
    check(not any(i.mnemonic in ('call', 'int', 'into') for i in ins), 'no hidden near call or interrupt')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] ==
          [(0x1820, '0xaf8, 0x23f'), (0x1839, '0xaf8, 0x1685'),
           (0x188D, '0xaf8, 0xb0b'), (0x1892, '0xba, 0x1693')], 'call sites')
    fixups = [p for p in unit['fixups'] if ENTRY <= p < END]
    check(fixups == [0x1823, 0x183C, 0x1890, 0x1895], 'segment relocations')
    literal = ovr[unit['ovr_offset'] + LITERAL:unit['ovr_offset'] + ENTRY]
    check(len(literal) == 17 and literal[0] == 16, 'literal extent')
    with contextlib.redirect_stdout(io.StringIO()):
        image = bytes(unpack(exe)[0])
    check(sha256(image).hexdigest() == meta['expanded_image']['sha256'], 'expanded image')
    bodies['overlay', ENTRY] = blob
    decoded.update({('overlay', i.address): i for i in ins})
    from system_getmem_023f_audit import SPANS
    for start, end in (*SPANS, (0xB0B, 0xB25), (0x1685, 0x16A8)):
        span = image[physical(0xAF8, start):physical(0xAF8, end)]
        instructions = list(md.disasm(span, start))
        check(sum(i.size for i in instructions) == len(span), 'callee extent')
        bodies['sys', start] = span
        decoded.update({('sys', i.address): i for i in instructions})
        meta[f'0AF8:{start:04X}'] = dict(bytes=len(span), instructions=len(instructions), sha256=sha256(span).hexdigest())
    meta['U77:17ED'] = dict(bytes=176, instructions=63, sha256=digest, file_offset=unit['ovr_offset'] + ENTRY,
                           first_return='189A RETF 4', end_exclusive='189D')
    meta['literal'] = dict(address=f'{LITERAL:04X}', bytes=17, length_byte=literal[0], sha256=sha256(literal).hexdigest())
    meta['segment_relocations'] = [f'{v:04X}' for v in fixups]
    return bodies, decoded, meta, literal


def library(directory):
    path = directory / 'split17ed.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), *[str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES],
                    '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u77_split_17ed_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Limits), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def install(m, bodies, literal):
    for (kind, start), blob in bodies.items():
        if kind == 'overlay':  # resident/SYSTEM bodies were installed with their relocations by base.build
            at = physical(OVERLAY, start)
            m[at:at + len(blob)] = blob
    for site in (0x1820, 0x1839, 0x188D):
        put16(m, OVERLAY, site + 3, base.SYS)
    put16(m, OVERLAY, 0x1895, base.DISPLAY)
    at = physical(OVERLAY, LITERAL)
    m[at:at + len(literal)] = literal


def build(bodies, literal, *, node_count=2, node=(0x1000, 0x0200), sp=SP, ss=SS,
          top=(0x4000, 0), end=(0x4800, 0), blocks=(), heap_error=None,
          lit=None, ret=(0x7000, 0x9001), fill=None, gc5=None, writes=(), regs=None, **options):
    m, c = base.build({k: v for k, v in bodies.items() if k[0] != 'overlay'}, sp=sp, ss=ss, **options)
    install(m, bodies, literal)
    if lit is not None:
        at = physical(OVERLAY, LITERAL)
        m[at:at + len(lit)] = lit
    c.cs, c.ip, c.sp = OVERLAY, ENTRY, sp
    if regs:
        for name, value in regs.items():
            setattr(c, name, value)
    # Independent synthetic heap; no bytes from original game state.
    for off, value in ((0x4400, 0), (0x4402, 0x4000), (0x4404, top[1]), (0x4406, top[0]),
                       (0x4408, end[1]), (0x440A, end[0]),
                       (0x4414, (heap_error or (0xA9, base.SYS))[0]),
                       (0x4416, (heap_error or (0xA9, base.SYS))[1])):
        put16(m, c.ds, off, value)
    chain = list(blocks) + [(top[0], top[1], None, None)]
    put16(m, c.ds, 0x440C, chain[0][1]); put16(m, c.ds, 0x440E, chain[0][0])
    for (seg, off, paragraphs, granule), nxt in zip(chain, chain[1:]):
        for disp, value in ((0, nxt[1]), (2, nxt[0]), (4, granule), (6, paragraphs)):
            put16(m, seg, (off + disp) & 65535, value)
    for j in range(63):
        m[physical(node[0], (node[1] + j) & 65535)] = (17 * j + 0x31) & 255
    m[physical(node[0], (node[1] + 0x39) & 65535)] = node_count
    for j, value in enumerate((ret[0], ret[1], node[1], node[0])):
        put16(m, ss, (sp + 2 * j) & 65535, value)
    for seg, off, blob in writes:
        for j, value in enumerate(blob):
            m[physical(seg, (off + j) & 65535)] = value
    return m, c, dict(fill=fill, gc5=gc5, ret=ret)


def make_device(spec):
    device = PythonEga()
    if spec['fill']:
        device.plane = [bytearray([spec['fill'][p]]) * 65536 for p in range(4)]
    if spec['gc5'] is not None:
        device.gc[5] = spec['gc5']
    return device


def run_c(lib, state, limits, refuse=None):
    m, c, spec = state
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    ega = Ega()
    lib.pr_ega_reset(ctypes.byref(ega))
    for p in range(4):
        ctypes.memset(ctypes.addressof(ega.plane[p]), spec['fill'][p] if spec['fill'] else 0x31 + p * 0x27, 65536)
    if spec['gc5'] is not None:
        ega.gc[5] = spec['gc5']
    events, writes = [], []

    def out(ctx, port, value, size, cs, ip):
        if refuse is not None and len(events) == refuse:
            return 1
        events.append((port, value, size, cs, ip))
        return 0
    env = Env(None, OUT_FN(out), IN_FN())
    bus = Bus(ram, ctypes.pointer(ega), ctypes.pointer(env), WRITE_FN(lambda ctx, off, v: writes.append((off, v))), None)
    d = Diag()
    actual = Cpu.from_buffer_copy(c)
    status = lib.pr_u77_split_17ed_cpu(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(limits), ctypes.byref(d))
    return status, actual, bytes(ram), events, writes, bytes(ega), d


def calls_table():
    table = {}
    for key, (child, (field, status), index, dest) in base.CALLS.items():
        table[key] = (child, ('d_' + field if field else None, 'd_' + status), index, dest)
    for site, child, index, target in ((0x1820, 'heap', 0, 0x23F), (0x1839, 'move', 1, 0x1685),
                                       (0x188D, 'strload', 2, 0xB0B), (0x1892, 'display', 3, 0x1693)):
        table[OVERLAY, site] = (child, ('r_callee', 'r_status'), index, target)
    return table


CALLS = calls_table()
RESETS = {'clear': ('clear_callee', 'clear_status', 'rect_callee', 'rect_status'), 'rect': ('rect_callee', 'rect_status'),
          'printer': ('printer_callee', 'printer_status', 'glyph_callee', 'glyph_status'),
          'glyph': ('glyph_callee', 'glyph_status'), 'wrapper': ('delay_status',)}
CALL_SITES = {0: 'GetMem', 1: 'Move', 2: 'StrLoad', 3: 'display'}


def stop_name(status, diag, c, seen, ret):
    if status == 0:
        return 'RETURN/' + ('split' if ('overlay', 0x1807) in seen else 'message') + (
            '' if (c.cs, c.ip) == (ret[1], ret[0]) else '/altered-frame')
    if status == 2:
        return f'CODE@{c.ip:04X}'
    name = CALL_SITES.get(diag['r_callee'], 'none')
    if status == 4:
        return f'TRANSFER/{name}'
    if name != 'display':
        return f'CALLEE/{name}/' + {2: 'RUNERROR', 3: 'HEAP_ERROR', 4: 'TRANSFER', 5: 'LIMIT'}.get(diag['r_status'], str(diag['r_status']))
    nested = types.SimpleNamespace(**{n: diag['d_' + n] for n in FIELDS})
    return 'CALLEE/display/' + base.stop_name(diag['r_status'], nested, c)


def compare(name, state, bodies, decoded, lib, budgets, refuse=None):
    from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_CODE, UC_HOOK_INSN
    from unicorn import x86_const as R
    m, c, spec = state
    uc = Uc(UC_ARCH_X86, UC_MODE_16)
    backing = (ctypes.c_uint8 * SIZE).from_buffer_copy(m)
    device = make_device(spec)

    def read(em, off, size, user):
        return sum(device.read((off + j) & 65535) << (8 * j) for j in range(size))

    def write(em, off, size, value, user):
        for j in range(size):
            device.write((off + j) & 65535, (value >> (8 * j)) & 255)
    for origin in (0, SIZE):
        uc.mem_map_ptr(origin, 0xA0000, 7, ctypes.addressof(backing))
        uc.mmio_map(origin + 0xA0000, 65536, read, None, write, None)
        uc.mem_map_ptr(origin + 0xB0000, 0x50000, 7, ctypes.addressof(backing) + 0xB0000)
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        uc.reg_write(r, getattr(c, n))
    diag = {k: -1 for k in KEYS}
    frames, seen, edges, events = [], set(), set(), []
    st = dict(active='root', returning=False, stop=None, last=None)
    used = {k: 0 for k in budgets}

    def halt(status):
        for frame in reversed(frames):
            if frame['spec'] is not None:
                diag[frame['spec'][1][1]] = status
                status = 3
        st['stop'] = status
        uc.emu_stop()

    def hook(em, address, size, user):
        cs, ip = uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)
        here = (cs, ip)
        if st['returning']:
            st['returning'] = False
            if not frames:
                st['stop'] = 0
                return uc.emu_stop()
            f = frames.pop()
            st['active'] = f['parent']
            if here != f['return']:
                if f['spec'] is not None:
                    diag[f['spec'][1][1]] = 0
                    return halt(4 if st['active'] in ('root', 'wrapper') else 5)
                if st['active'] == 'heap':
                    return halt(4)
                raise ValueError(f'unexpected internal RET: {name} {here}')
        if st['active'] == 'heap':
            if here == (base.SYS, 0xE2):
                return halt(2)
            if here[1] in (0x3B7, 0x3C3) and bytes(uc.mem_read(physical(c.ds, 0x4414), 4)) != struct.pack('<HH', 0xA9, base.SYS):
                return halt(3)
            if here == (base.SYS, 0x319) and st['last'] in ((base.SYS, 0x32C), (base.SYS, 0x334)):
                if used['heap'] == budgets['heap']:
                    return halt(5)
                used['heap'] += 1
        key = (next((k for k, v in SEGMENTS.items() if v == cs), '?'), ip)
        check(key in decoded, f'escape {name}: {here} active={st["active"]}')
        ins = decoded[key]
        active, last = st['active'], st['last']
        if active == 'delay' and last is not None and last[0] == base.DELAY and {0x2C1: 0x2C0, 0x2C6: 0x2BD, 0x2B8: 0x2B0}.get(last[1]) == ip:
            if used['delay'] == budgets['delay']:
                return halt(2)
            used['delay'] += 1
        spec = CALLS.get(here)
        if spec and ins.mnemonic == 'lcall':
            if bytes(uc.mem_read(address, 3)) != b'\x9a' + struct.pack('<H', spec[3]):
                return halt(2)
        if here == (base.CS, 0x691) and bytes(uc.mem_read(address, 3)) != bytes.fromhex('e84bfb'):
            return halt(2)
        if ins.mnemonic == 'out' and refuse is not None and len(events) == refuse:
            return halt(2 if active == 'reset' else 4)
        take = not bool(uc.reg_read(R.UC_X86_REG_EFLAGS) & 0x40)
        loop = ((active == 'clear' and ip in (0x1D2, 0x2E0) and take) or (active == 'rect' and ip == 0x14F and take) or
                (active == 'printer' and ip == 0x69F and take) or
                (active == 'glyph' and ip in base.printer.LOOPS and (ins.mnemonic == 'jmp' or take)))
        if loop:
            if used[active] == budgets[active]:
                return halt(6)
            used[active] += 1
        if last is not None:
            lk = (next((k for k, v in SEGMENTS.items() if v == last[0]), '?'), last[1])
            prev = decoded[lk]
            if (prev.mnemonic.startswith('j') and prev.mnemonic != 'jmp') or prev.mnemonic.startswith('loop'):
                edges.add((last, here))
        seen.add(key)
        st['last'] = here
        if ins.mnemonic in ('call', 'lcall'):
            frames.append(dict(parent=active, spec=spec, **{'return': (cs, ip + ins.size)}))
            if spec:
                child, (field, status), index, dest = spec
                if field:
                    diag[field] = index
                diag[status] = 0
                st['active'] = child
                if child in used:
                    used[child] = 0
                for n in RESETS.get(child, ()):
                    diag['d_' + n] = -1
            else:
                check(active in ('delay', 'heap'), f'unclassified CALL {name} {here} {ins.op_str}')
        if ins.mnemonic in ('ret', 'retf'):
            st['returning'] = True

    def out(em, port, size, value, user):
        events.append((port, value, size, uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)))
        device.out(port, value, size)
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.hook_add(UC_HOOK_INSN, out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=10000000)
    check(st['stop'] is not None, f'instruction ceiling {name}')
    for (kind, start), blob in bodies.items():
        at = physical(SEGMENTS[kind], start)
        check(bytes(backing[at:at + len(blob)]) == m[at:at + len(blob)], f'code mutation outside contract {name}')
    limits = Limits(budgets['clear'], budgets['rect'], budgets['printer'], budgets['glyph'], budgets['delay'], budgets['heap'])
    status, actual, ram, mine, stores, ega, d = run_c(lib, state, limits, refuse)
    check(status == st['stop'], f'status {name}: {status}/{st["stop"]} {diag}')
    got = {'r_callee': d.callee, 'r_status': d.status}
    got.update({'d_' + n: getattr(d.display, n) for n in FIELDS})
    check(got == diag, f'diagnostic {name}: {got}/{diag}')
    for (n, _), r in zip(Cpu._fields_, REGISTERS):
        want = uc.reg_read(r) & 65535
        check(getattr(actual, n) == want, f'CPU {name}/{n}: {getattr(actual, n):04X}/{want:04X}')
    if ram != bytes(backing):
        diffs = [(hex(i), a, b) for i, (a, b) in enumerate(zip(ram, backing)) if a != b]
        raise ValueError(f'RAM {name}: {diffs[:8]} ({len(diffs)})')
    check(events == mine, f'OUTs {name}')
    check(stores == device.writes, f'VRAM operands {name}')
    if ega != device.packed():
        diffs = [(hex(i), a, b) for i, (a, b) in enumerate(zip(ega, device.packed())) if a != b]
        raise ValueError(f'EGA {name}: {diffs[:8]} ({len(diffs)})')
    label = stop_name(status, diag, actual, seen, spec['ret'])
    return label, seen, edges, len(events), len(stores), len(device.reads)


def cases(literal, quick=False):
    # All byte counts in both incoming DF states, then directed call/heap/device cases.
    specs = [(f'count-{n}-df-{df}', dict(node_count=n, flags=0x202 | df))
             for n in range(256) for df in (0, 0x400)]
    if quick:
        specs = [(f'count-{n}', dict(node_count=n)) for n in (0, 1, 2, 3, 255)]
    for site in (0x1820, 0x1839, 0x188D, 0x1892):
        for offset in (0, 1):
            specs.append((f'code-{site + offset:x}', dict(node_count=0 if site >= 0x188D else 2,
                writes=((OVERLAY, site + offset, b'\x90'),))))
    specs += [('heap-full', dict(end=(0x4000, 0))),
              ('heap-custom-top', dict(heap_error=(0x1234, 0x5678))),
              ('heap-custom-fail', dict(end=(0x4000, 0), heap_error=(0x1234, 0x5678))),
              ('heap-exact', dict(blocks=((0x3900, 0, 4, 0),))),
              ('heap-split', dict(blocks=((0x3900, 8, 8, 8),))),
              ('heap-limit', dict(blocks=((0x3900, 0, 1, 0),), heap_limit=0)),
              ('heap-skip', dict(blocks=((0x3900, 0, 1, 0), (0x3920, 0, 4, 0))))]
    for off in (0, 8, 0x100, 0x400, 0xFFC0):
        specs.append((f'node-offset-{off:x}', dict(node=(0x1000, off))))
    for sp in (0x18C, 0x18D, 0x1000, 0x8001, 0xFFF8):
        specs.append((f'sp-{sp:x}', dict(sp=sp)))
    specs += [('ss-ds', dict(ss=base.DS, sp=0x6000)), ('ss-a20', dict(ss=0xFFFF)),
              ('clear-limit', dict(node_count=0, mode=0, clear_limit=0)),
              ('rect-limit', dict(node_count=0, rect_limit=0)),
              ('printer-limit', dict(node_count=0, printer_limit=0)),
              ('glyph-limit', dict(node_count=0, glyph_limit=0)),
              ('delay-limit', dict(node_count=0, factor=1, delay_limit=0)),
              ('port-refusal', dict(node_count=0, refuse=0)),
              ('display-code', dict(node_count=0, writes=((base.DISPLAY, 0x16A7, b'\x90'),))),
              ('node-alias-destination', dict(node=(0x4000, 0))),
              ('node-overlap-destination', dict(node=(0x4000, 8))),
              ('backward-move', dict(top=(0x4000, 0x800), node=(0x1000, 0x100))),
              ('move-return-alias', dict(top=(SS + 0x7FD, 8), end=(SS + 0x900, 0)))]
    check(quick or len(specs) == 550, f'case count {len(specs)}')
    return specs


def differential(bodies, decoded, literal, lib, quick=False):
    seen, edges, stops = set(), set(), {}
    outs = writes = reads = count = 0
    for name, options in cases(literal, quick):
        options = dict(options)
        budgets = dict(clear=65536, rect=65536, printer=65536, glyph=65536, delay=1000000, heap=64)
        for k in tuple(options):
            if k.endswith('_limit'):
                budgets[k[:-6]] = options.pop(k)
        refuse = options.pop('refuse', None)
        label, v, e, o, w, r = compare(name, build(bodies, literal, **options), bodies, decoded, lib, budgets, refuse)
        seen |= v
        edges |= e
        outs += o
        writes += w
        reads += r
        count += 1
        stops[label] = stops.get(label, 0) + 1
        print(name, label, flush=True)
    root = {k for k in decoded if k[0] == 'overlay'}
    cond = {k: decoded[k] for k in root if decoded[k].mnemonic.startswith('j') and decoded[k].mnemonic != 'jmp'}
    expected = {((OVERLAY, k[1]), (OVERLAY, t)) for k, i in cond.items() for t in (k[1] + i.size, int(i.op_str, 16))}
    closure = {(SEGMENTS[k], a) for k, a in decoded}
    cexpected = {((SEGMENTS[k], a), (SEGMENTS[k], t)) for (k, a), i in decoded.items()
                 if (i.mnemonic.startswith('j') and i.mnemonic != 'jmp') or i.mnemonic.startswith('loop')
                 for t in (a + i.size, int(i.op_str, 16))}
    seen_closure = {(SEGMENTS[k], a) for k, a in seen}
    edge_closure = edges  # already (loaded segment, offset) pairs
    report = dict(cases=count, partial_fixture_set=quick, synthetic_callee_stubs=0, stops=stops, cpu_words=14, ram_bytes=SIZE, ram_excluded_ranges=[], flag_masks=0,
                  ega_bytes=len(PythonEga().packed()), outs=outs, ega_writes=writes, ega_reads=reads,
                  root_starts=len(root & seen), root_total=len(root),
                  root_outcomes=len(expected & edges), root_outcomes_total=len(expected),
                  root_missing_starts=sorted(f'{a:04X}' for k, a in root - seen),
                  root_missing_outcomes=sorted(f'{k[1]:04X}->{t[1]:04X}' for k, t in expected - edges),
                  closure_starts=len(closure & seen_closure), closure_total=len(closure),
                  closure_outcomes=len(cexpected & edge_closure), closure_outcomes_total=len(cexpected),
                  closure_missing_starts=[f'{s:04X}:{a:04X}' for s, a in sorted(closure - seen_closure)],
                  closure_missing_outcomes=[[f'{s:04X}:{a:04X}' for s, a in pair] for pair in sorted(cexpected - edge_closure)])
    print(json.dumps({k: v for k, v in report.items() if not k.startswith('closure_missing_')}, sort_keys=True))
    check(('overlay', 0x1881) in seen and ('overlay', 0x1897) in seen, 'unconditional split epilogue reached')
    check(not report['root_missing_starts'] and not report['root_missing_outcomes'], 'root coverage incomplete')
    return report


def guards(lib):
    ram = (ctypes.c_uint8 * SIZE)()
    ega = Ega()
    lib.pr_ega_reset(ctypes.byref(ega))
    limits = Limits(65536, 65536, 65536, 65536, 1000000, 64)
    baseline = Cpu(0, 0, 0, 0, 0, 0, 0, SP, OVERLAY, ENTRY, base.DS, 0, SS, 2)
    count = 0
    specs = [('bus', None), ('ram', None), ('ega', None), ('cpu', None), ('limits', None),
             ('size', SIZE - 1), ('ip', ENTRY + 1), ('sp', 0x18B), ('sp', 0xFFF9),
             ('cs', 0xA000), ('ds', 0xA000), ('ss', 0xA000)]
    for field, value in specs:
        c, d = Cpu.from_buffer_copy(baseline), Diag()
        b = Bus(ram, ctypes.pointer(ega), None, WRITE_FN(), None)
        if field in ('ram', 'ega'):
            setattr(b, field, None)
        if field in ('ip', 'sp', 'cs', 'ds', 'ss'):
            setattr(c, field, value)
        before = bytes(c), bytes(ram), bytes(ega)
        status = lib.pr_u77_split_17ed_cpu(None if field == 'bus' else ctypes.byref(b),
            value if field == 'size' else SIZE, None if field == 'cpu' else ctypes.byref(c),
            None if field == 'limits' else ctypes.byref(limits), ctypes.byref(d))
        check(status == 1 and before == (bytes(c), bytes(ram), bytes(ega)), f'guard {field}')
        check(d.callee == d.status == -1 and all(getattr(d.display, n) == -1 for n in FIELDS), 'guard diagnostic')
        count += 1
    return dict(cases=count, preserved=['CPU14', 'RAM1MiB', 'EGA'], diagnostics='reset', original_execution=False)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--quick', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='split17ed-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        bodies, decoded, meta, literal = source()
        report = dict(target=meta, host_argument_guards=guards(lib))
        def save():
            if a.output:
                a.output.parent.mkdir(parents=True, exist_ok=True)
                a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
        save()
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, literal, lib, a.quick)
            save()
        if a.native:
            from u77_split_17ed_native import native
            report['native'] = native(bodies, literal, lib)
            save()


if __name__ == '__main__':
    sys.modules['u77_split_17ed_audit'] = sys.modules[__name__]
    main()
