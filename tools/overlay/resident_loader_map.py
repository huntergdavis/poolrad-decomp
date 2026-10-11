#!/usr/bin/env python3
"""Trace DOS v1.3 START.EXE's loader into address-only resident evidence.

The reconstructed image stays in memory. JSON contains offsets, counts, hashes,
and evidence labels only; it never contains executable bytes or decoded text.
"""

import argparse
from collections import Counter, defaultdict
import csv
import json
from pathlib import Path
import re

from overlay_inventory import find_units
from resident_inventory import START_SHA256, OVR_SHA256, collect, digest, require


ENTRY_CS = 0xF72
ENTRY_IP = 0x12
HEADER_BYTES = 0x200
LOADER_COPY_BYTES = 0x5AF
LOADER_DEST_PARAS = 0x10BF
RELOCATION_TABLE_OFFSET = 0x12D


def word(data, offset):
    require(0 <= offset <= len(data) - 2, "loader word is outside input")
    return int.from_bytes(data[offset:offset + 2], "little")


def unpack(exe):
    """Follow the entry's backward B0/B2 expansion and return source provenance."""
    require(digest(exe) == START_SHA256, "unsupported START.EXE hash")
    require(exe[:2] == b"MZ" and word(exe, 6) == 0 and
            word(exe, 8) * 16 == HEADER_BYTES and
            word(exe, 20) == ENTRY_IP and word(exe, 22) == ENTRY_CS,
            "MZ loader entry differs from the audited build")
    loader_file = HEADER_BYTES + 16 * ENTRY_CS
    require(word(exe, loader_file) == 6 and word(exe, loader_file + 2) == 0 and
            word(exe, loader_file + 6) == LOADER_COPY_BYTES and
            word(exe, loader_file + 12) == LOADER_DEST_PARAS and
            word(exe, loader_file + 14) == 1,
            "startup loader parameters differ from the audited build")
    require(loader_file + LOADER_COPY_BYTES == len(exe),
            "copied startup loader does not end at EOF")

    end = (LOADER_DEST_PARAS - 1) * 16 + 15
    raw = exe[HEADER_BYTES:]
    image = bytearray(raw) + bytearray(max(0, end + 1 - len(raw)))
    origin = list(range(len(raw))) + [None] * (len(image) - len(raw))
    fill = bytearray(len(image))
    source = 16 * (ENTRY_CS - 1) + 15
    for _ in range(16):
        if image[source] != 0xFF:
            break
        source -= 1
    else:
        source += 1
    source_start = source
    dest = end
    runs = Counter()
    while True:
        require(2 <= source < len(image), "backward source escaped image")
        control = image[source]
        count = word(image, source - 2)
        source -= 3
        require(0 < count <= 0xFFFF and dest - count + 1 >= 0,
                "invalid backward expansion length")
        kind = control & 0xFE
        if kind == 0xB0:
            require(source >= 0, "fill byte escaped image")
            value = image[source]
            value_origin = origin[source]
            source -= 1
            for i in range(count):
                image[dest - i] = value
                origin[dest - i] = value_origin
                fill[dest - i] = 1
            runs["fill"] += 1
        elif kind == 0xB2:
            require(source - count + 1 >= 0, "copy escaped image")
            for i in range(count):
                image[dest - i] = image[source - i]
                origin[dest - i] = origin[source - i]
                fill[dest - i] = fill[source - i]
            source -= count
            runs["copy"] += 1
        else:
            raise ValueError(f"unknown backward expansion control at image offset {source + 3:#x}")
        dest -= count
        if control & 1:
            break
        require(sum(runs.values()) < 2000, "backward expansion exceeded run bound")
    require(source_start == 0xF71E and source == 0x150 and dest + 1 == 0x151 and
            runs == Counter(fill=449, copy=440),
            "backward expansion differs from the audited build")
    return image[:end + 1], origin[:end + 1], fill[:end + 1], dict(
        entry_file_offset=loader_file + ENTRY_IP,
        copied_loader_bytes=LOADER_COPY_BYTES,
        source_start_image_offset=source_start,
        source_end_image_offset=source,
        expanded_start_image_offset=dest + 1,
        expanded_end_image_offset=end,
        expansion_runs=dict(runs))


