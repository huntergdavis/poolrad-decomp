#!/usr/bin/env python3
"""OVL_0025_0341 (unit 0x25 [0341,0411), the 1E00h-byte screen block loader) against the original bytes, as a composed closure.

The body clears a global buffer with FillChar, then loops: clear the message row (unit 0x3B0 +06FB through the stub 03C5:002F), print a message with 0709:0626, build a file name
from the digit of DS:[5376], and load a block through 0802:0716; a block of size below 2 repeats the loop (an unbounded retry in the original); otherwise it copies size-2 bytes of
the block into the buffer, frees the block and clears the row again. Every callee runs as its ORIGINAL bytes in the oracle and as its merged C model in pr_u25_entry4_0341_cpu:
SYSTEM FillChar, StrLoad, Str, StrConcat, Move and FreeMem, the printer 0709:0626 with its glyph writer, the row clear (+05EB beneath it), and the whole 0802:0716 cluster. The
substrate is the 0EC4 audit's (loader-expanded START.EXE at base 1000h with the loader's relocation table, the file service routed by name, the keyboard contract, the EGA device),
with the 0341 span and its constants at their own load segment and unit 0x3B0's +06FB span at its own, both relocated by their units' fixup tables. The ONE substitution here: the
INT 3Fh overlay-manager dispatch at 03C5:002F is not executed; its five stub bytes are replaced, identically in both runs, by a far JMP to the loaded unit 0x3B0 +06FB. Compared
unmasked: CPU14, ALL 1,048,576 RAM bytes, the file service state and log, the environment log, the full EGA state, the ordered OUT events and VRAM operands, and the end state. The
retry loop is bounded by a host budget; a LIMIT stop is compared at the same arrival on both sides, and the original is shown to be at a FIXED POINT (the same CPU and RAM at
consecutive arrivals), which is what makes it a hang. Independent statements pin the call sequence and every frame, the buffer contents, and the pointer ownership. Media stay under
ignored resources/."""
import argparse, ctypes, json, random, struct, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_006f_0ec4_cpu_audit as E
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import EXE_SHA, OVR_SHA
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

CHECKER_ROWS = ('OVL_0025_0341',)
ENTRY, END, RETF, CONST_LO = 0x0341, 0x0411, 0x040E, 0x0322
DIGEST = 'b8d29a86b654e5fd944e52f2b36335cc891fc25395196501926abb581cb44e18'
U3B_SHA = 'b306a980ebf183c4dd57a62414c019d47d68787e6f6577d61da8bec63e9fc09b'
BASE = E.BASE
W_CS, U3B_CS = 0x6A00, 0x6B00
BUF = (0x7A00, 0x0000)
BUFLEN = 0x1E00
STATUS = E.STATUS
NON_ARCHITECTURAL = ('ARGUMENT', 'CODE', 'SERVICE', 'CALLEE', 'TRANSFER')      # LIMIT is expected, and only in the budget cases, at 03DC
SITES = {0x0355: ('fill', 0xAF8, 0x16A8), 0x035F: ('rowclear', 0x3C5, 0x2F), 0x0377: ('strload', 0xAF8, 0x0B0B), 0x037C: ('print', 0x709, 0x0626),
         0x0396: ('str', 0xAF8, 0x1366), 0x03A6: ('strload', 0xAF8, 0x0B0B), 0x03B0: ('strcat', 0xAF8, 0x0B98), 0x03C0: ('strload', 0xAF8, 0x0B0B),
         0x03D3: ('load', 0x802, 0x0716), 0x03F3: ('move', 0xAF8, 0x1685), 0x0401: ('freemem', 0xAF8, 0x0254), 0x0406: ('rowclear', 0x3C5, 0x2F)}
PASS = [(0x035F, 'rowclear'), (0x0377, 'strload'), (0x037C, 'print'), (0x0396, 'str'), (0x03A6, 'strload'), (0x03B0, 'strcat'), (0x03C0, 'strload'), (0x03D3, 'load')]
SOURCES = list(dict.fromkeys(list(E.SOURCES) + ['overlay_u25_entry4_0341_cpu', 'overlay_u3b0_row_clear_06fb_cpu']))
ONLY = []
BUILD_RECEIPTS = []
BUILD_RECEIPT_PATH = None


class Env0341(ctypes.Structure):
    _fields_ = [('load', E.M.Env), ('space', ctypes.c_size_t), ('printer', ctypes.c_size_t), ('glyph', ctypes.c_size_t), ('heap_steps', ctypes.c_uint), ('retries', ctypes.c_uint)]


class SpaceDiag(ctypes.Structure):
    _fields_ = [(n, ctypes.c_int) for n in ('callee', 'status', 'glyph_callee', 'glyph_status')]


class RowDiag(ctypes.Structure):
    _fields_ = [('space_status', ctypes.c_int), ('space', SpaceDiag)]


class Diag0341(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee', ctypes.c_int), ('callee_status', ctypes.c_int), ('calls', ctypes.c_uint), ('retries_taken', ctypes.c_uint),
                ('load', E.M.Diag), ('rowclear', RowDiag), ('printer', SpaceDiag)]


