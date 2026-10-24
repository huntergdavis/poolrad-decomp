#!/usr/bin/env python3
"""Attribute R2 observer arrivals after a measured Camp entry, without credit."""
import argparse
from collections import Counter
import csv
import json
from pathlib import Path

from r1_report import report as base_report


def attribute(sequence, counters, candidates, markers, shared):
    """Use guest event order; UI timestamps and buffered file size are not gates."""
    observed = Counter(row['id'] for row in sequence)
    if set(observed) - set(counters) or any(
            observed[name] != count for name, count in counters.items()):
        raise ValueError('sequence/count mismatch: incomplete flush or sequence cap')
    required = set(candidates) | set(markers.values())
    if required - set(counters):
        raise ValueError('candidate or observation marker missing from registry report')
    camp = [i for i, row in enumerate(sequence) if row['id'] == markers['camp']]
    if len(camp) != 1:
        raise ValueError('expected exactly one measured Camp entry')
    instructions = [row['instruction'] for row in sequence]
    if any(a > b for a, b in zip(instructions, instructions[1:])):
        raise ValueError('observer instruction order is not monotonic')
    after = Counter(row['id'] for row in sequence[camp[0] + 1:])
    rows = [dict(id=name, whole_run=counters[name],
                 before_camp=counters[name] - after[name], after_camp=after[name],
                 shared_with_r1=name in shared,
                 status='arrived after Camp entry' if after[name] else 'not observed after Camp entry')
            for name in candidates]
    return dict(sequence_complete=True,
                path_scope='After the measured Camp entry through end of run; not reconstructed caller stacks.',
                camp_entry_instruction=sequence[camp[0]]['instruction'],
                marker_arrivals={phase: counters[name] for phase, name in markers.items()},
                arrived_after_camp=sum(row['after_camp'] > 0 for row in rows),
                shared_arrived_after_camp=sum(row['after_camp'] > 0 and row['shared_with_r1'] for row in rows),
                candidates_by_path=rows)


def report(run):
    route = json.loads((run / 'r1-route.json').read_text())
    result = base_report(run, route)
    if not result['success']:
        raise ValueError('route did not complete')
    if result['mode'] != 'observer':
        return result
    if result['forced_kill']:
        raise ValueError('forced termination cannot establish a final observer report')
    counters = {}
    for row in csv.reader((run / 'observer.tsv').read_text().splitlines(), delimiter='\t'):
        if row and not row[0].startswith('#'):
            counters[row[0]] = int(row[1])
    sequence = []
    for row in csv.reader((run / 'observer.seq').read_text().splitlines(), delimiter='\t'):
        if row and not row[0].startswith('#'):
            sequence.append(dict(id=row[1], instruction=int(row[3])))
    result.update(attribute(sequence, counters, route['candidates'],
                            route['observation_markers'], set(route['shared_with_r1'])))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    result = report(args.run)
    (args.run / 'r2-report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
