#!/usr/bin/env python3
"""OVL_0025_0411 (unit 0x25 [0411,0482), the LOAD MONSTER wrapper) against the original bytes, as a composed closure.

The wrapper allocates the 11Dh-byte record with GetMem, calls the unit 0x6F stub 007C:003E (entry 6 = +0EC4) with the record and the id, and copies the two chain
pointers into its output variables. Every callee runs as its ORIGINAL bytes in the oracle and as its merged C model in pr_u25_entry5_0411_cpu: GetMem 023F, and the whole
0EC4 closure of ovl_006f_0ec4_cpu_audit.py (Str, StrLoad, StrConcat, Move, FreeMem, the 0802:0716 cluster, 0709:0BF5 with ReadKey and QUIT, the idle, and the roll through the
00B0:0048 stub into unit 0x9E +0DE3 with SYSTEM Random). The substrate is the 0EC4 audit's: the loader-expanded START.EXE image at base 1000h with the loader's
relocation table, the unit spans placed at their own load segments with their own fixup tables, the DOS file service routed by name, the keyboard contract, the EGA device.
The ONE new substitution: the INT 3Fh overlay-manager dispatch at 007C:003E is not executed; its five stub bytes are replaced, identically in both runs, by a far JMP to the
loaded unit 0x6F +0EC4 (the 00B0:0048 substitution of the 0EC4 audit stays). Compared unmasked after every case: CPU14, ALL 1,048,576 RAM bytes, the file-service state and
log, the environment log, the full EGA state, the ordered OUT events and VRAM operands, and the end state. Independent statements pin the wrapper's call sequence and frames, its
three output variables, and (through the 0EC4 audit's own statements) the record, the chains and the five draws. Media stay under ignored resources/."""
import argparse, ctypes, json, random, struct, sys, tempfile, time
from hashlib import sha256
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
import ovl_006f_0ec4_cpu_audit as E
import res0802_0716_cpu_audit as A716
from overlay_inventory import find_units, parse_mz
from u86_procvars_3413_audit import EXE_SHA, OVR_SHA
from res0709_ega_reset_0000_cpu_audit import Cpu, ROOT, SIZE, physical, put16, check

CHECKER_ROWS = ('OVL_0025_0411',)
ENTRY, END, RETF = 0x0411, 0x0482, 0x047F
DIGEST = 'a9ba1fe3fd2cf0327048bd524dc38f1fd8f627eace099b6f379f4c4225dd716f'
BASE = E.BASE
W_CS = 0x6A00                                     # the wrapper's code segment (the unit's load segment in the test layout)
OUT_SEG, AE_AT, A6_AT, AA_AT = 0x5800, 0x0100, 0x0110, 0x0120
STATUS = E.STATUS
NON_ARCHITECTURAL = E.NON_ARCHITECTURAL
SOURCES = list(dict.fromkeys(list(E.SOURCES) + ['overlay_u25_entry5_0411_cpu']))
ONLY = []


class Diag0411(ctypes.Structure):
    _fields_ = [('site', ctypes.c_uint16), ('callee', ctypes.c_int), ('callee_status', ctypes.c_int), ('calls', ctypes.c_uint), ('inner', E.Diag0EC4)]


