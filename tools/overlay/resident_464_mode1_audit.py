#!/usr/bin/env python3
"""Audit RES_0464:02A5 video-state-1 route without emitting DOS bytes."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack
from resident_464_postprocess_map import collect as collect_postprocessor


BASE = 0x04640
LO, HI = BASE + 0x05F6, BASE + 0x07A9
SPAN_SHA = "d91ed4da7d2d1ff7d23a8f977f26bdde07b3d93067c91bcb9a746b31646cade1"
BRANCHES = (0x0605, 0x0607, 0x060D, 0x0621, 0x0623, 0x0629,
            0x063E, 0x0640, 0x0646, 0x066E, 0x069A, 0x06A4,
            0x06B7, 0x06D3, 0x06E8, 0x0715, 0x0725, 0x0727,
            0x0735, 0x073B, 0x077E, 0x078E, 0x0790, 0x0799,
            0x079B, 0x07A4, 0x07A6)
ES_WRITES = (0x068C, 0x070D, 0x0776)


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
    require(sha256(span).hexdigest() == SPAN_SHA, "mode-1 span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)), "noncontiguous source mapping")

    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    insns = list(decoder.disasm(span, 0x05F6))
    require(len(insns) == 162 and insns[0].address == 0x05F6 and
            insns[-1].address + insns[-1].size == 0x07A9,
            "mode-1 instruction extent changed")
    by_offset = {i.address: i for i in insns}
    branches = []
    calls = []
    es_writes = []
    for insn in insns:
        if insn.mnemonic.startswith("j"):
            target = int(insn.op_str, 16)
            require(target in by_offset or target == 0x07A9,
                    f"mode-1 branch target changed at +{insn.address:#x}")
            branches.append(insn.address)
        if insn.mnemonic in ("call", "lcall"):
            calls.append((insn.address, insn.mnemonic, insn.op_str))
        for operand in insn.operands:
            if (operand.type == X86_OP_MEM and
                    operand.mem.segment == X86_REG_ES and
                    operand.access & CS_AC_WRITE):
                es_writes.append(insn.address)
    require(tuple(branches) == BRANCHES, "mode-1 branches changed")
    require(calls == [(0x065E, "lcall", "0xaf8, 0x1685"),
                      (0x067B, "call", "0")],
            "mode-1 callees changed")
    require(tuple(es_writes) == ES_WRITES, "mode-1 ES writes changed")

    key = {
        0x05F9: ("mov", "al, byte ptr es:[di + 8]"),
        0x0615: ("mov", "ax, word ptr es:[di]"),
        0x0618: ("dec", "ax"),
        0x0631: ("mov", "ax, word ptr es:[di + 2]"),
        0x0635: ("dec", "ax"),
        0x064B: ("mov", "ax, word ptr [bp + 0xe]"),
        0x0651: ("add", "di, ax"),
        0x065A: ("mov", "ax, 4"),
        0x0666: ("mov", "ax, word ptr es:[di + 0x13]"),
        0x066A: ("or", "ax, word ptr es:[di + 0x15]"),
        0x0670: ("mov", "al, byte ptr [bp + 0xa]"),
        0x068C: ("mov", "byte ptr es:[di], dl"),
        0x068F: ("inc", "word ptr [bp - 0x1e]"),
        0x0692: ("mov", "byte ptr [bp - 0x22], 0x80"),
        0x06DF: ("mov", "al, byte ptr [di + 0x25f6]"),
        0x06E3: ("and", "al, byte ptr [bp - 0x24]"),
        0x070D: ("mov", "byte ptr es:[di + 0x17], dl"),
        0x071C: ("shr", "ax, 1"),
        0x0740: ("mov", "ax, word ptr [bp - 0x1e]"),
        0x0743: ("dec", "ax"),
        0x074D: ("mov", "al, byte ptr es:[di]"),
        0x0750: ("not", "al"),
        0x0776: ("mov", "byte ptr es:[di + 0x17], dl"),
        0x0780: ("add", "word ptr [bp - 0x1c], 4"),
        0x0784: ("add", "word ptr [bp + 0xe], 4"),
        0x079E: ("mov", "al, byte ptr [bp - 0x16]"),
    }
    for offset, expected in key.items():
        insn = by_offset.get(offset)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == expected,
                f"mode-1 operation changed at +{offset:#x}")

    require(parent["near_helpers"][0]["sha256"] ==
            "3e7b81eaa0e3e8109456fbdb6bd3ad59390b961eceb5f84fa2858055728e21a1",
            "bitmask callee span changed")
    return {
        "start_sha256": START_SHA256,
        "procedure_route": "RES_0464:02A5/video_state_1",
        "image_start": LO, "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO, "sha256": SPAN_SHA,
        "instructions": len(insns), "branch_offsets": BRANCHES,
        "move_call_offset": calls[0][0],
        "bitmask_call_offset": calls[1][0],
        "ds_table_start": 0x25F6,
        "es_write_offsets": ES_WRITES,
        "source_offset_update": 0x0784,
        "common_return_offset": 0x08CE,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
