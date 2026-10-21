#!/usr/bin/env python3
"""Hash-locked resident call-target inventory for the owner's DOS v1.3 build.

Only addresses, counts, evidence kinds, and hashes are emitted. The overlay
inventory supplies fixups and code bounds; no executable bytes are exported.
Mappings below apply only to the observed call targets, not whole segments.
"""

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re

from overlay_inventory import find_units, parse_mz


START_SHA256 = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA256 = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SYSTEM_SEGMENT = 0xAF8

# Empirical file-image shifts. Each range is bounded by called targets whose
# exact 55 89 E5 prologues were checked in this executable. Gaps stay unmapped.
MAPPED_RANGES = {
    0x0BA: ((0x0442, 0x2199, -292), (0x2228, 0x2B88, -316),
            (0x2C48, 0x2F57, -330)),
    0x3F1: ((0x0000, 0x02E8, -446),),
    0x464: ((0x00FF, 0x2A0D, -455),),
    0x709: ((0x017C, 0x0DD3, -455),),
    0x802: ((0x025D, 0x095D, -457),),
    0x899: ((0x0000, 0x0000, -457), (0x0059, 0x0122, -481)),
    0x8AE: ((0x00DC, 0x01A5, -489),),
}

# These targets operate on character sidecar lists, the loader-backed party
# list, or character-linked game state; see the RESIDENT_* evidence notes.
GAME_SPECIFIC_TARGETS = {(0x3F1, 0x0137), (0xBA, 0x17E3), (0x709, 0x0DD3), (0x709, 0x05EB), (0x709, 0x0CAB), (0x0BA, 0x1816),
                         (0x709, 0x01DF), (0x709, 0x0626),
                         (0x899, 0x0104),
                         (0x709, 0x0DBE),
                         (0x0BA, 0x1037), (0x0BA, 0x1509), (0x0BA, 0x15F7), (0x0BA, 0x2199),
                         (0x0BA, 0x0BB8), (0x0BA, 0x1168), (0x0BA, 0x13AB), (0x0BA, 0x23B7), (0x0BA, 0x23DB), (0x0BA, 0x2637), (0x464, 0x2595), (0x0BA, 0x1693),
                         (0x0BA, 0x26C2), (0x0BA, 0x2E03),
                         (0x0BA, 0x2E46), (0x0BA, 0x2E8B), (0x899, 0x0000), (0x899, 0x0059), (0x802, 0x02EB), (0x0BA, 0x0000), (0x0BA, 0x01FB), (0x0BA, 0x0282), (0x0BA, 0x03A0), (0x0BA, 0x10E6), (0x0BA, 0x11EA), (0x0BA, 0x1282), (0x0BA, 0x1325), (0x802, 0x0040)}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def image_offset(segment, offset):
    for lo, hi, shift in MAPPED_RANGES.get(segment, ()):
        if lo <= offset <= hi:
            return 16 * segment + offset + shift, shift
    return None, None


def last_overlay_segment(units):
    ordered = sorted(units, key=lambda unit: unit["descriptor_offset"])
    previous, last = ordered[-2:]
    require(previous["stub_para"] == 0x3D2 and last["stub_para"] == 0x40C,
            "unexpected final overlay descriptor order")
    require(previous["memory_segment"] == 0x3EE,
            "unexpected final overlay segment metadata")
    gap = last["descriptor_offset"] - previous["descriptor_offset"]
    predicted = previous["memory_segment"] + (gap + 15) // 16
    require(gap == 918 and predicted == 0x428 and last["entry_count"] == 3,
            "final overlay segment inference no longer holds")
    require(last["memory_segment"] == predicted and
            last["memory_segment_source"] == "inferred_descriptor_spacing",
            "shared overlay inventory disagrees about final segment inference")
    return last, predicted, gap


def runtime_anchors(image, header_bytes):
    # 0xAF8:B0B/B25 implement Pascal string copy and bounded copy;
    # 0xAF8:14FB/1502 select DOS AH=3F/40 and join one INT 21h path.
    anchors = ((0x0B0B, -1290, b"\xfc\x8b\xdc"),
               (0x0B25, -1290, b"\xfc\x8b\xdc"),
               (0x14FB, -1344, b"\xb4\x3f"),
               (0x1502, -1344, b"\xb4\x40"))
    result = []
    for offset, shift, prefix in anchors:
        pos = 16 * SYSTEM_SEGMENT + offset + shift
        require(image[pos:pos + len(prefix)] == prefix,
                f"runtime anchor 0x{offset:x} did not decode as expected")
        result.append(dict(id=f"SYS_AF8_{offset:04X}", segment=SYSTEM_SEGMENT,
                           offset=offset, file_offset=header_bytes + pos,
                           evidence="instruction_signature"))
    require(image[0xBF5E:0xBF60] == b"\xcd\x21",
            "runtime read/write pair does not reach the expected DOS interrupt")
    return result