# ---------------------------------------------------------------------------------------------------- the spans, from GAME.OVR
def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    u25, u3b = units[0x25], units[0x3B0]
    at = u25['ovr_offset'] + ENTRY
    body = ovr[at:at + END - ENTRY]
    check(len(body) == 208 and sha256(body).hexdigest() == DIGEST, 'unit 0x25 +0341 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 89 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] ==
          sorted((a, f'{s:#x}, {o:#x}') for a, (_, s, o) in SITES.items()), 'the 12 far calls')
    cond = [i for i in ins if i.mnemonic.startswith('j')]
    check([(i.address, i.mnemonic, int(i.op_str, 16)) for i in cond] == [(0x03DC, 'jb', 0x035F)], 'one conditional branch: JB 035F at 03DC, no jump')
    check([(i.address, int(i.op_str, 0)) for i in ins if i.mnemonic == 'retf'] == [(RETF, 2)], 'one RETF 2')
    check(not [i for i in ins if i.mnemonic in ('call', 'int', 'in', 'out', 'jmp')], 'no near call, interrupt, port or jmp')
    fix = sorted(f for f in u25['fixups'] if ENTRY <= f < END)
    check(fix == sorted(a + 3 for a in SITES), 'the 12 segment words are exactly the relocation-backed words')
    consts = ovr[u25['ovr_offset'] + CONST_LO:u25['ovr_offset'] + ENTRY]
    check(len(consts) == 0x1F and not [f for f in u25['fixups'] if CONST_LO <= f < ENTRY], 'the constants are plain data, no relocation')
    names = {}
    for key, off in (('msg', 0x322), ('name', 0x338), ('ext', 0x33C)):
        s = ovr[u25['ovr_offset'] + off:u25['ovr_offset'] + off + 1 + ovr[u25['ovr_offset'] + off]]
        names[key] = bytes(s[1:])
    check([len(names[k]) for k in ('msg', 'name', 'ext')] == [21, 3, 4], 'constant lengths')
    u3b_at = u3b['ovr_offset'] + 0x06FB
    u3b_span = ovr[u3b_at:u3b_at + 0x16]
    check(sha256(u3b_span).hexdigest() == U3B_SHA, 'unit 0x3B0 +06FB identity')
    u3b_fix = sorted(f for f in u3b['fixups'] if 0x06FB <= f < 0x0711)
    check(u3b['memory_segment'] == 0x3C5 and u3b['entries'][3] == 0x06FB and 0x20 + 5 * 3 == 0x2F and u3b_fix == [0x070D], 'unit 0x3B0 stub segment 03C5h, entry 3 = +06FB at 03C5:002F')
    stub25 = 0x20 + 5 * 4
    incoming = sorted(f'{u["stub_para"]:04X}:{f - 3:04X}' for u in units.values() for f in u['fixups']
                      if ovr[u['ovr_offset'] + f - 3:u['ovr_offset'] + f - 2] == b'\x9a' and
                      struct.unpack_from('<HH', ovr, u['ovr_offset'] + f - 2) == (stub25, u25['memory_segment']))
    return dict(body=body, consts=consts, consts_sha=sha256(consts).hexdigest(), names=names, decoded={i.address: i for i in ins}, dec_ips=set(i.address for i in ins),
                fix=fix, file_start=at, u3b_span=u3b_span, u3b_fix=u3b_fix, u3b_sequential=u3b['memory_segment_sequential_ok'], incoming=incoming,
                u25_stub=f'{u25["memory_segment"]:04X}:{stub25:04X}')


