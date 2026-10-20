#!/usr/bin/env python3
"""U38 +3134 gated message caller: CPU14, all 1 MiB backing RAM, independent Python EGA bus.

The original 70-byte body, the original 0AF8:0B0B string load and the original
00BA:1693 closure run under Unicorn; the C composition is compared against them.
No flag masks, no excluded RAM range, no callback substitutes for a callee.
The string literal and game media stay under ignored resources/; only counts,
lengths and hashes are reported. Synthetic inputs assert no natural game route.
"""
import argparse, contextlib, ctypes, io, json, random, struct, subprocess, sys, tempfile, types
from hashlib import sha256
from pathlib import Path
import res00ba_display_1693_cpu_audit as base
from res00ba_display_1693_cpu_audit import (Cpu, Env, Bus, Ega, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE,
    REGISTERS, physical, put16, check, FLAGS, PythonEga, SEGMENTS, ROOT_CALLS)
from resident_loader_map import unpack
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0038_3134',)
OVERLAY = 0x3000
SEGMENTS['overlay'] = OVERLAY
ENTRY, END = 0x3134, 0x317A
LITERAL = 0x311B
BODY_SHA = '9c024ea9f2fa'  # prefix, full digest pinned in source()
SOURCES = tuple(dict.fromkeys(('overlay_u38_message_3134_cpu', 'system_string_leaves_cpu') + base.SOURCES))
A_SEG, A_OFF, B_SEG, B_OFF = 0x1000, 0x0200, 0x1000, 0x4000
SS, SP = base.SS, 0x8000


class Diag(ctypes.Structure):
    _fields_ = [('callee', ctypes.c_int), ('status', ctypes.c_int), ('display', base.Diagnostic)]


class Limits(ctypes.Structure):
    _fields_ = [(n, ctypes.c_size_t) for n in ('clear', 'rect', 'printer', 'glyph')] + [('delay', ctypes.c_uint64)]


FIELDS = [n for n, _ in base.Diagnostic._fields_]
KEYS = ['r_callee', 'r_status'] + ['d_' + n for n in FIELDS]


def source():
    with contextlib.redirect_stdout(io.StringIO()):
        bodies, decoded, meta = base.source()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    unit = next(u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr) if u['stub_para'] == 0x38)
    check(unit['entries'].index(ENTRY) == 114, 'public entry slot')
    blob = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + END]
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(blob, ENTRY))
    digest = sha256(blob).hexdigest()
    check(len(blob) == 70 and len(ins) == 26 and sum(i.size for i in ins) == 70, '3134 extent')
    check(digest.startswith(BODY_SHA) and ins[-1].mnemonic == 'retf' and ins[-1].op_str == '0xa', '3134 identity')
    check([i.address for i in ins if i.mnemonic == 'retf'] == [0x3177], 'single return')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] ==
          [(0x316A, '0xaf8, 0xb0b'), (0x316F, '0xba, 0x1693')], 'call sites')
    check([p for p in unit['fixups'] if ENTRY <= p < END] == [0x316D, 0x3172], 'segment-word relocations')
    check(not any(i.mnemonic in ('int', 'into') or (i.mnemonic in ('jmp', 'call') and i.op_str.startswith(('word', 'dword', 'ax', 'bx', 'di', 'si')))
                  for i in ins), 'no indirect transfer')
    literal = ovr[unit['ovr_offset'] + LITERAL:unit['ovr_offset'] + ENTRY]
    check(len(literal) == 25 and literal[0] == 24 and LITERAL + 25 == ENTRY, 'literal precedes entry')
    image = bytes(unpack(exe)[0])
    b0b = image[physical(0xAF8, 0xB0B):physical(0xAF8, 0xB25)]
    b0b_ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(b0b, 0xB0B))
    check(len(b0b) == 26 and len(b0b_ins) == 12 and b0b_ins[-1].mnemonic == 'retf' and b0b_ins[-1].op_str == '4', '0B0B extent')
    check(bytes(unpack(exe)[0]) == image and sha256(image).hexdigest() == meta['expanded_image']['sha256'], 'expanded image')
    bodies['overlay', ENTRY] = blob
    bodies['sys', 0xB0B] = b0b
    decoded.update({('overlay', i.address): i for i in ins})
    decoded.update({('sys', i.address): i for i in b0b_ins})
    meta['U38:3134'] = dict(bytes=70, instructions=26, sha256=digest, file_offset=unit['ovr_offset'] + ENTRY, entry_slot=114)
    meta['literal'] = dict(address=f'{LITERAL:04X}', bytes=25, length_byte=literal[0], sha256=sha256(literal).hexdigest())
    meta['0AF8:0B0B'] = dict(bytes=26, instructions=12, sha256=sha256(b0b).hexdigest())
    meta['segment_relocations'] = ['316D', '3172']
    return bodies, decoded, meta, literal