def collect(exe, ovr):
    require(digest(exe) == START_SHA256 and digest(ovr) == OVR_SHA256,
            "unsupported DOS build: executable hashes differ from the audited version")
    mz = parse_mz(exe)
    image = exe[mz["header_bytes"]:]
    units = find_units(image, ovr)
    require(len(units) == 34 and sum(u["entry_count"] for u in units) == 404,
            "overlay inventory differs from the audited build")
    last, last_segment, descriptor_gap = last_overlay_segment(units)
    by_segment = {u["memory_segment"]: u for u in units
                  if u is not last and u["memory_segment"] is not None}
    require(last_segment not in by_segment, "inferred final segment collides")
    by_segment[last_segment] = last

    site_counts = Counter()
    targets = defaultdict(list)
    final_entry_counts = Counter()
    for unit in units:
        code = ovr[unit["ovr_offset"]:unit["ovr_offset"] + unit["code_size"]]
        for fixup in unit["fixups"]:
            if fixup < 3 or fixup + 2 > len(code) or code[fixup - 3] != 0x9A:
                continue
            site = fixup - 3
            offset = int.from_bytes(code[site + 1:site + 3], "little")
            segment = int.from_bytes(code[site + 3:site + 5], "little")
            source = dict(unit=unit["stub_para"], unit_offset=site,
                          ovr_file_offset=unit["ovr_offset"] + site)
            site_counts["far_fixup_confirmed"] += 1
            if segment in by_segment:
                target_unit = by_segment[segment]
                index, remainder = divmod(offset - 0x20, 5)
                require(remainder == 0 and 0 <= index < target_unit["entry_count"],
                        f"non-stub target in overlay segment 0x{segment:x}")
                if segment == last_segment:
                    site_counts["final_overlay_inferred"] += 1
                    final_entry_counts[index] += 1
                else:
                    site_counts["overlay_known"] += 1
            elif segment == SYSTEM_SEGMENT:
                site_counts["runtime_system"] += 1
                targets[(segment, offset)].append(source)
            else:
                site_counts["resident_non_system"] += 1
                targets[(segment, offset)].append(source)

    require(site_counts == Counter(far_fixup_confirmed=4012,
                                   overlay_known=1154, final_overlay_inferred=7,
                                   runtime_system=1790, resident_non_system=1061),
            f"far-call partition changed: {dict(site_counts)}")
    require(final_entry_counts == Counter({1: 4, 2: 3}),
            "unexpected final overlay entry distribution")

    resident, runtime = [], []
    for (segment, offset), sources in sorted(targets.items()):
        if segment == SYSTEM_SEGMENT:
            runtime.append(dict(id=f"SYS_{segment:04X}_{offset:04X}",
                                segment=segment, offset=offset,
                                call_sites=len(sources), evidence="fixup_confirmed_far_call",
                                classification="turbo_pascal_system"))
            continue
        pos, shift = image_offset(segment, offset)
        if pos is not None:
            require(0 <= pos <= len(image) - 3 and
                    image[pos:pos + 3] == b"\x55\x89\xe5",
                    f"mapped target {segment:04x}:{offset:04x} is not a Pascal prologue")
        classification = ("game_verified" if (segment, offset) in GAME_SPECIFIC_TARGETS
                          else "game_or_other_library_unresolved")
        resident.append(dict(id=f"RES_{segment:04X}_{offset:04X}",
                             segment=segment, offset=offset,
                             file_offset=None if pos is None else mz["header_bytes"] + pos,
                             image_offset=pos, mapping_shift=shift,
                             call_sites=len(sources), sources=sources,
                             evidence=["fixup_confirmed_far_call"] +
                                      (["exact_pascal_prologue"] if pos is not None else []),
                             classification=classification))

    require(len(runtime) == 48 and len(resident) == 86,
            "distinct resident target count changed")
    mapped_offsets = [x["file_offset"] for x in resident
                      if x["file_offset"] is not None]
    require(len(mapped_offsets) == 84,
            "mapped target count changed")
    require(len(set(mapped_offsets)) == len(mapped_offsets),
            "mapped resident targets alias the same file offset")
    require(sum(x["classification"] == "game_verified" for x in resident) == 29,
            "verified game subset changed")
    raw_prologues = len(list(re.finditer(b"\x55\x89\xe5", image)))
    require(raw_prologues == 132, "resident raw prologue count changed")
    return dict(inputs=dict(start_exe_sha256=digest(exe), game_ovr_sha256=digest(ovr),
                            start_exe_bytes=len(exe), game_ovr_bytes=len(ovr)),
                method=dict(resident_file_offsets="hash_locked_called_target_ranges_only",
                            final_overlay_segment=last_segment,
                            final_overlay_descriptor_gap=descriptor_gap,
                            runtime_segment=SYSTEM_SEGMENT),
                summary=dict(overlay_units=len(units), overlay_public_entries=404,
                             resident_image_bytes=len(image),
                             raw_resident_prologue_hits=raw_prologues,
                             fixup_call_sites=dict(site_counts),
                             runtime_distinct_targets=len(runtime),
                             non_system_resident_distinct_targets=len(resident),
                             mapped_exact_prologue_targets=84,
                             unmapped_called_targets=2,
                             resident_classifications=dict(
                                 game_verified=21,
                                 game_or_other_library_unresolved=66,
                                 other_library_verified=0),
                             game_specific_verified_lower_bound=10,
                             directly_called_game_candidate_range=[9, 86],
                             full_game_denominator="unresolved"),
                final_overlay_entries=dict(
                    (str(i), dict(target_unit_offset=last["entries"][i], call_sites=n))
                    for i, n in sorted(final_entry_counts.items())),
                runtime_anchors=runtime_anchors(image, mz["header_bytes"]),
                resident_targets=resident, runtime_targets=runtime)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", required=True, help="ignored owner-supplied START.EXE")
    parser.add_argument("--ovr", required=True, help="ignored owner-supplied GAME.OVR")
    parser.add_argument("--json", help="write address-only target ledger to this path")
    args = parser.parse_args()
    result = collect(Path(args.exe).read_bytes(), Path(args.ovr).read_bytes())
    if args.json:
        Path(args.json).write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result["summary"], indent=2))


if __name__ == "__main__":
    main()
