#!/usr/bin/env python3
"""Price non-strict candidates by cost per unlocked row, one hop deep.

Kept in the repository on purpose: the numbers this prints are quoted in
`STRICT_CANDIDATE_QUEUE.md`, and the VP's rule is *quote the artifact, publish
the method, keep the script next to the result* — a wrong price should be a
ten-minute re-run, not an argument. This script is that re-run.

Method, and the two errors that produced it:

1. **Ready means strict translation, not identity.** Callee readiness comes from
   `docs/re/strict_full_c_addresses.csv` plus runtime-covered endpoints and
   manifest-`full` residents. It is NOT `overlay_classification.csv ==
   game_verified`: a body can have settled identity and no C (e.g.
   `OVL_0025_0C77`), and treating identity as readiness priced
   `OVL_001C_13D7` at 94 B/row when it is 1,714.

2. **The callee list comes from the queue, not from our own disassembly.** The
   queue's `callee_readiness.targets` is fixup-verified; a linear sweep misses
   calls. Decode-only expansion priced `OVL_001C_2F2D` at 580 B with zero
   unready callees while the queue lists 49 targets for it.

Cost = the candidate's own span decoded from the media to the first `RETF`,
plus the decoded spans of its one-hop unready callees. Gain = the number of
candidate rows for which this id is the only remaining blocker. Nothing is
credited, promoted or written: this is a read-only pricing pass.

Usage: python3 tools/overlay/price_candidates.py [--limit-bytes 600] [--top 15]
"""
import argparse
import subprocess
import collections
import csv
import json
import sys
from pathlib import Path

import check_checker_builds as gate
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_QUEUE = ROOT / "resources/strict-candidate-queue/queue.json"
ACCOUNTING = ROOT / "resources/function-accounting-current"
MEDIA = ROOT / "resources/DOS/POOLRAD"
INPUT_EXIT = 4

# Readiness states that leave a callee unusable as a dependency.
UNREADY = {"untranslated", "resident_unmapped", "resident_callback",
           "runtime_unmapped", "partial", "modeled_blocked", "verified",
           "interface_caveat", "resident_interface_caveat"}


def media(root):
    # Imported here, not at module scope: a checkout without Capstone must reach
    # main()'s ENVIRONMENT MISSING message rather than dying on import.
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    base = root / "resources/DOS/POOLRAD"
    exe = (base / "START.EXE").read_bytes()
    ovr = (base / "GAME.OVR").read_bytes()
    image = bytes(unpack(exe)[0])
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    return image, ovr, {u["stub_para"]: u for u in units}, Cs(CS_ARCH_X86, CS_MODE_16)


def span(md, blob, origin, cap=8192):
    """Bytes from `origin` to the end of the first RET/RETF, or None."""
    at = steps = 0
    while at < len(blob) - 1 and steps < 4096:
        ins = list(md.disasm(blob[at:at + 16], origin + at))
        if not ins:
            at += 1
            continue
        step = ins[0]
        steps += 1
        if step.mnemonic in ("retf", "ret"):
            return at + step.size
        at += step.size
    return None


def span_overlay(md, ovr, units, stub, off):
    unit = units.get(stub)
    if unit is None:
        return None
    return span(md, ovr[unit["ovr_offset"] + off:unit["ovr_offset"] + off + 8192], off)


def span_resident(md, image, seg, off):
    at = ((seg << 4) + off) & 0xFFFFF
    return span(md, image[at:at + 8192], off)


def sole_blockers(strict_rows):
    """Rows for which exactly one id is the only remaining blocker."""
    sole = collections.Counter()
    for row in strict_rows:
        readiness = row["callee_readiness"]
        unready = {t.get("id") for t in readiness["targets"]
                   if t["readiness"] in UNREADY and t.get("id")}
        if readiness["counts"].get("indirect_unresolved"):
            unready.add("INDIRECT")
        if readiness["raw_cpu_caveats"]:
            unready.add("CAVEAT")
        if len(unready) == 1:
            sole[next(iter(unready))] += 1
    return sole


