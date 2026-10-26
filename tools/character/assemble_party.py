#!/usr/bin/env python3
"""Assemble existing DOS character files into a new save-slot copy; no stat edits."""
import argparse
import json
from pathlib import Path
import re

from disk_save import DiskFile, import_directory, _export_files


def assemble_directory(source, destination, members, *, world='SAVGAMA.DAT', slot='B'):
    """Preserve character/sidecar bytes; change world count and occupied basenames."""
    source, imported = import_directory(Path(source))
    by_name = {item.name: item for item in imported}
    # Conservative tool bounds, not claims about every legal game-party makeup.
    if not 1 <= len(members) <= 8 or len(set(members)) != len(members):
        raise ValueError('select one to eight distinct character filenames')
    if not re.fullmatch('[A-J]', slot):
        raise ValueError('output slot must be one uppercase letter A through J')
    if world not in by_name or not world.startswith('SAVGAM') or not world.endswith('.DAT'):
        raise ValueError('base world file is absent or unsupported')
    for name in members:
        if name not in by_name or not name.endswith('.SAV'):
            raise ValueError('selected character file is absent or unsupported')
    files = []
    originals = {}
    base = by_name[world]
    world_bytes = bytearray(base.data)
    world_bytes[0x3208] = len(members)
    reasons = {0x3208: "party member count"}
    for index, member in enumerate(members, 1):
        output_stem = f'CHRDAT{slot}{index}'
        source_stem = member[:-4]
        for suffix in ('.SAV', '.ITM', '.SPC'):
            original = by_name.get(source_stem + suffix)
            if original is None:
                continue
            name = output_stem + suffix
            files.append(DiskFile(name, original.data))
            originals[name] = original
        # Only the length and occupied basename bytes change. Preserve all tail
        # bytes and unused slots; the loader consumes only the new count.
        position = 0x3209 + 41 * (index - 1)
        encoded = output_stem.encode('ascii')
        world_bytes[position:position + 1 + len(encoded)] = bytes([len(encoded)]) + encoded
        reasons[position] = f'occupied basename length for member {index}'
        for offset in range(position + 1, position + 1 + len(encoded)):
            reasons[offset] = f'occupied basename for member {index}'
    output_world = f'SAVGAM{slot}.DAT'
    files.append(DiskFile(output_world, bytes(world_bytes)))
    originals[output_world] = base
    report = _export_files(source, tuple(files), destination, originals, 'party_assembly',
                           {output_world: reasons})
    report.update(member_count=len(members), world_file=output_world,
                  game_validation='UNVERIFIED: this assembled party has not been loaded')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--members', nargs='+', required=True, help='ordered CHRDAT*.SAV filenames')
    parser.add_argument('--world', default='SAVGAMA.DAT')
    parser.add_argument('--slot', default='B')
    args = parser.parse_args()
    try:
        report = assemble_directory(args.source, args.destination, args.members,
                                    world=args.world, slot=args.slot)
    except (OSError, ValueError) as exc:
        parser.exit(1, 'assembly refused: ' + str(exc) + '\n')
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
