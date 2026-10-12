#!/usr/bin/env python3
"""Verify the two static LOAD MONSTER caller lanes into RES_0802:0716.

Owner-supplied DOS bytes stay in memory. Output contains only addresses,
lengths, counts, and hashes. This audit does not establish DOS route execution.
"""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import OVR_SHA256, START_SHA256, digest, require
from resident_loader_map import unpack


SPANS = (
    ("RES_0464:0111", 0x04751, 0x048E5,
     "ac5dab193845bac0fc67613a82dc60b7148a930e9d1d9f319129e32a7f24271a"),
    ("RES_0802:025D", 0x0827D, 0x0830B,
     "eeea6613012d3db99c7ad85788fdd3ff5b9888d7cc6701e9ce7a612195210dc6"),
    ("RES_0802:02EB", 0x0830B, 0x08376,
     "141b948984a41382c1479d61b09a298f9662139a1bae0fc9f4601484532cd474"),
    ("RES_0802:05CD", 0x085ED, 0x08707,
     "d8067fb72bfede3fb56aa146e941d7bc9524cb23715f8f63e60f4918c65449b5"),
    ("RES_0802:0716", 0x08736, 0x0896A,
     "d14db7945a8e717bc536422426c5df4d35913bdd4c5f4fa10f8a51f5103b34d2"),
)
START_CALLS = (
    (0x047A7, "lcall", "0x802, 0x716"),
    (0x082AF, "call", "0x830b"),
    (0x08330, "lcall", "0xaf8, 0x13e2"),
    (0x0833F, "lcall", "0xaf8, 0x1410"),
    (0x08679, "call", "0x827d"),
    (0x08795, "call", "0x85ed"),
)
OVERLAY_CALLS = (
    ("U3CE_entry4_path_C_first", 0x31A6C, 0x045B,
     "0x464, 0x111"),
    ("U3CE_entry4_path_C_second", 0x31A6C, 0x04C4,
     "0x464, 0x111"),
    ("U6F_entry6_block_1", 0x1DFE6, 0x0F27,
     "0x802, 0x716"),
    ("U6F_entry6_block_2", 0x1DFE6, 0x10C2,
     "0x802, 0x716"),
    ("U6F_entry6_block_3", 0x1DFE6, 0x11AA,
     "0x802, 0x716"),
)


def collect(exe, ovr):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    from capstone.x86 import X86_OP_MEM, X86_REG_BP

    require(digest(exe) == START_SHA256 and digest(ovr) == OVR_SHA256,
            "unsupported DOS input hashes")
    image, origin, _, _ = unpack(exe)
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True

    def one(data, site, address):
        insn = next(decoder.disasm(bytes(data[site:site + 15]), address,
                                    count=1), None)
        require(insn is not None, f"undecodable call site {address:#x}")
        return insn

    spans = []
    for name, lo, hi, expected in SPANS:
        actual = sha256(image[lo:hi]).hexdigest()
        require(actual == expected and
                all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                    for i in range(lo, hi - 1)),
                f"resident span or provenance changed: {name}")
        spans.append({"id": name, "image_start": lo, "image_end_exclusive": hi,
                      "source_file_start": 0x200 + origin[lo],
                      "source_file_end_exclusive": 0x200 + origin[hi - 1] + 1,
                      "bytes": hi - lo, "sha256": actual})

    calls = []
    for site, mnemonic, operand in START_CALLS:
        insn = one(image, site, site)
        require((insn.mnemonic, insn.op_str) == (mnemonic, operand),
                f"resident call changed at {site:#x}")
        calls.append({"site_image": site, "source_file": 0x200 + origin[site],
                      "target": operand})
    for name, base, offset, operand in OVERLAY_CALLS:
        insn = one(ovr, base + offset, offset)
        require((insn.mnemonic, insn.op_str) == ("lcall", operand),
                f"overlay call changed at {name}")
        calls.append({"id": name, "unit_offset": offset,
                      "source_file": base + offset, "target": operand})

    # The two caller lanes use byte-for-byte equal length-four Pascal
    # constants. Publish only length and digest, never the content.
    extension = bytes(image[0x0474C:0x04751])
    other = bytes(ovr[0x1DFE6 + 0x0EA0:0x1DFE6 + 0x0EA5])
    require(extension == other and extension[0] == 4 and
            sha256(extension).hexdigest() ==
            "a374cb557933d9962ea3cc729cdda2459992f5b370d3bd9be9c42407eef0951e",
            "shared Pascal extension changed")

    # Across the whole 564-byte reader, only the low block-ID byte is read.
    id_reads = []
    instructions = list(decoder.disasm(bytes(image[0x08736:0x0896A]),
                                       0x08736))
    require(len(instructions) == 242 and
            instructions[-1].address + instructions[-1].size == 0x0896A,
            "reader instruction extent changed")
    for insn in instructions:
        for operand in insn.operands:
            if (operand.type == X86_OP_MEM and
                    operand.mem.base == X86_REG_BP and
                    operand.mem.disp == 0x0E):
                id_reads.append((insn.address, operand.size))
    require(id_reads == [(0x08875, 1), (0x088A7, 1)],
            "reader block-ID reads changed")
    return {"inputs": {"start_sha256": START_SHA256,
                       "ovr_sha256": OVR_SHA256},
            "spans": spans, "calls": calls,
            "shared_extension": {"pascal_length": 4,
                                 "sha256": sha256(extension).hexdigest()},
            "reader_block_id_read_sites": [address for address, _ in id_reads],
            "route_status": "static_only"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    parser.add_argument("--ovr", required=True, help="ignored DOS GAME.OVR")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes(),
                             Path(args.ovr).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
