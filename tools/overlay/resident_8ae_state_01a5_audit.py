#!/usr/bin/env python3
"""Audit the RES_08AE:01A5 state setter and its unresolved I/O callees."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from overlay_inventory import find_units, parse_mz
from resident_inventory import (OVR_SHA256, START_SHA256, collect as inventory,
                                digest, require)
from resident_loader_map import unpack


SPANS = (
    ("setter", 0x08C85, 0x08CB2,
     "7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f"),
    ("start_helper", 0x09E5C, 0x09E91,
     "2dbbe197d8c276b5cb7dc04dc19e0156b06d1772f880396de147fc244470696d"),
    ("stop_helper", 0x09E91, 0x09ECA,
     "0fa5df2cbe185bdb43d4ff225e14b5b23a6893e85721aac20a99f86b34a0b019"),
)

# Each far call pushes a literal WORD immediately before the call.
CALLERS = (
    (0x036, 0x0464, 0x0460, 100),
    (0x05E, 0x2816, 0x2813, 0),
    (0x05E, 0x286E, 0x286A, 10),
    (0x086, 0x0DF0, 0x0DED, 0),
    (0x086, 0x0E3E, 0x0E3A, 10),
    (0x086, 0x16DD, 0x16DA, 0),
    (0x086, 0x1736, 0x1732, 10),
    (0x3C6, 0x0E1A, 0x0E17, 0),
    (0x3C6, 0x109C, 0x1098, 10),
)


def collect(exe, ovr):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    require(digest(exe) == START_SHA256 and digest(ovr) == OVR_SHA256,
            "unsupported DOS input hash")
    image, origin, _, _ = unpack(exe)
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    spans = []
    decoded = {}
    for name, lo, hi, expected in SPANS:
        data = bytes(image[lo:hi])
        instructions = list(decoder.disasm(data, lo))
        require(sha256(data).hexdigest() == expected and
                sum(i.size for i in instructions) == hi - lo and
                all(origin[pos] is not None and
                    origin[pos + 1] == origin[pos] + 1
                    for pos in range(lo, hi - 1)) and
                instructions[-1].mnemonic == "retf",
                f"resident span changed: {name}")
        decoded[name] = {i.address: i for i in instructions}
        spans.append({"name": name, "image_start": lo,
                      "image_end_exclusive": hi, "bytes": hi - lo,
                      "file_start": 0x200 + origin[lo],
                      "file_end_exclusive": 0x200 + origin[hi - 1] + 1,
                      "instructions": len(instructions), "sha256": expected})

    setter = decoded["setter"]
    require(len(setter) == 18 and
            setter[0x08C88].mnemonic == "cmp" and
            setter[0x08C88].op_str == "byte ptr [0x5372], 2" and
            setter[0x08C91].op_str == "word ptr [bp + 6], 0" and
            setter[0x08C97].op_str == "word ptr [0x5374], 0" and
            setter[0x08CA8].op_str == "ax, word ptr [bp + 6]" and
            setter[0x08CAB].op_str == "word ptr [0x5374], ax" and
            setter[0x08CAF].op_str == "2",
            "setter reads, store, or return ABI changed")
    edges = ((0x08C8D, "jne", 0x08C91),
             (0x08C8F, "jmp", 0x08CAE),
             (0x08C95, "jbe", 0x08CA4),
             (0x08C9C, "jne", 0x08CA2),
             (0x08CA2, "jmp", 0x08CA8),
             (0x08C9F, "call", 0x09E91),
             (0x08CA5, "call", 0x09E5C))
    require(all(setter[site].mnemonic == mnemonic and
                setter[site].operands[0].imm == target
                for site, mnemonic, target in edges) and
            setter[0x08C9E].mnemonic == "push" and
            setter[0x08C9E].op_str == "cs" and
            setter[0x08CA4].mnemonic == "push" and
            setter[0x08CA4].op_str == "cs",
            "setter branch or near/far-call convention changed")
    for name in ("start_helper", "stop_helper"):
        instructions = decoded[name].values()
        require(any(i.mnemonic == "int" and i.op_str == "0x21"
                    for i in instructions) and
                any(i.mnemonic == "out" for i in instructions),
                f"{name} DOS/port boundary changed")

    known = inventory(exe, ovr)
    target = next(x for x in known["resident_targets"]
                  if x["segment"] == 0x08AE and x["offset"] == 0x01A5)
    expected_sites = {(unit, site) for unit, site, _, _ in CALLERS}
    require(target["call_sites"] == len(CALLERS) and
            {(x["unit"], x["unit_offset"]) for x in target["sources"]} ==
            expected_sites and target["file_offset"] == spans[0]["file_start"],
            "overlay caller set or source mapping changed")
    units = {u["stub_para"]: u for u in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    callers = []
    for unit, site, producer, argument in CALLERS:
        u = units[unit]
        code = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
        instructions = list(decoder.disasm(code[producer:site + 5],
                                           producer))
        require(len(instructions) == 3 and
                instructions[0].address == producer and
                instructions[1].address == site - 1 and
                instructions[1].mnemonic == "push" and
                instructions[1].op_str == "ax" and
                instructions[2].address == site and
                instructions[2].mnemonic == "lcall" and
                instructions[2].op_str == "0x8ae, 0x1a5",
                f"caller ABI changed: {unit:#x}:{site:#x}")
        first = instructions[0]
        require((argument == 0 and first.mnemonic == "xor" and
                 first.op_str == "ax, ax") or
                (argument != 0 and first.mnemonic == "mov" and
                 first.op_str == f"ax, {argument:#x}"),
                f"caller argument changed: {unit:#x}:{site:#x}")
        callers.append({"unit": unit, "unit_offset": site,
                        "pushed_word": argument})

    first_unit = units[0x036]
    first_code = ovr[first_unit["ovr_offset"]:
                     first_unit["ovr_offset"] + first_unit["code_size"]]
    preclear = list(decoder.disasm(first_code[0x045B:0x0460], 0x045B))
    require(len(preclear) == 2 and
            preclear[0].mnemonic == "xor" and
            preclear[0].op_str == "ax, ax" and
            preclear[1].mnemonic == "mov" and
            preclear[1].op_str == "word ptr [0x5374], ax",
            "unit 0x36 pre-call state clear changed")

    return {"inputs": {"start_sha256": START_SHA256,
                       "ovr_sha256": OVR_SHA256},
            "spans": spans,
            "setter_control_sites": [site for site, _, _ in edges],
            "callers": callers,
            "caller_count": len(callers),
            "callee_external_boundary": "DOS_INT_21_AND_PORT_IO"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    parser.add_argument("--ovr", required=True, help="ignored DOS GAME.OVR")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes(),
                             Path(args.ovr).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
