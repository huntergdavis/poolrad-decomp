#!/usr/bin/env python3
"""Audit the second primary-byte group of RES_0464:02A5 route 0."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack
from resident_464_mode0_primary_audit import collect as collect_first


BASE = 0x04640
LO, HI = BASE + 0x04CB, BASE + 0x05CB
SPAN_SHA = "e72925f180113d69b94255931a7db9152030c3e4d858a74f2861708108d1b497"
CALLS = (0x04F7, 0x0550)
TABLE_READS = (0x0505, 0x055E)
ES_WRITES = (0x0521, 0x057A, 0x05C0)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_ES

    require(digest(exe) == START_SHA256, "unsupported START.EXE")
    first = collect_first(exe)
    require(first["sha256"] ==
            "b3f223427391b4fc1a5fd4b2d8564ce8363e5c1d5e9623f431c294b01bf04cfe",
            "first primary-byte parent changed")
    image, origin, _, _ = unpack(exe)
    require(sha256(bytes(image[LO:HI])).hexdigest() == SPAN_SHA,
            "second primary-byte span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)),
            "noncontiguous START source")

    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True
    ins = list(md.disasm(bytes(image[LO:HI]), LO))
    require(len(ins) == 103 and ins[0].address == LO and
            ins[-1].address + ins[-1].size == HI,
            "second group instruction coverage changed")
    by_offset = {i.address - BASE: i for i in ins}
    branches = [i for i in ins if i.mnemonic.startswith("j")]
    require(len(branches) == 5, "second group branches changed")
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
            "second group write sites changed")
    require(all((by_offset[o].mnemonic, by_offset[o].op_str) ==
                ("mov", "dl, byte ptr [di + 0x25fa]")
                for o in TABLE_READS), "lookup sites changed")
    require((by_offset[0x04DA].mnemonic, by_offset[0x04DA].op_str) ==
            ("sub", "ax, 4") and
            (by_offset[0x052C].mnemonic, by_offset[0x052C].op_str) ==
            ("sub", "ax, 4"), "second-group table index shift changed")
    require((by_offset[0x05A1].mnemonic, by_offset[0x05A1].op_str) ==
            ("mov", "al, byte ptr es:[di]"),
            "second auxiliary mask read changed")
    require((by_offset[0x05C4].mnemonic, by_offset[0x05C4].op_str) ==
            ("inc", "word ptr [bp - 0x1c]") and
            (by_offset[0x05C7].mnemonic, by_offset[0x05C7].op_str) ==
            ("add", "word ptr [bp + 0xe], 4"),
            "pixel-end offsets changed")
    return {
        "start_sha256": START_SHA256,
        "slice": "RES_0464:02A5/video_state_0/second_primary_byte",
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
        "entry_offset": 0x04CE,
        "backedge_increment_offset": 0x04CB,
        "continuation_offset": 0x05CB,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
