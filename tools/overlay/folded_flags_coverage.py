#!/usr/bin/env python3
"""Index historical folded-flag claims to existing proofs; not a new liveness proof."""
import argparse
from collections import defaultdict
import csv
import io
from pathlib import Path
import re

import flag_fold_liveness as shared
from u3c6_10a7_folded_flags import GROUPS

ROOT = Path(__file__).resolve().parents[2]
INDEX = ROOT / 'docs/re/FLAG_FOLDED_COVERAGE.tsv'
DOCUMENTS = {
    'u31': 'FLAG_FOLDED_U31_0255_0675.md',
    'u3b0': 'FLAG_FOLDED_U3B0_00CB.md',
    'u77': 'FLAG_FOLDED_U77_28BE.md',
    'slice5': 'FLAG_FOLDED_SLICE5.md',
    'slice6': 'FLAG_FOLDED_SLICE6.md',
    'u3c6': 'U3C6_10A7_FOLDED_FLAGS.md',
    'u7f': 'FLAG_FOLDED_U7F_00A8.md',
}


def render():
    with (ROOT / 'docs/re/FLAG_FOLDED_SITES.tsv').open() as source:
        ledger = list(csv.DictReader(source, delimiter='\t'))
    keys = {(row['file'], int(row['line'])) for row in ledger}
    if len(keys) != len(ledger):
        raise ValueError('duplicate historical ledger row')
    owners = defaultdict(set)
    for name, entries in shared.PRESETS.items():
        for _, _, label, _ in entries:
            match = re.match(r'^([^:]+\.c):(\d+)', label)
            if match and (key := (match[1], int(match[2]))) in keys:
                owners[key].add(name)
    for line, _ in GROUPS:
        owners[('overlay_u3c6_caller_10a7_cpu.c', line)].add('u3c6')
    # The four rows documented by FLAG_FOLDED_U7F_00A8.md use its explicit CLI,
    # not a shared PRESETS entry. Do not infer coverage for future rows.
    for line in (196, 210, 215, 217):
        owners[('overlay_u7f_weighted_00a8_cpu.c', line)].add('u7f')
    if set(owners) != keys:
        raise ValueError(f'coverage mismatch: missing={sorted(keys-set(owners))}, extra={sorted(set(owners)-keys)}')
    if any(len(names) != 1 for names in owners.values()):
        raise ValueError('a ledger row has multiple proof-suite owners')
    stream = io.StringIO()
    writer = csv.writer(stream, delimiter='\t', lineterminator='\n')
    writer.writerow(('file', 'historical_line', 'proof_suite', 'evidence_document'))
    for row in ledger:
        name = next(iter(owners[(row['file'], int(row['line']))]))
        doc = 'docs/re/' + DOCUMENTS[name]
        if not (ROOT / doc).is_file():
            raise ValueError('missing evidence document: ' + doc)
        writer.writerow((row['file'], row['line'], name, doc))
    return stream.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='compare committed index; no media-proof claim')
    args = parser.parse_args()
    try:
        text = render()
        if args.check:
            if not INDEX.is_file() or INDEX.read_text() != text:
                raise ValueError('coverage index differs; regenerate with this command without --check')
            print('Coverage index matches existing proof references; this is not a media-proof verdict.')
        else:
            print(text, end='')
    except (ValueError, KeyError, OSError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
