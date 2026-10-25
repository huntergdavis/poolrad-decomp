#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0025_06D3 and OVL_0025_06FB: the ORIGINAL bytes run under a real DOS, compared with the C entries
on identical state. Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks:
the 14 CPU words and the five 64 KiB regions after the code segment (data, stack, three spares). IF is cleared in every native case (a real
timer interrupt pushes its frame onto the observed stack). The host's packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import u25_06d3_06fb_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16


def native_case(directory, base, name, entry, kw, bodies, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss = segments[:3]
    opts = dict(w_cs=code, ds=ds, ss=ss); opts.update(kw); opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    state = B.fixture(bodies, entry, **opts); memory, cpu, info = state
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


def native(bodies, lib):
    out = OUT / 'ovl0025-06d3-06fb'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = [(f'06d3-code-{c:02x}', 0x06D3, dict(arg=0x5A00 | c)) for c in (0x00, 0x01, 0x1E, 0x1F, 0x20, 0x21, 0x3F, 0x7F, 0x80, 0xC0, 0xFF)]
    cases += [(f'06fb-addr-{a:04x}', 0x06FB, dict(arg=a)) for a in (0x0000, 0x48FF, 0x4900, 0x4CFF, 0x4D00, 0x6AFF, 0x6B00, 0x6EFF, 0x6F00,
                                                                     0x96FF, 0x9700, 0x98FF, 0x9900, 0xB6FF, 0xB700, 0xFFFF)]
    for entry, arg in ((0x06D3, 0x001F), (0x06D3, 0x0020), (0x06FB, 0x4A00), (0x06FB, 0xC000)):
        cases += [(f'{entry:04x}-flags-{f:04x}-{arg:04x}', entry, dict(arg=arg, flags=f)) for f in (0x0002, 0x08D7, 0x0ED7, 0x0003)]
        cases += [(f'{entry:04x}-sp-{sp:04x}-{arg:04x}', entry, dict(arg=arg, sp=sp)) for sp in (B.SP_MIN, B.SP_MAX, 0xE001)]
    report = []
    for index, (name, entry, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, entry, kw, bodies, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
