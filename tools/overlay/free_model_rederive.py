#!/usr/bin/env python3
"""Workstream 1 of the free-model program: static independent re-derivation.

For a credited OVL_ body, build a self-contained prompt (disassembly of the
original GAME.OVR span, the recovered C, the evidence doc) and ask a free
model for AGREE or DISAGREE with a specific reason.  The model cannot run
anything, so this is a static check for doc-vs-bytes drift.  A DISAGREE is a
lead for a reviewer, not a verdict.  Output is one JSON record per body under
the output directory; failures (endpoint error, no text, bad verdict line)
are recorded as failures, never dropped.

Usage: free_model_rederive.py --out DIR --model opencode/MODEL ID [ID ...]
       free_model_rederive.py --prompt-only ID     (print the prompt)
"""
import argparse, csv, json, re, subprocess, sys, time
from pathlib import Path

import capstone

ROOT = Path(__file__).resolve().parents[2]
OVR = ROOT / 'resources/DOS/POOLRAD/GAME.OVR'
DOC_LIMIT = 14000
SRC_LIMIT = 16000
VERDICT = re.compile(r'^VERDICT:\s*(AGREE|DISAGREE)\b(.*)$', re.M)


def ledger_row(body_id):
    m = re.fullmatch(r'OVL_([0-9A-F]{4})_([0-9A-F]{4})', body_id)
    if not m:
        raise SystemExit(f'{body_id}: only OVL_UUUU_OOOO ids are supported')
    key = f'0x{int(m.group(1), 16):x}:0x{m.group(2).lower()}'
    for r in csv.DictReader(open(ROOT / 'docs/re/overlay_function_ledger.csv')):
        if r['id'] == key:
            return r
    raise SystemExit(f'{body_id}: no ledger row {key}')


def evidence_doc(body_id):
    for r in csv.DictReader(open(ROOT / 'docs/re/strict_full_c_addresses.csv')):
        if r['id'] == body_id:
            return r['evidence_doc']
    raise SystemExit(f'{body_id}: not in strict_full_c_addresses.csv')


def disassemble(row):
    off, size = int(row['ovr_offset']), int(row['bound'])
    data = OVR.read_bytes()[off:off + size]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_16)
    base = int(row['offset'], 16)
    lines = [f'{i.address:04X}: {i.bytes.hex():<14} {i.mnemonic} {i.op_str}'.rstrip()
             for i in md.disasm(data, base)]
    return data, lines


def seed_fault(src):
    """Control: change the first 4-digit hex constant in the C by +2."""
    m = re.search(r'0x([0-9A-Fa-f]{4})(?=u?\b)', src)
    if not m:
        return src, None
    new = f'0x{(int(m.group(1), 16) + 2) & 0xFFFF:04X}'
    return src[:m.start()] + new + src[m.end():], f'{m.group(0)} -> {new}'


def build_prompt(body_id, seed=False):
    row = ledger_row(body_id)
    doc_path = evidence_doc(body_id)
    doc = (ROOT / doc_path).read_text()[:DOC_LIMIT]
    srcs = sorted(set(re.findall(r'src/recovered/[A-Za-z0-9_]+\.c', doc)))
    own = [s for s in srcs if body_id[4:8].lower() in s and body_id[9:].lower() in s]
    src_path = (own or srcs[:1] or [None])[0]
    src = (ROOT / src_path).read_text()[:SRC_LIMIT] if src_path else '(no source named in the doc)'
    seeded = None
    if seed:
        src, seeded = seed_fault(src)
    data, lines = disassemble(row)
    return {
        'seeded': seeded,
        'id': body_id, 'doc': doc_path, 'source': src_path,
        'bytes': len(data), 'prompt': f"""You are a static reviewer of a reverse-engineered 16-bit DOS function.
You cannot run anything. Compare THREE things and report whether they agree:
 (A) the disassembly of the ORIGINAL bytes (authoritative),
 (B) the recovered C model,
 (C) the evidence document's claims (addresses, sizes, branch conditions, constants, callers).

Check specifically: every conditional jump's condition (JA/JBE/JL/JGE use different flags),
byte vs word stores, constants and offsets quoted in the doc against the bytes, the span size,
and any call target named. Do not guess: if you cannot verify a claim from the text given, say UNVERIFIED.

Reply with at most 12 lines, ending with EXACTLY one line of the form
VERDICT: AGREE
or
VERDICT: DISAGREE <one specific claim, with the address and what the bytes show>

=== (A) ORIGINAL BYTES, {body_id}, {len(data)} bytes, entry offset {row['offset']} ===
{chr(10).join(lines)}

=== (B) RECOVERED C ({src_path}) ===
{src}

=== (C) EVIDENCE DOC ({doc_path}) ===
{doc}
"""}


def run_model(model, prompt, timeout):
    t0 = time.time()
    try:
        p = subprocess.run(['nice', '-n', '15', 'opencode', 'run', '--pure', '-m', model, '--format', 'json', prompt],
                           capture_output=True, text=True, timeout=timeout, cwd='/tmp')
    except subprocess.TimeoutExpired:
        return {'status': 'FAIL_TIMEOUT', 'secs': round(time.time() - t0)}
    text, err = [], None
    for line in p.stdout.splitlines():
        try:
            ev = json.loads(line)
        except ValueError:
            continue
        if ev.get('type') == 'text':
            text.append(ev.get('part', {}).get('text', ''))
        if ev.get('type') == 'error':
            err = json.dumps(ev.get('error'))[:300]
    out = {'secs': round(time.time() - t0), 'rc': p.returncode, 'text': ''.join(text)}
    if err:
        out['status'], out['error'] = 'FAIL_ENDPOINT', err
    elif not out['text'].strip():
        out['status'], out['stderr'] = 'FAIL_NO_TEXT', p.stderr[-300:]
    else:
        v = VERDICT.findall(out['text'])
        out['status'] = 'FAIL_NO_VERDICT' if not v else 'OK'
        if v:
            out['verdict'], out['reason'] = v[-1][0], v[-1][1].strip()
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('ids', nargs='+')
    ap.add_argument('--out')
    ap.add_argument('--model')
    ap.add_argument('--seed', action='store_true', help='control: seed one fault in the C')
    ap.add_argument('--prompt-only', action='store_true')
    ap.add_argument('--timeout', type=int, default=600)
    a = ap.parse_args()
    if a.prompt_only:
        print(build_prompt(a.ids[0])['prompt'])
        return
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    for i, body_id in enumerate(a.ids, 1):
        print(f'START {i}/{len(a.ids)} {body_id} {a.model}', flush=True)
        b = build_prompt(body_id, a.seed)
        rec = {k: b[k] for k in ('id', 'doc', 'source', 'bytes', 'seeded')}
        rec['model'] = a.model
        rec.update(run_model(a.model, b['prompt'], a.timeout))
        (out / f'{body_id}.{a.model.split("/")[-1]}{".seed" if a.seed else ""}.json').write_text(json.dumps(rec, indent=1))
        print(f'DONE {body_id} {rec["status"]} {rec.get("verdict", "")} {rec["secs"]}s', flush=True)


if __name__ == '__main__':
    main()
