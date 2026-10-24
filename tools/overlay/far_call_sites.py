#!/usr/bin/env python3
"""List every fixup-backed far call to one overlay stub entry, across all GAME.OVR units.

Usage: far_call_sites.py SEG:OFF [--exe START.EXE] [--ovr GAME.OVR]

SEG:OFF is the call target as the original code writes it, e.g. 008F:002F for unit 0x7F's entry
slot 3 (+063A). A site counts when the unit's code has `9A off_lo off_hi seg_lo seg_hi` (LCALL ptr16:16)
AND the segment word at site+3 is in that unit's fixup table, i.e. the loader relocates it. This is
the factor-1 evidence an identity argument cites.

Units are keyed by stub paragraph (stub_para), the number the docs use for a unit: "unit 0x23" is
the unit whose stub paragraph is 0x23, not the 0x23rd row of an inventory file.

Only offsets and the 5 call bytes are printed. Game media stays under resources/.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]


def sites(exe, ovr, seg, off):
    """[(stub_para, unit_offset, file_offset)] for every relocated LCALL seg:off."""
    pattern = bytes([0x9A, off & 0xFF, off >> 8, seg & 0xFF, seg >> 8])
    found = []
    for unit in find_units(exe[parse_mz(exe)['header_bytes']:], ovr):
        start, size = unit['ovr_offset'], unit['code_size']
        code, fixups = ovr[start:start + size], set(unit['fixups'])
        at = code.find(pattern)
        while at >= 0:
            if at + 3 in fixups:
                found.append((unit['stub_para'], at, start + at))
            at = code.find(pattern, at + 1)
    return sorted(found)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('target', help='SEG:OFF, hex, e.g. 008F:002F')
    parser.add_argument('--exe', default=ROOT / 'resources/DOS/POOLRAD/START.EXE', type=Path)
    parser.add_argument('--ovr', default=ROOT / 'resources/DOS/POOLRAD/GAME.OVR', type=Path)
    args = parser.parse_args(argv)
    seg, off = (int(x, 16) for x in args.target.split(':'))
    result = sites(args.exe.read_bytes(), args.ovr.read_bytes(), seg, off)
    for para, at, file_at in result:
        print(f'unit {para:02X}h +{at:04X}  file 0x{file_at:05X}  9A {off & 0xFF:02X} {off >> 8:02X} {seg & 0xFF:02X} {seg >> 8:02X}  (segment word fixed up)')
    print(f'{len(result)} relocated far call site(s) to {seg:04X}:{off:04X}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
