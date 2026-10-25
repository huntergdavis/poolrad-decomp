#!/usr/bin/env python3
"""Static t100 caller-object census; no game text or instruction bytes emitted."""
# NOT_A_ROW_CHECKER: static identity evidence, not C/original execution parity.
import argparse
import csv
from hashlib import sha256
import json
from pathlib import Path
import re

from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone.x86 import X86_OP_MEM, X86_REG_BP
from far_call_sites import sites
from function_queue import DIGESTS
from overlay_inventory import find_units, parse_mz
from resident_ds_operands import load_image
from resident_loader_map import relocations, word

ROOT = Path(__file__).resolve().parents[2]
# Generic x86 instruction templates, with original immediates as wildcards.
# LEA DI,[BP+disp]; PUSH SS; PUSH DI; MOV DI,imm; PUSH CS; PUSH DI; StrLoad.
TEXT_SETUP = re.compile(rb'\x8d(?:\x7e.|\xbe..)\x16\x57\xbf(..)\x0e\x57\x9a\x0b\x0b\xf8\x0a', re.S)
SCALARS = re.compile(rb'(?:(?:\xb0.|\x8a\x46.)\x50){6}', re.S)


def require(ok, message):
    if not ok:
        raise ValueError(message)


def argument_setup(code, site, literal_image=None, cs_base=0):
    """Require the complete adjacent text-copy setup, rather than a nearby literal."""
    matches = [(n, m) for n in (15, 16)
               if site >= n and (m := TEXT_SETUP.fullmatch(code[site - n:site]))]
    require(len(matches) == 1, f'no unique CS-literal copy setup at {site:04X}')
    length, match = matches[0]
    scalar_sizes = [n for n in range(18, 25) if site >= length + n
                    and SCALARS.fullmatch(code[site - length - n:site - length])]
    require(len(scalar_sizes) == 1, f'unknown scalar setup at {site:04X}')
    scalar = code[site - length - scalar_sizes[0]:site - length]
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(scalar, 0))
    immediate = sum(i.mnemonic == 'mov' and i.op_str.startswith('al, ') and '[' not in i.op_str
                    for i in instructions)
    literal = int.from_bytes(match[1], 'little')
    image = code if literal_image is None else literal_image
    at = cs_base + literal
    require(0 <= at < len(image), 'literal is outside source image')
    size = 1 + image[at]
    require(at + size <= len(image), 'truncated Pascal literal')
    return dict(setup_start=f'{site - length - scalar_sizes[0]:04X}',
                copy_site=f'{site - 5:04X}', text_source='CS_literal_via_0AF8_0B0B',
                literal_offset=f'{literal:04X}', literal_length=size - 1,
                literal_sha256=sha256(image[at:at + size]).hexdigest(),
                immediate_scalar_low_bytes=immediate, frame_scalar_low_bytes=6 - immediate,
                scalar_high_bytes='AH not normalized by MOV AL')


def census(root=ROOT):
    root = Path(root)
    exe = (root / 'resources/DOS/POOLRAD/START.EXE').read_bytes()
    ovr = (root / 'resources/DOS/POOLRAD/GAME.OVR').read_bytes()
    require(sha256(exe).hexdigest() == DIGESTS['start_exe_sha256'], 'START.EXE hash mismatch')
    require(sha256(ovr).hexdigest() == DIGESTS['game_ovr_sha256'], 'GAME.OVR hash mismatch')
    image = load_image(root / 'resources/DOS/POOLRAD/START.EXE')
    def rows(name):
        with (root / 'docs/re' / name).open() as handle:
            return list(csv.DictReader(handle))
    ledger = rows('overlay_function_ledger.csv')
    classes = {r['id']: r['classification'] for r in rows('overlay_classification.csv')}
    units = {u['stub_para']: u for u in find_units(exe[parse_mz(exe)['header_bytes']:], ovr)}
    overlay = []
    for unit, site, _ in sites(exe, ovr, 0x3F1, 0x02E8):
        u = units[unit]
        code = ovr[u['ovr_offset']:u['ovr_offset'] + u['code_size']]
        owners = [r for r in ledger if int(r['unit'], 0) == unit
                  and int(r['offset'], 0) <= site < int(r['offset'], 0) + int(r['bound'], 0)]
        require(len(owners) == 1, 'overlay caller has no unique ledger owner')
        owner = owners[0]
        require(site - 2 in u['fixups'], 'preceding StrLoad segment is not relocated')
        overlay.append(dict(unit=f'{unit:04X}', site=f'{site:04X}',
                            owner=f"OVL_{unit:04X}_{int(owner['offset'], 0):04X}",
                            classification=classes[owner['id']], **argument_setup(code, site)))
    fixups, _ = relocations(exe, image)
    resident_sites = [p - 3 for p in fixups if p >= 3 and image[p - 3] == 0x9A
                      and word(image, p) == 0x3F1 and word(image, p - 2) == 0x02E8]
    resident = []
    resident_ledger = rows('resident_callable_ledger.csv')
    for site in resident_sites:
        owner = max((r for r in resident_ledger if int(r['image_offset'], 0) <= site),
                    key=lambda r: int(r['image_offset'], 0))
        require(site - 2 in fixups, 'resident StrLoad segment is not relocated')
        # All seven sites lie inside the 00BA resident code segment, bounded
        # by the known prologues at image 032CA, 03379 and 03728.
        require(int(owner['image_offset'], 0) in (0x32CA, 0x3379, 0x3728), 'resident owner changed')
        resident.append(dict(image_site=f'{site:05X}', owner=owner['id'],
                             classification=owner['classification'],
                             **argument_setup(image, site, image, 0xBA0)))
    body = image[0x3F10 + 0x02E8:0x3F10 + 0x0367]
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    instructions = list(decoder.disasm(body, 0x02E8))
    require(sum(i.size for i in instructions) == 127 and instructions[-1].mnemonic == 'retf'
            and instructions[-1].op_str == '0x10', 'complete body/return changed')
    require(all(op.mem.base == X86_REG_BP and not op.mem.index and not op.mem.segment
                for ins in instructions for op in ins.operands if op.type == X86_OP_MEM),
            'body has a memory operand outside its SS frame')
    require(len(overlay) == 27 and len(resident) == 7, 'caller census changed')
    verified = [(r['owner'], r['site']) for r in overlay if r['classification'] == 'game_verified']
    require(verified == [('OVL_001C_0001', '002F'), ('OVL_001C_0001', '0061'),
                         ('OVL_001C_0856', '08A2'), ('OVL_001C_2775', '2A3C')],
            'verified overlay caller set changed')
    return dict(inputs=DIGESTS, body=dict(bytes=127, starts=len(instructions),
                span='03F1:[02E8,0367)', retf='0364 RETF 10h', sha256=sha256(body).hexdigest(),
                memory_operands='SS:BP frame only; no DS/ES/indexed object operand'),
                overlay_callers=overlay, resident_callers=resident,
                verdict='NO factor-2 argument-borne text witness in the measured direct-caller census')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    text = json.dumps(census(), indent=2, sort_keys=True) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    print(text, end='')


if __name__ == '__main__':
    main()
