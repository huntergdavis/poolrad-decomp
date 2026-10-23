#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_006A_0824: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions (data, stack, array, two
spares). Only cases the model RETURNS from are run natively (an array over the return address transfers elsewhere and cannot return to the oracle program; an array over the loop counters may not
terminate), so the native set is the returning subset of the frame placements. IF is cleared in every native case. The host's packaged dosbox-x is used, not the pinned build."""
import random, shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_006a_0824_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16, SIZE


def native_case(directory, base, name, kw, bodies, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, grid, spare1, spare2 = segments
    kw = dict(kw); where = {'ss': ss, 'grid': grid, 'ds': ds}; g_seg = kw.pop('g_seg', 'grid')
    opts = dict(w_cs=code, ds=ds, ss=ss, g=(grid, 0x0100)); opts.update(kw)
    if opts['g'][0] is None:
        opts['g'] = (where[g_seg], opts['g'][1])
    opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002          # IF clear: a real timer interrupt would push its frame onto the observed stack
    state = B.fixture(bodies, **opts); memory, cpu, info = state
    symbols = assemble(directory, assembly(cpu, segments, None))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    expected = B.run_c(lib, state, B.BUDGET)
    if expected[0] != 'RETURNED':
        return None
    for index, segment in enumerate(segments):
        at = physical(segment, 0); (directory / f'I{index}.BIN').write_bytes(bytes(memory[at:at + 65536]))
    (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
    result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536, len(result)
    for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
        assert getattr(expected[2], n) == value, (name, n, hex(getattr(expected[2], n)), hex(value))
    for index, segment in enumerate(segments[1:]):
        at = physical(segment, 0); wanted = expected[1][at:at + 65536]; actual = result[28 + index * 65536:28 + (index + 1) * 65536]
        if wanted != actual:
            d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
            raise AssertionError((name, 'RAM', hex(segment), d[:8], len(d)))
    return dict(case=name, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest())


def native(bodies, decoded, lib, quick=False):
    out = OUT / 'u6a-0824'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    rng = random.Random(0x6A24); rb = lambda: [rng.randrange(256) for _ in range(21)]
    fixed = [9, 3, 0x85, 1, 0x80, 4, 4, 0x7F, 2, 0xFF, 6, 0, 8, 7, 5, 0x81, 12, 11, 10, 14, 13]
    specials = {'asc': list(range(21)), 'desc': list(range(20, -1, -1)), 'equal': [7] * 21, 'equal-hi': [0x87] * 21, 'ties': [3, 0x83, 3, 0x83, 1, 0x81, 1, 0x81, 2, 2, 0x82, 0x82, 3, 3, 3, 0x83, 0x83, 0x83, 1, 2, 3],
                'all-hi-desc': [0x80 | k for k in range(20, -1, -1)]}
    cases = [(f'arr-{n}', dict(arr=a)) for n, a in specials.items()] + [(f'rand-{k}', dict(arr=rb())) for k in range(12)]
    cases += [(f'g-off-{o:04x}', dict(g=(None, o), arr=rb())) for o in (0xFFE0, 0xFFE9, 0xFFEB, 0xFFF0, 0xFFFF)]
    cases += [(f'flags-{f:04x}', dict(flags=f, arr=fixed)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(sp=B.SP_MIN, arr=fixed)), ('sp-max', dict(sp=B.SP_MAX, arr=fixed)), ('sp-odd', dict(sp=0xE001, arr=fixed))]
    S0 = 0x8000; BP = S0 - 2
    for k in range(-0x30, -0x14):                                                                   # arrays entirely below the saved BP: over the locals and the unused area, never the return address
        cases.append((f'array-at-bp{k:+d}', dict(g=(None, (BP + k - 0x17) & 0xFFFF), g_seg='ss', sp=S0, arr=fixed)))
    for t in range(0, 21, 4):                                                                       # the array over the global pointer G in the data segment
        cases.append((f'array-over-g-{t:02d}', dict(g=(None, (0x5D92 - t - 0x17) & 0xFFFF), g_seg='ds', arr=fixed)))
    report = []; skipped = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        r = native_case(directory, base, name, kw, bodies, lib)
        if r is None:
            skipped.append(name); continue
        report.append(r); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), skipped_not_returning=skipped, results=report)
