#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_007F_044C's TRANSFER branch (the one without the message): +044C, +0000, the 13AB/11EA closure, +0020 and +003A as ORIGINAL bytes under a real DOS,
compared with the C model on identical state. The message branch (StrLoad, resident 1693, the EGA) is NOT run natively. Completed calls end at a label the oracle program provides as the entry frame's
return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions (data, stack, destination record, source record, pointer cells). IF is cleared in every native case. The host's
packaged dosbox-x is used, not the pinned build."""
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_007f_world1693 as WD
import ovl_007f_044c_cpu_audit as C
import ovl_007f_0054_cpu_audit as B54
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from res0709_printer_0626_cpu_audit import Cpu, Bus, WRITE_FN, physical, put16, SIZE
import ctypes


def light_fixture(bodies, *, base, index, extra, kind=0x0C, minor=0x20, pw=0x0100, parr=0x0200, qarr=0x0300, qw=0x0400, sp=0x8000, flags=0x0002):
    code, ds, ss, pseg, qseg, cseg = base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000, base + 0x6000
    res = code + 0x200; p, q = (pseg, 0x0100), (qseg, 0x0100); pcell, qcell = (cseg, 0x0000), (cseg, 0x0010)
    m = bytearray([0xA5]) * SIZE
    for rec in (p, q):
        for k in range(0x110):
            m[physical(rec[0], (rec[1] + k) & 0xFFFF)] = (k * 5 + 3) & 0xFF
    elem = 0x88 + 2 * C.sext8(index)
    put = lambda rec, off, v: (m.__setitem__(physical(rec[0], (rec[1] + off) & 0xFFFF), v & 0xFF), m.__setitem__(physical(rec[0], (rec[1] + off + 1) & 0xFFFF), (v >> 8) & 0xFF))
    m[physical(p[0], p[1] + 0x10)] = kind; m[physical(p[0], p[1] + 0x16)] = minor
    put(p, 0x102, pw); put(p, elem, parr); put(q, elem, qarr); put(q, 0x102, qw)
    for (kd, start), blob in bodies.items():
        if kd == 'w' and (start < 0x54 or start == 0x44C):
            at = physical(code, start); m[at:at + len(blob)] = blob
        elif kd == 'res13':
            at = physical(res, start); m[at:at + len(blob)] = blob
    put16(m, code, 0x0F, res)
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, code, 0x044C, ds, 0x1111, ss, (flags & ~0x0200) | 0x0002)
    for k, v in enumerate((0x9000, 0x8000, 0xAB00 | (index & 0xFF), extra & 0xFFFF, pcell[1], pcell[0], qcell[1], qcell[0])):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    for cell, rec in ((pcell, p), (qcell, q)):
        put16(m, cell[0], cell[1], rec[1]); put16(m, cell[0], cell[1] + 2, rec[0])
    return m, cpu, (code, ds, ss, pseg, qseg, cseg)


def run_c_light(lib, m, cpu, budget=100000):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget)
    bus = Bus(ram, None, None, WRITE_FN(lambda *a: None), None)
    lim = WD.Limits(65536, 65536, 65536, 65536, 1000000)
    status, _ = C.call(lib.pr_u7f_044c_cpu, ctypes.byref(bus), SIZE, ctypes.byref(out), ctypes.byref(left), lim)
    return status, bytes(ram), out, left.value


def native(bodies, decoded, lib, quick=False):
    out = OUT / 'u7f-044c'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    b54 = B54.source()[:2]; v = {}
    V = lambda k, mi=0x20: v.setdefault((k, mi), B54.original_value(b54[0], b54[1], k, mi, 0x6000, 0x7000))
    cases = []
    for k in (0x0C, 0x12, 0x13):
        for pw in (0x0000, 0x0100, 0xFFFF):
            for delta, tag in ((-1, 'm1'), (0, 'eq')):
                cases.append((f'b-{k:02x}-{pw:04x}-{tag}', dict(kind=k, pw=pw, index=2, extra=lambda vv, pw=pw, d=delta: ((vv + d) - pw) & 0xFFFF)))
    cases += [(f'index-{i:02x}', dict(index=i, pw=0, extra=lambda vv: 5)) for i in (0, 1, 5, 0x7F, 0x80, 0xFF)]
    cases += [(f'arr-{a:04x}-{b:04x}', dict(parr=a, qarr=b, pw=0, index=2, extra=lambda vv: 8)) for a, b in ((0xFFFF, 1), (0x8000, 0x7FFF), (0xFFFE, 0xFFFF), (5, 3))]
    cases += [(f'qw-{w:04x}', dict(qw=w, pw=0, index=2, extra=lambda vv: 5)) for w in (0, 5, 0xFFFF)]
    cases += [(f'flags-{f:04x}', dict(flags=f, pw=0, index=2, extra=lambda vv: 5)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(sp=C.SP_MIN, pw=0, index=2, extra=lambda vv: 5)), ('sp-max', dict(sp=C.SP_MAX, pw=0, index=2, extra=lambda vv: 5)), ('sp-odd', dict(sp=0xE001, pw=0, index=2, extra=lambda vv: 5))]
    report = []; skipped = []
    for n, (name, kw) in enumerate(cases):
        kw = dict(kw); kind = kw.get('kind', 0x0C); extra = kw.pop('extra')(V(kind)); directory = out / f'case-{n:02d}'; directory.mkdir(exist_ok=True)
        m, cpu, segs = light_fixture(bodies, base=base, extra=extra, **kw)
        symbols = assemble(directory, assembly(cpu, list(segs), None))
        put16(m, cpu.ss, cpu.sp, symbols['returned']); put16(m, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
        status, ram, expected, left = run_c_light(lib, m, cpu)
        if status != 'RETURNED':
            skipped.append((name, status)); continue
        for i, segment in enumerate(segs):
            at = physical(segment, 0); (directory / f'I{i}.BIN').write_bytes(bytes(m[at:at + 65536]))
        (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
        result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536
        for (nm, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
            assert getattr(expected, nm) == value, (name, nm, hex(getattr(expected, nm)), hex(value))
        for i, segment in enumerate(segs[1:]):
            at = physical(segment, 0); wanted = ram[at:at + 65536]; actual = result[28 + i * 65536:28 + (i + 1) * 65536]
            if wanted != actual:
                d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
                raise AssertionError((name, 'RAM', hex(segment), d[:8], len(d)))
        report.append(dict(case=name, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest())); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                branch='transfer only', cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), skipped=skipped, results=report)
