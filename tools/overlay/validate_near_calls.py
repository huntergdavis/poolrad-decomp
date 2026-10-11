#!/usr/bin/env python3
"""Decode overlay paths to validate E8 near-call sites at instruction boundaries.

Requires Capstone 5 with 16-bit x86 support. Every Pascal-style prologue from
overlay_inventory.py is a path seed, so this verifies call instructions under
that candidate-start assumption; it does not prove that every seed is a real
function or that indirect dispatch has been exhausted. Emits no game bytes.
"""

import argparse
import hashlib
import json
from pathlib import Path

try:
    from capstone import (Cs, CS_ARCH_X86, CS_MODE_16, CS_GRP_CALL,
                          CS_GRP_IRET, CS_GRP_JUMP, CS_GRP_RET)
    from capstone.x86 import X86_OP_IMM
except ImportError as exc:
    raise SystemExit("Capstone 5 is required for this optional audit") from exc


def audit(inventory, callgraph, overlay):
    expected_hash = inventory["inputs"]["game_ovr_sha256"]
    if hashlib.sha256(overlay).hexdigest() != expected_hash:
        raise ValueError("GAME.OVR hash differs from the inventory")
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    prologues = set()
    decoded_positions = set()
    decoded_near = set()
    invalid_paths = set()
    truncated_paths = set()
    for unit in inventory["units"]:
        unit_id = unit["stub_para"]
        code = overlay[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
        starts = {f["rel"] for f in unit["functions"]}
        prologues.update((unit_id, start) for start in starts)
        cache = {}
        for seed in sorted(starts):
            pending = [seed]
            seen = set()
            steps = 0
            while pending and steps < 4000:
                pos = pending.pop()
                while 0 <= pos < len(code) and pos not in seen and steps < 4000:
                    if pos != seed and pos in starts:
                        break  # another candidate function begins here
                    seen.add(pos)
                    steps += 1
                    if pos not in cache:
                        cache[pos] = next(decoder.disasm(code[pos:pos + 15], pos, count=1), None)
                    insn = cache[pos]
                    if insn is None:
                        invalid_paths.add((unit_id, seed))
                        break
                    decoded_positions.add((unit_id, pos))
                    next_pos = pos + insn.size
                    if (insn.group(CS_GRP_CALL) and insn.operands and
                            insn.operands[0].type == X86_OP_IMM and insn.bytes[0] == 0xE8):
                        decoded_near.add((unit_id, pos, insn.operands[0].imm))
                    if insn.group(CS_GRP_RET) or insn.group(CS_GRP_IRET):
                        break
                    if insn.group(CS_GRP_JUMP):
                        if insn.operands and insn.operands[0].type == X86_OP_IMM:
                            target = insn.operands[0].imm
                            if 0 <= target < len(code):
                                pending.append(target)
                        if insn.mnemonic in ("jmp", "ljmp"):
                            break
                    pos = next_pos
            if steps >= 4000:
                truncated_paths.add((unit_id, seed))
    raw_exact = {
        (edge["unit"], edge["site"], edge["target"])
        for edge in callgraph["edges"]
        if edge["kind"] == "near_candidate" and edge.get("exact_prologue")
    }
    raw_other_count = sum(
        edge["kind"] == "near_candidate" and not edge.get("exact_prologue")
        for edge in callgraph["edges"]
    )
    decoded_exact = {
        (unit, site, target) for unit, site, target in decoded_near
        if (unit, target) in prologues
    }
    entries = {
        (unit["stub_para"], target)
        for unit in inventory["units"] for target in unit["entries"]
    }
    supported = prologues & (entries | {(unit, target) for unit, _, target in decoded_exact})
    return {
        "inputs": inventory["inputs"],
        "decoder": "Capstone 5, x86 16-bit",
        "candidate_prologue_seeds": len(prologues),
        "decoded_instruction_positions": len(decoded_positions),
        "invalid_seed_paths": len(invalid_paths),
        "truncated_seed_paths": len(truncated_paths),
        "decoded_direct_near_call_sites": len(decoded_near),
        "decoded_near_calls_to_prologues": len(decoded_exact),
        "decoded_distinct_prologue_targets": len({(u, t) for u, _, t in decoded_exact}),
        "raw_near_call_candidates_to_prologues": len(raw_exact),
        "raw_near_call_candidates_elsewhere": raw_other_count,
        "raw_exact_not_decoded": len(raw_exact - decoded_exact),
        "decoded_exact_not_raw": len(decoded_exact - raw_exact),
        "prologues_with_entry_or_decoded_near_call": len(supported),
        "prologues_without_entry_or_decoded_near_call": [
            {"unit": unit, "offset": offset} for unit, offset in sorted(prologues - supported)
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--callgraph", required=True)
    parser.add_argument("--ovr", required=True)
    parser.add_argument("--json", help="write the counts to this file")
    args = parser.parse_args()
    result = audit(json.loads(Path(args.inventory).read_text()),
                   json.loads(Path(args.callgraph).read_text()),
                   Path(args.ovr).read_bytes())
    output = json.dumps(result, indent=2) + "\n"
    if args.json:
        Path(args.json).write_text(output)
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
