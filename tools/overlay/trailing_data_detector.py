#!/usr/bin/env python3
"""Classify what follows the last terminator inside each queue row's declared span.

The queue row's `bytes` is supposed to be a body's extent. For some rows it is
not: the span runs past the final return and swallows bytes that are a prologue,
data, or unknown material. This tool decodes exactly
`[ovr_offset, ovr_offset+bytes)` - the rule that settled 11AC - finds the LAST
terminator inside that window, and classifies the remainder in **three buckets**.

It deliberately does NOT claim defects. A span can be correct and still fail a
linear decode (a body with embedded data diverges), so the buckets are hygiene
measurements over a point-in-time export, not a defect count. The export's
SHA-256 and the commit it was produced at are printed with the numbers, because
a label is only as fresh as its export: `OVL_006C_3291` sat in this very queue
labelled `untranslated` long after it was strict-credited, and the fix for that
class is freshness checking at every consumer rather than a better label.
"""
import argparse, hashlib, json, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MEDIA = ROOT / "resources/DOS/POOLRAD"
DEFAULT_QUEUE = ROOT / "resources/strict-candidate-queue/queue.json"
TERMINATORS = {"retf", "ret", "iret", "iretd"}
# Prologues seen in this corpus: push bp / mov bp,sp with the usual spill forms.
PROLOGUES = (b"\x55\x89\xe5", b"\x55\x8b\xec")


def _capstone():
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    return Cs(CS_ARCH_X86, CS_MODE_16)


def classify_tail(tail):
    """Bucket a trailing byte string: 'prologue', 'non-code', or 'unknown'."""
    if not tail:
        return "clean"
    if any(tail.startswith(p) for p in PROLOGUES):
        return "prologue"
    if set(tail) == {0}:
        return "non-code"                      # zero fill
    if len(tail) >= 4 and len(set(tail)) == 1:
        return "non-code"                      # constant fill
    printable = sum(1 for b in tail if 0x20 <= b <= 0x7E)
    if printable >= 0.85 * len(tail):
        return "non-code"                      # text
    return "unknown"


def analyse_blob(blob, base, md):
    """Last terminator inside the window, and the bucket of what follows it."""
    ins = list(md.disasm(blob, base))
    if not ins:
        return dict(state="no-decode", last=None, tail_bytes=len(blob), bucket="unknown")
    last_term = None
    for i in ins:
        if i.mnemonic in TERMINATORS:
            last_term = i
    if last_term is None:
        return dict(state="no-terminator", last=None, tail_bytes=len(blob), bucket="unknown")
    end = last_term.address - base + last_term.size
    tail = blob[end:]
    return dict(state="terminator", last=last_term.address, tail_bytes=len(tail),
                bucket=classify_tail(tail), tail_sha256=hashlib.sha256(tail).hexdigest() if tail else None)


def load_media():
    exe = (MEDIA / "START.EXE").read_bytes()
    ovr = (MEDIA / "GAME.OVR").read_bytes()
    from resident_loader_map import unpack
    image = bytes(unpack(exe)[0])
    return dict(exe=exe, ovr=ovr, image=image,
                exe_sha256=hashlib.sha256(exe).hexdigest(),
                ovr_sha256=hashlib.sha256(ovr).hexdigest())


def commit_of_head():
    try:
        return subprocess.run(["git", "-C", str(ROOT), "rev-parse", "HEAD"],
                              capture_output=True, text=True, check=True).stdout.strip()
    except Exception:
        return "unknown"


def scan(queue_path, media=None):
    md = _capstone()
    media = media or load_media()
    raw = Path(queue_path).read_bytes()
    rows = json.loads(raw)["rows"]
    out = []
    for row in rows:
        off = row.get("ovr_offset")
        if off is None:
            continue
        src = media["ovr"] if row["id"].startswith("OVL_") else media["image"]
        blob = src[off:off + int(row["bytes"])]
        res = analyse_blob(blob, int(row.get("offset", 0)), md)
        res.update(id=row["id"], bytes=int(row["bytes"]))
        out.append(res)
    return out, hashlib.sha256(raw).hexdigest(), len(rows)


def summarise(results):
    counts = {"clean": 0, "prologue": 0, "non-code": 0, "unknown": 0}
    for r in results:
        counts[r["bucket"]] = counts.get(r["bucket"], 0) + 1
    counts["no-terminator"] = sum(1 for r in results if r["state"] == "no-terminator")
    counts["no-decode"] = sum(1 for r in results if r["state"] == "no-decode")
    return counts


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--queue", type=Path, default=DEFAULT_QUEUE)
    p.add_argument("--json", type=Path)
    p.add_argument("--examples", type=int, default=6)
    a = p.parse_args(argv)
    if not a.queue.exists():
        print(f"QUEUE INPUT MISSING: {a.queue}\n"
              "Regenerate it with tools/overlay/function_queue_audit.py "
              "--exe resources/DOS/POOLRAD/START.EXE --ovr resources/DOS/POOLRAD/GAME.OVR "
              "--refresh-classification")
        return 4
    results, queue_sha, row_count = scan(a.queue)
    counts = summarise(results)
    media = load_media()
    print(f"export: {a.queue}")
    print(f"queue sha256: {queue_sha}")
    print(f"rows in export: {row_count}; rows with a file offset: {len(results)}")
    print(f"media: START.EXE {media['exe_sha256'][:16]} GAME.OVR {media['ovr_sha256'][:16]}")
    print(f"head commit: {commit_of_head()}")
    print()
    print(f"clean (span ends at the last terminator): {counts['clean']}")
    print(f"bucket prologue  (tail starts with 55 89 E5 / 55 8B EC): {counts['prologue']}")
    print(f"bucket non-code  (zero fill, constant fill, or >=85% printable): {counts['non-code']}")
    print(f"bucket unknown   (everything else, incl. no terminator): {counts['unknown']}")
    print(f"  of which no terminator anywhere in the span: {counts['no-terminator']}")
    print(f"  of which the span does not decode at all: {counts['no-decode']}")
    for bucket in ("prologue", "non-code", "unknown"):
        rows = [r for r in results if r["bucket"] == bucket][:a.examples]
        for r in rows:
            print(f"  {bucket:<9} {r['id']:<18} bytes={r['bytes']:<5} tail={r['tail_bytes']:<4} "
                  f"last={r['last'] if r['last'] is None else hex(r['last'])} state={r['state']}")
    if a.json:
        a.json.write_text(json.dumps(dict(queue_sha256=queue_sha, head=commit_of_head(),
                                         counts=counts, rows=results), indent=2, sort_keys=True) + "\n")
    # Not a gate: this reports hygiene, so it always succeeds when it ran.
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
