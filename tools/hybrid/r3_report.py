#!/usr/bin/env python3
"""Compare a fresh R3 original pair and optional V1/route runs; no credit edits."""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def snapshot(run):
    summary = json.loads((run / 'r1-summary.json').read_text())
    require(summary['success'] and summary['error'] is None, f'{run}: route failed')
    require(summary['shutdown'] == 'SIGTERM', f'{run}: incomplete shutdown')
    require((run / 'r1-seed.txt').read_text().strip() == '12345678', f'{run}: wrong seed')
    counters = {}
    for row in csv.reader((run / 'observer.tsv').read_text().splitlines(), delimiter='\t'):
        if row and not row[0].startswith('#'):
            require(row[0] not in counters, 'duplicate observer row')
            counters[row[0]] = int(row[1])
    sequence = Counter()
    last_n, last_instruction = 0, -1
    for row in csv.reader((run / 'observer.seq').read_text().splitlines(), delimiter='\t'):
        if row and not row[0].startswith('#'):
            n, instruction = int(row[0]), int(row[3])
            require(n == last_n + 1 and instruction >= last_instruction, 'sequence order differs')
            last_n, last_instruction = n, instruction
            sequence[row[1]] += 1
    require(set(sequence) <= set(counters) and
            all(sequence[k] == v for k, v in counters.items()), 'sequence/count mismatch')
    draws = (run / 'observer.draws').read_text()
    pin = re.findall(r'# PINNED RandSeed := 12345678\b.*? at draw (\d+)', draws)
    require(len(pin) == 1, 'exactly one seed pin required')
    rows = [line.split('\t') for line in draws.splitlines() if line and not line.startswith('#')]
    require([int(r[0]) for r in rows] == list(range(1, len(rows) + 1)), 'draw sequence incomplete')
    markers = [k for k in counters if k.startswith('DRAW_')]
    require(len(markers) == 1 and counters[markers[0]] == len(rows), 'draw/count mismatch')
    rng = [r[4] for r in rows if int(r[0]) >= int(pin[0])]
    require(rng and rng[0] == '12345678', 'pin absent from RNG values')
    hashes = {name: hashlib.sha256((run / name).read_bytes()).hexdigest()
              for name in ('r1-driver.py', 'r1-route.json', 'r1-registry.tsv', '.oracle-input-sha256')}
    hashes['binary'] = (run / 'r1-hashes.txt').read_text().split()[0]
    return dict(run=str(run), mode=(run / 'r1-mode.txt').read_text().strip(), hashes=hashes,
                actions=[(r['name'], r['key']) for r in summary['gates']],
                final_pixels=summary['final_pixels_sha256'], rng=rng, counters=counters,
                sequence_rows=last_n, elapsed_s=summary['gates'][-1]['elapsed_s'])


def pair(a, b):
    require(a['mode'] == b['mode'] == 'original', 'pair must contain two original runs')
    checks = {field: a[field] == b[field] for field in ('hashes', 'actions', 'final_pixels', 'rng')}
    require(all(checks.values()), f'original pair differs: {checks}')
    return checks


def dispatch(run, routing):
    text = (run / 'observer.dispatch').read_text()
    require(('# route mode:' in text) == routing, 'wrong dispatch mode')
    rows = {}
    for line in text.splitlines():
        match = re.match(r'# ((?:OVL|RES)_\w+) arrivals=', line)
        if match:
            require(match[1] not in rows, 'duplicate dispatch summary')
            rows[match[1]] = {k: int(v) for k, v in re.findall(r'(\w+)=(\d+)', line)}
    require(rows, 'no dispatch summaries')
    for name, row in rows.items():
        require(not any(row[k] for k in ('c_differ', 'midbody_handback_differ',
                                        'argument_rerun_differ')), f'{name}: dispatch mismatch')
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original_a', type=Path)
    parser.add_argument('original_b', type=Path)
    parser.add_argument('--v1', type=Path)
    parser.add_argument('--route', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    a, b = snapshot(args.original_a), snapshot(args.original_b)
    result = dict(original_pair_checks=pair(a, b), originals=[a, b])
    for name, path in (('v1', args.v1), ('route', args.route)):
        if path is None:
            continue
        current = snapshot(path)
        require(current['mode'] == 'dispatch', 'expected dispatch run')
        checks = {field: a[field] == current[field] for field in ('hashes', 'actions', 'final_pixels')}
        require(all(checks.values()), f'{name} route differs: {checks}')
        rng_equal = a['rng'] == current['rng']
        if name == 'v1':
            require(rng_equal, 'V1 original RNG trace differs')
        rows = dispatch(path, name == 'route')
        candidates = json.loads((path / 'r1-route.json').read_text())['candidates']
        require(set(candidates) <= rows.keys(), 'target missing from dispatch summaries')
        current.update(comparison_checks=checks, original_rng_trace_equal=rng_equal,
                       dispatch=rows, targets=candidates)
        result[name] = current
    if 'route' in result:
        credits = {r['id'] for r in csv.DictReader((ROOT / 'docs/re/strict_full_c_addresses.csv').open())}
        route = result['route']
        require(credits <= a['counters'].keys() and credits <= route['counters'].keys(), 'credit registry incomplete')
        result['original_executions'] = dict(
            registered_credited_ids=len(credits),
            without_c=sum(a['counters'][k] for k in credits),
            with_c=sum(route['counters'][k] for k in credits) - sum(
                row['returned_by_c'] for k, row in route['dispatch'].items() if k in credits))
    result['scope'] = ('Measured route only; interruptions, timeouts, refusals and stops remain named. '
                       'Routed C RNG calls are absent from original guest RNG observations. '
                       'No recovery or replacement credit is assigned by this report.')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k not in ('originals', 'v1', 'route')}))


if __name__ == '__main__':
    main()
