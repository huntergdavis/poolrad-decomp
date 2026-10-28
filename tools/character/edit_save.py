#!/usr/bin/env python3
"""Inspect DOS save copies and apply two historically observed reference edits.

Local JSON stdin/stdout adapter, not an HTTP endpoint or general advancement engine.
The original no-op disk_save.py interface remains unchanged.
"""
import json
from pathlib import Path
import sys

from disk_save import DiskFile, FILE_NAME, digest, import_directory

PROFILES = {
    'reference_strength_17_to_16': {
        'field': 'strength', 'offset': 0x10, 'before': 17, 'after': 16,
        'before_sha256': 'd7ba1321e1fca8464d1220e77c26ce92173d60cb67b0f155c71868926cfa6c8e',
        'after_sha256': 'e29e16f7dfc4abb10ca25fc701166bab71f24e402276390fd6e202d8db5ccf5a',
        'evidence': 'docs/formats/DOS_CHARACTER_DISPLAY_FIELDS.md',
        'scope': 'One reference record: STR 17 to 16; observed THAC0 19 to 20. General ability ranges and combat effects UNVERIFIED.',
    },
    'reference_fighter_level_1_to_2': {
        'field': 'level', 'offset': 0x98, 'before': 1, 'after': 2,
        'before_sha256': 'e29e16f7dfc4abb10ca25fc701166bab71f24e402276390fd6e202d8db5ccf5a',
        'after_sha256': '92d1129e9b37bb26bd705ad6602a5eafad7d9e7aa9d86c8ce3e5559e8deefe6f',
        'evidence': 'docs/formats/DOS_CHARACTER_LEVEL_EDIT.md',
        'scope': 'One reference fighter: level slot 0x98 from 1 to 2; observed THAC0 20 to 19. XP/HP/spells are not recalculated; this is not coherent advancement.',
    },
}
VALIDATION_SCOPE = {
    'game_validation': 'UNVERIFIED', 'game_build_loaded': None,
    'historical_reference_only': True,
    'limits': 'Exact reference records/transitions only. No general legal ranges, class mapping, advancement, party assembly, spell editing, or new game-load proof.',
}


class Refused(ValueError):
    def __init__(self, code, message):
        super().__init__(message)
        self.code = code


def require(condition, code, message):
    if not condition:
        raise Refused(code, message)


def keys(value, required, label):
    require(type(value) is dict and set(value) == set(required),
            'invalid_request', label + ' has missing or unknown keys')


def path_value(value, label):
    require(type(value) is str and bool(value), 'invalid_request', label + ' must be a nonempty path string')
    return Path(value)


def source_snapshot(source):
    source, files = import_directory(source)
    require(not any((source / f.name).is_symlink() for f in files),
            'invalid_source', 'save files must be regular copies, not symlinks')
    return source, files


def inspect_files(files):
    records = []
    known = {p[k] for p in PROFILES.values() for k in ('before_sha256', 'after_sha256')}
    for item in files:
        if not item.name.endswith('.SAV'):
            continue
        sha = digest(item.data)
        profiles = []
        for name, profile in PROFILES.items():
            eligible = sha == profile['before_sha256']
            profiles.append({
                'validation_profile': name, 'field': profile['field'],
                'expected_before': profile['before'], 'value': profile['after'],
                'eligible': eligible,
                'reason_unavailable': None if eligible else 'Whole record does not match this profile input; value alone is insufficient.',
                'proof_scope': profile['scope'], 'evidence': profile['evidence'],
                'eligible_after_application': [n for n, p in PROFILES.items()
                                               if p['before_sha256'] == profile['after_sha256']],
            })
        records.append({
            'file': item.name, 'sha256': sha,
            'current_values': dict(zip(('strength', 'intelligence', 'wisdom', 'dexterity', 'constitution', 'charisma'), item.data[0x10:0x16])),
            'level_slots': list(item.data[0x96:0x9e]),
            'reference_fighter_level': item.data[0x98] if sha in known else None,
            'profiles': profiles,
            'unsupported_fields': 'Other stats, abilities, HP, spells, class/level mappings and party composition remain UNVERIFIED and cannot be edited.',
        })
    return records


def validate_changes(files, changes):
    require(type(changes) is list and bool(changes), 'invalid_request', 'changes must be a nonempty ordered array')
    originals = {f.name: f.data for f in files}
    edited = dict(originals)
    seen = set()
    applied = []
    for index, change in enumerate(changes):
        keys(change, ('file', 'field', 'expected_before', 'value', 'validation_profile'), 'change')
        name, field, profile_name = change['file'], change['field'], change['validation_profile']
        require(all(type(v) is str for v in (name, field, profile_name)), 'invalid_request', 'file, field and validation_profile must be strings')
        require(name in edited and name.endswith('.SAV') and FILE_NAME.fullmatch(name), 'unsupported_file', 'change must name an imported character record')
        require(profile_name in PROFILES, 'unsupported_profile', 'unknown validation profile')
        profile = PROFILES[profile_name]
        require(field == profile['field'], 'unsupported_field', 'field does not match the validation profile')
        require((name, field) not in seen, 'duplicate_change', 'a file/field may be changed only once')
        seen.add((name, field))
        require(type(change['expected_before']) is int and type(change['value']) is int,
                'invalid_request', 'before/value must be integers, not booleans')
        require((change['expected_before'], change['value']) == (profile['before'], profile['after']),
                'unsupported_value', 'only the exact reference transition is supported')
        before = edited[name]
        require(digest(before) == profile['before_sha256'] and before[profile['offset']] == profile['before'],
                'profile_mismatch', 'whole intermediate record does not match profile input at change ' + str(index))
        data = bytearray(before)
        data[profile['offset']] = profile['after']
        after = bytes(data)
        require(digest(after) == profile['after_sha256'], 'profile_mismatch', 'edited record does not match profile output')
        edited[name] = after
        applied.append({**change, 'index': index, 'offset': profile['offset'],
                        'before_sha256': digest(before), 'after_sha256': digest(after),
                        'historical_evidence': profile['evidence'], 'proof_scope': profile['scope']})
    return tuple(DiskFile(f.name, edited[f.name]) for f in files), applied


