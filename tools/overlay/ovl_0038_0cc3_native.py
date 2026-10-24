#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_0038_0CC3's NOT-FOUND walk: +0CC3 as ORIGINAL bytes under a real DOS, compared with the C
model on identical state. Every found path shows a message through resident 1693 (EGA, delay, clear) and then calls FreeMem; that path
is NOT run natively here (the 1693 native machine is the same declared gap as for +0203 and +0A88). Completed calls end at a label the
oracle program provides as the entry frame's return address. Compared with no masks: the 14 CPU words and the five 64 KiB regions
(code, data, stack, nodes, record). IF is cleared in every native case. The host's packaged dosbox-x is used, not the pinned build."""
# NOT_A_ROW_CHECKER: native helper imported by ovl_0038_0cc3_cpu_audit.py --native; the audit registers the row
import ctypes, shutil, struct, sys
from hashlib import sha256
from pathlib import Path
import ovl_0038_0cc3_cpu_audit as A
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from res0709_printer_0626_cpu_audit import Cpu, Bus, WRITE_FN, physical, put16, SIZE


def light_fixture(bodies, *, base, nodes, key=0x00011234, kind=3, rec3_low=5, sp=0x8000, flags=0x0002):
    """The 0CC3 span only: code, DS, SS, a node segment and the record, each its own segment above the oracle program."""
    code, ds, ss, heap, rec = base + 0x1000, base + 0x2000, base + 0x3000, base + 0x4000, base + 0x5000
    m = bytearray([0xA5]) * SIZE
    blob = bodies['w', A.ENTRY]; at = physical(code, A.ENTRY); m[at:at + len(blob)] = blob
    m[physical(rec, 0x0103)] = ((kind & 0xF) << 4) | (rec3_low & 0xF)
    places = [(heap, 0x0100 + 0x20 * k) for k in range(len(nodes))]
    for k, n in enumerate(nodes):
        seg, off = places[k]; nxt = places[k + 1] if k + 1 < len(nodes) else (0, 0)
        put16(m, seg, off, n[0] & 0xFFFF); put16(m, seg, off + 2, n[0] >> 16); put16(m, seg, off + 4, nxt[1]); put16(m, seg, off + 6, nxt[0])
        m[physical(seg, off + 0x12)] = n[1]
    head = places[0] if nodes else (0, 0)
    put16(m, ds, 0x6DBB, head[1]); put16(m, ds, 0x6DBD, head[0])
    cpu = Cpu(0xABCD, 0x3456, 0x5678, 0x789A, 0x9ABC, 0xBCDE, 0x7777, sp, code, A.ENTRY, ds, 0x1111, ss, (flags & ~0x0200) | 0x0002)
    for k, v in enumerate((0x9000, 0x8000, 0x00AB, 0x0100, rec, key & 0xFFFF, key >> 16)):
        put16(m, ss, (sp + 2 * k) & 0xFFFF, v)
    return m, cpu, (code, ds, ss, heap, rec)


def run_c_light(lib, m, cpu, budget=100000):
    ram = (ctypes.c_uint8 * SIZE).from_buffer_copy(m); out = Cpu.from_buffer_copy(cpu); left = ctypes.c_size_t(budget)
    bus = Bus(ram, None, None, WRITE_FN(lambda *a: None), None)
    status, _ = A.call(lib.pr_u38_0cc3_cpu, ctypes.byref(bus), SIZE, ctypes.byref(out), ctypes.byref(left), None)
    return status, bytes(ram), out, left.value


def native(bodies, decoded, lib, quick=False):
    out = OUT / 'u38-0cc3'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    K = 0x00011234
    cases = [('empty', dict(nodes=())), ('one-other-key', dict(nodes=((K + 1, 3),))), ('key-wrong-type', dict(nodes=((K, 4),))),
             ('high-word-differs', dict(nodes=((K ^ 0x10000, 3),))), ('five-no-match', dict(nodes=tuple((K + j, 3) for j in range(1, 6)))),
             ('type-nibble-0', dict(nodes=((K, 3),), kind=0)), ('type-nibble-f', dict(nodes=((K, 0xE),), kind=0xF)),
             ('rec-low-nibble', dict(nodes=((K, 4),), rec3_low=0xF))]
    cases += [(f'flags-{f:04x}', dict(nodes=((K + 1, 3), (K, 2)), flags=f)) for f in (0x0002, 0x0A87, 0x0E57, 0x08D7, 0x0003)]
    cases += [('sp-min', dict(nodes=((K + 1, 3),), sp=A.SP_MIN)), ('sp-max', dict(nodes=((K + 1, 3),), sp=A.SP_MAX)), ('sp-odd', dict(nodes=((K + 1, 3),), sp=0xE001))]
    report = []; skipped = []
    for index, (name, kw) in enumerate(cases):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        m, cpu, segs = light_fixture(bodies, base=base, **kw)
        symbols = assemble(directory, assembly(cpu, list(segs), None))
        put16(m, cpu.ss, cpu.sp, symbols['returned']); put16(m, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
        status, ram, expected, left = run_c_light(lib, m, cpu)
        if status != 'RETURNED':
            skipped.append((name, status)); continue
        for i, segment in enumerate(segs):
            at = physical(segment, 0); (directory / f'I{i}.BIN').write_bytes(bytes(m[at:at + 65536]))
        (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
        result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 4 * 65536
        for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
            assert getattr(expected, n) == value, (name, n, hex(getattr(expected, n)), hex(value))
        for i, segment in enumerate(segs[1:]):
            at = physical(segment, 0); wanted = ram[at:at + 65536]; actual = result[28 + i * 65536:28 + (i + 1) * 65536]
            if wanted != actual:
                d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
                raise AssertionError((name, 'RAM', hex(segment), d[:8], len(d)))
        report.append(dict(case=name, cpu_words=14, data_bytes=4 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest()))
        print(f'PASS native {name}', file=sys.stderr, flush=True)
    return dict(profile='DOSBox-X normal/386', dosbox_binary=shutil.which('dosbox-x'), dosbox_binary_sha256=sha256(Path(shutil.which('dosbox-x')).read_bytes()).hexdigest(),
                pinned_build_used=False, branch='not-found walk only', cases=len(report), bytes_compared=len(report) * (4 * 65536 + 28), skipped=skipped, results=report)