# ---------------------------------------------------------------------------------------------------- the span, from GAME.OVR
def source():
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    check(sha256(exe).hexdigest() == EXE_SHA and sha256(ovr).hexdigest() == OVR_SHA, 'media provenance')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    u25, u6 = units[0x25], units[0x6F]
    at = u25['ovr_offset'] + ENTRY
    body = ovr[at:at + END - ENTRY]
    check(len(body) == 113 and sha256(body).hexdigest() == DIGEST, 'unit 0x25 +0411 identity')
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, ENTRY))
    check(len(ins) == 36 and sum(i.size for i in ins) == len(body), 'decode covers the span')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'lcall'] == [(0x427, '0xaf8, 0x23f'), (0x444, '0x7c, 0x3e')], 'the two far calls')
    check([(i.address, i.op_str) for i in ins if i.mnemonic == 'retf'] == [(RETF, '0xe')], 'one RETF 0Eh')
    check(not [i for i in ins if i.mnemonic.startswith('j') or i.mnemonic in ('call', 'int', 'in', 'out')], 'straight line: no jump, near call, interrupt or port')
    fix = sorted(f for f in u25['fixups'] if ENTRY <= f < END)
    check(fix == [0x42A, 0x447], 'exactly the two far-call segment words are relocation-backed')
    check(u6['memory_segment'] == 0x7C and u6['memory_segment_sequential_ok'] and u6['entries'][6] == 0x0EC4 and 0x20 + 5 * 6 == 0x3E,
          'unit 0x6F stub segment 007Ch, entry 6 = +0EC4 at 007C:003E')
    check(struct.unpack_from('<H', body, 0x447 - ENTRY)[0] == 0x7C, 'the call names the stub segment 007Ch')
    stub25 = 0x20 + 5 * 5
    incoming = sorted(f'{u["stub_para"]:04X}:{f - 3:04X}' for u in units.values() for f in u['fixups']
                      if ovr[u['ovr_offset'] + f - 3:u['ovr_offset'] + f - 2] == b'\x9a' and
                      struct.unpack_from('<HH', ovr, u['ovr_offset'] + f - 2) == (stub25, u25['memory_segment']))
    return dict(body=body, decoded={i.address: i for i in ins}, dec_ips=set(i.address for i in ins), fix=fix, file_start=at, incoming=incoming,
                u25_stub=f'{u25["memory_segment"]:04X}:{stub25:04X}')


def library(directory, override=None):
    import subprocess
    path = directory / ('u25_0411_mut.so' if override else 'u25_0411.so')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(ROOT / 'include')] +
                   [str((override or {}).get(s, ROOT / f'src/recovered/{s}.c')) for s in SOURCES] + ['-o', str(path)], check=True)
    lib = ctypes.CDLL(str(path))
    f = lib.pr_u25_entry5_0411_cpu
    f.argtypes = [ctypes.POINTER(E.B5.Bus), ctypes.c_size_t, ctypes.POINTER(Cpu), ctypes.POINTER(E.Env0EC4), ctypes.POINTER(Diag0411)]
    f.restype = ctypes.c_int
    lib.pr_ega_reset.argtypes = [ctypes.POINTER(E.B5.Ega)]
    return f, lib


# ---------------------------------------------------------------------------------------------------- state
def build(ctx6, ctx25, reloc, stub, o, layout=None):
    """The 0EC4 audit's state, then the wrapper span, the stub 007C:003E (a far JMP to 0EC4), the output variables and the 0411 entry frame."""
    o = dict(o)
    m, c, spec = E.build(ctx6, reloc, stub, o)
    L = spec['L']; base, cs_ov = L['base'], L['cs_ov']
    w_cs = (layout or {}).get('w_cs', W_CS); out_seg = (layout or {}).get('out', OUT_SEG)
    sp, ssv = spec['sp'], spec['ssv']
    body = bytearray(ctx25['body'])
    for f in ctx25['fix']:
        struct.pack_into('<H', body, f - ENTRY, (struct.unpack_from('<H', body, f - ENTRY)[0] + base) & 0xFFFF)
    at = physical(w_cs, ENTRY); m[at:at + len(body)] = body
    at = physical(base + 0x7C, 0x3E); stub_before = bytes(m[at:at + 5]); m[at:at + 5] = struct.pack('<BHH', 0xEA, 0x0EC4, cs_ov)       # the declared trampoline
    for k in range(0x40):
        m[physical(out_seg, AE_AT + k)] = 0xEE
    outs = o.get('outs') or dict(ae=(out_seg, AE_AT), a6=(out_seg, A6_AT), aa=(out_seg, AA_AT))
    idw = (o.get('hi', 0xAB) << 8) | o.get('id', 7)
    for k, w in enumerate((L['ret'][1], L['ret'][0], outs['a6'][1], outs['a6'][0], outs['aa'][1], outs['aa'][0], outs['ae'][1], outs['ae'][0], idw)):
        put16(m, ssv, (sp + 2 * k) & 0xFFFF, w)
    c.cs, c.ip = w_cs, ENTRY
    seen_w = set()
    spec.update(outs=outs, w_cs=w_cs, bp0=(sp - 0x12) & 0xFFFF, extra_known=(w_cs, base + 0x7C), call_cs=(w_cs,), stub6_before=stub_before,
                extra_entries={(cs_ov, 0x0EC4): 'entry6'}, watch={w_cs: (ENTRY, END, ctx25['dec_ips'], seen_w)}, seen_w=seen_w)
    return m, c, spec


