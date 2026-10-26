#!/usr/bin/env python3
"""Reconcile the canonical address queue with the reviewed static CEILING.

Exports metadata only. Callable/library kinds inherit the census's segment
judgment; they do not establish game ownership or behavioral parity.
"""

import argparse
import ast
from collections import Counter
import csv
import hashlib
import io
import json
from pathlib import Path
import re

import check_checker_builds as checker_contracts


ROOT = Path(__file__).resolve().parents[2]
CENSUS = "docs/re/FUNCTION_CEILING_CENSUS.md"
CHECKPOINT = "docs/re/function_accounting_checkpoint.json"
MANIFESTS = ("partial_callee_models.csv", "driver_internal_models.csv",
             "resident_runtime_models.csv")
TREE_INPUTS = {
    "classification": "docs/re/overlay_classification.csv",
    "ledger": "docs/re/overlay_function_ledger.csv",
    "full_c_map": "docs/re/strict_full_c_addresses.csv",
    "reviewed_identities": "docs/re/reviewed_function_identities.csv",
}
SUPPLEMENTAL = (
    ("OVL_0016_0042", "heap-error callback via DS:4414", "Corrections to the queue, item 1"),
    ("OVL_0016_00AC", "unit initialization via resident stub", "Corrections to the queue, item 1"),
    ("RES_08AE_13EA", "INT 08h vector installed by RES_08AE_137C", "Corrections to the queue, item 2"),
    ("RES_08AE_1477", "timer-handler near helper", "Corrections to the queue, item 2"),
    ("RES_08AE_14EC", "timer-handler near helper", "Corrections to the queue, item 2"),
    ("RES_08AE_157A", "timer-handler near helper", "Corrections to the queue, item 2"),
    ("RES_08AE_160B", "timer-handler near helper (table labels are interior)", "Corrections to the queue, item 2"),
)
FIELDS = ("id", "domain", "original_address", "aliases", "kind", "library_tier",
          "kind_authority", "kind_evidence", "queue_classification", "game_ownership",
          "evidence_state", "state_evidence", "strict_full_c", "start_grade", "ingress",
          "model_declarations", "c_sources", "evidence_docs", "recorded_body_bytes",
          "recorded_body_sha256")
STATES = ("address_only", "disassembled", "bytes_recovered", "C_drafted", "credited")
KINDS = ("callable", "data", "duplicate_alias", "library_inferred", "unresolved")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def resident_image(identifier):
    match = re.fullmatch(r"(?:RES|SYS)_([0-9A-F]{4})_([0-9A-F]{4})", identifier)
    return int(match[1], 16) * 16 + int(match[2], 16) if match else None


def census_kind(row):
    """A policy join, not a fresh inference about authorship or callable bytes."""
    cls = row["classification"]
    if cls == "linked_runtime_evidenced":
        return "library_inferred", "SYSTEM", "classification=linked_runtime_evidenced"
    if cls == "linked_crt_inferred":
        return "library_inferred", "CRT", "classification=linked_crt_inferred"
    if row["id"] == "RES_IMAGE_0AA20":
        return ("library_inferred", "overlay_manager",
                "original_address=image:0x0AA20; census library segment=0xAA2")
    return "callable", "", "queue start included by census; " + ingress(row)


def ingress(row):
    return ";".join(f"{k}={v}" for k, v in sorted(row["evidence"].items()) if v)


