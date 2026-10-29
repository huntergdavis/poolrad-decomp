import hashlib
def verify_evidence(base="main", head="HEAD"):
    import subprocess
    out = subprocess.check_output(["git","diff","--name-only",f"{base}..{head}"], text=True)
    files = [f for f in out.split() if f.startswith("docs/re/") and f.endswith(".md")]
    ok = True
    for f in files:
        try:
            pre = subprocess.check_output(["git","show",f"{base}:{f}"], text=True, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError:
            continue
        post = subprocess.check_output(["git","show",f"{head}:{f}"], text=True, stderr=subprocess.DEVNULL)
        runs = []
        for m in BARE.finditer(pre):
            if has_byte_like(m.group(1)):
                runs.append(bytes.fromhex(m.group(1).replace(" ","").replace("\t","")))
        markers = re.findall(r"\[scrubbed: span=\S+ sha256=([0-9a-f]{64}) bytes=(\d+)", post)
        if not markers:
            continue
        if len(markers) != len(runs):
            print(f"MISMATCH count {f}: markers={len(markers)} runs={len(runs)}")
            ok = False
            continue
        for (h, n), run in zip(markers, runs):
            if int(n) != len(run) or h != hashlib.sha256(run).hexdigest():
                print(f"MISMATCH {f}: marker bytes={n}/hash={h[:12]} vs run bytes={len(run)}/hash={hashlib.sha256(run).hexdigest()[:12]}")
                ok = False
    if ok:
        print("verify-evidence: all markers match pre-image runs")
    return 0 if ok else 1

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
if "--verify-evidence" in sys.argv:
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument("--verify-evidence", action="store_true")
    ap.add_argument("--base", default="main")
    ap.add_argument("--head", default="HEAD")
    a = ap.parse_args()
    sys.exit(verify_evidence(a.base, a.head))

control = 'docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md'
if control in hits:
    print('ERROR: known-clean control flagged', file=sys.stderr)
    sys.exit(2)
sys.exit(1 if hits else 0)
