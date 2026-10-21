#!/usr/bin/env python3
"""Full-closure price of queue rows: every unready callee, transitively.

`price_candidates.py` prints one-hop figures; this walks the whole closure so a row
whose callee is itself a chain is priced as the chain. Overlay callees come from the
queue export (row bytes, readiness), resident callees from the loader-expanded
START.EXE (first-return span that no earlier branch jumps past, instruction-decoded
near/far calls), SYSTEM endpoints against RUNTIME_COVERED_ENDPOINTS. Strict-credited
resident bodies are terminals. Calls into the 08AE driver segment are recorded with the
resident gateway body that makes them. Far calls to any other segment (for example the
Turbo Pascal DOS unit at 0A32) are neither walked nor priced but are PRINTED with the
bodies that make them: before 2026-10-20 they were collected and never shown, which
hid 0A32:000B (Intr) from the OVL_006F_0010 breakdown. Figures are floors: bodies are counted once,
runtime_unmapped endpoints are listed and left unpriced. DECLARED boundary endpoints
(strict_candidate_queue.DECLARED_BOUNDARY_ENDPOINTS) compose as ready dependencies, so
they are terminals: listed in their own column, never walked or priced. Before
2026-10-20 the walk entered 0A40:02FD/030F and added the CRT internals behind them
(about 1,232 B) to every row that reached them.

    python3 tools/overlay/price_row_closures.py --queue resources/strict-candidate-queue/queue.json \
        --exe resources/DOS/POOLRAD/START.EXE [ROW ...]
"""
import argparse
import csv
import json
import sys
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_16

sys.path.insert(0, str(Path(__file__).resolve().parent))
import strict_candidate_queue as sq  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
OK = ("strict", "resident_covered", "runtime_covered", "declared_boundary")
DECLARED = frozenset(sq.DECLARED_BOUNDARY_ENDPOINTS)
RESIDENT_SEGMENTS = (0x464, 0x802, 0x899, 0x709, 0x3F1, 0xA40)
DEFAULT_ROWS = ("OVL_0025_1684 OVL_006F_0482 OVL_0077_299C OVL_006F_0010 OVL_006F_22D5 OVL_0025_1CC3 "
                "OVL_006F_1262 OVL_0025_0511 OVL_03CE_01C8 OVL_0025_0411").split()


