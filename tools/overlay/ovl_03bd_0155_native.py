#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_03BD_0155: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions (data, stack, three
spares). Cases where a store reaches the return address (TRANSFER) cannot return to the oracle program and are Unicorn-only. IF is cleared in every native case (a real timer interrupt
pushes its frame onto the observed stack). The host's packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_03bd_0155_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16, SIZE


def native_case(directory, base, name, kw, bodies, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss = segments[0], segments[1], segments[2]
    kw = dict(kw); same = kw.pop('ds_is_ss', False)
    opts = dict(w_cs=code, ds=ss if same else ds, ss=ss); opts.update(kw)
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
    out = OUT / 'u3bd-0155'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = [('program-body-call', dict(args=(0x0B, 0x00, 0x07, 0x06)))]
    for v in (0x00, 0x01, 0x7F, 0x80, 0xFE, 0xFF):
        cases.append((f'v-{v:02x}', dict(args=(v, v ^ 0xFF, (v + 1) & 0xFF, (v + 0x80) & 0xFF))))
    cases += [(f'highs-{h:02x}', dict(highs=(h, h, h, h))) for h in (0x00, 0x80, 0xFF)]
    cases += [(f'flags-{f:04x}', dict(flags=f)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(sp=B.SP_MIN + 0x10)), ('sp-max', dict(sp=B.SP_MAX)), ('sp-odd', dict(sp=0xE001))]
    for k in (-0x10, -4, -3, -2, 6, 7, 8, 9, 10, 11, 12, 13, 0x10):            # DS = SS, the four stores [BP+k, BP+k+3] on the saved BP, the argument slots and the caller's words; k = -1..5 would reach the return address (TRANSFER, Unicorn-only)
        cases.append((f'globals-at-bp{k:+d}', dict(ds_is_ss=True, sp=(B.GLOBALS - k + 2) & 0xFFFF)))
    report = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, bodies, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
