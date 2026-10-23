#!/usr/bin/env python3
"""Mutation check of the 03B6 +0000 and 03CE +00F1 C models: each deliberate change must be caught by the smoke subset of
the differential (ovl03b6_0000_cpu_audit.SMOKE).

Every mutant is a one-substring change inside one generated `case 0xADDR:` line of src/recovered/overlay_u3b6_window_0000_cpu.c
or overlay_u3ce_tile_00f1_cpu.c, built into a temporary library and run against the original-byte closure; a mutant that
survives is reported and fails the run. No mutant is claimed equivalent.
"""
# NOT_A_ROW_CHECKER: drives the 03B6/00F1 audit with mutated copies of its C sources; it registers and checks no row of its own
import ctypes, re, subprocess, tempfile
from pathlib import Path
import ovl03b6_0000_cpu_audit as A

B6 = 'overlay_u3b6_window_0000_cpu'
CE = 'overlay_u3ce_tile_00f1_cpu'
MUTANTS = [
    (CE, 0x00F4, '0x7Fu,0u,8u,1u', '0x7Eu,0u,8u,1u', 'tile test 7Fh -> 7Eh'),
    (CE, 0x0107, '&0x7Fu', '&0x3Fu', 'high-set tile mask 7Fh -> 3Fh'),
    (CE, 0x010D, '0x5E58u', '0x5E54u', 'high set segment read from the low pointer'),
    (CE, 0x0132, 'c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x00u))', 'c->ax=(uint16_t)((c->ax&0xFF00u)|(uint8_t)(0x01u))', 'low-set flag 0 -> 1'),
    (CE, 0x014B, 'c->sp+8u', 'c->sp+6u', 'RETF 8 -> RETF 6'),
    (B6, 0x0040, '0x002Cu', '0x002Bu', 'map stride 2Ch -> 2Bh'),
    (B6, 0x004F, 'incdec(c,c->ax,16u,1u)', 'c->ax', 'map tile not decremented'),
    (B6, 0x0078, '(c->ax&0x8000u)?0xFFFFu:0u', '0u', 'CWD of the column dropped'),
    (B6, 0x00BD, '0x0318u', '0x031Au', 'record 3 window-0 word +318h -> +31Ah'),
    (B6, 0x0162, '0x0C34u', '0x0C36u', 'record 6 tile read from +2'),
    (B6, 0x01AB, '((c->flags&1u)||(c->flags&0x40u))', '(c->flags&1u)', 'records 8/9 JBE -> JB'),
    (B6, 0x01E6, '0x03u,0u,8u,0u', '0x02u,0u,8u,0u', 'screen column step 3 -> 2'),
    (B6, 0x01EA, '0x04u,0u,8u,1u', '0x03u,0u,8u,1u', 'four columns, not five'),
    (B6, 0x0030, 'c->bp+0x0006u', 'c->bp+0x0008u', 'row origin read from [BP+8]'),
    (B6, 0x0203, 'c->sp+4u', 'c->sp+2u', 'RETF 4 -> RETF 2'),
]


def mutate(text, address, old, new):
    lines = text.split('\n')
    hits = [k for k, line in enumerate(lines) if line.lstrip().startswith(f'case 0x{address:04X}:')]
    if len(hits) != 1 or lines[hits[0]].count(old) != 1:
        raise SystemExit(f'mutant text absent at {address:04X}: {old}')
    lines[hits[0]] = lines[hits[0]].replace(old, new)
    return '\n'.join(lines)


def main():
    image, sites = A.Q.load_image()
    image_reloc = A.Q.relocate(image, sites, A.BASE)
    srcs = A.source(image)
    code, fixups = A.unit(0x3CE)
    survivors, caught = [], []
    for target, address, old, new, label in MUTANTS:
        with tempfile.TemporaryDirectory(prefix='mut03b6-') as tmp:
            mutated = Path(tmp) / f'{target}.c'
            mutated.write_text(mutate((A.ROOT / f'src/recovered/{target}.c').read_text(), address, old, new))
            sources = [str(mutated if s == target else A.ROOT / 'src/recovered' / (s + '.c')) for s in A.SOURCES]
            so = Path(tmp) / 'm.so'
            # A mutation that skips a helper call can leave the helper unused; that is the mutant, not a build fault.
            built = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Wno-unused-function',
                                    '-O2', '-fPIC', '-shared', '-I', str(A.ROOT / 'include'), *sources, '-o', str(so)],
                                   capture_output=True, text=True)
            if built.returncode:
                print(f'NOT BUILT (replace this mutant): {label}: {built.stderr[-200:]}')
                survivors.append(label + ' (did not build)')
                continue
            lib = ctypes.CDLL(str(so))
            fce, fb6 = lib.pr_ovl03ce_00f1_cpu, lib.pr_ovl03b6_0000_cpu
            fce.argtypes = [ctypes.POINTER(A.O.Bus), ctypes.c_size_t, ctypes.POINTER(A.Cpu), ctypes.POINTER(A.EnvCE), ctypes.POINTER(A.DiagCE)]
            fb6.argtypes = [ctypes.POINTER(A.O.Bus), ctypes.c_size_t, ctypes.POINTER(A.Cpu), ctypes.POINTER(A.EnvB6), ctypes.POINTER(A.DiagB6)]
            fce.restype = fb6.restype = ctypes.c_int
            try:
                A.differential(image_reloc, code, fixups, srcs, (fce, fb6), only=A.SMOKE)
                survivors.append(label)
                print('SURVIVED', label, flush=True)
            except (ValueError, AssertionError) as exc:
                caught.append(label)
                print('caught', label, '-', re.sub(r'\s+', ' ', str(exc))[:90], flush=True)
    print(f'{len(caught)}/{len(MUTANTS)} mutants caught')
    if survivors:
        raise SystemExit('survivors: ' + '; '.join(survivors))


if __name__ == '__main__':
    main()
