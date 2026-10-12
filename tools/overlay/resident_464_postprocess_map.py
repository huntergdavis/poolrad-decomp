#!/usr/bin/env python3
"""Map RES_0464:02A5 reachable instructions without emitting DOS bytes."""

import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack


LO, HI = 0x048E5, 0x04F14
SPAN_SHA = "1802227461346a3e8a0b9595513d31f820eb4c134903699706f557f5a4f92610"
HELPERS = (
    ("RES_0464:0000", 0x04640, 0x046BB,
     "3e7b81eaa0e3e8109456fbdb6bd3ad59390b961eceb5f84fa2858055728e21a1"),
    ("RES_0464:007B", 0x046BB, 0x0470F,
     "d16625eb920138f29792a8d6587b923b03737a7d832585b9c0567492fbdce5d7"),
    ("RES_0464:00CF", 0x0470F, 0x0473F,
     "1291e63d47f0d18e3c3708159546250c155647778751c8e8a2123f8bcc44afb3"),
    ("RES_0464:00FF", 0x0473F, 0x0474C,
     "cec981142e63bb53b622852e0f77455255fae44bc55ea80b0e47105e05e18ef6"),
)
ES_WRITES = (0x03AC, 0x0421, 0x0477, 0x04BE, 0x0521, 0x057A, 0x05C0,
             0x068C, 0x070D, 0x0776, 0x084E, 0x0871, 0x08A0)
SOURCE_OFFSET_WRITES = (0x05C7, 0x0784, 0x08C0)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_BP, X86_REG_ES

    require(digest(exe) == START_SHA256, "unsupported START.EXE hash")
    image, origin, _, _ = unpack(exe)
    span = bytes(image[LO:HI])
    require(sha256(span).hexdigest() == SPAN_SHA,
            "postprocessor span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)), "noncontiguous source mapping")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    todo = [LO]
    instructions = {}
    branches = []
    calls = []
    returns = []
    while todo:
        address = todo.pop()
        while LO <= address < HI and address not in instructions:
            insn = next(decoder.disasm(
                bytes(image[address:address + 15]), address, count=1), None)
            require(insn is not None, f"undecodable {address:#x}")
            instructions[address] = insn
            fallthrough = address + insn.size
            if insn.mnemonic in ("call", "lcall"):
                calls.append((insn.address, insn.mnemonic, insn.op_str))
            if insn.mnemonic.startswith("ret"):
                returns.append((insn.address, insn.mnemonic, insn.op_str))
                break
            if (insn.mnemonic.startswith("j") or
                    insn.mnemonic.startswith("loop")):
                target = int(insn.op_str, 16)
                require(LO <= target < HI,
                        f"edge leaves proposed extent at {address:#x}")
                branches.append((insn.address, insn.mnemonic, target))
                todo.append(target)
                if insn.mnemonic == "jmp":
                    break
                todo.append(fallthrough)
                break
            require(fallthrough <= HI, "instruction passes extent")
            address = fallthrough

    owner = {}
    for address, insn in instructions.items():
        for byte_address in range(address, address + insn.size):
            require(byte_address not in owner and LO <= byte_address < HI,
                    f"overlapping instruction at {byte_address:#x}")
            owner[byte_address] = address
    require(len(owner) == HI - LO and len(instructions) == 604,
            "reachable instruction coverage changed")
    require(returns == [(0x04F11, "retf", "0xe")] and
            len(branches) == 84, "return or branch graph changed")
    require(Counter((mnemonic, operand) for _, mnemonic, operand in calls) ==
            Counter({("lcall", "0xaf8, 0x1685"): 3,
                     ("call", "0x4640"): 2,
                     ("call", "0x46bb"): 4}),
            "near/runtime callee count changed")

    es_writes = []
    source_offset_writes = []
    unused_arg_reads = []
    for insn in instructions.values():
        for operand in insn.operands:
            if operand.type != X86_OP_MEM:
                continue
            if operand.mem.segment == X86_REG_ES and operand.access & CS_AC_WRITE:
                es_writes.append(insn.address - 0x04640)
            if operand.mem.base == X86_REG_BP and operand.mem.disp == 0x0E:
                if operand.access & CS_AC_WRITE:
                    source_offset_writes.append(insn.address - 0x04640)
            if operand.mem.base == X86_REG_BP and operand.mem.disp == 0x0C:
                unused_arg_reads.append(insn.address - 0x04640)
    require(tuple(sorted(es_writes)) == ES_WRITES,
            "object/auxiliary write sites changed")
    require(tuple(sorted(source_offset_writes)) == SOURCE_OFFSET_WRITES,
            "source offset updates changed")
    require(not unused_arg_reads,
            "formerly ignored [BP+0x0C] argument is now referenced")
    video = [i.address - 0x04640 for i in instructions.values()
             if i.mnemonic == "mov" and i.op_str ==
             "al, byte ptr [0x4a1e]"]
    require(video == [0x02C9], "video dispatch read changed")

    helpers = []
    for name, start, end, expected in HELPERS:
        actual = sha256(bytes(image[start:end])).hexdigest()
        require(actual == expected, f"helper span changed: {name}")
        helper_insns = list(decoder.disasm(
            bytes(image[start:end]), start - 0x04640))
        # +0x007B ends at RETF 6 before the next independent +0x00CF.
        require(helper_insns and
                helper_insns[-1].address + helper_insns[-1].size ==
                end - 0x04640, f"helper instruction bound changed: {name}")
        helpers.append({"id": name, "image_start": start,
                        "image_end_exclusive": end, "bytes": end - start,
                        "sha256": expected})
    next_insns = list(decoder.disasm(bytes(image[HI:HI + 4]), HI))
    require(len(next_insns) >= 2 and
            (next_insns[0].mnemonic, next_insns[1].mnemonic) ==
            ("push", "mov"), "next prologue changed")
    return {
        "start_sha256": START_SHA256,
        "procedure": "RES_0464:02A5",
        "image_start": LO, "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO, "sha256": SPAN_SHA,
        "reachable_instructions": len(instructions),
        "covered_bytes": len(owner),
        "branch_sites": len(branches), "direct_calls": len(calls),
        "returns": [address - 0x04640 for address, _, _ in returns],
        "callee_counts": {"runtime_Move": 3, "near_0000": 2,
                          "near_007B": 4},
        "es_write_offsets": ES_WRITES,
        "source_offset_updates": SOURCE_OFFSET_WRITES,
        "video_dispatch_read": video[0],
        "unused_frame_byte_offset": 0x0C,
        "near_helpers": helpers,
        "separate_data_start": 0x0474C,
        "next_prologue": HI,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
