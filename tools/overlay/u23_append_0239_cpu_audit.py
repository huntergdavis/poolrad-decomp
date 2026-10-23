#!/usr/bin/env python3
"""U23 +0239 inventory append: CPU14, all 1 MiB backing RAM, independent Python EGA bus, original-byte closure.

The original 278-byte body, its overlay stub resolver, the original U77 275B capacity check, the 00BA:0BB8 character
closure, SYSTEM GetMem, Move and StrLoad and the 00BA:1693 message closure all run under Unicorn; the C composition is
compared against them on unmasked CPU14, every RAM byte, the ordered OUT trace, the VRAM operands and the full EGA state
(planes, latches, registers). The over-capacity message and the list append are also checked against independent Python
statements that share no code with either side. No flag masks, no excluded RAM range, no callee substituted.
Game text stays under ignored resources/: only counts, lengths and hashes are reported.
"""
import argparse, contextlib, ctypes, io, json, random, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
import res00ba_display_1693_cpu_audit as base
import u9e_character_pair_115c_audit as r115
from res00ba_display_1693_cpu_audit import (Cpu, Env, Bus, Ega, OUT_FN, IN_FN, WRITE_FN, ROOT, SIZE, REGISTERS, physical, put16,
    check, FLAGS, PythonEga)
from resident_loader_map import unpack
from overlay_inventory import find_units, parse_mz

CHECKER_ROWS = ('OVL_0023_0239',)
UNIT = 0x23
ENTRY, END, LITERAL = 0x239, 0x34F, 0x22E
BODY_SHA = 'cfce7e60752c92e3d2975c742d7ef052e20090a2a56824bae13b3fa6083f27c9'
OVERLAY, STUB, U77, RES = 0x3000, 0x3200, 0x3400, 0x00BA   # the 275B model hard-codes 00BA:0BB8, so the resident closure sits at 00BA
DS, SS, SYS, DISPLAY = base.DS, base.SS, base.SYS, base.DISPLAY
RETURN = (0x7000, 0x9001)
CHAR_SEG, CHAR_OFF, NODE_SEG, ITEM_SEG, RESULT_SEG, HEAP_SEG = 0x1900, 0x100, 0x1B00, 0x1F00, 0x1F80, 0x9000   # clear of the native COM segment (0814..1813)
SITES = {0x24D: (0x4D, 0, 'check'), 0x260: (0xB0B, 1, 'strload'), 0x265: (0x1693, 2, 'display'), 0x29B: (0x23F, 3, 'getmem-head'),
         0x2B4: (0xA3F, 4, 'move-head'), 0x300: (0x23F, 5, 'getmem-tail'), 0x319: (0xA3F, 6, 'move-tail'), 0x344: (0xBB8, 7, 'redraw')}
SEG_WORDS = {0x250: STUB, 0x263: SYS, 0x268: DISPLAY, 0x29E: SYS, 0x2B7: SYS, 0x303: SYS, 0x31C: SYS, 0x347: RES}
U77_WORDS = {0x276A: RES, 0x27AB: RES}
SYS_SPANS = ((0x23F, 0x254), (0x308, 0x3D3), (0x491, 0x4A2), (0xA9, 0xAE), (0xA3F, 0xA57), (0xB0B, 0xB25))
SEGMENTS = {'stub': STUB, 'ovl': OVERLAY, 'u77': U77, 'res': RES, 'display': DISPLAY, 'sys': SYS, 'caller': base.CS, 'clear': base.CLEAR,
            'delay': base.DELAY}
SOURCES = tuple(dict.fromkeys(('overlay_u23_append_0239_cpu', 'overlay_0077_275b_cpu', 'resident_00ba_character_0bb8_cpu',
                               'resident_00ba_character_helpers_cpu', 'system_getmem_023f_cpu', 'system_runtime_leaves_cpu',
                               'system_string_leaves_cpu') + base.SOURCES))
STATUS = {-1: 'DIVERGED(expected)', 0: 'RETURN', 1: 'ARGUMENT', 2: 'CODE', 3: 'RESOLVE', 4: 'CALLEE', 5: 'TRANSFER', 6: 'LIMIT'}


class Check(ctypes.Structure):
    _fields_ = [('first', ctypes.c_int), ('second', ctypes.c_int), ('calls', ctypes.c_uint)]


class Diag(ctypes.Structure):
    _fields_ = [('callee', ctypes.c_int), ('status', ctypes.c_int), ('check', Check), ('display', base.Diagnostic), ('walked', ctypes.c_uint)]


class Limits(ctypes.Structure):
    _fields_ = [(n, ctypes.c_size_t) for n in ('clear', 'rect', 'printer', 'glyph')] + [('delay', ctypes.c_uint64),
                ('budget', ctypes.c_size_t), ('heap', ctypes.c_uint), ('walk', ctypes.c_uint)]


