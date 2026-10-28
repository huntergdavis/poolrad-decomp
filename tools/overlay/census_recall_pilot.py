#!/usr/bin/env python3
"""Bounded independent call-candidate pilot; private disassembly stays in resources.

This does not estimate whole-game recall or classify game ownership. Discovery
scans every byte before consulting the old start set; raw E8 bytes may be data.
The shared loader unpacker is a disclosed common dependency, not independent
evidence for its reconstruction. No original instructions execute.
"""
# NOT_A_ROW_CHECKER: static sampling experiment; no parity or credit verdicts
import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

from resident_loader_map import unpack, relocations

ROOT = Path(__file__).resolve().parents[2]
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
QUEUE_SHA = "ce7a5462da1f8f8b85c187c1d476a3b8fc8c4ddd0a6877ab5b5c638520ca3049"
SEED = "t248-pilot-v1-2026-10-27"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def word(data, at):
    return int.from_bytes(data[at:at + 2], "little")


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def descriptors(image, overlay):
    """Container headers only: no prologues, start inventory or census seeds."""
    out = {}
    for m in re.finditer(re.escape(b"\xcd\x3f\x00\x00"), image):
        d = m.start()
        if d + 14 > len(image):
            continue
        off, size, fix, count = struct.unpack_from("<IHHH", image, d + 4)
        if off >= 8 and size and fix % 2 == 0 and off + size + fix <= len(overlay) and 0 < count <= 512:
            # A CD3F-like word alone is not a container. Validate the complete
            # entry-stub array in the packed or expanded header layout.
            headers = [h for h in (22, 23, 24, 32)
                       if d + h + 5 * count <= len(image) and all(
                           image[d + h + 5 * j:d + h + 5 * j + 2] == b"\xcd\x3f"
                           for j in range(count))]
            if not headers:
                continue
            if len(headers) != 1:
                raise ValueError("ambiguous descriptor header size")
            if off in out:
                raise ValueError("ambiguous descriptor offset")
            out[off] = dict(descriptor=d, offset=off, size=size, fixup_bytes=fix,
                            entries=count, header_bytes=headers[0])
    return out


def descriptor_controls():
    """Literal 3F must not become regex '?'; incomplete stub arrays fail closed."""
    image = bytearray(37)
    image[:4] = b"\xcd\x3f\x00\x00"
    struct.pack_into("<IHHH", image, 4, 8, 4, 0, 1)
    image[32:37] = b"\xcd\x3f\x00\x00\x00"
    overlay = bytes(12)
    found = descriptors(image, overlay)
    require(set(found) == {8} and found[8]["descriptor"] == 0, "literal marker control failed")
    require(found[8]["header_bytes"] == 32, "expanded header control failed")
    broken = bytearray(image)
    broken[1] = 0x3E
    require(descriptors(broken, overlay) == {}, "wrong marker was accepted")
    broken = bytearray(image)
    broken[33] = 0x3E
    require(descriptors(broken, overlay) == {}, "incomplete stub array was accepted")


def near_candidates(data):
    """Raw E8 rel16, no boundary assumptions; no CS:IP wrap reconstruction."""
    for site in range(len(data) - 2):
        if data[site] == 0xE8:
            target = site + 3 + int.from_bytes(data[site + 1:site + 3], "little", signed=True)
            if 0 <= target < len(data):
                yield site, target


def rank(value):
    return sha((SEED + "\0" + value).encode())


def objdump(data, origin, path):
    binary = path.with_suffix(".bin")
    binary.write_bytes(data)
    result = subprocess.run(["objdump", "-D", "-b", "binary", "-m", "i8086",
                             "--no-show-raw-insn", f"--adjust-vma={origin}", str(binary)],
                            check=True, text=True, capture_output=True)
    path.write_text(result.stdout)
    instructions = {}
    for line in result.stdout.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(.+)", line)
        if m:
            instructions[int(m.group(1), 16)] = m.group(2).strip()
    return instructions


def address(identity):
    if identity.startswith("OVL_"):
        _, unit, offset = identity.split("_")
        return "O" + unit, int(offset, 16)
    if identity.startswith("RES_IMAGE_"):
        return "R", int(identity[10:], 16)
    _, segment, offset = identity.split("_")
    return "R", 16 * int(segment, 16) + int(offset, 16)


