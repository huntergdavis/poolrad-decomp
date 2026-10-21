#!/usr/bin/env python3
"""Mutation check of the 0239 C composition: each deliberate change must be caught by the smoke subset of the differential.

Every mutant is a one-line text change to src/recovered/overlay_u23_append_0239_cpu.c, built into a temporary library and run
against the original-byte closure on the fifteen smoke cases; a mutant that survives is reported and fails the run. Mutants
that cannot differ on any input would be named as equivalent; none is claimed here.
"""
# NOT_A_ROW_CHECKER: drives the 0239 audit with mutated copies of its C source; it registers and checks no row of its own
import subprocess, sys, tempfile, shutil
from pathlib import Path
import u23_append_0239_cpu_audit as A

SRC = A.ROOT / 'src/recovered/overlay_u23_append_0239_cpu.c'
MUTANTS = [
    ('RESULT := 0 on the over-capacity path', 'pr_bus_write8(bus,physical(c->es,c->di),1u);', 'pr_bus_write8(bus,physical(c->es,c->di),0u);'),
    ('Move count 3Fh -> 3Eh (head)', 'c->ax=0x003Fu;push(bus,c,c->ax);                                    /* 02B0..02B3 */', 'c->ax=0x003Eu;push(bus,c,c->ax);                                    /* 02B0..02B3 */'),
    ('walk ignores the segment link word', 'c->ax=logic(c,(uint16_t)(c->ax|word(bus,c->es,(uint16_t)(c->di+0x2Cu))),16u);\n        if(c->flags&0x0040u)break;', 'c->ax=logic(c,(uint16_t)(c->ax),16u);\n        if(c->flags&0x0040u)break;'),
    ('new tail link stores the offset word twice', 'put_word(bus,c->es,(uint16_t)(c->di+0x2Cu),c->dx); /* 0334..033B */', 'put_word(bus,c->es,(uint16_t)(c->di+0x2Cu),c->ax); /* 0334..033B */'),
    ('RETF 8 -> RETF 6', 'c->sp=(uint16_t)(c->sp+8u);', 'c->sp=(uint16_t)(c->sp+6u);'),
    ('empty list branch inverted', 'if(c->flags&0x0040u) {                                                  /* 0295 JNE 02DC not taken: empty list */', 'if(!(c->flags&0x0040u)) {                                               /* 0295 JNE 02DC not taken: empty list */'),
    ('over test inverted', 'if(!(c->flags&0x0040u)) {                                               /* 0254 JE 0274 not taken */', 'if(c->flags&0x0040u) {                                                  /* 0254 JE 0274 not taken */'),
    ('0BB8 skipped after a head append', 'goto redraw;                                                        /* 02DA */', 'goto done;                                                          /* 02DA */'),
    ('literal offset 022E -> 022F', 'c->di=0x022Eu;push(bus,c,c->cs);push(bus,c,c->di);', 'c->di=0x022Fu;push(bus,c,c->cs);push(bus,c,c->di);'),
    ('local string at bp-12h', 'c->di=local(c,-0x13);push(bus,c,c->ss);push(bus,c,c->di);', 'c->di=local(c,-0x12);push(bus,c,c->ss);push(bus,c,c->di);'),
    ('SUB SP flags dropped', 'subtract16(c,c->sp,0x14u);c->sp', 'c->sp'),
    ('result pointer read from [bp+8]', 'les(bus,c,c->ss,local(c,6));pr_bus_write8(bus,physical(c->es,c->di),0u);', 'les(bus,c,c->ss,local(c,8));pr_bus_write8(bus,physical(c->es,c->di),0u);'),
]


def main():
    original = SRC.read_text()
    survivors, caught = [], []
    bodies, decoded, meta, literal = A.source()
    for label, old, new in MUTANTS:
        if old not in original:
            raise SystemExit(f'mutant text absent: {label}')
        with tempfile.TemporaryDirectory(prefix='mut0239-') as tmp:
            mutated = Path(tmp) / 'overlay_u23_append_0239_cpu.c'
            mutated.write_text(original.replace(old, new, 1))
            sources = [str(mutated if s == 'overlay_u23_append_0239_cpu' else A.ROOT / 'src/recovered' / (s + '.c')) for s in A.SOURCES]
            so = Path(tmp) / 'm.so'
            built = subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-O2', '-fPIC', '-shared', '-I', str(A.ROOT / 'include'),
                                    *sources, '-o', str(so)], capture_output=True, text=True)
            if built.returncode:
                print(f'NOT BUILT (replace this mutant): {label}: {built.stderr[-200:]}')
                survivors.append(label + ' (did not build)')
                continue
            import ctypes
            lib = ctypes.CDLL(str(so))
            f = lib.pr_u23_append_0239_cpu
            f.argtypes = [ctypes.POINTER(A.Bus), ctypes.c_size_t, ctypes.POINTER(A.Cpu), ctypes.POINTER(A.Limits), ctypes.POINTER(A.Diag)]
            f.restype = ctypes.c_int
            lib.pr_ega_reset.argtypes = [ctypes.POINTER(A.Ega)]
            try:
                A.differential(bodies, decoded, literal, lib, quick=True, smoke=True)
                survivors.append(label)
                print('SURVIVED', label, flush=True)
            except (ValueError, AssertionError) as exc:
                caught.append(label)
                print('caught', label, '-', str(exc)[:90], flush=True)
    print(f'{len(caught)}/{len(MUTANTS)} mutants caught')
    if survivors:
        raise SystemExit('survivors: ' + '; '.join(survivors))


if __name__ == '__main__':
    main()
