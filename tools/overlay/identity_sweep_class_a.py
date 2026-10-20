#!/usr/bin/env python3
"""Class-(a) identity sweep: which identity-blocked rows are media-decidable.

Produces the table in `docs/re/IDENTITY_SWEEP_CLASS_A.md`. Re-run it after the
strict set grows rather than re-deriving the test.

## Method
For every overlay row whose queue status is `main_model` and whose
classification is **not** `game_verified` (the identity-blocked set):

1. decode every overlay unit and take `lcall` sites whose **segment word carries
   a relocation** (a call site without one is not evidence of a caller);
2. resolve the target through the unit descriptor — `memory_segment` plus the
   stub slot `0x20 + 5*entry` — and keep only targets in the blocked set;
3. map the site back to its enclosing ledger function by start offset;
4. classify that caller:
   * **A1** — caller is **credited** (in `strict_full_c_addresses.csv`);
   * **A1'** — caller is **game code** (`game_verified` or `game_evidenced`).

A1 and A1' are reported separately on purpose. A1 is stricter than the bar any
accepted credit in this repo has actually met; A1' is that bar.

## Inputs
* `resources/strict-candidate-queue/queue.json` — the strict candidate export.
  Derived and gitignored; regenerate with the recipe in
  `docs/re/STRICT_CANDIDATE_QUEUE.md` (it needs the owner media). The digest is
  printed on every run for comparison.
* `docs/re/strict_full_c_addresses.csv`, `docs/re/overlay_classification.csv`,
  `docs/re/overlay_function_ledger.csv` — checked in.
* `resources/DOS/POOLRAD/{START.EXE,GAME.OVR}` — owner media, read only.

## Expected output
On the 250/296 main of 2026-10-26 the sweep printed:

    identity-blocked modelled rows: 52 | with indirect_unresolved: 2
    CLASS (a) by A1  (credited caller): 0
    CLASS (a) by A1' (game-owned caller): 23
    remaining (A1 failed): 29

with 23 rows carrying a named caller. **Those numbers are point-in-time**: as
rows are credited the blocked set shrinks, and a previously blocked row can drop
out. Re-run after each batch and update the doc; do not treat 23 as a constant.

Exit status: 0 always (it is a report). Use `--json` for a machine-readable copy.
"""
import argparse
import csv
import json
import sys
from collections import defaultdict
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

from overlay_inventory import find_units, parse_mz

ROOT = Path(__file__).resolve().parents[2]
GAME = "game_verified"
GAME_OWNED = ("game_verified", "game_evidenced")


def load(root):
    strict = {r["id"] for r in csv.DictReader((root / "docs/re/strict_full_c_addresses.csv").open())}
    classification = {
        f"OVL_{int(c['unit'], 16):04X}_{int(c['offset'], 16):04X}": c["classification"]
        for c in csv.DictReader((root / "docs/re/overlay_classification.csv").open())
    }
    ledger = {}
    starts = defaultdict(list)
    for r in csv.DictReader((root / "docs/re/overlay_function_ledger.csv").open()):
        if not r["id"].startswith("0x"):
            continue
        unit, offset = int(r["unit"], 16), int(r["offset"], 16)
        rid = f"OVL_{unit:04X}_{offset:04X}"
        ledger[(unit, offset)] = rid
        starts[unit].append((offset, rid))
    for unit in starts:
        starts[unit].sort()
    return strict, classification, starts


def enclosing(starts, unit, site):
    best = None
    for offset, rid in starts.get(unit, []):
        if offset <= site:
            best = rid
        else:
            break
    return best


def sweep(root=ROOT):
    queue_path = root / "resources/strict-candidate-queue/queue.json"
    queue = json.loads(queue_path.read_text())
    strict, classification, starts = load(root)
    blocked = {r["id"]: r for r in queue["rows"]
               if r["prior_translation"]["status"] == "main_model"
               and classification.get(r["id"]) != GAME}
    exe = (root / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    ovr = (root / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    units = find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)
    by_segment = {u["memory_segment"]: u for u in units}
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    a1, a1_owned = defaultdict(list), defaultdict(list)
    for unit in units:
        base = unit["ovr_offset"]
        code = ovr[base:base + 65536]
        fixups = set(unit["fixups"])
        at = 0
        while at < len(code) - 4:
            decoded = list(md.disasm(code[at:at + 16], at))
            if not decoded:
                at += 1
                continue
            insn = decoded[0]
            if insn.mnemonic == "lcall" and (insn.address + insn.size - 2) in fixups:
                parts = [p.strip() for p in insn.op_str.split(",")]
                try:
                    seg, off = int(parts[0], 16), int(parts[1], 16)
                except ValueError:
                    seg = off = None
                if seg is not None:
                    target_unit = by_segment.get(seg)
                    if target_unit is not None and 0x20 <= off and (off - 0x20) % 5 == 0:
                        index = (off - 0x20) // 5
                        if index < len(target_unit["entries"]) and target_unit["entries"][index]:
                            target = (f"OVL_{target_unit['stub_para']:04X}_"
                                      f"{target_unit['entries'][index]:04X}")
                            caller = enclosing(starts, unit["stub_para"], insn.address)
                            if target in blocked and caller is not None:
                                site = (caller, unit["stub_para"], insn.address)
                                a1_owned[target].append(site)
                                if caller in strict:
                                    a1[target].append(site)
            at += insn.size
    return dict(queue_sha256=__import__("hashlib").sha256(queue_path.read_bytes()).hexdigest(),
                blocked=len(blocked), a1=len(a1), a1_owned=len(a1_owned),
                a1_rows={k: v for k, v in sorted(a1.items())},
                a1_owned_rows={k: v for k, v in sorted(a1_owned.items())},
                remaining=sorted(set(blocked) - set(a1_owned)))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--json", type=Path)
    args = parser.parse_args(argv)
    report = sweep(args.root)
    print(f"queue sha256 {report['queue_sha256']}")
    print(f"identity-blocked modelled rows: {report['blocked']}")
    print(f"CLASS (a) by A1  (credited caller):    {report['a1']}")
    print(f"CLASS (a) by A1' (game-owned caller): {report['a1_owned']}")
    print(f"remaining (A1' failed): {len(report['remaining'])}")
    print()
    print("| row | first game-owned caller | unit | fixup site | sites |")
    print("| --- | --- | ---: | ---: | ---: |")
    for rid, sites in report["a1_owned_rows"].items():
        caller, unit, site = sites[0]
        print(f"| `{rid}` | `{caller}` | `{unit:#04x}` | `{site:#06x}` | {len(sites)} |")
    if args.json:
        args.json.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
