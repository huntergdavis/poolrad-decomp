#!/usr/bin/env python3
"""Audit RES_0464:0000 and its two callers without emitting DOS bytes."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack


LO, HI = 0x04640, 0x046BB
SPAN_SHA = "3e7b81eaa0e3e8109456fbdb6bd3ad59390b961eceb5f84fa2858055728e21a1"
CALLERS = (0x034D, 0x0670)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_BP

    require(digest(exe) == START_SHA256, "unsupported START.EXE hash")
    image, origin, _, _ = unpack(exe)
    span = bytes(image[LO:HI])
    require(sha256(span).hexdigest() == SPAN_SHA, "bitmask span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)), "noncontiguous source mapping")

    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    instructions = list(decoder.disasm(span, LO))
    require(len(instructions) == 47 and
            instructions[0].address == LO and
            instructions[-1].address + instructions[-1].size == HI,
            "helper extent changed")
    require((instructions[-1].mnemonic, instructions[-1].op_str) ==
            ("retf", "6"), "helper return changed")
    require(not any(insn.mnemonic in ("call", "lcall")
                    for insn in instructions), "helper gained a call")
    branches = []
    writes = []
    for insn in instructions:
        if insn.mnemonic.startswith("j"):
            target = int(insn.op_str, 16)
            require(LO <= target < HI, "branch leaves helper")
            branches.append(insn.address - LO)
        for operand in insn.operands:
            if operand.type == X86_OP_MEM and operand.access & CS_AC_WRITE:
                require(operand.mem.base == X86_REG_BP and
                        operand.mem.disp < 0,
                        f"external write at {insn.address:#x}")
                writes.append(insn.address - LO)
    require(branches == [0x001F, 0x0033, 0x0054, 0x006A],
            "helper branches changed")
    require(writes == [0x0006, 0x0014, 0x0017, 0x001B, 0x0021,
                       0x0038, 0x0042, 0x0059, 0x0063, 0x006F],
            "local write sites changed")
    key = {
        0x000A: ("mov", "al, byte ptr [bp + 0xa]"),
        0x0012: ("shl", "ax, cl"),
        0x002B: ("mov", "al, byte ptr [bp + di + 6]"),
        0x002E: ("and", "al, 0xf0"),
        0x0030: ("cmp", "al, byte ptr [bp - 3]"),
        0x0038: ("add", "byte ptr [bp - 2], al"),
        0x004C: ("mov", "al, byte ptr [bp + di + 6]"),
        0x004F: ("and", "al, 0xf"),
        0x0051: ("cmp", "al, byte ptr [bp + 0xa]"),
        0x0059: ("add", "byte ptr [bp - 2], al"),
        0x0066: ("cmp", "byte ptr [bp - 5], 3"),
    }
    by_offset = {insn.address - LO: insn for insn in instructions}
    require(all(int(insn.op_str, 16) - LO in by_offset
                for insn in instructions if insn.mnemonic.startswith("j")),
            "branch target is not an instruction start")
    for offset, expected in key.items():
        insn = by_offset.get(offset)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == expected,
                f"helper contract changed at +{offset:#x}")

    caller_calls = []
    for start in CALLERS:
        address = 0x04640 + start
        sequence = list(decoder.disasm(bytes(image[address:address + 18]),
                                       address))
        expected = [
            ("mov", "al, byte ptr [bp + 0xa]"),
            ("push", "ax"),
            ("push", "word ptr [bp - 0x13]"),
            ("push", "word ptr [bp - 0x15]"),
            ("push", "cs"),
            ("call", "0x4640"),
        ]
        require([(i.mnemonic, i.op_str) for i in sequence[:6]] == expected,
                f"caller setup changed at +{start:#x}")
        caller_calls.append(sequence[5].address - 0x04640)
    require(caller_calls == [0x0358, 0x067B], "caller sites changed")

    next_insn = next(decoder.disasm(bytes(image[HI:HI + 4]), HI), None)
    require(next_insn is not None and next_insn.mnemonic == "push" and
            next_insn.op_str == "bp", "next helper prologue changed")
    return {
        "start_sha256": START_SHA256,
        "procedure": "RES_0464:0000",
        "image_start": LO, "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO, "sha256": SPAN_SHA,
        "instructions": len(instructions),
        "branch_offsets": branches, "local_write_offsets": writes,
        "direct_calls": 0, "return_offset": 0x0078,
        "caller_offsets": caller_calls, "next_prologue": HI,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
