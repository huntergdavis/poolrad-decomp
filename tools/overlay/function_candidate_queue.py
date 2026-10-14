#!/usr/bin/env python3
"""Rank uncredited callable originals using existing address/count metadata.

Reuses the merged function_queue schema (#377), alias normalization (#390),
and classifier measurements. No executable bytes or strings are read/emitted.
The candidate span is a next-prologue upper bound, not a verified routine body;
calls come from bounded linear decode, not a complete CFG. Resident spans and
outgoing calls are unmeasured and remain null.

Usage:
  python3 tools/overlay/function_candidate_queue.py \
    --queue resources/function-accounting-current/queue.json \
    --classification docs/re/overlay_classification.csv \
    --full-c-map docs/re/strict_full_c_addresses.csv \
    --json resources/function-candidates.json

Ranking is an explicit lexicographic policy, not a probability, ownership
proof, worker assignment, or completion measure. The whole-game denominator
remains unknown. Inferred-library candidates are retained and labeled last.
"""

import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

from function_accounting_refresh import aliases_for
from function_queue import (DEFAULT_CREDIT, OVERLAY_CLASSES, RESIDENT_CLASSES,
                            check_inputs, grade, index_unique, ownership, require)


POLICY = [
    "class tier: verified game, provisional game evidence, helper, unresolved, inferred library",
    "measured bounded-decode outgoing calls ascending; unmeasured last",
    "candidate span upper bound ascending; unmeasured last",
    "start-evidence grade A, B, C, D, U",
    "incoming call-site evidence count descending",
    "canonical original ID ascending",
]


def count(value):
    require(type(value) in (int, str), "measurement must be an integer")
    result = int(value)
    require(result >= 0, "negative measured count")
    return result


def validate_queue(queue, credits):
    check_inputs(queue)
    require(queue["schema_version"] == 1, "unsupported callable queue schema")
    rows = index_unique(queue["rows"])
    aliases = aliases_for(queue)
    for row in rows.values():
        domain = row["domain"]
        require(domain in {"overlay", "resident"}, "unknown candidate domain")
        if domain == "overlay":
            para, rel = (int(part, 16) for part in row["original_address"].split(":"))
            canonical = f"OVL_{para:04X}_{rel:04X}"
            address = f"{para:#x}:{rel:#06x}"
            fields = ("entry", "entry_alias", "far_called", "near_called", "procptr")
            classes = OVERLAY_CLASSES
        else:
            require(row["original_address"].startswith("image:"), "invalid resident address")
            image = int(row["original_address"].split(":")[1], 16)
            canonical = f"RES_IMAGE_{image:05X}"
            address = f"image:0x{image:05X}"
            fields = ("overlay_far_sites", "loader_far_sites", "decoded_near_sites", "init_fallthrough")
            classes = RESIDENT_CLASSES
        require(row["id"] == canonical and re.fullmatch(
            r"OVL_[0-9A-F]{4}_[0-9A-F]{4}|RES_IMAGE_[0-9A-F]{5}", canonical),
            "noncanonical original identity")
        require(row["original_address"] == address, "noncanonical original address")
        require(row["near_call_evidence"] in (
            {"decoded_validated", "raw_unvalidated"} if domain == "overlay" else {"decoded_candidate_seeded"}),
            "unknown near-call evidence label")
        require(row["classification"] in classes and
                row["game_or_library"] == ownership(row["classification"]), "inconsistent ownership class")
        require(type(row["strict_full_c"]) is int and row["strict_full_c"] in (0, 1),
                "invalid strict-credit flag")
        ev = {key: count(row["evidence"][key]) for key in fields}
        expected = (grade(ev["entry"], ev["entry_alias"], ev["far_called"], ev["near_called"], ev["procptr"])
                    if domain == "overlay" else grade(ev["init_fallthrough"], 0,
                    ev["overlay_far_sites"] + ev["loader_far_sites"], ev["decoded_near_sites"]))
        require(row["grade"] == expected, "inconsistent evidence grade")
    credited = set()
    for credit in credits:
        require(credit["id"] in aliases, "strict manifest original absent from queue")
        target = aliases[credit["id"]]
        require(target["id"] not in credited, "duplicate strict credit through aliases")
        require(target["game_or_library"] == "game_verified", "strict original lacks verified identity")
        doc = credit.get("evidence_doc", "")
        require(doc.startswith("docs/re/") and ".." not in doc.split("/"), "invalid strict evidence path")
        credited.add(target["id"])
    require(credited == {key for key, row in rows.items() if row["strict_full_c"]},
            "strict manifest differs from queue credit flags")
    summary = queue["summary"]
    require(summary["queue_originals"] == len(rows) and
            summary["strict_full_c_originals"] == len(credited) and
            summary["ownership"] == dict(Counter(r["game_or_library"] for r in rows.values())),
            "stale callable queue summary")
    require(summary["whole_game_denominator"] == "unknown" and summary["completion_percentage"] is None,
            "candidate count cannot become a whole-game denominator")
    return rows, aliases, credited


