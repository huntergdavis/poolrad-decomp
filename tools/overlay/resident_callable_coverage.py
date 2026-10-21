#!/usr/bin/env python3
"""Audit bounded resident callable coverage; emit addresses and counts only.

Requires Capstone 5. The unpacked executable and decoded instructions stay in
memory. Candidate-seeded decoding is evidence, not an exhaustive function list.
"""

import argparse
from collections import Counter, defaultdict
import csv
import json
from pathlib import Path

from resident_inventory import collect, require
from resident_loader_map import audit, relocations, unpack, word


CRT_NEAR_STARTS = {0xA42E, 0xA497, 0xA4E5, 0xA6BD, 0xA731, 0xAA0B}
PROGRAM_START = 0xF6
CRT_INIT_START = 0xA400
EXIT_CALL = 0x14C


def path_walk(image, seeds):
    from capstone import (Cs, CS_ARCH_X86, CS_MODE_16, CS_GRP_CALL,
                          CS_GRP_IRET, CS_GRP_JUMP, CS_GRP_RET)
    from capstone.x86 import X86_OP_IMM

    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    decoded = {}
    near = defaultdict(set)
    far = set()
    indirect_calls = set()
    indirect_jumps = set()
    invalid = set()
    truncated = set()
    for seed in sorted(seeds):
        pending = [seed]
        visited = set()
        steps = 0
        while pending and steps < 4000:
            pos = pending.pop()
            while 0 <= pos < len(image) and pos not in visited and steps < 4000:
                if pos != seed and pos in seeds:
                    break
                instruction = next(decoder.disasm(bytes(image[pos:pos + 15]),
                                                   pos, count=1), None)
                if instruction is None:
                    invalid.add((seed, pos))
                    break
                visited.add(pos)
                steps += 1
                decoded[pos] = instruction.size
                next_pos = pos + instruction.size
                if instruction.group(CS_GRP_CALL):
                    operand = instruction.operands[0]
                    if operand.type == X86_OP_IMM and instruction.bytes[0] == 0xE8:
                        near[operand.imm].add(pos)
                    elif operand.type == X86_OP_IMM and instruction.bytes[0] == 0x9A:
                        far.add(pos)
                    else:
                        indirect_calls.add(pos)
                if pos == EXIT_CALL:
                    require(instruction.bytes[0] == 0x9A and
                            word(image, pos + 1) == 0xE9 and
                            word(image, pos + 3) == 0xAF8,
                            "program termination call differs")
                    break
                if instruction.group(CS_GRP_RET) or instruction.group(CS_GRP_IRET):
                    break
                if instruction.group(CS_GRP_JUMP):
                    if instruction.operands and instruction.operands[0].type == X86_OP_IMM:
                        target = instruction.operands[0].imm
                        if 0 <= target < len(image):
                            pending.append(target)
                    else:
                        indirect_jumps.add(pos)
                    if instruction.mnemonic in ("jmp", "ljmp"):
                        break
                pos = next_pos
        if steps >= 4000:
            truncated.add(seed)
    return dict(decoded=decoded, near=near, far=far,
                indirect_calls=indirect_calls, indirect_jumps=indirect_jumps,
                invalid=invalid, truncated=truncated)


def near_closure(image, initial):
    seeds = set(initial)
    for _ in range(12):
        walk = path_walk(image, seeds)
        new = {target for target in walk["near"] if 0 <= target < len(image)} - seeds
        if not new:
            return seeds, walk
        seeds.update(new)
    raise ValueError("near-call closure exceeded iteration bound")


