#!/usr/bin/env python3
"""DOSBox-X normal/386 corroboration for OVL_007F_0130 and OVL_007F_0185: the ORIGINAL bytes run under a real DOS, compared with the C model on identical state.
Each case's six 64 KiB segments (code, DS, SS, the record segment, two spares) are built by the audit's fixture from the case's name and loaded by the oracle
program; the entry frame returns to a label in the oracle. Compared with no masks: the 14 CPU words (FLAGS exact: the pinned DIV rule is what DOSBox-X
itself does) and the five segments after the code. Only cases that the C model runs to RETURNED are sent (a TRANSFER or a never-ending loop cannot come
back to the oracle). IF is cleared in every native case. The host's packaged DOSBox-X 2026.01.02 is used, the same release as pinned 5a9c7c53."""
# NOT_A_ROW_CHECKER: native helper imported by ovl_007f_div5_leaves_cpu_audit.py --native; the audit registers the rows
import shutil, struct, sys
from hashlib import sha256
from pathlib import Path
from u9e_character_pair_115c_native import assemble, launch, find_layout, assembly, OUT
from res0709_printer_0626_cpu_audit import Cpu, physical, put16


def specs():
    out = []
    for entry in (0x0130, 0x0185):
        t = f'{entry:04x}'
        out += [(f'n-{t}-value-{v:04x}', entry, dict(value=v)) for v in (0, 1, 4, 5, 6, 7, 9, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF, 12345, 0x0F0F)]
        out += [(f'n-{t}-flags-{f:04x}', entry, dict(value=7, flags=f)) for f in (0x0002, 0x08D7, 0x0ED7, 0x0003, 0x0656)]
        out += [(f'n-{t}-sp-{sp:04x}', entry, dict(value=0x0103, sp=sp)) for sp in (0x0004, 0x0005, 0x7FFF, 0xFFF9, 0xFFFA)]
    out += [(f'n-0130-rec-off-{o:04x}', 0x0130, dict(value=0x4321, rec_off=o)) for o in (0x0000, 0xFF70, 0xFF77, 0xFFFF)]
    out += [(f'n-0130-rec-on-frame{k:+03x}', 0x0130, dict(value=0x0E0E, rec_on_frame=k)) for k in range(-0x0E, 0x08)]
    out += [(f'n-0185-block-on-frame{k:+03x}', 0x0185, dict(value=0x0B0B, block_on_frame=k)) for k in range(-0x18, 0x08)]
    return out


def native_case(directory, base, name, entry, kw, blob, lib, A):
    segments = [base + 0x1000 * k for k in range(1, 7)]
    code, ds, ss, rseg = segments[:4]
    kw = dict(kw); opts = dict(w=code, ds=ds, ss=ss, rec=(rseg, kw.pop('rec_off', 0x0100)), ret=(base, 0))
    if 'rec_on_frame' in kw:
        k = kw.pop('rec_on_frame'); opts.update(rec=(ss, (0x8000 + k - 0x88) & 0xFFFF), sp=0x8000)
    if 'block_on_frame' in kw:
        k = kw.pop('block_on_frame'); opts.update(ds=ss, sp=(0x67F4 + 0x14 + k) & 0xFFFF)
    opts.update(kw)
    opts['flags'] = (opts.get('flags', 0x0002) & ~0x0200) | 0x0002
    memory, cpu, info = A.fixture(name, blob, entry, **opts)
    symbols = assemble(directory, assembly(cpu, segments, None))
    put16(memory, cpu.ss, cpu.sp, symbols['returned']); put16(memory, cpu.ss, (cpu.sp + 2) & 0xFFFF, base)
    status, ram, writes, ega, out, left = A.run_c(lib, (memory, cpu, info))
    if status != 'RETURNED':
        return None
    for index, segment in enumerate(segments):
        at = physical(segment, 0); (directory / f'I{index}.BIN').write_bytes(bytes(memory[at:at + 65536]))
    (directory / 'RESULT.BIN').unlink(missing_ok=True); launch(directory)
    result = (directory / 'RESULT.BIN').read_bytes(); assert len(result) == 28 + 5 * 65536, (name, len(result))
    for (n, _), value in zip(Cpu._fields_, struct.unpack_from('<14H', result)):
        assert getattr(out, n) == value, (name, n, hex(getattr(out, n)), hex(value))
    for index, segment in enumerate(segments[1:]):
        at = physical(segment, 0); wanted = ram[at:at + 65536]; actual = result[28 + index * 65536:28 + (index + 1) * 65536]
        if wanted != actual:
            d = [(hex(j), a, b) for j, (a, b) in enumerate(zip(wanted, actual)) if a != b]
            raise AssertionError((name, 'RAM', hex(segment), d[:8], len(d)))
    return dict(case=name, cpu_words=14, data_bytes=5 * 65536, masked_flags=0, result_sha256=sha256(result).hexdigest())


def native(blob, decoded, lib):
    A = sys.modules['ovl_007f_div5_leaves_cpu_audit']
    out = OUT / 'u7f-div5-leaves'; out.mkdir(parents=True, exist_ok=True); base = find_layout(out)
    report, skipped = [], []
    for index, (name, entry, kw) in enumerate(specs()):
        directory = out / f'case-{index:02d}'; directory.mkdir(exist_ok=True)
        r = native_case(directory, base, name, entry, kw, blob, lib, A)
        if r is None:
            skipped.append(name); continue
        report.append(r); print(f'PASS native {name}', file=sys.stderr, flush=True)
    binary = shutil.which('dosbox-x')
    return dict(profile='DOSBox-X normal/386', dosbox_binary=binary, dosbox_binary_sha256=sha256(Path(binary).read_bytes()).hexdigest(), pinned_build_used=False,
                cases=len(report), not_returned_in_c=skipped, bytes_compared=len(report) * (5 * 65536 + 28), results=report)