class Pricer:
    def __init__(self, queue, exe):
        self.rows = {r["id"]: r for r in queue["rows"]}
        self.image = bytes(unpack(exe)[0])
        self.md = Cs(CS_ARCH_X86, CS_MODE_16)
        self.strict = {r["id"] for r in csv.DictReader(open(ROOT / "docs/re/strict_full_c_addresses.csv"))}
        self.covered = set(sq.RUNTIME_COVERED_ENDPOINTS)
        self.cache = {}

    def span(self, seg, entry):
        """First return that no earlier direct forward branch jumps past."""
        if (seg, entry) in self.cache:
            return self.cache[(seg, entry)]
        base = seg * 16
        code = self.image[base + entry:base + entry + 0x1200]
        farthest, ins, result = entry, [], (None, [])
        for i in self.md.disasm(code, entry):
            ins.append(i)
            if (i.mnemonic.startswith("j") or i.mnemonic == "loop") and i.op_str.startswith("0x"):
                target = int(i.op_str, 16)
                if target >= entry:
                    farthest = max(farthest, target)
            if i.mnemonic in ("ret", "retf") and i.address + i.size > farthest:
                result = (i.address + i.size - entry, ins)
                break
        self.cache[(seg, entry)] = result
        return result

    def resident(self, seg, offset, acc):
        rid = "RES_%04X_%04X" % (seg, offset)
        if rid in acc["bodies"] or rid in acc["strict"]:
            return
        if rid in DECLARED:
            acc["declared"].add(rid)
            return
        if rid in self.strict:
            acc["strict"].add(rid)
            return
        size, ins = self.span(seg, offset)
        acc["bodies"][rid] = size or 0
        if size is None:
            acc["bad"].add(rid)
            return
        for i in ins:
            if i.mnemonic == "call":
                try:
                    self.resident(seg, int(i.op_str, 16) & 0xFFFF, acc)
                except ValueError:
                    pass
            elif i.mnemonic == "lcall":
                try:
                    far_seg, far_off = (int(x, 16) for x in i.op_str.split(", "))
                except ValueError:
                    continue
                if far_seg == 0xAF8:
                    if "RES_%04X_%04X" % (far_seg, far_off) in DECLARED:
                        acc["declared"].add("RES_%04X_%04X" % (far_seg, far_off))
                    elif (far_seg, far_off) not in self.covered:
                        acc["unmapped"].add(far_off)
                elif far_seg in RESIDENT_SEGMENTS:
                    self.resident(far_seg, far_off, acc)
                elif far_seg == 0x8AE:
                    acc["driver"].setdefault(far_off, set()).add(rid)
                else:
                    acc["other"].setdefault((far_seg, far_off), set()).add(rid)

    def closure(self, target_id, acc):
        if target_id in acc["bodies"] or target_id in acc["strict"]:
            return
        if target_id.startswith("RES_"):
            self.resident(int(target_id[4:8], 16), int(target_id[9:13], 16), acc)
            return
        row = self.rows.get(target_id)
        if row is None:
            acc["bad"].add(target_id)
            return
        acc["bodies"][target_id] = row["bytes"]
        self.expand(row, acc)

    def expand(self, row, acc):
        for t in row["callee_readiness"]["targets"]:
            if t["readiness"] == "declared_boundary":
                acc["declared"].add(t.get("id") or "%04X:%04X" % (t.get("seg") or 0, t.get("offset") or 0))
                continue
            if t["readiness"] in OK:
                continue
            if t["readiness"] == "indirect_unresolved":
                acc["dispatch"].add(row["id"])
            elif t["kind"] == "runtime":
                acc["unmapped"].add(t["offset"])
            elif t.get("id"):
                self.closure(t["id"], acc)

    def price(self, row_id):
        acc = dict(bodies={}, strict=set(), unmapped=set(), driver={}, other={}, bad=set(), dispatch=set(),
                   declared=set())
        row = self.rows[row_id]
        acc["bodies"][row_id] = row["bytes"]
        self.expand(row, acc)
        return acc


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--queue", type=Path, required=True)
    p.add_argument("--exe", type=Path, required=True)
    p.add_argument("rows", nargs="*")
    a = p.parse_args()
    pricer = Pricer(json.load(open(a.queue)), a.exe.read_bytes())
    print("| Row | Closure (B) | Bodies | Uncovered SYSTEM endpoints | Declared boundaries (terminals) "
          "| 08AE entries (gateway bodies) | Other far targets (calling bodies) | Dispatch |")
    print("| --- | ---: | ---: | ---: | --- | --- | --- | --- |")
    for row_id in a.rows or DEFAULT_ROWS:
        if row_id not in pricer.rows:
            print("| %s | not in queue (credited or removed) | | | | | | |" % row_id)
            continue
        acc = pricer.price(row_id)
        driver = "; ".join("08AE:%04X <- %s" % (o, ", ".join(sorted(g))) for o, g in sorted(acc["driver"].items())) or "none"
        other = "; ".join("%04X:%04X <- %s" % (s, o, ", ".join(sorted(g)))
                          for (s, o), g in sorted(acc["other"].items())) or "none"
        print("| %s | %d | %d | %d | %s | %s | %s | %s |" % (row_id, sum(acc["bodies"].values()), len(acc["bodies"]),
              len(acc["unmapped"]), ", ".join(sorted(acc["declared"])) or "none", driver, other,
              ", ".join(sorted(acc["dispatch"])) or "none"))


if __name__ == "__main__":
    main()
