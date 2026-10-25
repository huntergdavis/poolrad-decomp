#!/usr/bin/env python3
"""Mutation check of the OVL_0038_0400 C model: each deliberate change must be caught by the smoke returns or resident
budget-stop comparisons (ovl038_0400_cpu_audit.SMOKE and PARTY_STOP_BUDGETS).

Harness rules (PM i22688, par-c i23059): the UNMUTATED library first runs the same smoke set and six resident budget stops; the run aborts unless all
pass; every case's world is seeded from its name (the audit's build), never from run order; every mutant is compile-checked
(cc -fsyntax-only) before any run, and a mutant that does not compile is a failure, never 'equivalent'. Each mutant is a
one-substring change inside one generated `case 0xADDR:` line of src/recovered/overlay_u38_handler_0400_cpu.c.
"""
# NOT_A_ROW_CHECKER: drives the 0400 audit with mutated copies of its C source; it registers and checks no row of its own
import ctypes, re, subprocess, tempfile
from pathlib import Path
import ovl038_0400_cpu_audit as A

TARGET = 'overlay_u38_handler_0400_cpu'
MUTANTS = [
    (0x0403, '0x00u,0u,8u,1u', '0x01u,0u,8u,1u', 'selector compared with 1'),
    (0x0410, '&0x40u', '&0x80u', 'bit 6 test -> bit 7'),
    (0x0414, '(uint8_t)(0x06u)', '(uint8_t)(0x05u)', 'first shift count 6 -> 5'),
    (0x0423, '0xB3u,0u,8u,1u', '0xB2u,0u,8u,1u', 'B+84h reset test B3h -> B2h'),
    (0x042E, '(uint8_t)(0x00u)', '(uint8_t)(0x01u)', 'B+84h reset to 1'),
    (0x043E, '&0x20u', '&0x10u', 'bit 5 test -> bit 4'),
    (0x045C, '0x0020u', '0x0010u', 'fold adds 10h'),
    (0x0471, '(uint8_t)(0x07u)', '(uint8_t)(0x06u)', 'second shift count 7 -> 6'),
    (0x0480, '(uint8_t)(0x01u)', '(uint8_t)(0x02u)', 'B+10Fh := 2'),
    (0x048F, '!((c->flags&1u)||(c->flags&0x40u))', '!(c->flags&1u)', 'JA -> JAE'),
    (0x0494, '(uint8_t)(0xB3u)', '(uint8_t)(0xB4u)', 'B+84h set to B4h'),
    (0x04A4, '0x000Au', '0x0008u', 'C word +0Ah -> +08h'),
    (0x04AC, 'int s=call_party_count_23db(b,c,dg);', '(void)dg;(void)call_party_count_23db;int s=0;', '23DB call skipped'),
    (0x04AC, 'int s=call_party_count_23db(b,c,dg);', 'dg->budget=0;int s=call_party_count_23db(b,c,dg);', '23DB party budget forced to zero'),
    (0x04AC, 'rd16(b,c->cs,0x04AFu)', 'rd16(b,c->cs,0x04ADu)', '23DB far-pointer segment from offset word'),
    (0x04B1, '(uint8_t)(0x64u)', '(uint8_t)(0x65u)', 'DS:6825 := 65h'),
    (0x04B7, 'c->sp+10u', 'c->sp+8u', 'RETF 0Ah -> RETF 8'),
]


def mutate(text, address, old, new):
    lines = text.split('\n')
    hits = [k for k, line in enumerate(lines) if line.lstrip().startswith(f'case 0x{address:04X}:')]
    if len(hits) != 1 or lines[hits[0]].count(old) != 1:
        raise SystemExit(f'mutant text absent at {address:04X}: {old}')
    lines[hits[0]] = lines[hits[0]].replace(old, new)
    return '\n'.join(lines)


def build(tmp, text):
    src = Path(tmp) / f'{TARGET}.c'; src.write_text(text)
    sources = [str(src if s == TARGET else A.ROOT / 'src/recovered' / (s + '.c')) for s in A.SOURCES]
    so = Path(tmp) / 'm.so'
    built = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Wno-unused-function', '-O2', '-fPIC', '-shared',
                            '-I', str(A.ROOT / 'include'), *sources, '-o', str(so)], capture_output=True, text=True)
    if built.returncode:
        raise SystemExit(f'build failed: {built.stderr[-300:]}')
    f = ctypes.CDLL(str(so)).pr_u38_handler_0400_cpu
    f.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(A.Cpu), ctypes.c_size_t, ctypes.POINTER(ctypes.c_int)]
    f.restype = ctypes.c_int
    return f


def main():
    original = (A.ROOT / f'src/recovered/{TARGET}.c').read_text()
    texts = [(label, mutate(original, a, o, n)) for a, o, n, label in MUTANTS]
    for label, text in texts:                                   # compile-check every mutant before any run
        with tempfile.TemporaryDirectory(prefix='mut0400-syntax-') as tmp:
            p = Path(tmp) / f'{TARGET}.c'; p.write_text(text)
            r = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Wno-unused-function', '-fsyntax-only', '-I', str(A.ROOT / 'include'), str(p)], capture_output=True, text=True)
            if r.returncode:
                raise SystemExit(f'mutant does not compile (a failure, not an equivalent): {label}: {r.stderr[-200:]}')
    image, sites = A.Q.load_image()
    image_reloc = A.Q.relocate(image, sites, A.BASE)
    code, fixups, _, body_ips = A.source()
    with tempfile.TemporaryDirectory(prefix='mut0400-base-') as tmp:   # baseline: the unmutated model on the same cases
        f = build(tmp, original)
        base = A.differential(image_reloc, code, fixups, body_ips, f, only=A.SMOKE)
        stops = A.party_stop_differential(image_reloc, code, fixups, body_ips, f)
    if base['cases'] != len(A.SMOKE) or base['stops'] != {'RETURN': len(A.SMOKE)}:
        raise SystemExit(f'baseline does not pass on the smoke set: {base}')
    print(f'baseline: {base["cases"]} smoke cases pass unmutated', flush=True)
    print(f'baseline: {stops["cases"]} resident budget stops pass unmutated', flush=True)
    survivors, caught = [], []
    for label, text in texts:
        with tempfile.TemporaryDirectory(prefix='mut0400-') as tmp:
            f = build(tmp, text)
            try:
                A.differential(image_reloc, code, fixups, body_ips, f, only=A.SMOKE)
                A.party_stop_differential(image_reloc, code, fixups, body_ips, f)
                survivors.append(label); print('SURVIVED', label, flush=True)
            except (ValueError, AssertionError) as exc:
                caught.append(label); print('caught', label, '-', re.sub(r'\s+', ' ', str(exc))[:90], flush=True)
    print(f'{len(caught)}/{len(MUTANTS)} mutants caught')
    if survivors:
        raise SystemExit('survivors: ' + '; '.join(survivors))


if __name__ == '__main__':
    main()
