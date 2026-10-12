#!/usr/bin/env python3
"""Audit direct resident stores near the saved one-Up dispatch gate.

Original DOS inputs and the reconstructed image remain in memory. Output is
limited to addresses, counts, and hashes. This candidate-seeded scan cannot
exclude indirect stores, aliases, or later live changes to dispatch inputs.
"""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_callable_coverage import near_closure
from resident_inventory import OVR_SHA256, START_SHA256, digest, require
from resident_loader_map import audit, unpack


WATCH = (0x2612, 0x2628, 0x5372, 0x5373, 0x5374)
EXPECTED_STORES = {
    0x2612: (),
    0x2628: (),
    0x5372: (0x0457B, 0x04586, 0x0458D, 0x08996, 0x08A14, 0x08A2E),
    0x5373: (0x08A08, 0x08C61),
    0x5374: (0x08CAB,),
}
SPANS = (
    ("RES_0899:0000_code", 0x08990, 0x089C3,
     "f4d6f25f38c01ad8730fc424ad38214c037804afe497ff125794d8e008465d7d"),
    ("RES_0899:0059", 0x089E9, 0x08A94,
     "4a361032123f0bdc7fa2ff158dc2d8f690716ee716462f66bda9740f32e3362f"),
    ("RES_08AE:01A5", 0x08C85, 0x08CB2,
     "7307263a3cf75d350b4ac4c02226dc74790e437194fb31922620ef24074e0c2f"),
)
WRITE_FIRST = {
    "mov", "add", "adc", "sub", "sbb", "inc", "dec", "and", "or",
    "xor", "not", "neg", "pop", "rol", "ror", "rcl", "rcr", "shl",
    "shr", "sal", "sar", "xchg",
}


def collect(exe, ovr):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    from capstone.x86 import X86_OP_MEM

    require(digest(exe) == START_SHA256 and digest(ovr) == OVR_SHA256,
            "unsupported DOS input hash")
    image, origin, _, _ = unpack(exe)
    inventory = audit(exe, ovr)
    seeds = {row["image_offset"] for row in
             inventory["resident_targets"] + inventory["unmatched_prologues"]}
    _, walk = near_closure(image, seeds)
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    stores = {address: [] for address in WATCH}
    for pos in sorted(walk["decoded"]):
        insn = next(decoder.disasm(bytes(image[pos:pos + 15]), pos, count=1), None)
        require(insn is not None and insn.size == walk["decoded"][pos],
                "decoded instruction differs from resident path walk")
        if insn.mnemonic not in WRITE_FIRST or not insn.operands:
            continue
        dst = insn.operands[0]
        if dst.type != X86_OP_MEM:
            continue
        mem = dst.mem
        if mem.base or mem.index or mem.segment or mem.disp not in stores:
            continue
        require(origin[pos] is not None, "direct store lacks file provenance")
        stores[mem.disp].append({"image_offset": pos,
                                 "source_file_offset": 0x200 + origin[pos]})
    for address, expected in EXPECTED_STORES.items():
        require(tuple(row["image_offset"] for row in stores[address]) == expected,
                f"direct store sites changed for {address:#x}")

    spans = []
    for name, lo, hi, expected_hash in SPANS:
        actual_hash = sha256(image[lo:hi]).hexdigest()
        require(actual_hash == expected_hash and
                all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                    for i in range(lo, hi - 1)),
                f"resident span changed: {name}")
        spans.append({"id": name, "image_start": lo, "image_end_exclusive": hi,
                      "source_file_start": 0x200 + origin[lo],
                      "source_file_end_exclusive": 0x200 + origin[hi - 1] + 1,
                      "bytes": hi - lo, "sha256": actual_hash})
    return {"inputs": {"start_sha256": START_SHA256,
                       "ovr_sha256": OVR_SHA256},
            "scope": {"candidate_seeds": len(seeds),
                      "decoded_instruction_starts": len(walk["decoded"]),
                      "direct_destination_stores_only": True},
            "stores": {f"0x{address:04X}": stores[address] for address in WATCH},
            "spans": spans}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    parser.add_argument("--ovr", required=True, help="ignored DOS GAME.OVR")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes(),
                             Path(args.ovr).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
