#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for RES_00BA_2E46 (with its callee 2E03): the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five
64 KiB regions after the code segment (data, stack, the character's segment, the link's segment, a spare). IF is cleared in every native case (a real
timer interrupt pushes its frame onto the observed stack). The host's packaged DOSBox-X 2026.01.02 is used, the same release as pinned 5a9c7c53."""
# NOT_A_ROW_CHECKER: native helper imported by res00ba_link_flags_2e46_cpu_audit.py --native; the audit registers the row
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import res00ba_link_flags_2e46_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16


def native_case(directory, base, name, kw, body, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, cseg, lseg = segments[:5]
    kw = dict(kw); same = kw.pop('ds_is_ss', False); coff = kw.pop('coff', 0x0100); loff = kw.pop('loff', 0x0200)
    char_on_ss = kw.pop('char_on_ss', None); link_on_ss = kw.pop('link_on_ss', None)
    sp = kw.get('sp', 0x8000)
    opts = dict(w_cs=code, ds=ss if same else ds, ss=ss, char=(cseg, coff), link=(lseg, loff))
    if char_on_ss is not None:
        opts['char'] = (ss, (sp + char_on_ss - 0xCC) & 0xFFFF)
    if link_on_ss is not None:
        opts['link'] = (ss, (sp + link_on_ss - 0x2E) & 0xFFFF)
    opts.update(kw); opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    opts['les_mirror'] = False                                                  # natively: the wrapped-only state, as DOSBox-X reads it
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
    out = OUT / 'res00ba-2e46'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = [('no-link', dict(nolink=True))]
    cases += [(f'table-{t:02x}', dict(tval=t)) for t in (0x00, 0x01, 0x02, 0x80, 0xFF)]
    cases += [(f'flags-byte-{v:02x}', dict(fval=v)) for v in (0x00, 0x04, 0x10, 0x14, 0xFF)]
    cases += [(f'idx-{i:02x}', dict(idx=i, tval=3)) for i in (0x00, 0x0F, 0x10, 0xFF)]
    cases += [('half-link-off', dict(loff=0x0001, tval=2)), ('char-wrap', dict(coff=0xFF33, tval=2)), ('link-wrap', dict(loff=0xFFD2, tval=2))]
    cases += [(f'flags-{f:04x}', dict(flags=f, tval=2)) for f in (0x0002, 0x08D7, 0x0ED7, 0x0003)]
    cases += [(f'flags-{f:04x}-nolink', dict(flags=f, nolink=True)) for f in (0x0002, 0x08D7)]
    cases += [('sp-min', dict(sp=B.SP_MIN)), ('sp-max', dict(sp=B.SP_MAX)), ('sp-odd', dict(sp=0xE001))]
    # the character's link words on the frame: only +4/+6, where the link stays inside loaded segments. At -2/0 the link is (saved BP, return
    # IP/CS), which names low DOS memory or the oracle program itself: real DOS bytes there, fixture background in the C (Unicorn covers -2/0)
    cases += [(f'char-on-frame{k:+d}', dict(char_on_ss=k)) for k in (4, 6)]
    cases += [(f'linkbyte-on-frame{k:+d}', dict(link_on_ss=k, tval=3)) for k in (-4, -1, 0, 2)]
    report = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, body, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
