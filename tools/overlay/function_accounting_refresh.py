#!/usr/bin/env python3
"""Stage and publish one explicitly approved overlay accounting change.

The approval is a PM-supplied record, never inferred from C files or passing
tests. Native products remain under ignored resources/. Existing candidate
addresses only; resident promotions and demotions fail explicitly.
"""

import argparse
from collections import Counter
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys

from classify_overlay import BASE_VERIFIED, load_reviewed_identities
from function_alias_audit import audit_aliases
from function_queue import DIGESTS


BEGIN = "<!-- FUNCTION_ACCOUNTING:BEGIN -->"
END = "<!-- FUNCTION_ACCOUNTING:END -->"
IDENTITY_FIELDS = ("id", "evidence_doc", "review_pr", "reviewed_commit", "body_sha256", "body_bytes")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(content):
    return hashlib.sha256(content).hexdigest()


def csv_bytes(rows, fields):
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(sorted(rows, key=lambda row: row["id"]))
    return stream.getvalue().encode()


def aliases_for(queue):
    aliases = {}
    for row in queue["rows"]:
        for key in [row["id"], row["original_address"]] + row["aliases"]:
            require(key not in aliases or aliases[key]["id"] == row["id"], "ambiguous original alias")
            aliases[key] = row
    return aliases


def plan_change(queue, credits, identities, approval):
    require(approval.get("status") == "approved" and approval.get("approved_by") == "PM",
            "an explicit PM approval record is required")
    require(isinstance(approval.get("review_pr"), int) and approval["review_pr"] > 0
            and re.fullmatch(r"[0-9a-f]{40}", approval.get("reviewed_commit", "")),
            "approval requires exact reviewed merge commit and PR")
    require(re.fullmatch(r"[0-9a-f]{64}", approval.get("body_sha256", ""))
            and isinstance(approval.get("body_bytes"), int) and approval["body_bytes"] > 0,
            "approval requires bounded original-body hash")
    require(all(isinstance(approval.get(key), bool) for key in ("game_identity", "strict_full_c")),
            "approval must state identity and strict-C decisions separately")
    doc = approval.get("evidence_doc", "")
    require(doc.startswith("docs/re/") and ".." not in doc.split("/"), "invalid evidence document")
    aliases = aliases_for(queue)
    require(approval["original_id"] in aliases, "approved original absent from queue")
    target = aliases[approval["original_id"]]
    require(target["domain"] == "overlay", "resident changes require a separate reviewed workflow")
    credit_ids = [aliases[row["id"]]["id"] for row in credits]
    require(len(credit_ids) == len(set(credit_ids)), "duplicate existing strict credit through aliases")
    require(set(credit_ids) == {r["id"] for r in queue["rows"] if r["strict_full_c"]},
            "canonical credit manifest differs from baseline queue")
    identity_ids = [row["id"] for row in identities]
    require(len(identity_ids) == len(set(identity_ids)), "duplicate existing reviewed identity")
    if approval["strict_full_c"]:
        require(target["id"] not in credit_ids, "original already has strict full-C credit")
        require(target["game_or_library"] == "game_verified" or approval["game_identity"],
                "strict credit lacks approved game identity")
    identity_delta = int(approval["game_identity"] and target["game_or_library"] != "game_verified")
    strict_delta = int(approval["strict_full_c"])
    require(identity_delta or strict_delta, "approval makes no new accounting change")
    if identity_delta:
        require(target["game_or_library"] == "unknown" and target["id"] not in identity_ids,
                "duplicate identity or inferred-library ownership conflict")
    normalized = dict(approval, original_id=target["id"])
    new_credits = list(credits)
    if strict_delta:
        new_credits.append(dict(id=target["id"], evidence_doc=doc))
    new_identities = list(identities)
    if identity_delta:
        new_identities.append({key: normalized["original_id"] if key == "id" else normalized[key]
                               for key in IDENTITY_FIELDS})
    return dict(approval=normalized, target=target, credits=new_credits, identities=new_identities,
                strict_delta=strict_delta, identity_delta=identity_delta)


