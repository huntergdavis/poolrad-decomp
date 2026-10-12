#!/usr/bin/env python3
"""Address-only audit of GAME.OVR unit 0x9E entry 8 and its DOS RNG."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack

OVR_SHA256 = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA256 = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
ENTRY_SHA256 = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"
RANDOM_SHA256 = "e4337c9ed94c3d0a72864fcf23dd6e8fb30693d25680a1d52401c919fcbbfeb9"
SEED_SHA256 = "f5b67b3eec594ce9fa18e9d78e0bb2eec93c9475a758f2c7ae9988ad00ce6242"


def require(ok, message):
    if not ok:
        raise ValueError(message)


def decode(decoder, data, address, expected_count):
    instructions = list(decoder.disasm(bytes(data), address))
    require(len(instructions) == expected_count and
            sum(i.size for i in instructions) == len(data),
            f"instruction extent changed at {address:#x}")
    return {i.address: i for i in instructions}


def audit(exe, ovr):
    from capstone import CS_ARCH_X86, CS_MODE_16, Cs

    require(sha256(exe).hexdigest() == EXE_SHA256, "START.EXE hash changed")
    require(sha256(ovr).hexdigest() == OVR_SHA256, "GAME.OVR hash changed")
    units = {u["stub_para"]: u for u in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    unit = units[0x9E]
    require(unit["ovr_offset"] == 0x2AEEA and
            unit["entries"][8:10] == [0x0DE3, 0x0E2E] and
            unit["code_size"] >= 0x0E2E and
            unit["memory_segment"] == 0xB0,
            "unit descriptor or entry bound changed")
    code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
    entry = code[0x0DE3:0x0E2E]
    require(sha256(entry).hexdigest() == ENTRY_SHA256 and len(entry) == 75,
            "entry 8 span changed")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    e = decode(decoder, entry, 0x0DE3, 29)
    sites = {
        0x0DED: ("cmp", "byte ptr [bp + 8], 0"),
        0x0DF3: ("mov", "al, byte ptr [bp + 8]"),
        0x0E08: ("mov", "al, byte ptr [bp + 6]"),
        0x0E0B: ("xor", "ah, ah"),
        0x0E0D: ("push", "ax"),
        0x0E0E: ("lcall", "0xaf8, 0x11c0"),
        0x0E13: ("inc", "ax"),
        0x0E14: ("add", "byte ptr [bp - 3], al"),
        0x0E1F: ("mov", "al, byte ptr [bp - 3]"),
        0x0E2B: ("retf", "4"),
    }
    require(all(e[a].mnemonic == op and e[a].op_str == args
                for a, (op, args) in sites.items()),
            "entry byte ABI or effect changed")
    branches = {0x0DF1: ("jbe", "0xe1f"),
                0x0DFE: ("ja", "0xe1f"),
                0x0E03: ("jmp", "0xe08"),
                0x0E1D: ("jne", "0xe05")}
    require(all(e[a].mnemonic == op and e[a].op_str == target
                for a, (op, target) in branches.items()),
            "entry branch changed")
    require(0x0E11 in unit["fixups"], "SYSTEM far call fixup changed")

    image, origin, _, _ = unpack(exe)
    runtime = []
    for name, lo, hi, digest, count in (
        ("random", 0x0C140, 0x0C156, RANDOM_SHA256, 10),
        ("seed", 0x0C18F, 0x0C1C5, SEED_SHA256, 22),
    ):
        data = bytes(image[lo:hi])
        require(sha256(data).hexdigest() == digest and
                all(origin[pos] is not None and
                    origin[pos + 1] == origin[pos] + 1
                    for pos in range(lo, hi - 1)),
                f"SYSTEM {name} span changed")
        ins = decode(decoder, data, lo, count)
        runtime.append({"name": name, "image_start": lo,
                        "image_end_exclusive": hi,
                        "file_start": origin[lo] + 0x200,
                        "file_end_exclusive": origin[hi - 1] + 0x201,
                        "instructions": len(ins), "sha256": digest})
        if name == "random":
            require(ins[0x0C140].op_str == "0xc18f" and
                    ins[0x0C147].op_str == "bx, word ptr ss:[bx + 4]" and
                    ins[0x0C14D].op_str == "0xc153" and
                    ins[0x0C150].op_str == "bx" and
                    ins[0x0C153].op_str == "2",
                    "SYSTEM random range/zero path changed")
        else:
            require(ins[0x0C18F].op_str == "ax, word ptr [0x4428]" and
                    ins[0x0C192].op_str == "bx, word ptr [0x442a]" and
                    ins[0x0C198].op_str == "word ptr cs:[0x1245]" and
                    ins[0x0C1BD].op_str == "word ptr [0x4428], ax" and
                    ins[0x0C1C0].op_str == "word ptr [0x442a], dx",
                    "SYSTEM seed reads/writes changed")
    require(int.from_bytes(image[0x0C1C5:0x0C1C7], "little") == 0x8405,
            "SYSTEM multiplier low word changed")

    callers = {}
    for u in units.values():
        c = ovr[u["ovr_offset"]:u["ovr_offset"] + u["code_size"]]
        hits = [fx - 3 for fx in u["fixups"] if fx >= 3 and
                fx + 1 < len(c) and c[fx - 3] == 0x9A and
                int.from_bytes(c[fx:fx + 2], "little") == 0xB0 and
                int.from_bytes(c[fx - 2:fx], "little") == 0x48]
        if hits:
            callers[f"0x{u['stub_para']:X}"] = hits
    require(sum(map(len, callers.values())) == 122 and
            len(callers) == 15 and len(callers.get("0x68", [])) == 13 and
            0x0513 in callers["0x68"], "fixup-confirmed caller set changed")
    return {
        "inputs_sha256": {"GAME.OVR": OVR_SHA256, "START.EXE": EXE_SHA256},
        "entry": {"unit": "0x9E", "index": 8, "offset": 0x0DE3,
                  "end_exclusive": 0x0E2E, "file_start": 0x2BCCD,
                  "file_end_exclusive": 0x2BD18, "bytes": 75,
                  "instructions": len(e), "sha256": ENTRY_SHA256},
        "system": runtime,
        "fixup_confirmed_far_callers": callers,
        "far_caller_count": sum(map(len, callers.values())),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--ovr", required=True)
    args = parser.parse_args()
    print(json.dumps(audit(Path(args.exe).read_bytes(),
                           Path(args.ovr).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
