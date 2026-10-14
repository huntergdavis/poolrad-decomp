#!/usr/bin/env python3
"""Audit the complete RES_0464:1484 leaf; never emit original bytes."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_inventory import collect as resident_inventory
from resident_loader_map import unpack

BASE, LO, HI = 0x4640, 0x1484, 0x1509
SPAN_SHA = "4f836924c0aff281cbe052049608847347095686294ada955e435357e552fc88"


def collect(exe, ovr=None):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16, CS_AC_WRITE
    from capstone.x86 import X86_OP_MEM, X86_REG_BP

    require(digest(exe) == START_SHA256, "unsupported START.EXE")
    image, origin, _, _ = unpack(exe)
    span = bytes(image[BASE + LO:BASE + HI])
    require(sha256(span).hexdigest() == SPAN_SHA, "1484 span changed")
    require(all(origin[p] is not None and origin[p + 1] == origin[p] + 1
                for p in range(BASE + LO, BASE + HI - 1)),
            "noncontiguous loader provenance")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    body = list(decoder.disasm(span, LO))
    require(len(body) == 54 and body[-1].address == 0x1506 and
            body[-1].address + body[-1].size == HI and
            (body[-1].mnemonic, body[-1].op_str) == ("retf", "2"),
            "1484 instruction extent or return changed")
    require(not any(i.mnemonic.startswith("j") or
                    i.mnemonic in ("call", "lcall") for i in body),
            "1484 is no longer a call-free straight-line leaf")
    writes = []
    for i in body:
        for op in i.operands:
            if op.type == X86_OP_MEM and op.access & CS_AC_WRITE:
                require(op.mem.base == X86_REG_BP and op.mem.disp in (-2, -1),
                        "1484 gained nonlocal direct write")
                writes.append(i.address)
    require(writes == [0x1494, 0x14a3, 0x14b2, 0x14be, 0x14ca,
                       0x14d9, 0x14e8, 0x14f7, 0x14fd],
            "local writes changed")
    caller = list(decoder.disasm(bytes(image[BASE + 0x15c7:BASE + 0x1951]),
                                 0x15c7))
    sites = [i.address for i in caller
             if (i.mnemonic, i.op_str) == ("call", "0x1484")]
    require(sites == [0x17ab, 0x183d], "1484 callers changed")
    by_offset = {i.address: i for i in caller}
    for site in sites:
        require((by_offset[site - 1].mnemonic, by_offset[site - 1].op_str) ==
                ("push", "cs") and
                (by_offset[site - 2].mnemonic, by_offset[site - 2].op_str) ==
                ("push", "ax"), "far frame setup changed")
    result = dict(procedure="RES_0464:1484", start_sha256=START_SHA256,
                image_start=BASE + LO, image_end_exclusive=BASE + HI,
                source_file_start=0x200 + origin[BASE + LO],
                source_file_end_exclusive=0x200 + origin[BASE + HI - 1] + 1,
                bytes=len(span), sha256=SPAN_SHA, instructions=len(body),
                branches=0, calls=0, local_write_offsets=writes,
                callers=sites, return_offset=0x1506)
    if ovr is not None:
        graph = resident_inventory(exe, ovr)
        parent = next(t for t in graph["resident_targets"]
                      if t["id"] == "RES_0464_15C7")
        require(parent["sources"] == [dict(unit=0x3ce, unit_offset=0x0592,
                                           ovr_file_offset=204798)],
                "fixup-confirmed overlay caller changed")
        result["game_ovr_sha256"] = digest(ovr)
        result["parent_overlay_callers"] = parent["sources"]
        result["source_ownership"] = parent["classification"]
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--ovr", help="also verify the GAME.OVR caller chain")
    args = parser.parse_args()
    ovr = None if args.ovr is None else Path(args.ovr).read_bytes()
    print(json.dumps(collect(Path(args.exe).read_bytes(), ovr), indent=2))
