#!/usr/bin/env python3
"""Stage 0 of the hybrid executor (docs/HYBRID_EXECUTOR_SCOPE.md, VP i20556): the smallest ID -> code registry that can
identify a credited body at run time.

For every ID in docs/re/strict_full_c_addresses.csv it records the entry offset and a SIGNATURE: the first SIG (32) bytes of the
body from the media (GAME.OVR unit code for OVL_, the loader-expanded START.EXE image for RES_), with every relocated word
masked (the unit's fixup table / the resident relocation sites), because the game's loader rewrites those words at run time.
A prefix that is not unique grows to 64, 128 and then 256 bytes (never past the unit or image end) until it is.
The observer matches a guest instruction at IP == offset whose masked bytes equal the signature. The signature is checked
for UNIQUENESS against all overlay code and the whole resident image: a signature that occurs more than once could be
misattributed, and the observer is told to ignore it.

It also records, where one exists, the C entry symbol an audit binds for the row (CHECKER_ROWS + ctypes CDLL(...).pr_*).
What this registry CANNOT do yet: it carries no call shape, CPU struct, env or budgets (dispatch needs all four; stage 0
only observes), and a body is identified by a prefix, not its whole span.

With --all-ledgers (stage (b), VP i20735) it also takes every function start the ledgers enumerate
(docs/re/overlay_function_ledger.csv, docs/re/resident_callable_ledger.csv), so the observer can count uncredited bodies
too. The registry does not say which IDs are credited; that is read from strict_full_c_addresses.csv when the counts are
analysed. RPRO_ and RNEAR_ rows (near-only resident starts) carry only an image offset; their IP is inferred from the highest
resident segment base at or below it, so a wrong inference shows as no arrivals, never as a false one. Function starts
no ledger enumerates (for example near-only residents) are still unobservable.

Output (signatures are game bytes): resources/hybrid/registry.tsv, under the ignored resources/. Stdout: counts only."""
import argparse, collections, csv, re, struct, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/overlay'))
import overlay_inventory as OI                                     # noqa: E402
import res0899_quit_0000_cpu_audit as Q                           # noqa: E402
SIG = 32


def masked(code, start, length, fixups):
    sig = bytearray(code[start:start + length]); mask = bytearray(b'\xff' * len(sig))
    for f in fixups:
        for k in (f, f + 1):
            if start <= k < start + len(sig):
                sig[k - start] = 0; mask[k - start] = 0
    return bytes(sig), bytes(mask)


def occurrences(sig, mask, blobs, budget=1 << 26):
    """Count positions in all blobs where the masked signature matches. Budgeted: past 2**26 candidate positions it stops
    and returns None (reported, never treated as unique)."""
    head = next(k for k in range(len(sig)) if mask[k])
    n, scanned = 0, 0
    for blob in blobs:
        at = blob.find(sig[head:head + 1])
        while at != -1:
            scanned += 1
            if scanned > budget:
                return None
            s = at - head
            if 0 <= s and s + len(sig) <= len(blob) and all(not mask[k] or blob[s + k] == sig[k] for k in range(len(sig))):
                n += 1
            at = blob.find(sig[head:head + 1], at + 1)
    return n


def symbols():
    """ID -> C entry symbol, from the audits that register the row (CHECKER_ROWS) and bind a pr_* symbol via ctypes."""
    out = collections.defaultdict(set)
    for path in sorted((ROOT / 'tools/overlay').glob('*.py')) + sorted((ROOT / 'tests/recovered').glob('*.py')):
        text = path.read_text(errors='replace')
        rows = re.search(r'CHECKER_ROWS\s*=\s*\(([^)]*)\)', text)
        if not rows:
            continue
        syms = set(re.findall(r'CDLL\([^)]*\)\)?\.(pr_[A-Za-z0-9_]+)', text)) | set(re.findall(r"getattr\([^,]*,\s*'(pr_[A-Za-z0-9_]+)'", text))
        for row in re.findall(r"'([A-Z]+_[0-9A-F]{4}_[0-9A-F]{4})'", rows.group(1)):
            out[row] |= syms
    return out


def declared():
    """Every function name declared in include/poolrad/recovered (a name followed by '(' at a prototype)."""
    names = set()
    for h in (ROOT / 'include/poolrad/recovered').glob('*.h'):
        names |= set(re.findall(r'\b(pr_[A-Za-z0-9_]+)\s*\(', h.read_text(errors='replace')))
    return names