def source():
    with contextlib.redirect_stdout(io.StringIO()):
        bodies, decoded, meta = base.source()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    units = find_units(exe[parse_mz(exe)['header_bytes']:], ovr)
    unit = next(u for u in units if u['stub_para'] == UNIT)
    blob = ovr[unit['ovr_offset'] + ENTRY:unit['ovr_offset'] + END]
    ins = list(md.disasm(blob, ENTRY))
    digest = sha256(blob).hexdigest()
    check(len(blob) == 278 and len(ins) == 94 and sum(i.size for i in ins) == 278 and digest == BODY_SHA, '0239 extent and identity')
    check([i.address for i in ins if i.mnemonic == 'retf'] == [0x34C] and ins[-1].op_str == '8', 'single RETF 8')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [
        (0x24D, '0x85, 0x4d'), (0x260, '0xaf8, 0xb0b'), (0x265, '0xba, 0x1693'), (0x29B, '0xaf8, 0x23f'), (0x2B4, '0xaf8, 0xa3f'),
        (0x300, '0xaf8, 0x23f'), (0x319, '0xaf8, 0xa3f'), (0x344, '0xba, 0xbb8')], 'call sites')
    check([p for p in unit['fixups'] if ENTRY <= p < END] == sorted(SEG_WORDS), 'segment-word relocations')
    check(not any(i.mnemonic in ('int', 'into') or (i.mnemonic in ('call', 'jmp') and not i.op_str.startswith('0x')) for i in ins), 'no indirect transfer')
    check(unit['entries'].count(ENTRY) == 1 and unit['entries'].index(ENTRY) == 2, 'public entry slot')
    literal = ovr[unit['ovr_offset'] + LITERAL:unit['ovr_offset'] + ENTRY]
    trailing = ovr[unit['ovr_offset'] + END:unit['ovr_offset'] + 0x361]
    check(len(literal) == 11 and literal[0] == 10 and len(trailing) == 18 and trailing[0] == 17, 'literal before, string after the RETF')
    u77 = next(u for u in units if u['stub_para'] == 0x77)
    body77 = ovr[u77['ovr_offset'] + 0x275B:u77['ovr_offset'] + 0x27DF]
    ins77 = list(md.disasm(body77, 0x275B))
    check(len(body77) == 132 and len(ins77) == 45 and sha256(body77).hexdigest().startswith('91d407737828c3a1'), '275B identity')
    check(u77['memory_segment'] == 0x85 and u77['entries'][9] == 0x275B and 0x20 + 5 * 9 == 0x4D, 'stub 0085:004D is unit 77 entry 9')
    image = bytes(unpack(exe)[0])
    for kind, start, end, count, h in r115.PINS:
        if kind != 'res':
            continue
        b = image[physical(0xBA, start):physical(0xBA, end)]
        i2 = list(md.disasm(b, start))
        check(sha256(b).hexdigest() == h and len(i2) == count, f'resident pin {start:04X}')
        bodies['res', start] = b
        decoded.update({('res', i.address): i for i in i2})
    for a, b_ in SYS_SPANS:
        b = image[physical(0xAF8, a):physical(0xAF8, b_)]
        i2 = list(md.disasm(b, a))
        check(sum(i.size for i in i2) == len(b), f'system span {a:04X}')
        bodies['sys', a] = b
        decoded.update({('sys', i.address): i for i in i2})
    stub = bytes([0xEA]) + struct.pack('<HH', 0x275B, U77)
    decoded[('stub', 0x4D)] = next(md.disasm(stub, 0x4D))
    bodies['ovl', ENTRY] = blob
    bodies['u77', 0x275B] = body77
    decoded.update({('ovl', i.address): i for i in ins})
    decoded.update({('u77', i.address): i for i in ins77})
    meta['U23:0239'] = dict(bytes=278, instructions=len(ins), sha256=digest, entry_slot=2, literal_bytes=11, trailing_string_bytes=18)
    meta['U77:275B'] = dict(bytes=132, instructions=45, sha256=sha256(body77).hexdigest())
    meta['literal'] = dict(address=f'{LITERAL:04X}', length_byte=literal[0], sha256=sha256(literal).hexdigest())
    meta['segment_relocations'] = [f'{p:04X}' for p in sorted(SEG_WORDS)]
    return bodies, decoded, meta, literal


def ownership():
    """Segment-wide caller census of 0239, asserted: near `PUSH CS; CALL 0239` transfers inside unit 23h and relocated far calls
    through its stub (0028:002A, public entry 2) from every unit. Found: two near sites in 0361 and one far site in unit 20h."""
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    units = find_units(exe[parse_mz(exe)['header_bytes']:], ovr)
    unit = next(u for u in units if u['stub_para'] == UNIT)
    code = ovr[unit['ovr_offset']:unit['ovr_offset'] + unit['code_size']]
    near = [o for o in range(1, len(code) - 2) if code[o] == 0xE8 and (o + 3 + struct.unpack_from('<h', code, o + 1)[0]) & 0xFFFF == ENTRY]
    check(near == [0x3DE, 0x427] and all(code[o - 1] == 0x0E for o in near), f'near callers of 0239: {[hex(o) for o in near]}')
    stub = (unit['memory_segment'], 0x20 + 5 * unit['entries'].index(ENTRY))
    far = [(u['stub_para'], f - 3) for u in units for f in u['fixups']
           if ovr[u['ovr_offset'] + f - 3:u['ovr_offset'] + f - 2] == b'\x9a'
           and struct.unpack_from('<HH', ovr, u['ovr_offset'] + f - 2) == (stub[1], stub[0])]
    check(far == [(0x20, 0xC60)], f'far callers of 0239 through {stub[0]:04X}:{stub[1]:04X}: {far}')
    return dict(near_sites=['23:03DE (in 0361)', '23:0427 (in 0361)'], far_sites=['20:0C60 (in 0C06)'], stub=f'{stub[0]:04X}:{stub[1]:04X}')