def validate_queue(blob, checkpoint, tree_hashes):
    require(sha(blob) == checkpoint["queue_sha256"], "stale/noncanonical queue SHA-256")
    queue = json.loads(blob)
    require(queue["inputs"] == checkpoint["inputs"], "queue media hashes differ")
    require(queue["artifacts_sha256"] == checkpoint["artifacts_sha256"],
            "queue product hashes differ from checkpoint")
    for key, digest in tree_hashes.items():
        require(queue["artifacts_sha256"].get(key) == digest, f"stale tree input: {key}")
    rows = queue["rows"]
    require(len(rows) == checkpoint["queue_originals"], "queue row count differs")
    require(sum(r["strict_full_c"] for r in rows) == checkpoint["strict_full_c_originals"],
            "strict-credit count differs")
    require(sum(r["game_or_library"] == "game_verified" for r in rows) ==
            checkpoint["verified_game_identities"], "verified-identity count differs")
    seen_ids, seen_aliases, seen_addresses = set(), set(), set()
    for row in rows:
        require(row["id"] not in seen_ids, "duplicate queue ID")
        seen_ids.add(row["id"])
        address = row["domain"], row["original_address"]
        require(address not in seen_addresses, "duplicate original address")
        seen_addresses.add(address)
        for alias in [row["id"]] + row["aliases"]:
            require(alias not in seen_aliases, "alias belongs to multiple originals")
            seen_aliases.add(alias)
        if row["strict_full_c"]:
            require(row["game_or_library"] == "game_verified", "credit lacks game identity")
            require(census_kind(row)[0] == "callable", "credit on excluded library row")
    return queue


def evidence_state(row, decoded, models, body):
    if row["strict_full_c"]:
        return "credited", "strict_full_c_addresses.csv:" + row["strict_full_c_evidence"]
    drafted = [m for m in models if m["sources"] and m["status"] != "callback"]
    if drafted:
        return "C_drafted", ";".join(sorted({m["declaration"] for m in drafted}))
    if body:
        return "bytes_recovered", body["declaration"] + ":recorded span/hash (not rehashed here)"
    if decoded:
        return "disassembled", decoded
    return "address_only", "queue address; no stronger mapped evidence"


def csv_blob(rows, fields=FIELDS):
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue().encode()