def relocations(exe, image):
    """Decode the entry's 16-group relocation stream, without applying load base."""
    cursor = HEADER_BYTES + 16 * ENTRY_CS + RELOCATION_TABLE_OFFSET
    table_start = cursor
    sites = []
    groups = []
    for group in range(16):
        count = word(exe, cursor)
        cursor += 2
        groups.append(count)
        for _ in range(count):
            site = group * 0x10000 + word(exe, cursor)
            cursor += 2
            require(site <= len(image) - 2, "relocation site escaped expanded image")
            sites.append(site)
    require(cursor == len(exe) and groups == [561] + [0] * 15 and
            len(set(sites)) == 561,
            "loader relocation table differs from the audited build")
    return sites, dict(table_file_offset=table_start, table_end_file_offset=cursor,
                       sites=len(sites), group_counts=groups)


def decoded_near_calls(image, seeds):
    """Optional candidate-seeded path walk, parallel to validate_near_calls.py."""
    try:
        from capstone import (Cs, CS_ARCH_X86, CS_MODE_16, CS_GRP_CALL,
                              CS_GRP_IRET, CS_GRP_JUMP, CS_GRP_RET)
        from capstone.x86 import X86_OP_IMM
    except ImportError as exc:
        raise SystemExit("Capstone 5 is required for --near") from exc
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    calls = defaultdict(set)
    decoded_positions = set()
    invalid_seeds = set()
    truncated_seeds = set()
    for seed in sorted(seeds):
        pending = [seed]
        seen = set()
        steps = 0
        while pending and steps < 4000:
            pos = pending.pop()
            while 0 <= pos < len(image) and pos not in seen and steps < 4000:
                if pos != seed and pos in seeds:
                    break
                seen.add(pos)
                steps += 1
                insn = next(decoder.disasm(bytes(image[pos:pos + 15]), pos, count=1), None)
                if insn is None:
                    invalid_seeds.add(seed)
                    break
                decoded_positions.add(pos)
                next_pos = pos + insn.size
                if (insn.group(CS_GRP_CALL) and insn.operands and
                        insn.operands[0].type == X86_OP_IMM and insn.bytes[0] == 0xE8):
                    calls[insn.operands[0].imm].add(pos)
                if insn.group(CS_GRP_RET) or insn.group(CS_GRP_IRET):
                    break
                if insn.group(CS_GRP_JUMP):
                    if insn.operands and insn.operands[0].type == X86_OP_IMM:
                        target = insn.operands[0].imm
                        if 0 <= target < len(image):
                            pending.append(target)
                    if insn.mnemonic in ("jmp", "ljmp"):
                        break
                pos = next_pos
        if steps >= 4000:
            truncated_seeds.add(seed)
    return calls, dict(decoder="Capstone 5, x86 16-bit",
                       candidate_seeds=len(seeds),
                       decoded_instruction_positions=len(decoded_positions),
                       invalid_seed_offsets=sorted(invalid_seeds),
                       truncated_seed_offsets=sorted(truncated_seeds),
                       decoded_direct_near_call_sites=sum(map(len, calls.values())))


def overlay_stub_words(exe, ovr, image):
    """Compare copied entry words with the live INT 3F stub operands."""
    units = find_units(exe[HEADER_BYTES:], ovr)
    differences = []
    deltas = Counter()
    for unit in units:
        segment = unit["memory_segment"]
        require(segment is not None, "overlay stub has no runtime segment")
        for index, raw in enumerate(unit["entries"]):
            pos = 16 * segment + 0x20 + 5 * index
            require(image[pos] == 0xCD and image[pos + 1] == 0x3F,
                    "overlay entry is not an INT 3F stub")
            live = word(image, pos + 2)
            delta = raw - live
            deltas[delta] += 1
            if delta:
                differences.append(dict(unit=unit["stub_para"], entry_index=index,
                                        stub_image_offset=pos, raw_entry_offset=raw,
                                        live_stub_offset=live, difference=delta))
    require(deltas == Counter({0: 399, 0x800: 5}),
            "overlay raw-to-stub entry differences changed")
    return dict(entry_count=sum(deltas.values()), equal_words=deltas[0],
                raw_plus_0x800_words=deltas[0x800], differences=differences)


