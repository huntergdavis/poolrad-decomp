#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_007F_0000 and its resident closure: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.

Completed calls end at a label the oracle program provides as the entry frame's return address. Budget cases stop at a declared first-unexecuted instruction by writing a far-jump capture
trampoline over it; that coordinate is the model's own LIMIT stop, confirmed against the Unicorn oracle, and is reported as an interception, never as original code. Compared with no masks:
the 14 CPU words and the five 64 KiB data regions (data, stack, record and two spares). The host's packaged dosbox-x is used, not the pinned build."""
import argparse, json, shutil, struct, subprocess, sys, tempfile
from hashlib import sha256
from pathlib import Path
import ovl_007f_0000_cpu_audit as A
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from u86_procvars_3413_audit import Cpu, physical, put16, SIZE

MUL_ARRIVALS = ((0x1430, dict(kind=0x11)), (0x144D, dict(kind=0x13)), (0x1475, dict(kind=0x17)))        # raw +10h bytes; 11EA maps 13h..19h to 18h..1Eh


def native_case(directory, base, name, kw, bodies, decoded, lib, checkpoint_ip=None):
    segments = [base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000]
    code, ds, ss, record, spare1, spare2 = segments; resident = code + 0x200
    opts = dict(w_cs=code, resident_cs=resident, ds=ds, ss=ss, rec=(record, 0x0100)); opts.update(kw)
    state = A.fixture(bodies, **opts); memory, cpu, info = state
    limit = 100000; checkpoint = None
    if checkpoint_ip is not None:
        for budget in range(1, 400):
            trial = A.run_c(lib, state, budget)
            if trial[0] == 'LIMIT' and trial[2].cs == resident and trial[2].ip == checkpoint_ip:
                limit = budget; break
        else:
            raise AssertionError(('checkpoint not reached', checkpoint_ip))
        status, _, boundary, _, _ = A.run_c(lib, state, limit); checkpoint = (boundary.cs, boundary.ip)
        raw = A.run_original(state, decoded, limit)
        assert (raw[2].cs, raw[2].ip) == checkpoint, ('original checkpoint location differs', checkpoint, (raw[2].cs, raw[2].ip))
        assert ('res', checkpoint[1]) not in raw[4], 'checkpoint already executed'
    symbols = assemble(directory, assembly(cpu, segments, checkpoint))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    expected = A.run_c(lib, state, limit)
    assert expected[0] == ('LIMIT' if checkpoint else 'RETURNED'), expected[0]
    raw_differences = {}
    if checkpoint:
        raw = A.run_original(state, decoded, limit)
        assert raw[1] == expected[1], 'raw original RAM differs before native corroboration'
        raw_differences = {n: dict(model=getattr(expected[2], n), unicorn=getattr(raw[2], n)) for n, _ in Cpu._fields_ if getattr(expected[2], n) != getattr(raw[2], n)}
    native_memory = bytearray(memory)
    if checkpoint:
        address = physical(*checkpoint); native_memory[address:address + 5] = b'\xea' + symbols['returned'].to_bytes(2, 'little') + base.to_bytes(2, 'little')
    for index, segment in enumerate(segments):
        at = physical(segment, 0); (directory / f'I{index}.BIN').write_bytes(native_memory[at:at + 65536])
    (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
    result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536, len(result)
    for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
        assert getattr(expected[2], n) == value, (name, n, hex(getattr(expected[2], n)), hex(value))
    for index, segment in enumerate(segments[1:]):
        at = physical(segment, 0); wanted = expected[1][at:at + 65536]; actual = result[28 + index * 65536:28 + (index + 1) * 65536]
        if wanted != actual:
            diffs = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
            raise AssertionError((name, 'RAM', hex(segment), diffs[:20]))
    return dict(case=name, checkpoint=list(checkpoint) if checkpoint else None, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest(),
                synthetic_interception=checkpoint is not None, raw_unicorn_cpu_differences=raw_differences)


def native(bodies, lib, quick=False):
    decoded = A.source()[1]
    out = OUT / 'u7f-0000'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    cases = [(f'kind-{k:02x}', dict(kind=k, minor=0x20)) for k in (0x00, 0x01, 0x03, 0x04, 0x06, 0x0C, 0x0E, 0x10, 0x11, 0x15, 0x16, 0x1A, 0x1B, 0x1C, 0x1D, 0x20, 0xFF)]
    cases += [(f'minor-{m:02x}', dict(kind=0x12, minor=m)) for m in (0x00, 0x01, 0x32, 0x33, 0x4B, 0x4C, 0x5A, 0x5B, 0x63, 0x64, 0xFF)]
    cases += [(f'flags-{f:04x}', dict(kind=0x13, flags=f)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(kind=0x12, minor=0x40, sp=A.SP_MIN)), ('sp-max', dict(kind=0x12, minor=0x40, sp=A.SP_MAX)), ('sp-odd', dict(kind=0x16, sp=0xE001))]
    cases += [('record-top', dict(rec=(None, 0xFFF8), kind=0x12, minor=0x70))]
    cases += [(f'record-in-frame-{b:02x}', dict(rec=(None, (0x8000 - 6 - b - 0x10) & 0xFFFF), rec_bytes={})) for b in (0, 2, 4, 8, 0x0C, 0x14, 0x1C)]
    report = []
    for index, (name, kw) in enumerate(cases):
        kw = dict(kw)
        if 'rec' in kw and kw['rec'][0] is None:
            kw['rec'] = (base + 0x3000 if name.startswith('record-in-frame') else base + 0x4000, kw['rec'][1])   # record-in-frame uses the stack segment
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, name, kw, bodies, decoded, lib)); print(f'PASS native {name}', file=sys.stderr, flush=True)
    for checkpoint, kw in MUL_ARRIVALS:
        directory = out / f'mul-{checkpoint:04x}'; directory.mkdir(exist_ok=True)
        report.append(native_case(directory, base, f'mul-arrival-{checkpoint:04x}', dict(kw, minor=0x20), bodies, decoded, lib, checkpoint_ip=checkpoint)); print(f'PASS native MUL {checkpoint:04x}', file=sys.stderr, flush=True)
    stops = {}
    for r in report:
        stops['LIMIT' if r['checkpoint'] else 'RETURNED'] = stops.get('LIMIT' if r['checkpoint'] else 'RETURNED', 0) + 1
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(),
                pinned_build_used=False, cases=len(report), stops=stops, bytes_compared=len(report) * (5 * 65536 + 28), mul_arrivals_native_equal_model=[r['case'] for r in report if r['checkpoint']],
                mul_arrival_unicorn_flag_differences=[dict(case=r['case'], diff=r['raw_unicorn_cpu_differences']) for r in report if r['checkpoint']], results=report)