NATIVE_STUB = ROOT / 'tools/overlay/u23_append_0239_native_stubs.c'


def library(directory, native=False):
    """The C composition; native=True links the declared 275B stub in place of the 275B model (see the native module)."""
    path = directory / ('append0239_native.so' if native else 'append0239.so')
    sources = [str(NATIVE_STUB) if (native and s == 'overlay_0077_275b_cpu') else str(ROOT / 'src/recovered' / (s + '.c')) for s in SOURCES]
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include'),
                    *sources, '-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u23_append_0239_cpu
    f.argtypes = [ctypes.POINTER(Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Limits), ctypes.POINTER(Diag)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(Ega)]
    return lib


def install(m, bodies, literal):
    for (kind, start), blob in bodies.items():
        if kind in ('ovl', 'u77', 'res', 'sys'):
            at = physical(SEGMENTS[kind], start)
            m[at:at + len(blob)] = blob
    for off, seg in SEG_WORDS.items():
        put16(m, OVERLAY, off, seg)
    for off, seg in U77_WORDS.items():
        put16(m, U77, off, seg)
    at = physical(OVERLAY, LITERAL)
    m[at:at + len(literal)] = literal
    at = physical(STUB, 0x4D)
    m[at:at + 5] = bytes([0xEA]) + struct.pack('<HH', 0x275B, U77)


def heap(m, mode, ds=DS, custom=False):
    """The SYSTEM heap variables and free list, as the 12E1 audit builds them (HeapPtr, HeapEnd, FreeList, HeapError)."""
    def word(seg, off, val):
        put16(m, seg, off & 65535, val & 65535)
    top, end, blocks = (HEAP_SEG + 0x300, 0), (HEAP_SEG + 0xFF0, 0), []
    if mode == 'exact':
        blocks = [(HEAP_SEG + 0x10, 0, 1, 8)]
    elif mode == 'split':
        blocks = [(HEAP_SEG + 0x10, 0, 8, 0)]
    elif mode == 'skip':
        blocks = [(HEAP_SEG + 0x10, 0, 0, 8), (HEAP_SEG + 0x20, 0, 1, 0), (HEAP_SEG + 0x30, 0, 1, 8)]
    elif mode == 'fail':
        end = top
    elif mode == 'cyclic':
        blocks = [(HEAP_SEG + 0x10, 0, 0, 8)]
    elif mode != 'top':
        raise ValueError(mode)
    for off, value in ((0x4400, 0), (0x4402, HEAP_SEG), (0x4404, top[1]), (0x4406, top[0]), (0x4408, end[1]), (0x440A, end[0]),
                       (0x4414, 0x1234 if custom else 0xA9), (0x4416, SYS)):
        word(ds, off, value)
    chain = blocks + [(top[0], top[1], 0, 0)]
    word(ds, 0x440C, chain[0][1])
    word(ds, 0x440E, chain[0][0])
    for (seg, off, paragraphs, granule), nxt in zip(chain, chain[1:]):
        if mode == 'cyclic':
            nxt = chain[0]
        word(seg, off, nxt[1])
        word(seg, off + 2, nxt[0])
        word(seg, off + 4, granule)
        word(seg, off + 6, paragraphs)


def node_at(i):
    return NODE_SEG, 0x2000 + i * 0x100


def build(bodies, literal, *, nodes=(), item=None, result=(RESULT_SEG, 0x40), item_ptr=(ITEM_SEG, 0x200), heap_mode='top', custom_heap=False,
          flags=2, sp=0x8000, ss=SS, ret=RETURN, cycle=False, char_fields=None, writes=(), regs=None, fill=None, gc5=None,
          alias=False, skip_contract=False, result_alias=False, char_ptr=None, expect_diverge=False, **options):
    """RAM image, CPU and the objects the body reads. `nodes` are dicts for the existing list records (weight, quantity, id)."""
    opts = dict(options)
    opts.update(sp=sp, ss=ss)
    m, c = base.build({k: v for k, v in bodies.items() if k[0] in ('display', 'caller', 'clear', 'delay')}, writes=(), **opts)
    install(m, bodies, literal)
    ds = ss if alias else DS
    items = tuple(dict(id=(i * 3) % 40, weight=nodes[i].get('weight', 1), quantity=nodes[i].get('quantity', 1)) for i in range(len(nodes)))
    template = r115.fixture(items=items, cycle=cycle, record_fields=char_fields or {}, ds=DS, ss=SS, record_seg=CHAR_SEG, node_seg=NODE_SEG)[0]
    for seg, off, length in ((CHAR_SEG, CHAR_OFF, 0x140), (NODE_SEG, 0x2000, max(1, len(nodes)) * 0x100)):
        at = physical(seg, off)
        m[at:at + length] = template[at:at + length]
    put16(m, ds, 0x5D92, char_ptr[1] if char_ptr else CHAR_OFF)
    put16(m, ds, 0x5D94, char_ptr[0] if char_ptr else CHAR_SEG)
    heap(m, heap_mode, ds, custom_heap)
    spec = dict(item or {})
    at = physical(*item_ptr)
    for j in range(0x3F):
        m[(at + j) & (SIZE - 1)] = (0x40 + j * 7) & 255
    put16(m, item_ptr[0], (item_ptr[1] + 0x37) & 65535, spec.get('weight', 1))
    m[physical(item_ptr[0], (item_ptr[1] + 0x39) & 65535)] = spec.get('quantity', 1)
    c.cs, c.ip, c.sp = OVERLAY, ENTRY, sp
    if regs:
        for name, value in regs.items():
            setattr(c, name, value)
    c.flags = flags
    for j, v in enumerate((ret[0], ret[1], result[1], result[0], item_ptr[1], item_ptr[0])):
        put16(m, ss, (sp + 2 * j) & 65535, v)
    for seg, off, blob in writes:
        for j, v in enumerate(blob):
            m[physical(seg, (off + j) & 65535)] = v
    return m, c, dict(fill=fill, gc5=gc5, ret=ret, nodes=nodes, item=spec, result=result, item_ptr=item_ptr, ds=ds, ss=ss, sp=sp,
                      skip_contract=skip_contract, result_alias=result_alias, expect_diverge=expect_diverge)


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
    status = lib.pr_u23_append_0239_cpu(ctypes.byref(bus), SIZE, ctypes.byref(actual), ctypes.byref(limits), ctypes.byref(d))
    return status, actual, bytes(ram), events, writes, bytes(ega), d


