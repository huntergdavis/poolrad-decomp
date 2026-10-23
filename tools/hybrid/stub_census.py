#!/usr/bin/env python3
"""Census of the overlay-stub precondition class (VP i21329).

A raw-CPU model that composes a callee in ANOTHER overlay unit stands in for a far call through that unit's overlay stub.
In the game the stub is either loaded (a far JMP, EAh, to the entry) or not (the overlay manager loads the unit with DOS
reads first). A model that runs the callee without reading the stub is valid only while the unit is resident.

For every src/recovered/*.c whose entry has the raw-CPU shape (uint8_t *ram, size_t, CPU *), this lists the cross-unit
model calls (pr_uXX_* with XX != the file's own unit) and classes the file:
  a  the source tests for EAh (taken as reading the stub)
  b  no EAh test, but the source or its header says the unit/stub is loaded/resident (a declared precondition)
  c  neither
Detection is lexical: every line is a lead for a reviewer, not a verdict. Usage: stub_census.py [--strict CSV]"""
import argparse, csv, glob, os, re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(); p.add_argument('--strict', default=str(ROOT / 'docs/re/strict_full_c_addresses.csv'))
    a = p.parse_args()
    credited = {r['id'] for r in csv.DictReader(open(a.strict))}
    rows = []
    for path in sorted(glob.glob(str(ROOT / 'src/recovered/*.c'))):
        name = os.path.basename(path); text = open(path, errors='replace').read(); flat = ' '.join(text.split())
        own = re.match(r'overlay_u([0-9a-f]+)_', name)
        own = own.group(1).lstrip('0') if own else 'resident'
        calls = sorted({u for _, u in re.findall(r'\b(pr_u([0-9a-f]+)_[a-z0-9_]+)\s*\(', text) if u.lstrip('0') != own})
        if not calls or not re.search(r'\(\s*uint8_t \*\s*\w+,\s*size_t \w+,\s*(const )?\w+ \*\s*\w+', flat):
            continue
        headers = ''.join(open(ROOT / 'include' / h, errors='replace').read()
                          for h in re.findall(r'#include "(poolrad/recovered/[^"]+)"', text) if (ROOT / 'include' / h).exists())
        reads = bool(re.search(r'0x[eE][aA]u?\b', text))
        declared = bool(re.search(r'(must be|already|assum\w*).{0,40}(loaded|resident)|loaded (stub|unit)|stub.{0,30}loaded',
                                  text + headers, re.I))
        cls = 'a' if reads else 'b' if declared else 'c'
        unit = re.match(r'overlay_u([0-9a-f]+)_', name)
        offs = re.findall(r'(?<=_)([0-9a-f]{4})(?=_|\.c$)', name)        # the last underscore-delimited 4-hex token
        ident = f'OVL_{int(unit.group(1), 16):04X}_{offs[-1].upper()}' if unit and offs else '-'
        rows.append((cls, name, ident, ident in credited, ','.join(calls)))
    for cls in 'abc':
        sel = [r for r in rows if r[0] == cls]
        print(f'class {cls}: {len(sel)} ({sum(r[3] for r in sel)} with a credited ID)')
    for r in rows:
        if r[0] != 'a':
            print(f'  {r[0]}  {r[1]:45} {r[2]:15} {"credited" if r[3] else "not credited"}  calls units {r[4]}')


if __name__ == '__main__':
    main()