def cases():
    out = []
    for name, o in E.cases():
        if 'rec' in o:
            continue                                   # the record is the wrapper's GetMem result now, not a placed object
        o = dict(o)
        if o.get('sp') == 0x09F4:
            o['sp'] = 0x0A04                           # 0EC4's entry-SP bound plus the 10h this wrapper has pushed
        if o.get('sp') == 0xFFF4:
            o['sp'] = 0xFFEE                           # the wrapper's argument words reach SP+11h
        out.append((name, o))
    std = dict(b1=E.record(random.Random(1), 3), b2=bytes(range(40)), b3=bytes(range(130)))
    heap0 = E.A716.HEAP
    out += [('out-alias-a6-aa', dict(std, alias=True, outs=dict(ae=(OUT_SEG, AE_AT), a6=(OUT_SEG, A6_AT), aa=(OUT_SEG, A6_AT)))),
            ('out-alias-ae-a6', dict(std, alias=True, outs=dict(ae=(OUT_SEG, AE_AT), a6=(OUT_SEG, AE_AT + 2), aa=(OUT_SEG, AA_AT)))),
            ('out-in-heap', dict(std, alias=True, outs=dict(ae=(heap0, 0x0000), a6=(OUT_SEG, A6_AT), aa=(OUT_SEG, AA_AT)))),
            ('out-in-heap-late', dict(std, alias=True, outs=dict(ae=(heap0, 0x0080), a6=(OUT_SEG, A6_AT), aa=(heap0, 0x00C8)))),
            ('out-on-stack', dict(std, alias=True, outs=dict(ae=(E.Q.SS, 0xEA00), a6=(OUT_SEG, A6_AT), aa=(OUT_SEG, AA_AT)))),
            ('out-top-of-segment', dict(std, outs=dict(ae=(OUT_SEG, 0xFFFC), a6=(OUT_SEG, 0xFFF0), aa=(OUT_SEG, 0xFFF8)))),
            # AE^ straddles offset FFFFh: the explicit stores wrap at 16 bits, LES reads its two words physically contiguous (the corpus convention, as Unicorn does), so the
            # wrapper reads back a different pointer than it stored. Differential only: the independent statement assumes no straddle.
            ('out-ae-straddles-64k', dict(std, alias=True, outs=dict(ae=(OUT_SEG, 0xFFFE), a6=(OUT_SEG, 0xFFF0), aa=(OUT_SEG, 0xFFF8)))),
            ('id-hi-ff', dict(std, hi=0xFF)), ('id-zero', dict(std, id=0, hi=0x00))]
    return out