MARKERS = {(SYS, 0x23F): 'getmem', (SYS, 0xA3F): 'move', (SYS, 0xB0B): 'strload', (DISPLAY, 0x1693): 'display',
           (RES, 0xBB8): 'redraw', (U77, 0x275B): 'check'}
RETURNS = {site + 5: idx for site, (_, idx, _) in SITES.items()}


def rd16(ram, seg, off):
    a = physical(seg, off & 65535)
    return ram[a] | (ram[(a + 1) & (SIZE - 1)] << 8)


def compare(name, state, bodies, dmap, lib, budgets, refuse=None):
    """Run the original closure under Unicorn and the C composition on the same state; every difference is an error."""
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
    st = dict(stop=None, walked=0, heap_walk=0, last=None, callee=-1, status=-1)
    cap = dict(order=[], al=None, regs=None, display=None, redraw=None)
    seen, edges, events = set(), set(), []
    prev = [None]
    ds = spec['ds']

    def halt(code, nested=None):
        if nested is not None:
            st['status'] = nested
        st['stop'] = code
        uc.emu_stop()

    def hook(em, address, size, user):
        cs, ip = uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)
        key = (cs, ip)
        if (ip, cs) == spec['ret']:
            return halt(0)
        if cs == SYS and ip == 0xE2:        # RunError's entry is not part of the closure's decoded bodies: an exact-state stop
            return halt(4, 2)
        check(key in dmap, f'escape {name}: {cs:04X}:{ip:04X}')
        ins = dmap[key]
        if cs == OVERLAY and ip in SITES:
            off, idx, _ = SITES[ip]
            if bytes(uc.mem_read(address, 3)) != bytes([0x9A, off & 255, off >> 8]):
                return halt(2)
            if ip == 0x24D:
                at = physical(rd16(backing, OVERLAY, 0x250), 0x4D)
                if bytes(backing[at:at + 3]) != b'\xEA\x5B\x27':
                    return halt(3)
            st['callee'] = idx
        if cs == OVERLAY and ip in RETURNS:
            st['status'] = 0
        if cs == OVERLAY and ip == 0x2FA:
            if st['walked'] >= budgets['walk']:
                return halt(6)
            st['walked'] += 1
        if cs == SYS and ip == 0xE2:
            return halt(4, 2)
        if cs == SYS and ip == 0x23F:
            st['heap_walk'] = 0
        if cs == SYS and ip in (0x3B7, 0x3C3) and (rd16(backing, ds, 0x4414), rd16(backing, ds, 0x4416)) != (0xA9, SYS):
            return halt(4, 3)
        if key == (SYS, 0x319) and st['last'] in ((SYS, 0x32C), (SYS, 0x334)):
            st['heap_walk'] += 1
            if st['heap_walk'] > budgets['heap']:
                return halt(4, 5)
        if key in MARKERS and not (key == (RES, 0xBB8) and st['callee'] != 7):   # 275B calls 0BB8 itself: that is not the redraw
            cap['order'].append(MARKERS[key])
        if key == (U77, 0x27C2):
            cap['regs'] = tuple(uc.reg_read(r) for r in (R.UC_X86_REG_AX, R.UC_X86_REG_CX, R.UC_X86_REG_BX))
        if key == (OVERLAY, 0x252):
            cap['al'] = uc.reg_read(R.UC_X86_REG_AX) & 255
        if key in ((DISPLAY, 0x1693), (RES, 0xBB8)):
            ss_, sp_ = uc.reg_read(R.UC_X86_REG_SS), uc.reg_read(R.UC_X86_REG_SP)
            ptr = (rd16(backing, ss_, sp_ + 6), rd16(backing, ss_, sp_ + 4))
            if key[1] == 0x1693:
                at = physical(*ptr)
                cap['display'] = (ptr, bytes(backing[at:at + 1 + backing[at]]))
            elif st['callee'] == 7:
                cap['redraw'] = ptr
        seen.add(key)
        if prev[0] is not None:
            edges.add((prev[0], key))
        prev[0] = key if (ins.mnemonic.startswith('j') and ins.mnemonic != 'jmp') or ins.mnemonic.startswith('loop') else None
        st['last'] = key

    def out(em, port, size, value, user):
        events.append((port, value, size, uc.reg_read(R.UC_X86_REG_CS), uc.reg_read(R.UC_X86_REG_IP)))
        device.out(port, value, size)
    uc.hook_add(UC_HOOK_CODE, hook)
    uc.hook_add(UC_HOOK_INSN, out, None, 1, 0, R.UC_X86_INS_OUT)
    uc.emu_start(physical(c.cs, c.ip), 2 * SIZE, count=20000000)
    check(st['stop'] is not None, f'instruction ceiling {name}')
    for (kind, start), blob in bodies.items():
        if kind in ('ovl', 'u77', 'res', 'sys', 'display'):
            at = physical(SEGMENTS[kind], start)
            check(bytes(backing[at:at + len(blob)]) == m[at:at + len(blob)], f'code mutation outside contract {name}')
    limits = Limits(budgets['clear'], budgets['rect'], budgets['printer'], budgets['glyph'], budgets['delay'], budgets['budget'],
                    budgets['heap'], budgets['walk'])
    status, actual, ram, mine, stores, ega, d = run_c(lib, state, limits, refuse)
    check(status == st['stop'], f'status {name}: C {STATUS.get(status)} / original {STATUS.get(st["stop"])} {d.callee} {d.status}')
    check((d.callee, d.status, d.walked) == (st['callee'], st['status'], st['walked']),
          f'diagnostic {name}: C {(d.callee, d.status, d.walked)} / original {(st["callee"], st["status"], st["walked"])}')
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
    return dict(status=status, seen=seen, edges=edges, outs=len(events), stores=len(stores), cap=cap, ram=ram, cpu=actual,
                diag=(d.callee, d.status, d.walked), events=len(events))