def verify_transition(before, after, plan):
    old = {r["id"]: r for r in before["rows"]}
    new = {r["id"]: r for r in after["rows"]}
    require(len(old) == len(before["rows"]) and len(new) == len(after["rows"]) and set(old) == set(new),
            "candidate originals changed during accounting refresh")
    target = plan["approval"]["original_id"]
    changed_credits = {key for key in old if old[key]["strict_full_c"] != new[key]["strict_full_c"]}
    changed_owners = {key for key in old if old[key]["game_or_library"] != new[key]["game_or_library"]}
    require(changed_credits == ({target} if plan["strict_delta"] else set()), "unapproved strict-credit change")
    require(changed_owners == ({target} if plan["identity_delta"] else set()), "unapproved ownership change")
    require(new[target]["game_or_library"] == "game_verified", "approved original not game verified")
    strict = sum(r["strict_full_c"] for r in new.values())
    verified = sum(r["game_or_library"] == "game_verified" for r in new.values())
    require(strict == sum(r["strict_full_c"] for r in old.values()) + plan["strict_delta"]
            and verified == sum(r["game_or_library"] == "game_verified" for r in old.values()) + plan["identity_delta"],
            "accounting deltas differ from exact approval")
    require(after["summary"]["strict_full_c_originals"] == strict
            and after["summary"]["ownership"]["game_verified"] == verified, "stale generated counts")
    require(after["summary"]["whole_game_denominator"] == "unknown"
            and after["summary"]["completion_percentage"] is None, "denominator was promoted")
    return strict, verified


def make_checkpoint(queue, queue_content, approval=None):
    rows = queue["rows"]
    return dict(schema_version=1, inputs=queue["inputs"], queue_sha256=digest(queue_content),
                artifacts_sha256=queue["artifacts_sha256"],
                strict_full_c_originals=sum(r["strict_full_c"] for r in rows),
                verified_game_identities=sum(r["game_or_library"] == "game_verified" for r in rows),
                strict_full_c_by_domain=dict(Counter(r["domain"] for r in rows if r["strict_full_c"])),
                verified_game_by_domain=dict(Counter(r["domain"] for r in rows if r["game_or_library"] == "game_verified")),
                queue_originals=len(rows), unknown_ownership=sum(r["game_or_library"] == "unknown" for r in rows),
                whole_game_denominator="unknown", reviewed_change=approval)


def render_battle_card(text, checkpoint):
    require(text.count(BEGIN) == text.count(END) == 1, "battle-card accounting markers missing or ambiguous")
    strict, verified = checkpoint["strict_full_c_by_domain"], checkpoint["verified_game_by_domain"]
    pr = checkpoint["reviewed_change"]["review_pr"]
    paragraph = (f"The independently reviewed checkpoint after PR #{pr} is "
                 f"**{checkpoint['strict_full_c_originals']} strict full-C game routines** "
                 f"({strict.get('overlay', 0)} overlay, {strict.get('resident', 0)} resident) and "
                 f"**{checkpoint['verified_game_identities']} verified game identities** "
                 f"({verified.get('overlay', 0)} overlay, {verified.get('resident', 0)} resident). "
                 "Both whole-game denominators remain unknown. Counts and queue provenance are in "
                 "[the generated accounting checkpoint](re/function_accounting_checkpoint.json).")
    return text.split(BEGIN)[0] + BEGIN + "\n" + paragraph + "\n" + END + text.split(END)[1]


