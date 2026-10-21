#!/usr/bin/env python3
"""List a resident routine's one-hop callees, loader-derived.

The strict candidate queue covers non-strict OVERLAYS only, so a resident body
like `RES_00BA_16EC` has no row and no `callee_readiness.targets` there. That
makes "check the queue's target list first" unsatisfiable for residents, and it
means any resident cost quoted across lanes has to come from an artifact the
receiving lane can regenerate. This is that generator.

Everything is measured from the **loader-expanded** START.EXE image
(`resident_loader_map.unpack`), the same path the credited residents used — not
from a linear sweep of the overlay file, and not from a row.

Usage:
    python3 tools/overlay/resident_target_list.py --resident 00BA:16EC
    python3 tools/overlay/resident_target_list.py --resident 00BA:16EC --markdown
"""
import argparse
import csv
import sys
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

import strict_candidate_queue as queue_rules
from overlay_inventory import find_units, parse_mz
from resident_inventory import GAME_SPECIFIC_TARGETS

ROOT = Path(__file__).resolve().parents[2]
MEDIA = ROOT / "resources/DOS/POOLRAD"
IN_IMAGE_BASE = 0xBA0          # 00BA resident segment inside the expanded image


def loaded():
    """(image, disassembler, units by memory segment, manifest-full ids)."""
    from resident_loader_map import unpack
    exe = (MEDIA / "START.EXE").read_bytes()
    ovr = (MEDIA / "GAME.OVR").read_bytes()
    image = bytes(unpack(exe)[0])
    units = {u["memory_segment"]: u for u in
             find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    manifest = {row["id"] for row in
                csv.DictReader((ROOT / "docs/re/partial_callee_models.csv").open())
                if row["status"] == "full"}
    return image, Cs(CS_ARCH_X86, CS_MODE_16), units, manifest


def span(md, blob, origin, cap=4096):
    """Bytes from `origin` to the end of the first RET/RETF, or None."""
    at = 0
    while at < len(blob) - 1:
        ins = list(md.disasm(blob[at:at + 16], origin + at))
        if not ins:
            at += 1
            continue
        step = ins[0]
        if step.mnemonic in ("retf", "ret"):
            return at + step.size
        at += step.size
    return None


def resident_span(md, image, seg, off):
    at = ((seg << 4) + off) & 0xFFFFF
    if at >= len(image):
        return None
    return span(md, image[at:at + 4096], off)


def readiness(seg, off, units, manifest):
    """(kind, id, readiness-string) for one far-call target."""
    if seg == 0x0AF8:                      # SYSTEM runtime endpoints
        covered = (seg, off) in queue_rules.RUNTIME_COVERED_ENDPOINTS
        return ("runtime", f"{seg:04X}:{off:04X}",
                "runtime_covered" if covered else "runtime_unmapped")
    unit = units.get(seg)
    if unit is not None and 0x20 <= off < 0x20 + 5 * len(unit["entries"]) \
            and (off - 0x20) % 5 == 0:
        index = (off - 0x20) // 5
        if index < len(unit["entries"]) and unit["entries"][index]:
            target = unit["entries"][index]
            rid = f"OVL_{unit['stub_para']:04X}_{target:04X}"
            strict = {row["id"] for row in
                      csv.DictReader((ROOT / "docs/re/strict_full_c_addresses.csv").open())}
            return ("overlay", rid, "strict" if rid in strict else "untranslated")
    rid = f"RES_{seg:04X}_{off:04X}"
    if (seg, off) in GAME_SPECIFIC_TARGETS:
        return ("resident", rid, "strict")
    if rid in manifest:
        return ("resident", rid, "resident_covered")
    return ("resident", rid, "resident_unmapped")


def solo_unlocks(root, resident_id):
    """Overlay rows for which this resident id is the only remaining blocker."""
    rows = __import__("json").loads(
        (root / "resources/strict-candidate-queue/queue.json").read_text())["rows"]
    unready_states = {"untranslated", "resident_unmapped", "resident_callback",
                      "runtime_unmapped", "partial", "modeled_blocked", "verified",
                      "interface_caveat", "resident_interface_caveat"}
    hits = []
    for row in rows:
        readiness_block = row["callee_readiness"]
        blockers = {t.get("id") for t in readiness_block["targets"]
                    if t["readiness"] in unready_states and t.get("id")}
        if readiness_block["counts"].get("indirect_unresolved"):
            blockers.add("INDIRECT")
        if readiness_block["raw_cpu_caveats"]:
            blockers.add("CAVEAT")
        if blockers == {resident_id}:
            hits.append(row["id"])
    return hits


def targets(seg, off, root=ROOT):
    image, md, units, manifest = loaded()
    own = resident_span(md, image, seg, off)
    at = ((seg << 4) + off) & 0xFFFFF
    body = image[at:at + (own or 0)]
    found, seen = [], set()
    for ins in md.disasm(body, off):
        if ins.mnemonic != "lcall":
            continue
        try:
            seg_text, off_text = [p.strip() for p in ins.op_str.split(",")]
            cseg, coff = int(seg_text, 16), int(off_text, 16)
        except ValueError:
            continue
        kind, rid, state = readiness(cseg, coff, units, manifest)
        if (kind, rid) in seen:
            continue
        seen.add((kind, rid))
        size = resident_span(md, image, cseg, coff) if kind == "resident" else None
        starts = len(list(md.disasm(image[((cseg << 4) + coff) & 0xFFFFF:][:size or 0], coff))) \
            if size else None
        found.append(dict(site=ins.address, kind=kind, id=rid, span=size,
                          starts=starts, readiness=state))
    return own, found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--resident", required=True, help="SEG:OFF, e.g. 00BA:16EC")
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--markdown", action="store_true")
    args = parser.parse_args(argv)
    seg_text, off_text = args.resident.split(":")
    seg, off = int(seg_text, 16), int(off_text, 16)
    rid = f"RES_{seg:04X}_{off:04X}"
    queue_rules.load_fresh_export(args.root / "resources/strict-candidate-queue/queue.json", args.root)
    own, found = targets(seg, off, args.root)
    unready = [t for t in found if t["readiness"].endswith("unmapped")
               or t["readiness"] == "untranslated"]
    one_hop = (own or 0) + sum(t["span"] or 0 for t in unready)
    unlocks = solo_unlocks(args.root, rid)
    if args.markdown:
        print(f"| callee | kind | extent to first RETF | starts | readiness | misses? | unlocks alone |")
        print(f"| --- | --- | ---: | ---: | --- | --- | ---: |")
        for t in found:
            miss = "yes" if t in unready else "-"
            alone = len(solo_unlocks(args.root, t["id"])) if t["kind"] == "resident" else 0
            print(f"| `{t['id']}` | {t['kind']} | {t['span'] if t['span'] else '-'} | "
                  f"{t['starts'] if t['starts'] else '-'} | {t['readiness']} | {miss} | {alone} |")
        print()
        print(f"`{rid}` own extent: **{own} B**; one-hop total (own + unready callees): "
              f"**{one_hop} B**; rows this body alone unlocks: **{len(unlocks)}**")
    else:
        print(f"{rid}: own {own} B | one-hop {one_hop} B | unlocks {len(unlocks)} rows alone")
        for t in found:
            print(f"  {t['site']:04X} -> {t['id']:<16} {t['readiness']:<18} span={t['span']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
