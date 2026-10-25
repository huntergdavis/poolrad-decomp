#!/usr/bin/env python3
"""Report R1 timings and observed candidate arrivals, never replacement credit."""
import argparse
import csv
import json
from pathlib import Path
import sys
from emulator_provenance import MissingProvenance, read_receipt


def report(run, route):
    summary=json.loads((run/'r1-summary.json').read_text())
    result=dict(run=run.name,success=summary['success'],error=summary['error'],
                shutdown=summary['shutdown'],final_pixels_sha256=summary.get('final_pixels_sha256'),mode=(run/'r1-mode.txt').read_text().strip(),
                timings=[dict(gate=r['name'],visit=r['visit'],wait_s=r['wait_s'],
                              elapsed_s=r['elapsed_s']) for r in summary['gates']])
    seed_file=run/'r1-seed.txt'
    result['seed_pin']=seed_file.read_text().strip() if seed_file.exists() else ''
    if result['seed_pin']:
        result['seed_pin_observed']=('# PINNED RandSeed := '+result['seed_pin'].upper()+' ') in (run/'observer.draws').read_text()
    if result['mode'] in ('observer', 'dispatch'):   # a dispatch run's observer counts arrivals the same way
        result['forced_kill'] = summary['shutdown'].startswith('SIGKILL')
        result['counts_scope'] = ('last periodic snapshot; zero is inconclusive'
                                  if result['forced_kill'] else 'report file; final flush expected on normal or SIGTERM exit')
        counts={}
        for row in csv.reader((run/'observer.tsv').read_text().splitlines(),delimiter='\t'):
            if row and not row[0].startswith('#') and len(row)>=2:
                counts[row[0]]=int(row[1])
        result['candidates']=[dict(id=name,arrivals=counts.get(name),
                                   status='missing' if name not in counts else
                                   'arrived' if counts[name] else 'not observed')
                              for name in route['candidates']]
        result['arrived_bodies']=sum(r['status']=='arrived' for r in result['candidates'])
        result['missing_candidates']=sum(r['status']=='missing' for r in result['candidates'])
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=Path)
    args=parser.parse_args()
    try:
        emulator = read_receipt(args.run)
    except MissingProvenance as error:
        print('native evidence CANNOT_RUN: ' + str(error), file=sys.stderr)
        return 4
    route=json.loads((args.run/'r1-route.json').read_text())
    result=report(args.run,route)
    result['emulator'] = emulator
    (args.run/'r1-report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))
    return 0


if __name__=='__main__':
    sys.exit(main())