def rank_candidates(queue, credits, classification, domain="all", limit=None):
    require(domain in {"all", "overlay", "resident"}, "invalid domain filter")
    require(limit is None or type(limit) is int and limit > 0, "limit must be positive")
    rows, aliases, credited = validate_queue(queue, credits)
    features = {}
    for source in classification:
        require(source["id"] in aliases, "classifier original absent from queue")
        target = aliases[source["id"]]
        require(target["domain"] == "overlay" and target["id"] not in features,
                "duplicate or non-overlay classifier original")
        require(source["classification"] == target["classification"] and source["grade"] == target["grade"],
                "classifier class or grade differs from queue")
        span, decoded, calls = (count(source[k]) for k in ("bound", "decoded", "calls"))
        require(span > 0 and decoded <= span, "invalid bounded-decode span")
        features[target["id"]] = dict(span_upper_bound=span, decoded_bytes=decoded, outgoing_calls=calls)
    require(set(features) == {key for key, row in rows.items() if row["domain"] == "overlay"},
            "classifier and overlay candidate sets differ")
    ranked = []
    for key, source in rows.items():
        if key in credited or domain != "all" and source["domain"] != domain:
            continue
        fields = (("entry", "entry_alias", "far_called", "near_called", "procptr") if source["domain"] == "overlay"
                  else ("overlay_far_sites", "loader_far_sites", "decoded_near_sites", "init_fallthrough"))
        ev = {k: count(source["evidence"][k]) for k in fields}
        incoming = (count(ev["far_called"]) + count(ev["near_called"]) if source["domain"] == "overlay"
                    else sum(count(ev[k]) for k in ("overlay_far_sites", "loader_far_sites", "decoded_near_sites")))
        measured = features.get(key, dict(span_upper_bound=None, decoded_bytes=None, outgoing_calls=None))
        cls = source["classification"]
        tier = (4 if source["game_or_library"] == "library_inferred" else
                0 if cls == "game_verified" else 1 if cls == "game_evidenced" else
                2 if cls == "helper_candidate" else 3)
        ranked.append(dict(id=key, domain=source["domain"], original_address=source["original_address"],
            classification=cls, game_or_library=source["game_or_library"], grade=source["grade"],
            class_tier=tier, incoming_call_sites=incoming,
            evidence=ev,
            near_call_evidence=source["near_call_evidence"], **measured))
    def order(row):
        calls, span = row["outgoing_calls"], row["span_upper_bound"]
        return (row["class_tier"], calls is None, calls or 0, span is None, span or 0,
                "ABCDU".index(row["grade"]), -row["incoming_call_sites"], row["id"])
    ranked.sort(key=order)
    eligible = len(ranked)
    ranked = ranked if limit is None else ranked[:limit]
    for position, row in enumerate(ranked, 1):
        row["rank"] = position
    return dict(schema_version=1, inputs=queue["inputs"], ranking_policy=POLICY,
        summary=dict(queue_originals=len(rows), strict_originals_excluded=len(credited),
            uncredited_originals=len(rows) - len(credited), eligible_originals=eligible,
            emitted_originals=len(ranked), domain_filter=domain,
            whole_game_denominator="unknown", completion_percentage=None), rows=ranked)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--queue", required=True)
    parser.add_argument("--full-c-map", default=str(DEFAULT_CREDIT))
    parser.add_argument("--classification", required=True)
    parser.add_argument("--domain", choices=("all", "overlay", "resident"), default="all")
    parser.add_argument("--limit", type=int)
    parser.add_argument("--json", help="output file; without this flag print the address/count JSON")
    args = parser.parse_args()
    try:
        blobs = {key: Path(getattr(args, key)).read_bytes() for key in ("queue", "full_c_map", "classification")}
        queue = json.loads(blobs["queue"])
        hashes = {key: hashlib.sha256(value).hexdigest() for key, value in blobs.items()}
        for key in ("full_c_map", "classification"):
            require(hashes[key] == queue["artifacts_sha256"][key], "input differs from queue provenance: " + key)
        csv_rows = lambda key: list(csv.DictReader(blobs[key].decode().splitlines()))
        result = rank_candidates(queue, csv_rows("full_c_map"), csv_rows("classification"), args.domain, args.limit)
        result["artifacts_sha256"] = hashes
        content = json.dumps(result, sort_keys=True, indent=2) + "\n"
        if args.json:
            Path(args.json).write_text(content)
            print(json.dumps(result["summary"], sort_keys=True))
        else:
            print(content, end="")
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(2, f"candidate queue refused: {error}\n")


if __name__ == "__main__":
    main()
