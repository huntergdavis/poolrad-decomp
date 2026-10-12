#!/usr/bin/env python3
"""Audit RES_0464:02A5 route-0 loop continuation and backedges."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

from resident_inventory import START_SHA256, digest, require
from resident_loader_map import unpack
from resident_464_mode0_second_audit import collect as collect_second


BASE = 0x04640
SPANS = (
    ("loop_gate", 0x05CB, 0x05EF, 13,
     "a186da72ac0767e72fbefab8c2e06829f5d63edb2a6bf35a8beb0de988d9d4d5"),
    ("layer_backedge", 0x02EC, 0x0328, 23,
     "9ec211aeff74791df4c55e89e758139066bf9f278d09a6487835be3abaa4308e"),
    ("x_backedge", 0x0308, 0x0328, 12,
     "0e579be0b811f9fa79302e6125dec28616dca2204d8bdeb8bd6e21711ed85753"),
    ("y_backedge", 0x0325, 0x0328, 1,
     "836fb65002fd742d43b2733338df4103d534419c2f3a08eed093b6cd5a46d0e0"),
    ("common_return", 0x08CE, 0x08D4, 3,
     "90f66eae99f51c421d8db68fa3d7a7629f728669d6c3fd06db0bb96e74651939"),
)
EDGES = (
    (0x05D1, "je", "0x4c16"), (0x05D3, "jmp", "0x4965"),
    (0x05DC, "je", "0x4c21"), (0x05DE, "jmp", "0x4948"),
    (0x05E7, "je", "0x4c2c"), (0x05E9, "jmp", "0x492c"),
    (0x05EC, "jmp", "0x4f0e"),
)


def collect(exe):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    require(digest(exe) == START_SHA256, "unsupported START.EXE")
    second = collect_second(exe)
    require(second["sha256"] ==
            "e72925f180113d69b94255931a7db9152030c3e4d858a74f2861708108d1b497",
            "second primary-byte parent changed")
    image, origin, _, _ = unpack(exe)
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    outputs = []
    decoded = {}
    for name, start, end, expected_count, expected_hash in SPANS:
        lo, hi = BASE + start, BASE + end
        require(sha256(bytes(image[lo:hi])).hexdigest() ==
                expected_hash, f"{name} span changed")
        require(all(origin[i] is not None and origin[i + 1] == origin[i] + 1
                    for i in range(lo, hi - 1)),
                f"{name} source mapping changed")
        ins = list(md.disasm(bytes(image[lo:hi]), lo))
        require(len(ins) == expected_count and ins[0].address == lo and
                ins[-1].address + ins[-1].size == hi,
                f"{name} instruction coverage changed")
        decoded[name] = {i.address - BASE: i for i in ins}
        outputs.append({
            "name": name, "image_start": lo, "image_end_exclusive": hi,
            "source_file_start": 0x200 + origin[lo],
            "source_file_end_exclusive": 0x200 + origin[hi - 1] + 1,
            "bytes": hi - lo, "sha256": expected_hash,
            "instructions": len(ins),
        })

    gate = decoded["loop_gate"]
    require(all((gate[o].mnemonic, gate[o].op_str) == (m, p)
                for o, m, p in EDGES), "loop branch targets changed")
    require((decoded["layer_backedge"][0x02EC].mnemonic,
             decoded["layer_backedge"][0x02EC].op_str) ==
            ("inc", "byte ptr [bp - 0x16]") and
            (decoded["x_backedge"][0x0308].mnemonic,
             decoded["x_backedge"][0x0308].op_str) ==
            ("inc", "word ptr [bp - 0x1a]") and
            (decoded["y_backedge"][0x0325].mnemonic,
             decoded["y_backedge"][0x0325].op_str) ==
            ("inc", "word ptr [bp - 0x18]"),
            "backedge increment semantics changed")
    require((decoded["layer_backedge"][0x02F2].mnemonic,
             decoded["layer_backedge"][0x02F2].op_str) ==
            ("mov", "ax, word ptr es:[di]") and
            (decoded["x_backedge"][0x030E].mnemonic,
             decoded["x_backedge"][0x030E].op_str) ==
            ("mov", "ax, word ptr es:[di + 2]"),
            "dimension reload sites changed")
    epilogue = decoded["common_return"]
    require([(epilogue[o].mnemonic, epilogue[o].op_str)
             for o in (0x08CE, 0x08D0, 0x08D1)] ==
            [("mov", "sp, bp"), ("pop", "bp"), ("retf", "0xe")],
            "common return changed")
    entry = list(md.disasm(bytes(image[BASE + 0x02D3:BASE + 0x02DD]),
                            BASE + 0x02D3))
    require([(i.mnemonic, i.op_str) for i in entry] ==
            [("les", "di, ptr [bp - 0x28]"),
             ("mov", "al, byte ptr es:[di + 8]"),
             ("mov", "byte ptr [bp - 0x29], al")],
            "entry count snapshot changed")
    return {
        "start_sha256": START_SHA256,
        "route": "RES_0464:02A5/video_state_0/loop_continuation",
        "spans": outputs,
        "gate_edges": [
            {"offset": o, "kind": m, "target": p} for o, m, p in EDGES
        ],
        "entry_count_local_offset": 0x02DA,
        "next_move_offset": 0x0328,
        "common_return_offset": 0x08CE,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored DOS START.EXE")
    args = parser.parse_args()
    print(json.dumps(collect(Path(args.exe).read_bytes()), indent=2))


if __name__ == "__main__":
    main()