def reconcile(queue, root):
    """Return metadata products and hashes of every tree file read for the join."""
    hashes = {}

    def read(rel):
        blob = (root / rel).read_bytes()
        hashes[rel] = sha(blob)
        return blob

    def csv_read(rel):
        return list(csv.DictReader(read(rel).decode().splitlines()))

    for rel in (CENSUS, "docs/re/function_ceiling_census.json", CHECKPOINT,
                "tools/overlay/function_backlog_reconciliation.py",
                "tools/overlay/function_queue.py", "tools/overlay/function_queue_audit.py"):
        read(rel)
    ceiling = json.loads(read("docs/re/function_ceiling_census.json"))
    require(ceiling["bound_kind"] == "upper_bound" and ceiling["count"] == 815,
            "CEILING policy changed; review reconciliation rule")
    require(ceiling["source_digests"] == queue["inputs"], "CEILING media hashes differ")
    by_alias = {alias: row["id"] for row in queue["rows"] for alias in [row["id"]] + row["aliases"]}
    by_image = {int(row["id"].rsplit("_", 1)[1], 16): row["id"]
                for row in queue["rows"] if row["domain"] == "resident"}

    def resolve(identifier):
        return by_alias.get(identifier) or by_image.get(resident_image(identifier))

    ledger = {"OVL_%04X_%04X" % (int(r["unit"], 0), int(r["offset"], 0)): r
              for r in csv_read(TREE_INPUTS["ledger"])}
    resident = {int(r["image_offset"], 0): r for r in csv_read("docs/re/resident_callable_ledger.csv")}
    models, bodies, outside = {}, {}, []
    for manifest in MANIFESTS + ("reviewed_function_identities.csv",):
        rel = "docs/re/" + manifest
        for record in csv_read(rel):
            identifier = resolve(record["id"])
            if identifier is None:
                outside.append({"id": record["id"], "manifest": rel})
                continue
            if record.get("body_sha256"):
                require(re.fullmatch(r"[0-9a-f]{64}", record["body_sha256"]) and
                        int(record["body_bytes"]) > 0, "invalid recorded body span/hash")
                body = dict(record, declaration=rel + ":" + record["id"])
                if identifier in bodies:
                    require(all(bodies[identifier][key] == body[key] for key in
                                ("body_bytes", "body_sha256")), "conflicting recorded body spans")
                bodies[identifier] = body
            if manifest == "reviewed_function_identities.csv":
                continue
            doc = record["evidence_doc"]
            text = read(doc).decode()
            sources = sorted(set(re.findall(r"src/recovered/[A-Za-z0-9_./-]+\.c\b", text)) |
                             {"src/recovered/" + name for name in
                              re.findall(r"\b([A-Za-z0-9_]+\.c)\b", text)})
            sources = [source for source in sources if (root / source).is_file()]
            for source in sources:
                read(source)
            models.setdefault(identifier, []).append(dict(
                declaration=rel + ":" + record["id"], status=record.get("status", "full"),
                sources=sources, evidence_doc=doc))

    read("tools/overlay/check_checker_builds.py")
    registry = checker_contracts.discover(root)
    for registered_id, checkers in sorted(registry.items()):
        identifier = resolve(registered_id)
        if identifier is None:
            continue
        existing = models.get(identifier, [])
        if existing and all(m["status"] == "callback" for m in existing):
            continue
        for checker in checkers:
            rel = checker.as_posix()
            commands = checker_contracts._cc_commands(ast.parse(read(rel).decode()), root)
            sources = set()
            for command in commands:
                for arg in command:
                    if not isinstance(arg, str) or not arg.endswith(".c"):
                        continue
                    path = Path(arg)
                    if path.is_absolute():
                        if not path.is_relative_to(root):
                            continue
                        path = path.relative_to(root)
                    if path.as_posix().startswith("src/recovered/") and (root / path).is_file():
                        sources.add(path.as_posix())
            for source in sorted(sources):
                read(source)
            if sources:
                models.setdefault(identifier, []).append(dict(
                    declaration=rel + ":CHECKER_ROWS:" + registered_id,
                    status="registered_checker", sources=sorted(sources), evidence_doc=""))

    # These explicit endpoint declarations cover library C models too. Read the
    # literal table; importing the readiness tool would add unrelated dependencies.
    helper = "tools/overlay/strict_candidate_queue.py"
    module = ast.parse(read(helper).decode())
    endpoints = next(ast.literal_eval(node.value) for node in module.body
                     if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and
                         t.id == "RUNTIME_COVERED_ENDPOINTS" for t in node.targets))
    for (segment, offset), description in sorted(endpoints.items()):
        identifier = by_image.get(segment * 16 + offset)
        if identifier is None:
            continue
        match = re.search(r"\(([a-z0-9_]+)(?:[, )])", description)
        source = "src/recovered/" + match[1] + ".c" if match else ""
        sources = [source] if source and (root / source).is_file() else []
        for path in sources:
            read(path)
        models.setdefault(identifier, []).append(dict(
            declaration=f"{helper}:RUNTIME_COVERED_ENDPOINTS:{segment:04X}:{offset:04X}",
            status="full", sources=sources, evidence_doc=""))

    rows = []
    for row in sorted(queue["rows"], key=lambda r: r["id"]):
        identifier = row["id"]
        kind, tier, kind_evidence = census_kind(row)
        decoded = ""
        if row["domain"] == "overlay":
            require(identifier in ledger, "overlay missing ledger row")
            if int(ledger[identifier]["decoded"]) > 0:
                decoded = TREE_INPUTS["ledger"] + ":decoded>0; candidate-bound decode"
        else:
            image = int(identifier.rsplit("_", 1)[1], 16)
            if image in resident:
                decoded = "resident_callable_coverage.py:candidate-seeded decoded path audit"
            elif row["classification"] == "linked_runtime_evidenced":
                decoded = "resident_callable_coverage.py:decoded_system_closure_seed"
        body = bodies.get(identifier)
        declarations = models.get(identifier, [])
        state, state_evidence = evidence_state(row, decoded, declarations, body)
        docs = {m["evidence_doc"] for m in declarations if m["evidence_doc"]}
        if row.get("strict_full_c_evidence"):
            docs.add(row["strict_full_c_evidence"])
        if body:
            docs.add(body["evidence_doc"])
        rows.append(dict(id=identifier, domain=row["domain"], original_address=row["original_address"],
                         aliases=";".join(row["aliases"]), kind=kind, library_tier=tier,
                         kind_authority=CENSUS, kind_evidence=kind_evidence,
                         queue_classification=row["classification"], game_ownership=row["game_or_library"],
                         evidence_state=state, state_evidence=state_evidence,
                         strict_full_c=row["strict_full_c"], start_grade=row["grade"], ingress=ingress(row),
                         model_declarations=";".join(sorted(m["declaration"] for m in declarations)),
                         c_sources=";".join(sorted({s for m in declarations for s in m["sources"]})),
                         evidence_docs=";".join(sorted(docs)), recorded_body_bytes=body["body_bytes"] if body else "",
                         recorded_body_sha256=body["body_sha256"] if body else ""))
    supplemental = []
    for identifier, evidence, section in SUPPLEMENTAL:
        require(resolve(identifier) is None, "census-only start is now queued; review reconciliation")
        supplemental.append(dict(id=identifier, kind="callable", evidence_state="address_only",
                                 strict_full_c=0, authority=CENSUS + ":" + section,
                                 evidence=evidence))
    counts = {kind: sum(r["kind"] == kind for r in rows) for kind in KINDS}
    summary = dict(total_candidates=len(rows), kinds=counts,
                   callable_already_strict_credited=sum(r["strict_full_c"] for r in rows if r["kind"] == "callable"),
                   callable_verified_game_identity=sum(r["game_ownership"] == "game_verified" for r in rows if r["kind"] == "callable"),
                   library_tiers=dict(sorted(Counter(r["library_tier"] for r in rows if r["library_tier"]).items())),
                   kind_by_state={kind: {state: sum(r["kind"] == kind and r["evidence_state"] == state for r in rows)
                                         for state in STATES} for kind in KINDS},
                   census_only_starts=len(supplemental), static_CEILING=ceiling["count"],
                   whole_game_verified_denominator="unknown", registered_model_ids_outside_queue=outside)
    require(counts["callable"] + len(supplemental) == ceiling["count"], "CEILING arithmetic differs")
    products = {
        "function_backlog_reconciliation.csv": csv_blob(rows),
        "function_backlog_excluded.csv": csv_blob([r for r in rows if r["kind"] != "callable"]),
        "function_backlog_census_only.csv": csv_blob(supplemental, tuple(supplemental[0])),
    }
    return summary, products, hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--queue", type=Path, required=True,
                        help="canonical function_queue_audit.py output; stale exports are refused")
    parser.add_argument("--repo", type=Path, default=ROOT)
    parser.add_argument("--out-dir", type=Path, default=ROOT / "docs/re")
    parser.add_argument("--check", action="store_true", help="compare all products; never repair them")
    args = parser.parse_args()
    root = args.repo.resolve()
    try:
        blob = args.queue.read_bytes()
        checkpoint = json.loads((root / CHECKPOINT).read_bytes())
        tree_hashes = {key: sha((root / rel).read_bytes()) for key, rel in TREE_INPUTS.items()}
        queue = validate_queue(blob, checkpoint, tree_hashes)
        summary, products, hashes = reconcile(queue, root)
        hashes.update({rel: digest for key, rel in TREE_INPUTS.items() for digest in [tree_hashes[key]]})
        report = dict(schema_version=1, definition="distinct callable original function bodies in non-Borland-library code",
                      kind_method="census policy join, not independent ownership classification",
                      queue_sha256=sha(blob), media_sha256=queue["inputs"],
                      queue_product_sha256=queue["artifacts_sha256"], tree_inputs_sha256=dict(sorted(hashes.items())),
                      summary=summary, output_sha256={name: sha(data) for name, data in sorted(products.items())})
        products["function_backlog_reconciliation.json"] = (json.dumps(report, indent=2, sort_keys=True) + "\n").encode()
        if args.check:
            for name, data in products.items():
                require((args.out_dir / name).read_bytes() == data, "generated product differs: " + name)
        else:
            args.out_dir.mkdir(parents=True, exist_ok=True)
            for name, data in products.items():
                (args.out_dir / name).write_bytes(data)
    except (ValueError, KeyError, OSError, StopIteration) as error:
        parser.exit(2, f"backlog reconciliation refused: {error}\n")
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