def chain(ram, head_seg, head_off, limit=64):
    """The list as an independent Python walk: [(seg, off)] following the +2Ah/+2Ch links."""
    out, seg, off = [], head_seg, head_off
    while (seg, off) != (0, 0) and len(out) < limit:
        out.append((seg, off))
        off, seg = rd16(ram, seg, off + 0x2A), rd16(ram, seg, off + 0x2C)
    return out


def contract(name, state, r, literal):
    """Independent statement of a RETURNING call, from the fixture and the original's own operands (no model code).

    over-capacity iff the character's item count exceeds 15 (the chain length, recounted by 0BB8) or the 16-bit sum
    S+P exceeds R+1500 (R the 13AB result), with the signed high word deciding first (the decode of 275B, 27C2..27CC).
    Over: *RESULT = 1, the literal is copied to the local and displayed, nothing is appended, 0BB8 is not called.
    Otherwise *RESULT = 0 and one record (ITEM's 63 bytes, links cleared) is appended after the tail and 0BB8 runs once."""
    m, c, spec = state
    cap, ram = r['cap'], r['ram']
    nodes, sp, ss = spec['nodes'], spec['sp'], spec['ss']
    ax, cx, bx = cap['regs']
    signed_bx = bx - 65536 if bx >= 0x8000 else bx
    heavy = signed_bx < 0 or (signed_bx == 0 and ax > cx)
    over = len(nodes) > 15 or heavy
    check(cap['al'] == int(over), f'{name}: 275B result {cap["al"]} but the independent predicate says {int(over)}')
    rseg, roff = spec['result']
    if not 0xA0000 <= physical(rseg, roff) < 0xB0000 and not spec.get('result_alias'):
        check(ram[physical(rseg, roff)] == int(over), f'{name}: RESULT byte')
    # the list as the fixture built it, walked from the pre-call RAM (independent of both models; equals the node_at layout
    # except where a case relinks it, e.g. walk-zero-offset-link)
    base_chain = chain(m, rd16(m, CHAR_SEG, CHAR_OFF + 0xCA), rd16(m, CHAR_SEG, CHAR_OFF + 0xC8))
    char_head = chain(ram, rd16(ram, CHAR_SEG, CHAR_OFF + 0xCA), rd16(ram, CHAR_SEG, CHAR_OFF + 0xC8))
    if over:
        check(cap['order'] == ['check', 'strload', 'display'], f'{name}: over-capacity call order {cap["order"]}')
        check(char_head == base_chain, f'{name}: the list changed on the over-capacity path')
        local = physical(ss, sp - 2 - 0x13)
        check(bytes(ram[local:local + 11]) == literal and cap['display'][1] == literal, f'{name}: message copy')
        check(cap['display'][0] == (ss, (sp - 2 - 0x13) & 65535), f'{name}: display argument')
    else:
        check(cap['order'] == ['check', 'getmem', 'move', 'redraw'], f'{name}: append call order {cap["order"]}')
        check(len(char_head) == len(base_chain) + 1 and char_head[:-1] == base_chain, f'{name}: chain {len(char_head)}')
        new = char_head[-1]
        check(new not in base_chain and HEAP_SEG <= new[0] < HEAP_SEG + 0x1000, f'{name}: new record is a fresh heap block {new}')
        at, item = physical(*new), physical(*spec['item_ptr'])
        expected = bytearray(ram[item:item + 0x3F])
        expected[0x2A:0x2E] = bytes(4)
        check(bytes(ram[at:at + 0x3F]) == bytes(expected), f'{name}: the new record is ITEM with its links cleared')
        check(cap['redraw'] == (CHAR_SEG, CHAR_OFF), f'{name}: 0BB8 got the character object')
        if base_chain:
            tail = base_chain[-1]
            check((rd16(ram, tail[0], tail[1] + 0x2C), rd16(ram, tail[0], tail[1] + 0x2A)) == new, f'{name}: tail link')
    return int(over)


