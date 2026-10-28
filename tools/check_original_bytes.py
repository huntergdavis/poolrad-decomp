#!/usr/bin/env python3
"""t139 predicate: flag docs/re/*.md that reproduce original game bytes.

A doc is IN SCOPE (flagged) iff it contains >=8 consecutive hex byte tokens on
one line (a hex byte dump). BATCH1 (docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md)
must NOT be flagged (known-clean control).
"""
import glob, re, sys
pat = re.compile(r'(?:(?:[0-9a-fA-F]{2}[ \t]+){7,}[0-9a-fA-F]{2}\b)')
files = sorted(glob.glob('docs/re/*.md'))
hits = [f for f in files if pat.search(open(f).read())]
for h in hits:
    print(h)
control = 'docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md'
if control in hits:
    print('ERROR: known-clean control flagged', file=sys.stderr)
    sys.exit(2)
sys.exit(1 if hits else 0)