# ---------------------------------------------------------------------------------------------------- independent statements
def check_outer(name, calls, stop, spec, ram, o, sizes):
    """The wrapper's own call sequence and frames, and its output variables, from the listing and the case alone."""
    w = spec['w_cs']
    outer = [c for c in calls if c[2][1] == w]
    want = [(0x0427, 'getmem'), (0x0444, 'entry6')]
    got = [(site, key) for site, key, *_ in outer]
    if stop == 'RETURN':
        check(got == want, f'{name}: outer call sequence {got}')
    else:
        check(got == want[:len(got)], f'{name}: outer call sequence {got} is not a prefix at stop {stop}')
    at = lambda s, f: physical(s, f & 0xFFFF)
    rw = lambda s, f: ram[at(s, f)] | ram[at(s, f + 1)] << 8
    ae, a6, aa = spec['outs']['ae'], spec['outs']['a6'], spec['outs']['aa']
    ptr = (rw(ae[0], ae[1]), rw(ae[0], ae[1] + 2))                      # AE^ as (offset, segment)
    for site, key, words, *_ in outer:
        check(words[0] == (site + 5) & 0xFFFF and words[1] == w, f'{name}: wrapper return frame at {site:04x}')
        if key == 'getmem':
            check(words[2] == 0x11D, f'{name}: GetMem size {words[2]:04x}')
        if key == 'entry6' and not o.get('alias'):
            check((words[2], words[3]) == ptr and (words[4] & 0xFF) == o.get('id', 7), f'{name}: 0EC4 frame {words[2]:04x} {words[3]:04x} {words[4]:04x}')
    if got == [(0x0427, 'getmem')] and stop == 'RUNERROR' and not o.get('alias'):
        check(ptr == (0, 0), f'{name}: AE^ is nil when GetMem fails')
    return ptr, len(outer) == 2


# ---------------------------------------------------------------------------------------------------- the differential
def differential(ctx6, ctx25, reloc, stub, f, lib):
    specs = [x for x in cases() if not ONLY or any(t in x[0] for t in ONLY)]
    ctx6['cond_at'] = {i.address: i for i in ctx6['cond']}
    stats = dict(stops={}, calls=0, cases=len(specs)); seen6, seen25, edges = set(), set(), set(); non_arch = []; draw_cases = 0; outputs_checked = 0
    for index, (name, o0) in enumerate(specs):
        o = E.normalise(o0); t0 = time.time()
        print(f'[{index + 1}/{len(specs)}] {name}: START', file=sys.stderr, flush=True)
        if o.get('stall'):
            o['b1'] = None
        m, c, spec = build(ctx6, ctx25, reloc, stub, o)
        if o.get('stall'):
            spec['blobs']['b1'] = E.M.stall_archive(o['id']); o['b1'] = None
        r = E.run_c(f, lib, m, c, spec, diag_cls=Diag0411, status_map=STATUS)
        st, d = r['status'], r['diag']
        if st in NON_ARCHITECTURAL:
            non_arch.append((name, st, f'{d.site:04x}', d.callee, d.callee_status))
        check(st not in NON_ARCHITECTURAL, f'{name}: C {st} at site {d.site:04x} callee {d.callee} status {d.callee_status} (inner site {d.inner.site:04x}, load seam {d.inner.load.seam:04x} leaf {d.inner.load.leaf_status})')
        u = E.run_unicorn(name, ctx6, m, c, spec, stop_at=d.inner.load.decode_arrivals if st == 'STALLED' else None)
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
        # the independent statements
        ptr, reached = check_outer(name, u['calls'], u['stop'], spec, u['ram'], o, None)
        if reached and not o.get('stall') and not o.get('alias'):
            sizes = E.sizes_of(o)
            heap_lo = spec['L']['heap'] * 16; heap_hi = heap_lo + o.get('heap_paras', 0x2000) * 16
            check(ptr != (0, 0) and heap_lo <= physical(ptr[1], ptr[0]) < heap_hi, f'{name}: the record is a heap block {ptr}')
            ostub = dict(o, ssv=spec['ssv'], rec=(ptr[1], ptr[0]))
            inner = [c_ for c_ in u['calls'] if c_[2][1] == E.CS_OV]
            E.check_calls(name, ostub, sizes, inner, u['stop'], spec['bp0'])
            if u['stop'] == 'RETURN':
                ram = u['ram']; at = lambda s, f_: physical(s, f_ & 0xFFFF); rw = lambda s, f_: ram[at(s, f_)] | ram[at(s, f_ + 1)] << 8
                for key, field in (('a6', 0x7F), ('aa', 0xC8)):
                    seg, off = spec['outs'][key]
                    check((rw(seg, off), rw(seg, off + 2)) == (rw(ptr[1], ptr[0] + field), rw(ptr[1], ptr[0] + field + 2)), f'{name}: output variable {key} = R+{field:#x}')
                outputs_checked += 1
                if E.expected_record(ostub, u['ram'], sizes):
                    E.expected_record(ostub, r['ram'], sizes); draw_cases += 1
        stats['stops'][st] = stats['stops'].get(st, 0) + 1; stats['calls'] += d.calls
        seen6 |= u['seen']; edges |= u['edges']; seen25 |= spec['seen_w']
        print(f'[{index + 1}/{len(specs)}] {name}: {st}, {time.time() - t0:.1f}s', file=sys.stderr, flush=True)
    if ONLY:
        return dict(only=ONLY, cases=len(specs), end_states=stats['stops'])
    check(seen25 == ctx25['dec_ips'], f'wrapper starts missing {sorted(ctx25["dec_ips"] - seen25)}')
    check(seen6 == set(ctx6['decoded']), f'0EC4 starts missing {sorted(set(ctx6["decoded"]) - seen6)}')
    outcomes = {(i.address, t) for i in ctx6['cond'] for t in (i.address + i.size, int(i.op_str, 16))}
    missing = sorted(outcomes - edges)
    return dict(cases=len(specs), end_states=dict(sorted(stats['stops'].items())), non_architectural_stops=len(non_arch), non_architectural=non_arch,
                wrapper_far_calls_composed=stats['calls'], wrapper_starts=f'{len(seen25)}/{len(ctx25["dec_ips"])}', closure_0ec4_starts=f'{len(seen6)}/{len(ctx6["decoded"])}',
                closure_conditional_outcomes=f'{len(outcomes) - len(missing)}/{len(outcomes)}', output_variable_cases_checked=outputs_checked,
                draw_cases_checked=draw_cases, draw_bytes_checked=5 * draw_cases, dirty_flag_images=[f'{x:04X}' for x in E.DIRTY_FLAGS], cpu_words=14,
                ram_bytes=SIZE, flag_masks=0)


