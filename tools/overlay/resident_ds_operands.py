#!/usr/bin/env python3
"""List the DS-absolute memory operands and the calls of one resident START.EXE body.

Usage: resident_ds_operands.py SEG:START:END [SEG:START:END ...]

Decodes the loader-expanded START.EXE image (hash-checked, as the resident audits do) from SEG:START up to END and
prints every instruction whose operand is a DS-relative absolute address (`[0x49f3]`), plus every call and RETF. This
is the factor-2 evidence an identity argument cites for a resident body: which game cells the body itself reads or
writes. ES:, SS: and register-based operands are not listed; argument-borne state needs its own caller trace.

Only offsets and decoded instructions are printed. Game media stays under resources/.
"""
import re
import sys
from hashlib import sha256
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from capstone import Cs, CS_ARCH_X86, CS_MODE_16  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
IMAGE_DIGEST = 'a42a57e68d3f7815831821cc9365bb917befda45a58e958544451fc7a878df8e'
DS_ABSOLUTE = re.compile(r'(?<![a-z:])\[0x[0-9a-f]+\]')


def load_image(exe=ROOT / 'resources/DOS/POOLRAD/START.EXE'):
    image = bytes(unpack(Path(exe).read_bytes())[0])
    if sha256(image).hexdigest() != IMAGE_DIGEST:
        raise SystemExit('loader-expanded START.EXE image does not match the pinned digest')
    return image


def operands(image, seg, start, end):
    """[(ip, text)] for the DS-absolute operands, calls and RETFs of [start, end) in segment seg."""
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    found = []
    for insn in md.disasm(image[seg * 16 + start:seg * 16 + end], start):
        text = f'{insn.mnemonic} {insn.op_str}'.strip()
        if DS_ABSOLUTE.search(text) or insn.mnemonic in ('call', 'lcall', 'retf'):
            found.append((insn.address, text))
    return found


def ds_cells(found):
    """The set of DS-absolute addresses in `found`, as ints."""
    return {int(m[1:-1], 16) for _, text in found for m in DS_ABSOLUTE.findall(text)}


def main(argv=None):
    specs = (argv if argv is not None else sys.argv[1:])
    if not specs:
        raise SystemExit(__doc__.splitlines()[2])
    image = load_image()
    for spec in specs:
        seg, start, end = (int(x, 16) for x in spec.split(':'))
        found = operands(image, seg, start, end)
        print(f'== {seg:04X}:[{start:04X},{end:04X}) {end - start} bytes')
        for ip, text in found:
            print(f'  {ip:04X} {text}')
        cells = ' '.join(f'DS:{c:04X}' for c in sorted(ds_cells(found))) or 'none'
        print(f'  DS cells: {cells}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
