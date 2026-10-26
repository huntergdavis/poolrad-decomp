#!/usr/bin/env python3
"""Read-only DOS v1.3 frame/coverage audit; prints no save contents or names."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LAYOUT = ROOT / 'docs/formats/DOS_CHARACTER_EDITOR_LAYOUT.csv'
HASHES = {
    'START.EXE': 'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd',
    'GAME.OVR': '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614',
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(layout, media):
    with layout.open(newline='') as stream:
        rows = list(csv.DictReader(stream))
    cursor = 0
    for row in rows:
        offset, width = int(row['offset'], 0), int(row['width'], 0)
        require(offset == cursor and width > 0, 'layout gap, overlap, or empty range')
        require(row['status'] in ('VERIFIED', 'UNVERIFIED'), 'invalid field status')
        require(all(row.values()), 'empty layout cell')
        cursor += width
    require(cursor == 285, 'layout must cover exactly 285 bytes')
    result = {'layout_ranges': len(rows), 'covered_bytes': cursor,
              'unverified_ranges': sum(r['status'] == 'UNVERIFIED' for r in rows),
              'game_loaded': False, 'writes': 0}
    if media is None:
        return result
    for name, expected in HASHES.items():
        require(hashlib.sha256((media / name).read_bytes()).hexdigest() == expected,
                'executable hash mismatch: ' + name)
    records = sorted(media.glob('CHRDAT*.SAV'))
    worlds = sorted(media.glob('SAVGAM*.DAT'))
    require(records and worlds, 'character and world files are required')
    name_lengths = []
    for path in records:
        data = path.read_bytes()
        require(len(data) == 285, 'character frame size mismatch')
        require(data[0] <= 15, 'name exceeds conservative reader capacity')
        name_lengths.append(data[0])
    sidecars = {}
    for suffix, unit in (('ITM', 63), ('SPC', 9)):
        paths = sorted(media.glob('CHRDAT*.' + suffix))
        for path in paths:
            require(path.stat().st_size % unit == 0, 'sidecar size is not a framing multiple')
            require(path.with_suffix('.SAV').is_file(), 'sidecar lacks character record')
        sidecars[suffix] = {'files': len(paths), 'bytes': sum(p.stat().st_size for p in paths)}
    referenced = 0
    for path in worlds:
        data = path.read_bytes()
        require(len(data) == 13137, 'world frame size mismatch')
        count = data[0x3208]
        require(count <= 8, 'world count exceeds eight basename slots')
        for index in range(count):
            slot = data[0x3209 + 41 * index:0x3209 + 41 * (index + 1)]
            require(0 < slot[0] <= 40, 'invalid occupied basename length')
            stem = slot[1:1 + slot[0]].decode('ascii')
            require(all(c.isalnum() or c == '_' for c in stem), 'unsupported basename spelling')
            require((media / (stem + '.SAV')).is_file(), 'world reference lacks character file')
            referenced += 1
    result.update(executable_hashes=HASHES, character_files=len(records),
                  character_bytes=sum(p.stat().st_size for p in records),
                  name_length_min=min(name_lengths), name_length_max=max(name_lengths),
                  world_files=len(worlds), world_character_references=referenced,
                  sidecars=sidecars)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--layout', type=Path, default=LAYOUT)
    parser.add_argument('--media', type=Path, help='private DOS directory; omitted means coverage only')
    args = parser.parse_args()
    try:
        result = audit(args.layout, args.media)
    except (OSError, ValueError, KeyError) as exc:
        parser.exit(1, 'audit failed: ' + str(exc) + '\n')
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