def audit_coverage(exe, ovr):
    prior = audit(exe, ovr)
    image, origin, fill, loader = unpack(exe)
    sites, _ = relocations(exe, image)
    initial = {x["image_offset"] for x in
               prior["resident_targets"] + prior["unmatched_prologues"]}
    require(len(initial) == 134, "merged resident candidate set changed")

    # The loader copy begins with the far pointer 0000:0006. The copied
    # loader's final indirect jump uses that pointer after relocation.
    loader_file = loader["entry_file_offset"] - 0x12
    require(word(exe, loader_file) == 6 and word(exe, loader_file + 2) == 0,
            "loader transfer pointer changed")
    prefix = []
    pos = 6
    while pos < PROGRAM_START and image[pos] == 0x9A:
        require(pos + 3 in sites, "initialization far call lacks relocation")
        prefix.append(dict(site=pos, segment=word(image, pos + 3),
                           offset=word(image, pos + 1)))
        pos += 5
    require(len(prefix) == 48 and pos == PROGRAM_START and
            image[PROGRAM_START:PROGRAM_START + 3] == b"\x55\x89\xe5" and
            image[0xB121:0xB125] == b"\xB4\x4C\xCD\x21",
            "initialization chain or exit path changed")
    require(any(x["segment"] == 0xA40 and x["offset"] == 0 for x in prefix)
            and any(x["segment"] == 0xA31 and x["offset"] == 0 for x in prefix),
            "expected unit initialization targets missing")

    seeds, walk = near_closure(image, initial)
    near_only = sorted(seeds - initial)
    require(len(seeds) == 145 and len(near_only) == 11 and
            len(walk["near"]) == 79 and
            sum(map(len, walk["near"].values())) == 170 and
            not walk["invalid"] and not walk["truncated"],
            "candidate-seeded path audit changed")
    require(CRT_NEAR_STARTS <= set(near_only), "CRT helper targets changed")
    runtime_inventory = collect(exe, ovr)
    runtime_called = {16 * x["segment"] + x["offset"]
                      for x in runtime_inventory["runtime_targets"]}
    runtime_initial = runtime_called | {0xAF80, 0xB069}
    runtime_seeds, runtime_walk = near_closure(image, runtime_initial)
    require(len(runtime_called) == 48 and len(runtime_initial) == 50 and
            len(runtime_seeds) == 80 and len(runtime_walk["decoded"]) == 1827 and
            sum(map(len, runtime_walk["near"].values())) == 69 and
            runtime_walk["indirect_calls"] ==
            {0xB273, 0xB337, 0xB343, 0xB581, 0xB77B, 0xB80C} and
            not runtime_walk["invalid"] and not runtime_walk["truncated"],
            "SYSTEM candidate path audit changed")
    # Startup writes the two words of [0x4414] at 0xAFB7 and 0xAFBD.
    # Its two call sites may be resolved only while this pointer is unchanged.
    require(image[0xAFB7] == 0xC7 and word(image, 0xAFB9) == 0x4414 and
            word(image, 0xAFBB) == 0xA9 and
            image[0xAFBD] == 0x8C and word(image, 0xAFBF) == 0x4416 and
            image[0xB02B:0xB02E] == b"\xCA\x02\x00",
            "SYSTEM callback initialization changed")
    far_sites = {site - 3 for site in sites if site >= 3 and image[site - 3] == 0x9A}
    require(len(far_sites) == 547 and walk["far"] <= far_sites,
            "relocation-backed far calls changed")
    prefix_sites = {x["site"] for x in prefix}
    unwalked_far = sorted(far_sites - walk["far"] - prefix_sites)
    require(walk["far"].isdisjoint(prefix_sites) and
            unwalked_far == [0x8A9C] and
            word(image, 0x8A9D) == 0x2FD and
            word(image, 0x8A9F) == 0xA40 and
            image[0x8A9A:0x8A9C] == b"\xEB\x12" and
            image[0x8AAC:0x8AAE] == b"\xEB\xEE",
            "unreached far-call block changed")
    decoded_bytes = {byte for position, length in walk["decoded"].items()
                     for byte in range(position, position + length)}
    runtime_bytes = {byte for position, length in runtime_walk["decoded"].items()
                     for byte in range(position, position + length)}
    bins = []
    for start in range(0, len(image), 0x1000):
        stop = min(start + 0x1000, len(image))
        bins.append(dict(start=start, end_exclusive=stop,
                         decoded_bytes=sum(start <= p < stop for p in decoded_bytes),
                         system_decoded_bytes=sum(start <= p < stop
                                                  for p in runtime_bytes),
                         initialization_bytes=sum(start <= p < stop
                                                  for p in range(6, PROGRAM_START))))

    rows = []
    for item in prior["resident_targets"]:
        classification = item["classification"]
        if classification not in ("game_verified", "linked_crt_inferred"):
            classification = "unresolved"
        rows.append(dict(id=item["id"], kind="overlay_called",
                         image_offset=item["image_offset"],
                         source_file_offset=item["source_file_offset"],
                         boundary=item["instruction_boundary"],
                         overlay_far_sites=item["incoming_overlay_calls"],
                         loader_far_sites=item["relocated_far_call_sites"],
                         decoded_near_sites=len(walk["near"].get(item["image_offset"], ())),
                         init_fallthrough=0,
                         classification=classification,
                         confidence=item["classification_confidence"]))
    for item in prior["unmatched_prologues"]:
        start = item["image_offset"]
        classification = ("game_verified" if start == PROGRAM_START else
                          "linked_crt_inferred" if start == CRT_INIT_START else
                          "unresolved")
        rows.append(dict(id=item["id"], kind="other_prologue",
                         image_offset=start,
                         source_file_offset=item["source_file_offset"],
                         boundary="raw_pascal_prologue",
                         overlay_far_sites=0,
                         loader_far_sites=len(item["relocated_far_callers"]),
                         decoded_near_sites=len(walk["near"].get(start, ())),
                         init_fallthrough=int(start == PROGRAM_START),
                         classification=classification,
                         confidence=("verified_entry" if start == PROGRAM_START else
                                     "high_inference" if start == CRT_INIT_START else
                                     "candidate")))
    for start in near_only:
        require(origin[start] is not None, "near target has no source provenance")
        rows.append(dict(id=f"RNEAR_{start:05X}", kind="near_only_start",
                         image_offset=start,
                         source_file_offset=0x200 + origin[start],
                         boundary="direct_near_target",
                         overlay_far_sites=0,
                         loader_far_sites=sum(16 * word(image, site + 3) +
                                              word(image, site + 1) == start
                                              for site in far_sites),
                         decoded_near_sites=len(walk["near"].get(start, ())),
                         init_fallthrough=0,
                         classification=("linked_crt_inferred" if start in CRT_NEAR_STARTS
                                         else "unresolved"),
                         confidence=("high_inference" if start in CRT_NEAR_STARTS
                                     else "candidate")))
    classes = Counter(row["classification"] for row in rows)
    require(len(rows) == 145 and classes == Counter(
        game_verified=30, linked_crt_inferred=9, unresolved=106),
        "candidate classification partition changed")
    return dict(inputs=prior["inputs"],
                summary=dict(merged_overlay_called=86, merged_exact_prologues=84,
                             merged_after_retf_starts=2, merged_other_prologues=48,
                             near_only_starts=11, audited_candidate_starts=145,
                             game_verified=classes["game_verified"],
                             linked_crt_inferred=classes["linked_crt_inferred"],
                             system_runtime_within_candidate_set=0,
                             data_false_hits_proven=0,
                             unresolved=classes["unresolved"],
                             separately_known_system_targets=48,
                             resident_game_lower_bound=11,
                             whole_game_denominator="unresolved"),
                startup=dict(loader_jump_target_image_offset=6,
                             initialization_far_calls=len(prefix),
                             initialization_first_site=prefix[0]["site"],
                             initialization_last_site=prefix[-1]["site"],
                             program_entry_image_offset=PROGRAM_START,
                             exit_call_image_offset=EXIT_CALL,
                             dos_terminate_image_offset=0xB121,
                             selected_targets=[x for x in prefix if
                                               (x["segment"], x["offset"]) in
                                               {(0xA40, 0), (0xA31, 0),
                                                (0xBA, 0x309E)}]),
                calls=dict(decoded_direct_near_sites=sum(map(len, walk["near"].values())),
                           decoded_direct_near_targets=len(walk["near"]),
                           loader_relocation_backed_far_sites=len(far_sites),
                           far_sites_on_candidate_paths=len(walk["far"]),
                           far_sites_in_initialization_prefix=len(prefix_sites),
                           far_sites_outside_both=unwalked_far,
                           decoded_indirect_call_sites=sorted(walk["indirect_calls"]),
                           decoded_indirect_jump_sites=sorted(walk["indirect_jumps"])),
                runtime_paths=dict(overlay_called_system_targets=len(runtime_called),
                                   startup_and_exit_seeds=2,
                                   initial_seeds=len(runtime_initial),
                                   direct_near_closure_seeds=len(runtime_seeds),
                                   decoded_instruction_starts=len(runtime_walk["decoded"]),
                                   decoded_direct_near_sites=sum(map(len, runtime_walk["near"].values())),
                                   decoded_indirect_call_sites=sorted(runtime_walk["indirect_calls"]),
                                   startup_initialized_pointer_address=0x4414,
                                   pointer_write_image_offsets=[0xAFB7, 0xAFBD],
                                   pointer_initial_segment=0xAF8,
                                   pointer_initial_offset=0xA9,
                                   conditional_pointer_call_sites=[0xB337, 0xB343],
                                   other_indirect_call_sites=[0xB273, 0xB581,
                                                              0xB77B, 0xB80C]),
                coverage=dict(loaded_image_bytes=len(image),
                              candidate_path_instruction_starts=len(walk["decoded"]),
                              candidate_path_decoded_bytes=len(decoded_bytes),
                              initialization_prefix_bytes=PROGRAM_START - 6,
                              decoded_copy_bytes=sum(not fill[p] for p in decoded_bytes),
                              decoded_fill_bytes=sum(fill[p] for p in decoded_bytes),
                              system_candidate_decoded_bytes=len(runtime_bytes),
                              shared_candidate_and_system_bytes=len(decoded_bytes & runtime_bytes),
                              bins=bins),
                rows=rows,
                # Expose the already-audited SYSTEM closure as addresses only.
                # These starts are not new game identities; aliases are joined
                # by loaded image address by function_queue.py.
                runtime_rows=[dict(
                    id=(f"SYS_0AF8_{start - 0xAF80:04X}" if start in runtime_called
                        else f"SYSNEAR_{start:05X}"),
                    image_offset=start,
                    boundary="decoded_system_closure_seed",
                    overlay_far_sites=sum(item["call_sites"] for item in runtime_inventory["runtime_targets"]
                                          if 16 * item["segment"] + item["offset"] == start),
                    loader_far_sites=sum(16 * word(image, site + 3) +
                                         word(image, site + 1) == start for site in far_sites),
                    decoded_near_sites=len(runtime_walk["near"].get(start, ())),
                    init_fallthrough=int(start == 0xAF80),
                    classification="linked_runtime_evidenced")
                    for start in sorted(runtime_seeds)])


