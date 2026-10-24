#!/usr/bin/env python3
"""Report R1 timings and observed candidate arrivals, never replacement credit."""
import argparse
import csv
import json
from pathlib import Path


def report(run, route):
    summary=json.loads((run/'r1-summary.json').read_text())
    result=dict(run=run.name,success=summary['success'],error=summary['error'],
                shutdown=summary['shutdown'],mode=(run/'r1-mode.txt').read_text().strip(),
                timings=[dict(gate=r['name'],visit=r['visit'],wait_s=r['wait_s'],
                              elapsed_s=r['elapsed_s']) for r in summary['gates']])
    if result['mode']=='observer':
        result['counts_verified_shutdown'] = not summary['shutdown'].startswith('SIGKILL')
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
    route=json.loads((args.run/'r1-route.json').read_text())
    result=report(args.run,route)
    (args.run/'r1-report.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__':
    main()