def library(directory):
    path = directory / 'message3134.so'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared',
                    '-I', str(ROOT / 'include'), *[str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES],
                    '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u38_message_3134_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Limits), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def install(m, bodies, literal):
    for (kind, start), blob in bodies.items():
        if kind == 'overlay':  # resident/SYSTEM bodies were installed with their relocations by base.build
            at = physical(OVERLAY, start)
            m[at:at + len(blob)] = blob
    put16(m, OVERLAY, 0x316D, base.SYS)
    put16(m, OVERLAY, 0x3172, base.DISPLAY)
    at = physical(OVERLAY, LITERAL)
    m[at:at + len(literal)] = literal


def build(bodies, literal, *, gate=0, gate_hi=0xA5, b_byte=0x12, a=(A_SEG, A_OFF), b=(B_SEG, B_OFF), sp=SP, ss=SS,
          lit=None, ret=(0x7000, 0x9001), fill=None, gc5=None, writes=(), regs=None, **options):
    options = dict(options)
    options.update(sp=sp, ss=ss)
    m, c = base.build({k: v for k, v in bodies.items() if k[0] != 'overlay'}, writes=(), **options)
    install(m, bodies, literal)
    if lit is not None:
        at = physical(OVERLAY, LITERAL)
        m[at:at + len(lit)] = lit
    c.cs, c.ip, c.sp = OVERLAY, ENTRY, sp
    if regs:
        for name, value in regs.items():
            setattr(c, name, value)
    for j, v in enumerate((ret[0], ret[1], (gate_hi << 8) | gate, a[1], a[0], b[1], b[0])):
        put16(m, ss, (sp + 2 * j) & 65535, v)
    if not 0xA0000 <= physical(b[0], b[1] + 0x10) < 0xB0000:
        m[physical(b[0], (b[1] + 0x10) & 65535)] = b_byte
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 65535)] = v
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
    status = lib.pr_u38_message_3134_cpu(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(limits), ctypes.byref(d))
    return status, actual, bytes(ram), events, writes, bytes(ega), d


def calls_table():
    table = {}
    for key, (child, (field, status), index, dest) in base.CALLS.items():
        table[key] = (child, ('d_' + field if field else None, 'd_' + status), index, dest)
    table[OVERLAY, 0x316A] = ('strload', ('r_callee', 'r_status'), 0, 0xB0B)
    table[OVERLAY, 0x316F] = ('display', ('r_callee', 'r_status'), 1, 0x1693)
    return table


CALLS = calls_table()
RESETS = {'clear': ('clear_callee', 'clear_status', 'rect_callee', 'rect_status'), 'rect': ('rect_callee', 'rect_status'),
          'printer': ('printer_callee', 'printer_status', 'glyph_callee', 'glyph_status'),
          'glyph': ('glyph_callee', 'glyph_status'), 'wrapper': ('delay_status',)}
CALL_SITES = {0x316A: 'strload', 0x316F: 'display'}


