#!/usr/bin/env python3
"""Static direct-call selector provenance; no game content or parity claim."""
# NOT_A_ROW_CHECKER: static source provenance, not original/C parity.
import csv
from hashlib import sha256
import json
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from far_call_sites import sites
from function_queue import DIGESTS
from overlay_inventory import find_units, parse_mz
from resident_ds_operands import load_image

ROOT = Path(__file__).resolve().parents[2]
FAR_SITES = {(0x5e, 0x2453): 1, (0x6a, 0x04d6): 1,
             (0x6a, 0x0937): 0, (0x6a, 0x09d9): 2,
             (0x6a, 0x0a6f): 0, (0x6a, 0x0b88): 0,
             (0x6a, 0x0bf1): 3, (0x6a, 0x0ded): 0,
             (0x6c, 0x2f81): 4}
NEAR_SITES = {0x0ccb: 0, 0x1acb: 1}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_setup(instructions, expected, near=False):
    """Check Pascal argument order immediately preceding an actual call."""
    pairs = [(i.mnemonic, i.op_str) for i in instructions]
    if near:
        require(pairs.pop() == ('push', 'cs'), 'near call lacks PUSH CS')
    require(len(pairs) >= 10, 'short argument setup')
    p = pairs[-10:]
    require(p[0][0:1] == ('mov',) and p[0][1].startswith('al, ')
            and p[1] == ('push', 'ax') and p[2] == ('mov', f'al, {expected}')
            and p[3] == ('push', 'ax'), 'selector is not the pinned immediate low byte')
    require(p[4][0] == 'lea' and p[4][1].startswith('di, [bp - ')
            and p[5:7] == [('push', 'ss'), ('push', 'di')]
            and p[7][0] == 'lea' and p[7][1].startswith('di, [bp - ')
            and p[8:10] == [('push', 'ss'), ('push', 'di')], 'pointer setup changed')


def decoded_at(code, start, end):
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(code[start:end], start))
    require(instructions and instructions[0].address == start
            and instructions[-1].address + instructions[-1].size == end,
            f'decode does not reach boundary {start:04X}..{end:04X}')
    return instructions


def check_chain(code86, code77, image):
    """Bind the exceptional selector to constants and then to box coordinates."""
    checks = [
        (code86, 0x124, 0x134, [('cmp', 'byte ptr [bp + 0xa], 2'), ('jne', '0x130'),
                               ('mov', 'byte ptr [bp - 0x5e], 0xf'), ('jmp', '0x134'),
                               ('mov', 'byte ptr [bp - 0x5e], 0x16')]),
        (code86, 0x151, 0x178, [('mov', 'al, 1'), ('push', 'ax'),
                               ('mov', 'al, 3'), ('push', 'ax'),
                               ('mov', 'al, 0x26'), ('push', 'ax'),
                               ('mov', 'al, byte ptr [bp - 0x5e]'), ('push', 'ax'),
                               ('mov', 'al, 0'), ('push', 'ax'),
                               ('mov', 'al, 0'), ('push', 'ax'),
                               ('lea', 'di, [bp - 0x62]'), ('push', 'ss'), ('push', 'di'),
                               ('mov', 'di, 0x1c'), ('push', 'cs'), ('push', 'di'),
                               ('lcall', '0xaf8, 0xb0b'), ('lcall', '0x3f1, 0x2e8')]),
        (code77, 0x2aed, 0x2afb, [('mov', 'al, byte ptr [bp + 0xe]'), ('push', 'ax'),
                                  ('les', 'di, ptr [bp + 0xa]'), ('push', 'es'),
                                  ('push', 'di'), ('lcall', '0x96, 0x25')]),
        (image, 0x3f10 + 0x315, 0x3f10 + 0x32d,
         [('mov', 'al, byte ptr [bp + 0x14]'), ('push', 'ax'),
          ('mov', 'al, byte ptr [bp + 0x12]'), ('push', 'ax'),
          ('mov', 'al, byte ptr [bp + 0x10]'), ('push', 'ax'),
          ('mov', 'al, byte ptr [bp + 0xe]'), ('push', 'ax'),
          ('mov', 'al, byte ptr [bp + 0xc]'), ('push', 'ax'), ('push', 'cs'),
          ('call', '0x3f10')]),
    ]
    for code, start, end, expected in checks:
        actual = [(i.mnemonic, i.op_str) for i in decoded_at(code, start, end)]
        require(actual == expected, f'provenance chain changed at {start:05X}')


def census(root=ROOT):
    root = Path(root)
    exe = (root / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (root / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    require(sha256(exe).hexdigest() == DIGESTS['start_exe_sha256'], 'START.EXE hash mismatch')
    require(sha256(ovr).hexdigest() == DIGESTS['game_ovr_sha256'], 'GAME.OVR hash mismatch')
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    def code(unit):
        u = units[unit]
        return ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
    with (root / 'docs/re/overlay_function_ledger.csv').open() as f:
        rows = list(csv.DictReader(f))
    far = {(unit, site) for unit, site, _ in sites(exe, ovr, 0x85, 0x5c)}
    require(far == set(FAR_SITES), 'relocated far-call census changed')
    require({(unit, site) for unit, site, _ in sites(exe, ovr, 0x96, 0x25)} == {(0x77, 0x2af6)},
            '008A relocated overlay caller census changed')
    code77 = code(0x77)
    near = {site for site in range(len(code77) - 2)
            if code77[site] == 0xe8 and
            (site + 3 + int.from_bytes(code77[site + 1:site + 3], 'little', signed=True)) & 0xffff == 0x299c}
    require(near == set(NEAR_SITES), 'raw near-call candidates changed')
    for (unit, site), value in list(FAR_SITES.items()) + [((0x77, s), v) for s, v in NEAR_SITES.items()]:
        owners = [r for r in rows if int(r['unit'], 0) == unit and
                  int(r['offset'], 0) <= site < int(r['offset'], 0) + int(r['bound'], 0)]
        require(len(owners) == 1, 'call does not have one ledger owner')
        body = code(unit)
        ins = decoded_at(body, int(owners[0]['offset'], 0), site + (3 if unit == 0x77 else 5))
        require(ins[-1].address == site and ins[-1].mnemonic == ('call' if unit == 0x77 else 'lcall'),
                'candidate is not an aligned direct call')
        check_setup(ins[:-1], value, near=unit == 0x77)
    check_chain(code(0x86), code77, load_image(root / 'resources/DOS/POOLRAD/START.EXE'))
    return {'far_sites': len(far), 'near_sites': len(near), 'selector_values': sorted(set(FAR_SITES.values()) | set(NEAR_SITES.values())),
            'scope': 'enumerated direct sites only; no runtime reachability or indirect-call claim',
            'conclusion': 'the enumerated direct setups select the exceptional coordinate from immediates; no game-object witness established'}


if __name__ == '__main__':
    print(json.dumps(census(), indent=2, sort_keys=True))