def budgets_of(options):
    b = dict(clear=65536, rect=65536, printer=65536, glyph=65536, delay=1000000, budget=1 << 20, heap=64, walk=1000)
    for k in tuple(options):
        if k.endswith('_limit'):
            b[k[:-6]] = options.pop(k)
    return b


def run_case(name, options, bodies, dmap, literal, lib, results):
    options = dict(options)
    refuse = options.pop('refuse', None)
    budgets = budgets_of(options)
    state = build(bodies, literal, **options)
    if state[2].get('expect_diverge'):
        # A negative control for the declared aperture hole: a pointer a FLAT callee dereferences is placed in A0000..AFFFF, where the
        # oracle reads the planes and the flat model reads RAM. The case passes only if the two DISAGREE (the hole is reachable).
        try:
            compare(name, state, bodies, dmap, lib, budgets, refuse)
        except (ValueError, AssertionError) as exc:
            results.append((name, dict(status=-1, diverged=str(exc)[:120], seen=set(), edges=set(), outs=0, stores=0, cap={}, ram=b'', cpu=None, diag=None, events=0)))
            return results[-1][1]
        raise AssertionError(f'{name}: expected the flat callee to disagree with the oracle on an aperture pointer, but they agreed')
    r = compare(name, state, bodies, dmap, lib, budgets, refuse)
    if r['status'] == 0 and not state[2].get('skip_contract'):
        r['over'] = contract(name, state, r, literal)
    results.append((name, r))
    return r


def calibrate(nodes, bodies, dmap, literal, lib, **kw):
    """The weight that puts S+P exactly on R+1500 for this list: measured from the ORIGINAL closure's own registers at 27C2
    (AX = S+P, CX = R+1500 with ITEM weight 0 and count 1), never from the model. P = weight * count with count 1."""
    results = []
    r = run_case('calibrate', dict(nodes=nodes, item=dict(weight=0, quantity=1), skip_contract=True, **kw), bodies, dmap, literal, lib, results)
    ax, cx, bx = r['cap']['regs']
    check(bx == 0 and cx >= ax, f'calibration premise: R+1500 is positive and above S ({ax:04X}, {cx:04X}, {bx:04X})')
    return (cx - ax) & 0xFFFF


