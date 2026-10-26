#!/usr/bin/env python3
"""Lossless DOS save import/export and guarded copy writes. No game validation."""
import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re

CHARACTER_SIZE = 285
WORLD_SIZE = 13137
FILE_NAME = re.compile(r'(?:CHRDAT[A-Z0-9_]*\.(?:SAV|ITM|SPC)|SAVGAM[A-Z0-9_]*\.DAT)')


def digest(data):
    return hashlib.sha256(data).hexdigest()


@dataclass(frozen=True)
class DiskFile:
    """Own the complete serialized bytes; do not apply runtime-load fixups."""
    name: str
    data: bytes

    @classmethod
    def read(cls, path):
        name = path.name
        if not FILE_NAME.fullmatch(name):
            raise ValueError('unsupported save filename')
        data = path.read_bytes()
        suffix = path.suffix
        if suffix == '.SAV' and len(data) != CHARACTER_SIZE:
            raise ValueError('character record must be exactly 285 bytes')
        if suffix == '.DAT' and len(data) != WORLD_SIZE:
            raise ValueError('world frame must be exactly 13137 bytes')
        if suffix in ('.ITM', '.SPC') and len(data) % {'.ITM': 63, '.SPC': 9}[suffix]:
            raise ValueError('sidecar has a partial record')
        return cls(name, data)

    def serialize(self):
        """Return the entire disk frame, including unknown and stale fields."""
        return self.data


def import_directory(source):
    source = source.resolve(strict=True)
    if not source.is_dir():
        raise ValueError('source must be a directory')
    paths = sorted(p for p in source.iterdir() if FILE_NAME.fullmatch(p.name))
    files = tuple(DiskFile.read(path) for path in paths)
    names = {item.name for item in files}
    if not any(name.endswith('.SAV') for name in names):
        raise ValueError('no character records found')
    for item in files:
        if item.name.endswith(('.ITM', '.SPC')) and item.name[:-4] + '.SAV' not in names:
            raise ValueError('sidecar has no matching character record')
        if item.name.endswith('.DAT'):
            count = item.data[0x3208]
            if count > 8:
                raise ValueError('world character count exceeds eight slots')
            for index in range(count):
                start = 0x3209 + 41 * index
                length = item.data[start]
                if not 0 < length <= 40:
                    raise ValueError('invalid occupied world basename length')
                name = item.data[start + 1:start + 1 + length].decode('ascii') + '.SAV'
                if name not in names:
                    raise ValueError('world references an absent character record')
    return source, files


def export_directory(source, destination):
    """No-op export only. Refuse existing destinations and source descendants."""
    source, files = import_directory(Path(source))
    return _export_files(source, files, destination,
                         {item.name: item for item in files}, 'no_op_export')


def _export_files(source, files, destination, originals, operation, change_reasons=None):
    """Shared exclusive writer for trusted, fully planned fixed-frame changes."""
    names = [item.name for item in files]
    if len(set(names)) != len(names) or set(names) != set(originals):
        raise ValueError('duplicate output name or missing original snapshot')
    for item in files:
        if not FILE_NAME.fullmatch(item.name):
            raise ValueError('unsupported output filename')
        if len(item.data) != len(originals[item.name].data):
            raise ValueError('fixed-frame write cannot change file length')
    destination = Path(destination)
    # lexists semantics: an existing broken symlink is also an occupied target.
    if destination.exists() or destination.is_symlink():
        raise ValueError('destination already exists; choose a new copy directory')
    destination = destination.resolve()
    if destination == source or source in destination.parents:
        raise ValueError('destination must be outside the original source directory')
    if not destination.parent.is_dir():
        raise ValueError('destination parent must already exist')
    receipts = []
    created = []
    destination.mkdir(mode=0o700)
    try:
        for item in files:
            target = destination / item.name
            serialized = item.serialize()
            # Exclusive creation prevents replacing any pre-existing file.
            with target.open('xb') as stream:
                created.append(target)
                stream.write(serialized)
            reread = target.read_bytes()
            if reread != item.data:
                raise OSError('output reread differs from imported bytes')
            original = originals[item.name]
            changes = [{'offset': index, 'before': f'{before:02x}',
                        'after': f'{after:02x}'}
                       for index, (before, after) in enumerate(zip(original.data, reread))
                       if before != after]
            if changes:
                reasons = (change_reasons or {}).get(item.name, {})
                if any(change['offset'] not in reasons for change in changes):
                    raise ValueError('every changed byte requires an explicit reason')
                for change in changes:
                    change['reason'] = reasons[change['offset']]
            receipts.append({
                'file': str(target), 'source': str(source / original.name),
                'operation': operation, 'size': len(reread),
                'before_sha256': digest(original.data), 'after_sha256': digest(reread),
                'changed_offsets': [change['offset'] for change in changes],
                'before_after_bytes': changes,
                'byte_identical': not changes, 'game_build_loaded': None,
                'game_validation': 'UNVERIFIED: not loaded by the game',
            })
        # A changed source invalidates this proof; never silently claim success.
        for item in originals.values():
            if (source / item.name).read_bytes() != item.data:
                raise OSError('source changed during export; no stable round-trip proof')
        receipt_path = destination / 'write-receipts.json'
        with receipt_path.open('x') as stream:
            created.append(receipt_path)
            json.dump({'schema_version': 1, 'operation': operation,
                       'files': receipts, 'game_build_loaded': None}, stream, indent=2)
            stream.write('\n')
    except BaseException:
        # Roll back only paths created by this invocation, never the source.
        for path in reversed(created):
            path.unlink()
        destination.rmdir()
        raise
    return {'output_copy': str(destination), 'files_written': len(files),
            'bytes_written': sum(len(item.data) for item in files),
            'byte_identical_files': sum(r['byte_identical'] for r in receipts),
            'receipt': str(receipt_path), 'game_validation': 'UNVERIFIED'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    try:
        report = export_directory(args.source, args.destination)
    except (OSError, ValueError) as exc:
        parser.exit(1, 'export refused: ' + str(exc) + '\n')
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
