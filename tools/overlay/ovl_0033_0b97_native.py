#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0033_0B97: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions (data, stack, grid, two
spares). Cases where a store or the increment reaches the return address (TRANSFER) cannot return to the oracle program and are Unicorn-only. IF is cleared in every native case (a real timer
interrupt pushes its frame onto the observed stack). The host's packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_0033_0b97_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16, SIZE


def native_case(directory, base, name, kw, bodies, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, grid, spare1, spare2 = segments
    kw = dict(kw); where = {'ss': ss, 'grid': grid}; g_seg = kw.pop('g_seg', 'grid')
    opts = dict(w_cs=code, ds=ds, ss=ss, g=(grid, 0x0100)); opts.update(kw)
    if opts['g'][0] is None:
        opts['g'] = (where[g_seg], opts['g'][1])
    opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    state = B.fixture(bodies, **opts); memory, cpu, info = state
    symbols = assemble(directory, assembly(cpu, segments, None))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    expected = B.run_c(lib, state, 100000)
    assert expected[0] == 'RETURNED', (name, expected[0])
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
    out = OUT / 'u33-0b97'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = []
    for x in (0x00, 0x01, 0x7F, 0x80, 0xFF):
        for y in (0x00, 0x01, 0x7F, 0x80, 0xFF):
            cases.append((f'xy-{x:02x}-{y:02x}', dict(x=x, y=y)))
    cases += [(f'g-off-{o:04x}', dict(g=(None, o), x=0x0A, y=0)) for o in (0x0000, 0xFFF0, 0xFFFA, 0xFFFB, 0xFFFF)]
    cases += [(f'counter-{c:02x}', dict(counter=c)) for c in (0x00, 0x7F, 0x80, 0xFF, 0x0F)]
    cases += [(f'flags-{f:04x}', dict(flags=f, x=0x90, y=0xF0)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(sp=B.SP_MIN, p=0x0110)), ('sp-max', dict(sp=B.SP_MAX, p=0x0100)), ('sp-odd', dict(sp=0xE001, p=0xE100))]
    S0 = 0x8000; BP = S0 - 2
    for k in (-0x10, -3, -1, 0, 1, 6, 7, 8, 9, 10, 11, 0x10):                                  # the counter on the saved BP, the pointer words and the arguments (never the return address)
        cases.append((f'near-ptr-at-bp{k:+d}', dict(p=(BP + k + 2) & 0xFFFF, sp=S0)))
    for k in (-0x10, -4, -3, -2, -1, 0, 6, 7, 8, 9, 10, 11):                                    # the first store on the same bytes
        cases.append((f'grid-at-bp{k:+d}', dict(g=(None, (BP + k - 7) & 0xFFFF), g_seg='ss', x=0, y=0, sp=S0)))
    for k in (8, 10):
        cases.append((f'grid-args-bp{k:+d}-3-2', dict(g=(None, (BP + k - 7 - (2 * 50 + 3)) & 0xFFFF), g_seg='ss', x=3, y=2, sp=S0)))
    report = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, bodies, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
