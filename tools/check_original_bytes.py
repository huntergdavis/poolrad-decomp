#!/usr/bin/env python3
"""t139 predicate: flag docs/re/*.md that reproduce original game bytes.

A doc is IN SCOPE (flagged) iff it contains >=8 consecutive hex byte tokens on
one line (a hex byte dump). BATCH1 (docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md)
must NOT be flagged (known-clean control).
"""
import glob, re, sys
BARE = re.compile(r'(?<![0-9A-Za-z])([0-9a-fA-F]{2}(?:[ \t]+[0-9a-fA-F]{2}){7,})(?![0-9A-Za-z])')
files = sorted(glob.glob('docs/re/*.md'))
def has_byte_like(run):
    for tok in run.split():
        if any(c in 'abcdefABCDEF' for c in tok) or int(tok,16) >= 0x80:
            return True
    return False

hits = []
for f in files:
    txt = open(f).read()
    for m in BARE.finditer(txt):
        if has_byte_like(m.group(1)):
            hits.append(f)
            break
for h in hits:
    print(h)
control = 'docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md'
if control in hits:
    print('ERROR: known-clean control flagged', file=sys.stderr)
    sys.exit(2)
sys.exit(1 if hits else 0)
