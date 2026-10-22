#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_007F_0054 composed with +0000 and the resident closure: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions (data, stack, record, out and a
spare). The cases where the body's own store reaches its return address (TRANSFER) cannot return to the oracle program and are Unicorn-only. Two cases probe the 16-bit-offset straddle convention
(a word at offset FFFFh): the result is REPORTED, not asserted, because the corpus convention (physically contiguous) and the real machine are known to differ there. The host's packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_007f_0054_cpu_audit as B
import ovl_007f_0000_cpu_audit as A
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16, SIZE


def native_case(directory, base, name, kw, bodies, decoded, lib, report_only=False):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, record, outseg, spare = segments; resident = code + 0x200
    kw = dict(kw); where = {'ss': ss, 'record': record, 'out': outseg}
    rec_seg = kw.pop('rec_seg', 'record'); out_seg = kw.pop('out_seg', 'out')
    opts = dict(w_cs=code, resident_cs=resident, ds=ds, ss=ss, rec=(record, 0x0100), out=(outseg, 0x0200)); opts.update(kw)
    opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002          # IF clear: a real timer interrupt would push its frame onto the observed stack (seen once: 18 bytes below SP)
    if opts['rec'][0] is None:
        opts['rec'] = (where[rec_seg], opts['rec'][1])
    if opts['out'][0] is None:
        opts['out'] = (where[out_seg], opts['out'][1])
    state = B.fixture(bodies, **opts); memory, cpu, info = state
    symbols = assemble(directory, assembly(cpu, segments, None))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    expected = B.run_c(lib, state, 100000)
    assert expected[0] == 'RETURNED', (name, expected[0])
    for index, segment in enumerate(segments):
        at = physical(segment, 0); (directory / f'I{index}.BIN').write_bytes(bytes(memory[at:at + 65536]))
    (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
    result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536, len(result)
    diffs = []
    for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
        if getattr(expected[2], n) != value:
            diffs.append((n, hex(getattr(expected[2], n)), hex(value)))
    for index, segment in enumerate(segments[1:]):
        at = physical(segment, 0); wanted = expected[1][at:at + 65536]; actual = result[28 + index * 65536:28 + (index + 1) * 65536]
        if wanted != actual:
            d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
            diffs.append(('RAM', hex(segment), d[:8], len(d)))
    if diffs and not report_only:
        raise AssertionError((name, diffs))
    return dict(case=name, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest(), equal=not diffs, differences=diffs if diffs else None, report_only=report_only)


def native(bodies, decoded, lib, quick=False):
    out = OUT / 'u7f-0054'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    v = {}
    from ovl_007f_0054_cpu_audit import original_value
    def V(k, m=0x20):
        return v.setdefault((k, m), original_value(bodies, decoded, k, m, A.W_CS, 0x7000))
    cases = []
    for k in (0x01, 0x0C, 0x10, 0x11, 0x12, 0x13, 0x16, 0x17, 0x1B, 0x1D, 0x20, 0xFF):
        for delta, tag in ((-1, 'm1'), (0, 'eq'), (1, 'p1')):
            cases.append((f'b-{k:02x}-{tag}', dict(kind=k, minor=0x20, w=0x0100, extra=0)))     # extra is filled in below from V
    cases += [(f'minor-{m:02x}', dict(kind=0x12, minor=m, w=0x0100, extra=0)) for m in (0x00, 0x33, 0x64, 0xFF)]
    cases += [(f'flags-{f:04x}', dict(kind=0x15, flags=f, w=0x20, extra=0x7000)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7)]
    cases += [('sp-min', dict(kind=0x12, sp=B.SP_MIN, w=0x10, extra=0x7000)), ('sp-max', dict(kind=0x12, sp=B.SP_MAX, w=0x10, extra=0x7000)), ('sp-odd', dict(kind=0x16, sp=0xE001, w=1, extra=0x7000))]
    S0 = 0x8000; BP = S0 - 2
    for k in (-0x10, -3, -2, -1, 0, 6, 8, 10, 12, 14):                                             # the out store lands on the local flag, the saved BP's low byte, the pointer words (never the return address)
        for branch, extra in (('above', 0x7000), ('not', 0)):
            cases.append((f'clobber-out-bp{k:+d}-{branch}', dict(out=(None, (BP + k) & 0xFFFF), out_seg='ss', sp=S0, kind=0x12, minor=0x20, w=0x0100, extra=extra)))
    for k in (-3, -2, -1, 0, 6, 12):
        for branch, extra in (('above', 0x7000), ('not', 0)):
            cases.append((f'rec102-at-bp{k:+d}-{branch}', dict(rec=(None, (BP + k - 0x102) & 0xFFFF), rec_seg='ss', sp=S0, extra=extra)))
    probes = [('probe-rec-straddle-feff-fd', dict(rec=(None, 0xFEFD), kind=0x12, minor=0x20, w=0x0100, extra=0x7000)), ('probe-out-straddle-ffff', dict(out=(None, 0xFFFF), kind=0x12, minor=0x20, w=0x0100, extra=0x7000))]
    report = []
    for index, (name, kw) in enumerate(cases):
        kw = dict(kw)
        if name.startswith(('b-', 'minor-')):
            k = kw['kind']; m = kw['minor']
            delta = {'m1': -1, 'eq': 0, 'p1': 1}.get(name.rsplit('-', 1)[-1])
            if delta is not None:
                kw['extra'] = (V(k, m) + delta - kw['w']) & 0xFFFF
            else:
                kw['extra'] = (V(k, m) + 1 - kw['w']) & 0xFFFF
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, bodies, decoded, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    for index, (name, kw) in enumerate(probes):
        directory = out / f'probe-{index}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, bodies, decoded, lib, report_only=True)); print(f'PROBE native {name}: equal={report[-1]["equal"]}', file=sys.stderr, flush=True)
    asserted = [r for r in report if not r['report_only']]
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(asserted), probes=[dict(case=r['case'], native_equals_model=r['equal'], differences=r['differences']) for r in report if r['report_only']],
                bytes_compared=len(asserted) * (5 * 65536 + 28), results=report)