def refresh_dashboard(snapshot, queue, classification, checkpoint):
    """Refresh local accounting fields; preserve separately observed replay data."""
    result = json.loads(json.dumps(snapshot))
    aliases = aliases_for(queue)
    features = {row["id"]: row for row in classification}
    for row in result["queue"]:
        if row["id"] not in aliases:
            continue
        original = aliases[row["id"]]
        row["classification"] = original["classification"]
        row["identity_verified"] = original["game_or_library"] == "game_verified"
        row["full_c"] = bool(original["strict_full_c"])
        row["queue_state"] = "reviewed_full_c" if row["full_c"] else row["queue_state"]
        if row["id"] in features:
            source = features[row["id"]]
            row["confidence"] = source["confidence"]
            row["verified_calls"] = int(source["ver_calls"])
    result["strict_full_c_checkpoint"] = dict(count=checkpoint["strict_full_c_originals"],
        meaning=f"independently reviewed through PR #{checkpoint['reviewed_change']['review_pr']}; original-address credit",
        source="docs/re/function_accounting_checkpoint.json")
    result["function_accounting_checkpoint"] = checkpoint
    result["summary"]["overlay_verified_identities"] = checkpoint["verified_game_by_domain"].get("overlay", 0)
    result["summary"]["resident_verified_identities"] = checkpoint["verified_game_by_domain"].get("resident", 0)
    result["summary"]["overlay_unverified_candidates"] = (result["summary"]["overlay_candidates"] -
                                                           result["summary"]["overlay_verified_identities"])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("approval", "baseline-queue", "exe", "ovr", "out-dir"):
        parser.add_argument("--" + flag, required=True)
    parser.add_argument("--apply", action="store_true", help="publish only after regeneration, tests and hygiene")
    parser.add_argument("--dashboard-snapshot")
    parser.add_argument("--dashboard-html")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    out = Path(args.out_dir).resolve()
    require(out.is_relative_to(root / "resources"), "generated products must stay under ignored resources")
    require(bool(args.dashboard_snapshot) == bool(args.dashboard_html), "both dashboard paths are required")
    credit_path = root / "docs/re/strict_full_c_addresses.csv"
    identity_path = root / "docs/re/reviewed_function_identities.csv"
    checkpoint_path = root / "docs/re/function_accounting_checkpoint.json"
    baseline_content = Path(args.baseline_queue).read_bytes()
    baseline = json.loads(baseline_content)
    require(baseline["inputs"] == DIGESTS, "unsupported baseline build")
    require(digest(credit_path.read_bytes()) == baseline["artifacts_sha256"]["full_c_map"]
            and digest((root / "docs/re/overlay_classification.csv").read_bytes()) ==
            baseline["artifacts_sha256"]["classification"], "baseline source manifests are stale")
    if checkpoint_path.exists():
        require(json.loads(checkpoint_path.read_text())["queue_sha256"] == digest(baseline_content),
                "baseline queue differs from published checkpoint")
    credits = list(csv.DictReader(credit_path.read_text().splitlines()))
    identities = list(csv.DictReader(identity_path.read_text().splitlines()))
    source_verified = BASE_VERIFIED | load_reviewed_identities(identity_path)
    require({f"OVL_{p:04X}_{r:04X}" for p, r in source_verified} ==
            {r["id"] for r in baseline["rows"] if r["domain"] == "overlay" and r["game_or_library"] == "game_verified"},
            "classifier identity set differs from baseline queue")
    plan = plan_change(baseline, credits, identities, json.loads(Path(args.approval).read_text()))
    approval = plan["approval"]
    doc = root / approval["evidence_doc"]
    require(doc.is_file() and approval["body_sha256"] in doc.read_text(), "evidence note lacks approved body hash")
    subprocess.run(["git", "merge-base", "--is-ancestor", approval["reviewed_commit"], "HEAD"], cwd=root, check=True)
    for path, key in ((Path(args.exe), "start_exe_sha256"), (Path(args.ovr), "game_ovr_sha256")):
        require(digest(path.read_bytes()) == DIGESTS[key], "owner-media hash mismatch")
    # Derive the original file span from the hash-locked inventory, not a
    # user-provided file offset. The documented body hash binds the review.
    from overlay_inventory import find_units, parse_mz
    exe, ovr = Path(args.exe).read_bytes(), Path(args.ovr).read_bytes()
    inventory_units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    para, rel = (int(x, 16) for x in plan["target"]["original_address"].split(":"))
    unit = next(u for u in inventory_units if u["stub_para"] == para)
    require(rel + approval["body_bytes"] <= unit["code_size"], "approved span exceeds unit code")
    start = unit["ovr_offset"] + rel
    require(digest(ovr[start:start + approval["body_bytes"]]) == approval["body_sha256"], "approved original-body hash differs")
    out.mkdir(parents=True, exist_ok=True)
    (out / "strict.csv").write_bytes(csv_bytes(plan["credits"], ("id", "evidence_doc")))
    (out / "identities.csv").write_bytes(csv_bytes(plan["identities"], IDENTITY_FIELDS))
    previous = make_checkpoint(baseline, baseline_content)
    (out / "previous_checkpoint.json").write_text(json.dumps(previous, indent=2, sort_keys=True) + "\n")
    def run(script, *arguments):
        result = subprocess.run([sys.executable, str(root / "tools/overlay" / script), *map(str, arguments)],
                                cwd=root, capture_output=True, text=True)
        (out / (script + ".log")).write_text(result.stdout + result.stderr)
        require(result.returncode == 0, f"stage failed: {script}; inspect ignored stage log")
    regenerated = out / "regenerated"
    run("function_queue_audit.py", "--exe", Path(args.exe).resolve(), "--ovr", Path(args.ovr).resolve(),
        "--out-dir", regenerated, "--full-c-map", out / "strict.csv",
        "--reviewed-identities", out / "identities.csv", "--refresh-classification")
    queue_content = (regenerated / "queue.json").read_bytes()
    queue = json.loads(queue_content)
    strict, verified = verify_transition(baseline, queue, plan)
    audit_aliases(queue, json.loads((regenerated / "inventory.json").read_text()),
                  json.loads((regenerated / "resident.json").read_text()))
    checkpoint = make_checkpoint(queue, queue_content, approval)
    (out / "checkpoint.json").write_text(json.dumps(checkpoint, indent=2, sort_keys=True) + "\n")
    card_path = root / "docs/BATTLE_CARD.md"
    card = render_battle_card(card_path.read_text(), checkpoint)
    if args.apply:
        tests = subprocess.run([sys.executable, "-m", "unittest", "tools/overlay/test_function_queue.py",
            "tools/overlay/test_classify_overlay.py", "tools/overlay/test_function_alias_audit.py",
            "tools/overlay/test_function_accounting_refresh.py"], cwd=root, capture_output=True, text=True)
        (out / "tests.log").write_text(tests.stdout + tests.stderr)
        require(tests.returncode == 0, "synthetic tests failed; published accounting unchanged")
        checks = (["git", "diff", "--check"], ["bash", "-n", "tools/oracle/prepare_run.sh", "tools/oracle/launch.sh"])
        for command in checks:
            subprocess.run(command, cwd=root, check=True)
        subprocess.run(["sha256sum", "-c", "SHA256SUMS"], cwd=root / "docs/prior-research/macmaps",
                       check=True, stdout=subprocess.DEVNULL)
        tracked = subprocess.check_output(["git", "ls-files"], cwd=root, text=True).splitlines()
        require(not any(re.search(r"^resources/|(^|/)(\.DS_Store|\._)|\.(DAX|EXE|OVR|SAV|ITM|SPC|ZIP|ADF|D64|PRG|SID)$", p, re.I)
                        for p in tracked), "tracked media hygiene failed")
        dashboard_writes = []
        if args.dashboard_snapshot:
            snapshot_path, html_path = Path(args.dashboard_snapshot), Path(args.dashboard_html)
            snapshot = refresh_dashboard(json.loads(snapshot_path.read_text()), queue,
                list(csv.DictReader((regenerated / "classification.csv").read_text().splitlines())), checkpoint)
            html = html_path.read_text()
            pattern = r'(<script id="data" type="application/json">)(.*?)(</script>)'
            require(len(re.findall(pattern, html, re.S)) == 1, "dashboard embedded-data block is ambiguous")
            html = re.sub(pattern, lambda m: m[1] + json.dumps(snapshot).replace("</", "<\\/") + m[3], html, flags=re.S)
            dashboard_writes = [(snapshot_path, (json.dumps(snapshot, indent=2, sort_keys=True) + "\n").encode()),
                                (html_path, html.encode())]
        writes = [(credit_path, (out / "strict.csv").read_bytes()),
                  (identity_path, (out / "identities.csv").read_bytes()),
                  (root / "docs/re/overlay_classification.csv", (regenerated / "classification.csv").read_bytes()),
                  (root / "docs/re/overlay_classification_summary.md", (regenerated / "classification.md").read_bytes()),
                  (checkpoint_path, (out / "checkpoint.json").read_bytes()), (card_path, card.encode())] + dashboard_writes
        for path, content in writes:
            temporary = path.with_name(path.name + ".accounting-tmp")
            temporary.write_bytes(content)
            temporary.replace(path)
    print(json.dumps(dict(applied=args.apply, strict_full_c_originals=strict, verified_game_identities=verified,
                          queue_sha256=checkpoint["queue_sha256"], whole_game_denominator="unknown"), sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, subprocess.CalledProcessError) as error:
        raise SystemExit(f"accounting refresh refused: {error}")
