#!/usr/bin/env python3
"""Audit the first primary-byte group of RES_0464:02A5 route 0."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack
from resident_464_mode0_prefix_audit import collect as collect_route


BASE = 0x04640
LO, HI = BASE + 0x03CE, BASE + 0x04CB
SPAN_SHA = "b3f223427391b4fc1a5fd4b2d8564ce8363e5c1d5e9623f431c294b01bf04cfe"
CALLS = (0x03F7, 0x044D)
TABLE_READS = (0x0405, 0x045B)
ES_WRITES = (0x0421, 0x0477, 0x04BE)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_ES

    require(digest(exe) == START_SHA256, "unsupported START.EXE")
    route = collect_route(exe)
    require(route["sha256"] ==
            "5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66",
            "route-0 parent changed")
    image, origin, _, _ = unpack(exe)
    require(sha256(bytes(image[LO:HI])).hexdigest() == SPAN_SHA,
            "first primary-byte span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)),
            "noncontiguous START source")

    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True
    ins = list(md.disasm(bytes(image[LO:HI]), LO))
    require(len(ins) == 103 and ins[0].address == LO and
            ins[-1].address + ins[-1].size == HI,
            "first group instruction coverage changed")
    by_offset = {i.address - BASE: i for i in ins}
    branches = [i for i in ins if i.mnemonic.startswith("j")]
    require(len(branches) == 6, "first group branches changed")
    calls = tuple(i.address - BASE for i in ins if i.mnemonic == "call")
    require(calls == CALLS and
            all(by_offset[o].op_str == "0x46bb" for o in calls),
            "nibble helper call sites changed")
    es_writes = []
    for i in ins:
        if any(op.type == X86_OP_MEM and
               op.mem.segment == X86_REG_ES and op.access & CS_AC_WRITE
               for op in i.operands):
            es_writes.append(i.address - BASE)
    require(tuple(es_writes) == ES_WRITES,
            "primary write sites changed")
    require(all((by_offset[o].mnemonic, by_offset[o].op_str) ==
                ("mov", "dl, byte ptr [di + 0x25fa]")
                for o in TABLE_READS), "private lookup sites changed")
    require((by_offset[0x049F].mnemonic, by_offset[0x049F].op_str) ==
            ("mov", "al, byte ptr es:[di]"),
            "auxiliary mask read changed")
    require((by_offset[0x04C2].mnemonic, by_offset[0x04C2].op_str) ==
            ("inc", "word ptr [bp - 0x1c]"),
            "primary data offset update changed")
    require((by_offset[0x04C9].mnemonic, by_offset[0x04C9].op_str) ==
            ("jmp", "0x4b0e"), "second-group continuation changed")
    return {
        "start_sha256": START_SHA256,
        "slice": "RES_0464:02A5/video_state_0/first_primary_byte",
        "image_start": LO,
        "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO,
        "sha256": SPAN_SHA,
        "instructions": len(ins),
        "branch_sites": len(branches),
        "nibble_call_offsets": CALLS,
        "private_lookup_offsets": TABLE_READS,
        "es_write_offsets": ES_WRITES,
        "entry_offset": 0x03D1,
        "backedge_increment_offset": 0x03CE,
        "continuation_offset": 0x04CE,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