MUTANT_CASES = ('all-three', 'heap-0002', 'class-02', 'chain3-64', 'blocks23-absent', 'id-hi-ff', 'sp-min', 'out-in-heap-late', 'out-ae-straddles-64k')
MUTANTS = (   # (name, text in the model, replacement)
    ('GetMem size 11Ch', "c->ax = 0x011Du; push(ram, c, c->ax);", "c->ax = 0x011Cu; push(ram, c, c->ax);"),
    ('the initial nil becomes 1', "c->ax = 0; c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x44u);", "c->ax = 1; c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x44u);"),
    ('the id is read from [BP+13h]', "rd8(ram, c->ss, (uint16_t)(c->bp + 0x12u))", "rd8(ram, c->ss, (uint16_t)(c->bp + 0x13u))"),
    ('R offset and segment pushed swapped', "push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u))); push(ram, c, brd16(bus, c->es, c->di));",
     "push(ram, c, brd16(bus, c->es, c->di)); push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u)));"),
    ('AA receives R+C0/C2', "c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0xC8u)); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0xCAu));",
     "c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0xC0u)); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0xC2u));"),
    ('A6 receives R+80/82', "c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x7Fu)); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x81u));",
     "c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x80u)); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x82u));"),
    ('the stub entry must be 0EC5', "rd16(ram, stub, 0x3Fu) != 0x0EC4u", "rd16(ram, stub, 0x3Fu) != 0x0EC5u"),
    ('LES second word wraps at 64 KiB', "uint16_t seg = (uint16_t)(pr_bus_read8(bus, (at + 2u) & MASK) | ((uint16_t)pr_bus_read8(bus, (at + 3u) & MASK) << 8u));",
     "uint16_t seg = (uint16_t)(pr_bus_read8(bus, physical(c->es, (uint16_t)(c->di + 2u))) | ((uint16_t)pr_bus_read8(bus, physical(c->es, (uint16_t)(c->di + 3u))) << 8u));"),
    ('pointer reads bypass the bus (flat RAM)', "return (uint16_t)(pr_bus_read8(bus, at) | ((uint16_t)pr_bus_read8(bus, (at + 1u) & MASK) << 8u));",
     "return (uint16_t)(bus->ram[at] | ((uint16_t)bus->ram[(at + 1u) & MASK] << 8u));"),
    ('RETF pops 0Ch', "c->sp = (uint16_t)(c->sp + 4u + 0x0Eu);", "c->sp = (uint16_t)(c->sp + 4u + 0x0Cu);"),
    ('SUB SP,6', "c->sp = (uint16_t)(x - 4u); sub16_flags(c, x, 4u);", "c->sp = (uint16_t)(x - 6u); sub16_flags(c, x, 6u);"),
    ('the segment of the stub call is not read live', "stub = rd16(ram, home, 0x0447u);", "stub = 0x007Cu;"),
)


