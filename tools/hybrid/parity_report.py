#!/usr/bin/env python3
"""Compare t116 checkpoint captures without masking any captured state.

Only hashes, counts, addresses and register values enter the public receipt.
Raw media, frames and memory remain in the private run directories.
"""
import argparse
import hashlib
import json
from pathlib import Path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def byte_diff(left, right):
    if len(left) != len(right):
        raise ValueError('capture sizes differ')
    offsets = [i for i, (a, b) in enumerate(zip(left, right)) if a != b]
    return dict(bytes=len(left), differing_bytes=len(offsets),
                first_offsets=offsets[:32], original_sha256=digest(left),
                replacement_sha256=digest(right))


def fields_diff(left, right):
    if left.keys() != right.keys():
        raise ValueError('capture field sets differ')
    return {k: {'original': left[k], 'replacement': right[k]}
            for k in left if left[k] != right[k]}


def read_capture(run):
    data = json.loads((run / 'checkpoint.json').read_text())
    required = {'eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp',
                'eip', 'cs', 'ds', 'es', 'ss', 'fs', 'gs', 'flags'}
    if data['schema'] != 1 or data['registers'].keys() != required:
        raise ValueError('incomplete register capture')
    blobs = {name: (run / ('checkpoint.' + name)).read_bytes()
             for name in ('ram', 'vram', 'rgb')}
    frame = data['frame']
    if (len(blobs['ram']) != 1 << 20 or
            len(blobs['vram']) != data['video']['vmemsize'] or
            len(blobs['rgb']) != frame['width'] * frame['height'] * 3 or
            frame['format'] != 'RGB24' or digest(blobs['rgb']) != frame['sha256']):
        raise ValueError('truncated or corrupt capture')
    return data, blobs


def report(original, replacement):
    if ((original / 'parity-mode.txt').read_text().strip() != 'original' or
            (replacement / 'parity-mode.txt').read_text().strip() != 'replacement'):
        raise ValueError('expected an original/replacement pair')
    inputs = [(run / '.oracle-input-sha256').read_bytes() for run in (original, replacement)]
    if inputs[0] != inputs[1]:
        raise ValueError('input/save manifests differ')
    builds = [(run / 'parity-build-sha256').read_bytes() for run in (original, replacement)]
    if builds[0] != builds[1]:
        raise ValueError('build manifests differ')
    a, ab = read_capture(original)
    b, bb = read_capture(replacement)
    registries = [(run / 'registry.tsv').read_bytes() for run in (original, replacement)]
    if registries[0] != registries[1]:
        raise ValueError('capture registries differ')
    rows = [line.split('\t') for line in registries[0].decode().splitlines()
            if line.split('\t')[0] == a['id']]
    if len(rows) != 1 or len(rows[0]) < 6 or rows[0][5] != 'unique':
        raise ValueError('capture target is not a unique registry row')
    row = rows[0]
    signature, mask = bytes.fromhex(row[3]), bytes.fromhex(row[4])
    if not signature or len(signature) != len(mask) or not any(mask):
        raise ValueError('capture signature is empty or invalid')
    if a['registers']['eip'] != int(row[1], 16):
        raise ValueError('capture IP differs from signature registry')
    if (a['id'], a['case'], a['registers']['eip']) != (b['id'], b['case'], b['registers']['eip']):
        raise ValueError('capture arrivals/cases differ')
    if any(a['frame'][k] != b['frame'][k] for k in ('width', 'height', 'format')):
        raise ValueError('frame geometry differs')
    diffs = {name: byte_diff(ab[name], bb[name]) for name in ('ram', 'vram', 'rgb')}
    diffs['rgb']['differing_pixels'] = sum(ab['rgb'][i:i+3] != bb['rgb'][i:i+3]
                                         for i in range(0, len(ab['rgb']), 3))
    regs = fields_diff(a['registers'], b['registers'])
    video = fields_diff(a['video'], b['video'])
    equal = not regs and not video and all(d['differing_bytes'] == 0 for d in diffs.values())
    save_files = [line for line in inputs[0].decode().splitlines()
                  if 'SAVGAMA.DAT' in line or 'CHRDATA' in line]
    dispatch = (replacement / 'parity-dispatch.txt').read_text().strip()
    if not dispatch:
        raise ValueError('replacement dispatch list is empty')
    routed = b['routed_returns']
    if a['routed_returns'] != 0 or a['routed_stops'] != 0:
        raise ValueError('original capture contains routed C execution')
    if routed <= 0:
        raise ValueError('no measured C route returns in replacement run')
    return dict(schema=1, case=a['case'], result='PASS' if equal else 'FAIL',
                input_manifest_sha256=digest(inputs[0]), save_state_id=digest('\n'.join(save_files).encode()),
                save_files=save_files, build_manifest=builds[0].decode().splitlines(),
                arrival=dict(id=a['id'], instruction_boundary='entry before guest instruction',
                             registry_sha256=digest(registries[0]),
                             signature_sha256=digest(signature), signature_bytes=len(signature),
                             checked_bytes=sum(bool(n) for n in mask),
                             mismatch_behavior='no matching callback; checkpoint times out without receipt',
                             original_count=a['arrival'], replacement_count=b['arrival'],
                             original_cs_ip=[a['registers']['cs'], a['registers']['eip']],
                             replacement_cs_ip=[b['registers']['cs'], b['registers']['eip']]),
                frame=dict(width=a['frame']['width'], height=a['frame']['height'], **diffs.pop('rgb')),
                state=dict(register_differences=regs, video_metadata_differences=video, **diffs),
                replacement_routed_returns=routed, replacement_routed_stops=b['routed_stops'],
                dispatch_ids=dispatch.split(','),
                commands=[(r / 'parity-command.txt').read_text().strip() for r in (original, replacement)],
                not_verified=[
                    'Identical complete machine state at boot: these are independent boots of identical slot-A files.',
                    'Same polling ordinal or emulated time: capture is the first matching poll after the driver arms it.',
                    'A newly completed frame at the checkpoint: RGB is the last presented window, captured while stopped.',
                    'Full machine state: backing RAM and VGA memory plus listed registers/metadata omit device internals, palette, PIC, DOS handles and timers.',
                    'Whole-game C replacement: non-dispatched functions and service replay still execute original code.',
                    'Universal novelty: turn is a newly collected route fixture, not proof that nobody previously executed it.',
                    'Other saves, combat, timing parity, full-game parity, or replacement credit.'
                ])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('original', type=Path)
    parser.add_argument('replacement', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = report(args.original, args.replacement)
    text = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.write_text(text)
    else:
        print(text, end='')
    return 0 if result['result'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