def price(root, queue_path, limit_bytes=600, top=15):
    image, ovr, units, md = media(root)
    rows = json.loads(Path(queue_path).read_text())["rows"]
    sole = sole_blockers(rows)
    priced = []
    for row in rows:
        rid = row["id"]
        nominal = row.get("bytes") or row.get("decoded_bytes") or 0
        if not nominal or nominal > limit_bytes:
            continue
        readiness = row["callee_readiness"]
        # An unresolved indirect dispatch cannot be bounded at one hop.
        if readiness["counts"].get("indirect_unresolved"):
            continue
        gain = sole.get(rid, 0)
        if not gain:
            continue
        if rid.startswith("OVL_"):
            _, stub, off = rid.split("_")
            own = span_overlay(md, ovr, units, int(stub, 16), int(off, 16))
        elif rid.startswith("RES_"):
            _, seg, off = rid.split("_")
            own = span_resident(md, image, int(seg, 16), int(off, 16))
        else:
            continue
        if not own:
            continue
        cost = own
        unknown = False
        for target in readiness["targets"]:
            if target["readiness"] not in UNREADY:
                continue
            tid = target.get("id")
            if not tid:
                continue
            if tid.startswith("OVL_"):
                _, stub, off = tid.split("_")
                hop = span_overlay(md, ovr, units, int(stub, 16), int(off, 16))
            elif tid.startswith("RES_"):
                _, seg, off = tid.split("_")
                hop = span_resident(md, image, int(seg, 16), int(off, 16))
            else:
                hop = None
            if hop is None:
                unknown = True
            else:
                cost += hop
        priced.append(dict(id=rid, span=own, cost=cost, gain=gain, unknown=unknown))
    priced.sort(key=lambda row: row["cost"] / row["gain"])
    return priced[:top]


def input_problem(queue_path, root=ROOT):
    """Why the queue input is missing, and exactly what produces it.

    The queue JSON is gitignored (2.7 MB, owner-media dependent), so a fresh
    checkout does not have it. An environment error must not read as a tree
    failure and must not read as "read the source to find out" either, so the
    message names the two commands and the media requirement.
    """
    if Path(queue_path).is_file():
        return None
    accounting = root / "resources/function-accounting-current"
    media_dir = root / "resources/DOS/POOLRAD"
    products = [accounting / name for name in
                ("inventory.json", "callgraph.json", "near.json")]
    have_products = all(path.is_file() for path in products)
    have_media = ((media_dir / "START.EXE").is_file()
                  and (media_dir / "GAME.OVR").is_file())
    if have_products and have_media:
        return "generate"
    steps = [
        f"QUEUE INPUT MISSING: {queue_path} is not present - this is NOT a tree failure.",
        "That file is gitignored and derived, never committed.",
        "To create it (requires the owner's media under resources/DOS/POOLRAD):",
        "  python3 tools/overlay/function_queue_audit.py --exe resources/DOS/POOLRAD/START.EXE \\",
        "      --ovr resources/DOS/POOLRAD/GAME.OVR --out-dir resources/function-accounting-current \\",
        "      --refresh-classification",
        "  python3 tools/overlay/strict_candidate_queue.py \\",
        "      --inventory resources/function-accounting-current/inventory.json \\",
        "      --callgraph resources/function-accounting-current/callgraph.json \\",
        "      --near-calls resources/function-accounting-current/near.json \\",
        f"      --ovr resources/DOS/POOLRAD/GAME.OVR --repo . --json {queue_path}",
        "Without the owner's media there is nothing to price; use a checkout that has it.",
    ]
    return "\n".join(steps)


def generate_queue(queue_path, root=ROOT):
    """Build the queue input from existing products; returns the completed path."""
    queue_path = Path(queue_path)
    queue_path.parent.mkdir(parents=True, exist_ok=True)
    accounting = root / "resources/function-accounting-current"
    command = [sys.executable, str(root / "tools/overlay/strict_candidate_queue.py"),
               "--inventory", str(accounting / "inventory.json"),
               "--callgraph", str(accounting / "callgraph.json"),
               "--near-calls", str(accounting / "near.json"),
               "--ovr", str(root / "resources/DOS/POOLRAD/GAME.OVR"),
               "--repo", str(root), "--json", str(queue_path)]
    print(f"generating the queue input (missing): {queue_path}")
    done = subprocess.run(command, capture_output=True, text=True)
    if done.returncode:
        print((done.stderr or done.stdout).strip()[-400:])
        raise SystemExit(INPUT_EXIT)
    return queue_path


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--limit-bytes", type=int, default=600)
    parser.add_argument("--top", type=int, default=15)
    parser.add_argument("--queue", type=Path, default=DEFAULT_QUEUE,
                        help="derived queue JSON (auto-generated when its inputs exist)")
    args = parser.parse_args(argv)
    missing = gate.missing_dependencies()
    if missing:
        print(gate.environment_message(missing))
        return gate.ENVIRONMENT_EXIT
    problem = input_problem(args.queue, args.root)
    if problem == "generate":
        args.queue = generate_queue(args.queue, args.root)
    elif problem:
        print(problem)
        return INPUT_EXIT
    priced = price(args.root, args.queue, args.limit_bytes, args.top)
    print(f"{'id':<16}{'span':>6}{'cost':>8}{'rows':>5}{'B/row':>8}")
    for row in priced:
        flag = " +unknown" if row["unknown"] else ""
        print(f"{row['id']:<16}{row['span']:>6}{row['cost']:>8}{row['gain']:>5}"
              f"{row['cost'] / row['gain']:>8.0f}{flag}")
    if not priced:
        print("no candidate priced under the limit")
    return 0


if __name__ == "__main__":
    sys.exit(main())