def audit(exe, ovr, near=False):
    require(digest(ovr) == OVR_SHA256, "unsupported GAME.OVR hash")
    previous = collect(exe, ovr)
    image, origin, fill, loader = unpack(exe)
    reloc_sites, reloc = relocations(exe, image)
    stubs = overlay_stub_words(exe, ovr, image)
    prologues = set(m.start() for m in re.finditer(b"\x55\x89\xe5", image))
    require(len(prologues) == 132, "expanded prologue count changed")

    far_sites = defaultdict(list)
    for site in reloc_sites:
        if site < 3 or image[site - 3] != 0x9A:
            continue
        offset = word(image, site - 2)
        segment = word(image, site)
        far_sites[16 * segment + offset].append(
            dict(call_image_offset=site - 3, target_segment=segment,
                 target_offset=offset))
    require(sum(map(len, far_sites.values())) == 547,
            "relocation-backed far-call count changed")

    targets = []
    called_prologues = set()
    for item in previous["resident_targets"]:
        target = 16 * item["segment"] + item["offset"]
        require(target < len(image) and origin[target] is not None,
                f"resident target {item['id']} has no source provenance")
        source_file = HEADER_BYTES + origin[target]
        if item["file_offset"] is not None:
            require(item["file_offset"] == source_file,
                    f"earlier empirical mapping disagrees for {item['id']}")
        prologue = target in prologues
        if prologue:
            require(all(origin[target + i] == origin[target] + i
                        for i in range(3)) and
                    exe[source_file:source_file + 3] == b"\x55\x89\xe5",
                    f"packed prologue bytes are not contiguous for {item['id']}")
            called_prologues.add(target)
        else:
            require(item["segment"] == 0xA40 and
                    item["offset"] in (0x029C, 0x02FD) and
                    image[target - 1] == 0xCB,
                    f"unexpected nonprologue target {item['id']}")
        classification = ("linked_crt_inferred" if item["segment"] == 0xA40
                          else item["classification"])
        targets.append(dict(id=item["id"], segment=item["segment"],
                            offset=item["offset"], image_offset=target,
                            source_file_offset=source_file,
                            packed_source_shift=origin[target] - target,
                            source_kind="fill" if fill[target] else "copy",
                            incoming_overlay_calls=item["call_sites"],
                            relocated_far_call_sites=len(far_sites.get(target, ())),
                            instruction_boundary=("exact_pascal_prologue" if prologue
                                                  else "immediately_after_retf"),
                            classification=classification,
                            classification_confidence=("high_inference" if
                                                       item["segment"] == 0xA40
                                                       else "verified" if
                                                       classification == "game_verified"
                                                       else "unresolved")))
    require(len(targets) == 86 and len(called_prologues) == 84 and
            len({x["source_file_offset"] for x in targets}) == 86 and
            all(x["source_kind"] == "copy" for x in targets),
            "called-target source mapping differs from the audited build")

    near_calls, near_result = ({}, None)
    if near:
        seeds = prologues | {x["image_offset"] for x in targets}
        near_calls, near_result = decoded_near_calls(image, seeds)
    unmatched = []
    for target in sorted(prologues - called_prologues):
        require(origin[target] is not None, "raw prologue has no file provenance")
        callers = far_sites.get(target, [])
        near_callers = sorted(near_calls.get(target, ()))
        unmatched.append(dict(id=f"RPRO_{target:05X}", image_offset=target,
                              source_file_offset=HEADER_BYTES + origin[target],
                              source_kind="fill" if fill[target] else "copy",
                              relocated_far_callers=callers,
                              decoded_near_call_sites=near_callers if near else None,
                              evidence=(["raw_prologue"] +
                                        (["relocation_backed_far_call"] if callers else []) +
                                        (["candidate_seeded_decoded_near_call"]
                                         if near_callers else [])),
                              classification="unresolved"))
    require(len(unmatched) == 48 and
            sum(bool(x["relocated_far_callers"]) for x in unmatched) == 18,
            "unmatched prologue evidence count changed")
    if near:
        require(near_result["candidate_seeds"] == 134 and
                near_result["decoded_instruction_positions"] == 12909 and
                near_result["decoded_direct_near_call_sites"] == 158 and
                sum(bool(x["decoded_near_call_sites"]) for x in unmatched) == 31,
                "candidate-seeded near-call audit changed")
        near_result["unmatched_with_decoded_near_call"] = 31
        near_result["unmatched_with_either_call_kind"] = sum(
            bool(x["relocated_far_callers"] or x["decoded_near_call_sites"])
            for x in unmatched)
        near_result["unmatched_with_neither_call_kind"] = 48 - near_result[
            "unmatched_with_either_call_kind"]
    return dict(inputs=previous["inputs"], loader=loader, relocations=reloc,
                overlay_stub_words=stubs,
                summary=dict(previous_overlay_called_targets=86,
                             previous_empirical_exact_prologue_mappings=84,
                             previous_unmapped_targets=2,
                             loader_mapped_overlay_called_targets=86,
                             exact_prologue_called_targets=84,
                             after_retf_nonprologue_called_targets=2,
                             expanded_raw_prologue_hits=132,
                             prologues_without_overlay_caller=48,
                             unmatched_with_relocated_far_call=18,
                             unmatched_without_relocated_far_call=30,
                             resident_relocation_backed_far_call_sites=547,
                             game_verified=2, linked_crt_inferred=2,
                             remaining_overlay_called_classification_unresolved=82,
                             full_game_denominator="unresolved"),
                near_audit=near_result,
                resident_targets=targets, unmatched_prologues=unmatched)