def stop_name(status, diag, c, seen, ret):
    if status == 0:
        path = 'message' if ('overlay', 0x3158) in seen else 'b-gate' if ('overlay', 0x314E) in seen else 'gate'
        return f'RETURN/{path}' + ('' if (c.cs, c.ip) == (ret[1], ret[0]) else '/altered-frame')
    if status == 2:
        return f'CODE@{c.ip:04X}'
    name = CALL_SITES[0x316A if diag['r_callee'] == 0 else 0x316F]
    if status == 4:
        return f'TRANSFER/{name}'
    if name == 'strload':
        return f'CALLEE/strload/{diag["r_status"]}'
    nested = types.SimpleNamespace(**{n: diag['d_' + n] for n in FIELDS})
    return f'CALLEE/display/' + base.stop_name(diag['r_status'], nested, c)


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
                raise ValueError(f'unexpected internal RET: {name} {here}')
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
                check(active == 'delay', f'unclassified CALL {name} {here} {ins.op_str}')
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
    limits = Limits(budgets['clear'], budgets['rect'], budgets['printer'], budgets['glyph'], budgets['delay'])
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
    ret = (0x7000, 0x9001)
    real = bytes(literal)

    def lit(length, filler=0x41):
        return bytes([length]) + bytes([filler + (j % 26) for j in range(max(length, 24))])[:max(length, 24)]
    specs = [('message', dict()), ('gate1', dict(gate=1)), ('gate-high-only', dict(gate=0, gate_hi=0xFF)),
             ('b-gate-13', dict(b_byte=0x13)), ('b-gate-12', dict(b_byte=0x12)), ('b-gate-0', dict(b_byte=0))]
    if quick:
        return specs + [('code-316A', dict(writes=((OVERLAY, 0x316A, b'\x90'),))), ('printer-limit', dict(printer_limit=0))]
    for gate in (1, 0x80, 0xFF):
        for hi in (0, 0xFF):
            specs.append((f'gate-{gate:x}-{hi:x}', dict(gate=gate, gate_hi=hi)))
    for value in (0x14, 0x80, 0xFF, 1, 0x11):
        specs.append((f'b-byte-{value:x}', dict(b_byte=value)))
    for flags in FLAGS:
        specs.append((f'flags-msg-{flags:x}', dict(flags=flags)))
        specs.append((f'flags-gate-{flags:x}', dict(flags=flags, gate=1)))
    specs.append(('flags-all-set-gate', dict(flags=0x0ED7, gate=1)))
    for sp in (0x192, 0x193, 0x1000, 0x8001, 0xFFF1, 0xFFF2):
        specs.append((f'sp-{sp:x}', dict(sp=sp)))
        specs.append((f'sp-gate-{sp:x}', dict(sp=sp, gate=1)))
    specs += [('ss-ds-alias', dict(ss=base.DS, sp=0x6000)), ('a20-stack', dict(ss=0xFFFF)),
              ('a20-stack-gate', dict(ss=0xFFFF, gate=1))]
    # pointer A: wrap, alias and aperture targets
    for off in (0xFFCB, 0xFFCC, 0xFFFF, 0, 0x7FFF):
        specs.append((f'a-offset-{off:x}', dict(a=(A_SEG, off))))
    specs += [('a-literal-length', dict(a=(OVERLAY, (LITERAL - 0x34) & 65535))),
              ('a-literal-char0', dict(a=(OVERLAY, (LITERAL + 1 - 0x34) & 65535))),
              ('a-literal-last', dict(a=(OVERLAY, (LITERAL + 24 - 0x34) & 65535))),
              ('a-saved-bp', dict(a=(SS, (SP - 2 - 0x34) & 65535))),
              ('a-return-ip', dict(a=(SS, (SP - 0x34) & 65535))),
              ('a-return-cs', dict(a=(SS, (SP + 3 - 0x34) & 65535))),
              ('a-local-buffer', dict(a=(SS, (SP - 2 - 0x1D - 0x34) & 65535))),
              ('a-local-a', dict(a=(SS, (SP - 2 - 4 - 0x34) & 65535))),
              ('a-ds-mode', dict(a=(base.DS, (0x4A1E - 0x34) & 65535)))]
    for gc5 in (None, 1, 2):
        specs.append((f'a-aperture-gc5-{gc5}', dict(a=(0xA000, 0x0100), gc5=gc5)))
    specs.append(('a-aperture-wrap', dict(a=(0xAFFF, 0x0F00))))
    # pointer B: aperture reads (latch state), read modes, planar fills
    for gc5 in (None, 8):
        for fill in (None, (5, 6, 7, 8), (0x40, 0x41, 0x42, 0x43)):
            specs.append((f'b-aperture-gc5-{gc5}-{fill and fill[0]}', dict(b=(0xA000, 0x0200 - 0x10), gc5=gc5, fill=fill)))
    specs.append(('b-offset-wrap', dict(b=(B_SEG, 0xFFF0), b_byte=3)))
    specs.append(('b-alias-return', dict(b=(SS, (SP - 0x10) & 65535), b_byte=3)))
    # string literal data variants: length bytes beyond the local buffer overflow into the frame
    for length in (0, 1, 24, 28, 29, 30, 40, 41, 64, 128, 255):
        specs.append((f'literal-{length}', dict(lit=bytes([length]))))  # only the length byte changes; later bytes stay the real code
    specs.append(('literal-real-gate-b13', dict(lit=real, b_byte=0x13)))
    # code guards
    for at, blob in ((0x316A, b'\x90'), (0x316B, bytes(2)), (0x316F, b'\x90'), (0x3170, bytes(2))):
        specs.append((f'code-{at:x}', dict(writes=((OVERLAY, at, blob),))))
    for site in base.ROOT_CALLS:
        specs.append((f'root-code-{site:x}', dict(writes=((base.DISPLAY, site, b'\x90'),))))
    for seg, site in ((base.CLEAR, 0x1F1), (base.CLEAR, 0x20C), (base.CS, 0x140), (base.CS, 0x63C), (base.CS, 0x691), (base.CS, 0x207), (base.CS, 0xDCC)):
        specs.append((f'nested-code-{seg:x}-{site:x}', dict(writes=((seg, site, b'\x90'),))))
    # budgets and device modes inherited through 1693
    specs += [('clear-limit', dict(mode=0, clear_limit=0)), ('rect-limit', dict(rect_limit=0)),
              ('printer-limit', dict(printer_limit=0)), ('glyph-limit', dict(glyph_limit=0)),
              ('delay-limit', dict(factor=255, delay_limit=17)), ('delay-zero', dict(factor=0)),
              ('delay-one', dict(factor=1)), ('delay-inner-limit', dict(factor=1, count=1, delay_limit=0))]
    for mode in (0, 1, 2, 3, 255):
        specs.append((f'mode-{mode}', dict(mode=mode)))
    for active in range(8):
        specs.append((f'page-{active}', dict(active=active)))
    specs += [('same-pages', dict(pages=(0xA000, 0xA000))), ('ordinary-pages', dict(pages=(0xB000, 0xC000))),
              ('no-font', dict(writes=((base.DS, 0x5E20, bytes(4)),)))]
    for n in (0, 1, 2, 17, 21, 22, 23, 43, 44, 45, 65, 66, 67, 88, 100, 120, 150):
        specs.append((f'port-{n}', dict(refuse=n)))
    rng = random.Random(0x3134)
    for n in range(24):
        regs = {r: rng.randrange(65536) for r in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'es')}
        specs.append((f'random-{n}', dict(gate=rng.choice((0, 0, 0, 1, 255)), gate_hi=rng.randrange(256), b_byte=rng.choice((0, 0x12, 0x13, 0xFF, rng.randrange(256))),
                                         flags=rng.choice(FLAGS), sp=rng.choice((rng.randrange(0x192, 0xFFF3), 0x8000)), regs=regs,
                                         mode=rng.choice((0, 1, 2, 3)), active=rng.randrange(8), factor=n % 2)))
    return specs