def mutation_check(ctx6, ctx25, reloc, stub):
    out = []
    ONLY[:] = list(MUTANT_CASES)
    for k, (name, old, new) in enumerate(MUTANTS):
        src = (ROOT / 'src/recovered/overlay_u25_entry5_0411_cpu.c').read_text(); t0 = time.time()
        print(f'[mutant {k + 1}/{len(MUTANTS)}] START {name}', file=sys.stderr, flush=True)
        check(src.count(old) == 1, f'mutant text occurs once: {name}')
        with tempfile.TemporaryDirectory(prefix='u25-0411-mut-') as tmp:
            path = Path(tmp) / 'mutant.c'; path.write_text(src.replace(old, new))
            mf, mlib = library(Path(tmp), override={'overlay_u25_entry5_0411_cpu': path})
            try:
                differential(ctx6, ctx25, reloc, stub, mf, mlib)
                out.append((name, 'SURVIVED'))
            except (ValueError, AssertionError) as e:
                out.append((name, 'CAUGHT: ' + str(e)[:80]))
        print(f'[mutant {k + 1}/{len(MUTANTS)}] {out[-1][1][:60]} ({time.time() - t0:.0f}s)', file=sys.stderr, flush=True)
    ONLY[:] = []
    return out


def main():
    p = argparse.ArgumentParser()
    for flag in ('--compile-only', '--unicorn', '--native', '--mutate'):
        p.add_argument(flag, action='store_true')
    p.add_argument('--output', type=Path); p.add_argument('--only', nargs='*', default=[]); a = p.parse_args()
    ONLY[:] = a.only
    with tempfile.TemporaryDirectory(prefix='u25-0411-') as tmp:
        f, lib = library(Path(tmp))
        if a.compile_only:
            print('CHECKER_BUILD_OK'); return
        image, sites = E.Q.load_image()
        ctx6 = E.source(); ctx25 = source()
        stub = E.B.assemble_stub(scratch=E.SCRATCH)
        reloc = E.Q.relocate(image, sites, BASE)
        report = dict(target=dict(span='unit 0x25 [0411,0482)', bytes=len(ctx25['body']), instructions=len(ctx25['decoded']), sha256=DIGEST, file_offset=ctx25['file_start'],
                                  image_sha256=E.Q.IMAGE_DIGEST, load_base=f'{BASE:04X}', relocated_words=len(ctx25['fix']), incoming_far_calls=ctx25['incoming'],
                                  unit25_entry5_stub=ctx25['u25_stub'], stub_segment='007C', stub_offset='003E',
                                  stub_original_bytes=reloc[physical(BASE + 0x7C, 0x3E):physical(BASE + 0x7C, 0x3E) + 5].hex(),
                                  closure_0ec4_sha256=E.DIGEST))
        if a.unicorn:
            report['unicorn'] = differential(ctx6, ctx25, reloc, stub, f, lib)
        if a.mutate:
            results = mutation_check(ctx6, ctx25, reloc, stub)
            report['mutation'] = dict(cases=list(MUTANT_CASES), results=results, caught=sum(1 for _, r in results if r.startswith('CAUGHT')), total=len(results))
        if a.native:
            import ovl_0025_0411_native
            report['native'] = ovl_0025_0411_native.native(ctx6, ctx25, f, lib, sys.modules[__name__])
    if a.output:
        a.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))


if __name__ == '__main__':
    main()