def identity(region, offset):
    return f"OVL_{region[1:]}_{offset:04X}" if region.startswith("O") else f"RES_IMAGE_{offset:05X}"


def run(args):
    descriptor_controls()
    output = args.output.resolve()
    if not output.is_relative_to((ROOT / "resources").resolve()):
        raise ValueError("private output must be under this worktree's ignored resources/")
    output.mkdir(parents=True, exist_ok=True)
    exe, ovr, queue_bytes = args.exe.read_bytes(), args.ovr.read_bytes(), args.queue.read_bytes()
    for blob, expected in [(exe, EXE_SHA), (ovr, OVR_SHA), (queue_bytes, QUEUE_SHA)]:
        if sha(blob) != expected:
            raise ValueError("unsupported input hash")
    image = bytes(unpack(exe)[0])
    packed = descriptors(exe[0x200:], ovr)
    expanded = descriptors(image, ovr)
    if len(packed) != 34 or len(expanded) != 35:
        raise ValueError(f"container count differs: packed={len(packed)}, expanded={len(expanded)}")
    regions, stubs, unit_records = {"R": image}, {}, []
    cursor = 8
    for off, unit in sorted(expanded.items()):
        if off != cursor:
            raise ValueError("overlay container tiling gap")
        cursor = off + unit["size"] + unit["fixup_bytes"]
        canonical = packed[off]["descriptor"] // 16 if off in packed else unit["descriptor"] // 16
        name = f"O{canonical:04X}"
        regions[name] = ovr[off:off + unit["size"]]
        d = unit["descriptor"]
        if d % 16 or unit["header_bytes"] != 32:
            raise ValueError("expanded descriptor not paragraph aligned")
        for j in range(unit["entries"]):
            at = d + 32 + 5 * j
            if image[at:at + 2] != b"\xcd\x3f":
                raise ValueError("expanded stub not recognized")
            stubs[(d // 16, 32 + 5 * j)] = (name, word(image, at + 2))
        unit_records.append(dict(unit=canonical, **unit))
    if cursor != len(ovr):
        raise ValueError("overlay tail not accounted for by containers")

    # Finish discovery before loading any census-address set.
    candidates = defaultdict(list)
    for region, data in regions.items():
        for site, target in near_candidates(data):
            candidates[(region, target)].append(dict(region=region, site=site, kind="raw-E8"))
    def far(region, data, fixups):
        for fx in fixups:
            site = fx - 3
            if site < 0 or data[site] != 0x9A:
                continue
            target_offset, target_segment = word(data, site + 1), word(data, fx)
            dest = stubs.get((target_segment, target_offset), ("R", target_segment * 16 + target_offset))
            if dest[0] in regions and 0 <= dest[1] < len(regions[dest[0]]):
                candidates[dest].append(dict(region=region, site=site, kind="fixup-far"))
    for u in unit_records:
        off = u["offset"] + u["size"]
        fixups = [word(ovr, p) for p in range(off, off + u["fixup_bytes"], 2)]
        far(f"O{u['unit']:04X}", regions[f"O{u['unit']:04X}"], fixups)
    far("R", image, relocations(exe, image)[0])

    queue = json.loads(queue_bytes)["rows"]
    extras_path = ROOT / "docs/re/function_backlog_census_only.csv"
    extras = list(csv.DictReader(extras_path.open()))
    known = {address(r["id"]) for r in queue + extras}
    if len(queue) != 898 or len(extras) != 7 or len(known) != 905:
        raise ValueError("comparison set shape changed")
    anchors = {region: sorted(p for r, p in known if r == region) for region in regions}
    outside = set(candidates) - known
    selected = []
    for domain, count in [("overlay", 32), ("resident", 16)]:
        pool = [p for p in outside if (p[0] != "R") == (domain == "overlay")]
        selected.extend(sorted(pool, key=lambda p: rank(identity(*p)))[:count])
    samples = []
    for region, offset in selected:
        name = identity(region, offset)
        data = regions[region]
        objdump(data[offset:offset + 192], offset, output / (name + "-target.txt"))
        sources = []
        for n, source in enumerate(candidates[(region, offset)]):
            row = dict(source)
            marks = anchors[source["region"]]
            j = bisect_right(marks, source["site"]) - 1
            anchor = marks[j] if j >= 0 else 0
            row["catalog_anchor"] = anchor
            if source["site"] - anchor <= 2048:
                code = regions[source["region"]]
                decoded = objdump(code[anchor:source["site"] + 8], anchor,
                                  output / (name + f"-source-{n}.txt"))
                instruction = decoded.get(source["site"], "")
                # Diagnostic only: linear alignment from a catalog anchor is
                # not independent proof of reachability or a function boundary.
                row["linear_call_aligned"] = bool(re.match(r"(?:lcall|call)\b", instruction))
            else:
                row["linear_call_aligned"] = None
            sources.append(row)
        samples.append(dict(id=name, rank=rank(name), target_window_sha256=sha(data[offset:offset + 192]),
                            target_window_bytes=len(data[offset:offset + 192]), sources=sources,
                            verdict="UNASSESSED"))

    excluded_path = ROOT / "docs/re/function_backlog_excluded.csv"
    excluded = list(csv.DictReader(excluded_path.open()))
    rejects = []
    for tier, count in [("SYSTEM", 4), ("CRT", 2), ("overlay_manager", 1)]:
        pool = [r for r in excluded if r["library_tier"].lower() == tier.lower()]
        if not pool:
            raise ValueError(f"missing reject stratum {tier}: {Counter(r['library_tier'] for r in excluded)}")
        for r in sorted(pool, key=lambda r: rank("reject:" + r["id"]))[:count]:
            _, at = address(r["id"])
            objdump(image[at:at + 384], at, output / (r["id"] + "-reject.txt"))
            rejects.append(dict(id=r["id"], tier=tier, window_bytes=len(image[at:at + 384]),
                                window_sha256=sha(image[at:at + 384]), verdict="UNASSESSED"))
    fake = b"\xb8\xe8\x00\x00\xc3"
    raw = list(near_candidates(fake))
    decoded = objdump(fake, 0, output / "synthetic-operand-control.txt")
    if raw != [(1, 4)] or 1 in decoded or 4 in decoded:
        raise ValueError("false raw-call control did not reject operand positions")
    hidden = []
    for domain in ("overlay", "resident"):
        pool = [p for p in set(candidates) & known if (p[0] != "R") == (domain == "overlay")]
        chosen = min(pool, key=lambda p: rank("hidden:" + identity(*p)))
        require(chosen in set(candidates) - (known - {chosen}), "hidden-start control failed")
        hidden.append(identity(*chosen))
    objdump(regions["O001C"][0x126:0x1A3], 0x126, output / "known-game-0126-control.txt")
    result = dict(method_version=1, seed=SEED, base_commit=subprocess.check_output(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip(),
        inputs=dict(exe=EXE_SHA, overlay=OVR_SHA, queue=QUEUE_SHA,
                    exclusions=sha(excluded_path.read_bytes()), extras=sha(extras_path.read_bytes())),
        tools=dict(objdump=subprocess.check_output(["objdump", "--version"], text=True).splitlines()[0],
                   sampler_sha256=sha(Path(__file__).read_bytes()),
                   unpacker_sha256=sha((ROOT / "tools/overlay/resident_loader_map.py").read_bytes())),
        containers=unit_records, resident_image_bytes=len(image),
        overlay_code_bytes=sum(len(v) for k, v in regions.items() if k != "R"),
        candidate_targets=len(candidates), catalog_starts=905,
        captured_catalog_starts=len(set(candidates) & known), outside_targets=len(outside),
        outside_by_domain=dict(Counter("resident" if r == "R" else "overlay" for r, _ in outside)),
        outside_with_fixup_far=sum(any(x["kind"] == "fixup-far" for x in candidates[p]) for p in outside),
        samples=samples, reject_samples=rejects,
        controls=dict(literal_marker_and_stub_controls_passed=True,
                      raw_operand_false_positive_rejected=True, hidden_known_starts_recovered=hidden,
                      known_game_body="OVL_001C_0126; ownership control requires manual assessment"),
        limitations=["outside905 is not proof of absence from the full947 historical census",
                     "shared unpacker/container interpretation not independently verified",
                     "linear validation uses catalog anchors; raw discovery does not",
                     "raw E8 bytes include data and operands; far fixups do not prove source reachability",
                     "no complete CS wrap, indirect/computed pointer, fallthrough or code-region recovery",
                     "sample is not uniform over all function starts; no whole-game recall estimate"])
    (output / "pilot.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ("samples", "containers", "reject_samples")}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--queue", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
