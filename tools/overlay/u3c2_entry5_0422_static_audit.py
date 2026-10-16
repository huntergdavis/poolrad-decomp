#!/usr/bin/env python3
"""Check U3C2 entry5 original identity/shape without executing it or building C."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import struct
import re

ROOT = Path(__file__).resolve().parents[2]
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
BODY_SHA = "f075c6dba0b31e12aea19318958a72f4caee6fe7954c53f3583dd5dd9fa8114d"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(media_root, source_root=ROOT):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    start = (media_root / "START.EXE").read_bytes()
    ovr = (media_root / "GAME.OVR").read_bytes()
    require(len(start) == 65231 and sha256(start).hexdigest() == START_SHA,
            "START.EXE identity mismatch")
    require(len(ovr) == 206105 and sha256(ovr).hexdigest() == OVR_SHA,
            "GAME.OVR identity mismatch")
    mz = struct.unpack_from("<H", start, 8)[0] * 16
    descriptor = mz + 0x3C28  # Unaligned descriptor, not paragraph base 3C20.
    require(mz == 512 and start[descriptor:descriptor + 4] == b"\xcd\x3f\0\0",
            "U3C2 descriptor mismatch")
    codebase, code_size, fixups, count = struct.unpack_from("<IHHH", start, descriptor + 4)
    require((codebase, code_size, fixups, count) == (195410, 3038, 10, 7),
            "U3C2 descriptor fields mismatch")
    entries = []
    for i in range(count):
        at = descriptor + 23 + 5 * i
        require(start[at:at + 2] == b"\xcd\x3f", "entry marker mismatch")
        entries.append(struct.unpack_from("<H", start, at + 2)[0])
    require(entries == [0xBD9, 0, 0x1A0, 0x2CC, 0x57B, 0x422, 0x909],
            "U3C2 entry table mismatch")
    body = ovr[codebase + 0x422:codebase + 0x57B]
    require(len(body) == 345 and sha256(body).hexdigest() == BODY_SHA,
            "entry5 body mismatch")
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, 0x422))
    require(len(ins) == 141 and sum(x.size for x in ins) == 345,
            "entry5 instruction coverage mismatch")
    by_address = {x.address: x for x in ins}
    returns = [x for x in ins if x.mnemonic.startswith("ret")]
    require([(x.address, x.size, x.mnemonic, x.op_str) for x in returns] ==
            [(0x578, 3, "retf", "0x14")], "entry5 RETF mismatch")
    calls = [x for x in ins if "call" in x.mnemonic]
    expected_calls = [(0x45C, 0x1A0), (0x4D6, 0x1A0), (0x541, 0x2CC), (0x54D, 0x2CC)]
    require([(x.address, int(x.op_str, 0)) for x in calls] == expected_calls and
            all(x.mnemonic == "call" and x.size == 3 for x in calls),
            "entry5 near-callee mismatch")
    for at, target in expected_calls:
        require(body[at - 0x422 - 3:at - 0x422] == b"\x16\x57\x0e" and
                all(by_address[at - i].size == 1 for i in (1, 2, 3)),
                "PUSH SS/DI/CS frame mismatch")
    cbw = [x.address for x in ins if x.size == 1 and body[x.address - 0x422] == 0x98]
    require(len(cbw) == 8, "CBW16 count mismatch")
    branches = [x for x in ins if x.mnemonic.startswith("j")]
    targets = {int(x.op_str, 0) for x in branches}
    require(targets <= set(by_address), "branch leaves entry5 body")
    backedges = [(x.address, int(x.op_str, 0)) for x in branches
                 if int(x.op_str, 0) < x.address]
    require(backedges == [(0x560, 0x4DD)], "entry5 loop shape mismatch")
    c_path = source_root / "src/recovered/overlay_u3c2_entry5_0422.c"
    candidate = c_path.read_text()
    sites = [int(x, 16) for x in re.findall(r"/\* \+([0-9A-F]{4}) \*/", candidate)]
    require(sites == [x.address for x in ins], "C source-site coverage mismatch")
    require(candidate.count("cbw(cpu);") == 8, "C CBW count mismatch")
    labels = {int(x, 16) for x in re.findall(r"^L([0-9A-F]{4}):", candidate, re.M)}
    require(labels == targets, "C branch-label coverage mismatch")
    hook_sites = [(int(t, 16), int(c, 16)) for t, c in re.findall(
        r"near_call\(ram, ram_size, cpu, hooks, (0x[0-9a-f]+)u, (0x[0-9a-f]+)u\)", candidate)]
    require(hook_sites == [(target, at + 3) for at, target in expected_calls],
            "C callee continuation mismatch")
    return {"mode": "static-only", "unit": "0x3c2", "entry_slot": 5,
            "body_sha256": BODY_SHA, "bytes": 345, "instruction_starts": 141,
            "extent": [0x422, 0x57B], "cbw16": 8, "near_calls": expected_calls,
            "backedges": backedges, "return": "0578 RETF 20",
            "C_annotated_sites": len(sites), "C_source_sha256": sha256(c_path.read_bytes()).hexdigest(),
            "execution_cases": 0, "strict_credit_delta": 0,
            "limits": "Shape/identity only; no callee behavior, loop termination or native return proof"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--media-root", type=Path, default=ROOT / "resources/DOS/POOLRAD")
    args = parser.parse_args()
    print(json.dumps(audit(args.media_root), sort_keys=True))


if __name__ == "__main__":
    main()