FIELDS = ("id", "kind", "image_offset", "source_file_offset", "boundary",
          "overlay_far_sites", "loader_far_sites", "decoded_near_sites",
          "init_fallthrough", "classification", "confidence")


def write_ledger(path, rows):
    with Path(path).open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=FIELDS, lineterminator="\n")
        writer.writeheader()
        for row in rows:
            formatted = dict(row)
            for key in ("image_offset", "source_file_offset"):
                formatted[key] = f"0x{formatted[key]:05X}"
            writer.writerow(formatted)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored owner START.EXE")
    parser.add_argument("--ovr", required=True, help="ignored owner GAME.OVR")
    parser.add_argument("--json", help="write address-only audit JSON")
    parser.add_argument("--csv", help="write address-only candidate ledger")
    args = parser.parse_args()
    result = audit_coverage(Path(args.exe).read_bytes(), Path(args.ovr).read_bytes())
    if args.json:
        Path(args.json).write_text(json.dumps(result, indent=2) + "\n")
    if args.csv:
        write_ledger(args.csv, result["rows"])
    print(json.dumps({key: result[key] for key in
                      ("summary", "startup", "calls", "runtime_paths")}, indent=2))
    print(json.dumps({key: value for key, value in result["coverage"].items()
                      if key != "bins"}, indent=2))


if __name__ == "__main__":
    main()
