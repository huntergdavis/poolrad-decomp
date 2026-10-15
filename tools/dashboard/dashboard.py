#!/usr/bin/env python3
"""Local, dependency-free Poolrad evidence queue and verification dashboard.

This reads the source repository but writes only inside this directory (and an
isolated /tmp build directory). It never reads original game media by default.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import subprocess
from datetime import datetime
from pathlib import Path
from zoneinfo import ZoneInfo

HERE = Path(__file__).resolve().parent
DEFAULT_REPO = HERE.parent.parent
STATE = HERE / "state"


def timestamp() -> str:
    return datetime.now(ZoneInfo("America/Los_Angeles")).isoformat(timespec="seconds")


def head(repo: Path) -> str:
    return subprocess.check_output(["git", "-C", str(repo), "rev-parse", "HEAD"], text=True).strip()


def rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def queue(repo: Path) -> tuple[list[dict], dict]:
    ledger = {r["id"]: r for r in rows(repo / "docs/re/overlay_function_ledger.csv")}
    classes = rows(repo / "docs/re/overlay_classification.csv")
    result = []
    for item in classes:
        detail = ledger.get(item["id"], {})
        identity = item["classification"] == "game_verified"
        # A ranking hint, not an owner assignment or a claim of full C status.
        score = (10 if item["grade"] == "A" else 0) + min(int(item["ver_calls"]), 10)
        score += min(int(item["strings"]), 5) + (3 if item["doc_globals"] else 0)
        result.append({
            "id": item["id"], "domain": "overlay", "classification": item["classification"],
            "confidence": item["confidence"], "grade": item["grade"],
            "identity_verified": identity, "full_c": "unlinked",
            "queue_state": "identity verified; C status unlinked" if identity else "triage candidate",
            "priority_hint": score, "strings": int(item["strings"]),
            "verified_calls": int(item["ver_calls"]),
            "decoded": int(item["decoded"]), "bound": int(item["bound"]),
            "ledger_class": detail.get("cls", ""),
        })
    resident = rows(repo / "docs/re/resident_callable_ledger.csv")
    for item in resident:
        identity = item["classification"] == "game_verified"
        score = min(int(item["overlay_far_sites"]), 20) + min(int(item["decoded_near_sites"]), 10)
        result.append({
            "id": item["id"], "domain": "resident", "classification": item["classification"],
            "confidence": item["confidence"], "grade": item["boundary"],
            "identity_verified": identity, "full_c": "unlinked",
            "queue_state": "identity verified; C status unlinked" if identity else "triage candidate",
            "priority_hint": score, "strings": 0, "verified_calls": 0,
            "decoded": 0, "bound": 0, "ledger_class": item["kind"],
        })
    result.sort(key=lambda x: (x["identity_verified"], -x["priority_hint"], x["id"]))
    summary = {
        "overlay_candidates": len(classes),
        "overlay_verified_identities": sum(r["identity_verified"] for r in result if r["domain"] == "overlay"),
        "overlay_unverified_candidates": sum(not r["identity_verified"] for r in result if r["domain"] == "overlay"),
        "resident_ledger_rows": len(resident),
        "resident_verified_identities": sum(r["identity_verified"] for r in result if r["domain"] == "resident"),
    }
    return result, summary


def mark(repo: Path, function_id: str, status: str, note: str) -> dict:
    available = {item["id"] for item in queue(repo)[0]}
    if function_id not in available:
        raise ValueError(f"unknown function id: {function_id}")
    path = STATE / "queue_marks.json"
    marks = read_json(path) or {}
    marks[function_id] = {"status": status, "note": note, "time": timestamp(),
                          "source_commit": head(repo)}
    write_json(path, marks)
    return marks[function_id]


def cited_evidence(repo: Path) -> list[dict]:
    # Only claim results actually present in published reports. Never combine
    # different runs or treat a bounded comparison as whole-frame parity.
    cases = [
        ("EGA first-view row 64", "docs/re/SLOT_A_FIRST_EGA_ROW64.md",
         r"\*\*320 indexed pixels match all 320\*\*", 320, 320,
         "one 320-pixel row at the saved first-view return"),
        ("Printer page, row 6 column 38", "docs/re/SLOT_A_ROW6_COL38_PRINTER_PAGE.md",
         r"\*\*28,160/28,160\*\* pixel indices equal", 28160, 28160,
         "one reached printer call; 320×88 diagnostic crop"),
    ]
    output = []
    for label, source, proof, matched, compared, scope in cases:
        path = repo / source
        body = path.read_text(encoding="utf-8")
        output.append({
            "label": label, "source": source, "scope": scope,
            "matched": matched if re.search(proof, body) else None,
            "compared": compared if re.search(proof, body) else None,
            "status": "reported in source" if re.search(proof, body) else "source claim changed; re-review",
            "method": "published scoped report; not rerun by dashboard",
        })
    return output


def read_json(path: Path) -> dict | None:
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else None


def write_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def verify(repo: Path) -> dict:
    STATE.mkdir(exist_ok=True)
    build = Path("/tmp/poolrad-dashboard-build")
    steps = [
        ("configure", ["cmake", "-S", str(repo), "-B", str(build), "-DCMAKE_BUILD_TYPE=Debug",
                       "-DCMAKE_C_FLAGS=-Wall -Wextra -Wpedantic -Werror"]),
        ("build", ["cmake", "--build", str(build), "-j", "4"]),
        ("ctest", ["ctest", "--test-dir", str(build), "--output-on-failure", "-j", "4"]),
    ]
    report = {"commit": head(repo), "time": timestamp(), "steps": [], "status": "failed"}
    for label, command in steps:
        run = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, check=False)
        log_path = STATE / f"{label}.log"
        log_path.write_text(run.stdout, encoding="utf-8")
        last = re.search(r"(\d+)% tests passed, (\d+) tests failed out of (\d+)", run.stdout)
        report["steps"].append({"name": label, "exit": run.returncode,
                                "log": str(log_path),
                                "tests_total": int(last.group(3)) if last else None,
                                "tests_failed": int(last.group(2)) if last else None})
        if run.returncode:
            break
    else:
        report["status"] = "passed"
    report["head_unchanged"] = report["commit"] == head(repo)
    if not report["head_unchanged"]:
        report["status"] = "stale: repo head changed during verification"
    write_json(STATE / "verification.json", report)
    return report


def compare(dos: Path, portable_c: Path, width: int, height: int, label: str) -> dict:
    if width <= 0 or height <= 0:
        raise ValueError("width and height must be positive")
    native = dos.read_bytes()
    candidate = portable_c.read_bytes()
    expected = width * height
    if len(native) != expected or len(candidate) != expected:
        raise ValueError(f"both files must contain exactly {expected} indexed pixel bytes")
    matched = sum(a == b for a, b in zip(native, candidate))
    report = {"label": label, "time": timestamp(), "scope": f"{width}×{height} indexed pixels",
              "matched": matched, "compared": expected, "mismatched": expected - matched,
              "dos_sha256": hashlib.sha256(native).hexdigest(),
              "c_sha256": hashlib.sha256(candidate).hexdigest(),
              "method": "direct local byte comparison; input files retained outside dashboard",
              "qualification": "Caller must establish same state, palette, and capture provenance separately"}
    write_json(STATE / "local_diff.json", report)
    return report


def render(repo: Path) -> Path:
    q, counts = queue(repo)
    rev = head(repo)
    marks = read_json(STATE / "queue_marks.json") or {}
    for item in q:
        annotation = marks.get(item["id"])
        if annotation:
            item["local_review"] = annotation
    verification = read_json(STATE / "verification.json")
    if verification and verification["commit"] != rev:
        verification = {**verification, "status": "stale: commit differs from current repo"}
    evidence = cited_evidence(repo)
    reports_dir = STATE / "reports"
    reports_dir.mkdir(parents=True, exist_ok=True)
    for item in evidence:
        source = repo / item["source"]
        (reports_dir / source.name).write_bytes(source.read_bytes())
    local_diff = read_json(STATE / "local_diff.json")
    data = {"generated": timestamp(), "commit": rev, "summary": counts, "queue": q,
            "verification": verification, "published_comparisons": evidence, "local_diff": local_diff}
    write_json(STATE / "snapshot.json", data)
    payload = json.dumps(data).replace("<", "\\u003c").replace(">", "\\u003e")
    page = """<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Poolrad evidence queue</title>
