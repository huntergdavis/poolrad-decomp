#!/usr/bin/env python3
"""Record the exact executable selected for a hybrid run, conservatively."""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/overlay'))
from native_emulator_provenance import describe_dosbox


class MissingProvenance(ValueError):
    pass


def read_receipt(run):
    """Validate a completed launch receipt without querying a different binary."""
    try:
        identity = json.loads((run / 'emulator.json').read_text())
        if not isinstance(identity, dict):
            raise ValueError('emulator receipt must be an object')
        required = {'path', 'sha256', 'version', 'version_exit_code', 'pinned_source',
                    'pinned_build_used', 'launch_verified_sha256'}
        if not required <= identity.keys():
            raise ValueError('emulator receipt lacks required identity fields')
        if (not Path(identity['path']).is_absolute() or not isinstance(identity['version'], str) or not identity['version'] or
                type(identity['pinned_build_used']) is not bool or not identity['pinned_source'] or
                type(identity['version_exit_code']) is not int):
            raise ValueError('emulator receipt has incomplete identity fields')
        binary_hash = (run / 'r1-hashes.txt').read_text().split()[0]
        if identity['sha256'] != binary_hash or identity['launch_verified_sha256'] != binary_hash:
            raise ValueError('launch, receipt and recorded binary hashes differ')
        if identity['pinned_build_used'] and not identity.get('private_build'):
            raise ValueError('pinned binary claim lacks build provenance')
        return identity
    except (OSError, ValueError, KeyError, IndexError, TypeError) as error:
        raise MissingProvenance(str(error)) from error


def describe(binary, build_receipt=None):
    result = describe_dosbox(binary)
    if build_receipt is not None:
        build = json.loads(Path(build_receipt).read_text())
        if build.get('binary_sha256') != result['sha256']:
            raise ValueError('private build receipt names a different binary')
        if not build.get('source_commit') or not build.get('commands') or not build.get('compiler_version'):
            raise ValueError('private build receipt lacks source revision, compiler or commands')
        result['private_build'] = build
    # An instrumented local build is identified, not certified as a pinned binary.
    # In particular, a source revision alone never changes this field to true.
    assert result['pinned_build_used'] is False
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary')
    parser.add_argument('--build-receipt', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        result = describe(args.binary, args.build_receipt)
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print('emulator provenance CANNOT_RUN: ' + str(error), file=sys.stderr)
        return 4
    args.out.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
