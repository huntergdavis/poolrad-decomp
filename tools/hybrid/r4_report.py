#!/usr/bin/env python3
"""Attribute service-route arrivals by measured guest entry order, never credit."""
import argparse
from collections import Counter
import csv
import json
from pathlib import Path

from r1_report import report as base_report


def attribute(sequence, counters, candidates, phases, actions):
    if len(set(candidates)) != len(candidates):
        raise ValueError('duplicate candidate ID')
    if set(actions) - set(phases):
        raise ValueError('action marker has no service phase')
    observed = Counter(row['id'] for row in sequence)
    if set(observed) - set(counters) or any(observed[k] != v for k, v in counters.items()):
        raise ValueError('sequence/count mismatch: incomplete flush or sequence cap')
    required = set(candidates) | set(phases.values()) | set(actions.values())
    if required - set(counters):
        raise ValueError('candidate or marker missing from registry report')
    if set(candidates) & (set(phases.values()) | set(actions.values())):
        raise ValueError('markers must not be candidates')
    instructions = [row['instruction'] for row in sequence]
    if any(a > b for a, b in zip(instructions, instructions[1:])):
        raise ValueError('observer instruction order is not monotonic')
    entries = []
    for phase, marker in phases.items():
        indices = [i for i, row in enumerate(sequence) if row['id'] == marker]
        if len(indices) != 1:
            raise ValueError('expected exactly one measured entry: ' + phase)
        entries.append((phase, indices[0]))
    if not entries or any(a[1] >= b[1] for a, b in zip(entries, entries[1:])):
        raise ValueError('service entries absent or out of route order')
    counts = {'before_services': Counter(row['id'] for row in sequence[:entries[0][1]])}
    for index, (phase, start) in enumerate(entries):
        end = entries[index + 1][1] if index + 1 < len(entries) else len(sequence)
        counts[phase] = Counter(row['id'] for row in sequence[start + 1:end])
        if phase in actions and counts[phase][actions[phase]] == 0:
            raise ValueError('action marker absent within service phase: ' + phase)
        if phase in actions and counts[phase][actions[phase]] != counters[actions[phase]]:
            raise ValueError('action marker outside service phase: ' + phase)
    rows = [dict(id=name, whole_run=counters[name],
                 **{phase: count[name] for phase, count in counts.items()}) for name in candidates]
    return dict(sequence_complete=True,
                path_scope='From each service entry until the next service entry or run end; not caller stacks.',
                entry_instructions={phase: sequence[i]['instruction'] for phase, i in entries},
                marker_arrivals={name: counters[name] for name in set(phases.values()) | set(actions.values())},
                arrived_by_phase={phase: sum(count[name] > 0 for name in candidates)
                                  for phase, count in counts.items()},
                candidates_by_path=rows)


def report(run):
    route = json.loads((run / 'r1-route.json').read_text())
    result = base_report(run, route)
    if not result['success']:
        raise ValueError('route did not complete')
    if result['mode'] not in ('observer', 'dispatch'):
        return result
    if result['forced_kill']:
        raise ValueError('forced termination cannot establish a final observer report')
    counters = {}
    with (run / 'observer.tsv').open() as stream:
        for row in csv.reader(stream, delimiter='\t'):
            if row and not row[0].startswith('#'):
                if row[0] in counters:
                    raise ValueError('duplicate counter ID')
                counters[row[0]] = int(row[1])
    sequence = []
    with (run / 'observer.seq').open() as stream:
        for row in csv.reader(stream, delimiter='\t'):
            if row and not row[0].startswith('#'):
                sequence.append(dict(id=row[1], instruction=int(row[3])))
    result.update(attribute(sequence, counters, route['candidates'],
                            route['service_markers'], route['action_markers']))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    result = report(args.run)
    (args.run / 'r4-report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
