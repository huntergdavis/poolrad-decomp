#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0025_1815: the ORIGINAL bytes of unit 25h +1815 and SYSTEM 0B25, 16A8 and 0C10 run under a real DOS, compared
with the C model on identical state.

Six 64 KiB segments: S0 unit 25h's loaded code, S1 the SYSTEM segment of the relocated START.EXE, S2 its data segment, S3 the stack, S4 the strings, S5 a
spare. The nine LCALL segment words of +1815 are the unit's relocations (fixups 182E..18DC) and are written as S1, as the loader would. Each case's strings,
flag bytes, stack residue and registers come from its own name. Compared with no masks: the 14 CPU words and the five segments after the code. IF is
cleared in every native case. The host's packaged DOSBox-X 2026.01.02 is used, the same release as pinned 5a9c7c53."""
# NOT_A_ROW_CHECKER: native helper imported by ovl_0025_1815_cpu_audit.py --native; the audit registers the row
import random, shutil, struct, sys
from hashlib import sha256
from pathlib import Path
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from res0709_printer_0626_cpu_audit import Cpu, physical, put16


def specs():
    out = []
    pairs = [(b'', b''), (b'', b'A'), (b'A', b''), (b'ABC', b'ABC'), (b'ABC', b'ABD'), (b'ABD', b'ABC'), (b'AB', b'ABC'), (b'ABC', b'AB'), (b'a', b'A'),
             (b'\x80', b'\x7f'), (b'\xff' * 255, b'\xff' * 255), (b'\xff' * 255, b'\xff' * 254), (b'x' * 200 + b'a', b'x' * 200 + b'b'), (b'Z', b'AAAA')]
    out += [(f'n-pair{k:02d}', dict(a=a, b=b)) for k, (a, b) in enumerate(pairs)]
    out += [(f'n-flags-{fl:04x}', dict(flags=fl, a=b'AB', b=b'AC')) for fl in (0x0002, 0x08D7, 0x0ED7, 0x0003)]
    out += [(f'n-sp-{sp:04x}', dict(sp=sp, a=b'HELLO', b=b'HELP')) for sp in (0x0220, 0x0221, 0x7FFF, 0xFFF3, 0xFFF4)]
    out += [('n-wrap-a', dict(a=b'WRAPPED', b=b'WRAPPED', a_off=0xFFFE)), ('n-same', dict(a=b'SAME', b=None, b_off=0x0100))]
    out += [(f'n-a-over-flags{k}', dict(a=b'\x05FLAGS', b=b'FLAGS', a_in_ds=0x6F77 + k)) for k in range(3)]
    return out


def world(name, A, segs, base_world, o):
    s0, s1, s2, s3, s4, s5 = segs
    rng = random.Random(f'OVL_0025_1815/native/{name}')
    m = bytearray(rng.randbytes(1 << 20))
    for dst, src in ((s0, A.W), (s1, A.SYS), (s2, A.DS)):
        m[physical(dst, 0):physical(dst, 0) + 65536] = base_world[physical(src, 0):physical(src, 0) + 65536]
    for site, _ in A.CALLS:
        put16(m, s0, site + 3, s1)                                       # the relocated segment word of each LCALL
    at_a = (s2, o['a_in_ds']) if 'a_in_ds' in o else (s4, o.get('a_off', 0x0100)); at_b = (s4, o.get('b_off', 0x0400))
    for (seg, off), s in ((at_a, o.get('a')), (at_b, o.get('b'))):
        if s is None:
            continue
        for j, v in enumerate(bytes([len(s)]) + s):
            m[physical(seg, (off + j) & 0xFFFF)] = v
    sp = o.get('sp', 0xF000)
    for k, w in enumerate((0, 0, at_b[1], at_b[0], at_a[1], at_a[0])):
        put16(m, s3, (sp + 2 * k) & 0xFFFF, w)
    r = tuple(rng.randrange(65536) for _ in range(8))
    c = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, s0, A.ENTRY, s2, r[7], s3, (o.get('flags', 0x0002) & ~0x0300) | 0x0002)
    return m, c


def native(base_world, body, lib):
    A = sys.modules['ovl_0025_1815_cpu_audit']
    out = OUT / 'u25-1815'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    segs = [base + 0x1000 * k for k in range(1, 7)]
    report = []
    for index, (name, o) in enumerate(specs()):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        memory, cpu = world(name, A, segs, base_world, o)
        symbols = assemble(directory, assembly(cpu, segs, None))
        put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
        status, ram, cout, left = A.run_c(lib, (memory, cpu, dict(name=name, budget=A.BUDGET)))
        assert status == 'RETURNED', (name, status)
        for k, segment in enumerate(segs):
            at = physical(segment, 0); (directory / f'I{k}.BIN').write_bytes(bytes(memory[at:at + 65536]))
        (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
        result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536, (name, len(result))
        for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
            assert getattr(cout, n) == value, (name, n, hex(getattr(cout, n)), hex(value))
        for k, segment in enumerate(segs[1:]):
            at = physical(segment, 0); wanted = ram[at:at + 65536]; actual = result[28 + k * 65536:28 + (k + 1) * 65536]
            if wanted != actual:
                d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
                raise AssertionError((name, 'RAM', hex(segment), d[:8], len(d)))
        report.append(dict(case=name, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest()))
        print(f'PASS native {name}', file=sys.stderr, flush=True)
    binary = shutil.which('dosbox-x')
    return dict(profile='DOSBox-X normal/386', dosbox_binary=binary, dosbox_binary_sha256=sha256(Path(binary).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), bytes_compared=len(report) * (5 * 65536 + 28), results=report)
