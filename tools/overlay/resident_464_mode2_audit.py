#!/usr/bin/env python3
"""Audit RES_0464:02A5 video-state-2 route without emitting DOS bytes."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack
from resident_464_postprocess_map import collect as collect_postprocessor


BASE = 0x04640
LO, HI = BASE + 0x07B3, BASE + 0x08CE
SPAN_SHA = "47505813c4fbaaac9d09797ba7df42c72f2fb76a8669657e2d5201f38f66f771"
BRANCHES = (0x07C2, 0x07C4, 0x07CA, 0x07FE, 0x0800, 0x0817,
            0x0819, 0x081F, 0x0840, 0x0863, 0x08AA, 0x08AC,
            0x08C9, 0x08CB)
ES_WRITES = (0x084E, 0x0871, 0x08A0)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_ES

    require(digest(exe) == START_SHA256, "unsupported START.EXE hash")
    parent = collect_postprocessor(exe)
    require(parent["sha256"] ==
            "1802227461346a3e8a0b9595513d31f820eb4c134903699706f557f5a4f92610",
            "parent postprocessor map changed")
    image, origin, _, _ = unpack(exe)
    span = bytes(image[LO:HI])
    require(sha256(span).hexdigest() == SPAN_SHA, "mode-2 span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)), "noncontiguous source mapping")

    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    insns = list(decoder.disasm(span, 0x07B3))
    require(len(insns) == 101 and insns[0].address == 0x07B3 and
            insns[-1].address + insns[-1].size == 0x08CE,
            "mode-2 instruction extent changed")
    by_offset = {i.address: i for i in insns}
    branches = []
    calls = []
    es_writes = []
    for insn in insns:
        if insn.mnemonic.startswith("j"):
            target = int(insn.op_str, 16)
            require(target in by_offset or target == 0x08CE,
                    f"mode-2 branch target changed at +{insn.address:#x}")
            branches.append(insn.address)
        if insn.mnemonic in ("call", "lcall"):
            calls.append((insn.address, insn.mnemonic, insn.op_str))
        for operand in insn.operands:
            if (operand.type == X86_OP_MEM and
                    operand.mem.segment == X86_REG_ES and
                    operand.access & CS_AC_WRITE):
                es_writes.append(insn.address)
    require(tuple(branches) == BRANCHES, "mode-2 branches changed")
    require(calls == [(0x07EE, "lcall", "0xaf8, 0x1685")],
            "mode-2 Move call changed")
    require(tuple(es_writes) == ES_WRITES, "mode-2 ES writes changed")

    key = {
        0x07B6: ("mov", "al, byte ptr es:[di + 8]"),
        0x07BF: ("cmp", "al, byte ptr [bp - 0x29]"),
        0x07CF: ("mov", "ax, word ptr [bp + 0xe]"),
        0x07D5: ("add", "di, ax"),
        0x07E1: ("add", "di, 0x17"),
        0x07EA: ("push", "word ptr es:[di + 0x11]"),
        0x07F6: ("mov", "ax, word ptr es:[di + 0x13]"),
        0x07FA: ("or", "ax, word ptr es:[di + 0x15]"),
        0x0809: ("add", "ax, word ptr es:[di + 0x11]"),
        0x080D: ("dec", "ax"),
        0x0814: ("cmp", "ax, word ptr [bp - 0x2b]"),
        0x0824: ("mov", "al, byte ptr [bp + 0xa]"),
        0x0833: ("mov", "al, byte ptr es:[di + 0x17]"),
        0x083C: ("shr", "ax, cl"),
        0x083E: ("cmp", "ax, dx"),
        0x084E: ("mov", "byte ptr es:[di], 0xf0"),
        0x085E: ("and", "al, 0xf"),
        0x0860: ("cmp", "al, byte ptr [bp + 0xa]"),
        0x0871: ("add", "byte ptr es:[di], 0xf"),
        0x0884: ("not", "al"),
        0x0894: ("and", "al, dl"),
        0x08A0: ("mov", "byte ptr es:[di + 0x17], dl"),
        0x08B6: ("add", "word ptr [bp - 0x1c], ax"),
        0x08C0: ("add", "word ptr [bp + 0xe], ax"),
        0x08C6: ("cmp", "al, byte ptr [bp - 0x29]"),
    }
    for offset, expected in key.items():
        insn = by_offset.get(offset)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == expected,
                f"mode-2 operation changed at +{offset:#x}")

    # +0x08CE is the parent's common epilogue, outside this route slice.
    epilogue = list(decoder.disasm(bytes(image[HI:HI + 6]), 0x08CE))
    require([(i.mnemonic, i.op_str) for i in epilogue[:3]] ==
            [("mov", "sp, bp"), ("pop", "bp"), ("retf", "0xe")],
            "common return changed")
    return {
        "start_sha256": START_SHA256,
        "procedure_route": "RES_0464:02A5/video_state_2",
        "image_start": LO, "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO, "sha256": SPAN_SHA,
        "instructions": len(insns), "branch_offsets": BRANCHES,
        "move_call_offset": calls[0][0], "es_write_offsets": ES_WRITES,
        "common_return_offset": 0x08CE,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
