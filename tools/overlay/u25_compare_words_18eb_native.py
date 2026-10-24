#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0025_18EB: the ORIGINAL body and the original SYSTEM FillChar it far-calls run under a real DOS,
compared with the C model on identical state. Completed calls end at a label the oracle program provides as the entry frame's return
address. Compared with no masks: the 14 CPU words and the five 64 KiB regions after the code segment (data, stack, FillChar's segment, two
spares). IF is cleared in every native case (a real timer interrupt pushes its frame onto the observed stack). Only cases the C model
completes are run natively (a transfer would never reach the label). The host's packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import u25_compare_words_18eb_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16


def native_case(directory, base, name, kw, src, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, sysseg = segments[:4]
    kw = dict(kw); alias = kw.pop('alias', False)
    opts = dict(w_cs=code, sys=sysseg, ds=ss if alias else ds, ss=ss)
    opts.update(kw); opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    state = B.fixture(src, **opts); memory, cpu, info = state
    symbols = assemble(directory, assembly(cpu, segments, None))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    expected = B.run_c(lib, state)
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


def native(src, lib):
    out = OUT / 'ovl0025-18eb'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    prior = (0xFF, 0x00, 0x5A, 0x01, 0xA5, 0x7F)
    cases = [(f'pair-{a:04x}-{b:04x}', dict(a=a, b=b, prior=prior)) for a, b in B.PAIRS]
    cases += [(f'flags-{f:04x}', dict(flags=f, a=0x4000, b=0x3FFF)) for f in (0x0002, 0x0602, 0x08D7, 0x0ED7, 0x0003)]
    cases += [('sp-min', dict(sp=B.SP_MIN, a=3, b=9)), ('sp-max', dict(sp=B.SP_MAX, a=9, b=3)), ('sp-odd', dict(sp=0xE001, a=5, b=5))]
    cases += [(f'alias-{s:04x}', dict(alias=True, sp=s, a=0x0101, b=0x0001, prior=(9,) * 6)) for s in range(0x6F68, 0x6F8C)]
    report = []; skipped = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        r = native_case(directory, base, name, kw, src, lib)
        if r is None:
            skipped.append(name); continue
        report.append(r); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), not_completed_by_c=skipped, bytes_compared=len(report) * (5 * 65536 + 28), results=report)
