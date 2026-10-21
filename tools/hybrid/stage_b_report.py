#!/usr/bin/env python3
"""Stage (b) acceptance numbers (VP i20735) from two observer runs made with the same registry:
a cold start to the main menu (BASE) and the save-walk route (WALK).

Prints, numbers only:
  1. per-ID arrival counts on the walk and its total instruction count (the observer header);
  2. the registry: rows, rows observable (unique signature), and every unobservable row by ID (named exclusions);
  3. the IDs the walk reaches that the cold start does not, credited and not credited, against the CREDITED list
     (docs/re/strict_full_c_addresses.csv at the commit this runs on);
  4. the top N by walk arrivals with credited status and cold-start arrivals.
Usage: stage_b_report.py BASE_observer.tsv WALK_observer.tsv [--top 10] [--tsv OUT]"""
import argparse, csv, re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]


def load(path):
    head, skipped, counts = '', [], {}
    for line in open(path):
        if line.startswith('#skipped\t'):
            skipped.append(line.split('\t', 1)[1].strip())
        elif line.startswith('# stage0'):
            head = line[2:].strip()
        elif not line.startswith('#') and line.strip():
            ident, n, first_insn, first_cs = line.rstrip('\n').split('\t')
            counts[ident] = (int(n), int(first_insn), first_cs)
    return head, skipped, counts


def main():
    p = argparse.ArgumentParser(); p.add_argument('base'); p.add_argument('walk')
    p.add_argument('--top', type=int, default=10); p.add_argument('--tsv'); a = p.parse_args()
    credited = {r['id'] for r in csv.DictReader(open(ROOT / 'docs/re/strict_full_c_addresses.csv'))}
    bh, bskip, base = load(a.base); wh, wskip, walk = load(a.walk)
    assert set(base) == set(walk) and bskip == wskip, 'the two runs used different registries'
    field = lambda h, k: int(re.search(k + r'=(\d+)', h).group(1))
    obs = set(walk); cred_obs = obs & credited
    print(f'walk header: {wh}\nbase header: {bh}')
    print(f'registry: observable={len(obs)} unobservable={len(wskip)} credited={len(credited)} '
          f'credited_observable={len(cred_obs)} credited_unobservable={sorted(credited - obs)}')
    for s in wskip:
        print(f'  unobservable: {s}')
    reached_b = {i for i, v in base.items() if v[0]}; reached_w = {i for i, v in walk.items() if v[0]}
    print(f'cold start reaches {len(reached_b)} ({len(reached_b & credited)} credited); '
          f'walk reaches {len(reached_w)} ({len(reached_w & credited)} credited)')
    print(f'walk arrivals={field(wh, "arrivals")} instructions={field(wh, "instructions")}; '
          f'credited arrivals={sum(walk[i][0] for i in reached_w & credited)}')
    off_cold = cred_obs - reached_b
    new = reached_w - reached_b
    print(f'credited observable IDs not reached by the cold start: {len(off_cold)}; of those the walk reaches '
          f'{len(new & credited)}')
    print(f'uncredited IDs the walk reaches that the cold start does not: {len(new - credited)}')
    print(f'reached by cold start but not by the walk: {sorted(reached_b - reached_w)}')
    rank = sorted(reached_w, key=lambda i: (-walk[i][0], i))
    print(f'\ntop {a.top} by walk arrivals:\nrank\tid\twalk\tcold_start\tcredited')
    for k, i in enumerate(rank[:a.top], 1):
        print(f'{k}\t{i}\t{walk[i][0]}\t{base[i][0]}\t{"credited" if i in credited else "NOT credited"}')
    print(f'\ntop {a.top} among IDs the cold start does not reach:\nrank\tid\twalk\tcredited')
    for k, i in enumerate([i for i in rank if i in new][:a.top], 1):
        print(f'{k}\t{i}\t{walk[i][0]}\t{"credited" if i in credited else "NOT credited"}')
    if a.tsv:
        with open(a.tsv, 'w') as f:
            f.write('id\twalk_arrivals\tcold_start_arrivals\tcredited\twalk_first_instruction\twalk_first_cs\n')
            for i in rank:
                f.write(f'{i}\t{walk[i][0]}\t{base[i][0]}\t{int(i in credited)}\t{walk[i][1]}\t{walk[i][2]}\n')


if __name__ == '__main__':
    main()
