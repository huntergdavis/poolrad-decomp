#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for RES_00BA_23DB: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five
64 KiB regions after the code segment (data, stack, two character segments, a spare). IF is cleared in every native case. The host's packaged
DOSBox-X 2026.01.02 is used, the same release as pinned 5a9c7c53."""
# NOT_A_ROW_CHECKER: native helper imported by res00ba_party_count_23db_cpu_audit.py --native; the audit registers the row
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import res00ba_party_count_23db_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16


def native_case(directory, base, name, kw, body, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, c1, c2 = segments[:5]
    kw = dict(kw); layout = kw.pop('layout')
    chars = tuple((c1 if seg == 1 else c2, off, flag, idx) for seg, off, flag, idx in layout)
    opts = dict(w_cs=code, ds=ds, ss=ss, chars=chars, budget=100000); opts.update(kw)
    opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    state = B.fixture(body, **opts); memory, cpu, info = state
    symbols = assemble(directory, assembly(cpu, segments, None))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    expected = B.run_c(lib, state)
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


def native(body, lib):
    out = OUT / 'res00ba-23db'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    three = ((1, 0x0100, 1, 0), (1, 0x0400, 0, 1), (2, 0x0100, 5, 1))
    cases = [('empty', dict(layout=())), ('one', dict(layout=((1, 0x0100, 1, 0),))), ('three-mixed', dict(layout=three))]
    cases += [(f'idx-{i:02x}', dict(layout=((1, 0x0100, 1, i), (2, 0x0200, 1, i)))) for i in (0x00, 0x01, 0x7F, 0x80, 0xFF)]
    cases += [('count-to-80', dict(layout=tuple((1, 0x0200 * k, 1, 1) for k in range(1, 0x7F)) + tuple((2, 0x0200 * k, 1, 1) for k in range(1, 3)), counts=(0, 0)))]
    cases += [('char-wrap', dict(layout=((1, 0xFEF2, 1, 0), (2, 0x0100, 1, 1))))]
    cases += [(f'flags-{f:04x}', dict(layout=three, flags=f)) for f in (0x0002, 0x08D7, 0x0ED7, 0x0003)]
    cases += [('sp-min', dict(layout=three, sp=B.SP_MIN)), ('sp-max', dict(layout=three, sp=B.SP_MAX)), ('sp-odd', dict(layout=three, sp=0xE001))]
    report = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, body, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