LEDGER_FIELDS = ("id", "kind", "segment", "offset", "image_offset",
                 "source_file_offset", "packed_source_shift", "boundary",
                 "overlay_far_sites", "loader_far_sites", "decoded_near_sites",
                 "classification", "confidence")


def write_ledger(path, result):
    with Path(path).open("w", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=LEDGER_FIELDS, lineterminator="\n")
        writer.writeheader()
        for item in result["resident_targets"]:
            writer.writerow(dict(id=item["id"], kind="overlay_called",
                                 segment=f"0x{item['segment']:04X}",
                                 offset=f"0x{item['offset']:04X}",
                                 image_offset=f"0x{item['image_offset']:05X}",
                                 source_file_offset=f"0x{item['source_file_offset']:05X}",
                                 packed_source_shift=item["packed_source_shift"],
                                 boundary=item["instruction_boundary"],
                                 overlay_far_sites=item["incoming_overlay_calls"],
                                 loader_far_sites=item["relocated_far_call_sites"],
                                 decoded_near_sites="",
                                 classification=item["classification"],
                                 confidence=item["classification_confidence"]))
        for item in result["unmatched_prologues"]:
            writer.writerow(dict(id=item["id"], kind="prologue_without_overlay_caller",
                                 segment="", offset="",
                                 image_offset=f"0x{item['image_offset']:05X}",
                                 source_file_offset=f"0x{item['source_file_offset']:05X}",
                                 packed_source_shift=(item["source_file_offset"] -
                                                      HEADER_BYTES - item["image_offset"]),
                                 boundary="raw_pascal_prologue",
                                 overlay_far_sites=0,
                                 loader_far_sites=len(item["relocated_far_callers"]),
                                 decoded_near_sites=("" if item["decoded_near_call_sites"] is None
                                                     else len(item["decoded_near_call_sites"])),
                                 classification="unresolved",
                                 confidence="candidate"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored owner-supplied START.EXE")
    parser.add_argument("--ovr", required=True, help="ignored owner-supplied GAME.OVR")
    parser.add_argument("--json", help="write address-only evidence to this path")
    parser.add_argument("--csv", help="write address-only public candidate ledger")
    parser.add_argument("--near", action="store_true",
                        help="also run optional Capstone 5 candidate-seeded near-call audit")
    args = parser.parse_args()
    result = audit(Path(args.exe).read_bytes(), Path(args.ovr).read_bytes(), args.near)
    if args.json:
        Path(args.json).write_text(json.dumps(result, indent=2) + "\n")
    if args.csv:
        write_ledger(args.csv, result)
    print(json.dumps(result["summary"], indent=2))
    if result["near_audit"]:
        print(json.dumps(result["near_audit"], indent=2))


if __name__ == "__main__":
    main()