def library(directory, override=None, mutation_label=None):
    import subprocess
    path = directory / ('u25_0341_mut.so' if override else 'u25_0341.so')
    command = ['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] + \
              [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)]
    compiled = subprocess.run(command, check=False)
    if BUILD_RECEIPT_PATH is not None:
        BUILD_RECEIPTS.append(dict(argv=command, exit_code=compiled.returncode,
            name=mutation_label or 'unmutated',
            variant='mutant' if override else 'unmutated',
            source_sha256={s: sha256(Path(p).read_bytes()).hexdigest() for s, p in (override or {}).items()},
            object_sha256=sha256(path.read_bytes()).hexdigest() if compiled.returncode == 0 else None,
            finished_at=subprocess.check_output(['date', '+%Y-%m-%d %H:%M:%S %Z'], text=True).strip()))
        BUILD_RECEIPT_PATH.write_text(json.dumps(BUILD_RECEIPTS, indent=2) + '\n')
    compiled.check_returncode()  # outside every differential catch: failed builds cannot count as kills
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u25_entry4_0341_cpu
    f.argtypes = [ctypes.POINTER(E.B5.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(Env0341), ctypes.POINTER(Diag0341)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(E.B5.Ega)]
    return f, lib


# ---------------------------------------------------------------------------------------------------- state
def build(ctx6, ctx25, reloc, stub, o, layout=None):
    """The 0EC4 audit's state (image, heap, DS, file service, keyboard, glyph state), then the 0341 span and its constants, the +06FB span, the stub 03C5:002F, the
    buffer pointer DS:[49DE], the buffer and the entry frame."""
    o = dict(o)
    m, c, spec = E.build(ctx6, reloc, stub, dict(o, b1=None, b2=None, b3=None))
    L = spec['L']; base = L['base']
    w_cs = (layout or {}).get('w_cs', W_CS); u3b_cs = (layout or {}).get('u3b', U3B_CS); buf = o.get('buf', (layout or {}).get('buf', BUF))
    sp, ssv = spec['sp'], spec['ssv']; ds = base + E.SEG['data']
    body = bytearray(ctx25['body'])
    for f in ctx25['fix']:
        struct.pack_into('<H', body, f - ENTRY, (struct.unpack_from('<H', body, f - ENTRY)[0] + base) & 0xFFFF)
    at = physical(w_cs, ENTRY); m[at:at + len(body)] = body
    at = physical(w_cs, CONST_LO); m[at:at + len(ctx25['consts'])] = ctx25['consts']
    span = bytearray(ctx25['u3b_span'])
    for f in ctx25['u3b_fix']:
        struct.pack_into('<H', span, f - 0x06FB, (struct.unpack_from('<H', span, f - 0x06FB)[0] + base) & 0xFFFF)
    at = physical(u3b_cs, 0x06FB); m[at:at + len(span)] = span
    at = physical(base + 0x3C5, 0x2F); stub_before = bytes(m[at:at + 5]); m[at:at + 5] = struct.pack('<BHH', 0xEA, 0x06FB, u3b_cs)       # the declared trampoline
    put16(m, ds, 0x49DE, buf[1]); put16(m, ds, 0x49E0, buf[0])
    for k in range(BUFLEN + 0x40):
        m[physical(buf[0], (buf[1] + k) & 0xFFFF)] = 0xCC
    blob = None
    if o.get('b1') is not None:
        blob = E.pack_block(o.get('id', 7), o['b1'])
    elif o.get('b1_idmiss'):
        blob = E.A716.archive([(0xEE, b'xx')])
    suffix = ctx25['names']['name'].decode('latin-1').upper()
    spec['blobs'] = {'b1': blob}; spec['suffixes'] = {'b1': suffix}
    idw = (o.get('hi', 0xAB) << 8) | o.get('id', 7)
    for k, w in enumerate((L['ret'][1], L['ret'][0], idw)):
        put16(m, ssv, (sp + 2 * k) & 0xFFFF, w)
    c.cs, c.ip = w_cs, ENTRY
    seen_w = set()
    retries = o.get('retries', 3)
    spec.update(w_cs=w_cs, u3b_cs=u3b_cs, buf=buf, bp0=(sp - 2) & 0xFFFF, extra_known=(w_cs, u3b_cs, base + 0x3C5), call_cs=(w_cs,), stub_before=stub_before,
                extra_entries={(E.SYS, 0x16A8): 'fill', (base + 0x0709, 0x0626): 'print', (u3b_cs, 0x06FB): 'rowclear'},
                watch={w_cs: (ENTRY, END, ctx25['dec_ips'], seen_w)}, seen_w=seen_w, retries=retries, retry_limit=(w_cs, 0x03DC, retries))
    return m, c, spec


def make_env(retries):
    def factory(load, ke):
        return Env0341(load, 65536, 65536, 65536, 100000, retries)
    return factory


def cases():
    rng = random.Random(0x250341)
    raw = lambda n: bytes(rng.randrange(256) for _ in range(n))
    K = E.K
    out = []

    def add(name, **kw):
        out.append((name, kw))
    for n in (2, 3, 4, 9, 100, 400, 1000, 0x1E00, 0x1E02, 0x1F00):
        add(f'size-{n:x}', b1=raw(n))
    add('retry-id-missing', b1_idmiss=True, retries=3)
    add('retry-size-0', b1=b'', retries=3)
    add('retry-size-1', b1=raw(1), retries=3)
    add('retry-budget-0', b1_idmiss=True, retries=0)
    add('retry-budget-1', b1=raw(1), retries=1)
    add('retry-budget-5', b1_idmiss=True, retries=5)
    for digit in (0, 1, 9, 10, 99, 100, 255):
        add(f'digit-{digit}', b1=raw(50), digit=digit)
    add('file-absent-prompt-ctrl-c', keys=(0x1C0D, K['c']), cur=2, saved=1)
    add('file-absent-prompt-never', keys=(0x1C0D, 0x1C0D, K['c']))
    add('file-absent-prompt-then-appears', b1=raw(60), appear_after=2, keys=(0x1C0D, 0x1C0D))
    add('file-absent-break', keys=(0x1C0D,), brk=1)
    add('stall-in-load', id=40, stall=True, keys=(K['a'],))
    for hp in (0x0002, 0x0010, 0x0020, 0x0030, 0x0040, 0x0060, 0x0100):
        add(f'heap-{hp:04x}', b1=raw(400), heap_paras=hp)
    add('sp-min', sp=0x09EC, b1=raw(40)); add('sp-max', sp=0xFFFA, b1=raw(40)); add('sp-odd', sp=0xF001, b1=raw(40)); add('ss-ds', ss='ds', sp=0xE000, b1=raw(40))
    for fl in E.Q.FLAGS + E.DIRTY_FLAGS:
        add(f'flags{fl:04x}', flags=fl, b1=raw(80))
    for mode in (0, 1, 2, 3):
        add(f'mode{mode}', mode=mode, vmode=mode, b1=raw(30))
    for pa, pb in ((0xA000, 0xA200), (0xA200, 0xA000), (0xA400, 0xA600)):
        add(f'pages{pa:04x}', pages=(pa, pb), mode=1, b1=raw(30))
    add('nofont', fontoff=0, b1=raw(30))
    add('buf-wrap', buf=(0x7A00, 0xF000), b1=raw(500))
    add('buf-offset-ff', buf=(0x7A00, 0xFFFF), b1=raw(300))
    # the VP's adversarial shapes (i20700): a caller-supplied pointer placed INSIDE a block the body itself returns or frees
    add('buf-inside-block', buf=(0x3000, 0x0100), b1=raw(1000), alias=True)                 # the buffer overlaps the block 0716 returns (the first heap allocation) and FreeMem frees
    add('buf-inside-block-low', buf=(0x3000, 0x0000), b1=raw(1000), alias=True)             # the buffer starts at the block's first byte: Move copies the block over itself shifted by 2
    for k in range(12):
        n = rng.choice((2, 3, 17, 255, 256, 1000, 0x1E00, rng.randrange(2, 0x2000)))
        add(f'random{k}', b1=raw(n), regs=tuple(rng.randrange(65536) for _ in range(7)), flags=rng.choice(E.Q.FLAGS + E.DIRTY_FLAGS), digit=rng.randrange(256),
            hi=rng.randrange(256), id=rng.randrange(1, 255), seed=rng.randrange(2 ** 32), keys=(K['a'],))
    # Append after the random cases so the original 74 fixtures remain identical.
    add('bufptr-in-returned-block', bufptr_in_block=True, alias=True)
    return out


# ---------------------------------------------------------------------------------------------------- independent statements
def expected_calls(o, size, spec, passes):
    bp, ss, w, buf = spec['bp0'], spec['ssv'], spec['w_cs'], spec['buf']
    chk = {}
    chk['fill'] = {2: 0, 3: BUFLEN, 4: buf[1], 5: buf[0]}
    chk['rowclear'] = {}
    chk[(0x0377, 'strload')] = {2: 0x0322, 3: w, 4: (bp - 0x22) & 0xFFFF, 5: ss}
    chk['print'] = {2: (bp - 0x22) & 0xFFFF, 3: ss}
    chk['str'] = {2: 1, 3: (bp - 6) & 0xFFFF, 4: ss, 5: 0, 6: o.get('digit', 3), 7: 0}
    chk[(0x03A6, 'strload')] = {2: 0x0338, 3: w, 4: (bp - 0x10C) & 0xFFFF, 5: ss}
    chk['strcat'] = {2: (bp - 6) & 0xFFFF, 3: ss, 4: (bp - 0x10C) & 0xFFFF, 5: ss}
    chk[(0x03C0, 'strload')] = {2: 0x033C, 3: w, 4: (bp - 0x111) & 0xFFFF, 5: ss}
    chk['load'] = {2: (bp - 0xC) & 0xFFFF, 3: ss, 4: (bp - 8) & 0xFFFF, 5: ss, 6: o.get('id', 7), 7: (bp - 0x111) & 0xFFFF, 8: ss, 9: (bp - 0x10C) & 0xFFFF, 10: ss}
    seq = [(0x0355, 'fill')]
    for _ in range(passes):
        seq += PASS
    if size >= 2 and passes:
        seq += [(0x03F3, 'move'), (0x0401, 'freemem'), (0x0406, 'rowclear')]
    return seq, chk


def check_calls(name, o, calls, stop, spec, ram, retries_taken):
    w = spec['w_cs']
    outer = [c for c in calls if c[2][1] == w]
    got = [(site, key) for site, key, *_ in outer]
    at = lambda s, f: physical(s, f & 0xFFFF)
    rw = lambda s, f: ram[at(s, f)] | ram[at(s, f + 1)] << 8
    bp, ss = spec['bp0'], spec['ssv']
    size = rw(ss, bp - 8)
    passes = sum(1 for g in got if g == (0x03D3, 'load'))
    reached_load = passes > 0
    want, chk = expected_calls(o, size if stop == 'RETURN' else 0, spec, max(1, passes) if stop != 'RETURN' else max(1, passes))
    if stop == 'RETURN':
        check(got == want, f'{name}: call sequence {[hex(a) for a, _ in got][:8]}.. ({len(got)}/{len(want)})')
    else:
        want = [(0x0355, 'fill')] + PASS * 64
        check(got == want[:len(got)], f'{name}: stop {stop}: call sequence is not a prefix of the statement at {len(got)}')
    for site, key, words, spv, _, local in outer:
        check(words[0] == (site + 5) & 0xFFFF and words[1] == w, f'{name}: return frame at {site:04x}')
        c_ = chk.get((site, key)) or chk.get(key) or {}
        for k, v in c_.items():
            mask = 0xFF if (key in ('fill',) and k == 2) or (key == 'load' and k == 6) else 0xFFFF
            check((words[k] & mask) == v, f'{name}: {key}@{site:04x} word {k} = {words[k]:04x}, expected {v:04x}')
        if key == 'print':
            check((words[4] & 0xFF, words[5] & 0xFF, words[6] & 0xFF) == (0x0F, 0x18, 0x00), f'{name}: print colour/y/x {words[4]:04x} {words[5]:04x} {words[6]:04x}')
        if key == 'move':
            blk = (rw(ss, bp - 0xC), rw(ss, bp - 0xA))
            check(words[2] == size - 2 and (words[4], words[3]) == spec['buf_at_move'] and (words[5], words[6]) == ((blk[0] + 2) & 0xFFFF, blk[1]), f'{name}: Move frame {[hex(x) for x in words[:7]]}')
        if key == 'freemem':
            blk = (rw(ss, bp - 0xC), rw(ss, bp - 0xA))
            check((words[2], words[3], words[4]) == (size, blk[0], blk[1]), f'{name}: FreeMem frame')
    return size


def expected_buffer(o, raw, spec, ram):
    """After a returning case the buffer holds the block's bytes from offset 2, then fill (zero) to its end."""
    seg, off = spec['buf_at_move']
    n = len(raw) - 2
    for k in range(min(n, BUFLEN + 0x40)):
        check(ram[physical(seg, (off + k) & 0xFFFF)] == raw[2 + k], f'buffer byte {k}')
    for k in range(max(n, 0), BUFLEN):
        check(ram[physical(seg, (off + k) & 0xFFFF)] == 0, f'buffer fill byte {k}')


# ---------------------------------------------------------------------------------------------------- the differential
def differential(ctx6, ctx25, reloc, stub, f, lib):
    specs = [x for x in cases() if not ONLY or any(t in x[0] for t in ONLY)]
    if 'bufptr-in-returned-block' in ONLY:
        specs.sort(key=lambda case: case[0] != 'bufptr-in-returned-block')
    ctx6['cond_at'] = {}
    stats = dict(stops={}, calls=0); seen25, non_arch = set(), []; limit_cases = []; buffer_cases = 0; edge_taken = set(); pointer_alias_cases = []
    for index, (name, o0) in enumerate(specs):
        o = E.normalise(o0); t0 = time.time()
        print(f'[{index + 1}/{len(specs)}] {name}: START', file=sys.stderr, flush=True)
        stall = o.pop('stall', False)
        pointer_alias = o.pop('bufptr_in_block', False)
        if pointer_alias:
            ds_at = (E.BASE + E.SEG['data']) * 16 + 0x49DE
            first = ds_at & 15
            data = bytearray([0x77] * 32); newbuf = (0x7C00, 0x0000)
            data[:2] = b'\x12\x34'  # two distinct header bytes keep the packed allocation at 16 bytes
            data[first:first + 4] = struct.pack('<HH', newbuf[1], newbuf[0])
            # 0716 allocates its packed buffer through 0020 BEFORE its returned
            # block. Both allocations round to eight bytes. Position the second
            # allocation at the paragraph containing the pointer variable.
            packed_bytes = len(E.A716.rle_pack(data))
            packed_allocation = (packed_bytes + 7) & ~7
            block_at = ds_at - first
            heap_at = block_at - packed_allocation
            check(heap_at % 16 == 0, 'alias fixture heap must be paragraph aligned')
            o['L'] = dict(heap=heap_at >> 4); o['heap_paras'] = 0x0200
            o['b1'] = bytes(data); o['newbuf'] = newbuf
        m, c, spec = build(ctx6, ctx25, reloc, stub, o)
        if pointer_alias:
            check(m[ds_at:ds_at + 4] == struct.pack('<HH', spec['buf'][1], spec['buf'][0]),
                  f'{name}: initial pointer cell must name the FillChar destination')
        if stall:
            spec['blobs']['b1'] = E.M.stall_archive(o['id'])
        r = E.run_c(f, lib, m, c, spec, diag_cls=Diag0341, status_map=STATUS, make_env=make_env(spec['retries']))
        st, d = r['status'], r['diag']
        budget_case = name.startswith('retry-')
        if st in NON_ARCHITECTURAL or (st == 'LIMIT' and not budget_case):
            non_arch.append((name, st, f'{d.site:04x}', d.callee, d.callee_status))
        check(st not in NON_ARCHITECTURAL and (st != 'LIMIT' or budget_case), f'{name}: C {st} at site {d.site:04x} callee {d.callee} status {d.callee_status} '
              f'(load seam {d.load.seam:04x} leaf {d.load.leaf_status}; rowclear {d.rowclear.space_status}; printer {d.printer.status}/{d.printer.glyph_status})')
        u = E.run_unicorn(name, ctx6, m, c, spec, stop_at=d.load.decode_arrivals if st == 'STALLED' else None)
        check(st == u['stop'], f'{name}: C {st} original {u["stop"]}')
        E.A716.compare_cpu(name, r['cpu'], u['cpu'])
        if r['ram'] != u['ram']:
            dd = [i for i in range(SIZE) if r['ram'][i] != u['ram'][i]]
            print(name, [(hex(i), hex(r['ram'][i]), hex(u['ram'][i])) for i in dd[:12]], f'({len(dd)} bytes)', file=sys.stderr)
        E.A716.compare_ram(name, r['ram'], u['ram'])
        check(r['fs'].state() == u['fs'].state(), f'{name}: file state'); check(r['fs'].log == u['fs'].log, f'{name}: file log')
        check(r['env'].log == u['env'].log, f'{name}: environment log')
        check(r['events'] == u['ega'], f'{name}: EGA OUT events'); check(r['writes'] == u['writes'], f'{name}: VRAM write operands')
        check(r['ega'] == u['ega_packed'], f'{name}: EGA state')
        if st == 'LIMIT':
            check(d.site == 0x03DC and r['cpu'].ip == 0x03DC and d.retries_taken == spec['retries'], f'{name}: LIMIT at 03DC after {d.retries_taken} loop-backs')
            snaps = u['retry_snaps']
            check(len(snaps) == spec['retries'] + 1, f'{name}: {len(snaps)} arrivals at 03DC with the loop-back pending, budget {spec["retries"]}')
            hashes = [x[1] for x in snaps]
            classes = [hashes.index(h) for h in hashes]
            cpu_same = len({x[0] for x in snaps}) == 1
            if len(snaps) >= 3 and o.get('b1_idmiss'):          # id missing: nothing is allocated per pass, so arrival 2 onwards is the same CPU and the same 1 MiB = a fixed point
                check(cpu_same and len(set(hashes[1:])) == 1, f'{name}: not a fixed point across arrivals')
            limit_cases.append((name, len(snaps), len(snaps) >= 3, cpu_same, classes))
        if not (stall or o.get('alias')):
            spec['buf_at_move'] = spec['buf']
            raw = o.get('b1')
            size = check_calls(name, o, u['calls'], u['stop'], spec, u['ram'], d.retries_taken)
            if u['stop'] == 'RETURN' and raw is not None:
                expected_buffer(o, raw, spec, u['ram']); expected_buffer(o, raw, spec, r['ram']); buffer_cases += 1
                at = lambda s_, f_: physical(s_, f_ & 0xFFFF)
                check(u['ram'][at(E.BASE + E.SEG['data'], 0x557C)] == 0, f'{name}: DS:[557C] is 0 on return')
        elif o.get('alias') and u['stop'] == 'RETURN' and o.get('b1') is not None:
            # the adversarial shapes: the statement is the corpus-wide equality plus the buffer the body was handed, read from the pointer it used last
            seg, off = o.get('newbuf', spec['buf']); raw = o['b1']
            n = len(raw) - 2
            if o.get('newbuf'):
                for k in range(min(n, 0x1E00)):
                    check(u['ram'][physical(seg, off + k)] == raw[2 + k], f'{name}: relocated buffer byte {k}')
                buffer_cases += 1
        if pointer_alias:
            check(st == 'RETURN', f'{name}: alias must reach the post-load Move and return, got {st}')
            moves = [call for call in u['calls'] if call[0:2] == (0x03F3, 'move') and call[2][1] == spec['w_cs']]
            check(len(moves) == 1, f'{name}: expected one original Move frame')
            words = moves[0][2]
            returned_block = (words[6], (words[5] - 2) & 0xFFFF)
            returned_at = physical(*returned_block)
            check(returned_at == block_at and returned_at <= ds_at and ds_at + 4 <= returned_at + len(data),
                  f'{name}: DS:[49DE] must lie wholly inside the actual returned block')
            check(spec['buf'] != newbuf and (words[4], words[3]) == newbuf,
                  f'{name}: Move must use the overwritten pointer, not the initial FillChar destination')
            spec['buf_at_move'] = newbuf
            check_calls(name, o, u['calls'], u['stop'], spec, u['ram'], d.retries_taken)
            for result in (r, u):
                check(result['ram'][ds_at:ds_at + 4] == struct.pack('<HH', newbuf[1], newbuf[0]),
                      f'{name}: final pointer must retain the decoded value')
                at = physical(*newbuf); copied = len(data) - 2
                check(result['ram'][at:at + copied] == bytes(data[2:]), f'{name}: redirected payload')
                check(result['ram'][at + copied:at + BUFLEN] == m[at + copied:at + BUFLEN],
                      f'{name}: no FillChar on the newly selected destination')
                old_at = physical(*spec['buf'])
                check(result['ram'][old_at:old_at + BUFLEN] == bytes(BUFLEN),
                      f'{name}: initial destination is cleared but receives no copied payload')
            pointer_alias_cases.append(dict(name=name, stop=st, pointer_cell=f'{ds_at:05X}',
                returned_block=f'{returned_block[0]:04X}:{returned_block[1]:04X}', block_bytes=len(data),
                packed_bytes=packed_bytes, packed_allocation_bytes=packed_allocation,
                pointer_offset_in_block=ds_at-returned_at, original_buffer=f'{spec["buf"][0]:04X}:{spec["buf"][1]:04X}',
                move_destination=f'{newbuf[0]:04X}:{newbuf[1]:04X}', copied_bytes=len(data)-2))
        stats['stops'][st] = stats['stops'].get(st, 0) + 1; stats['calls'] += d.calls
        seen25 |= spec['seen_w']
        print(f'[{index + 1}/{len(specs)}] {name}: {st}, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
    if ONLY:
        return dict(only=ONLY, cases=len(specs), end_states=stats['stops'], pointer_alias_cases=pointer_alias_cases)
    check(seen25 == ctx25['dec_ips'], f'0341 starts missing {sorted(ctx25["dec_ips"] - seen25)}')
    return dict(cases=len(specs), end_states=dict(sorted(stats['stops'].items())), non_architectural_stops=len(non_arch), non_architectural=non_arch,
                far_calls_composed=stats['calls'], starts=f'{len(seen25)}/{len(ctx25["dec_ips"])}', retry_loop=dict(limit_cases=limit_cases),
                buffer_cases_checked=buffer_cases, pointer_alias_cases=pointer_alias_cases,
                dirty_flag_images=[f'{x:04X}' for x in E.DIRTY_FLAGS], cpu_words=14, ram_bytes=SIZE, flag_masks=0)


def aperture_demo(ctx6, ctx25, reloc, stub, f, lib):
    """Construct the shape the 0411 audit found: a buffer pointer aimed into A0000..AFFFF. FillChar and Move are flat-RAM SYSTEM models whose contracts exclude the aperture, so the model and the
    original are NOT expected to agree here; the run records what each did. It is a demonstration of a contract limit, not a case of the differential."""
    o = E.normalise(dict(b1=bytes(range(200)), buf=(0xA000, 0x0100)))
    m, c, spec = build(ctx6, ctx25, reloc, stub, o)
    ctx6['cond_at'] = {}
    r = E.run_c(f, lib, m, c, spec, diag_cls=Diag0341, status_map=STATUS, make_env=make_env(spec['retries']))
    u = E.run_unicorn('aperture-demo', ctx6, m, c, spec)
    diff = [i for i in range(SIZE) if r['ram'][i] != u['ram'][i]]
    cpu_diff = [n for (n, _) in Cpu._fields_ if getattr(r['cpu'], n) != getattr(u['cpu'], n)]
    return dict(buffer='A000:0100', c_status=r['status'], original_stop=u['stop'], ram_bytes_differing=len(diff), first_differing=[hex(i) for i in diff[:6]],
                cpu_words_differing=cpu_diff, ega_state_equal=(r['ega'] == u['ega_packed']), agree=(not diff and not cpu_diff and r['ega'] == u['ega_packed']))


def rng_bytes(n, seed):
    r = random.Random(seed)
    return bytes(r.randrange(256) for _ in range(n))


MUTANT_CASES = ('bufptr-in-returned-block', 'size-64', 'size-2', 'size-3', 'retry-id-missing', 'retry-budget-0', 'mode1', 'digit-10', 'sp-min', 'buf-inside-block', 'file-absent-prompt-then-appears')
MUTANTS = (
    ('FillChar count 1E01h', "c->ax = 0x1E00u; push(&m, c->ax);", "c->ax = 0x1E01u; push(&m, c->ax);"),
    ('FillChar value 1', "set_al(c, 0u); push(&m, c->ax); /* 0352 MOV AL,0; PUSH AX */", "set_al(c, 1u); push(&m, c->ax); /* 0352 MOV AL,0; PUSH AX */"),
    ('JB sense inverted', "if (!(c->flags & 1u)) break;", "if (c->flags & 1u) break;"),
    ('budget off by one (> for >=)', "if (d->retries_taken >= env->retries)", "if (d->retries_taken > env->retries)"),
    ('the size test uses 3', "sub16_flags(c, bp16(&m, -8), 2u);", "sub16_flags(c, bp16(&m, -8), 3u);"),
    ('Move source is block+3', "add16_flags(c, c->di, 2u); c->di = (uint16_t)(c->di + 2u);", "add16_flags(c, c->di, 3u); c->di = (uint16_t)(c->di + 3u);"),
    ('Move count size-1 (one DEC)', "    dec16_flags(c, c->ax); c->ax = (uint16_t)(c->ax - 1u); /* 03F1 DEC AX */\n", ""),
    ('the id is read from [BP+7]', "rd8(ram, c->ss, (uint16_t)(c->bp + 6u))", "rd8(ram, c->ss, (uint16_t)(c->bp + 7u))"),
    ('print colour 0Eh', "set_al(c, 0x0Fu); push(&m, c->ax);   /* 0364", "set_al(c, 0x0Eu); push(&m, c->ax);   /* 0364"),
    ('print row 17h', "set_al(c, 0x18u); push(&m, c->ax);", "set_al(c, 0x17u); push(&m, c->ax);"),
    ('DS:[557C] set to 1', "wr8(ram, c->ds, 0x557Cu, 0u);", "wr8(ram, c->ds, 0x557Cu, 1u);"),
    ('Str maximum length 2', "lea_di(&m, -6); push_ss_di(&m); c->ax = 1; push(&m, c->ax);", "lea_di(&m, -6); push_ss_di(&m); c->ax = 2; push(&m, c->ax);"),
    ('the closing row clear is dropped', "    CALL(0x0406u, PR_U25_0341_ROWCLEAR);\n", ""),
    ('the stub entry must be 06FC', "rd16(ram, seg, 0x30u) != 0x06FBu", "rd16(ram, seg, 0x30u) != 0x06FCu"),
)


def mutation_check(ctx6, ctx25, reloc, stub):
    out = []
    ONLY[:] = list(MUTANT_CASES)
    for k, (name, old, new) in enumerate(MUTANTS):
        src = (ROOT / 'src/recovered/overlay_u25_entry4_0341_cpu.c').read_text(); t0 = time.time()
        print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        check(src.count(old) == 1, f'mutant text occurs once: {name}')
        with tempfile.TemporaryDirectory(prefix='u25-0341-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(src.replace(old, new))
            mf, mlib = library(Path(tmp), override={'overlay_u25_entry4_0341_cpu': path}, mutation_label=name)
            try:
                differential(ctx6, ctx25, reloc, stub, mf, mlib)
                out.append((name, 'SURVIVED'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:80]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:60]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    # The new case must reject a model that caches the first LES destination.
    # Compile outside the catch: a build failure is never a caught mutant.
    src = (ROOT / 'src/recovered/overlay_u25_entry4_0341_cpu.c').read_text()
    first_load = next(line for line in src.splitlines() if '/* 0348 LES DI,[49DEh] */' in line)
    second_load = next(line for line in src.splitlines() if '/* 03E7 LES DI,[49DEh] */' in line)
    check(src.count(first_load) == src.count(second_load) == 1, 'pointer reload mutation sites are unique')
    src = src.replace(first_load, first_load + '\n    uint16_t saved_buffer_off = c->di, saved_buffer_seg = c->es;')
    src = src.replace(second_load, '    c->di = saved_buffer_off; c->es = saved_buffer_seg; /* deliberately stale destination */')
    ONLY[:] = ['bufptr-in-returned-block']
    name = 'Move reuses the pre-load buffer pointer'
    print(f'[alias mutant] START {name}', file=sys.stderr, flush=True)
    with tempfile.TemporaryDirectory(prefix='u25-0341-alias-mut-') as tmp:
        path = Path(tmp) / 'mutant.c'; path.write_text(src)
        mf, mlib = library(Path(tmp), override={'overlay_u25_entry4_0341_cpu': path}, mutation_label=name)
        try:
            differential(ctx6, ctx25, reloc, stub, mf, mlib)
            out.append((name, 'SURVIVED'))
        except (ValueError, AssertionError) as e:
            out.append((name, 'CAUGHT: ' + str(e)[:80]))
    print(f'[alias mutant] {out[-1][1]}', file=sys.stderr, flush=True)
    ONLY[:] = []
    return out


def main():
    global BUILD_RECEIPT_PATH
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native', '--mutate', '--demo'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); p.add_argument('--only', nargs='*', default=[]); a = p.parse_args()
    ONLY[:] = a.only
    BUILD_RECEIPTS.clear()
    BUILD_RECEIPT_PATH = a.output.with_suffix('.compiled-mutants.json') if a.mutate and a.output else None
    with tempfile.TemporaryDirectory(prefix='u25-0341-') as tmp:
        f, lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = E.Q.load_image()
        ctx6 = E.source(); ctx25 = source()
        stub = E.B.assemble_stub(scratch=E.SCRATCH)
        reloc = E.Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='unit 0x25 [0341,0411)', bytes=len(ctx25['body']), instructions=len(ctx25['decoded']), sha256=DIGEST, file_offset=ctx25['file_start'],
                                  constants_span=f'{CONST_LO:04X}+{len(ctx25["consts"]):X}', constants_sha256=ctx25['consts_sha'], image_sha256=E.Q.IMAGE_DIGEST,
                                  load_base=f'{BASE:04X}', relocated_words=len(ctx25['fix']), incoming_far_calls=ctx25['incoming'], unit25_entry4_stub=ctx25['u25_stub'],
                                  stub_segment='03C5', stub_offset='002F', unit3b0_sequential_check=ctx25['u3b_sequential'], unit3b0_span_sha256=U3B_SHA,
                                  stub_original_bytes=reloc[physical(BASE + 0x3C5, 0x2F):physical(BASE + 0x3C5, 0x2F) + 5].hex()))
        if a.unicorn:
            report['unicorn'] = differential(ctx6, ctx25, reloc, stub, f, lib)
        if a.demo:
            report['aperture_demo'] = aperture_demo(ctx6, ctx25, reloc, stub, f, lib)
        if a.mutate:
            results = mutation_check(ctx6, ctx25, reloc, stub)
            report['mutation'] = dict(cases=list(MUTANT_CASES), alias_case='bufptr-in-returned-block', results=results,
                                      caught=sum(1 for _, r in results if r.startswith('CAUGHT')), total=len(results))
            check(report['mutation']['caught'] == report['mutation']['total'], 'one or more compiled mutants survived')
        if a.native:
            import ovl_0025_0341_native
            report['native'] = ovl_0025_0341_native.native(ctx6, ctx25, f, lib, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