<style>body{font:17px/1.45 system-ui;margin:0 auto;padding:1.5rem;max-width:1100px;background:#101924;color:#f0f5f8}
h1,h2{color:#faf2bb}.cards{display:flex;flex-wrap:wrap;gap:1rem}.card{background:#203141;border:1px solid #48617a;border-radius:8px;padding:1rem;min-width:210px;flex:1}
.num{font-size:2rem;font-weight:750}small,.muted{color:#b9cbd8}table{border-collapse:collapse;width:100%}th,td{text-align:left;padding:.5rem;border-bottom:1px solid #405366}
input,select{font:inherit;background:#203141;color:white;padding:.4rem;border:1px solid #7d9ab2}a{color:#92d5ff}.warning{color:#ffda86}code{word-break:break-all}</style>
<h1>POOLRAD // evidence queue</h1><p id="meta"></p><p class="warning">Candidate priority is a sorting hint. No PM assignment or strict full-C credit is inferred per row.</p>
<div class="cards" id="cards"></div><h2>Compile and CTest</h2><div class="card" id="verification"></div>
<h2>Original DOS versus portable C</h2><p>Comparisons below have separate runs and scopes. They are not additive frame coverage.</p><div class="cards" id="comparisons"></div>
<h2>Function triage queue</h2><label>Search <input id="search" placeholder="unit, offset, class"></label> <label>Class <select id="filter"><option value="">All</option><option>game_verified</option><option>game_evidenced</option><option>helper_candidate</option><option>game_or_other_library_unresolved</option><option>unresolved</option><option>linked_crt_inferred</option></select></label>
<p id="shown"></p><table><thead><tr><th>Function</th><th>Evidence class</th><th>Queue state</th><th>Local review tag</th><th>Grade</th><th>Hint</th><th>Calls to verified</th></tr></thead><tbody id="queue"></tbody></table>
<script id="data" type="application/json">""" + payload + """</script><script>
const d=JSON.parse(document.getElementById('data').textContent);const esc=s=>String(s).replace(/[&<>\"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',"'":'&#39;'}[c]));
document.getElementById('meta').textContent=`Generated ${d.generated} | source commit ${d.commit.slice(0,12)}`;
const card=(n,t)=>`<div class="card"><div class="num">${esc(n)}</div><div>${esc(t)}</div></div>`;
document.getElementById('cards').innerHTML=card(d.summary.overlay_candidates,'overlay prologue candidates')+card(d.summary.resident_ledger_rows,'resident callable rows')+card(d.summary.overlay_verified_identities+d.summary.resident_verified_identities,'verified game identities')+card('unknown','whole-game function denominator');
const v=d.verification;document.getElementById('verification').innerHTML=v?`<b>${esc(v.status)}</b> at ${esc(v.time)} on ${esc(v.commit.slice(0,12))}<br>`+v.steps.map(s=>`${esc(s.name)}: exit ${s.exit}`+(s.tests_total!==null?`, ${s.tests_total-s.tests_failed}/${s.tests_total} CTests`:'')).join('<br>'):'Not run on this dashboard. Use <code>python3 tools/dashboard/dashboard.py verify</code>.';
let evidence=d.published_comparisons.map(x=>`<div class="card"><b>${esc(x.label)}</b><br>${esc(x.scope)}<br>${x.matched===null?'Source claim changed; re-review':`${x.matched}/${x.compared} matched`}<br><small>${esc(x.method)}</small><br><a href="${esc('state/reports/'+x.source.split('/').pop())}">source report</a></div>`);
if(d.local_diff){let x=d.local_diff;evidence.push(`<div class="card"><b>${esc(x.label)}</b><br>${esc(x.scope)}<br>${x.matched}/${x.compared} matched; ${x.mismatched} mismatched<br><small>${esc(x.method)} at ${esc(x.time)}</small></div>`)}
document.getElementById('comparisons').innerHTML=evidence.join('');
function update(){const s=document.getElementById('search').value.toLowerCase(),f=document.getElementById('filter').value;const selected=d.queue.filter(x=>(!f||x.classification===f)&&(x.id.toLowerCase().includes(s)||x.classification.includes(s)||x.domain.includes(s)));document.getElementById('shown').textContent=`Showing ${Math.min(selected.length,100)} of ${selected.length} matching rows; full C status is unlinked.`;document.getElementById('queue').innerHTML=selected.slice(0,100).map(x=>`<tr><td>${esc(x.id)}<br><small>${esc(x.domain)}</small></td><td>${esc(x.classification)}</td><td>${esc(x.queue_state)}</td><td>${x.local_review?esc(x.local_review.status+(x.local_review.source_commit!==d.commit?' (stale)':'')+(x.local_review.note?' — '+x.local_review.note:'')):'—'}</td><td>${esc(x.grade)}</td><td>${x.priority_hint}</td><td>${x.verified_calls}</td></tr>`).join('')}
document.getElementById('search').addEventListener('input',update);document.getElementById('filter').addEventListener('change',update);update();
</script></html>"""
    output = HERE / "dashboard.html"
    output.write_text(page, encoding="utf-8")
    return output


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=DEFAULT_REPO)
    sub = parser.add_subparsers(dest="action", required=True)
    sub.add_parser("render")
    sub.add_parser("verify")
    diff = sub.add_parser("compare")
    diff.add_argument("--dos", type=Path, required=True)
    diff.add_argument("--c", type=Path, required=True)
    diff.add_argument("--width", type=int, required=True)
    diff.add_argument("--height", type=int, required=True)
    diff.add_argument("--label", required=True)
    queue_mark = sub.add_parser("mark")
    queue_mark.add_argument("--id", required=True)
    queue_mark.add_argument("--status", required=True,
                            choices=["watch", "needs-evidence", "ready-for-review", "blocked", "archived"])
    queue_mark.add_argument("--note", default="")
    args = parser.parse_args()
    repo = args.repo.resolve()
    if args.action == "verify":
        print(json.dumps(verify(repo), indent=2))
    elif args.action == "compare":
        print(json.dumps(compare(args.dos, args.c, args.width, args.height, args.label), indent=2))
    elif args.action == "mark":
        print(json.dumps(mark(repo, args.id, args.status, args.note), indent=2))
    print(render(repo))


if __name__ == "__main__":
    main()
