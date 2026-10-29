#!/usr/bin/env python3
"""Check t356 metadata against pinned media; unresolved spans never pass.

Exit 0: all recorded spans verified; 1: malformed/mismatched evidence;
2: resolved spans verified, but at least one span remains suspended.
Only addresses, lengths, hashes and errors are emitted, never original bytes.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
MEDIA_SHA256 = {
    'START.EXE': 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd',
    'GAME.OVR': '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614',
}
MARKER = re.compile(r'\[scrubbed: span=(\S+) sha256=([0-9a-f]{64}) bytes=(\d+) offset=(\S+)\]')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify(root, rows, media, expected_hashes):
    errors, verified, suspended = [], [], []
    markers = {}
    for path in sorted((root / 'docs/re').glob('*.md')):
        text = path.read_text()
        found = list(MARKER.finditer(text))
        if text.count('[scrubbed:') != len(found):
            errors.append(f'malformed marker: {path.relative_to(root)}')
        for match in found:
            span, digest, length, offset = match.groups()
            if span in markers:
                errors.append(f'duplicate marker: {span}')
            markers[span] = (str(path.relative_to(root)), digest, int(length), offset)
    ids = [r['span'] for r in rows]
    if not rows or len(set(ids)) != len(ids):
        errors.append('empty manifest or duplicate span IDs')
    if set(ids) != set(markers):
        errors.append('manifest/marker membership differs')
    for name, expected in expected_hashes.items():
        if name not in media or sha(media[name]) != expected:
            errors.append(f'unsupported or missing media: {name}')
    if errors:
        return dict(status='ERROR', errors=errors, verified=[], suspended=[])
    for row in rows:
        span = row['span']
        try:
            length = int(row['length'])
            if length <= 0 or not re.fullmatch('[0-9a-f]{64}', row['sha256']):
                raise ValueError('invalid length or digest')
            expected_offset = 'UNRESOLVED'
            if row['status'] == 'resolved':
                name = row['media']
                if name not in expected_hashes:
                    raise ValueError('unrecognized media')
                offset = int(row['file_offset'], 0)
                if offset < 0 or offset + length > len(media[name]):
                    raise ValueError('span outside file')
                expected_offset = name + ':' + row['file_offset']
                if sha(media[name][offset:offset + length]) != row['sha256']:
                    raise ValueError('span digest mismatch')
            elif row['status'] == 'unresolved':
                if row['media'] or row['file_offset'] or not row['attribution']:
                    raise ValueError('unresolved row needs empty location and a reason')
            else:
                raise ValueError('unknown status')
            if markers[span] != (row['doc'], row['sha256'], length, expected_offset):
                raise ValueError('document/manifest metadata differs')
            if row['status'] == 'resolved':
                verified.append(dict(span=span, media=row['media'],
                                     file_offset=row['file_offset'], length=length,
                                     sha256=row['sha256']))
            else:
                suspended.append(dict(span=span, reason=row['attribution']))
        except (KeyError, ValueError) as exc:
            errors.append(f'{span}: {exc}')
    status = 'ERROR' if errors else 'SUSPENDED' if suspended else 'PASS'
    return dict(status=status, errors=errors, verified=verified, suspended=suspended)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--media-dir', type=Path, required=True)
    args = parser.parse_args()
    manifest = args.manifest or args.root / 'docs/re/scrubbed_span_offsets.csv'
    try:
        with manifest.open() as stream:
            rows = list(csv.DictReader(stream))
        media = {name: (args.media_dir / name).read_bytes() for name in MEDIA_SHA256}
        result = verify(args.root, rows, media, MEDIA_SHA256)
        result['manifest_sha256'] = sha(manifest.read_bytes())
        result['media_sha256'] = {name: sha(data) for name, data in media.items()}
    except (OSError, ValueError, KeyError, csv.Error) as exc:
        result = dict(status='ERROR', errors=[str(exc)], verified=[], suspended=[])
    print(json.dumps(result, indent=2, sort_keys=True))
    return {'PASS': 0, 'ERROR': 1, 'SUSPENDED': 2}[result['status']]


if __name__ == '__main__':
    raise SystemExit(main())