def differential(bodies, decoded, literal, lib, quick=False):
    seen, edges, stops = set(), set(), {}
    outs = writes = reads = count = 0
    for name, options in cases(literal, quick):
        options = dict(options)
        budgets = dict(clear=65536, rect=65536, printer=65536, glyph=65536, delay=1000000)
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
    report = dict(cases=count, stops=stops, cpu_words=14, ram_bytes=SIZE, ram_excluded_ranges=[], flag_masks=0,
                  ega_bytes=len(PythonEga().packed()), outs=outs, ega_writes=writes, ega_reads=reads,
                  root_starts=len(root & seen), root_total=len(root),
                  root_outcomes=len(expected & edges), root_outcomes_total=len(expected),
                  root_missing_starts=sorted(f'{a:04X}' for k, a in root - seen),
                  root_missing_outcomes=sorted(f'{k[1]:04X}->{t[1]:04X}' for k, t in expected - edges),
                  closure_starts=len(closure & seen_closure), closure_total=len(closure),
                  closure_outcomes=len(cexpected & edge_closure), closure_outcomes_total=len(cexpected),
                  closure_missing_starts=[f'{s:04X}:{a:04X}' for s, a in sorted(closure - seen_closure)],
                  closure_missing_outcomes=[[f'{s:04X}:{a:04X}' for s, a in pair] for pair in sorted(cexpected - edge_closure)])
    print(json.dumps(report, sort_keys=True))
    check(not report['root_missing_starts'] and not report['root_missing_outcomes'], 'root coverage incomplete')
    return report


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--quick', action='store_true')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='message3134-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        bodies, decoded, meta, literal = source()
        report = dict(target=meta)
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, literal, lib, a.quick)
        if a.native:
            from u38_message_3134_cpu_native import native
            report['native'] = native(bodies, literal, lib)
        if a.output:
            a.output.parent.mkdir(parents=True, exist_ok=True)
            a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    sys.modules['u38_message_3134_cpu_audit'] = sys.modules[__name__]
    main()
