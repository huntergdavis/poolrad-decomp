#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0025_19DB: the ORIGINAL bytes of unit 25h +19DB and unit 3BDh (+057A, +031F, +047B) run under a real DOS,
compared with the C model on identical state.

Six 64 KiB segments: S0 unit 25h's loaded code, S1 the overlay stub (slots 2 and 4 hold the overlay manager's JMP FAR to S2), S2 unit 3BDh's loaded code
(no relocation in the bytes used), S3 the data segment (the relocated START.EXE's), S4 the stack, S5 the map. The two LCALL segment words of +19DB are the
unit's relocations (fixups at 1A4A and 1A5E) and are written as the stub segment S1, as the loader would. Each case's state bytes, map id, map and stack
residue come from its own name. Compared with no masks: the 14 CPU words and the five segments after the code. IF is cleared in every native case. The
host's packaged DOSBox-X 2026.01.02 is used, the same release as pinned 5a9c7c53."""
# NOT_A_ROW_CHECKER: native helper imported by ovl_0025_19db_cpu_audit.py --native; the audit registers the row
import random, shutil, struct, sys
from hashlib import sha256
from pathlib import Path
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from res0709_printer_0626_cpu_audit import Cpu, physical, put16


def specs():
    out = []
    for f in (0, 2, 4, 6, 1, 7):
        for v in (0x00, 0x01, 0x0E, 0x0F, 0x80, 0xFF):
            out.append((f'n-f{f:02x}-v{v:02x}', dict(facing=f, x=v, y=v)))
    for mid in (0x00, 0x0A, 0x01):
        for f in (0, 3):
            out.append((f'n-map{mid:02x}-f{f}', dict(map_id=mid, facing=f, x=0x20, y=0x05)))
    out += [(f'n-flags-{fl:04x}', dict(flags=fl, facing=2)) for fl in (0x0002, 0x08D7, 0x0ED7, 0x0003)]
    out += [(f'n-sp-{sp:04x}', dict(sp=sp, facing=4)) for sp in (0x0030, 0x0031, 0x7FFF, 0xFFFB, 0xFFFC)]
    return out


def world(name, A, segs, base_world, o):
    s0, s1, s2, s3, s4, s5 = segs
    rng = random.Random(f'OVL_0025_19DB/native/{name}')
    m = bytearray(rng.randbytes(1 << 20))
    for dst, src in ((s0, A.W), (s2, A.U.SEG[0x3BD]), (s3, A.DS)):
        m[physical(dst, 0):physical(dst, 0) + 65536] = base_world[physical(src, 0):physical(src, 0) + 65536]
    for site in (0x1A47, 0x1A5B):
        put16(m, s0, site + 3, s1)                                       # the relocated segment word of each LCALL (fixups 1A4A, 1A5E)
    for slot, entry in ((0x0034, 0x057A), (0x002A, 0x047B)):
        m[physical(s1, slot)] = 0xEA; put16(m, s1, slot + 1, entry); put16(m, s1, slot + 3, s2)
    x, y, f = o.get('x', 3), o.get('y', 4), o.get('facing', 0)
    for a, v in zip(A.U.STATE, (x, y, f, rng.randrange(256), rng.randrange(256))):
        m[physical(s3, a)] = v & 0xFF
    m[physical(s3, 0x84DC)] = o.get('map_id', 1); put16(m, s3, 0x6A5C, 0x0000); put16(m, s3, 0x6A5E, s5)
    sp = o.get('sp', 0xF000)
    r = tuple(rng.randrange(65536) for _ in range(8))
    c = Cpu(r[0], r[1], r[2], r[3], r[4], r[5], r[6], sp, s0, A.ENTRY, s3, r[7], s4, (o.get('flags', 0x0002) & ~0x0300) | 0x0002)
    return m, c


def native(base_world, body, lib):
    A = sys.modules['ovl_0025_19db_cpu_audit']
    out = OUT / 'u25-19db'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    segs = [base + 0x1000 * k for k in range(1, 7)]
    report = []
    for index, (name, o) in enumerate(specs()):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        memory, cpu = world(name, A, segs, base_world, o)
        symbols = assemble(directory, assembly(cpu, segs, None))
        put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
        status, ram, writes, ega, cout, left = A.run_c(lib, (memory, cpu, dict(name=name, budget=A.BUDGET)))
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
