#!/usr/bin/env python3
"""Reject stale t131 closure reasons against recorded review dispositions.

This checks documentation consistency and reference shape, not the truth of
proofs, native parity, acceptance, readiness, or accounting. A recorded
discharge needs independent review; the checker does not manufacture it.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
STATE = ROOT / 'docs/re/res16ec_closure_obligations.json'
DOCUMENT = ROOT / 'docs/re/RES_00BA_16EC_CLOSURE_STATUS.md'
IDS = {f'NC{i}' for i in range(1, 6)}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def normalized(text):
    return ' '.join(text.split())


def check(state, document):
    require(state.get('schema') == 1, 'unsupported closure-status schema')
    rows = state.get('obligations', [])
    counts = Counter(row['id'] for row in rows)
    require(set(counts) == IDS and all(n == 1 for n in counts.values()),
            'obligation IDs must be exactly NC1 through NC5 once each')
    stated = re.findall(r'^\| (NC\d+) \|', document, re.M)
    require(len(stated) == len(set(stated)), 'duplicate documented open reason')
    require(set(stated) <= IDS, 'unknown documented open reason')
    open_ids = set()
    for row in rows:
        ident = row['id']
        require(row['state'] in ('open', 'discharged'), f'{ident}: invalid state')
        require(bool(row.get('removal_test')), f'{ident}: missing falsification criterion')
        if row['state'] == 'open':
            open_ids.add(ident)
            require(row.get('discharge') is None, f'{ident}: open with a discharge record')
            require(ident in stated, f'{ident}: open reason missing from verdict')
            require(normalized(row['removal_test']) in normalized(document),
                    f'{ident}: documented removal test differs from status record')
        else:
            proof = row.get('discharge') or {}
            for field in ('reviewer', 'review_ref', 'proof_ref', 'scope'):
                require(isinstance(proof.get(field), str) and bool(proof[field].strip()),
                        f'{ident}: discharge missing {field}')
            for field, size in (('reviewed_head', 40), ('proof_sha256', 64)):
                require(re.fullmatch(r'[0-9a-f]{' + str(size) + '}', proof.get(field, '')),
                        f'{ident}: discharge missing valid {field}')
            require(ident not in stated, f'{ident}: stale open reason after recorded discharge')
    require(set(stated) == open_ids, 'documented reasons differ from recorded open set')
    return sorted(open_ids)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--state', type=Path, default=STATE)
    parser.add_argument('--document', type=Path, default=DOCUMENT)
    args = parser.parse_args()
    try:
        open_ids = check(json.loads(args.state.read_text()), args.document.read_text())
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(1, f'closure-document freshness FAILED: {error}\n')
    print('closure-document freshness PASS: recorded open reasons ' + ', '.join(open_ids)
          + '; proof validity, readiness and closure NOT VERIFIED')


if __name__ == '__main__':
    main()
