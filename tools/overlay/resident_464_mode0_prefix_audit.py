#!/usr/bin/env python3
"""Audit RES_0464:02A5 route 0 and its bounded first-pixel prefix."""

import argparse
from collections import Counter
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack
from resident_464_postprocess_map import collect as collect_parent


BASE = 0x04640
ROUTE_LO, ROUTE_HI = BASE + 0x02D3, BASE + 0x05EF
PREFIX_HI = BASE + 0x03CE
ROUTE_SHA = "5b3cd8261e021741533286f4acbcd7c6bedb71d5fd13b3363698ac067f9ffc66"
PREFIX_SHA = "62cab58be4313e124ebd1ae7778e543cfc60d51f22f6bc9865d36c2ba8dc9b68"
CALLS = ((0x033B, "lcall", "0xaf8, 0x1685"),
         (0x0358, "call", "0x4640"),
         (0x03F7, "call", "0x46bb"),
         (0x044D, "call", "0x46bb"),
         (0x04F7, "call", "0x46bb"),
         (0x0550, "call", "0x46bb"))
ES_WRITES = (0x03AC, 0x0421, 0x0477, 0x04BE, 0x0521, 0x057A, 0x05C0)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_ES

    require(digest(exe) == START_SHA256, "unsupported START.EXE")
    parent = collect_parent(exe)
    require(parent["sha256"] ==
            "1802227461346a3e8a0b9595513d31f820eb4c134903699706f557f5a4f92610",
            "parent procedure changed")
    image, origin, _, _ = unpack(exe)
    require(sha256(bytes(image[ROUTE_LO:ROUTE_HI])).hexdigest() ==
            ROUTE_SHA, "route-0 span changed")
    require(sha256(bytes(image[ROUTE_LO:PREFIX_HI])).hexdigest() ==
            PREFIX_SHA, "prefix span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(ROUTE_LO, ROUTE_HI - 1)),
            "noncontiguous START source")

    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True
    ins = list(md.disasm(bytes(image[ROUTE_LO:ROUTE_HI]), ROUTE_LO))
    require(len(ins) == 313 and
            ins[0].address == ROUTE_LO and
            ins[-1].address + ins[-1].size == ROUTE_HI,
            "route-0 instruction coverage changed")
    by_offset = {i.address - BASE: i for i in ins}
    require(len(by_offset) == len(ins), "duplicate instruction start")
    prefix = [i for i in ins if i.address < PREFIX_HI]
    require(len(prefix) == 94 and
            prefix[-1].address + prefix[-1].size == PREFIX_HI,
            "prefix instruction coverage changed")
    branches = [i for i in ins if i.mnemonic.startswith("j") or
                i.mnemonic.startswith("loop")]
    require(len(branches) == 34 and
            sum(i.address < PREFIX_HI for i in branches) == 16,
            "route branch count changed")
    calls = tuple((i.address - BASE, i.mnemonic, i.op_str) for i in ins
                  if i.mnemonic in ("call", "lcall"))
    require(calls == CALLS, "route call sites changed")
    es_writes = []
    for i in ins:
        if any(op.type == X86_OP_MEM and
               op.mem.segment == X86_REG_ES and op.access & CS_AC_WRITE
               for op in i.operands):
            es_writes.append(i.address - BASE)
    require(tuple(es_writes) == ES_WRITES, "route write sites changed")

    require((by_offset[0x034B].mnemonic, by_offset[0x034B].op_str) ==
            ("je", "0x4a08"), "auxiliary gate changed")
    require((by_offset[0x03CC].mnemonic, by_offset[0x03CC].op_str) ==
            ("jmp", "0x4a11"), "prefix continuation changed")
    require((by_offset[0x0387].mnemonic, by_offset[0x0387].op_str) ==
            ("mov", "dx, word ptr [di + 0x25e5]"),
            "auxiliary pattern table changed")
    require((by_offset[0x05C7].mnemonic, by_offset[0x05C7].op_str) ==
            ("add", "word ptr [bp + 0xe], 4"),
            "source offset update changed")
    require(Counter(i.op_str for i in ins if i.mnemonic == "call") ==
            Counter({"0x4640": 1, "0x46bb": 4}),
            "near helper count changed")

    return {
        "start_sha256": START_SHA256,
        "route": "RES_0464:02A5/video_state_0",
        "image_start": ROUTE_LO,
        "image_end_exclusive": ROUTE_HI,
        "source_file_start": 0x200 + origin[ROUTE_LO],
        "source_file_end_exclusive": 0x200 + origin[ROUTE_HI - 1] + 1,
        "bytes": ROUTE_HI - ROUTE_LO,
        "sha256": ROUTE_SHA,
        "instructions": len(ins),
        "branch_sites": len(branches),
        "calls": [{"offset": o, "kind": k, "target": t}
                  for o, k, t in calls],
        "es_write_offsets": ES_WRITES,
        "prefix_image_end_exclusive": PREFIX_HI,
        "prefix_bytes": PREFIX_HI - ROUTE_LO,
        "prefix_sha256": PREFIX_SHA,
        "prefix_instructions": len(prefix),
        "prefix_branch_sites": 16,
        "prefix_continuation_offset": 0x03D1,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
