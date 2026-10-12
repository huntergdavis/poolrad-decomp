#!/usr/bin/env python3
"""Hash-lock RES_0464:248E and its wrapper call; print addresses only."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack


LO, HI = 0x06ACE, 0x06BD5
EXPECTED = "3476a8ae8b3a3c1430c591b9922d493db2e4c475d542f089257464a82ce9d32f"
BRANCHES = (
    (0x24A4, "je", "0x24ae"),
    (0x2520, "jbe", "0x258f"),
    (0x2527, "jne", "0x2562"),
    (0x2560, "jmp", "0x258f"),
)
CALLS = (
    (0x24C0, "lcall", "0xaf8, 0x23f"),
    (0x24E1, "lcall", "0xaf8, 0x16a8"),
    (0x2531, "lcall", "0xaf8, 0x23f"),
    (0x255B, "lcall", "0xaf8, 0x16a8"),
    (0x2565, "lcall", "0xaf8, 0x23f"),
    (0x258A, "lcall", "0xaf8, 0x16a8"),
)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    require(digest(exe) == START_SHA256, "unsupported START.EXE hash")
    image, origin, _, _ = unpack(exe)
    span = bytes(image[LO:HI])
    require(sha256(span).hexdigest() == EXPECTED, "constructor span changed")
    require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                for i in range(LO, HI - 1)), "noncontiguous source mapping")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    insns = list(decoder.disasm(span, 0x248E))
    require(len(insns) == 95 and insns[0].address == 0x248E and
            insns[-1].address + insns[-1].size == 0x2595 and
            (insns[-1].mnemonic, insns[-1].op_str) == ("retf", "0xc"),
            "constructor instruction extent changed")
    actual_branches = tuple((i.address, i.mnemonic, i.op_str)
                            for i in insns if i.mnemonic.startswith("j"))
    actual_calls = tuple((i.address, i.mnemonic, i.op_str)
                         for i in insns if i.mnemonic in ("call", "lcall"))
    require(actual_branches == BRANCHES and actual_calls == CALLS,
            "constructor branch/call graph changed")
    by_address = {i.address: i for i in insns}
    for address, expected in (
        (0x249F, ("cmp", "byte ptr [0x4a1e], 0")),
        (0x2522, ("cmp", "byte ptr [0x4a1e], 1")),
        (0x2508, ("mov", "word ptr es:[di + 0x11], ax")),
        (0x2515, ("mov", "byte ptr es:[di + 8], al")),
    ):
        insn = by_address.get(address)
        require(insn is not None and
                (insn.mnemonic, insn.op_str) == expected,
                f"constructor field/read site changed: {address:#x}")
    wrapper_site = 0x0485F
    wrapper_call = next(decoder.disasm(
        bytes(image[wrapper_site:wrapper_site + 5]), 0x021F, count=1), None)
    require(wrapper_call is not None and
            (wrapper_call.mnemonic, wrapper_call.op_str) ==
            ("call", "0x248e"), "wrapper caller changed")
    return {
        "start_sha256": START_SHA256,
        "procedure": "RES_0464:248E",
        "image_start": LO,
        "image_end_exclusive": HI,
        "source_file_start": 0x200 + origin[LO],
        "source_file_end_exclusive": 0x200 + origin[HI - 1] + 1,
        "bytes": HI - LO,
        "sha256": EXPECTED,
        "instructions": len(insns),
        "branches": [address for address, _, _ in BRANCHES],
        "runtime_calls": [address for address, _, _ in CALLS],
        "wrapper_call": 0x021F,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
