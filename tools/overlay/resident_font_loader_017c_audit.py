#!/usr/bin/env python3
"""Hash-locked source-only RES0709:017C audit; no execution or global inventory."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_loader_map import unpack, relocations

BODY_SHA = "2da845b46455670c4382adeb8c5038613e458534d915d312d6103f6b01c01300"


def audit(start_exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    image, origins, _, _ = unpack(start_exe)
    relocation_sites, _ = relocations(start_exe, image)
    lo, hi = 0x720C, 0x726F
    body = bytes(image[lo:hi])
    assert len(body) == 99 and sha256(body).hexdigest() == BODY_SHA
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x17C))
    assert len(instructions) == 44
    assert sum(i.size for i in instructions) == 99
    returns = [i for i in instructions if i.mnemonic.startswith("ret")]
    assert len(returns) == 1
    assert returns[0].address == 0x1DE and returns[0].mnemonic == "retf"
    assert returns[0].size == 1 and returns[0].op_str == ""
    calls = [i for i in instructions if i.mnemonic in ("call", "lcall")]
    expected = ((0x18C, "0xaf8, 0xb0b"), (0x19B, "0xaf8, 0xb0b"),
                (0x1AD, "0x802, 0x716"), (0x1C8, "0xaf8, 0x1685"),
                (0x1D6, "0xaf8, 0x254"))
    assert [(i.address, i.op_str) for i in calls] == list(expected)
    assert all(0x7090 + i.address + 3 in relocation_sites for i in calls)
    starts = {i.address for i in instructions}
    jumps = [i for i in instructions if i.mnemonic.startswith("j")]
    assert len(jumps) == 2
    assert all(int(i.op_str, 16) in starts for i in jumps)
    assert 0x200 + origins[lo] == 0x7245
    return {"function": "RES_0709_017C", "mode": "source-only",
            "image_extent": [lo, hi], "offset_extent": [0x17C, 0x1DF],
            "bytes": 99, "instruction_starts": 44, "far_calls": 5,
            "relocated_call_segments": 5, "return": "01DE RETF",
            "body_sha256": BODY_SHA, "execution_cases": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--start-exe", type=Path,
                        default=Path("resources/DOS/POOLRAD/START.EXE"))
    parser.add_argument("--json", type=Path)
    args = parser.parse_args()
    result = audit(args.start_exe.read_bytes())
    output = json.dumps(result, indent=2) + "\n"
    if args.json:
        args.json.write_text(output)
    print(output, end="")


if __name__ == "__main__":
    main()
