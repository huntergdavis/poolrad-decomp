#!/usr/bin/env python3
"""Build the hybrid executor's models object (VP i20767: glue every credited body).

Links exactly the C the requested entry symbols need: each src/recovered/*.c is compiled once into a cache keyed by
its content hash (gcc -std=c11 -O2 -I include -c), `nm` maps the global symbols, and the closure of undefined `pr_*`
symbols from the entries is followed to the objects that define them. memory_bus.c and ega_hw.c are never linked: the
DOSBox-X patch defines the pr_bus_* entry points (replay or route). A needed symbol that no object defines, or that two
define, is an error, not a guess. Output: one `ld -r` object (default resources/hybrid/pr_models.o) and its source list.
Usage: build_models.py --entries SYM[,SYM...] [--out PATH] [--cache DIR]"""
import argparse, hashlib, os, subprocess, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
HOST = {'memory_bus.c', 'ega_hw.c'}      # pr_bus_* come from the dispatch patch


def nm(obj):
    out = subprocess.run(['nm', '--defined-only', '-g', str(obj)], capture_output=True, text=True, check=True).stdout
    undef = subprocess.run(['nm', '-u', str(obj)], capture_output=True, text=True, check=True).stdout
    return ({l.split()[-1] for l in out.splitlines() if l.strip()},
            {l.split()[-1] for l in undef.splitlines() if l.strip()})


def main():
    p = argparse.ArgumentParser(); p.add_argument('--entries', required=True)
    p.add_argument('--out', type=Path, default=ROOT / 'resources/hybrid/pr_models.o')
    p.add_argument('--cache', type=Path, default=ROOT / 'resources/hybrid/objs'); a = p.parse_args()
    a.cache.mkdir(parents=True, exist_ok=True)
    sources = sorted(s for s in (ROOT / 'src/recovered').glob('*.c') if s.name not in HOST)
    defines, needs, objs = {}, {}, {}
    for n, src in enumerate(sources, 1):        # bounded: one compile per source
        digest = hashlib.sha256(src.read_bytes()).hexdigest()[:16]
        obj = a.cache / f'{src.stem}.{digest}.o'
        if not obj.exists():
            r = subprocess.run(['gcc', '-std=c11', '-O2', '-I', str(ROOT / 'include'), '-c', str(src), '-o', str(obj)],
                               capture_output=True, text=True)
            if r.returncode:
                print(f'COMPILE FAILED {src.name}: {r.stderr.strip()[:300]}', file=sys.stderr); continue
        d, u = nm(obj); objs[src.name] = obj; needs[src.name] = u
        for s in d:
            defines.setdefault(s, []).append(src.name)
        if n % 50 == 0:
            print(f'[{n}/{len(sources)}] sources compiled or cached', file=sys.stderr, flush=True)
    todo = [s.strip() for s in a.entries.split(',') if s.strip()]
    chosen, seen, errors = set(), set(), []
    while todo:
        s = todo.pop()
        if s in seen:
            continue
        seen.add(s)
        where = defines.get(s, [])
        if len(where) != 1:
            if s.startswith('pr_bus_'):
                continue                         # provided by the patch
            errors.append(f'{s}: defined by {where or "nothing"}'); continue
        if where[0] not in chosen:
            chosen.add(where[0])
            todo += [u for u in needs[where[0]] if u.startswith('pr_')]
    if errors:
        print('UNRESOLVED:\n  ' + '\n  '.join(sorted(errors)), file=sys.stderr); sys.exit(2)
    subprocess.run(['ld', '-r', '-o', str(a.out)] + [str(objs[c]) for c in sorted(chosen)], check=True)
    a.out.with_suffix('.sources').write_text('\n'.join(sorted(chosen)) + '\n')
    print(f'entries={len(a.entries.split(","))} sources_linked={len(chosen)} out={os.path.relpath(a.out, ROOT)}')


if __name__ == '__main__':
    main()
