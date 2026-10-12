#!/usr/bin/env python3
"""Audit RES_0464:007B and four callers without emitting DOS bytes."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack


BASE, LO, HI = 0x04640, 0x046BB, 0x0470F
SPAN_SHA = "d16625eb920138f29792a8d6587b923b03737a7d832585b9c0567492fbdce5d7"
CALLERS = (0x03F7, 0x044D, 0x04F7, 0x0550)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_BP

    require(digest(exe) == START_SHA256, "unsupported START.EXE hash")
    image, origin, _, _ = unpack(exe)
    span = bytes(image[LO:HI])
    require(sha256(span).hexdigest() == SPAN_SHA, "007B span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)), "noncontiguous source mapping")

    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    insns = list(decoder.disasm(span, LO - BASE))
    require(len(insns) == 37 and insns[0].address == 0x007B and
            insns[-1].address + insns[-1].size == 0x00CF and
            (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "6"),
            "007B extent/return changed")
    by_offset = {i.address: i for i in insns}
    branches = [(i.address, i.mnemonic, int(i.op_str, 16))
                for i in insns if i.mnemonic.startswith("j")]
    require(branches == [(0x0099, "jae", 0x00AE),
                         (0x00AC, "jmp", 0x00C6)] and
            all(target in by_offset for _, _, target in branches),
            "007B branch graph changed")
    calls = [(i.address, i.mnemonic, i.op_str) for i in insns
             if i.mnemonic in ("call", "lcall")]
    require(calls == [(0x008F, "lcall", "0xaf8, 0xa3f")],
            "Move call changed")
    key = {
        0x0081: ("les", "di, ptr [bp + 6]"),
        0x0084: ("push", "es"),
        0x0085: ("push", "di"),
        0x0086: ("lea", "di, [bp - 9]"),
        0x0089: ("push", "ss"),
        0x008A: ("push", "di"),
        0x008B: ("mov", "ax, 8"),
        0x008E: ("push", "ax"),
        0x0094: ("mov", "al, byte ptr [bp + 0xa]"),
        0x0097: ("shr", "al, 1"),
        0x00A4: ("mov", "al, byte ptr [bp + di - 9]"),
        0x00A7: ("and", "al, 0xf"),
        0x00B7: ("mov", "al, byte ptr [bp + di - 9]"),
        0x00BA: ("and", "al, 0xf0"),
        0x00C1: ("shr", "ax, cl"),
    }
    for offset, expected in key.items():
        insn = by_offset.get(offset)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == expected,
                f"007B contract changed at +{offset:#x}")
    writes = []
    for insn in insns:
        for operand in insn.operands:
            if operand.type == X86_OP_MEM and operand.access & CS_AC_WRITE:
                require(operand.mem.base == X86_REG_BP and
                        operand.mem.disp == -1,
                        f"external direct write at +{insn.address:#x}")
                writes.append(insn.address)
    require(writes == [0x00A9, 0x00C3], "direct write sites changed")

    caller_insns = list(decoder.disasm(
        bytes(image[BASE + 0x03C8:BASE + 0x0553]), 0x03C8))
    caller_by_offset = {i.address: i for i in caller_insns}
    # The four selector pushes each follow a low-nibble mask or
    # high-nibble mask plus right shift. The postprocessor itself is
    # separately hash-locked by resident_464_postprocess_map.py.
    selectors = {
        0x03EC: (0x03EA, "and", "al, 0xf"),
        0x0442: (0x0440, "shr", "ax, cl"),
        0x04EC: (0x04EA, "and", "al, 0xf"),
        0x0545: (0x0543, "shr", "ax, cl"),
    }
    for push, (prior, mnemonic, operand) in selectors.items():
        p = caller_by_offset.get(prior)
        q = caller_by_offset.get(push)
        require(p is not None and (p.mnemonic, p.op_str) ==
                (mnemonic, operand) and q is not None and
                (q.mnemonic, q.op_str) == ("push", "ax"),
                f"caller selector changed at +{push:#x}")
    for offset in (0x0439, 0x053C):
        insn = caller_by_offset.get(offset)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == ("and", "al, 0xf0"),
                f"caller high-nibble mask changed at +{offset:#x}")
    for offset in (0x03ED, 0x0443, 0x04ED, 0x0546):
        first = caller_by_offset.get(offset)
        second = caller_by_offset.get(offset + 3)
        require(first is not None and second is not None and
                (first.mnemonic, first.op_str) ==
                ("les", "di, ptr [bp - 0x28]") and
                (second.mnemonic, second.op_str) == ("add", "di, 9"),
                f"caller source pointer changed at +{offset:#x}")
    for offset in CALLERS:
        insn = caller_by_offset.get(offset)
        prev = caller_by_offset.get(offset - 1)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == ("call", "0x7b") and
                prev is not None and
                (prev.mnemonic, prev.op_str) == ("push", "cs"),
                f"007B caller changed at +{offset:#x}")

    next_insn = next(decoder.disasm(bytes(image[HI:HI + 4]), 0x00CF), None)
    require(next_insn is not None and
            (next_insn.mnemonic, next_insn.op_str) == ("push", "bp"),
            "next prologue changed")
    return {
        "start_sha256": START_SHA256,
        "procedure": "RES_0464:007B",
        "image_start": LO, "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO, "sha256": SPAN_SHA,
        "instructions": len(insns), "branches": branches,
        "runtime_move_call": calls[0][0],
        "direct_local_write_offsets": writes,
        "caller_offsets": CALLERS, "return_offset": 0x00CC,
        "next_prologue": 0x00CF,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