def doc_symbols(evidence, known):
    """Candidate C entry symbols named in a row's evidence doc, kept only when a header declares them."""
    path = ROOT / evidence
    if not path.exists():
        return set()
    return set(re.findall(r'\b(pr_[A-Za-z0-9_]+)', path.read_text(errors='replace'))) & known


def main():
    p = argparse.ArgumentParser(); p.add_argument('--out', type=Path, default=ROOT / 'resources/hybrid/registry.tsv')
    p.add_argument('--all-ledgers', action='store_true', help='add every ledger function start, credited or not')
    a = p.parse_args()
    exe = (ROOT / 'resources/DOS/POOLRAD/START.EXE').read_bytes(); ovr = (ROOT / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    packed = exe[OI.parse_mz(exe)['header_bytes']:]
    units = {u['stub_para']: u for u in OI.find_units(packed, ovr)}
    image, sites = Q.load_image()
    codes = [ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']] for u in units.values()]
    table = list(csv.DictReader(open(ROOT / 'docs/re/strict_full_c_addresses.csv')))
    ids = [r['id'] for r in table]; evidence = {r['id']: r['evidence_doc'] for r in table}
    if a.all_ledgers:
        more = [f"OVL_{int(r['unit'], 16):04X}_{int(r['offset'], 16):04X}"
                for r in csv.DictReader(open(ROOT / 'docs/re/overlay_function_ledger.csv'))]
        more += [r['id'] for r in csv.DictReader(open(ROOT / 'docs/re/resident_callable_ledger.csv'))]
        ids += sorted(set(more) - set(ids))
    syms, known = symbols(), declared(); rows = []; stats = collections.Counter()
    res_segs = sorted({0} | {int(i.split('_')[1], 16) for i in ids if i.startswith('RES_')})   # 0: main program segment
    for n, ident in enumerate(ids):
        if ident.startswith(('RPRO_', 'RNEAR_')):                    # near-only start: image offset only. Its IP is
            io = int(ident.split('_')[1], 16)                                  # inferred from the highest resident segment base
            seg = max(s for s in res_segs if s * 16 <= io)           # at or below it; a wrong guess can only miss
            kind, off = 'RES', io - seg * 16                          # arrivals (the signature is still checked)
        else:
            kind, seg, off = ident.split('_'); seg, off = int(seg, 16), int(off, 16)
        if kind == 'OVL':
            u = units.get(seg)
            if u is None:
                stats['unit_not_found'] += 1; rows.append((ident, 'UNIT_NOT_FOUND')); continue
            code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
            if off >= len(code):
                stats['offset_outside_unit'] += 1; rows.append((ident, 'OFFSET_OUTSIDE')); continue
            blob, start, fix = code, off, u['fixups']
        else:
            blob, start, fix = image, seg * 16 + off, sites
        for length in (SIG, 64, 128, 256):                            # grow an ambiguous prefix until it is unique
            sig, mask = masked(blob, start, min(length, len(blob) - start), fix)
            count = occurrences(sig, mask, codes + [image])
            if count == 1 or len(sig) < length:
                break
        stats[f'sig_len_{len(sig)}'] += 1
        unique = count == 1
        stats['unique' if unique else 'scan_budget' if count is None else 'ambiguous' if count else 'not_found_in_code'] += 1
        found = (syms.get(ident, set()) & known) or (doc_symbols(evidence[ident], known) if ident in evidence else set())
        stats['symbols_per_row_' + ('0' if not found else '1' if len(found) == 1 else 'many')] += 1
        sym = ','.join(sorted(found)) or '-'
        stats['symbol_found' if sym != '-' else 'symbol_not_found'] += 1
        rows.append((ident, f'{off:04X}', str(len(sig)), sig.hex(), mask.hex(), 'unique' if unique else f'count={count}', sym))
        if (n + 1) % 30 == 0:
            print(f'[{n + 1}/{len(ids)}] registry rows built', file=sys.stderr, flush=True)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    with a.out.open('w') as f:                                       # game bytes: ignored resources/ only
        f.write('# id\tentry_ip\tsig_len\tsig_hex\tmask_hex\tuniqueness\tc_symbol\n')
        for r in rows:
            f.write('\t'.join(r) + '\n')
    print(dict(ids=len(ids), **stats, out=str(a.out)))


if __name__ == '__main__':
    main()
