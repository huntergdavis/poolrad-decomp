#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0068_068D: the ORIGINAL bytes of the body AND of unit 0x3BD (+0350, +047B, +031F) run under a real DOS through a
loaded stub (EAh far JMP), compared with the C model on identical state. Completed calls end at a label the oracle program provides as the entry frame's
return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions after the body's code segment (data, stack, unit 0x3BD's code, the
window/buffer segment, the stub segment). An unloaded stub is not run natively (its INT 3Fh would enter the real DOS vector); OVERLAY is pinned by Unicorn
and the C test. IF is cleared in every native case (a real timer interrupt pushes its frame onto the observed stack). The host's packaged dosbox-x is used,
not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_0068_068d_cpu_audit as B
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16


def native_case(directory, base, name, kw, body, code3bd, lib):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, u3bd, win, stub = segments
    kw = dict(kw); same = kw.pop('ds_is_ss', False); woff = kw.pop('window_off', 0x8000); wss = kw.pop('window_on_ss', None)
    opts = dict(w_cs=code, u3bd_cs=u3bd, ds=ss if same else ds, ss=ss, stub_seg=stub, buf_seg=win,
                window=(ss, (kw.get('sp', 0x8000) - wss - 0x5AA) & 0xFFFF) if wss is not None else (win, woff))
    opts.update(kw); opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    state = B.fixture(body, code3bd, **opts); memory, cpu, info = state
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


def native(body, code3bd, lib):
    out = OUT / 'u68-068d'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = [('in-range', dict(x=5, y=3, dx=1, dy=0, cellfill=0))]
    for x, y, dx, dy in ((15, 3, 1, 0), (0, 3, -1, 0), (5, 15, 0, 1), (5, 0, 0, -1), (15, 15, 1, 1), (0, 0, -1, -1), (0x80, 3, 0, 0), (0x7F, 0x7F, 1, 1)):
        cases.append((f'clamp-{x:02x}-{y:02x}-{dx:+d}{dy:+d}', dict(x=x, y=y, dx=dx, dy=dy, cellfill=0xFF)))
    cases += [(f'map-{m:02x}-out', dict(x=0x14, y=3, dx=1, mapid=m, cellfill=0xFF)) for m in (0, 10, 1)]
    cases += [(f'facing-{f:02x}', dict(facing=f, x=15, y=15, dx=1, dy=1, cellfill=0x55)) for f in (0, 1, 7, 8, 0xFF)]
    cases += [(f'flags-{f:04x}', dict(flags=f, x=15, dx=1, cellfill=0xFF)) for f in (0x0002, 0x0A87, 0x08D7, 0x0003)]
    cases += [('flags-08d7-zero', dict(flags=0x08D7, x=0x14, mapid=0))]
    cases += [('sp-min', dict(sp=B.SP_MIN, cellfill=0xFF)), ('sp-max', dict(sp=B.SP_MAX, cellfill=0xFF)), ('sp-odd', dict(sp=0xE001, cellfill=0xFF))]
    cases += [('window-wrap', dict(window_off=0xFA56, x=15, dx=1, cellfill=0xFF)), ('ds-is-ss', dict(ds_is_ss=True, sp=0x9000, x=15, dx=1, cellfill=0xFF))]
    for k, n in ((2, 'bp'), (4, 'x'), (6, 'y'), (8, 'facing')):                 # the raised window word on the frame
        cases.append((f'window-on-{n}', dict(window_on_ss=k, x=15, dx=1, cellfill=0xFF)))
    cases.append(('window-on-x-negative', dict(window_on_ss=4, x=0, dx=-1, cellfill=0xFF)))
    report = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, body, code3bd, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