def write_copy(source, originals, edited, destination, applied):
    require(not destination.exists() and not destination.is_symlink(), 'invalid_destination', 'destination already exists')
    destination = destination.resolve()
    require(destination != source and source not in destination.parents, 'invalid_destination', 'destination must be outside source')
    require(destination.parent.is_dir(), 'invalid_destination', 'destination parent must already exist')
    created = []
    receipts = []
    destination.mkdir(mode=0o700)
    try:
        for before, after in zip(originals, edited):
            target = destination / after.name
            with target.open('xb') as stream:
                created.append(target)
                stream.write(after.data)
            require(target.read_bytes() == after.data, 'reread_failed', 'output reread mismatch')
            offsets = [i for i, (a, b) in enumerate(zip(before.data, after.data)) if a != b]
            receipts.append({
                'file': str(target), 'source': str(source / before.name),
                'operation': 'edit_copy', 'size': len(after.data),
                'before_sha256': digest(before.data), 'after_sha256': digest(after.data),
                'changed_offsets': offsets,
                'before_after_bytes': [{'offset': i, 'before': before.data[i], 'after': after.data[i]} for i in offsets],
                'byte_identical': not offsets, 'game_build_loaded': None, 'game_validation': 'UNVERIFIED',
            })
        # Re-import also detects added/removed save files and changed references.
        _, current = source_snapshot(source)
        require(current == originals, 'source_changed', 'source changed during export')
        receipt_path = destination / 'write-receipts.json'
        receipt = {'schema_version': 1, 'operation': 'edit_copy', 'files': receipts,
                   'applied_changes': applied, 'rejected_changes': [], **VALIDATION_SCOPE}
        with receipt_path.open('x') as stream:
            created.append(receipt_path)
            json.dump(receipt, stream, indent=2)
            stream.write('\n')
    except BaseException:
        for path in reversed(created):
            path.unlink()
        destination.rmdir()
        raise
    return {'schema_version': 1, 'operation': 'edit_copy', 'output_copy': str(destination),
            'files_written': len(edited), 'bytes_written': sum(len(f.data) for f in edited),
            'byte_identical_files': sum(r['byte_identical'] for r in receipts),
            'receipt': str(receipt_path), 'applied_changes': applied, 'rejected_changes': [],
            'validation_scope': VALIDATION_SCOPE, 'game_validation': 'UNVERIFIED',
            'game_build_loaded': None, 'characters': inspect_files(edited)}


def process(request):
    require(type(request) is dict, 'invalid_request', 'request must be an object')
    require(type(request.get('schema_version')) is int and request['schema_version'] == 1,
            'invalid_request', 'schema_version must be integer 1')
    operation = request.get('operation')
    require(operation in ('inspect', 'edit_copy'), 'invalid_request', 'operation must be inspect or edit_copy')
    fields = ['schema_version', 'operation', 'source']
    if operation == 'edit_copy':
        fields += ['destination', 'expected_source_hashes', 'changes']
    keys(request, fields, 'request')
    source, files = source_snapshot(path_value(request['source'], 'source'))
    hashes = {f.name: digest(f.data) for f in files}
    if operation == 'inspect':
        return {'schema_version': 1, 'operation': 'inspect', 'expected_source_hashes': hashes,
                'characters': inspect_files(files), 'validation_scope': VALIDATION_SCOPE, 'writes': 0}
    require(type(request['expected_source_hashes']) is dict and request['expected_source_hashes'] == hashes,
            'source_mismatch', 'expected_source_hashes must match every imported save file, with no missing or extra names')
    destination = path_value(request['destination'], 'destination')
    edited, applied = validate_changes(files, request['changes'])
    return write_copy(source, files, edited, destination, applied)


def unique_keys(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'invalid_request', 'duplicate JSON key: ' + key)
        result[key] = value
    return result


def main():
    request = None
    try:
        require(len(sys.argv) == 1, 'invalid_request', 'pass one JSON request on stdin; no positional arguments')
        request = json.load(sys.stdin, object_pairs_hook=unique_keys)
        result = process(request)
    except (OSError, ValueError, TypeError) as exc:
        changes = request.get('changes', []) if type(request) is dict else []
        error = {'schema_version': 1, 'error': {'code': getattr(exc, 'code', 'invalid_source_or_request'), 'message': str(exc)},
                 'applied_changes': [], 'rejected_changes': changes}
        print(json.dumps(error, sort_keys=True), file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
