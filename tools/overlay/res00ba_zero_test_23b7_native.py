#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for RES_00BA_23B7: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five
64 KiB regions after the code segment (data, stack, the character's segment, two spares). IF is cleared in every native case (a real
timer interrupt pushes its frame onto the observed stack). The host's packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import res00ba_zero_test_23b7_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16


def native_case(directory, base, name, kw, body, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, cseg = segments[:4]
    kw = dict(kw); coff = kw.pop('coff', 0x0100); field_on_ss = kw.pop('field_on_ss', None)
    sp = kw.get('sp', 0x8000)
    opts = dict(w_cs=code, ds=ds, ss=ss, char=(cseg, coff))
    if field_on_ss is not None:
        opts['char'] = (ss, (sp + field_on_ss - B.FIELD) & 0xFFFF)
    opts.update(kw); opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
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
    out = OUT / 'res00ba-23b7'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = [(f'field-{v:02x}', dict(field=v)) for v in (0x00, 0x01, 0x80, 0xFF, 0x03)]
    cases += [('char-wrap', dict(coff=0xFEF2, field=0)), ('char-wrap-ff', dict(coff=0xFFFF, field=7))]
    cases += [(f'flags-{f:04x}', dict(flags=f, field=5)) for f in (0x0002, 0x08D7, 0x0ED7, 0x0003)]
    cases += [(f'flags-{f:04x}-zero', dict(flags=f, field=0)) for f in (0x0002, 0x08D7)]
    cases += [('sp-min', dict(sp=B.SP_MIN, field=0)), ('sp-max', dict(sp=B.SP_MAX, field=0)), ('sp-odd', dict(sp=0xE001, field=5))]
    cases += [(f'field-on-frame{k:+d}', dict(field_on_ss=k)) for k in (-4, -3, -2, -1, 0, 2, 4, 6)]
    report = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, body, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