def specs(bodies, dmap, literal, lib, quick=False):
    out = []
    N = lambda n, **k: tuple(dict(weight=1 + (i * 5) % 9, quantity=1 + i % 3, **k) for i in range(n))
    for n in (0, 1, 2, 3, 5, 14, 15):
        out.append((f'append-{n}', dict(nodes=N(n))))
    for mode in ('exact', 'split', 'skip'):
        for n in (0, 2):
            out.append((f'heap-{mode}-{n}', dict(nodes=N(n), heap_mode=mode)))
    # the item-count boundary: the list is recounted by 0BB8; more than 15 nodes is over capacity, exactly 15 is not
    for n in (15, 16, 17, 20):
        out.append((f'count-{n}', dict(nodes=N(n))))
    # the weight boundary, from the original's own R+1500 and S: equal is not over, one more is
    for n in (0, 2, 7):
        w = calibrate(N(n), bodies, dmap, literal, lib)
        check(0 < w < 0xFFFF, f'boundary weight for {n} nodes: {w:04X}')
        for tag, weight in (('minus1', w - 1), ('equal', w), ('plus1', w + 1)):
            out.append((f'weight-{n}-{tag}', dict(nodes=N(n), item=dict(weight=weight, quantity=1))))
        out.append((f'weight-{n}-count0-equal', dict(nodes=N(n), item=dict(weight=w, quantity=0))))
        out.append((f'weight-{n}-count0-plus1', dict(nodes=N(n), item=dict(weight=w + 1, quantity=0))))
        out.append((f'weight-{n}-count2-half', dict(nodes=N(n), item=dict(weight=w // 2, quantity=2))))
    for weight in (0, 1, 0x7FFF, 0x8000, 0xFFFF):
        for quantity in (0, 1, 2, 255):
            out.append((f'item-{weight:04x}-{quantity}', dict(nodes=N(2), item=dict(weight=weight, quantity=quantity))))
    flags_all = tuple(FLAGS) + (0x0202, 0x0602, 0x0A87, 0x0402, 0x0E57)       # the standing dirty set
    for f in flags_all:
        out.append((f'flags-append-{f:x}', dict(nodes=N(2), flags=f)))
        out.append((f'flags-over-{f:x}', dict(nodes=N(16), flags=f)))
    for sp in (0x200, 0x201, 0x8001, 0xFFDF, 0xFFE0):
        out.append((f'sp-append-{sp:x}', dict(nodes=N(2), sp=sp)))
        out.append((f'sp-over-{sp:x}', dict(nodes=N(16), sp=sp)))
    # a link whose OFFSET word is 0 but whose segment is not: still a node (the walk ORs both words); a segment-blind walk
    # would stop at node 0. Node 0's link is redirected to a fresh node at 1E00:0000 (clear of the nodes, heap, ITEM and RESULT).
    zero_node = bytes(0x37) + struct.pack('<H', 1) + bytes([1]) + bytes(0x3F - 0x3A)
    out.append(('walk-zero-offset-link', dict(nodes=N(2), writes=((0x1E00, 0x0000, zero_node),
                                                                  (NODE_SEG, 0x2000 + 0x2A, struct.pack('<HH', 0x0000, 0x1E00)))))) 
    out += [('ss-ds-alias', dict(nodes=N(2), ss=DS, alias=True, sp=0x6000)), ('a20-stack', dict(nodes=N(2), ss=0xFFFF)),
            ('a20-stack-over', dict(nodes=N(16), ss=0xFFFF))]
    # RESULT pointer: wrap, aperture, and aliases of live data (the contract skips the cases whose effect it cannot state)
    for off in (0xFFFF, 0x0000):
        out.append((f'result-offset-{off:x}', dict(nodes=N(2), result=(RESULT_SEG, off))))
        out.append((f'result-offset-over-{off:x}', dict(nodes=N(16), result=(RESULT_SEG, off))))
    for gc5 in (None, 1, 2):
        out.append((f'result-aperture-append-{gc5}', dict(nodes=N(2), result=(0xA000, 0x0100), gc5=gc5)))
        out.append((f'result-aperture-over-{gc5}', dict(nodes=N(16), result=(0xA000, 0x0100), gc5=gc5)))
    out.append(('result-aperture-wrap', dict(nodes=N(2), result=(0xAFFF, 0x0F00))))
    for tag, ptr in (('char-ptr-high', (DS, 0x5D93)), ('local-slot', (SS, (0x8000 - 2 - 4) & 65535)), ('item-byte', (ITEM_SEG, 0x200 + 0x10))):
        out.append((f'result-alias-{tag}', dict(nodes=N(2), result=ptr, result_alias=True, skip_contract=True)))
        out.append((f'result-alias-over-{tag}', dict(nodes=N(16), result=ptr, result_alias=True, skip_contract=True)))
    # adversarial aliases (VP i20701): a caller-supplied pointer placed INSIDE the block this body obtains from GetMem (HeapPtr is the
    # top of the heap, so the block is predictable), where the body itself then writes. ITEM is the Move source, RESULT is written first.
    top = HEAP_SEG + 0x300
    for tag, ptr in (('block-start', (top, 0x0)), ('block-mid', (top, 0x20)), ('block-tail', (top, 0x3A)), ('block-straddle', (top, 0x00FF - 0x20))):
        out.append((f'alias-item-{tag}', dict(nodes=N(2), item_ptr=ptr, skip_contract=True)))
        out.append((f'alias-item-{tag}-empty', dict(nodes=N(0), item_ptr=ptr, skip_contract=True)))
    for tag, ptr in (('start', (top, 0x0)), ('mid', (top, 0x1F)), ('last-byte', (top, 0x3E))):
        out.append((f'alias-result-{tag}', dict(nodes=N(2), result=ptr, result_alias=True, skip_contract=True)))
        out.append((f'alias-result-{tag}-empty', dict(nodes=N(0), result=ptr, result_alias=True, skip_contract=True)))
    # negative controls, expected to DISAGREE with the EGA oracle: pointers a FLAT callee dereferences, placed in the aperture
    out.append(('hole-item-in-aperture', dict(nodes=N(2), item_ptr=(0xA000, 0x0200), skip_contract=True, expect_diverge=True)))
    out.append(('hole-character-in-aperture', dict(nodes=N(2), char_ptr=(0xA000, 0x0100), skip_contract=True, expect_diverge=True)))
    # heap: failure stops (RunError), a custom HeapError handler, the free-list walk cap
    for n in (0, 2):
        out.append((f'heap-fail-{n}', dict(nodes=N(n), heap_mode='fail')))
        out.append((f'heap-custom-top-{n}', dict(nodes=N(n), custom_heap=True)))
        out.append((f'heap-custom-fail-{n}', dict(nodes=N(n), heap_mode='fail', custom_heap=True)))
        out.append((f'heap-cyclic-cap-{n}', dict(nodes=N(n), heap_mode='cyclic', heap_limit=2)))
    # the list walk: N nodes need N-1 backward jumps. A CYCLIC list is not a case here: 0BB8 inside 275B walks the same list
    # first and never returns (the callee budget stops it, a CALLEE stop covered by the C unit test), so the walk is never reached.
    for lim in (0, 1, 2, 3):
        out.append((f'walk-3-limit-{lim}', dict(nodes=N(3), walk_limit=lim)))
    out.append(('walk-1-limit-0', dict(nodes=N(1), walk_limit=0)))
    # code guards: an original opcode or offset that differs stops at the call, and a stub that does not resolve
    for site in SITES:
        out.append((f'code-op-{site:x}', dict(nodes=N(2), writes=((OVERLAY, site, b'\x90'),))))
        out.append((f'code-off-{site:x}', dict(nodes=N(2), writes=((OVERLAY, site + 1, b'\x00'),))))
        out.append((f'code-op-over-{site:x}', dict(nodes=N(16), writes=((OVERLAY, site, b'\x90'),))))
    out.append(('stub-opcode', dict(nodes=N(2), writes=((STUB, 0x4D, b'\x90'),))))
    out.append(('stub-target', dict(nodes=N(2), writes=((STUB, 0x4E, b'\x5C'),))))
    # display path: mode, active page, factor, planar fills and write modes, fonts, refused pages (as the 3134 closure)
    for mode in (0, 1, 2, 3, 255):
        out.append((f'display-mode-{mode}', dict(nodes=N(16), mode=mode)))
    for active in range(8):
        out.append((f'display-page-{active}', dict(nodes=N(16), active=active)))
    for gc5 in (None, 8):
        for fill in (None, (5, 6, 7, 8), (0x40, 0x41, 0x42, 0x43)):
            out.append((f'display-gc5-{gc5}-{fill and fill[0]}', dict(nodes=N(16), gc5=gc5, fill=fill)))
    out += [('display-factor-0', dict(nodes=N(16), factor=0)), ('display-factor-1', dict(nodes=N(16), factor=1)),
            ('display-same-pages', dict(nodes=N(16), pages=(0xA000, 0xA000))), ('display-ordinary-pages', dict(nodes=N(16), pages=(0xB000, 0xC000))),
            ('display-no-font', dict(nodes=N(16), writes=((DS, 0x5E20, bytes(4)),)))]
    # the literal the message is made from: its length byte decides how much StrLoad copies into the 19-byte local
    for length in (0, 1, 10, 18, 19, 20):     # 19 and 20 overwrite the saved BP; 21 or more would overwrite the return address
        out.append((f'literal-{length}', dict(nodes=N(16), writes=((OVERLAY, LITERAL, bytes([length])),), skip_contract=True)))
    rng = random.Random(0x23239)
    for i in range(12 if quick else 60):
        n = rng.choice((0, 0, 1, 2, 3, 6, 14, 15, 16, 17))
        regs = {r: rng.randrange(65536) for r in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'es')}
        out.append((f'random-{i}', dict(nodes=tuple(dict(weight=rng.randrange(65536), quantity=rng.randrange(256)) for _ in range(n)),
                                        item=dict(weight=rng.randrange(65536), quantity=rng.choice((0, 1, 2, 255, rng.randrange(256)))),
                                        heap_mode=rng.choice(('top', 'top', 'exact', 'split', 'skip')), flags=rng.choice(flags_all),
                                        sp=rng.choice((rng.randrange(0x200, 0xFFE1), 0x8000)), regs=regs, mode=rng.choice((0, 1, 2, 3)),
                                        active=rng.randrange(8), factor=i % 2)))
    return out


SMOKE = ('append-0', 'append-2', 'append-15', 'count-16', 'weight-2-equal', 'weight-2-plus1', 'heap-fail-0', 'heap-fail-2', 'walk-3-limit-1',
         'walk-3-limit-2', 'code-op-24d', 'code-off-300', 'stub-target', 'flags-append-e57', 'sp-append-201', 'walk-zero-offset-link')


def differential(bodies, decoded, literal, lib, quick=False, smoke=False):
    dmap = {(SEGMENTS[k], a): i for (k, a), i in decoded.items()}
    results = []
    for name, options in specs(bodies, dmap, literal, lib, quick):
        if smoke and name not in SMOKE:
            continue
        run_case(name, options, bodies, dmap, literal, lib, results)
        r = results[-1][1]
        print(name, STATUS[r['status']], r.get('over'), flush=True)
    seen, edges = set(), set()
    stops, over = {}, {0: 0, 1: 0}
    for name, r in results:
        seen |= r['seen']
        edges |= r['edges']
        stops[STATUS[r['status']]] = stops.get(STATUS[r['status']], 0) + 1
        if 'over' in r:
            over[r['over']] += 1
    root = {k for k in dmap if k[0] == OVERLAY}
    cond = {k: dmap[k] for k in root if dmap[k].mnemonic.startswith('j') and dmap[k].mnemonic != 'jmp'}
    expected = {(k, (OVERLAY, t)) for k, i in cond.items() for t in (k[1] + i.size, int(i.op_str, 16))}
    report = dict(cases=len(results), stops=stops, contract_checked=dict(append=over[0], over_capacity=over[1]),
                  root_starts=len(root & seen), root_total=len(root), root_outcomes=len(expected & edges), root_outcomes_total=len(expected),
                  root_missing_starts=sorted(f'{a:04X}' for _, a in root - seen),
                  root_missing_outcomes=sorted(f'{a[1]:04X}->{b[1]:04X}' for a, b in expected - edges))
    members = {}
    for name, group in (('0239', OVERLAY), ('275B', U77), ('0BB8 closure', RES), ('1693', DISPLAY), ('SYSTEM', SYS)):
        keys = {k for k in dmap if k[0] == group}
        members[name] = f'{len(keys & seen)}/{len(keys)}'
    report['closure_starts'] = members
    print(json.dumps(report, sort_keys=True))
    check(smoke or (not report['root_missing_starts'] and not report['root_missing_outcomes']), 'root coverage incomplete')
    return report


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compile-only', action='store_true')
    p.add_argument('--unicorn', action='store_true')
    p.add_argument('--quick', action='store_true')
    p.add_argument('--smoke', action='store_true', help='the sixteen cases the mutation check runs')
    p.add_argument('--native', action='store_true')
    p.add_argument('--output', type=Path)
    a = p.parse_args()
    with tempfile.TemporaryDirectory(prefix='append0239-') as tmp:
        lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK')
            return
        bodies, decoded, meta, literal = source()
        report = dict(target=meta, ownership=ownership())
        if a.unicorn:
            report['unicorn'] = differential(bodies, decoded, literal, lib, a.quick, a.smoke)
        if a.native:
            sys.modules.setdefault('u23_append_0239_cpu_audit', sys.modules[__name__])   # one module, so ctypes classes match
            from u23_append_0239_native import native
            report['native'] = native(bodies, literal, library(Path(tmp), native=True))
        if a.output:
            a.output.parent.mkdir(parents=True, exist_ok=True)
            a.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
