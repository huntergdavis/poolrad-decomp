#!/usr/bin/env python3
"""Export a deterministic address-only work queue; the game denominator stays open.

Consumes overlay inventory, function ledger and classifier products, optionally
the resident callable audit. No disassembly, strings or executable bytes are
copied to the output. IDs are scoped to the input build hashes.
"""

import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path

from classify_overlay import KNOWN_GAME_OVR_SHA256, KNOWN_START_EXE_SHA256


DIGESTS = {"start_exe_sha256": KNOWN_START_EXE_SHA256,
           "game_ovr_sha256": KNOWN_GAME_OVR_SHA256}
DEFAULT_CREDIT = Path(__file__).resolve().parents[2] / "docs/re/strict_full_c_addresses.csv"
OVERLAY_CLASSES = {"game_verified", "game_evidenced", "helper_candidate",
                   "game_or_other_library_unresolved"}
RESIDENT_CLASSES = {"game_verified", "linked_crt_inferred", "unresolved",
                    "linked_runtime_evidenced"}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def check_inputs(document):
    require(all(document.get("inputs", {}).get(k) == v for k, v in DIGESTS.items()),
            "input hashes differ from the audited DOS build")


def number(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def index_unique(rows):
    result = {}
    for row in rows:
        require(row["id"] not in result, "duplicate input ID")
        result[row["id"]] = row
    return result


def grade(entry=0, alias=0, far=0, near=0, procptr=0):
    return ("A" if entry or far else "B" if near else "C" if procptr else
            "D" if alias else "U")


def ownership(classification):
    return ("game_verified" if classification == "game_verified" else
            "library_inferred" if classification in
            {"linked_crt_inferred", "linked_runtime_evidenced"} else "unknown")


def overlay_rows(inventory, ledger_rows, classifications, near_validated=False):
    ledger = index_unique(ledger_rows)
    classes = index_unique(classifications)
    rows, stub_images, candidate_ids = [], {}, set()
    unit_ids = set()
    for unit in sorted(inventory["units"], key=lambda u: u["stub_para"]):
        para = unit["stub_para"]
        require(para not in unit_ids, "duplicate overlay unit")
        unit_ids.add(para)
        starts = [f["rel"] for f in unit["functions"]]
        require(len(starts) == len(set(starts)), "duplicate candidate address")
        slots = {rel: [] for rel in starts}
        for slot, raw in enumerate(unit["entries"]):
            target = raw if raw in slots else raw - 0x800
            require(target in slots, "entry has no exact or +0x800 candidate")
            slots[target].append((slot, raw))
        for rel in sorted(starts):
            require(0 <= rel < unit["code_size"], "candidate outside unit code")
            old_id = f"{para:#x}:{rel:#06x}"
            candidate_ids.add(old_id)
            require(old_id in ledger and old_id in classes, "candidate missing from input CSV")
            ev, cls = ledger[old_id], classes[old_id]
            for record in (ev, cls):
                require(number(record["unit"]) == para and number(record["offset"]) == rel
                        and number(record["ovr_offset"]) == unit["ovr_offset"] + rel,
                        "CSV address disagrees with inventory")
            evidence = {key: number(ev[key]) for key in
                        ("entry", "entry_alias", "far_called", "near_called", "procptr")}
            require(all(v >= 0 for v in evidence.values()), "negative evidence count")
            require(evidence["entry"] == int(any(raw == rel for _, raw in slots[rel]))
                    and evidence["entry_alias"] == int(any(raw != rel for _, raw in slots[rel])),
                    "ledger entry evidence disagrees with inventory")
            expected_grade = grade(evidence["entry"], evidence["entry_alias"],
                                   evidence["far_called"], evidence["near_called"], evidence["procptr"])
            require(ev["grade"] == cls["grade"] == expected_grade, "inconsistent evidence grade")
            require(number(cls["far_in"]) == evidence["far_called"], "far-call evidence disagrees")
            classification = cls["classification"]
            require(classification in OVERLAY_CLASSES, "unknown overlay classification")
            rid = f"OVL_{para:04X}_{rel:04X}"
            aliases = [old_id]
            entry_slots = []
            for slot, raw in slots[rel]:
                entry_slots.append({"slot": slot, "raw_offset": raw,
                                    "normalized_offset": rel})
                segment = unit.get("memory_segment")
                if segment is not None:
                    stub_offset = 0x20 + 5 * slot
                    aliases.append(f"STUB_{segment:04X}_{stub_offset:04X}")
                    image = 16 * segment + stub_offset
                    require(image not in stub_images, "overlay stubs overlap")
                    stub_images[image] = rid
            rows.append(dict(id=rid, domain="overlay", original_address=old_id,
                             aliases=sorted(aliases), grade=expected_grade, evidence=evidence,
                             entry_slots=entry_slots, classification=classification,
                             start_evidence=["pascal_prologue_pattern"],
                             game_or_library=ownership(classification),
                             near_call_evidence="decoded_validated" if near_validated else "raw_unvalidated",
                             strict_full_c=0))
    require(set(ledger) == set(classes) == candidate_ids, "CSV candidate sets differ from inventory")
    return rows, stub_images


def add_residents(rows, stub_images, audit):
    check_inputs(audit)
    by_id = {row["id"]: row for row in rows}
    aliases_seen = set()
    for source in sorted(audit["rows"] + audit.get("runtime_rows", []),
                         key=lambda row: (number(row["image_offset"]), row["id"])):
        # A reviewed RES_ name may carry the raw-prologue id it was discovered as; both resolve to the one original.
        names = [source["id"]] + ([source["discovery_id"]] if source.get("discovery_id") else [])
        require(not aliases_seen & set(names), "duplicate resident alias")
        aliases_seen.update(names)
        image = number(source["image_offset"])
        require(image >= 0, "negative resident image address")
        rid = stub_images.get(image, f"RES_IMAGE_{image:05X}")
        classification = source["classification"]
        require(classification in RESIDENT_CLASSES, "unknown resident classification")
        counts = {k: number(source[k]) for k in
                  ("overlay_far_sites", "loader_far_sites", "decoded_near_sites", "init_fallthrough")}
        require(all(v >= 0 for v in counts.values()), "negative resident evidence count")
        if rid in by_id:
            target = by_id[rid]
            # A loader stub aliases its overlay original; its resident-looking
            # label is not an independent resident identity or ownership claim.
            if target["domain"] == "resident":
                old_owner, new_owner = target["game_or_library"], ownership(classification)
                require("unknown" in (old_owner, new_owner) or old_owner == new_owner,
                        "conflicting game/library classification at one image address")
                if old_owner == "unknown" and new_owner != "unknown":
                    target["classification"] = classification
                    target["game_or_library"] = new_owner
                for key, value in counts.items():
                    target["evidence"][key] = max(target["evidence"][key], value)
                ev = target["evidence"]
                target["grade"] = grade(entry=ev["init_fallthrough"],
                                        far=ev["overlay_far_sites"] + ev["loader_far_sites"],
                                        near=ev["decoded_near_sites"])
            target["aliases"].extend(names)
            target["start_evidence"] = sorted(set(target["start_evidence"] +
                                                  [source.get("boundary", "candidate_seed")]))
            target.setdefault("resident_alias_image_offsets", []).append(image)
            continue
        target = dict(id=rid, domain="resident", original_address=f"image:0x{image:05X}",
                      aliases=list(names), grade=grade(entry=counts["init_fallthrough"],
                      far=counts["overlay_far_sites"] + counts["loader_far_sites"],
                      near=counts["decoded_near_sites"]), evidence=counts,
                      classification=classification, game_or_library=ownership(classification),
                      start_evidence=[source.get("boundary", "candidate_seed")],
                      near_call_evidence="decoded_candidate_seeded", strict_full_c=0)
        rows.append(target)
        by_id[rid] = target
    for row in rows:
        row["aliases"] = sorted(set(row["aliases"]))
        if "resident_alias_image_offsets" in row:
            row["resident_alias_image_offsets"] = sorted(set(row["resident_alias_image_offsets"]))


def export_queue(inventory, ledger, classifications, credits, resident=None, near=None):
    check_inputs(inventory)
    if near is not None:
        check_inputs(near)
        require(all(near.get(k) == 0 for k in ("invalid_seed_paths", "truncated_seed_paths",
                    "raw_exact_not_decoded", "decoded_exact_not_raw")), "near-call validation failed")
        total = sum(number(row["near_called"]) for row in ledger)
        require(near.get("decoded_near_calls_to_prologues") ==
                near.get("raw_near_call_candidates_to_prologues") == total,
                "near-call validation site count differs from ledger")
    rows, stubs = overlay_rows(inventory, ledger, classifications, near is not None)
    if resident is not None:
        add_residents(rows, stubs, resident)
    aliases = {}
    for row in rows:
        for alias in [row["id"]] + row["aliases"]:
            require(alias not in aliases, "alias maps to more than one original")
            aliases[alias] = row
    credited = set()
    for credit in index_unique(credits).values():
        # The curated manifest covers both lanes; overlay-only exports apply
        # only its overlay IDs. All mappings are checked in combined mode.
        if resident is None and credit["id"].startswith("RES_"):
            continue
        require(credit["id"] in aliases, "strict full-C address is absent from queue")
        target = aliases[credit["id"]]
        require(target["id"] not in credited, "strict full-C aliases double-credit an original")
        require(target["game_or_library"] == "game_verified", "strict full-C credit lacks verified game identity")
        require(credit.get("evidence_doc", "").startswith("docs/re/")
                and ".." not in credit["evidence_doc"].split("/"), "missing strict full-C provenance")
        target["strict_full_c"] = 1
        target["strict_full_c_evidence"] = credit["evidence_doc"]
        credited.add(target["id"])
    # Original-address order is stable when input order or evidence changes.
    rows.sort(key=lambda row: row["id"])
    counts = lambda values: dict(sorted(Counter(values).items()))
    return dict(schema_version=1, inputs=DIGESTS.copy(),
                summary=dict(queue_originals=len(rows),
                             overlay_candidates=sum(r["domain"] == "overlay" for r in rows),
                             overlay_public_entry_slots=sum(len(u["entries"]) for u in inventory["units"]),
                             resident_input_rows=0 if resident is None else len(resident["rows"]),
                             runtime_input_rows=0 if resident is None else len(resident.get("runtime_rows", [])),
                             resident_unique_originals=sum(r["domain"] == "resident" for r in rows),
                             grades=counts(r["grade"] for r in rows),
                             ownership=counts(r["game_or_library"] for r in rows),
                             classifications=counts(r["classification"] for r in rows),
                             overlay_grades=counts(r["grade"] for r in rows if r["domain"] == "overlay"),
                             strict_full_c_by_domain=counts(r["domain"] for r in rows if r["strict_full_c"]),
                             verified_game_by_domain=counts(r["domain"] for r in rows
                                                           if r["game_or_library"] == "game_verified"),
                             strict_full_c_originals=sum(r["strict_full_c"] for r in rows),
                             whole_game_denominator="unknown", completion_percentage=None), rows=rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--ledger", required=True)
    parser.add_argument("--classification", required=True)
    parser.add_argument("--full-c-map", default=str(DEFAULT_CREDIT))
    parser.add_argument("--near-calls")
    parser.add_argument("--resident-audit")
    parser.add_argument("--reviewed-identities", help="bind approved identity manifest into queue provenance")
    parser.add_argument("--json", required=True)
    args = parser.parse_args()
    paths = {name: Path(value) for name, value in vars(args).items()
             if name != "json" and value is not None}
    blobs = {name: path.read_bytes() for name, path in paths.items()}
    def csv_rows(name):
        return list(csv.DictReader(blobs[name].decode().splitlines()))
    try:
        for credit in csv_rows("full_c_map"):
            doc = Path(__file__).resolve().parents[2] / credit["evidence_doc"]
            require(doc.is_file(), "strict full-C evidence document is absent")
        result = export_queue(json.loads(blobs["inventory"]), csv_rows("ledger"),
                              csv_rows("classification"), csv_rows("full_c_map"),
                              json.loads(blobs["resident_audit"]) if "resident_audit" in blobs else None,
                              json.loads(blobs["near_calls"]) if "near_calls" in blobs else None)
    except (ValueError, KeyError, TypeError) as error:
        parser.exit(2, f"function queue refused: {error}\n")
    result["artifacts_sha256"] = {name: hashlib.sha256(blob).hexdigest()
                                  for name, blob in sorted(blobs.items())}
    Path(args.json).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps(result["summary"], sort_keys=True))


if __name__ == "__main__":
    main()
