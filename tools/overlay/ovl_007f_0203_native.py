#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_007F_0203's NOT-ABOVE branch: +0203, +0054, +0000, the 13AB/11EA closure and +003A as ORIGINAL bytes under a real DOS, compared with the C model on
identical state. The above branch shows a message through resident 1693 (EGA, delay, clear) and is NOT run natively here: that needs the 1693 native harness's EGA machine and is declared not
established. Completed calls end at a label the oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions (data, stack,
record, two spares). IF is cleared in every native case. The host's packaged dosbox-x is used, not the pinned build."""
import ctypes, random, shutil, struct, sys, tempfile
from hashlib import sha256
from pathlib import Path
import ovl_007f_world1693 as WD
import ovl_007f_0203_cpu_audit as C
import ovl_007f_0054_cpu_audit as B54
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from res0709_printer_0626_cpu_audit import Cpu, Bus, Ega, Env, OUT_FN, IN_FN, WRITE_FN, physical, put16, SIZE


def light_fixture(bodies, *, base, amount, kind=0x0C, minor=0x20, w=0x0100, acc=0x1000, g=None, sp=0x8000, flags=0x0002, ctr=(0x10, 0)):
    """The unit-0x7F spans and the 13AB closure only (no 1693 world): code at segment code, the resident closure 200h paragraphs above, DS/SS/record in their own segments."""
    code, ds, ss, rec = base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000
    res = code + 0x200; g = g or (rec, 0x0100)
    m = bytearray([0xA5]) * SIZE
    for k in range(0x110):
        m[physical(g[0], (g[1] + k) & 0xFFFF)] = (k * 5 + 1) & 0xFF
    for off, v in {0x10: kind, 0x16: minor, 0x90: acc & 0xFF, 0x91: acc >> 8, 0x102: w & 0xFF, 0x103: w >> 8}.items():
        m[physical(g[0], (g[1] + off) & 0xFFFF)] = v & 0xFF
    for (kd, start), blob in bodies.items():
        if kd == 'w' and (start < 0x20 or 0x3A <= start < 0xA8 or start == 0x203):
            at = physical(code, start); m[at:at + len(blob)] = blob
        elif kd == 'res13':
            at = physical(res, start); m[at:at + len(blob)] = blob
    put16(m, code, 0x0F, res)
    put16(m, ds, 0x5D92, g[1]); put16(m, ds, 0x5D94, g[0]); put16(m, ds, 0x6804, ctr[0]); put16(m, ds, 0x6806, ctr[1])
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, code, 0x0203, ds, 0x1111, ss, (flags & ~0x0200) | 0x0002)
    for k, v in enumerate((0x9000, 0x8000, amount & 0xFFFF)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    return m, cpu, (code, ds, ss, rec, base + 0x5000, base + 0x6000)


def run_c_light(lib, m, cpu, budget=100000):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget)
    bus = Bus(ram, None, None, WRITE_FN(lambda *a: None), None)
    lim = WD.Limits(65536, 65536, 65536, 65536, 1000000)
    status, _ = C.call(lib.pr_u7f_0203_cpu, ctypes.byref(bus), SIZE, ctypes.byref(out), ctypes.byref(left), lim)
    return status, bytes(ram), out, left.value


def native(bodies, decoded, lib, quick=False):
    out = OUT / 'u7f-0203'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    b54 = B54.source()[:2]; v = {}
    V = lambda k, mi=0x20: v.setdefault((k, mi), B54.original_value(b54[0], b54[1], k, mi, 0x6000, 0x7000))
    cases = []
    for k in (0x0C, 0x10, 0x12, 0x13, 0x1D):
        for w in (0x0000, 0x0100, 0xFFFF):
            for delta, tag in ((-1, 'm1'), (0, 'eq')):
                cases.append((f'b-{k:02x}-{w:04x}-{tag}', dict(kind=k, w=w, amount=lambda vv, w=w, d=delta: ((vv + d) - w) & 0xFFFF)))
    cases += [(f'acc-{a:04x}', dict(acc=a, amount=lambda vv: 0x0001, w=0)) for a in (0x0000, 0x7FFF, 0xFFFF)]
    cases += [(f'flags-{f:04x}', dict(flags=f, amount=lambda vv: 1, w=0)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(sp=C.SP_MIN, amount=lambda vv: 1, w=0)), ('sp-max', dict(sp=C.SP_MAX, amount=lambda vv: 1, w=0)), ('sp-odd', dict(sp=0xE001, amount=lambda vv: 1, w=0))]
    report = []; skipped = []
    for index, (name, kw) in enumerate(cases):
        kw = dict(kw); kind = kw.get('kind', 0x0C); amount = kw.pop('amount')(V(kind)); directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        m, cpu, segs = light_fixture(bodies, base=base, amount=amount, **kw)
        symbols = assemble(directory, assembly(cpu, list(segs), None))
        put16(m, cpu.ss, cpu.sp, symbols['returned']); put16(m, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
        status, ram, expected, left = run_c_light(lib, m, cpu)
        if status != 'RETURNED':
            skipped.append((name, status)); continue
        for i, segment in enumerate(segs):
            at = physical(segment, 0); (directory / f'I{i}.BIN').write_bytes(bytes(m[at:at + 65536]))
        (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
        result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536
        for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
            assert getattr(expected, n) == value, (name, n, hex(getattr(expected, n)), hex(value))
        for i, segment in enumerate(segs[1:]):
            at = physical(segment, 0); wanted = ram[at:at + 65536]; actual = result[28 + i * 65536:28 + (i + 1) * 65536]
            if wanted != actual:
                d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
                raise AssertionError((name, 'RAM', hex(segment), d[:8], len(d)))
        report.append(dict(case=name, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest())); print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(), pinned_build_used=False,
                branch='not-above only', cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), skipped=skipped, results=report)
